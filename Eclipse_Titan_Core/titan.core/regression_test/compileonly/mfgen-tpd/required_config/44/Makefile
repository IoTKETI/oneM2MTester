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
#          /   \
#         /     \h:Default
#         b      c
# b:1=>d:1\      |e:1
#          \     |  e:1=>f:1   f:1=>g:1
#           d--->e----------->f--------->g
#                |
#                |e:1=>h:1
#                h
#Result will be: a:Default, b:Default, c:Default, d:Default, e:1, f:1, g:1, h:1

MAKEPROG := ${MAKE}

all: CheckTpd 

BuildTpd:
	$(TTCN3_DIR)/bin/ttcn3_makefilegen -f $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	-t a.tpd -r -c -W -g 2>&1 | tee output

CheckTpd: BuildTpd
	if [ `grep -c "error: Required configuration is inconsistent or circular : Project 'h' cannot have 2 different configuration 'Default' and '1'" ./output` -ne 1 ]; \
	then echo "Makefilegen required config test 44 failed: Overall verdict: fail" && exit 1; fi

clean:
	-rm -rf output a_binDefault b_binDefault c_binDefault d_binDefault e_bin1 f_bin1 g_bin1 h_bin1

distclean: clean
	-rm -f *.out

.PHONY: all clean distclean BuildTpd CheckTpd

