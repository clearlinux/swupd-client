# How to contribute

## Clear Linux OS - Software Contribution Guide

Clear Linux OS is composed of many different open source software projects and welcomes all contributors to improve the project.

Before contributing, please review and abide by the [Code of Conduct](https://01.org/blogs/2018/intel-covenant-code).

Ensure any contributions align with Clear Linux philosophies on [stateless](https://github.com/clearlinux/clear-linux-documentation/blob/master/source/guides/clear/stateless.rst) and [security](https://github.com/clearlinux/clear-linux-documentation/blob/master/source/guides/clear/security.rst).

More information on general Clear Linux OS constribution guidelines on [distribution guide](https://github.com/clearlinux/distribution/blob/master/contributing.md)

## Swupd

Swupd is open for code contributions. Nevertheless, if you are planning to propose a new feature, it's suggested that you file an issue first so we can discuss the feature idea and the proposed implementation. This way we can avoid future conflicts with other planned features.

When your code is ready, open a pull request to have your patches reviewed. Patches are only going to be merged after being reviewed and approved by at least 2 maintainers (with some exceptions for trivial patches), all functional tests passes and enough tests are added to test the proposed functionality or bug fix. Make sure your [git user.name](https://help.github.com/en/articles/setting-your-username-in-git) and [git user.email](https://help.github.com/en/articles/setting-your-commit-email-address-in-git) are set correctly.

You should also add tests to the feature or bug fix you are proposing. That's how we ensure we won't have software regressions or we won't break any functionality in the future. For more information about function tests, take a look at the [functional test documentation](https://github.com/clearlinux/swupd-client/blob/master/test/functional/README.md).

## Error handling

Always check and handle errors. And if possible prefer to have a fallback than to fail swupd execution. Never use abort()(unless on out of memory errors) or assert(), on errors swupd should always print the corresponded error and return an appropriated error code documented on swupd-error.h.

## Swupd Code Stye

Swupd code style is defined on .clang-format and checked with "make compliant". Make sure you are complying with the code style before creating a pull request.

### Comments

You are going to see different comment styles in swupd codebase. We are working on improving the quality of function comments on header files, so for new code, always comment all new exported (non static) functions you create in the header file. We use doxygen to validate our header api comments, so use a Doxygen supported standard.

 - Use /* */ for multiline or single line header comments. It's ok to use // for comments in the code
 - If needed, use '' to highlight the variable name or a value
 - If function returns a value, describe what is returned

Some examples of good comments:

```c
/**
 * @returns 1 if 'list' is longer than 'count' and 0 otherwise
 */
extern int list_longer_than(struct list *list, int count);

/*
 * Get a duplicated copy of the string using strdup().
 * Abort if there's no memory to allocate the new string.
 */
char *strdup_or_die(const char *const str);

...
/* Check if the command was a success */
if (success) {
    return NO_ERRORS; // No errors reported
}
...
```

### Function names

Always use meaningful function names, specially non-static functions. Avoid plurals and use imperative verbs. Add a prefix of the component/module to what this function refers to (usually the name of the header file). For example, bundle_sort_files() is a better name than sorts_bundle_files() or sort_files_in_bundle().

### Line Length

Avoid extremely long lines. Don't use lines longer than 80~100 characters.

### Return Codes

When writing functions that return a code consider the following guidelines:

 - swupd should always exit with a [swupd code](https://github.com/clearlinux/swupd-client/blob/master/src/swupd_exit_codes.h).
 - When a function returns a boolean state, prefer to explicitly return *true*/*false* instead of returning *0*/*1*.
 - Avoid using a *"catch all"* error in your code that would make harder debugging a failure.
 - Internally, swupd functions can return values other than a [swupd code](https://github.com/clearlinux/swupd-client/blob/master/src/swupd_exit_codes.h) as long as those values are not propagated all the way to the end user.
   For example a function could return an error defined in any of these files:
   - /usr/include/asm-generic/errno-base.h
   - /usr/include/asm-generic/errno.h
   - /usr/include/curl/curl.h

## Swupd generic library

Library on [src/lib](https://github.com/clearlinux/swupd-client/tree/master/src/lib) was created to group the implementation of
generic data types, helpers and system interaction used by swupd, but without
any swupd specific code or swupd.h dependency.
