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
LIB_RESOLV="-lresolv"
LIB_EV="-lev"
AC_CHECK_LIB([resolv], [ns_initparse], [AC_SUBST(LIB_RESOLV)], [libresolv=no])
AC_CHECK_LIB([ev], [ev_default_loop], [AC_SUBST(LIB_EV)], [libev=no])
AM_CONDITIONAL(BUILD_RESOLV, test x"$libresolv" = x"no")
AM_CONDITIONAL(BUILD_EV, test x"$libev" = x"no")

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
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UID_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([bzero gettimeofday memset setegid seteuid sigaction socket strchr strdup strerror strrchr strtol])


# libev
case $libev in
  no)
    case $host in
      *-mingw*)
        AH_VERBATIM([FD_SETSIZE],
        [#ifdef FD_SETSIZE
         #  undef FD_SETSIZE
         #endif])
        AC_DEFINE([FD_SETSIZE], [2048], [Reset max file descriptor size.])
        AC_DEFINE([EV_FD_TO_WIN32_HANDLE(fd)], [(fd)], [Override libev default fd conversion macro.])
        AC_DEFINE([EV_WIN32_HANDLE_TO_FD(handle)], [(handle)], [Override libev default handle conversion macro.])
        AC_DEFINE([EV_WIN32_CLOSE_FD(fd)], [closesocket(fd)], [Override libev default fd close macro.])
        AC_MSG_CHECKING([for select in ws2_32])
        AC_TRY_LINK(
        [#ifdef HAVE_WINSOCK2_H
         #ifndef WIN32_LEAN_AND_MEAN
         #define WIN32_LEAN_AND_MEAN
         #endif
         #include <winsock2.h>
         #endif
        ],[
          select(0,(fd_set *)NULL,(fd_set *)NULL,(fd_set *)NULL,(struct timeval *)NULL);
        ],[
          AC_MSG_RESULT([yes])
          HAVE_SELECT="1"
          AC_DEFINE_UNQUOTED(HAVE_SELECT, 1,
            [Define to 1 if you have the `select' function.])
          HAVE_SYS_SELECT_H="1"
          AC_DEFINE_UNQUOTED(HAVE_SYS_SELECT_H, 1,
            [Define to 1 if you have the <sys/select.h> header file.])
          ],[
          AC_MSG_ERROR([no])
        ])
        ;;
      *)
        ;;
    esac
    AC_CHECK_HEADERS(sys/inotify.h sys/epoll.h sys/event.h port.h poll.h sys/select.h sys/eventfd.h sys/signalfd.h)
    AC_CHECK_FUNCS(inotify_init epoll_ctl kqueue port_create poll select eventfd signalfd)
    AC_CHECK_FUNCS(clock_gettime, [], [
       # on linux, try syscall wrapper first
       if test $(uname) = Linux; then
          AC_MSG_CHECKING(for clock_gettime syscall)
          AC_LINK_IFELSE([AC_LANG_PROGRAM(
                          [#include <unistd.h>
                           #include <sys/syscall.h>
                           #include <time.h>],
                          [struct timespec ts; int status = syscall (SYS_clock_gettime, CLOCK_REALTIME, &ts)])],
                         [ac_have_clock_syscall=1
                          AC_DEFINE(HAVE_CLOCK_SYSCALL, 1, Define to 1 to use the syscall interface for clock_gettime)
                          AC_MSG_RESULT(yes)],
                         [AC_MSG_RESULT(no)])
       fi
       if test -z "$LIBEV_M4_AVOID_LIBRT" && test -z "$ac_have_clock_syscall"; then
          AC_CHECK_LIB(rt, clock_gettime)
          unset ac_cv_func_clock_gettime
          AC_CHECK_FUNCS(clock_gettime)
       fi
    ])
    AC_CHECK_FUNCS(nanosleep, [], [
       if test -z "$LIBEV_M4_AVOID_LIBRT"; then
          AC_CHECK_LIB(rt, nanosleep)
          unset ac_cv_func_nanosleep
          AC_CHECK_FUNCS(nanosleep)
       fi
    ])
    if test -z "$LIBEV_M4_AVOID_LIBM"; then
       LIBM=m
    fi
    AC_SEARCH_LIBS(floor, $LIBM, [AC_DEFINE(HAVE_FLOOR, 1, Define to 1 if the floor function is available)])
    ;;
  *)
    ;;
esac

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
