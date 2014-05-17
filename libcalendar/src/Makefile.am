lib_LTLIBRARIES = libmapicalendar.la

AM_CPPFLAGS = $(ZCPPFLAGS) $(DEBUGFLAGS) $(BOOST_CPPFLAGS) \
        -I$(top_srcdir)/mapi4linux/include \
        -I${top_srcdir}/common \
        -I${top_srcdir}/libtimezone \
        -I${top_srcdir}/libcalendar/include

libmapicalendar_la_LDFLAGS = 

libmapicalendar_la_LIBADD = \
                        ${top_builddir}/common/libcommon_mapi.la \
                        ${top_builddir}/common/libcommon_util.la \
                        ${top_builddir}/libtimezone/libmapitimezone.la \
                        $(PROG_LIBS) \
                        $(DL_LIBS)

libmapicalendar_la_SOURCES = \
                        Appointment.cpp \
                        CalendarTable.cpp \
                        CalendarUtil.cpp \
                        CompositeMessage.cpp \
                        Conversions.cpp \
                        Occurrence.cpp \
                        OccurrenceData.cpp \
                        PropertyPool.cpp \
                        PropertySet.cpp \
                        RecurrencePattern.cpp


EXTRA_DIST = \
            CalendarUtil.h \
            CompositeMessage.h \
            ConversionsInternal.h \
            OccurrenceData.h \
            Occurrence.h \
            PropertySet.h


check-syntax:
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) \
                -fsyntax-only -fmessage-length=0 ${CHK_SOURCES} -Wall -Wformat=2
