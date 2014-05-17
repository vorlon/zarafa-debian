lib_LTLIBRARIES = libmapitimezone.la

AM_CPPFLAGS = $(DEBUGFLAGS) $(BOOST_CPPFLAGS) \
	-I${top_srcdir}/common -I${top_srcdir}/mapi4linux/include -DTIMEZONEDIR=\"${TIMEZONEDIR}\"

libmapitimezonedir = $(includedir)/mapitimezone
libmapitimezone_la_SOURCES = ECMAPITimezone.cpp OlsonNames.cpp
libmapitimezone_HEADERS = ECMAPITimezone.h
libmapitimezone_la_LIBADD = \
    ${top_builddir}/mapi4linux/src/libmapi.la \
    ${top_builddir}/common/libcommon_util.la \
    $(BOOST_FILESYSTEM_LIBS) 

OlsonNames.cpp: makeOlsonNames.py windowsZones.xml
	$(AM_V_GEN)python ${srcdir}/makeOlsonNames.py ${srcdir}/windowsZones.xml > ${srcdir}/$@

BUILT_SOURCES = OlsonNames.cpp
CLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = $(BUILT_SOURCES) makeOlsonNames.py windowsZones.xml

# timezone definitions
datadir = @TIMEZONEDIR@
EXTRA_DIST += timezones

install-data-hook:
	install -d -m 755 ${DESTDIR}/${TIMEZONEDIR}
	cp -a timezones/* ${DESTDIR}/${TIMEZONEDIR}/

check-syntax:
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) \
		-fsyntax-only -fmessage-length=0 ${CHK_SOURCES} -Wall -Wformat=2
