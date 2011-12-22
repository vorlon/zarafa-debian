SUBDIRS = linux userscripts quotamail indexerscripts

datadir = ${docdir}
dist_data_DATA = ldap/zarafa.schema zarafa.ldif licenseagreement/eula.txt licenseagreement/AGPL-3
BUILT_SOURCES = zarafa.ldif
EXTRA_DIST = ldap/ol-schema-migrate.pl

$(BUILT_SOURCES): ldap/zarafa.schema ldap/ol-schema-migrate.pl
	${top_srcdir}/installer/ldap/ol-schema-migrate.pl -b ldap/zarafa.schema > ${top_srcdir}/installer/zarafa.ldif
	
