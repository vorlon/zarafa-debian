bin_PROGRAMS = zarafa-search

AM_CPPFLAGS = ${ZCPPFLAGS} \
	-I${top_srcdir}/mapi4linux/include \
	-I${top_srcdir}/common \
	-I${top_srcdir}/provider/include \
	-I${top_srcdir}/ECtools/zarafa-archiver \
	$(DEBUGFLAGS) ${CLUCENE_CFLAGS} $(BOOST_CPPFLAGS) -D$(CLUCENE_VERSION)

zarafa_search_LDADD = \
	${top_builddir}/mapi4linux/src/libmapi.la \
	${top_builddir}/common/libcommon_mapi.la \
	${top_builddir}/common/libcommon_util.la \
	${top_builddir}/common/libcommon_ssl.la \
    ${top_builddir}/common/libcommon_service.la \
    ${top_builddir}/ECtools/zarafa-archiver/libarchiver.la \
	$(PROG_LIBS) $(SSL_LIBS) $(KYOTOCABINET_LIBS) $(CLUCENE_LIBS) \
	$(BOOST_SYSTEM_LIBS) $(BOOST_FILESYSTEM_LIBS)

zarafa_search_SOURCES = \
	zarafa-search.cpp zarafa-search.h \
	ECIndexer.cpp ECIndexer.h \
	ECServerIndexer.cpp ECServerIndexer.h \
	ECIndexerUtil.cpp ECIndexerUtil.h \
	ECSearcher.cpp ECSearcher.h \
	ECSearcherRequest.cpp ECSearcherRequest.h \
	ECIndexImporterAttachments.cpp ECIndexImporterAttachments.h \
	ECLuceneSearcher.cpp ECLuceneSearcher.h \
	ECAnalyzers.cpp ECAnalyzers.h \
	ECIndexDB.cpp ECIndexDB.h \
	ECIndexFactory.cpp ECIndexFactory.h \
	ECIndexImporter.cpp ECIndexImporter.h \
	ECIndexOptimizer.cpp ECIndexOptimizer.h

check-syntax:
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) \
		-fsyntax-only -fmessage-length=0 ${CHK_SOURCES} -Wall -Wformat=2
