bin_PROGRAMS = zarafa-ical

AM_CPPFLAGS = ${ZCPPFLAGS} $(DEBUGFLAGS) $(BOOST_CPPFLAGS) \
	-I${top_srcdir}/mapi4linux/include \
	-I${top_srcdir}/inetmapi -I${top_srcdir}/provider/common -I${top_srcdir}/common \
	-I${top_srcdir}/gateway -I${top_srcdir}/libicalmapi -I${top_srcdir}/libfreebusy \
	$(XML2_CFLAGS)

zarafa_ical_SOURCES = CalDAV.cpp CalDAV.h \
	CalDavProto.cpp CalDavProto.h \
	Http.cpp Http.h \
	iCal.cpp iCal.h \
	CalDavUtil.cpp CalDavUtil.h \
	WebDav.cpp WebDav.h \
	ProtocolBase.cpp ProtocolBase.h	

zarafa_ical_LDADD = ${top_builddir}/mapi4linux/src/libmapi.la \
	${top_builddir}/common/libcommon_mapi.la \
	${top_builddir}/common/libcommon_util.la \
	${top_builddir}/libicalmapi/libicalmapi.la \
	${top_builddir}/common/libcommon_ssl.la \
	${top_builddir}/libfreebusy/libfreebusy.la \
	$(PROG_LIBS) $(SSL_LIBS) $(ICONV_LIBS) $(XML2_LIBS) $(ICAL_LIBS)

check-syntax:
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) \
		-fsyntax-only -fmessage-length=0 ${CHK_SOURCES} -Wall -Wformat=2
