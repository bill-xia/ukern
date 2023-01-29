# Code Format

Code should conform to following rules:

1. Header files should be protected by #ifndef FILENAME_H clauses.
2. All files should be aligned by tab rather than space.
3. In constant #define clauses, value should be aligned by tab; in variable and struct definition, each variable name should be aligned by tab.
4. All error codes should be defined inside errno.h by a enum, and corresponding error message defined in errno.c.
5. All names are in lower case and divided by underscore, except for constants, which should be in upper case and divided by underscore.
6. Types may be defined by typedef clause, those type names must end with `_t`.
7. Constant names should be identifiable between multiple context, e.g. CMD is a bad name.
8. Define multiple relative constants in an `enum{}` is preferred.