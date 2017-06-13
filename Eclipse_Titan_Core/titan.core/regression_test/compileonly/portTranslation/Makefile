##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Szabo, Bence Janos
#
##############################################################################
TOPDIR := ../..
include $(TOPDIR)/Makefile.regression

ifdef LCOV
COVERAGE_FLAG := -C
endif


MAKEPROG := ${MAKE}

all: 
	rm -rf bin && mkdir -p bin && cd bin && $(TTCN3_COMPILER) -t ../*.ttcn && $(TTCN3_DIR)/bin/ttcn3_makefilegen -f ../*.ttcn *.cc *.hh && make


clean:
	-rm -rf bin

distclean: clean
	-rm -f *.out

.PHONY: all clean distclean Normal Recursive RecursiveDynamic

