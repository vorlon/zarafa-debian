if WITH_CLUCENE
bin_PROGRAMS = zarafa-indexer
endif

AM_CPPFLAGS = \
	-I${top_srcdir}/mapi4linux/include \
	-I${top_srcdir}/common \
	-I${top_srcdir}/provider/include \
	-I${top_srcdir}/ECtools/zarafa-archiver \
	$(DEBUGFLAGS) ${CLUCENE_CFLAGS} $(BOOST_CPPFLAGS) -DLINUX $(MYSQL_INCLUDES)
	
zarafa_indexer_LDADD = \
	${top_builddir}/mapi4linux/src/libmapi.la \
	${top_builddir}/common/libcommon_mapi.la \
	${top_builddir}/common/libcommon_util.la \
	${top_builddir}/common/libcommon_ssl.la \
    ${top_builddir}/common/libcommon_service.la \
    ${top_builddir}/ECtools/zarafa-archiver/libarchiver.la \
	$(PROG_LIBS) $(SSL_LIBS) $(CLUCENE_LIBS) $(MYSQL_LIBS)

zarafa_indexer_SOURCES = \
	zarafa-indexer.cpp zarafa-indexer.h \
	ECEntryData.cpp ECEntryData.h \
	ECFileIndex.cpp ECFileIndex.h \
	ECChangeData.cpp ECChangeData.h \
	ECIndexer.cpp ECIndexer.h \
	ECIndexerData.cpp ECIndexerData.h \
	ECSynchronizationThread.cpp ECSynchronizationThread.h \
	ECSynchronization.cpp ECSynchronization.h \
	ECSynchronizationImportChanges.cpp ECSynchronizationImportChanges.h \
	ECIndexerUtil.cpp ECIndexerUtil.h \
	ECSearcher.cpp ECSearcher.h \
	ECSearcherRequest.cpp ECSearcherRequest.h \
	ECLucene.cpp ECLucene.h \
	ECLuceneIndexer.cpp ECLuceneIndexer.h \
	ECLuceneIndexerAttachment.cpp ECLuceneIndexerAttachment.h \
	ECLuceneSearcher.cpp ECLuceneSearcher.h \
	ECAnalyzers.cpp ECAnalyzers.h \
	ECDatabase.cpp ECDatabase.h \
	ECIndexDB.cpp ECIndexDB.h
	
check-syntax:
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) \
		-fsyntax-only -fmessage-length=0 ${CHK_SOURCES} -Wall -Wformat=2
