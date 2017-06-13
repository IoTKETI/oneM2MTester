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
#            a--->e-------->
#            |    |        |
#            |    |        |
#            b<---|---<    |
#            |    |   |    |f:1
#            c    |   |b:1 |
#            |    |   |    |
#            d>--->   <----f
# d refers both b and e
#Result will be: a:Default, b:1, c:Default, d:Default, e:Default, f:1
MAKEPROG := ${MAKE}

all: CheckTpd 

BuildTpd:
	$(TTCN3_DIR)/bin/ttcn3_makefilegen -F $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	-t a.tpd -r -c -W -g

CheckTpd: BuildTpd
	if [ ! -f "./a_binDefault/Makefile" ] || [ `grep -c "EXECUTABLE = binDefault" ./a_binDefault/Makefile` -ne 1 ]; \
	then echo "Makefilegen required config test 32 failed: Overall verdict: fail" && exit 1; fi
	if [ ! -f "./b_bin1/Makefile" ] || [ `grep -c "EXECUTABLE = bin1" ./b_bin1/Makefile` -ne 1 ]; \
	then echo "Makefilegen required config test 32 failed: Overall verdict: fail" && exit 1; fi
	if [ ! -f "./c_binDefault/Makefile" ] || [ `grep -c "EXECUTABLE = binDefault" ./c_binDefault/Makefile` -ne 1 ]; \
	then echo "Makefilegen required config test 32 failed: Overall verdict: fail" && exit 1; fi
	if [ ! -f "./d_binDefault/Makefile" ] || [ `grep -c "EXECUTABLE = binDefault" ./d_binDefault/Makefile` -ne 1 ]; \
	then echo "Makefilegen required config test 32 failed: Overall verdict: fail" && exit 1; fi
	if [ ! -f "./e_binDefault/Makefile" ] || [ `grep -c "EXECUTABLE = binDefault" ./e_binDefault/Makefile` -ne 1 ]; \
	then echo "Makefilegen required config test 32 failed: Overall verdict: fail" && exit 1; fi
	if [ ! -f "./f_bin1/Makefile" ] || [ `grep -c "EXECUTABLE = bin1" ./f_bin1/Makefile` -ne 1 ]; \
	then echo "Makefilegen required config test 32 failed: Overall verdict: fail" && exit 1; fi

clean:
	-rm -rf a_binDefault b_bin1 c_binDefault d_binDefault e_binDefault f_bin1

distclean: clean
	-rm -f *.out

.PHONY: all clean distclean BuildTpd CheckTpd

