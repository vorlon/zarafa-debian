lib_LTLIBRARIES = libarchiver.la libarchiver-core.la
bin_PROGRAMS = zarafa-archiver



AM_CPPFLAGS = ${ZCPPFLAGS} $(DEBUGFLAGS) $(BOOST_CPPFLAGS) \
	-DARCHIVER_EXTRA \
	-I${top_srcdir}/mapi4linux/include \
	-I${top_srcdir}/provider/client \
	-I${top_srcdir}/provider/include \
	-I${top_srcdir}/provider/soap \
	-I${top_builddir}/provider/soap \
	-I${top_srcdir}/common \
	$(MYSQL_INCLUDES)



libarchiver_la_LIBADD = ${top_builddir}/mapi4linux/src/libmapi.la \
	${top_builddir}/common/libcommon_mapi.la \
	${top_builddir}/common/libcommon_util.la

libarchiver_la_SOURCES = \
	ArchiverSession.cpp ArchiverSession.h archiver-session_fwd.h \
	archiver-common.cpp archiver-common.h ArchiveControl.h \
	ArchiveManageImpl.cpp ArchiveManageImpl.h ArchiveManage.h\
	ArchiveStateCollector.cpp ArchiveStateCollector.h \
	ArchiveStateUpdater.cpp ArchiveStateUpdater.h archivestateupdater_fwd.h \
	helpers/ArchiveHelper.cpp helpers/ArchiveHelper.h \
	helpers/storehelper.cpp helpers/storehelper.h \
	helpers/mapiprophelper.cpp helpers/mapiprophelper.h \
	ECArchiverLogger.cpp ECArchiverLogger.h



libarchiver_core_la_LIBADD = ${top_builddir}/mapi4linux/src/libmapi.la \
	${top_builddir}/common/libcommon_mapi.la \
	${top_builddir}/common/libcommon_service.la \
	libarchiver.la \
	$(PROG_LIBS) $(SSL_LIBS) $(MYSQL_LIBS)

libarchiver_core_la_SOURCES = \
	Archiver.cpp Archiver.h AutoMAPI.h\
	ArchiverImpl.cpp ArchiverImpl.h\
	operations/instanceidmapper.cpp operations/instanceidmapper.h operations/instanceidmapper_fwd.h \
	operations/operations.cpp operations/operations.h operations/operations_fwd.h \
	operations/copier.cpp operations/copier.h \
	operations/deleter.cpp operations/deleter.h \
	operations/stubber.cpp operations/stubber.h \
	operations/transaction.cpp operations/transaction.h operations/transaction_fwd.h \
	operations/postsaveaction.h \
	operations/postsaveiidupdater.cpp operations/postsaveiidupdater.h \
	ArchiveControlImpl.cpp ArchiveControlImpl.h \
	ECDatabase.h ECDatabase.cpp



zarafa_archiver_LDADD = ${top_builddir}/mapi4linux/src/libmapi.la \
	${top_builddir}/common/libcommon_mapi.la \
	${top_builddir}/common/libcommon_util.la \
	libarchiver.la libarchiver-core.la \
	$(PROG_LIBS) ${top_builddir}/common/libcommon_service.la

zarafa_archiver_SOURCES = main.cpp



check-syntax:
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) \
		-fsyntax-only -fmessage-length=0 ${CHK_SOURCES} -Wall -Wformat=2
