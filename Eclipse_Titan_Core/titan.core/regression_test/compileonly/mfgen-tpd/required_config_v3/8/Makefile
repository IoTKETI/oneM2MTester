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
#            a--->e------------->
#            |                  |
#            |                  |
#            b<-------<         |
#            |        |         |f:1
#            c        |f:1=>b:1 |
#            |        |         |
#            d        <---------f
# d refers both b and e
#Result will be: a:Default, b:1, c:Default, d:Default, e:Default, f:1
MAKEPROG := ${MAKE}

all: CheckTpd 

BuildTpd:
	$(TTCN3_DIR)/bin/ttcn3_makefilegen $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	-t a/a.tpd -FWrZH

CheckTpd: BuildTpd
	if [ ! -f "./a/a_binDefault/Makefile" ] || [ ! -L "./a/a_binDefault/aDefault.ttcn" ] || [ -L "./a/a_binDefault/a1.ttcn" ]; \
	then echo "Makefilegen required config test 8 failed: Overall verdict: fail" && exit 1; fi
	if [ ! -f "./b/b_bin1/Makefile" ] || [ ! -L "./b/b_bin1/b1.ttcn" ] || [ -L "./b/b_bin1/bDefault.ttcn" ]; \
	then echo "Makefilegen required config test 8 failed: Overall verdict: fail" && exit 1; fi
	if [ ! -f "./c/c_binDefault/Makefile" ] || [ ! -L "./c/c_binDefault/cDefault.ttcn" ] || [ -L "./c/c_binDefault/c1.ttcn" ]; \
	then echo "Makefilegen required config test 8 failed: Overall verdict: fail" && exit 1; fi
	if [ ! -f "./d/d_binDefault/Makefile" ] || [ ! -L "./d/d_binDefault/dDefault.ttcn" ] || [ -L "./d/d_binDefault/d1.ttcn" ]; \
	then echo "Makefilegen required config test 8 failed: Overall verdict: fail" && exit 1; fi
	if [ ! -f "./e/e_binDefault/Makefile" ] || [ ! -L "./e/e_binDefault/eDefault.ttcn" ] || [ -L "./e/e_binDefault/e1.ttcn" ]; \
	then echo "Makefilegen required config test 8 failed: Overall verdict: fail" && exit 1; fi
	if [ ! -f "./f/f_bin1/Makefile" ] || [ ! -L "./f/f_bin1/f1.ttcn" ] || [ -L "./f/f_bin1/fDefault.ttcn" ]; \
	then echo "Makefilegen required config test 8 failed: Overall verdict: fail" && exit 1; fi

clean:
	-rm -rf a/a_binDefault b/b_bin1 c/c_binDefault d/d_binDefault e/e_binDefault f/f_bin1

distclean: clean
	-rm -f *.out

.PHONY: all clean distclean BuildTpd CheckTpd

