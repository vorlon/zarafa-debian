noinst_LTLIBRARIES = libserver.la

AM_CPPFLAGS = ${ZCPPFLAGS} $(DEBUGFLAGS) $(BOOST_CPPFLAGS) \
	-I${top_srcdir}/mapi4linux/include \
	-I${top_srcdir}/provider/libserver -I${top_srcdir}/provider/plugins \
	-I${top_srcdir}/provider/include -I${top_srcdir}/provider/common \
	-I${top_srcdir}/provider/client -I${top_builddir}/provider/soap \
	-I${top_srcdir}/provider/soap \
	-I${top_srcdir}/common -DPKGLIBDIR='"${pkglibdir}"' \
	$(MYSQL_INCLUDES) $(GSOAP_CFLAGS) $(SSL_CFLAGS) $(SSL_HAS_EVP_PKEY_CMP) $(KRB5_CFLAGS)

libserver_la_SOURCES = \
	ZarafaCmd.cpp \
	ECABObjectTable.cpp ECABObjectTable.h \
	ECCacheManager.cpp ECCacheManager.h \
	ECDBDef.h ECDatabase.h \
	ECDatabaseFactory.cpp ECDatabaseFactory.h \
	ECDatabaseMySQL.cpp ECDatabaseMySQL.h \
	ECDatabaseUtils.cpp ECDatabaseUtils.h \
	ECDatabaseUpdate.cpp ECDatabaseUpdate.h \
	ECGenProps.cpp ECGenProps.h \
	ECGenericObjectTable.cpp ECGenericObjectTable.h \
	ECICS.cpp ECICS.h \
	ECICSHelpers.cpp ECICSHelpers.h \
	ECMAPI.h \
	ECNotification.cpp ECNotification.h \
	ECPluginFactory.cpp ECPluginFactory.h \
	ECPluginSharedData.cpp ECPluginSharedData.h \
	ECPamAuth.cpp ECPamAuth.h \
	ECKrbAuth.cpp ECKrbAuth.h \
	ECSecurity.cpp ECSecurity.h \
	ECServerEntrypoint.cpp ECServerEntrypoint.h \
	ECSession.cpp ECSession.h \
	ECSessionGroup.cpp ECSessionGroup.h \
	ECSessionManager.cpp ECSessionManager.h \
	ECStoreObjectTable.cpp ECStoreObjectTable.h \
	ECStringCompat.cpp ECStringCompat.h \
	ECSubRestriction.cpp ECSubRestriction.h \
	ECTableManager.cpp ECTableManager.h \
	ECUserManagement.cpp ECUserManagement.h \
	ECSessionManagerOffline.cpp ECSessionManagerOffline.h \
	ECUserManagementOffline.cpp ECUserManagementOffline.h \
	ECSecurityOffline.cpp ECSecurityOffline.h \
	ECSearchFolders.cpp ECSearchFolders.h \
	ECSearchObjectTable.cpp ECSearchObjectTable.h \
	ECConvenientDepthObjectTable.cpp ECConvenientDepthObjectTable.h \
	ECConvenientDepthABObjectTable.cpp ECConvenientDepthABObjectTable.h \
	ECMultiStoreTable.cpp ECMultiStoreTable.h \
	ECUserStoreTable.cpp ECUserStoreTable.h \
	ECConversion.cpp ECConversion.h \
	ECAttachmentStorage.cpp ECAttachmentStorage.h \
	ECStatsCollector.cpp ECStatsCollector.h \
	ECStatsTables.cpp ECStatsTables.h \
	ECNotificationManager.cpp ECNotificationManager.h \
	ECIndexer.cpp ECIndexer.h \
	StreamUtil.cpp StreamUtil.h \
	StorageUtil.cpp StorageUtil.h \
	ECTPropsPurge.cpp ECTPropsPurge.h \
	ECMailBoxTable.cpp ECMailBoxTable.h \
	ECTestProtocol.cpp ECTestProtocol.h \
	ZarafaCmdUtil.cpp ZarafaCmdUtil.h \
	ECLockManager.cpp ECLockManager.h

check-syntax:
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) \
		-fsyntax-only -fmessage-length=0 ${CHK_SOURCES} -Wall -Wformat=2
