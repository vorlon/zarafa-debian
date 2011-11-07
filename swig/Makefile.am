# make sure common swig lib builds first
SUBDIRS = .

noinst_LTLIBRARIES = libswigcommon.la

if WITH_PERL
SUBDIRS += perl
endif

if WITH_PYTHON
SUBDIRS += python
endif

AM_CPPFLAGS = $(DEBUGFLAGS) -DLINUX -I$(top_srcdir)/mapi4linux/include -I$(top_srcdir)/provider/common -I${top_srcdir}/common

libswigcommon_la_LDFLAGS = -static

libswigcommon_la_SOURCES = MAPINotifSink.cpp MAPINotifSink.h 

EXTRA_DIST = edkmdb.i mapi.i mapiutil.i mapidefs.i mapicode.i mapix.i mapinotifsink.i typemap.i helpers.i IECMultiStoreTable.i IECServiceAdmin.i IECSpooler.i IECTestProtocol.i IECLicense.i inetmapi.i icalmapi.i swig_iunknown.h ecdefs.i libcommon.i archiver.i licenseclient.i

if ! OSS_ONLY
EXTRA_DIST += licensereq.i
endif

check-syntax:
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) \
		-fsyntax-only -fmessage-length=0 ${CHK_SOURCES} -Wall -Wformat=2
