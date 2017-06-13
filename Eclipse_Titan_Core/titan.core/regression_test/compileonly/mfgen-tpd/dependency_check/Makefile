##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#   Szalay, Akos
#
##############################################################################
#Makefile to test HR30356
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

all: CheckDependencies

GenerateMakefile:
	${TTCN3_DIR}/bin/ttcn3_makefilegen -Fgrc -t "controller/controller.tpd" || true

CheckDependencies: GenerateMakefile
	$(MAKEPROG) -C controller/bin dep
	ls -R > dir.txt
	if [ `grep -c "controller.d" ./dir.txt` -ne 1 ]; then exit 1; fi

clean:
	-rm -rf client/bin
	-rm -rf controller/bin
	-rm -rf server/bin
	-rm -rf testport/bin

distclean: clean
	-rm -f dir.txt

.PHONY: all clean distclean GenerateMakefile CheckDependencies
