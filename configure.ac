#============================start configure.in============================
AC_PREREQ([2.71])
AC_INIT([mod_psldap],[0.96])
AC_CONFIG_SRCDIR([mod_psldap.c])
AC_CONFIG_HEADERS([config.h])
AC_PROG_CC
#AC_PROG_LIBTOOL
AC_CHECK_HEADERS([time.h lber.h ldap.h unistd.h],, AC_MSG_ERROR([Missing needed header file ]))

AC_CHECK_LIB([lber],[ber_free],,AC_MSG_ERROR([ Missing required lber library ]) )
AC_CHECK_LIB([ldap],[ldap_bind_s],,AC_MSG_ERROR([ Missing required ldap library ]) )

AC_ARG_WITH(apxs,AS_HELP_STRING([[--with-apxs=FILE]],[Path to apxs program]),
            [ APXS="$withval" ], [ AC_PATH_PROG(APXS, apxs, [], [$PATH:/usr/sbin]) ])

AC_MSG_CHECKING(whether to disable xslt)
AC_ARG_ENABLE(xslt, AS_HELP_STRING([[--disable-xslt]],[Disable XSLT capabilities, enabled by default]),
              [disable_xslt=yes], [disable_xslt=no] )

if test "x$disable_xslt" != "xyes"; then
  AC_ARG_WITH(xml2-config, AS_HELP_STRING([[--with-xml2-config=FILE]],[Path to xml2-config program]),
            [ XML2="$withval" ], [ AC_PATH_PROG(XML2, xml2-config, [], [$PATH:/usr/bin]) ])
  CPPFLAGS="`$XML2 --cflags` $CPPFLAGS"
  AC_CHECK_HEADERS([libxml/tree.h libxml/xmlsave.h libxslt/transform.h],,
                   AC_MSG_ERROR([Required XSLT and XML headers not available]))
  AC_CHECK_LIB(xml2,xmlNewChild,,AC_MSG_ERROR([XML Library does not exist or has inadequate support]))
  AC_CHECK_LIB(xslt,xsltRunStylesheet,,AC_MSG_ERROR([XML Library does not exist or has inadequate support]))
  AC_DEFINE([USE_LIBXML2_LIBXSL], [], [XML2 and XSLT libraries are installed and XSLT support is desired])
fi

AC_MSG_CHECKING(whether to enable caching)
AC_ARG_ENABLE(xslt, AS_HELP_STRING([[--enable-pscache]],[Enable authentication cache capabilities, off by default]),
              [enable_pscache=yes], [enable_pscache=no])

if test "x$enable_pscache" != "xno"; then
  AC_CHECK_HEADERS([ap_mm.h ap_rmm.h],,
                   AC_MSG_ERROR([Required Apache MM headers not available]))
  AC_CHECK_LIB([apr-1],[apr_rmm_malloc],,AC_MSG_ERROR([APR Library does not exist or has inadequate support]),-lmm)
  AC_DEFINE([USE_PSLDAP_CACHING], [], [MM libraries are installed and cache implementation is desired])
fi

EXTRA_DIST="mod_psldap.c ChangeLog FAQ.html License.txt *.schema RELEASE web"

AC_SUBST(CFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(APXS)
AC_SUBST(XML2)
AC_SUBST(EXTRA_DIST)
AM_INIT_AUTOMAKE([])
# Autoupdate added the next two lines to ensure that your configure
# script's behavior did not change.  They are probably safe to remove.
AC_CHECK_INCLUDES_DEFAULT
AC_PROG_EGREP

AC_CHECK_FUNCS([bzero memchr memmove memset strcasecmp strchr strcspn strncasecmp strpbrk strstr])
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_TYPE_SIZE_T
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
#============================end configure.in============================
