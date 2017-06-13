##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Baji, Laszlo
#   Balasko, Jeno
#   Raduly, Csaba
#   Szabados, Kristof
#   Szabo, Bence Janos
#
##############################################################################
#only works with RT2
RT2=1

TOPDIR := ../..
include   ../../Makefile.regression

ifndef RT2
$(error note: XML/NegativeTest requires RT2)
endif

# WARNING! This Makefile can be used with GNU make only.
# Other versions of make may report syntax errors in it.

#
# Do NOT touch this line...
#
.PHONY: all archive check clean distclean dep objects

.SUFFIXES: .d

# Flags for dependency generation
CXXDEPFLAGS = -MM

# Flags for the C++ compiler:
#CXXFLAGS = -Wall -g -coverage

# Flags for the linker:
#LDFLAGS = -g -rdynamic -coverage

# Flags for the TTCN-3 and ASN.1 compiler (-R already added)
#COMPILER_FLAGS +=

# Execution mode: (either ttcn3 or ttcn3-parallel)
TTCN3_LIB := ttcn3-rt2$(DYNAMIC_SUFFIX)

ifdef DYN
LIB_SUFFIX := .so
else
LIB_SUFFIX := .a
endif

#
# You may change these variables. Add your files if necessary...
#

# TTCN-3 modules of this project:
TTCN3_MODULES = rec.ttcn rec_of.ttcn uni.ttcn exer_rec.ttcn exer_rec_of.ttcn exer_uni.ttcn \
ReadXml.ttcn runner.ttcn

# ASN.1 modules of this project:
ASN1_MODULES =

# C++ source & header files generated from the TTCN-3 & ASN.1 modules of
# this project:
GENERATED_SOURCES = $(TTCN3_MODULES:.ttcn=.cc) $(ASN1_MODULES:.asn=.cc)
GENERATED_HEADERS = $(GENERATED_SOURCES:.cc=.hh)
ifdef SPLIT_TO_SLICES
POSTFIXES := $(foreach file, $(SPLIT_TO_SLICES), $(addsuffix $(file), _part_))
POSTFIXES := $(foreach file, $(POSTFIXES), $(addprefix $(file), .cc))
GENERATED_SOURCES2 := $(foreach file, $(GENERATED_SOURCES:.cc=), $(addprefix $(file), $(POSTFIXES)))
GENERATED_SOURCES += $(GENERATED_SOURCES2)
endif
# C/C++ Source & header files of Test Ports, external functions and
# other modules:
USER_SOURCES = ReadXmlImpl.cc
USER_HEADERS = $(USER_SOURCES:.cc=.hh)

# Object files of this project that are needed for the executable test suite:
OBJECTS = $(GENERATED_OBJECTS) $(USER_OBJECTS)

GENERATED_OBJECTS = $(GENERATED_SOURCES:.cc=.o)

USER_OBJECTS = $(USER_SOURCES:.cc=.o)

DEPFILES = $(USER_OBJECTS:.o=.d)  $(GENERATED_OBJECTS:.o=.d)

# Other files of the project (Makefile, configuration files, etc.)
# that will be added to the archived source files:
OTHER_FILES = Makefile

# The name of the executable test suite:
TARGET = neg$(EXESUFFIX)

#
# Rules for building the executable...
#

all: $(TARGET) ;

objects: $(OBJECTS) ;

$(TARGET): $(OBJECTS) $(TTCN3_DIR)/lib/lib$(TTCN3_LIB)$(LIB_SUFFIX)
	$(CXX) $(LDFLAGS) -o $@ $^ \
	-L$(TTCN3_DIR)/lib -l$(TTCN3_LIB) \
	-L$(OPENSSL_DIR)/lib -lcrypto \
	-L$(XMLDIR)/lib $($(PLATFORM)_LIBS) \
	|| $(TTCN3_DIR)/bin/titanver $(OBJECTS)

.cc.o .c.o:
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $@ $<

.cc.d .c.d:
	@echo Creating dependency file for '$<'; set -e; \
	$(CXX) $(CXXDEPFLAGS) $(CPPFLAGS) $(CXXFLAGS) $< \
	| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
	[ -s $@ ] || rm -f $@

$(GENERATED_SOURCES) $(GENERATED_HEADERS): compile
	@if [ ! -f $@ ]; then $(RM) compile; $(MAKE) compile; fi

compile:: $(firstword $(TTCN3_COMPILER)) $(TTCN3_DIR)/lib/lib$(TTCN3_LIB)$(LIB_SUFFIX)
	@if [ -f $@ ]; then $(RM) compile; $(MAKE) compile; fi

compile:: $(TTCN3_MODULES) $(ASN1_MODULES)
	$(TTCN3_COMPILER) $(COMPILER_FLAGS) $^ - $?
	touch $@

clean:
	-$(RM) $(TARGET) $(OBJECTS) $(GENERATED_HEADERS) \
	$(GENERATED_SOURCES) compile $(DEPFILES) \
	tags *.log *.log.orig

distclean: clean
	-$(RM) $(DEPFILES)

dep: $(GENERATED_SOURCES) $(USER_SOURCES) ;

ifeq ($(findstring n,$(MAKEFLAGS)),)
ifeq ($(filter clean distclean check compile archive diag,$(MAKECMDGOALS)),)
-include $(DEPFILES)
endif
endif

diag:
	$(TTCN3_DIR)/bin/compiler -v 2>&1
	$(TTCN3_DIR)/bin/mctr_cli -v 2>&1
	$(CXX) -v 2>&1
	@echo TTCN3_DIR=$(TTCN3_DIR)
	@echo OPENSSL_DIR=$(OPENSSL_DIR)
	@echo XMLDIR=$(XMLDIR)
	@echo PLATFORM=$(PLATFORM)

#
# Add your rules here if necessary...
#

RUN := $(shell which colortitan 2>/dev/null)
ifeq ($(firstword $(RUN)),no)
# stupid /bin/which on Solaris writes errors to stdout instead of stderr
RUN :=
endif

run: $(TARGET) run.cfg
	if $(RUN) ./$^; then ./indent.pl neg.log; else ./indent.pl neg.log; exit 1; fi 

