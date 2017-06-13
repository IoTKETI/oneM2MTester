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
#   Raduly, Csaba
#   Szabo, Janos Zoltan – initial implementation
#
##############################################################################
# Makefile for the langviz

include ../Makefile.cfg

TARGETS := langviz$(EXESUFFIX)

GENERATED_SOURCES := bison_la.yy.cc bison_p.tab.cc

SOURCES := main.cc error.c Node.cc Symbol.cc Rule.cc Grammar.cc Iterator.cc Graph.cc $(GENERATED_SOURCES)

GENERATED_HEADERS := bison_p.tab.hh

GENERATED_OTHERS := bison_p.output lex.backup

OBJECTS := $(patsubst %.cc,%.o,$(patsubst %.c,%.o,$(SOURCES)))

COMMON_OBJECTS := ../common/memory.o ../common/new.o ../compiler2/string.o

DEPFILES := $(patsubst %.cc,%.d,$(patsubst %.c,%.d,$(SOURCES)))

SUBDIRS :=

.PHONY:

all run: $(TARGETS)

install: $(TARGETS)
ifeq ($(DEBUG), no)
	$(STRIP) $(TARGETS)
endif
	mkdir -p $(BINDIR)
	cp $(TARGETS) $(BINDIR)

langviz$(EXESUFFIX): $(OBJECTS) $(COMMON_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJECTS) $(COMMON_OBJECTS)

bison_la.yy.cc: bison_la.l
	$(FLEX) $(FLEXFLAGS) -o$@ $<

bison_p.tab.cc bison_p.tab.hh: bison_p.y
	$(BISON) $(BISONFLAGS) $<

include ../Makefile.genrules
