# Writing tests for swupd-client
<br/>

When writing tests for swupd-client, the swupd test library (testlib.bash) should
be used. This will facilitate the creation of test environments and test objects,
and will also ensure that validations are performed in a consistent manner accross
tests. The swupd-client uses BATS as the test framework of choice, to discover and
run tests.
<br/>
To use the test library you just need to source it in your shell
```bash
$ source testlib.bash
```
 or load it in your test script.  
```bash
load "testlib"
```
<br/>

## Quickstart

1. Source testlib.bash from your terminal, this will load all the test functions
from the library into your current shell process:
```bash
$ source testlib.bash
```

2. If necessary, create a directory to group the new test script with other test
scripts of the same theme.
```bash
$ mkdir <some_test_theme>
$ cd <some_test_theme>
```

3. Create the new test script.  
```bash
$ generate_test <test_name>
```

A new test script will be generated in the current directory.  
```bash
#!/usr/bin/env bats

load "../testlib"

global_setup() {

	# global setup

}

test_setup() {

	# create_test_environment "$TEST_NAME"
	# create_bundle -n <bundle_name> -f <file_1>,<file_2>,<file_N> "$TEST_NAME"

}

test_teardown() {

	# destroy_test_environment "$TEST_NAME"
}

global_teardown() {

	# global cleanup

}

@test "<test description>" {

	run sudo sh -c "$SWUPD <swupd_command> $SWUPD_OPTS <command_options>"

	# assert_status_is 0
	# expected_output=$(cat <<-EOM
	# 	<expected output>
	# EOM
	# )
	# assert_is_output "$expected_output"

}
```

4. The global_setup() function contains commands that are run only once per test
script, it runs before any test in the script is executed.

5. The test_setup() function contains commands that should be executed before every
test in the test script. This is usually the best place for creating test
objects that are prerequisites for all tests in the script. The test library
provides many [fixtures](#test-fixtures) to create all these prerequisites easily.
A default value for the test_setup() function is already defined in the test library,
but this is a minimal definition, only a test environment with name $TEST_NAME gets
created, so if this is all your test needs you can then just remove the test_setup()
from the test script so the pre-defined one is used, otherwise you will need to
overwrite that function with your test prerequisites.

6. The test_teardown() function contains commands that should be executed after
every test in the test script. It is the most common place for cleaning up
test resources created for and by tests.
Similarly as with the test_setup(), there is a minimal definition of the test_teardown()
function already in the test library. In this definition, the test environment $TEST_NAME
gets deleted, in most tests this is all it needs to be done as part of the cleanup, so
if this is the case you can go ahead and remove the local test_teardown definition from the
test script so the pre-defined one is used, if you have more specific cleanup needs you will
need to overwrite the function.

6. The global_teardown() function contains commands that are run only once per test
script, it runs once all tests in the scripts have finished.

7. Every test within the test file starts with the @test identifier followed
by a description of the test. Add one or more tests to the script as needed.

8. From within each test, execute the command you want to validate by using
the `run` helper.

9. Validate that the program under test performed as expected by using one
or more of the [assertions](#assertions) provided by the test library.
<br/>

### Test Principles

* Every test needs to run in its own test environment so they don't interfere with
each other.
* Every test should clean up after itself.
* Tests should be atomic and test only one thing per test.
* Tests should be independent from each other, one should not need another
one in order to work.
* Test files from the same "theme" should be grouped within the theme directory.
Tests from the same type can optionally be added to the same test file, for
example:
  * bundle-add  # A theme
    * add-single-bundle.bats
      * @test "add a bundle"
      * @test "add a bundle that has only a directory"
      * @test "add a bundle that has many files"
      * @test "add a bundle that is already installed"
      * @test "add a bundle that is invalid"
      * etc.
    * add-multiple-bundles.bats
      * @test "add multiple bundles"
      * @test "add multiple bundles, one valid, one invalid"
      * etc.
  * bundle-remove  # Another theme
  * etc.
<br/>

### Running Tests
To run a specific test script locally:  
```bash
$ bats <theme_directory>/<test_script>.bats
```  

To run all tests from a theme locally:  
```bash
$ bats <theme_directory>
```  

To run all tests locally:
```bash
$ cd swupd-client/test/functional
$ bats *
```

Alternatively, to run all tests locally you can also use make:
```bash
$ cd swupd-client/
$ make check
```

To include tests to be run in the CI system:  
In order for the CI system to run a new test the test needs to be added to the
BATS variable in the Makefile.am file
```bash
BATS = \
        test/documentation/manpages/test.bats \
        test/functional/bundleadd_v2/add-directory.bats \
        test/functional/bundleadd_v2/add-existing.bats \
        ...
```  
<br/>

### Debugging Tests

As mentioned in the [test principles](#test-principles), automated tests should
create their own test environment including all needed dependencies when the test
starts, and should clean up after themselves once the test is over. This means
the test environment will be deleted as soon as the test finishes running.
Sometimes it is necessary to debug faulty tests, and therefore a persistent test
environment is required. If you want to temporarily skip the deletion of the test
environment, you can achieve this by setting the DEBUG_TEST environmental variable
to *true*.
```bash
$ DEBUG_TEST=true bats <theme_directory>/<test_script>.bats
```
Note: Remember you will have to delete this test environment manually before running
the test again or it could have unexpected effects.

## Test Fixtures
### Test Environment
A test environment is nothing but a directory that contains the necessary file
structure that is used to emulate the existance of the following resources:
* a target file system <test_enviroment>/target-dir
* a local state directory to avoid conflicts with the system state directory
<test_enviroment>/state
* a remote system that provides content file downloads <test_enviroment>/web-dir
* the os-core bundle, since this bundle is required in every system it gets
created by default in the content download directory, and "installed" in the
target system

To create a test environment called "my_env" for a test you would run the following
command from within the test script.
```bash
# create a test environment for version 10 (default),
# with format "staging" (default)
$ create_test_environment my_env
```
Or
```bash
# create a test environment for version 20 with format 1
$ create_test_environment my_env 20 1
```
It is also possible to create a virtual environment with multiple versions, this
is handy for testing some behaviors, like doing updates or system verifications.
```bash
# create a test environment with version 10, format 2
$ create_test_environment my_env 2
# create a new version 20 based on version 10, format 2
$ create_version my_env 20 10 2
```
In occasions you may want to create a test environment that has an os-core
bundle that includes the os-release and format files tracked in the bundle's
manifest, this is particularly useful for upgrade related tests. To create
this kind of test environment you can use the *-r* (release) option.
```bash
# test environment with a more complete os-core bundle
$ create_test_environment -r my_env
```
In a similar manner is possible to create a test environment with no bundle using
the *-e* (empty) option.
```bash
# empty test environment
$ create_test_environment -e my_env
```

The test library provides the following functions for handling test environments:
* create_test_environment
* destroy_test_environment
* set_current_version: sets the current version of the system under test to a given value
* set_latest_version: sets the latest version of the server side content to a given value
<br/>

### Test Objects
Test objects are all those elements that need to be mocked up in order to automate
a test. The test library provides many functions for the creation and manipulation
of these test objects. By far, the most useful test object that can be created by
the test library are bundles. When creating a bundle using the test library, many
other required test objects are created as a side effect, all things that are
necessary for the bundle like files, directories, manifests, packs, etc.

Use the following command to create a bundle with two files called "test-bundle" in
the "my_env" test environment.
```bash
$ create_bundle -n test-bundle -f /foo/bar/test-file1,/baz/test-file2 my_env
```
By creating that bundle the following objects will also be created:
* A hashed directory (to be used for the /foo, /foo/bar and /baz directories)
* Two hashed files (test-file1 and test-file2, the content for these files is
randomly generated so hashes are different)
* A hashed tracking file for the bundle
* A bundle manifest
* A Manifest of Manifests (MoM)
* A tar file for each file, directory and manifest
* A zero pack for the bundle

Another example:
```bash
$ create_bundle -L -n another-bundle -d /some_dir -f /baz/test-file -l /test-link my_env
```
This bundle, besides having characteristics similar to the previous bundle will
have this new characteristics:
* One directory without any file (/some_dir)
* One link (test-link), this will also generate another extra file to which the
symbolic link is pointing to
* Since the -L flag was used, the bundle will not only be created in the directory
for content download, but it will also be installed in the target file system, this
is useful for tests that need a pre-installed bundle as prerequisite

Sometimes it is useful to create a bundle using an existing file in your system
instead of letting the helper functions create a random file for you. This can be
achieved by using the ':' character and specifying the file you want to use.  
For example:  
```bash
$ create_bundle -L -n test-bundle -f /foo/bar/test-file:"$WEBDIR"/10/files/"$hashed_name" my_env
```
This will create a bundle that has the file */foo/bar/test-file* in the manifest and
will refer to the file specified by you. Note that it is your responsibility to copy
that file to the appropriate directory first (*"$WEBDIR"/\<version\>/files*), to
name it properly according to its hash, and to create a tar for that file.

### Using Multiple Versions
So far we have been working with a test environment that contains only one version
of the server side content. To be able to validate other behaviors of the swupd client
like doing bundle updates, verifying files in the target system, etc. we need to have
multiple versions. This section shows how that can be accomplished using the test library.

You can create a test environment with multiple versions by using the *create_version*
function:
```bash
# create a test environment with version 10
$ create_test_environment my_env 10

# create a version 20 based on version 10
$ create_version my_env 20 10

# create a version 30
$ create_version my_env 30 20
```
Once you have multiple versions you can do all sort of things with that in your
environment, for example creating bundles in different versions, or adding updates
to an existing bundle. Updates are always created for the latest version.
```bash
$ create_test_environment my_env 10
$ create_version my_env 20 10
$ create_version my_env 30 20

# create a bundle in version 20
$ create_bundle -n test-bundle -v 20 -f /foo/test-file my_env

# create an update in version 30 for test-bundle in which
# /foo/test-file have changed from the previous version
$ update_bundle my_env test-bundle --update /foo/test-file

# create an update in version 40 for test-bundle in which
# /foo/test-file has been renamed, also add one more file
# to test-bundle
$ create_version my_env 40 30
$ update_bundle my_env test-bundle --rename /foo/test-file /foo/new-name
$ update_bundle my_env test-bundle --add /bar/another-file

# set the current version in the target system to 20
$ set_current_version my_env 20
```
You can also have different formats between versions, this is commonly known as a
*format bump*. The format refers to the way manifests are set out. You can create
a format bump in your environment by using the *bump_format* function. Format bumps
require the creation of two new versions, these versions are created automatically
with a separation of 10 between each other and between the latest existing version.
The format will be bumped by one value, so if initially it was 1, it will end up
being 2. It is important to note that most of the times when doing format bumps you
want to have an os-core bundle that includes the *os-release* and *format* files as
tracked files so swupd can handle updates correctly. So it is important you use the
*-r* flag when creating the test environment and the subsequent versions, this will
tell the test library to include those files into the os-core bundle that is created
by default.
```bash
# create an environment with version 10 and format 1
$ create_test_environment -r my_env 10 1

# create a format bump (will create 2 versions: 20 and 30 )
$ bump_format my_env

# create version 40 (this should use the new format 2)
$ create_version -r my_env 40 30 2
```

The following are some of the functions provided by the test library to create and
handle test objects:
* create_dir: creates a hashed directory
* create_file: creates a hashed file
* create_link: creates a hashed file and a hashed link pointing to the file
* create_tar: creates a tar of the specified object (manifest, full file, etc.)
* add_to_pack: adds the specified file or directory to a zero or delta pack
* create_bundle: creates a bundle
* install_bundle: copy the files from a test bundle into the target-dir so it
appears as installed
* remove_bundle: remove the files from a test bundle from the target-dir
* create_manifest: creates an empty manifest (with initial headers)
* add_to_manifest: adds the specified object (directory, file, or link) to the
manifest, and creates/updates the manifest tar
* add_dependency_to_manifest: adds another bundle as dependency in the manifest,
and creates/updates the manifest tar
* remove_from_manifest: removes an object from the manifest, and updates the
manifest tar
* update_manifest: updates the specified field of a manifest
* sign_manifest: signs the manifest using Swupd_Root.pem
* update_hashes_in_mom: after modifying manifests included in a MoM, this function
can be run to update the manifest hashes in the MoM
* get_hash_from_manifest: retrieves the hash of an object within a manifest
<br/>

## Assertions
The following assertions are included in the test library. These should be used to
perform the test validations.

*assert_status_is*  
passes if the exit status matches the provided one, fails otherwise  
Example:  
```bash
run <some_command>
assert_status_is 0
```

*assert_status_is_not*  
passes if the exit status does not match the provided one, fails otherwise  
Example:  
```bash
run <some_command>
assert_status_is_not 0
```

*assert_dir_exists*  
passes if the provided directory exists, fails otherwise  
Example:  
```bash
run <some_command>
assert_dir_exists /some/directory
```

*assert_dir_not_exists*  
passes if the provided directory does not exist, fails otherwise  
Example:  
```bash
run <some_command>
assert_dir_not_exists /some/directory
```

*assert_file_exists*  
passes if the provided file exists, fails otherwise  
Example:  
```bash
run <some_command>
assert_file_exists /some/file
```

*assert_file_not_exists*  
passes if the provided file does not exist, fails otherwise  
Example:  
```bash
run <some_command>
assert_file_not_exists /some/file
```

*assert_is_output*  
passes if the provided text matches the whole command output, fails otherwise  
**NOTE:** dots and blank lines are removed by default from the output before comparing it to the expected output,
if you want to disable this behavior use the assertion with the *--identical* option   
Examples:  
```bash
# one line strings
run <some_command>
assert_is_output "Some output"

# multi-line strings
run <some_command>
expected_output=$(cat <<-EOM
  Some text that needs
  to match exactly with
  the whole command output
EOM
)
assert_is_output "$expected_output"

# multi-line strings with dots and blank lines
run <some_command>
expected_output=$(cat <<-EOM
  Some text that has dots
  ..
  and some empty lines
  
  and you want those to be included when
  comparing the output
EOM
)
assert_is_output --identical "$expected_output"
```

*assert_is_not_output*  
passes if the provided text does not match the whole command output, fails otherwise  
Examples:  
```bash
run <some_command>
assert_is_not_output "This should not be the command output"
```

*assert_in_output*  
passes if the provided text is included in the command output (partial match), fails otherwise  
Examples:  
```bash
run <other_command>
expected_output=$(cat <<-EOM
  Some multi-line output
  some more lines,
  bla bla
EOM
)
assert_in_output "$expected_output"
```

*assert_not_in_output*  
passes if the provided text is not included in the command output (partial match), fails otherwise  
Examples:  
```bash
run <some_command>
assert_not_in_output "Error"
```

*assert_regex_is_output*  
similar to assert_is_output but this assertion receives a regular expression, passes
if the provided regular expression matches the whole command output, fails otherwise  
Examples:  
```bash
# remember to skip characters that are part of the expected
# output that match special regex characters like .*?()[] 
run <some_command>
expected_output=$(cat <<-EOM
  Some expected text that can have
  regex characters like .* in it
  \(skipping these parentheses\)\.
EOM
)
assert_regex_is_output "$expected_output"
```

*assert_regex_is_not_output*  
similar to assert_is_not_output but this assertion receives a regular expression, passes
if the provided regular expression does not match the command output, fails otherwise  
Examples:  
```bash
run <some_command>
assert_regex_is_not_output "Error .?"
```

*assert_regex_in_output*  
similar to assert_in_output but this assertion receives a regular expression, passes
if the provided regular expression is part of the command output (partial match), fails otherwise  
Examples:  
```bash
run <some_command>
assert_regex_in_output "This is part .* of the output"
```

*assert_regex_not_in_output*  
similar to assert_not_in_output but this assertion receives a regular expression, passes
if the provided regular expression is not part of the command output (partial match), fails otherwise  
Examples:  
```bash
run <some_command>
assert_regex_not_in_output "Error."
```

*assert_equal*  
passes if the two values provided are equal, fails otherwise  
Example:  
```bash
run <some_command>
assert_equal "some_value" "$my_variable"
```

*assert_not_equal*  
passes if the two values provided are not equal, fails otherwise  
Example:  
```bash
run <some_command>
assert_not_equal "$variable1" "$variable2"
```

*assert_files_equal*  
passes if the two files provided are equal, fails otherwise  
Example:  
```bash
run <some_command>
assert_files_equal foo/bar foo/baz
```

*assert_files_not_equal*  
passes if the two files provided are not equal, fails otherwise  
Example:  
```bash
run <some_command>
assert_files_not_equal foo/bar foo/baz
```

### Ignore lists
When using assertions that compare the command output to an expected output (for example
assert_is_output, assert_in_output, etc.), blank lines and lines containing only
dots are removed by default before comparing outputs.  
Besides removing empty lines and lines with dots from the output, it is also possible to
use an *ignore list* to remove other patterns that are not important for a test when using these assertions.  

There are three different ignore list files that can be used (but only one can be used at a time):
- <functional_tests_directory>/<test_theme_directory>/<test_name>.ignore-list: this file is intended
to specify an ignore-list for only one particular test.
- <functional_tests_directory>/<test_theme_directory>/ignore-list: this file is intended to
be used as an ignore-list for a group of test cases that are similar (the same theme).
- <functional_tests_directory>/ignore-list: this is a global ignore-list intended to be
used as a fallback for all tests.

To disable the use of ignore lists you can use the *--identical* option with every assertion
that checks a command output.  
Example:  
```bash
assert_is_output --identical "$expected_output"
assert_regex_not_in_output --identical "$some_other_output"
```