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

.SUFFIXES: .ttcn .asn .hh
.PHONY: all clean dep run

TTCN3_LIB = ttcn3$(RT2_SUFFIX)$(DYNAMIC_SUFFIX)

TTCN3_MODULES = Types.ttcn Functions.ttcn AttributeTestcases.ttcn Testcases.ttcn SemanticCheck.ttcn

ASN1_MODULES = JsonData.asn

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

USER_SOURCES =  

OBJECTS = $(GENERATED_SOURCES:.cc=.o) $(USER_SOURCES:.cc=.o)

TARGET = json$(EXESUFFIX)

all: $(TARGET)

$(TARGET): $(GENERATED_SOURCES) $(USER_SOURCES)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ -L$(TTCN3_DIR)/lib -l$(TTCN3_LIB) -L$(OPENSSL_DIR)/lib -lcrypto $($(PLATFORM)_LIBS)

	
#.ttcn.cc .ttcn.hh:
#	$(TTCN3_COMPILER) $<

$(GENERATED_SOURCES) $(GENERATED_HEADERS): compile
	@if [ ! -f $@ ]; then $(RM) compile; $(MAKE) compile; fi

compile: $(TTCN3_MODULES) $(ASN1_MODULES)
	$(filter-out -Nold -E, $(TTCN3_COMPILER)) $(COMPILER_FLAGS) $^ 
	touch compile
	
clean distclean:
	-rm -rf $(TARGET) $(OBJECTS) $(GENERATED_HEADERS) \
	$(GENERATED_SOURCES) *.log Makefile.bak compile logs

dep: $(GENERATED_SOURCES)
	makedepend $(CPPFLAGS) $(GENERATED_SOURCES)

run: $(TARGET) JSON.cfg
	./$^

.NOTPARALLEL:

ifdef SRCDIR
$(foreach src, $(USER_SOURCES), $(eval vpath $(src) $(ABS_SRC)))
endif
