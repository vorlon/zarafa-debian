EXTRA_DIST = setup.py		\
	zarafa/__init__.py \
	zarafa/lockfile.py \
	zarafa/daemon/LICENSE \
	zarafa/daemon/__init__.py \
	zarafa/daemon/daemon.py \
	zarafa/daemon/pidlockfile.py \
	zarafa/daemon/runner.py \
	zarafa/daemon/version/__init__.py \
	zarafa/daemon/version/version_info.py


.NOTPARALLEL: install-exec-local
install-exec-local:
	cd ${top_srcdir}/swig/python/zarafa/ && ${PYTHON} setup.py install --root="$(DESTDIR)" --prefix="$(prefix)" --install-purelib $(pythonexeclibdir)
