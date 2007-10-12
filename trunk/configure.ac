#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([geocache], [0.1.0], [henearkrxern@gmail.com])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE()

# Checks for programs.
AC_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([sys/select.h])
AC_CHECK_HEADERS([db.h])

# Checks for library functions.
AC_CHECK_FUNCS([gethostbyname socket])

CFLAGS="-Wall -O3"
#CFLAGS="-Wall -g -dH -O0"
#CFLAGS="-Wall -pg"

AC_CONFIG_FILES([Makefile man/Makefile src/Makefile])

AC_OUTPUT
