noinst_LTLIBRARIES = libzarafacommon.la

AM_CPPFLAGS = $(DEBUGFLAGS) -DLINUX $(GSOAP_CFLAGS) $(BOOST_CPPFLAGS) \
	-I${top_srcdir}/mapi4linux/include -I${top_srcdir}/provider/include \
	-I${top_builddir}/provider/soap -I${top_srcdir}/provider/soap -I${top_srcdir}/common

libzarafacommon_la_SOURCES = SOAPUtils.cpp SOAPUtils.h \
	SOAPDebug.cpp SOAPDebug.h \
	ZarafaUtil.cpp ZarafaUtil.h \
	ZarafaUser.h ZarafaUser.cpp \
	ECChannelClient.h ECChannelClient.cpp \
	ECLicenseClient.h ECLicenseClient.cpp \
	ECSearchClient.h ECSearchClient.cpp \
	ECOfflineState.h ECOfflineState.cpp \
	StreamTypes.h ZarafaVersions.h

check-syntax:
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) \
		-fsyntax-only -fmessage-length=0 ${CHK_SOURCES} -Wall -Wformat=2
