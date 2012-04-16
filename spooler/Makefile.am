bin_PROGRAMS = zarafa-dagent zarafa-spooler

SUBDIRS=python

AM_CPPFLAGS = $(DEBUGFLAGS) $(BOOST_CPPFLAGS) $(PYTHON_INCLUDES) -DLINUX -DSWIG_TYPE_TABLE=MAPI \
	-I${top_srcdir}/mapi4linux/include \
	-I${top_srcdir}/inetmapi -I${top_srcdir}/provider/common -I${top_srcdir}/common \
	-I${top_srcdir}/libfreebusy \
	-I${top_srcdir}/ECtools/zarafa-archiver

zarafa_dagent_LDADD = ${top_builddir}/inetmapi/libinetmapi.la \
	${top_builddir}/mapi4linux/src/libmapi.la \
	${top_builddir}/common/libcommon_mapi.la \
	${top_builddir}/common/libcommon_util.la \
	${top_builddir}/common/libcommon_ssl.la \
	${top_builddir}/ECtools/zarafa-archiver/libarchiver.la \
	${top_builddir}/ECtools/zarafa-archiver/libarchiver-core.la \
	$(PROG_LIBS) $(SSL_LIBS) $(PYTHON_LIBS)

zarafa_spooler_LDADD = ${top_builddir}/inetmapi/libinetmapi.la \
	${top_builddir}/mapi4linux/src/libmapi.la \
	${top_builddir}/common/libcommon_mapi.la \
	${top_builddir}/common/libcommon_util.la \
	${top_builddir}/common/libcommon_ssl.la \
	${top_builddir}/ECtools/zarafa-archiver/libarchiver.la \
	${top_builddir}/ECtools/zarafa-archiver/libarchiver-core.la \
	$(PROG_LIBS) $(PYTHON_LIBS)

zarafa_dagent_SOURCES = DAgent.cpp rules.cpp rules.h LMTP.cpp LMTP.h archive.cpp archive.h PyMapiPlugin.cpp PyMapiPlugin.h PythonSWIGRuntime.h
zarafa_spooler_SOURCES = Spooler.cpp mailer.cpp mailer.h archive.cpp archive.h PyMapiPlugin.cpp PyMapiPlugin.h PythonSWIGRuntime.h

BUILT_SOURCES=PythonSWIGRuntime.h

CLEANFILES=PythonSWIGRuntime.h
EXTRA_DIST=PythonSWIGRuntime.h

PythonSWIGRuntime.h:
	$(SWIG_EXEC) -python -external-runtime PythonSWIGRuntime.h

check-syntax:
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) \
		-fsyntax-only -fmessage-length=0 ${CHK_SOURCES} -Wall -Wformat=2

