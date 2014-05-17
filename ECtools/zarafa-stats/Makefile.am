bin_PROGRAMS = zarafa-stats

if ! OSS_ONLY
dist_noinst_DATA = zarafa-ssm zarafa-ssm.default zarafa-ssm.cron
endif

AM_CPPFLAGS = ${ZCPPFLAGS} $(DEBUGFLAGS) $(BOOST_CPPFLAGS) \
	$(NCURSES_FLAGS) \
	-I${top_srcdir}/mapi4linux/include \
	-I${top_srcdir}/libfreebusy	\
	-I${top_srcdir}/common

# ncurses needs to come first, because ICU_LIBS erroneously has -L/usr/lib64
# which overrides the ncurses lib selection >:-(
zarafa_stats_LDADD = \
	$(NCURSES_LIBS) \
	${top_builddir}/mapi4linux/src/libmapi.la \
	${top_builddir}/common/libcommon_mapi.la \
	${top_builddir}/common/libcommon_util.la \
	$(PROG_LIBS)

zarafa_stats_SOURCES = zarafa-stats.cpp

check-syntax:
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) \
		-fsyntax-only -fmessage-length=0 ${CHK_SOURCES} -Wall -Wformat=2
