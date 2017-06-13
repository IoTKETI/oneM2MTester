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
#            a----->
#            |     |
#            b     d<-----<
#    b:1=>c:2|     |e:1   |
#            c<----e      |
#            |  e:1=>c:1  |
#            |            |
#            >------------>
#


#Result will be: a:Default, b:Default, c:1, d:Default, e:1
MAKEPROG := ${MAKE}

all: CheckTpd 

BuildTpd:
	$(TTCN3_DIR)/bin/ttcn3_makefilegen -F $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	-t a.tpd -r -c -W -g

CheckTpd: BuildTpd
	if [ ! -f "./a_binDefault/Makefile" ] || [ `grep -c "EXECUTABLE = binDefault" ./a_binDefault/Makefile` -ne 1 ]; \
	then echo "Makefilegen required config test 54 failed: Overall verdict: fail" && exit 1; fi
	if [ ! -f "./b_binDefault/Makefile" ] || [ `grep -c "EXECUTABLE = binDefault" ./b_binDefault/Makefile` -ne 1 ]; \
	then echo "Makefilegen required config test 54 failed: Overall verdict: fail" && exit 1; fi
	if [ ! -f "./c_bin1/Makefile" ] || [ `grep -c "EXECUTABLE = bin1" ./c_bin1/Makefile` -ne 1 ]; \
	then echo "Makefilegen required config test 54 failed: Overall verdict: fail" && exit 1; fi
	if [ ! -f "./d_binDefault/Makefile" ] || [ `grep -c "EXECUTABLE = binDefault" ./d_binDefault/Makefile` -ne 1 ]; \
	then echo "Makefilegen required config test 54 failed: Overall verdict: fail" && exit 1; fi
	if [ ! -f "./e_bin1/Makefile" ] || [ `grep -c "EXECUTABLE = bin1" ./e_bin1/Makefile` -ne 1 ]; \
	then echo "Makefilegen required config test 54 failed: Overall verdict: fail" && exit 1; fi

clean:
	-rm -rf a_binDefault b_binDefault c_bin1 d_binDefault e_bin1

distclean: clean
	-rm -f *.out

.PHONY: all clean distclean BuildTpd CheckTpd

