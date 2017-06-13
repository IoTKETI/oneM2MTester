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
TOPDIR := ../..
include $(TOPDIR)/Makefile.regression

ifdef LCOV
COVERAGE_FLAG := -C
endif

MFGEN := $(TTCN3_DIR)/bin/ttcn3_makefilegen
MFGEN_FLAGS := -sf $(RT2_FLAG) $(SPLIT_FLAG)

FOLDER := folder-with-hyphen
FILES := $(FOLDER)/ASN1Module1.asn


# ${MAKEPROG} has the same content as the built-in ${MAKE},
# except the special handling of ${MAKE} does not apply.
# If we used ${MAKE} in the rules below, 'make -n' would succeed once,
# then fail on every subsequent invocation until a 'make clean' is done. 
MAKEPROG := ${MAKE}

all:
	$(MFGEN) $(MFGEN_FLAGS) -o GenMakefile $(FILES) && rm GenMakefile


.PHONY: all clean distclean run

