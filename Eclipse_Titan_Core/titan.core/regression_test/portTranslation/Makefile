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
TOPDIR := ..
include   ../Makefile.regression

#
# Do NOT touch this line...
#
.PHONY: all run archive check clean dep objects

.SUFFIXES: .d

#
# Set these variables...
#

#CXXFLAGS += -save-temps
CXXFLAGS += -g

# Flags for dependency generation
CXXDEPFLAGS = -MM

#COMPILER_FLAGS +=

# Flags for the linker:
LDFLAGS += -g

ifeq ($(PLATFORM), WIN32)
# Silence linker warnings.
LDFLAGS += -Wl,--enable-auto-import,--enable-runtime-pseudo-reloc
endif

# Flags for the TTCN-3 and ASN.1 compiler:
#COMPILER_FLAGS += -L

# Execution mode: (either ttcn3 or ttcn3-parallel)
TTCN3_LIB = ttcn3$(RT2_SUFFIX)-parallel$(DYNAMIC_SUFFIX)


# TTCN-3 modules of this project:
TTCN3_MODULES = PortTranslation.ttcn Setstate_neg.ttcn PortVariables.ttcn

# ASN.1 modules of this project:
ASN1_MODULES =

# C++ source & header files generated from the TTCN-3 & ASN.1 modules of
# this project:
GENERATED_SOURCES = $(TTCN3_MODULES:.ttcn=.cc) $(ASN1_MODULES:.asn=.cc) NP1.cc NPT2.cc NVP1.cc
GENERATED_HEADERS = $(GENERATED_SOURCES:.cc=.hh)
ifdef SPLIT_TO_SLICES
POSTFIXES := $(foreach file, $(SPLIT_TO_SLICES), $(addsuffix $(file), _part_))
POSTFIXES := $(foreach file, $(POSTFIXES), $(addprefix $(file), .cc))
GENERATED_SOURCES2 := $(foreach file, $(GENERATED_SOURCES:.cc=), $(addprefix $(file), $(POSTFIXES)))
GENERATED_SOURCES += $(GENERATED_SOURCES2)
endif
# C/C++ Source & header files of Test Ports, external functions and
# other modules:
USER_SOURCES = PT2.cc P1.cc P2.cc P3.cc VP1.cc NVP1.cc
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
TARGET = PortTranslation$(EXESUFFIX)

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

compile:: $(TTCN3_DIR)/bin/compiler # $(TTCN3_COMPILER)
	@if [ -f $@ ]; then $(RM) compile; $(MAKE) compile; fi

compile:: $(TTCN3_MODULES) $(ASN1_MODULES)
	$(TTCN3_COMPILER) $(COMPILER_FLAGS) -t $^ - $?
	touch $@

clean distclean:
	-$(RM) $(TARGET) $(OBJECTS) $(GENERATED_HEADERS) \
	$(GENERATED_SOURCES) compile $(DEPFILES) \
	tags *.log $(RT2CFG)

dep: $(GENERATED_SOURCES) $(USER_SOURCES) ;

ifeq ($(findstring n,$(MAKEFLAGS)),)
ifeq ($(filter clean distclean check compile archive diag,$(MAKECMDGOALS)),)
-include $(DEPFILES)
endif
endif


#
# Add your rules here if necessary...
#
run: $(TARGET) config.cfg
	$(TTCN3_DIR)/bin/ttcn3_start $^
