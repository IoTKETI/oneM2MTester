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

#This tests that 'a' references 'b' and 'a' requires
#'b''s '1' configuration and 'c''s '1' configuration.
#      a
#     / \
# b:1/   \c:1
#   /     \
#  b       c

MAKEPROG := ${MAKE}

all: CheckTpd 

BuildTpd:
	$(TTCN3_DIR)/bin/ttcn3_makefilegen -f $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	-t a/a.tpd -FWrZHc

CheckTpd: BuildTpd
	if [ ! -f "./a/a_binDefault/Makefile" ] || [ ! -L "./a/a_binDefault/aDefault.ttcn" ] || [ -L "./a/a_binDefault/a1.ttcn" ]; \
	then echo "Makefilegen required config test 3 failed: Overall verdict: fail" && exit 1; fi
	if [ ! -f "./b/b_bin1/Makefile" ] || [ ! -L "./b/b_bin1/b1.ttcn" ] || [ -L "./b/b_bin1/bDefault.ttcn" ]; \
	then echo "Makefilegen required config test 3 failed: Overall verdict: fail" && exit 1; fi
	if [ ! -f "./c/c_bin1/Makefile" ] || [ ! -L "./c/c_bin1/c1.ttcn" ] || [ -L "./c/c_bin1/cDefault.ttcn" ]; \
	then echo "Makefilegen required config test 3 failed: Overall verdict: fail" && exit 1; fi

clean:
	-rm -rf a/a_binDefault b/b_bin1 c/c_bin1

distclean: clean
	-rm -f *.out

.PHONY: all clean distclean BuildTpd CheckTpd

