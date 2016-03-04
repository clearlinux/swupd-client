#!/usr/bin/python3

# swupd functional test driver
#
# Things to test:
# update
# update --status
# update --download
# verify
# verify --fix
# verify --install
# bundle add
# bundle add --list
# bundle remove
# check-update
#
# Out of scope (for now at least):
# Files going into /boot (integration test)
#
# Current issues with not running as root:
# Use of system /var
# chown of files to root for hash comparison

import functools
import inspect
import os
import subprocess
import sys
import time
import unittest

from tap import TAPTestRunner

COMMAND_MAP = {'bundleadd': 'bundle-add',
               'bundleremove': 'bundle-remove',
               'checkupdate': 'check-update',
               'hashdump': 'hashdump',
               'update': 'update',
               'verify': 'verify'}
PATH_PREFIX = 'test/functional'
PORT = '8089'


def path_from_name(name, path_type):
    """Based on a test class name and path_type, create a path"""
    # Basic conversion looks like:
    # class name == 'subfunction' + '_' + 'test_name'
    # Paths will use '-' instead of '_'
    # So conversion will split on '_' and the first element will be the
    # subfunction test directory and the rest will be a specific test
    # folder within that subfunction
    # So the checkupdate_version_match class would use the
    # test/functional/checkupdate/version-match/ path
    # with path_type determining the base folder
    type_map = {'web': 'web-dir', 'target': 'target-dir'}
    folders = name.split('_')
    subfunction_path = folders[0]
    test_dir = ('-').join(folders[1:])
    if path_type == 'plain':
        return os.path.join(PATH_PREFIX, subfunction_path, test_dir)
    else:
        return os.path.join(PATH_PREFIX, subfunction_path, test_dir,
                            type_map[path_type])


def http_server(func):
    """Create an http server for the lifetime of a test function"""
    @functools.wraps(func)
    def wrapper(*args):
        """http_server wrapper"""
        web_path = path_from_name(args[0].__class__.__name__, 'web')
        httpd = subprocess.Popen(['python3', '-m', 'http.server', PORT],
                                 cwd=web_path,
                                 stdout=subprocess.DEVNULL,
                                 stderr=subprocess.DEVNULL)
        # Stupid but give the web server a chance to run
        time.sleep(.1)
        try:
            func(*args)
        finally:
            httpd.kill()
    return wrapper


def with_command(func):
    """Add the command variable to the lexical environment of a function"""
    @functools.wraps(func)
    def wrapper(*args):
        """with_command wrapper"""
        target_path = path_from_name(args[0].__class__.__name__, 'target')
        subcommand = COMMAND_MAP[args[0].__class__.__name__.split('_')[0]]
        command = ['sudo', './swupd', subcommand, '-u', 'http://localhost/',
                   '-P', PORT, '-p', target_path]
        func(*args, command)
    return wrapper


def http_command(**kwargs):
    """Make the standard test class method"""
    def class_wrapper(original_class):
        """Update class to add test function"""
        @http_server
        @with_command
        def runTest(*args):
            subprocess.run('sudo rm -fr /var/lib/swupd'.split())
            """Basic test wrapper function"""
            # Options are for subcommand so insert after the subcommand index.
            # Currently defaults to the -u option
            if kwargs.get('option') != "":
                i = args[1].index('-u')
                args[1][i:i] = kwargs['option'].split(' ')
            process = subprocess.run(args[1], stdout=subprocess.PIPE,
                                     universal_newlines=True)
            args[0].validate(process.stdout.splitlines())
        msg = kwargs.get('skip')
        if msg:
            original_class.runTest = unittest.skip(msg)(runTest)
        else:
            original_class.runTest = runTest
        return original_class
    return class_wrapper


@http_command(option="test-bundle")
class bundleadd_add_directory(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('Downloading test-bundle pack for version 10',
                      test_output)
        self.assertIn('Installing bundle(s) files...', test_output)
        self.assertIn('Tracking test-bundle bundle on the system', test_output)


@http_command(option="--list")
class bundleadd_list(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('os-core', test_output)


@http_command(option="test-bundle")
class bundleremove_remove_file(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('Deleting bundle files...', test_output)
        self.assertIn('Total deleted files: 1', test_output)
        self.assertIn('Untracking bundle from system...', test_output)
        self.assertIn('Success: Bundle removed', test_output)


@http_command(option="")
class checkupdate_version_match(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('There are no updates available', test_output)


@http_command(option="")
class checkupdate_new_version(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('There is a new OS version available: 100', test_output)


@http_command(option="")
class checkupdate_no_server_content(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('Cannot reach update server', test_output)


@http_command(option="")
class checkupdate_no_target_content(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('Unable to determine current OS version', test_output)


class hashdump_file_hash(unittest.TestCase):
    def runTest(self):
        target_path = path_from_name(self.__class__.__name__, 'target')
        subcommand = COMMAND_MAP[self.__class__.__name__.split('_')[0]]
        command = ['./swupd', subcommand, "--basepath={0}/{1}"
                   .format(os.getcwd(), target_path), "/test-hash"]
        process = subprocess.run(command, stdout=subprocess.PIPE,
                                 universal_newlines=True)
        test_output = process.stdout.splitlines()
        self.assertIn('b03e11e3307de4d4f3f545c8a955c2208507dbc1927e9434d1da42917712c15b', test_output)


@http_command(option="--download")
class update_download(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('    changed files     : 1', test_output)
        self.assertIn('    changed manifests : 1', test_output)


@http_command(option="--status")
class update_status(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('Current OS version: 10', test_output)
        self.assertIn('Latest server version: 100', test_output)


@http_command(option="--status")
class update_status_no_server_content(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('Current OS version: 10', test_output)
        self.assertIn('Cannot get latest the server version.Could not reach server', test_output)


@http_command(option="--status")
class update_status_no_target_content(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('Cannot determine current OS version', test_output)
        self.assertIn('Latest server version: 100', test_output)


@http_command(option="--status")
class update_status_version_single_quote(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('Current OS version: 10', test_output)
        self.assertIn('Latest server version: 100', test_output)


@http_command(option="--status")
class update_status_version_double_quote(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('Current OS version: 10', test_output)
        self.assertIn('Latest server version: 100', test_output)


@http_command(option="")
class update_use_full_file(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('    changed files     : 1', test_output)
        self.assertIn('    changed manifests : 1', test_output)
        self.assertIn('Staging file content', test_output)
        self.assertIn('Update was applied.', test_output)
        self.assertIn('Update successful. System updated from version 10 to version 100', test_output)

@http_command(option="")
class update_verify_fix_path_missing_dir(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('    changed files     : 1', test_output)
        self.assertIn('    changed manifests : 1', test_output)
        self.assertIn('Staging file content', test_output)
        target_path = os.path.join(os.getcwd(), path_from_name(__class__.__name__, 'target') , 'usr/bin')
        self.assertIn('Update target directory does not exist: ' + target_path + '. Trying to fix it', test_output)
        self.assertIn('Update was applied.', test_output)
        self.assertIn('Update successful. System updated from version 10 to version 100', test_output)


@http_command(option="")
class update_verify_fix_path_hash_mismatch(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('    changed files     : 1', test_output)
        self.assertIn('    changed manifests : 1', test_output)
        self.assertIn('Staging file content', test_output)
        target_path = os.path.join(os.getcwd(), path_from_name(__class__.__name__, 'target') , 'usr/bin')
        self.assertIn('Update target directory does not exist: ' + target_path + '. Trying to fix it', test_output)
        self.assertIn('Hash did not match for path : /usr', test_output)
        self.assertIn('Path /usr/bin is missing on the file system', test_output)
        self.assertIn('Update was applied.', test_output)
        self.assertIn('Update successful. System updated from version 10 to version 100', test_output)


@http_command(option="")
class update_use_pack(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('    changed files     : 1', test_output)
        self.assertIn('    changed manifests : 1', test_output)
        self.assertIn('Staging file content', test_output)
        self.assertIn('Update was applied.', test_output)
        self.assertIn('Update successful. System updated from version 10 to version 100', test_output)


@http_command(option="--fix")
class verify_add_missing_directory(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('Verifying version 10', test_output)
        self.assertIn('  1 files were missing', test_output)
        self.assertIn('    1 of 1 missing files were replaced', test_output)
        self.assertIn('Fix successful', test_output)


@http_command(option="")
class verify_check_missing_directory(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('Verifying version 10', test_output)
        self.assertIn('  1 files did not match', test_output)
        self.assertIn('Verify successful', test_output)


@http_command(option="--install -m 10")
class verify_install_directory(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('Verifying version 10', test_output)
        self.assertIn('  1 files were missing', test_output)
        self.assertIn('    1 of 1 missing files were replaced', test_output)
        self.assertIn('Fix successful', test_output)


def get_test_cases():
    tests = ['bundleadd', 'bundleremove', 'checkupdate', 'hashdump', 'update',
             'verify']
    test_cases = []
    class_members = inspect.getmembers(sys.modules[__name__], inspect.isclass)
    for (name, cls) in class_members:
        if name.split('_')[0] in tests:
            test_cases.append((name, cls))
    return test_cases


def run_setups(tests):
    setups = []
    for test in tests:
        test_dir = path_from_name(test[0], 'plain')
        if os.path.isfile(os.path.join(test_dir, 'setup.sh')):
            setups.append(subprocess.Popen('./setup.sh', cwd=test_dir,
                                           stdout=subprocess.DEVNULL,
                                           stderr=subprocess.DEVNULL))
    for setup in setups:
        setup.wait()
        if setup.returncode != 0:
            raise Exception('Setup failed')


def run_teardowns(tests):
    teardowns = []
    for test in tests:
        test_dir = path_from_name(test[0], 'plain')
        if os.path.isfile(os.path.join(test_dir, 'teardown.sh')):
            teardowns.append(subprocess.Popen('./teardown.sh', cwd=test_dir,
                                              stdout=subprocess.DEVNULL,
                                              stderr=subprocess.DEVNULL))
    for teardown in teardowns:
        teardown.wait()


if __name__ == '__main__':
    tests = get_test_cases()
    run_setups(tests)
    suite = unittest.TestSuite()
    for test in tests:
        suite.addTest(test[1]())
    runner = TAPTestRunner()
    runner.set_format('{method_name}')
    runner.set_stream(True)
    runner.run(suite)
    run_teardowns(tests)
