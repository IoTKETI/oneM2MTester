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
MFGEN_FLAGS := -fs $(RT2_FLAG) $(SPLIT_FLAG) -e Main
COMPILER := $(TTCN3_DIR)/bin/ttcn3_compiler
COMPILER_FLAGS := $(RT2_FLAG) $(SPLIT_FLAG)

# ${MAKEPROG} has the same content as the built-in ${MAKE},
# except the special handling of ${MAKE} does not apply.
# If we used ${MAKE} in the rules below, 'make -n' would succeed once,
# then fail on every subsequent invocation until a 'make clean' is done. 
MAKEPROG := ${MAKE}

all: compiler makefile

compiler:
	mkdir -p bin && cp compiler_files1.txt bin/files.txt && cd bin \
	&& $(COMPILER) $(COMPILER_FLAGS) -J files.txt ../src/ASN1Module3.asn \
	&& cd .. && rm -rf bin

	mkdir -p bin && cp compiler_files2.txt bin/files.txt && cd bin \
	&& $(COMPILER) $(COMPILER_FLAGS) -J files.txt ../src/ASN1Module3.asn \
	&& cd .. && rm -rf bin

makefile:
	mkdir -p bin && cp mfgen_files1.txt bin/files.txt && cd bin \
	&& $(MFGEN) $(MFGEN_FLAGS) -J files.txt ../src/ASN1Module3.asn \
	&& $(MAKEPROG) && ./Main && make clean && cd .. && rm -rf bin

	mkdir -p bin && cp mfgen_files2.txt bin/files.txt && cd bin \
	&& $(MFGEN) $(MFGEN_FLAGS) -J files.txt ../src/ASN1Module3.asn \
	&& $(MAKEPROG) && ./Main && make clean && cd .. && rm -rf bin

.PHONY: all clean distclean run compiler makefile
