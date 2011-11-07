bin_PROGRAMS = zarafa-server

AM_CPPFLAGS = $(DEBUGFLAGS) -DLINUX $(BOOST_CPPFLAGS) \
	-I${top_srcdir}/mapi4linux/include \
	-I${top_srcdir}/provider/libserver -I${top_srcdir}/provider/plugins \
	-I${top_srcdir}/provider/include -I${top_srcdir}/provider/common \
	-I${top_srcdir}/provider/client -I${top_builddir}/provider/soap \
	-I${top_srcdir}/provider/soap \
	-I${top_srcdir}/common -DPKGLIBDIR='"${pkglibdir}"' \
	$(MYSQL_INCLUDES) $(GSOAP_CFLAGS) $(SSL_CFLAGS) $(SSL_HAS_EVP_PKEY_CMP) $(EPOLL_CFLAGS)

zarafa_server_LDFLAGS = -Wl,-E $(BOOST_FILESYSTEM_LDFLAGS)
zarafa_server_LDADD = \
	${top_builddir}/provider/libserver/libserver.la							\
	${top_builddir}/provider/soap/libgsoap.la								\
	${top_builddir}/provider/soap/libsoapserver.la							\
	${top_builddir}/provider/common/libzarafacommon.la						\
	${top_builddir}/common/libcommon_util.la								\
	${top_builddir}/common/libcommon_mapi.la								\
	${top_builddir}/common/libcommon_ssl.la									\
	$(PROG_LIBS)															\
	$(BOOST_FILESYSTEM_LIBS) $(BOOST_SYSTEM_LIBS) $(ICU_LIBS) $(DL_LIBS)	\
	$(MYSQL_LIBS) $(SSL_LIBS) $(PAM_LIBS) $(KRB5_LIBS) $(TCMALLOC_LIBS) -lrt

zarafa_server_SOURCES = ECServer.cpp  \
						ECSoapServerConnection.h ECSoapServerConnection.cpp \
						ECClientUpdate.h ECClientUpdate.cpp \
						ECThreadManager.h ECThreadManager.cpp

check-syntax:
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) \
		-fsyntax-only -fmessage-length=0 ${CHK_SOURCES} -Wall -Wformat=2
