SUBDIRS=plugins
zarafa_dagentdir = ${datadir}/zarafa-dagent/python
zarafa_dagent_SCRIPTS = mapiplugin.py  pluginmanager.py  plugintemplates.py  wraplogger.py

zarafa_spoolerdir = ${datadir}/zarafa-spooler/python
zarafa_spooler_SCRIPTS = mapiplugin.py  pluginmanager.py  plugintemplates.py  wraplogger.py

install-data-hook:
	mkdir -p ${localstatedir}/lib/zarafa/dagent/python/plugins/
	mkdir -p ${localstatedir}/lib/zarafa/spooler/python/plugins/
