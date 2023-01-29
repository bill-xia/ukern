# Coding Style

This document refers to Linux kernel coding style, and is building in progress.

## Formatting

1. Header files should be protected by #ifndef FILENAME_H clauses.
2. All files should be aligned by tab rather than space, tab size is 8.
3. In `#define` clauses, seperate `#define` and name with 1 space, name with value (if exist) with tab(s), nearby values should be vertically aligned by tab.
4. Define local variables inside function on their first usage, exceptions in 5); while defining multiple local variables at once, each variable takes 1 line, variable names should be aligned by tab.
5. Several common local variable names: `i`, `j`, `k` as enumerating variables, `r` as result in clauses like `if ((r = func()) < 0)`, `tmp` as temporary variable. These names should be defined at beginning of function.
6. Only use typedef with a clear reason, refer to Linux kernel coding style. All typedef-ed name should end with `_t`, to distinguish from variable names.

## Naming

1. Use `{i|u}{8|16|32|64}` as integer type name if size is important.
2. Global names should be descriptive, and identifiable between multiple context, e.g. CMD is a bad name.
3. Local names could be short, while not meaningless.
4. All names are in lower case and divided by underscore, except for constants, which should be in upper case and divided by underscore. As we don't use typedef heavily to redefine struct-s, all struct appears with the `struct` keyword, we can distinguish struct and variables according to this.
5. Define multiple relative constants in an `enum{}` is preferred.

## OS-specific

1. All error codes should be defined inside errno.h by a enum, and corresponding error message defined in errno.c.