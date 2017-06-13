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
#
##############################################################################
TOPDIR := ..
include   ../Makefile.regression

# WARNING! This Makefile can be used with GNU make only.
# Other versions of make may report syntax errors in it.

#
# Do NOT touch this line...
#
.PHONY: all archive check clean dep objects

.SUFFIXES: .d

#
# Set these variables...
#


# Flags for the C++ preprocessor (and makedepend as well):
#CPPFLAGS +=

# Flags for dependency generation
CXXDEPFLAGS = -MM

# Flags for the C++ compiler:
#CXXFLAGS +=

# Flags for the linker:
#LDFLAGS += 


# Flags for the TTCN-3 and ASN.1 compiler:
COMPILER_FLAGS += -Lg

# Execution mode: (either ttcn3 or ttcn3-parallel)
TTCN3_LIB = ttcn3$(RT2_SUFFIX)-parallel$(DYNAMIC_SUFFIX)

#
# You may change these variables. Add your files if necessary...
#

# TTCN-3 modules of this project:
TTCN3_MODULES = concat.ttcn

# ASN.1 modules of this project:
ASN1_MODULES =

# C++ source & header files generated from the TTCN-3 & ASN.1 modules of
# this project:
GENERATED_SOURCES = $(TTCN3_MODULES:.ttcn=.cc) $(ASN1_MODULES:.asn=.cc)
GENERATED_HEADERS = $(GENERATED_SOURCES:.cc=.hh)

# C/C++ Source & header files of Test Ports, external functions and
# other modules:
USER_SOURCES =
USER_HEADERS = $(USER_SOURCES:.cc=.hh)

# Object files of this project that are needed for the executable test suite:
OBJECTS = $(GENERATED_OBJECTS) $(USER_OBJECTS)

GENERATED_OBJECTS = $(GENERATED_SOURCES:.cc=.o)

USER_OBJECTS = $(USER_SOURCES:.cc=.o)

DEPFILES = $(USER_OBJECTS:.o=.d)  $(GENERATED_OBJECTS:.o=.d)

# Other files of the project (Makefile, configuration files, etc.)
# that will be added to the archived source files:
OTHER_FILES = config.cfg Makefile

# The name of the executable test suite:
TARGET = concat

#
# Rules for building the executable...
#

all: $(TARGET) ;

objects: $(OBJECTS) compile;

$(TARGET): $(OBJECTS)
	if $(CXX) $(LDFLAGS) -o $@ $^ \
	-L$(TTCN3_DIR)/lib -l$(TTCN3_LIB) \
	-L$(OPENSSL_DIR)/lib -lcrypto \
	-L$(XMLDIR)/lib $($(PLATFORM)_LIBS); \
	then : ; else $(TTCN3_DIR)/bin/titanver $(OBJECTS); exit 1; fi

.cc.o .c.o:
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $@ $<

.cc.d .c.d:
	@echo Creating dependency file for '$<'; set -e; \
	$(CXX) $(CXXDEPFLAGS) $(CPPFLAGS) $(CXXFLAGS) $< \
	| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
	[ -s $@ ] || rm -f $@

$(GENERATED_SOURCES) $(GENERATED_HEADERS): compile
	@if [ ! -f $@ ]; then $(RM) compile; $(MAKE) compile; fi

compile: $(TTCN3_MODULES) $(ASN1_MODULES)
	$(TTCN3_DIR)/bin/compiler $(COMPILER_FLAGS) $^ - $?
	touch $@

clean:
	-$(RM) $(TARGET) $(OBJECTS) $(GENERATED_HEADERS) \
	$(GENERATED_SOURCES) compile \
	tags *.log

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

run: $(TARGET) everything.cfg
ifeq ($(findstring parallel,$(TTCN3_LIB)),)
	$(RUN) ./$^
else
	$(TTCN3_DIR)/bin/ttcn3_start $^
endif

debug:
	$(MAKE) run RUN='gdb --args'

# The end
