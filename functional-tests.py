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
               'search': 'search',
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
                   '-P', PORT, '-p', target_path, '-F', 'staging']
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


@http_command(option="test-bundle")
class bundleadd_add_existing(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('bundle(s) already installed, exiting now', test_output)


@http_command(option="test-bundle1 test-bundle2")
class bundleadd_add_multiple(unittest.TestCase):
    def validate(self, test_output):
        newfile = os.path.join(os.getcwd(),
                               path_from_name(__class__.__name__, 'target'),
                               'usr/foo')
        newdir = os.path.join(os.getcwd(),
                               path_from_name(__class__.__name__, 'target'),
                               'usr/bin')
        self.assertTrue(os.path.isfile(newfile))
        self.assertTrue(os.path.isdir(newdir))


@http_command(option="test-bundle")
class bundleadd_boot_file(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('Downloading test-bundle pack for version 10',
                      test_output)
        self.assertIn('Installing bundle(s) files...', test_output)
        self.assertIn('Tracking test-bundle bundle on the system', test_output)
        # bundle-add output does not list files it did or did not install,
        # so check for the file existence in the target directory.
        newfile = os.path.join(os.getcwd(),
                               path_from_name(__class__.__name__, 'target'),
                               'usr/lib/kernel/testfile')
        self.assertTrue(os.path.isfile(newfile))


@http_command(option="test-bundle")
class bundleadd_include(unittest.TestCase):
    def validate(self, test_output):
        newfile = os.path.join(os.getcwd(),
                               path_from_name(__class__.__name__, 'target'),
                               'usr/foo')
        newdir = os.path.join(os.getcwd(),
                               path_from_name(__class__.__name__, 'target'),
                               'usr/bin')
        self.assertTrue(os.path.isfile(newfile))
        self.assertTrue(os.path.isdir(newdir))


@http_command(option="--list")
class bundleadd_list(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('os-core', test_output)


@http_command(option="test-bundle")
class bundleremove_boot_file(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('Deleting bundle files...', test_output)
        self.assertIn('Total deleted files: 1', test_output)
        self.assertIn('Untracking bundle from system...', test_output)
        self.assertIn('Success: Bundle removed', test_output)
        # bundle-remove output does not list files it removed,
        # so check for the file absence in the target directory.
        newfile = os.path.join(os.getcwd(),
                               path_from_name(__class__.__name__, 'target'),
                               'usr/lib/kernel/testfile')
        self.assertFalse(os.path.isfile(newfile))


@http_command(option="test-bundle")
class bundleremove_remove_file(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('Deleting bundle files...', test_output)
        self.assertIn('Total deleted files: 1', test_output)
        self.assertIn('Untracking bundle from system...', test_output)
        self.assertIn('Success: Bundle removed', test_output)

# Positive test for a library in lib32/
@http_command(option="-l test-lib32")
class search_content_check_poslib32(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('\'test-bundle\'  :  \'/usr/lib/test-lib32\'', test_output)
        self.assertNotIn('/libtest-nohit', test_output)

# Positive test for a binary
@http_command(option="-b test-bin")
class search_content_check_posbin(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('\'test-bundle\'  :  \'/usr/bin/test-bin\'', test_output)
        self.assertNotIn('/libtest-nohit', test_output)

# Posititve test for a binary using the -e everywhere flag
@http_command(option="test-bin")
class search_content_check_posebin(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('\'test-bundle\'  :  \'/usr/bin/test-bin\'', test_output)
        self.assertNotIn('/libtest-nohit', test_output)

# Negative test for a binary using the -l library flag
@http_command(option="-l test-bin")
class search_content_check_poslbin(unittest.TestCase):
    def validate(self, test_output):
        self.assertNotIn('\'test-bundle\'  :  \'/usr/bin/test-bin\'', test_output)
        self.assertNotIn('/libtest-nohit', test_output)

# Positive test for a library in lib64/
@http_command(option="-l test-lib64")
class search_content_check_poslib64(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('\'test-bundle\'  :  \'/usr/lib64/test-lib64\'', test_output)
        self.assertNotIn('/libtest-nohit', test_output)

#Negative test for a non-existent library
@http_command(option="-l test-lib36")
class search_content_check_neglib32(unittest.TestCase):
    def validate(self, test_output):
        self.assertNotIn('\'test-bundle\'  :  \'test-lib36\'', test_output)
        self.assertNotIn('/libtest-nohit', test_output)

# Negative test for a file which exists but is not a library
@http_command(option="-l libtest-nohit")
class search_content_check_neglibtest(unittest.TestCase):
    def validate(self, test_output):
        self.assertNotIn('\'test-bundle\'  :  \'libtest-nohit\'', test_output)
        self.assertNotIn('/libtest-nohit', test_output)

# Positive test for a file, specifying full path
@http_command(option="/usr/lib64/test-lib64")
class search_content_check_posfull_path(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('\'test-bundle\'  :  \'/usr/lib64/test-lib64\'', test_output)
        self.assertNotIn('/libtest-nohit', test_output)

# Nagative test for a non-existent file, specifying full path
@http_command(option="/usr/lib64/test-lib100")
class search_content_check_negfull_path(unittest.TestCase):
    def validate(self, test_output):
        self.assertNotIn('\'test-bundle\'  :  \'/usr/lib64/test-lib100\'', test_output)
        self.assertNotIn('\'test-bundle\'  :  \'/usr/lib64/test-lib64\'', test_output)
        self.assertNotIn('/libtest-nohit', test_output)

# Negative test for a file which exists, but providing incorrect path
@http_command(option="/usr/lib/test-lib64")
class search_content_check_negwrongpath(unittest.TestCase):
    def validate(self, test_output):
        self.assertNotIn('\'test-bundle\'  :  \'/usr/lib/test-lib64\'', test_output)
        self.assertNotIn('/libtest-nohit', test_output)

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

class hashdump_file_hash_no_path_prefix(unittest.TestCase):
    def runTest(self):
        target_path = path_from_name(self.__class__.__name__, 'target')
        subcommand = COMMAND_MAP[self.__class__.__name__.split('_')[0]]
        command = ['./swupd', subcommand, "{0}/{1}/test-hash".format(os.getcwd(), target_path)]
        process = subprocess.run(command, stdout=subprocess.PIPE,
                                 universal_newlines=True)
        test_output = process.stdout.splitlines()
        self.assertIn('b03e11e3307de4d4f3f545c8a955c2208507dbc1927e9434d1da42917712c15b', test_output)

@http_command(option="")
class update_apply_full_file_delta(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('Extracting pack.', test_output)
        self.assertIn('    changed files     : 1', test_output)
        self.assertIn('    changed manifests : 1', test_output)
        self.assertIn('Staging file content', test_output)
        self.assertIn('Update was applied.', test_output)
        self.assertIn('Update successful. System updated from version 10 to version 100', test_output)


@http_command(option="")
class update_boot_file(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('    new files         : 1', test_output)
        # update must always add a new boot file, so check for the file
        # existence in the target directory.
        newfile = os.path.join(os.getcwd(),
                               path_from_name(__class__.__name__, 'target'),
                               'usr/lib/kernel/testfile')
        self.assertTrue(os.path.isfile(newfile))


@http_command(option="--download")
class update_download(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('    changed files     : 1', test_output)
        self.assertIn('    changed manifests : 1', test_output)


@http_command(option="")
class update_include(unittest.TestCase):
    def validate(self, test_output):
        core = os.path.join(os.getcwd(),
                               path_from_name(__class__.__name__, 'target'),
                               'usr/core')
        self.assertTrue(os.path.isfile(core))
        test1 = os.path.join(os.getcwd(),
                               path_from_name(__class__.__name__, 'target'),
                               'usr/bin')
        self.assertTrue(os.path.isdir(test1))
        test2 = os.path.join(os.getcwd(),
                               path_from_name(__class__.__name__, 'target'),
                               'usr/foo2')
        self.assertTrue(os.path.isfile(test2))
        test3 = os.path.join(os.getcwd(),
                               path_from_name(__class__.__name__, 'target'),
                               'usr/foo3')
        self.assertTrue(os.path.isfile(test3))
        test4 = os.path.join(os.getcwd(),
                               path_from_name(__class__.__name__, 'target'),
                               'usr/foo4')
        self.assertTrue(os.path.isfile(test4))
        test4 = os.path.join(os.getcwd(),
                               path_from_name(__class__.__name__, 'target'),
                               'usr/foo4')
        self.assertTrue(os.path.isfile(test4))
        test5 = os.path.join(os.getcwd(),
                               path_from_name(__class__.__name__, 'target'),
                               'usr/foo5')
        self.assertTrue(os.path.isfile(test5))
        test6 = os.path.join(os.getcwd(),
                               path_from_name(__class__.__name__, 'target'),
                               'usr/foo6')
        self.assertTrue(os.path.isfile(test6))
        test7 = os.path.join(os.getcwd(),
                               path_from_name(__class__.__name__, 'target'),
                               'usr/foo7')
        self.assertTrue(os.path.isfile(test7))


@http_command(option="")
class update_include_old_bundle(unittest.TestCase):
    def validate(self, test_output):
        core = os.path.join(os.getcwd(),
                            path_from_name(__class__.__name__, 'target'),
                            'os-core')
        self.assertTrue(os.path.isfile(core))
        test1 = os.path.join(os.getcwd(),
                             path_from_name(__class__.__name__, 'target'),
                             'test-bundle1')
        self.assertTrue(os.path.isfile(test1))
        test2 = os.path.join(os.getcwd(),
                             path_from_name(__class__.__name__, 'target'),
                             'test-bundle2')
        self.assertTrue(os.path.isfile(test2))


@http_command(option="")
class update_missing_os_core(unittest.TestCase):
    def validate(self, test_output):
        os_core_dir = os.path.join(os.getcwd(),
                               path_from_name(__class__.__name__, 'target'),
                               'usr/bin')
        self.assertTrue(os.path.isdir(os_core_dir))


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


@http_command(option="--fix")
class verify_add_missing_include(unittest.TestCase):
    def validate(self, test_output):
        core = os.path.join(os.getcwd(),
                            path_from_name(__class__.__name__, 'target'),
                            'os-core')
        self.assertTrue(os.path.isfile(core))
        test1 = os.path.join(os.getcwd(),
                             path_from_name(__class__.__name__, 'target'),
                             'test-bundle1')
        self.assertTrue(os.path.isfile(test1))
        test2 = os.path.join(os.getcwd(),
                             path_from_name(__class__.__name__, 'target'),
                             'test-bundle2')
        self.assertTrue(os.path.isfile(test2))
        self.assertIn('    3 of 3 missing files were replaced', test_output)


@http_command(option="--fix")
class verify_add_missing_include_old(unittest.TestCase):
    def validate(self, test_output):
        core = os.path.join(os.getcwd(),
                            path_from_name(__class__.__name__, 'target'),
                            'os-core')
        self.assertTrue(os.path.isfile(core))
        test1 = os.path.join(os.getcwd(),
                             path_from_name(__class__.__name__, 'target'),
                             'test-bundle1')
        self.assertTrue(os.path.isfile(test1))
        test2 = os.path.join(os.getcwd(),
                             path_from_name(__class__.__name__, 'target'),
                             'test-bundle2')
        self.assertTrue(os.path.isfile(test2))
        self.assertIn('    3 of 3 missing files were replaced', test_output)


@http_command(option="")
class verify_boot_file_mismatch(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('Verifying version 10', test_output)
        corruptfile = os.path.join(os.getcwd(),
                                   path_from_name(__class__.__name__, 'target'),
                                   'usr/lib/kernel/testfile')
        self.assertIn('Hash mismatch for file: ' + corruptfile, test_output)
        self.assertIn('  1 files did not match', test_output)
        self.assertIn('Verify successful', test_output)


@http_command(option="")
class verify_check_missing_directory(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('Verifying version 10', test_output)
        self.assertIn('  1 files did not match', test_output)
        self.assertIn('Verify successful', test_output)


@http_command(option="--fix")
class verify_boot_file_deleted(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('Verifying version 10', test_output)
        # Deleted boot files should be ignored completely
        self.assertNotIn('  1 files found which should be deleted', test_output)
        bootfile = os.path.join(os.getcwd(),
                                path_from_name(__class__.__name__, 'target'),
                                'usr/lib/kernel/testfile')
        self.assertTrue(os.path.isfile(bootfile))
        self.assertIn('Fix successful', test_output)


@http_command(option="--fix")
class verify_boot_file_mismatch_fix(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('Verifying version 10', test_output)
        corruptfile = os.path.join(os.getcwd(),
                                   path_from_name(__class__.__name__, 'target'),
                                   'usr/lib/kernel/testfile')
        self.assertIn('Hash mismatch for file: ' + corruptfile, test_output)
        self.assertIn('    1 of 1 files were fixed', test_output)
        self.assertIn('Fix successful', test_output)


@http_command(option="--fix")
class verify_empty_dir_deleted(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('Verifying version 10', test_output)
        testdir = os.path.join(os.getcwd(),
                               path_from_name(__class__.__name__, 'target'),
                               'testdir')
        self.assertIn('Deleted {}'.format(testdir), test_output)
        self.assertFalse(os.path.isdir(testdir))
        self.assertIn('Fix successful', test_output)


@http_command(option="--install -m 10")
class verify_install_directory(unittest.TestCase):
    def validate(self, test_output):
        self.assertIn('Verifying version 10', test_output)
        self.assertIn('  1 files were missing', test_output)
        self.assertIn('    1 of 1 missing files were replaced', test_output)
        self.assertIn('Fix successful', test_output)


def get_test_cases():
    tests = ['bundleadd', 'bundleremove', 'checkupdate', 'hashdump', 'update',
             'verify', 'search']
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
