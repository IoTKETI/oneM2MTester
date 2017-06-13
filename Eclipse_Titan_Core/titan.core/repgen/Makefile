##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#   Forstner, Matyas
#   Horvath, Gabriella
#   Koppany, Csaba
#   Ormandi, Matyas
#   Raduly, Csaba
#   Szabo, Janos Zoltan – initial implementation
#   Tatarka, Gabor
#
##############################################################################
# Makefile for the TTCN-3 log formatter, log merger and HTML report generator

TOP := ..
include $(TOP)/Makefile.cfg

ifndef MINGW
TARGETS := ttcn3_logmerge$(EXESUFFIX) ttcn3_logfilter$(EXESUFFIX) ttcn3_logformat$(EXESUFFIX) ttcn3_repgen$(EXESUFFIX)
endif

ORIGINATORS := parser.l logformat.l

GENERATED_SOURCES := logformat.c parser.c

STATIC_SOURCES := logfilter.c logmerge.c repgen.c

SOURCES := $(STATIC_SOURCES) $(GENERATED_SOURCES)

GENERATED_HEADERS :=

GENERATED_OTHERS := lex.backup

OBJECTS := $(patsubst %.cc,%.o,$(patsubst %.c,%.o,$(SOURCES)))

COMMON_OBJECTS := ../common/memory.o

ifeq ($(LICENSING), yes)
    COMMON_OBJECTS += ../common/license.o
endif

DEPFILES := $(patsubst %.cc,%.d,$(patsubst %.c,%.d,$(SOURCES)))

SUBDIRS :=

MANPAGES1 := ttcn3_logmerge.1 ttcn3_logfilter.1 ttcn3_logformat.1

all run: $(TARGETS)

install: $(TARGETS)
ifdef MINGW
	@echo Skipped ${CURDIR} for MinGW
else
ifeq ($(DEBUG), no)
	$(STRIP) $(TARGETS)
endif
	mkdir -p $(BINDIR)
	cp $(TARGETS) $(BINDIR)
	$(RM) $(BINDIR)/logformat$(EXESUFFIX) $(BINDIR)/repgen$(EXESUFFIX)
	ln -s ttcn3_logformat$(EXESUFFIX) $(BINDIR)/logformat$(EXESUFFIX)
	ln -s ttcn3_repgen$(EXESUFFIX) $(BINDIR)/repgen$(EXESUFFIX)
	cp $(MANPAGES1) $(MANDIR)/man1
	$(RM) $(MANDIR)/man1/logformat.1
	ln -s ttcn3_logformat.1 $(MANDIR)/man1/logformat.1
endif

ttcn3_logmerge$(EXESUFFIX): logmerge.o $(COMMON_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LICENSE_LIBS) $(MINGW_LIBS)

ttcn3_logfilter$(EXESUFFIX): logfilter.o $(COMMON_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LICENSE_LIBS) $(MINGW_LIBS)

ttcn3_logformat$(EXESUFFIX): logformat.o $(COMMON_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LICENSE_LIBS) $(MINGW_LIBS)

ttcn3_repgen$(EXESUFFIX): parser.o repgen.o $(COMMON_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LICENSE_LIBS) $(MINGW_LIBS)

logformat.c: logformat.l
	$(FLEX) $(FLEXFLAGS) -o$@ $<

parser.c: parser.l
	$(FLEX) $(FLEXFLAGS) -o$@ $<

include ../Makefile.genrules
