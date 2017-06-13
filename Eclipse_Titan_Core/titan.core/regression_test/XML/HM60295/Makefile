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
TOPDIR := ../..
include   ../../Makefile.regression

.SUFFIXES: .ttcn .hh .cc .d
.PHONY: all clean dep run convert

# Execution mode: (either ttcn3 or ttcn3-parallel)
TTCN3_LIB := ttcn3$(RT2_SUFFIX)$(DYNAMIC_SUFFIX)

#CPPFLAGS
#CXXFLAGS
#COMPILER_FLAGS +=

GENERATED_TTCN := XSD.ttcn UsefulTtcn3Types.ttcn \
www_example_org_Schema1.ttcn  www_example_org_Schema2.ttcn www_example_org_Import.ttcn

TTCN3_MODULES := tester.ttcn $(GENERATED_TTCN)

#ttcn_ericsson_se_protocolModules_xtdp1_0_xtdl2.ttcn
#ttcn_ericsson_se_protocolModules_xtdp1_0_xtdl.ttcn

GENERATED_SOURCES := $(TTCN3_MODULES:.ttcn=.cc)
GENERATED_HEADERS := $(GENERATED_SOURCES:.cc=.hh)
ifdef CODE_SPLIT
GENERATED_SOURCES := $(foreach file, $(GENERATED_SOURCES:.cc=), $(addprefix $(file), .cc _seq.cc _set.cc  _seqof.cc _setof.cc _union.cc))
else ifdef SPLIT_TO_SLICES
POSTFIXES := $(foreach file, $(SPLIT_TO_SLICES), $(addsuffix $(file), _part_))
POSTFIXES := $(foreach file, $(POSTFIXES), $(addprefix $(file), .cc))
GENERATED_SOURCES2 := $(foreach file, $(GENERATED_SOURCES:.cc=), $(addprefix $(file), $(POSTFIXES)))
GENERATED_SOURCES += $(GENERATED_SOURCES2)
endif
USER_SOURCES =
USER_HEADERS = $(USER_SOURCES:.cc=.hh)

OBJECTS = $(GENERATED_SOURCES:.cc=.o) $(USER_SOURCES:.cc=.o)

DEPFILES := $(OBJECTS:.o=.d)

TARGET = Schemas$(EXESUFFIX)

all: $(TARGET)

ifeq ($(findstring dynamic,$(TTCN3_LIB)),)
# not dynamic
CORELIB_BINARY := $(TTCN3_DIR)/lib/lib$(TTCN3_LIB).a
else
CORELIB_BINARY := $(TTCN3_DIR)/lib/lib$(TTCN3_LIB).so
endif

# libttcn3.a appears twice; this should be harmless
$(TARGET): $(OBJECTS) $(CORELIB_BINARY)
	$(CXX) $(LDFLAGS) -o $@ $(OBJECTS) \
	-L$(TTCN3_DIR)/lib -l$(TTCN3_LIB) \
	-L$(OPENSSL_DIR)/lib -lcrypto $($(PLATFORM)_LIBS) \
	|| $(TTCN3_DIR)/bin/titanver $(OBJECTS)

%.o: %.cc
	@echo '(C++)' $<; $(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

%.d: %.cc
	@echo '(dep)' $<; $(CXX) -MM -MP -MF $@ $(CPPFLAGS) $(CXXFLAGS) $<

$(GENERATED_SOURCES) $(GENERATED_HEADERS): compile
	@if [ ! -f $@ ]; then $(RM) compile; $(MAKE) compile; fi


compile:: $(firstword $(TTCN3_COMPILER))
	@if [ -f $@ ]; then $(RM) compile; $(MAKE) compile; fi
#	if [ -f $(GENERATED_SOURCES) ]; then rm -f $(GENERATED_SOURCES); $(MAKE) $(GENERATED_SOURCES); fi

compile:: $(TTCN3_MODULES) $(PREPROCESSED_TTCN3_MODULES) $(ASN1_MODULES)
#	"modulepar boolean verbose" makes this unusable with old names
	$(TTCN3_COMPILER) $(COMPILER_FLAGS) $^ - $?
	touch $@

clean celan clena:
	-$(RM) $(TARGET) $(OBJECTS) $(GENERATED_HEADERS) \
	$(GENERATED_SOURCES) $(PREPROCESSED_TTCN3_MODULES) compile \
	tags *.log

distclean: clean
	rm -f $(GENERATED_TTCN) $(DEPFILES)

$(GENERATED_TTCN) convert: Schema1.xsd Schema2.xsd Importer.xsd
	$(TTCN3_DIR)/bin/xsd2ttcn -z $^

RUN := $(shell which colortitan 2>/dev/null)
ifeq ($(firstword $(RUN)),no)
# stupid /bin/which on Solaris writes errors to stdout instead of stderr
RUN :=
endif

run: $(TARGET) config.cfg
	$(RUN) ./$^

debug:
	$(MAKE) run RUN='gdb --args'


dep: ;

ifeq (,$(findstring n,$(MAKEFLAGS)))
ifeq (,$(filter clean distclean,$(MAKECMDGOALS)))
#-include $(DEPFILES)
endif
endif

vpath %.xsd $(ABS_SRC)

