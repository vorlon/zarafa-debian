noinst_LTLIBRARIES = libgsoap.la

AM_CPPFLAGS = ${ZCPPFLAGS}

#GSOAP_DEBUGFLAGS=-DDEBUG
# disable warnings since code is mostly generated
libgsoap_la_CPPFLAGS = ${AM_CPPFLAGS} -DLINUX $(GSOAP_CFLAGS) $(GSOAP_DEBUGFLAGS) -DWITH_SOAPDEFS_H -I${top_srcdir}/common -w
libgsoap_la_SOURCES = stdsoap2.cpp stdsoap2.h soapdefs.h
