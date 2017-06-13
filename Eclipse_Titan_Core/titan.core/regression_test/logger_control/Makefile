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
#   Szabo, Bence Janos
#
##############################################################################
TOPDIR := ..
include ../Makefile.regression

#
# Do NOT touch this line...
#
.PHONY: all archive check clean dep objects

.SUFFIXES: .d

#
# Set these variables...
#


# Flags for the C++ preprocessor (and makedepend as well):
CPPFLAGS += -I../../core/loggerapi -I..

# Flags for dependency generation
CXXDEPFLAGS = -MM

# Flags for the C++ compiler: -Wall
#CXXFLAGS +=

# Flags for the linker:
#LDFLAGS +=

# Flags for the TTCN-3 and ASN.1 compiler:
ifeq ($(findstring -L,$(TTCN3_COMPILER)),) 
# logfilecheck requires location info.
COMPILER_FLAGS += -L
endif

# Execution mode: (either ttcn3 or ttcn3-parallel)
TTCN3_LIB = ttcn3$(RT2_SUFFIX)$(DYNAMIC_SUFFIX)

#
# You may change these variables. Add your files if necessary...
#

# TTCN-3 modules of this project:
TTCN3_MODULES := logcontrol.ttcn

# This is needed by the compiler for semantic analysis, but it should
# not be built and linked to the executable (since it's already part
# of the runtime library)
TITAN_RUNTIME_MODULES := TitanLoggerControl.ttcn

# ASN.1 modules of this project:
ASN1_MODULES =

# C++ source & header files generated from the TTCN-3 & ASN.1 modules of
# this project:
GENERATED_SOURCES := $(TTCN3_MODULES:.ttcn=.cc) $(ASN1_MODULES:.asn=.cc)
GENERATED_HEADERS := $(GENERATED_SOURCES:.cc=.hh)
ifdef SPLIT_TO_SLICES
POSTFIXES := $(foreach file, $(SPLIT_TO_SLICES), $(addsuffix $(file), _part_))
POSTFIXES := $(foreach file, $(POSTFIXES), $(addprefix $(file), .cc))
GENERATED_SOURCES2 := $(foreach file, $(GENERATED_SOURCES:.cc=), $(addprefix $(file), $(POSTFIXES)))
GENERATED_SOURCES += $(GENERATED_SOURCES2)
endif
# C/C++ Source & header files of Test Ports, external functions and
# other modules:
USER_SOURCES :=
USER_HEADERS :=

GENERATED_OBJECTS := $(GENERATED_SOURCES:.cc=.o)

USER_OBJECTS := $(USER_SOURCES:.cc=.o)

# Object files of this project that are needed for the executable test suite:
OBJECTS = $(GENERATED_OBJECTS) $(USER_OBJECTS)

DEPFILES = $(USER_OBJECTS:.o=.d)  $(GENERATED_OBJECTS:.o=.d)

# Other files of the project (Makefile, configuration files, etc.)
# that will be added to the archived source files:
OTHER_FILES = Makefile

# The name of the executable test suite:
TARGET = logcontrol$(EXESUFFIX)

#
# Rules for building the executable...
#

all: $(TARGET) ;

objects: $(OBJECTS) ;

$(TARGET): $(OBJECTS)
	$(TTCN3_DIR)/bin/titanver $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ \
	-L$(TTCN3_DIR)/lib -l$(TTCN3_LIB) \
	-L$(OPENSSL_DIR)/lib -lcrypto \
	-L$(XMLDIR)/lib $($(PLATFORM)_LIBS)

.cc.o .c.o:
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $@ $<

.cc.d .c.d:
	@echo Creating dependency file for '$<'; set -e; \
	$(CXX) $(CXXDEPFLAGS) $(CPPFLAGS) $(CXXFLAGS) $< \
	| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
	[ -s $@ ] || rm -f $@

$(GENERATED_SOURCES) $(GENERATED_HEADERS): compile
	@if [ ! -f $@ ]; then $(RM) compile; $(MAKE) compile; fi

compile: $(TTCN3_MODULES) $(ASN1_MODULES) $(TITAN_RUNTIME_MODULES)
	$(TTCN3_COMPILER) $(COMPILER_FLAGS) $^ - $?
	touch $@

clean:
	-$(RM) $(TARGET) $(OBJECTS) $(GENERATED_HEADERS) \
	$(GENERATED_SOURCES) compile $(DEPFILES) \
	tags *.log

distclean: clean
	-$(RM) TitanLoggerControl.ttcn

dep: $(GENERATED_SOURCES) $(USER_SOURCES) ;

ifeq ($(findstring n,$(MAKEFLAGS)),)
ifeq ($(filter clean distclean check compile archive diag,$(MAKECMDGOALS)),)
-include $(DEPFILES)
endif
endif


diag:
	$(TTCN3_DIR)/bin/compiler -v 2>&1
	$(TTCN3_DIR)/bin/mctr_cli -v 2>&1
	$(CXX) -v 2>&1	@echo TTCN3_DIR=$(TTCN3_DIR)
	@echo OPENSSL_DIR=$(OPENSSL_DIR)
	@echo XMLDIR=$(XMLDIR)
	@echo PLATFORM=$(PLATFORM)

#
# Add your rules here if necessary...
#

run: $(TARGET) config.cfg
	rm -f *.log
	./$^
	$(ABS_SRC)/logfilecheck

# files from the runtime
TitanLoggerControl.ttcn: ../../core/TitanLoggerControl.ttcn
	ln -s $< $@

