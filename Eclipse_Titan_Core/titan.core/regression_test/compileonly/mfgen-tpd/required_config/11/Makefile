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

#This tests that 'a' references 'b' and 'b' references 'c' and 'a' requires
#'b''s '1' configuration and 'c''s '2' configuration but 'c''s '2' configuration
#does not exists.
#Additionally and 'b''s '1' configuration requires 'c''s '1' config.
#This tests that two different configuration was required for 'c' but
#one of them does not exitst so a different error message produced than in test 10.
#      a
#     / \
# b:1/   \c:2 (not exist)
#   /     \
#  b------>c
#   b:1=>c:1

MAKEPROG := ${MAKE}

all: CheckTpd 

BuildTpd:
	$(TTCN3_DIR)/bin/ttcn3_makefilegen -f $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	-t a.tpd -r -c -W -g 2>&1 | tee output

CheckTpd: BuildTpd
	if [ `grep -c "error: The active build configuration named '2' of project 'c' does not exist" ./output` -ne 1 ]; \
	then echo "Makefilegen required config test 11 failed: Overall verdict: fail" && exit 1; fi

clean:
	-rm -rf output a_binDefault b_bin1

distclean: clean
	-rm -f *.out

.PHONY: all clean distclean BuildTpd CheckTpd

