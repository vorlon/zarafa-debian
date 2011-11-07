bin_PROGRAMS = zarafa-monitor

AM_CPPFLAGS = $(DEBUGFLAGS) $(BOOST_CPPFLAGS) -DLINUX \
	-I${top_srcdir}/mapi4linux/include \
	-I${top_srcdir}/common

zarafa_monitor_LDADD = ${top_builddir}/mapi4linux/src/libmapi.la \
	${top_builddir}/common/libcommon_mapi.la \
	${top_builddir}/common/libcommon_util.la \
	$(PROG_LIBS)

zarafa_monitor_SOURCES = zarafa-monitor.cpp ECMonitorDefs.h ECQuotaMonitor.h ECQuotaMonitor.cpp

check-syntax:
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) \
		-fsyntax-only -fmessage-length=0 ${CHK_SOURCES} -Wall -Wformat=2
