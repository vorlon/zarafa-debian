# function tests for the availability and requirement of swig
# must be called to call other SWIG_CHECK_* functions
AC_DEFUN([SWIG_CHECK_SWIG], [
AC_ARG_ENABLE(swig, AC_HELP_STRING([--enable-swig], [enable regenerating swig code]), [want_swig=${enableval}], [want_swig=no])
AC_ARG_ENABLE(python, AC_HELP_STRING([--enable-python], [enable building python binding]), [want_python=${enableval}], [want_python=auto])

if test "$want_swig" = "yes"; then
   need_swig=yes
elif test "$want_python" = "yes" -a ! -f "$srcdir/swig/python/mapi_wrap.cxx"; then
   need_swig=yes
else
   need_swig=no
fi

AC_PATH_PROG(SWIG_EXEC, [swig])
if test "$need_swig" = "yes" -a "x$SWIG_EXEC" = "x"; then
   AC_MSG_ERROR([Current options require swig, but swig binary is not found])
fi
AM_CONDITIONAL([WITH_SWIG], [test "$want_swig" = "yes"])
])

#python
AC_DEFUN([SWIG_CHECK_PYTHON],[
if test "$want_python" = "yes" -o "$want_python" = "auto"; then
  AC_MSG_CHECKING([for python])
  AC_ARG_WITH(python, AC_HELP_STRING([--with-python=PATH],[path to the python interpreter]),
    [PYTHON=${withval}],[PYTHON=python])
  if ! test -x "`which $PYTHON 2>/dev/null`"; then
	if test "$want_python" = "yes" -a "$want_swig" = "yes"; then
      AC_MSG_ERROR([Cannot execute $PYTHON])
	else
	  AC_MSG_RESULT([no])
	fi
    want_python=no
  else
    if test "$want_python" = "auto" -a "x$SWIG_EXEC" = "x"; then
		want_python=no
		AC_MSG_RESULT([no])
	else
		want_python=yes
		cat > python-config << EOF
#! /usr/bin/python

import sys
import os
import getopt
from distutils import sysconfig

valid_opts = [['prefix', 'exec-prefix', 'includes', 'libs', 'cflags', 
              'ldflags', 'libdir', 'help']]

def exit_with_usage(code=1):
    print >>sys.stderr, "Usage: %s [[%s]]" % (sys.argv[[0]], 
                                            '|'.join('--'+opt for opt in valid_opts))
    sys.exit(code)

try:
    opts, args = getopt.getopt(sys.argv[[1:]], '', valid_opts)
except getopt.error:
    exit_with_usage()

if not opts:
    exit_with_usage()

opt = opts[[0]][[0]]

pyver = sysconfig.get_config_var('VERSION')
getvar = sysconfig.get_config_var

if opt == '--help':
    exit_with_usage(0)

elif opt == '--prefix':
    print sysconfig.PREFIX

elif opt == '--exec-prefix':
    print sysconfig.EXEC_PREFIX

elif opt in ('--includes', '--cflags'):
    flags = [['-I' + sysconfig.get_python_inc(),
             '-I' + sysconfig.get_python_inc(plat_specific=True)]]
    if opt == '--cflags':
        flags.extend(getvar('CFLAGS').split())
    print ' '.join(flags)

elif opt in ('--libs', '--ldflags'):
    libs = getvar('LIBS').split() + getvar('SYSLIBS').split()
    libs.append('-lpython'+pyver)
    # add the prefix/lib/pythonX.Y/config dir, but only if there is no
    # shared library in prefix/lib/.
    if opt == '--ldflags' and not getvar('Py_ENABLE_SHARED'):
        libs.insert(0, '-L' + getvar('LIBPL'))
    print ' '.join(libs)

elif opt in ('--libdir'):
	print sysconfig.get_python_lib(1)
EOF

		PYTHON_INCLUDES=`$PYTHON python-config --includes`
		PYTHON_LIBS=`$PYTHON python-config --libs`
  		PYTHON_LIBDIR=`$PYTHON python-config --libdir`
  		pythonexeclibdir=$PYTHON_LIBDIR
		AC_SUBST(PYTHON_INCLUDES)
		AC_SUBST(PYTHON_LIBS)
		AC_SUBST(PYTHON_LIBDIR)
		AC_SUBST(pythonexeclibdir)
		AC_SUBST(PYTHON)
		AC_MSG_RESULT([yes])
	fi
  fi
fi
AM_CONDITIONAL([WITH_PYTHON], [test "$want_python" = "yes"])
])
