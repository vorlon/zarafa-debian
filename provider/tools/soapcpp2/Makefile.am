AM_YFLAGS=-d -v
AM_LFLAGS=$(LEX_FLAGS)

noinst_PROGRAMS=soapcpp2

# platform=LINUX
soapcpp2_CFLAGS=$(BISON_DEFINE) $(LEX_DEFINE) $(C_DEBUG_FLAGS)
soapcpp2_LDADD=$(YACC_LIB) $(LEXLIB)
soapcpp2_SOURCES= soapcpp2_yacc.y soapcpp2_lex.l symbol2.c error2.c init2.c soapcpp2.c soapcpp2.h error2.h
# make sure build waits for this file
BUILT_SOURCES=soapcpp2_yacc.h

CLEANFILES= *~ soapcpp2_lex.c soapcpp2_yacc.c soapcpp2_yacc.h y.output soapcpp2_yacc.output
