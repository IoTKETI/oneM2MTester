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
#            a--->e------<
#         b:2|    |(b:1) |
#            b<---<      |
#    b:1=>c:1|    |      |
#            c    |      |d:1=>e:1
#    c:1=>d:1|    |      |
#            d>--->------>
#Result will be: Error
MAKEPROG := ${MAKE}

all: CheckTpd 

BuildTpd:
	$(TTCN3_DIR)/bin/ttcn3_makefilegen -f $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	-t a.tpd -r -c -W -g 2>&1 | tee output

CheckTpd: BuildTpd
	if [ `grep -c "error: The active build configuration named '2' of project 'b' does not exist" ./output` -ne 1 ]; \
	then echo "Makefilegen required config test 55 failed: Overall verdict: fail" && exit 1; fi

clean:
	-rm -rf output a_binDefault e_binDefault

distclean: clean
	-rm -f *.out

.PHONY: all clean distclean BuildTpd CheckTpd

