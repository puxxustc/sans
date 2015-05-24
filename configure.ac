#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT([sans], [0.1.0], [https://github.com/XiaoxiaoPu/sans/issues], [sans], [https://github.com/XiaoxiaoPu/sans])
AC_CONFIG_SRCDIR([sans])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_AUX_DIR([.])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

# Checks for programs.
AC_PROG_CC_C99
AC_PROG_INSTALL
LT_INIT

# Custom options
AC_ARG_ENABLE(
    [static],
    [AS_HELP_STRING([--enable-static], [build with static linking])],
    [LDFLAGS="$LDFLAGS -static"])
AM_CONDITIONAL(STATIC, test x"$static" = x"true")
AC_ARG_ENABLE(
    [debug],
    [AS_HELP_STRING([--enable-debug], [build with additional debugging code])],
    [CFLAGS="$CFLAGS -g -DDEBUG -O0"])
AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")
AC_ARG_ENABLE(
    [coverage],
    [AS_HELP_STRING([--enable-coverage], [enable coverage analysis])],
    [CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"])

# Add library for MinGW
case $host in
  *-mingw*)
    mingw=yes
    LIBS="$LIBS -lws2_32"
    LDFLAGS="$LDFLAGS -static"
    ;;
  *)
    ;;
esac

# Checks for libraries.

# Checks for header files.
AC_HEADER_ASSERT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h grp.h netdb.h netinet/in.h pwd.h stddef.h stdint.h stdlib.h string.h sys/socket.h sys/time.h unistd.h])
case $host in
  *-mingw*)
    AC_CHECK_HEADERS([windows.h winsock2.h ws2tcpip.h], [], [AC_MSG_ERROR([Missing MinGW headers])], [])
    ;;
  *)
    ;;
esac

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UID_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([bzero gettimeofday memset setegid seteuid sigaction select socket strchr strdup strerror strrchr strtol])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
