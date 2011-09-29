bin_PROGRAMS = zarafa-admin

AM_CPPFLAGS = $(DEBUGFLAGS) $(BOOST_CPPFLAGS) -DLINUX \
	-I${top_srcdir}/mapi4linux/include \
	-I${top_srcdir}/common \
	-I${top_srcdir}/ECtools/zarafa-archiver

zarafa_admin_LDADD = ${top_builddir}/mapi4linux/src/libmapi.la \
	${top_builddir}/common/libcommon_mapi.la \
	${top_builddir}/common/libcommon_util.la \
	${top_builddir}/ECtools/zarafa-archiver/libarchiver.la
	$(PROG_LIBS)

zarafa_admin_SOURCES = zarafa-admin.cpp ConsoleTable.h ConsoleTable.cpp

check-syntax:
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) \
		-fsyntax-only -fmessage-length=0 ${CHK_SOURCES} -Wall -Wformat=2
