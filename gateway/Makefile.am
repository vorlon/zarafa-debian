bin_PROGRAMS = zarafa-gateway

AM_CPPFLAGS = $(DEBUGFLAGS) $(BOOST_CPPFLAGS) -DLINUX \
	-I${top_srcdir}/mapi4linux/include \
	-I${top_srcdir}/inetmapi -I${top_srcdir}/provider/common -I${top_srcdir}/common

zarafa_gateway_LDADD = ${top_builddir}/inetmapi/libinetmapi.la \
	${top_builddir}/mapi4linux/src/libmapi.la \
	${top_builddir}/common/libcommon_mapi.la \
	${top_builddir}/common/libcommon_util.la \
	${top_builddir}/common/libcommon_ssl.la \
	$(PROG_LIBS) $(SSL_LIBS)

zarafa_gateway_SOURCES = Gateway.cpp POP3.cpp POP3.h IMAP.cpp IMAP.h ClientProto.h

check-syntax:
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) \
		-fsyntax-only -fmessage-length=0 ${CHK_SOURCES} -Wall -Wformat=2
