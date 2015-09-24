#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([ecut], [1.2], [sage@sooper.com])
AM_INIT_AUTOMAKE(ecut, 1.2)

AC_CONFIG_SRCDIR([src/ecut.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

PKG_CHECK_MODULES([DEPS], [libpcre >= 8.12])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_STRNLEN
AC_CHECK_FUNCS([memset])

#AC_OUTPUT
AC_OUTPUT(Makefile src/Makefile man/Makefile)
