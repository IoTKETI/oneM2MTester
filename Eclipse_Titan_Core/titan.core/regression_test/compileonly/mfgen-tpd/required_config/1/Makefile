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

#This tests that the -b goodConfig exists and it is being used.

MAKEPROG := ${MAKE}

all: CheckTpd 

BuildTpd:
	$(TTCN3_DIR)/bin/ttcn3_makefilegen -f $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	-t a.tpd -r -c -b goodConfig

CheckTpd: BuildTpd
	if [ ! -f "./binGood/Makefile" ] || [ `grep -c "EXECUTABLE = binGood" ./binGood/Makefile` -ne 1 ]; \
	then echo "Makefilegen required config test 1 failed: Overall verdict: fail" && exit 1; fi

clean:
	-rm -rf binGood

distclean: clean
	-rm -f *.out

.PHONY: all clean distclean BuildTpd CheckTpd

