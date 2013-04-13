SUBDIRS=plugins

EXTRA_DIST = mapiplugin.py pluginmanager.py plugintemplates.py wraplogger.py wraplogger.py zconfig.py zinterval.py zunit.py

zarafa_dagentdir = ${datadir}/zarafa-dagent/python
zarafa_dagent_SCRIPTS = mapiplugin.py pluginmanager.py plugintemplates.py wraplogger.py zconfig.py zinterval.py zunit.py

zarafa_spoolerdir = ${datadir}/zarafa-spooler/python
zarafa_spooler_SCRIPTS = mapiplugin.py pluginmanager.py plugintemplates.py wraplogger.py zconfig.py zinterval.py zunit.py

install-data-hook:
	mkdir -p ${DESTDIR}/${localstatedir}/lib/zarafa/dagent/plugins/
	mkdir -p ${DESTDIR}/${localstatedir}/lib/zarafa/spooler/plugins/
