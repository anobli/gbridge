#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([greybus-bridge], [0.1], [])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.
AC_CHECK_LIB([nl-3], [nl_socket_alloc])
AC_CHECK_LIB([nl-genl-3], [genl_register_family])
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([avahi-common], [main])
AC_CHECK_LIB([avahi-client], [main])

AC_CONFIG_SRCDIR([main.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
	Makefile
])

AC_ARG_ENABLE([bluetooth],
[  --enable-bluetooth    Enable bluetooth],
[case "${enableval}" in
	yes) bluetooth=true ;
	     AC_CHECK_LIB([bluetooth], [hci_inquiry]) ;;
	no)  bluetooth=false ;;
	*) AC_MSG_ERROR([bad value ${enableval} for --enable-bluetooth]) ;;
esac])
AM_CONDITIONAL([BLUETOOTH], [test x$bluetooth = xtrue])

AC_ARG_ENABLE([tcpip],
[  --enable-tcpip    Enable TCP/IP],
[case "${enableval}" in
	yes) tcpip=true ;
	     AC_DEFINE([HAVE_TCPIP], [1], ["TCP/IP support"]) ;;
	no)  tcpip=false ;;
	*) AC_MSG_ERROR([bad value ${enableval} for --enable-tcpip]) ;;
esac])
AM_CONDITIONAL([TPCIP], [test x$tcpip = xtrue])

AC_ARG_VAR([GBDIR], ["greybus sources directory"])
AS_IF([test "$GBDIR" = ""], [
  AC_MSG_ERROR([Environment variable GBDIR needs to be set])
])

AC_OUTPUT
