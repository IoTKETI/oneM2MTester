##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#   Baranyi, Botond
#   Szabo, Bence Janos
#
##############################################################################
TOPDIR := ..
include $(TOPDIR)/Makefile.regression

.SUFFIXES: .ttcn .asn .hh .json
.PHONY: all clean dep run

TTCN3_LIB = ttcn3$(RT2_SUFFIX)$(DYNAMIC_SUFFIX)

# Tester modules
TTCN3_MODULES = PIPEasp_PortType.ttcn PIPEasp_Types.ttcn PIPEasp_Templates.ttcn Shell.ttcn Testcases.ttcn

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

USER_SOURCES = PIPEasp_PT.cc
USER_HEADERS = $(USER_SOURCES:.cc=.hh)

OBJECTS = $(GENERATED_SOURCES:.cc=.o) $(USER_SOURCES:.cc=.o)

TARGET = profiler_test$(EXESUFFIX)

# Tested modules (these are compiled with profiling enabled, and are executed by the tester modules)
PROF_MODULES = prof1.ttcn prof2.ttcn prof3.ttcn

PROF_COMPILER_FLAGS = $(COMPILER_FLAGS) -z prof_files.txt

PROF_TTCN3_LIB = ttcn3$(RT2_SUFFIX)-parallel$(DYNAMIC_SUFFIX)

PROF_GENERATED_SOURCES = $(PROF_MODULES:.ttcn=.cc)
PROF_GENERATED_HEADERS = $(PROF_GENERATED_SOURCES:.cc=.hh)
ifdef SPLIT_TO_SLICES
POSTFIXES := $(foreach file, $(SPLIT_TO_SLICES), $(addsuffix $(file), _part_))
POSTFIXES := $(foreach file, $(POSTFIXES), $(addprefix $(file), .cc))
PROF_GENERATED_SOURCES2 := $(foreach file, $(PROF_GENERATED_SOURCES:.cc=), $(addprefix $(file), $(POSTFIXES)))
PROF_GENERATED_SOURCES += $(PROF_GENERATED_SOURCES2)
endif

PROF_OBJECTS = $(PROF_GENERATED_SOURCES:.cc=.o)

PROF_TARGET = prof.exe

TEMP_FILES = $(PROF_GENERATED_SOURCES) $(PROF_GENERATED_HEADERS) $(PROF_OBJECTS) $(PROF_TARGET) \
data.json data2.json data3.json prof1.stats empty.stats double.json double.stats merged.json merged.stats

# Rules for tester modules
all: $(TARGET)

$(TARGET): $(GENERATED_SOURCES) $(USER_SOURCES)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ -L$(TTCN3_DIR)/lib -l$(TTCN3_LIB) -L$(OPENSSL_DIR)/lib -lcrypto $($(PLATFORM)_LIBS)

$(GENERATED_SOURCES) $(GENERATED_HEADERS): compile
	@if [ ! -f $@ ]; then $(RM) compile; $(MAKE) compile; fi

compile: $(TTCN3_MODULES)
	$(filter-out -Nold -E, $(TTCN3_COMPILER)) $(COMPILER_FLAGS) $^ 
	touch compile
	
clean distclean:
	-rm -f $(TARGET) $(OBJECTS) $(GENERATED_HEADERS) \
	$(GENERATED_SOURCES) *.log Makefile.bak $(TEMP_FILES) compile

dep: $(GENERATED_SOURCES) $(PROF_GENERATED_SOURCES)
	makedepend $(CPPFLAGS) $^

run: $(TARGET)
	./$^

.NOTPARALLEL:

# Rules for tested modules
$(PROF_TARGET): $(PROF_GENERATED_SOURCES)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ -L$(TTCN3_DIR)/lib -l$(PROF_TTCN3_LIB) -L$(OPENSSL_DIR)/lib -lcrypto $($(PLATFORM)_LIBS)

$(PROF_GENERATED_SOURCES) $(PROF_GENERATED_HEADERS): prof_compile
	@if [ ! -f $@ ]; then $(RM) prof_compile; $(MAKE) prof_compile; fi

prof_compile: $(PROF_MODULES)
	$(filter-out -Nold -E, $(TTCN3_COMPILER)) $(PROF_COMPILER_FLAGS) $^ 
	touch compile
