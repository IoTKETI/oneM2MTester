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
TOPDIR := ../../..
include $(TOPDIR)/Makefile.regression

ifdef LCOV
COVERAGE_FLAG := -C
endif


MAKEPROG := ${MAKE}

all: Normal Recursive RecursiveDynamic

Normal:
	rm -rf binDefault ../b/binDefault ../c/binDefault && cd a && \
	$(TTCN3_DIR)/bin/ttcn3_makefilegen -f $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	-t a.tpd -b None && cd binDefault && make && cd .. && rm -rf binDefault && \
	$(TTCN3_DIR)/bin/ttcn3_makefilegen -f $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	-t a.tpd -b Type && cd binDefault && make && cd .. && rm -rf binDefault && \
	$(TTCN3_DIR)/bin/ttcn3_makefilegen -f $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	-t a.tpd -b Number && cd binDefault && make && cd .. && rm -rf binDefault

Recursive:
	rm -rf binDefault ../b/binDefault ../c/binDefault && cd a && \
	$(TTCN3_DIR)/bin/ttcn3_makefilegen -F $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	-t a.tpd -rgpcb None && cd binDefault && make && cd .. && \
	rm -rf binDefault ../b/binDefault ../c/binDefault && \
	$(TTCN3_DIR)/bin/ttcn3_makefilegen -F $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	-t a.tpd -rgpcb Type && cd binDefault && make && cd .. && \
	rm -rf binDefault ../b/binDefault ../c/binDefault && \
	$(TTCN3_DIR)/bin/ttcn3_makefilegen -F $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	-t a.tpd -rgpcb Number && cd binDefault && make && cd .. && \
	rm -rf binDefault ../b/binDefault ../c/binDefault

RecursiveDynamic:
	rm -rf binDefault ../b/binDefault ../c/binDefault && cd a && \
	$(TTCN3_DIR)/bin/ttcn3_makefilegen -F $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	-t a.tpd -rcpb None-Dyn && cd binDefault && make && cd .. && \
	rm -rf binDefault ../b/binDefault ../c/binDefault && \
	$(TTCN3_DIR)/bin/ttcn3_makefilegen -F $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	-t a.tpd -rcpb Type-Dyn && cd binDefault && make && cd .. && \
	rm -rf binDefault ../b/binDefault ../c/binDefault && \
	$(TTCN3_DIR)/bin/ttcn3_makefilegen -F $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	-t a.tpd -rcpb Number-Dyn && cd binDefault && make && cd .. && \
	rm -rf binDefault ../b/binDefault ../c/binDefault


clean:
	-rm -rf a/binDefault b/binDefault c/binDefault

distclean: clean
	-rm -f *.out

.PHONY: all clean distclean Normal Recursive RecursiveDynamic

