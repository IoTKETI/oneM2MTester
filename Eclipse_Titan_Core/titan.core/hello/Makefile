##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#   Delic, Adam
#   Forstner, Matyas
#   Kovacs, Ferenc
#   Lovassy, Arpad
#   Raduly, Csaba
#   Szabo, Bence Janos
#   Szabo, Janos Zoltan – initial implementation
#
##############################################################################
# Makefile for installing the "Hello World!" example.

TOP := $(shell cd .. && pwd)

include ../Makefile.cfg

DEMOFILES := MyExample.ttcn PCOType.hh PCOType.cc hello_world.ttcn MyExample.cfg

MAKEFILEGENFLAGS := -g
ifeq ($(COVERAGE), yes)
MAKEFILEGENFLAGS += -C
endif

install:
ifdef MINGW
	@echo Skipped ${CURDIR} for MinGW
else
	mkdir -p $(DEMODIR)
	cp $(DEMOFILES) $(DEMODIR)
	cd $(DEMODIR) && $(BINDIR)/ttcn3_makefilegen $(MAKEFILEGENFLAGS) \
	-o Makefile.tmp $(DEMOFILES) && \
	sed -e 's/# TTCN3_DIR =/TTCN3_DIR =/' <Makefile.tmp >Makefile && \
	$(RM) Makefile.tmp && \
	$(MAKE) CXX='$(CXX)' CXXFLAGS='$(CXXFLAGS)' LDFLAGS='$(LDFLAGS)' && \
	(sleep 5 && echo 'Hello, TTCN-3!') | ttcn3_start MyExample MyExample.cfg && \
	logformat -o MyExample_merged.log *.log
	cd $(DEMODIR) && make clean
endif
