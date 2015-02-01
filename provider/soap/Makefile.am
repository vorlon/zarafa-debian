noinst_LTLIBRARIES = libsoapserver.la libsoapclient.la 

AM_CPPFLAGS = ${ZCPPFLAGS}

#libsoapserver_la_CXXFLAGS = ${AM_CXXFLAGS} -Wno-vla -Wno-sign-compare $(DEBUGFLAGS) $(GSOAP_CFLAGS) -I${top_srcdir}/mapi4linux/include -I${top_srcdir}/provider/include ${GSOAP_CFLAGS}
#libsoapclient_la_CXXFLAGS = ${AM_CXXFLAGS} -Wno-vla -Wno-sign-compare $(DEBUGFLAGS) $(GSOAP_CFLAGS) -I${top_srcdir}/mapi4linux/include -I${top_srcdir}/provider/include ${GSOAP_CFLAGS}

libsoapserver_la_CXXFLAGS = ${AM_CXXFLAGS} -Wno-sign-compare $(DEBUGFLAGS) $(GSOAP_CFLAGS) -I${top_srcdir}/mapi4linux/include -I${top_srcdir}/provider/include ${GSOAP_CFLAGS}
libsoapclient_la_CXXFLAGS = ${AM_CXXFLAGS} -Wno-sign-compare $(DEBUGFLAGS) $(GSOAP_CFLAGS) -I${top_srcdir}/mapi4linux/include -I${top_srcdir}/provider/include ${GSOAP_CFLAGS}

nodist_libsoapserver_la_SOURCES = soapC.cpp soapServer.cpp
nodist_libsoapclient_la_SOURCES = soapC.cpp soapClient.cpp

SOAPHEADER = ${top_srcdir}/provider/include/ZarafaProto.h
SOAP_INC = ${top_srcdir}/provider/include/
SOAP_NS = ZarafaCmd.nsmap ZarafaCmd.wsdl

SOAP_H_FILES = soapH.h soapStub.h
SOAP_CPP_CLIENT = soapClient.cpp soapC.cpp
SOAP_LIB_FILES = soapClientLib.cpp soapServerLib.cpp soapZarafaCmdObject.h
SOAP_CPP_SRC=$(SOAP_CPP_CLIENT) soapServer.cpp $(SOAP_H_FILES) $(SOAP_NS) $(SOAP_LIB_FILES)

$(SOAP_CPP_SRC) : ns.xsd
ns.xsd: $(SOAPHEADER)
	$(GSOAP_SOAPCPP2) -x -I$(SOAP_INC) $< >& /dev/null

#libsoapserver_a_LDADD=$(SOAP_CPP_LIB)
BUILT_SOURCES=ns.xsd $(SOAP_CPP_SRC)
CLEANFILES=$(SOAP_CPP_SRC) *.req.xml *.res.xml *.wsdl *.nsmap *.xsd *Proxy\.h *~

check-syntax:
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) \
		-fsyntax-only -fmessage-length=0 ${CHK_SOURCES} -Wall -Wformat=2
