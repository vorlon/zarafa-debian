ACLOCAL_AMFLAGS = -I autoconf

if ! OSS_ONLY
PROSUBS = liblicense licensed
endif

SUBDIRS = common libfreebusy mapi4linux zarafa-libsync $(PROSUBS) provider libicalmapi inetmapi php-ext ECtools spooler gateway caldav installer po doc

if WITH_SWIG
SUBDIRS += swig
else
if WITH_PYTHON
SUBDIRS += swig
endif
endif

EXTRA_DIST = version versionheader.sh common/ecversion.h php-webclient-ajax

BUILT_SOURCES = common/ecversion.h

common/ecversion.h: versionheader.sh
	sh ${top_srcdir}/versionheader.sh > common/ecversion.h

if ! OSS_ONLY
CLEANFILES=common/ecversion.h
endif

dist-hook:
	find $(distdir) -type d -name .svn -exec rm -rf {} \; 2>/dev/null || true
if OSS_ONLY
	[ -x $(top_srcdir)/fixup-distro.sh ] && $(top_srcdir)/fixup-distro.sh $(distdir)
endif

install-ajax-webaccess:
	install -d -m 755 $(DESTDIR)/usr/share
	cp -a $(top_srcdir)/php-webclient-ajax $(DESTDIR)/usr/share/zarafa-webaccess
	rm -rf $(DESTDIR)/usr/share/zarafa-webaccess/plugins
	ln -sf /var/lib/zarafa-webaccess/plugins $(DESTDIR)/usr/share/zarafa-webaccess/plugins
	install -d -m 755 $(DESTDIR)/var/lib/zarafa-webaccess/plugins
	install -d -m 755 $(DESTDIR)/var/lib/zarafa-webaccess/tmp
	install -d -m 755 $(DESTDIR)/etc/zarafa/webaccess-ajax
	mv $(DESTDIR)/usr/share/zarafa-webaccess/config.php.dist $(DESTDIR)/etc/zarafa/webaccess-ajax/config.php
	ln -sf /etc/zarafa/webaccess-ajax/config.php $(DESTDIR)/usr/share/zarafa-webaccess/config.php
	rm -f $(DESTDIR)/usr/share/zarafa-webaccess/debug.php
	for podir in $(DESTDIR)/usr/share/zarafa-webaccess/server/language/*; do \
		if [ `grep msgstr $$podir/LC_MESSAGES/*po | grep -v 'msgstr ""' |wc -l` = 0 ]; then \
			echo "Remove empty language $$podir"; \
			rm -rf $$podir; \
		fi; \
	done; \
	for po in $(DESTDIR)/usr/share/zarafa-webaccess/server/language/*/LC_MESSAGES/*.po; do \
		msgfmt -f -v -o $${po%.po}.mo $$po; \
		rm $$po; \
	done
