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

# WARNING! This Makefile can be used with GNU make only.
# Other versions of make may report syntax errors in it.

#
# Do NOT touch this line...
#
.PHONY: all archive check clean dep objects


# Flags for the C++ compiler:
CXXFLAGS += -g

# Flags for the linker:
LDFLAGS += -g

# Flags for the TTCN-3 and ASN.1 compiler:
#COMPILER_FLAGS += -Lg

# Execution mode: (either ttcn3 or ttcn3-parallel)
TTCN3_LIB = ttcn3$(RT2_SUFFIX)$(DYNAMIC_SUFFIX)

#
# You may change these variables. Add your files if necessary...
#

# TTCN-3 modules of this project:
TTCN3_MODULES = converter.ttcn convertest.ttcn

# ASN.1 modules of this project:
ASN1_MODULES =

# C++ source & header files generated from the TTCN-3 & ASN.1 modules of
# this project:
GENERATED_SOURCES = $(TTCN3_MODULES:.ttcn=.cc) $(ASN1_MODULES:.asn=.cc)
GENERATED_HEADERS = $(GENERATED_SOURCES:.cc=.hh)
ifdef CODE_SPLIT
GENERATED_SOURCES := $(foreach file, $(GENERATED_SOURCES:.cc=), $(addprefix $(file), .cc _seq.cc _set.cc  _seqof.cc _setof.cc _union.cc))
else ifdef SPLIT_TO_SLICES
POSTFIXES := $(foreach file, $(SPLIT_TO_SLICES), $(addsuffix $(file), _part_))
POSTFIXES := $(foreach file, $(POSTFIXES), $(addprefix $(file), .cc))
GENERATED_SOURCES2 := $(foreach file, $(GENERATED_SOURCES:.cc=), $(addprefix $(file), $(POSTFIXES)))
GENERATED_SOURCES += $(GENERATED_SOURCES2)
endif

# C/C++ Source & header files of Test Ports, external functions and
# other modules:
USER_SOURCES = iconver.cc
USER_HEADERS = $(USER_SOURCES:.cc=.hh)

# Object files of this project that are needed for the executable test suite:
OBJECTS = $(GENERATED_SOURCES:.cc=.o) $(USER_SOURCES:.cc=.o)

# Other files of the project (Makefile, configuration files, etc.)
# that will be added to the archived source files:
OTHER_FILES = Makefile

# The name of the executable test suite:
TARGET = convertest$(EXESUFFIX)

#
# Do not modify these unless you know what you are doing...
# Platform specific additional libraries:
# -lxml2 (and -lxnet on SOLARIS*) already there
SOLARIS_LIBS  += -lsocket -lnsl -lcrypto
SOLARIS8_LIBS += -lsocket -lnsl -lcrypto
LINUX_LIBS    += -lcrypto
FREEBSD_LIBS  += -lcrypto -liconv
WIN32_LIBS    += -lcrypto -liconv
INTERIX_LIBS  += -L/usr/local/ssl/lib -lcrypto

#
# Rules for building the executable...
#

all: $(TARGET) ;

objects: $(OBJECTS) ;

# Don't bother with .o files
$(TARGET): $(GENERATED_SOURCES) $(USER_SOURCES)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ \
	-L$(TTCN3_DIR)/lib -l$(TTCN3_LIB) \
	-L$(XMLDIR)/lib -L$(OPENSSL_DIR)/lib $($(PLATFORM)_LIBS)

$(GENERATED_SOURCES) $(GENERATED_HEADERS): compile
	@if [ ! -f $@ ]; then $(RM) compile; $(MAKE) compile; fi

compile: $(TTCN3_MODULES) $(ASN1_MODULES)
	$(TTCN3_COMPILER) $(COMPILER_FLAGS) $^ - $?
	touch $@

clean distclean:
	-$(RM) $(TARGET) $(OBJECTS) $(GENERATED_HEADERS) \
	$(GENERATED_SOURCES) compile \
	tags *.log


#
# Add your rules here if necessary...
#

RUN := $(shell which colortitan 2>/dev/null)
ifeq ($(firstword $(RUN)),no)
# stupid /bin/which on Solaris writes errors to stdout instead of stderr
RUN :=
endif

run: $(TARGET) config.cfg
	$(RUN) ./$^

vpath iconver.cc $(ABS_SRC)
vpath %.ttcn $(ABS_SRC)
