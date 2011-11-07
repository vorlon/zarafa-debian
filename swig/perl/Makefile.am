lib_LTLIBRARIES = MAPICore.la

libdir=$(PERL_INSTDIR)/auto/MAPICore
datadir=$(PERL_INSTDIR)

# disable warnings since code is mostly generated
AM_CPPFLAGS = $(DEBUGFLAGS) -DLINUX -I$(top_srcdir)/mapi4linux/include -I$(top_srcdir)/provider/common -I${top_srcdir}/common -I$(PERL_ARCHLIBDIR)/CORE -DPERL_GCC_PEDANTIC -I${top_srcdir}/swig -w

MAPICore_la_LDFLAGS = -module -avoid-version
MAPICore_la_LIBADD = ${top_builddir}/mapi4linux/src/libmapi.la \
						${top_builddir}/common/libcommon_mapi.la \
						${top_builddir}/common/libcommon_util.la \
						${top_builddir}/swig/libswigcommon.la \
						$(PROG_LIBS)

CLEANFILES = mapi_wrap.cxx MAPICore.pm
BUILT_SOURCES = mapi_wrap.cxx

# wait for full output of swig
MAPICore.pm: $(BUILT_SOURCES)

MAPICore_la_SOURCES = conversion.h conversion.cpp ../director_util.h ../director_util.cpp $(BUILT_SOURCES)

EXTRA_DIST = typemap_perl.i helpers_perl.i $(BUILT_SOURCES) MAPICore.pm MAPI.pm

mapi_wrap.cxx: ../mapi.i ../mapiutil.i ../mapidefs.i ../mapicode.i ../mapix.i ../mapinotifsink.i ../mapiutil.i ../typemap.i ../helpers.i ../edkmdb.i ../IECServiceAdmin.i typemap_perl.i helpers_perl.i
	if test "x$(SWIG_EXEC)" = "x"; then echo "swig is required for 'make dist'"; exit 1; fi
	$(SWIG_EXEC) $(SWIG_FLAGS) -Wall -const -shadow -perl5 -c++ -o ${top_builddir}/swig/perl/mapi_wrap.cxx ${top_srcdir}/swig/mapi.i 

check-syntax:
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) \
		-fsyntax-only -fmessage-length=0 ${CHK_SOURCES} -Wall -Wformat=2
