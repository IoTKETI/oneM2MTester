##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#   Beres, Szabolcs
#   Raduly, Csaba
#
##############################################################################
TOPDIR := ../../../
include $(TOPDIR)/Makefile.regression

ifdef LCOV
COVERAGE_FLAG := -C
endif

# ${MAKEPROG} has the same content as the built-in ${MAKE},
# except the special handling of ${MAKE} does not apply.
# If we used ${MAKE} in the rules below, 'make -n' would succeed once,
# then fail on every subsequent invocation until a 'make clean' is done. 
MAKEPROG := ${MAKE}

all: CheckHelloTpd 

BuildHelloTpd:
	mkdir -p HelloTpd/bin && cd HelloTpd/bin \
	&& $(TTCN3_DIR)/bin/ttcn3_makefilegen -f $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	 -t ../HelloTpd.tpd -b test1 && ${MAKEPROG}

CheckHelloTpd: BuildHelloTpd
	if [ ! -f HelloTpd/bin/test1_config.exe ]; then exit 1; fi

clean:
	-rm -rf HelloTpd/bin 

distclean: clean
	-rm -f *.out

.PHONY: all clean distclean BuildHelloTpd CheckHelloTpd

