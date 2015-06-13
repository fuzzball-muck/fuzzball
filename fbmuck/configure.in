dnl
dnl Process this file with autoconf to produce a configure script.
dnl
dnl TinyMUCK fb5.x auto-config script writen by: Peter "WhiteFire" Torkelson
dnl Rewritten for fb6.x by revar@belfry.com
dnl
AC_REVISION($Revision: 1.30 $)
AC_PREREQ(2.69)
AC_INIT(fbmuck, 7.00a, [fuzzball.muck@gmail.com])
AC_CONFIG_AUX_DIR(auto)
AC_CONFIG_HEADER([include/autoconf.h])

echo " "
echo "TinyMUCK fb7.x auto-configure script."
echo " "
echo "This script will try and determine things about your system so"
echo "that FB can compile correctly. This will create your Makefile"
echo "and the header file autoconf.h in the include directory."
echo " "

dnl
dnl Find the compiler first.
dnl
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
dnl AC_PROG_CXX for finding the C++ compiler.

dnl
dnl Specific systems tests.
dnl
AC_ISC_POSIX
AC_MINIX

dnl
dnl Libraries and functions.
dnl
AC_CHECK_LIB(m, cos)
AC_CHECK_LIB(socket, shutdown)
AC_CHECK_LIB(nsl, gethostbyname)
AC_FUNC_VPRINTF


checkpcredir() { :
    if test -f "$1/include/pcre/pcre.h"; then
        AC_DEFINE(HAVE_PCREINCDIR, [], [PCRE headers are under pcre dir.])
        pcredir="$1"
        return 0
    fi
    if test -f "$1/include/pcre.h"; then
        pcredir="$1"
        return 0
    fi
    return 1
}

dnl Let the user specify where to find the PCRE libs
AC_ARG_WITH(pcre,
    [  --with-pcre=DIR         location of installed PCRE libraries/include files],
    [
	if test "$withval" = "no"; then
		pcreenable=0
	elif test "$withval" != "yes"; then
		pcreenable=1
		checkpcredir "$withval"
	else
		pcreenable=1
	fi

    ],
    [
	pcreenable=1
    ]
)
if test "$pcreenable" = "1"; then
	AC_MSG_CHECKING([for PCRE directory])
	if test -z "$pcredir"; then
		for maindir in /usr /usr/local /usr/pkg /opt /sw
		do
			for dir in $maindir $maindir/pcre
			do
				checkpcredir $dir && break 2
			done
		done
	fi
	if test -z "$pcredir"; then
   		AC_MSG_RESULT([Not found])
		echo
		echo "Couldn't find your PCRE library installation dir."
		echo "Use --with-pcre option to fix this problem"
		echo
		exit -1
	fi
	AC_MSG_RESULT([$pcredir])
	AC_SUBST(pcredir)
	AC_DEFINE_UNQUOTED(pcredir, "$pcredir", [The base path to the installation for PCRE.  Usually /usr.])

	if test "$pcredir"; then
		INC="$INC -I$pcredir/include"
		LIBS="$LIBS -L$pcredir/lib -lpcre"
		AC_CHECK_LIB(pcre, pcre_free)
	fi
	AC_SUBST(INC)
else
	echo
	echo "fbmuck requires the PCRE library to compile."
	echo "Use --with-pcre option to fix this problem"
	echo
fi




checkssldir() { :
    if test -f "$1/include/openssl/ssl.h"; then
        AC_DEFINE(HAVE_OPENSSL, [], [ssl headers are under openssl dir.])
        ssldir="$1"
        return 0
    fi
    if test -f "$1/include/ssl.h"; then
        ssldir="$1"
        return 0
    fi
    return 1
}

dnl if the user wants SSL, he has to ask for it
AC_ARG_WITH(ssl,
    [  --with-ssl=DIR          location of installed SSL libraries/include files],
    [
        dnl Check the specified localtion only
		AC_MSG_CHECKING([for SSL directory])
		if test "$withval" = "no"; then
			sslenable=0
		elif test "$withval" != "yes"; then
			sslenable=1
        	checkssldir "$withval"
		else
			sslenable=1
			for maindir in /usr /usr/local /usr/lib /usr/pkg /opt /var/ssl /sw
			do
				for dir in $maindir $maindir/openssl $maindir/ssl
				do
					checkssldir $dir && break 2
				done
			done
		fi

    ],
    [
        dnl THE USER DID NOT ASK FOR SSL, DON'T CHECK IT!!!
		sslenable=0
    ]
)
if test "$sslenable" = "1"; then
	if test -z "$ssldir"; then
   		AC_MSG_RESULT([Not found])
    	echo
    	echo "Couldn't find your SSL library installation dir."
    	echo "Use --with-ssl option to fix this problem, if you want ssl support"
    	echo
	fi
	AC_MSG_RESULT([$ssldir])
	AC_SUBST(ssldir)
	AC_DEFINE_UNQUOTED(ssldir, "$ssldir", [The base path to the installation for openssl.  Usually /usr.])

	if test "$ssldir"; then
		INC="$INC -I$ssldir/include"
		LIBS="$LIBS -L$ssldir/lib -lssl"
		AC_CHECK_LIB(crypto, CRYPTO_free)
		AC_CHECK_LIB(ssl, SSL_read)
	fi
	AC_SUBST(INC)
fi



AC_ARG_WITH(lint,
[  --with-lint[=lint_path]    use lint at lint_path, defaults to splint],
[AC_DEFINE(LINT, [], [Configures which LINT to use to evaluate the sources.])
if test "$withval" = yes; then
  LINT=splint
else
  LINT=$withval
fi
AC_DEFINE(WITH_LINT, [], [Makefile is configured to run lint on sources.])
AC_SUBST(LINT)
])

AC_ARG_ENABLE(memprof,
[  --enable-memprof           enable memory leak detection and profiling (slow)],
[
if test "$enableval" = yes; then
	AC_DEFINE(MALLOC_PROFILING)
else
	if test "$enableval" = debug; then
		AC_DEFINE(MALLOC_PROFILING, [], [Enables memory usage profiling.])
		AC_DEFINE(MALLOC_PROFILING_EXTRA, [], [With MALLOC_PROFILING, can detect double-frees, buffer overruns, etc.])
	fi
fi
])

AC_ARG_ENABLE(debug,
[  --enable-debug             enable developer assertions],
[
if test "$enableval" = yes; then
	AC_DEFINE(DEBUG, [], [Enable debugging assertions.])
fi
])


#
# IPv6
#
AC_ARG_ENABLE(ipv6,
    [  --enable-ipv6             enable IPv6-support],
    [
        if test $enable_ipv6 == "yes";
        then
	    AC_DEFINE(USE_IPV6,[], [Enable IPv6 support.])
	fi
    ])



dnl
dnl Header files
dnl
AC_HEADER_STDC
AC_CHECK_HEADERS(malloc.h sys/resource.h sys/signal.h)
AC_HEADER_TIME
AC_CHECK_HEADERS(sys/time.h timebits.h)
AC_CHECK_HEADERS(varargs.h stdarg.h)
AC_CHECK_HEADERS(errno.h sys/errno.h)
AC_HEADER_DIRENT

AC_CHECK_SIZEOF([long int])
AC_CHECK_SIZEOF([int])

dnl
dnl Types and structures
dnl
AC_TYPE_PID_T
AC_TYPE_UID_T
AC_TYPE_SIZE_T
AC_TYPE_SIGNAL
AC_STRUCT_TM
AC_STRUCT_TIMEZONE
AC_CACHE_CHECK(for struct tm.tm_gmtoff, rb_cv_member_struct_tm_tm_gmtoff,
  [AC_TRY_COMPILE([#include <time.h>],
    [struct tm t; t.tm_gmtoff = 3600;],
    [rb_cv_member_struct_tm_tm_gmtoff=yes],
    [rb_cv_member_struct_tm_tm_gmtoff=no])])
if test "$rb_cv_member_struct_tm_tm_gmtoff" = yes; then
  AC_DEFINE(HAVE_STRUCT_TM_TM_GMTOFF, [], [Define to 1 if 'tm_gmtoff' is a member of 'struct tm'.])
fi


dnl
dnl Check if socklen_t is defined in sys/socket.h.
dnl
AC_DEFUN([TYPE_SOCKLEN_T],
[AC_CACHE_CHECK([for socklen_t], ac_cv_type_socklen_t,
  [AC_TRY_COMPILE(
    [#include <sys/types.h>
     #include <sys/socket.h>],
    [socklen_t len = 42; return 0;],
    ac_cv_type_socklen_t=yes,
    ac_cv_type_socklen_t=no)
  ])
  if test $ac_cv_type_socklen_t != yes; then
    AC_DEFINE(socklen_t, int, [The type that holds socket length values on this system.])
  fi
])
TYPE_SOCKLEN_T

AC_CHECK_FUNCS(mallinfo getrlimit getrusage random snprintf vsnprintf)
AC_CHECK_MEMBER([struct mallinfo.hblks])
AC_CHECK_MEMBER([struct mallinfo.keepcost])
AC_CHECK_MEMBER([struct mallinfo.treeoverhead])
AC_CHECK_MEMBER([struct mallinfo.grain])
AC_CHECK_MEMBER([struct mallinfo.allocated])
AC_CHECK_MEMBER([struct tm.tm_gmtoff])
AC_CHECK_DECL([_timezone])


dnl
dnl Compiler characteristics
dnl
AC_C_CONST
AC_C_INLINE
AC_C_LONG_DOUBLE

AC_DEFUN([AC_C_LONG_LONG],
[AC_CACHE_CHECK(for long long int, ac_cv_c_long_long,
[if test "$GCC" = yes; then
    ac_cv_c_long_long=yes
 else
    AC_TRY_COMPILE(,[long long int i;],
       ac_cv_c_long_long=yes,
       ac_cv_c_long_long=no)
 fi])
   if test $ac_cv_c_long_long = yes; then
     AC_DEFINE(HAVE_LONG_LONG, [], [Define to 1 if you have long long.])
   fi
])
AC_C_LONG_LONG


dnl
dnl Uname -a, just because.
dnl
echo "checking value of uname -a"
AC_DEFINE_UNQUOTED(UNAME_VALUE, "`uname -a`", [The value of the uname -a command.])

dnl
dnl Remove HAVE_CONFIG_H
dnl
AC_CONFIG_COMMANDS_PRE([DEFS=])

dnl And in the end, there was no more.
dnl
AC_OUTPUT(
	Makefile
	src/Makefile
	game/restart
)
echo " "
echo "You should review the options in include/config.h, and"
echo "then type make to build your system."
echo " "
