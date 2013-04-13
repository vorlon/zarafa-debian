lib_LTLIBRARIES = libmapi.la

AM_CPPFLAGS = $(DEBUGFLAGS) $(BOOST_CPPFLAGS) -DLINUX -DMAPICONFIGDIR=\"${MAPICONFIGDIR}\" \
	-I$(top_srcdir)/mapi4linux/include -I$(top_srcdir)/provider/common -I${top_srcdir}/common -I${top_srcdir}/libfreebusy

libmapi_la_LDFLAGS = $(BOOST_FILESYSTEM_LDFLAGS)

libmapi_la_LIBADD = \
			${top_builddir}/common/libcommon_mapi.la \
			${top_builddir}/common/libcommon_util.la \
			$(BOOST_FILESYSTEM_LIBS) \
			$(BOOST_SYSTEM_LIBS) \
			$(PROG_LIBS) \
			$(DL_LIBS)

libmapi_la_SOURCES = common.cpp mapidefs.cpp mapispi.cpp mapiutil.cpp mapix.cpp rtf.cpp imessage.cpp \
	m4l.common.h m4l.mapidefs.h m4l.mapispi.h m4l.mapiutil.h m4l.mapix.h mapi4linux.h m4l.debug.h rtf.h \
	m4l.mapisvc.cpp m4l.mapisvc.h

check-syntax:
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) \
		-fsyntax-only -fmessage-length=0 ${CHK_SOURCES} -Wall -Wformat=2
