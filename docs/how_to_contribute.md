# How to contribute

Swupd is open for code contributions. Nevertheless, if you are planning to propose a new feature, it's suggested that you file an issue first so we can discuss the feature idea and the proposed implementation. This way we can avoid future conflicts with other planned features.

When your code is ready, open a pull request to have your patches reviewed. Patches are only going to be merged after being reviewed and approved by at least 2 maintainers (with some exceptions for trivial patches), all functional tests passes and enough tests are added to test the proposed functionality or bug fix.

You should also add tests to the feature or bug fix you are proposing. That's how we ensure we won't have software regressions or we won't break any functionality in the future. For more information about function tests, take a look at the [functional test documentation](test/functional/README.md)

## Error handling

Always check and handle errors. And if possible prefer to have a fallback than to fail swupd execution. Never use abort()(unless on out of memory errors) or assert(), on errors swupd should always print the corresponded error and return an appropriated error code documented on swupd-error.h.

## Swupd Code Stye

Swupd code style is defined on .clang-format and checked with "make compliant". Make sure you are complying with the code style before creating a pull request.

### Comments

You are going to see different comment styles in swupd codebase. We are working on improving the quality of function comments on header files, so for new code, always comment all new exported (non static) functions you create in the header file. It's also well appreciated if you add comments to undocumented functions you have changed.

 - Use /* */ for multiline or single line header comments. It's ok to use // for comments in the code
 - If needed, use '' to highlight the variable name or a value
 - If function returns a value, describe what is returned

Some examples of good comments:

```c
/*
 * Returns 1 if 'list' is longer than 'count'. Returns 0 otherwise
 */
extern int list_longer_than(struct list *list, int count);

/*
 * Returns a duplicated copy of the string using strdup().
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

## Swupd generic library

Library on [src/lib](src/lib) was created to group the implementation of
generic data types, helpers and system interaction used by swupd, but without
any swupd specific code or swupd.h dependency.
