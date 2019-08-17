#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([greybus-bridge], [0.1], [])
AM_INIT_AUTOMAKE([subdir-objects])


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

AC_ARG_ENABLE([uart],
[  --enable-uart    Enable uart],
[case "${enableval}" in
	yes) uart=true ;
	     AC_DEFINE([HAVE_UART], [1], ["UART support"]) ;;
	no)  uart=false ;;
	*) AC_MSG_ERROR([bad value ${enableval} for --enable-uart]) ;;
esac])
AM_CONDITIONAL([UART], [test x$uart = xtrue])

AC_ARG_ENABLE([netlink],
[  --enable-netlink    Enable Netlink],
[case "${enableval}" in
	yes) netlink=true ;
	     AC_DEFINE([NETLINK], [1], ["Netlink support"]) ;;
	no)  netlink=false ;;
	*) AC_MSG_ERROR([bad value ${enableval} for --enable-netlink]) ;;
esac])
AM_CONDITIONAL([NETLINK], [test x$netlink = xtrue])

AC_ARG_ENABLE([gbsim],
[  --enable-gbsim    Enable GBSIM],
[case "${enableval}" in
	yes) gbsim=true ;
	     AC_DEFINE([GBSIM], [1], ["GBSIM support"]) ;;
	no)  gbsim=false ;;
	*) AC_MSG_ERROR([bad value ${enableval} for --enable-gbsim]) ;;
esac])
AM_CONDITIONAL([GBSIM], [test x$gbsim = xtrue])

KERNEL_VERSION=`uname -r`
HIGHER_VERSION=`echo -e "$KERNEL_VERSION\n4.9.0" | sort -g -t '.' | tail --lines=1`
AS_IF([test "$HIGHER_VERSION" = "4.9.0"], [
	AC_MSG_ERROR(["A kernel >= 4.19 is required"])
])

AC_ARG_VAR([GBDIR], ["greybus sources directory"])
AC_ARG_VAR([KERNEL_DIR], ["kernel sources directory"])
AS_IF([test "$KERNEL_DIR" = ""], [
	KERNEL_DIR="/lib/modules/$KERNEL_VERSION/build"
	AS_IF([test "$GBDIR" = ""], [
		GBDIR="$KERNEL_DIR/drivers/staging/greybus"
	])
])

AC_ARG_VAR([GBNETLINKDIR], ["greybus netlink sources directory"])
AS_IF([test "$GBNETLINKDIR" = ""], [
	AC_MSG_ERROR(["You must give the path of Greybus netlink sources"])
]])

AC_OUTPUT
