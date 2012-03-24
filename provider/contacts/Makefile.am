lib_LTLIBRARIES = libzarafacontacts.la

sysconf_configdir = ${MAPICONFIGDIR}
dist_sysconf_config_DATA = zcontacts.inf

AM_CPPFLAGS = $(DEBUGFLAGS) -DLINUX		\
	-I${top_srcdir}/provider/include	\
	-I${top_srcdir}/provider/common		\
	-I${top_srcdir}/provider/client		\
	-I${top_srcdir}/mapi4linux/include	\
	-I${top_srcdir}/common

libzarafacontacts_la_LDFLAGS = -avoid-version
libzarafacontacts_la_LIBADD = \
	${top_builddir}/common/libcommon_util.la			\
	${top_builddir}/common/libcommon_mapi.la			\
	${top_builddir}/mapi4linux/src/libmapi.la			\
	-lstdc++ $(UUID_LIBS) $(ICU_LIBS)

libzarafacontacts_la_SOURCES = EntryPoint.cpp ZCABData.h \
	ZCMAPIProp.h ZCMAPIProp.cpp \
	ZCABProvider.h ZCABProvider.cpp \
	ZCABLogon.h ZCABLogon.cpp \
	ZCABContainer.h ZCABContainer.cpp

check-syntax:
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) \
		-fsyntax-only -fmessage-length=0 ${CHK_SOURCES} -Wall -Wformat=2
