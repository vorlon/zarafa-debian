AM_CPPFLAGS = ${ZCPPFLAGS} $(DEBUGFLAGS) $(BOOST_CPPFLAGS) \
	-I${top_srcdir}/common -I${top_srcdir}/provider/libserver \
	-I${top_srcdir}/provider/common

COMMON_PLUGIN_FILES = plugin.h ${top_builddir}/provider/common/ZarafaUser.cpp ${top_builddir}/provider/common/ZarafaUser.h
COMMON_DBPLUGIN_FILES = DBBase.h DBBase.cpp ../libserver/ECDatabase.h 


pkglib_LTLIBRARIES = 

if WITH_UNIX
pkglib_LTLIBRARIES += unixplugin.la
unixplugin_la_CPPFLAGS = ${AM_CPPFLAGS} $(MYSQL_INCLUDES)
unixplugin_la_SOURCES = UnixUserPlugin.cpp UnixUserPlugin.h $(COMMON_PLUGIN_FILES) $(COMMON_DBPLUGIN_FILES)
unixplugin_la_LDFLAGS = ${AM_LDFLAGS} -module -avoid-version
unixplugin_la_LIBADD = ${top_builddir}/common/libcommon_util.la $(MYSQL_LIBS) $(CRYPT_LIBS)
endif

if WITH_LDAP
pkglib_LTLIBRARIES += ldapplugin.la
ldapplugin_la_CPPFLAGS = ${AM_CPPFLAGS} $(LDAP_FLAGS) -DLDAP_THREAD_SAFE
ldapplugin_la_SOURCES = LDAPUserPlugin.cpp LDAPUserPlugin.h LDAPCache.cpp LDAPCache.h ldappasswords.cpp ldappasswords.h $(COMMON_PLUGIN_FILES)
ldapplugin_la_LDFLAGS = ${AM_LDFLAGS} -module -avoid-version
ldapplugin_la_LIBADD = ${top_builddir}/common/libcommon_util.la $(LDAP_LIBS)

if ! OSS_ONLY
pkglib_LTLIBRARIES += ldapmsplugin.la
ldapmsplugin_la_CPPFLAGS = ${AM_CPPFLAGS} $(ldapplugin_la_CPPFLAGS) -DWITH_MULTISERVER
ldapmsplugin_la_SOURCES = $(ldapplugin_la_SOURCES)
ldapmsplugin_la_LDFLAGS = $(ldapplugin_la_LDFLAGS)
ldapmsplugin_la_LIBADD = $(ldapplugin_la_LIBADD)
endif

endif

# default plugin
pkglib_LTLIBRARIES += dbplugin.la
dbplugin_la_CPPFLAGS = ${AM_CPPFLAGS} $(MYSQL_INCLUDES)
dbplugin_la_SOURCES = DBUserPlugin.cpp DBUserPlugin.h $(COMMON_PLUGIN_FILES) $(COMMON_DBPLUGIN_FILES)
dbplugin_la_LDFLAGS = -module -avoid-version
dbplugin_la_LIBADD = ${top_builddir}/common/libcommon_util.la $(MYSQL_LIBS)

check-syntax:
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) $(LDAP_FLAGS) \
		-fsyntax-only -fmessage-length=0 ${CHK_SOURCES} -Wall -Wformat=2
