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
TOPDIR := ../../../..
include $(TOPDIR)/Makefile.regression

ifdef LCOV
COVERAGE_FLAG := -C
endif

# ${MAKEPROG} has the same content as the built-in ${MAKE},
# except the special handling of ${MAKE} does not apply.
# If we used ${MAKE} in the rules below, 'make -n' would succeed once,
# then fail on every subsequent invocation until a 'make clean' is done. 

# Hierarchy
#            a
#            |
#            b<---<
#         c:1|    |
#            c    |d:Default=>b:1
#    c:1=>d:1|    |
#            d>--->
#Result will be: a:Default, b:Default, c:1, d:1
MAKEPROG := ${MAKE}

all: CheckTpd 

BuildTpd:
	$(TTCN3_DIR)/bin/ttcn3_makefilegen -f $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	-t a.tpd -g

CheckTpd: BuildTpd
	if [ ! -f "./binDefault/Makefile" ] || [ ! -L "./binDefault/aDefault.ttcn" ] || [ -L "./binDefault/a1.ttcn" ]; \
	then echo "Makefilegen required config test 4 failed: Overall verdict: fail" && exit 1; fi
	if [ ! -L "./binDefault/bDefault.ttcn" ] || [ -L "./binDefault/b1.ttcn" ]; \
	then echo "Makefilegen required config test 4 failed: Overall verdict: fail" && exit 1; fi
	if [ ! -L "./binDefault/c1.ttcn" ] || [ -L "./binDefault/cDefault.ttcn" ]; \
	then echo "Makefilegen required config test 4 failed: Overall verdict: fail" && exit 1; fi
	if [ ! -L "./binDefault/d1.ttcn" ] || [ -L "./binDefault/dDefault.ttcn" ]; \
	then echo "Makefilegen required config test 4 failed: Overall verdict: fail" && exit 1; fi

clean:
	-rm -rf binDefault

distclean: clean
	-rm -f *.out

.PHONY: all clean distclean BuildTpd CheckTpd

