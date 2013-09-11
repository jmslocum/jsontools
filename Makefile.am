lib_LTLIBRARIES = libjsontools.la
libjsontools_la_SOURCES = jsonbuilder.c jsonerror.c jsonhelper.c jsonoutput.c jsonparser.c jsonbuilder.h jsoncommon.h jsonerror.h jsonhelper.h jsonoutput.h jsonparser.h jsontools.h

libjsontools_la_LDFLAGS = -version-info 2:0:0
include_HEADERS = jsonbuilder.h jsoncommon.h jsonerror.h jsonhelper.h jsonoutput.h jsonparser.h jsontools.h

bin_PROGRAMS = jsontools
jsontools_SOURCES = jsontools.c jsontools.h
jsontools_LDADD = libjsontools.la

CFLAGS += --std=gnu99

