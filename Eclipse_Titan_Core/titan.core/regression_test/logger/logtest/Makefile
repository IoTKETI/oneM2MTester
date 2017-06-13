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
#   Baranyi, Botond
#
##############################################################################
TOPDIR := ../../
include ${TOPDIR}/Makefile.regression

#
# Do NOT touch this line...
#
.PHONY: all archive check clean dep objects

.SUFFIXES: .d

#
# Set these variables...
#

# The path of your TTCN-3 Test Executor installation:
# Uncomment this line to override the environment variable.
# TTCN3_DIR = 

# Your platform: (SOLARIS, SOLARIS8, LINUX, FREEBSD or WIN32)
#PLATFORM = LINUX

# Your C++ compiler:
# (if you change the platform, you may need to change the compiler)
#CXX = g++

# Flags for the C++ preprocessor (and makedepend as well):
CPPFLAGS = -D$(PLATFORM) -I$(TTCN3_DIR)/include

# Flags for dependency generation
CXXDEPFLAGS = -MM

# Flags for the C++ compiler:
CXXFLAGS = -Wall

# Flags for the linker:
LDFLAGS = 

ifeq ($(PLATFORM), WIN32)
# Silence linker warnings.
LDFLAGS += -Wl,--enable-auto-import,--enable-runtime-pseudo-reloc
endif

# Flags for the TTCN-3 and ASN.1 compiler:
COMPILER_FLAGS = -L  

# Execution mode: (either ttcn3 or ttcn3-parallel)
TTCN3_LIB = ttcn3-parallel

# The path of your OpenSSL installation:
# If you do not have your own one, leave it unchanged.
OPENSSL_DIR = $(TTCN3_DIR)

# The path of your libxml2 installation:
# If you do not have your own one, leave it unchanged.
XMLDIR = $(TTCN3_DIR)

# Directory to store the archived source files:
ARCHIVE_DIR = backup

#
# You may change these variables. Add your files if necessary...
#

# TTCN-3 modules of this project:
TTCN3_MODULES = Titan_LogTestDefinitions.ttcn Titan_LogTest.ttcn 

# ASN.1 modules of this project:
ASN1_MODULES =

# C++ source & header files generated from the TTCN-3 & ASN.1 modules of
# this project:
GENERATED_SOURCES = $(TTCN3_MODULES:.ttcn=.cc) $(ASN1_MODULES:.asn=.cc)
GENERATED_HEADERS = $(GENERATED_SOURCES:.cc=.hh)

# C/C++ Source & header files of Test Ports, external functions and
# other modules:
USER_SOURCES = PCO_PT.cc
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
TARGET = Titan_LogTest

#
# Do not modify these unless you know what you are doing...
# Platform specific additional libraries:
#
SOLARIS_LIBS += -lsocket -lnsl -lxml2
SOLARIS8_LIBS += -lsocket -lnsl -lxml2
LINUX_LIBS += -lxml2
FREEBSD_LIBS += -lxml2
WIN32_LIBS += -lxml2

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

check: $(TTCN3_MODULES) $(ASN1_MODULES)
	$(TTCN3_DIR)/bin/compiler -s $(COMPILER_FLAGS) $^

compile: $(TTCN3_MODULES) $(ASN1_MODULES)
	$(TTCN3_DIR)/bin/compiler $(COMPILER_FLAGS) $^ - $?
	touch $@

clean distclean:
	-$(RM) $(TARGET) $(OBJECTS) $(GENERATED_HEADERS) \
	$(GENERATED_SOURCES) compile $(DEPFILES) \
	tags *c.log

dep: $(GENERATED_SOURCES) $(USER_SOURCES) ;

ifeq ($(findstring n,$(MAKEFLAGS)),)
ifeq ($(filter clean check compile archive diag,$(MAKECMDGOALS)),)
-include $(DEPFILES)
endif
endif

archive:
	mkdir -p $(ARCHIVE_DIR)
	tar -cvhf - $(TTCN3_MODULES) $(ASN1_MODULES) \
	$(USER_HEADERS) $(USER_SOURCES) $(OTHER_FILES) \
	| gzip >$(ARCHIVE_DIR)/`basename $(TARGET) .exe`-`date '+%y%m%d-%H%M'`.tgz

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
run: $(TARGET)
	./logtest.sh

