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
#   Delic, Adam
#   Raduly, Csaba
#   Szabados, Kristof
#   Szabo, Bence Janos
#
##############################################################################
ifeq "$(MAKELEVEL)" "0"
# not recursive
RT2:=1
endif

TOPDIR := ..
include $(TOPDIR)/Makefile.regression

#ony works with RT2
ifndef RT2
$(error note: negativTest requires RT2)
endif

.PHONY: all clean dep run
.SUFFIXES: .d

CXXFLAGS += -Wall -Wextra -Wshadow -g
CXXDEPFLAGS := -MM

#COMPILER_FLAGS +=
#LDFLAGS += -rdynamic

TTCN3_LIB = ttcn3$(RT2_SUFFIX)$(DYNAMIC_SUFFIX)

TTCN3_MODULES = negtest.ttcn NegTestTestcases.ttcn \
NegTest_TEXT_Types.ttcn NegTest_TEXT_Testcases.ttcn \
NegTest_RAW_Types.ttcn NegTest_RAW_Testcases.ttcn \
www_XmlTest_org_negativeTest_XML_Types.ttcn NegTest_XML_Testcases.ttcn XSD.ttcn UsefulTtcn3Types.ttcn \
NegTest_JSON.ttcn NegTest_Update.ttcn

ASN1_MODULES = Types.asn NegTestTypes.asn

GENERATED_SOURCES = $(TTCN3_MODULES:.ttcn=.cc) $(ASN1_MODULES:.asn=.cc)
GENERATED_HEADERS = $(TTCN3_MODULES:.ttcn=.hh) $(ASN1_MODULES:.asn=.hh)
ifdef CODE_SPLIT
GENERATED_SOURCES := $(foreach file, $(GENERATED_SOURCES:.cc=), $(addprefix $(file), .cc _seq.cc _set.cc  _seqof.cc _setof.cc _union.cc))
else ifdef SPLIT_TO_SLICES
POSTFIXES := $(foreach file, $(SPLIT_TO_SLICES), $(addsuffix $(file), _part_))
POSTFIXES := $(foreach file, $(POSTFIXES), $(addprefix $(file), .cc))
GENERATED_SOURCES2 := $(foreach file, $(GENERATED_SOURCES:.cc=), $(addprefix $(file), $(POSTFIXES)))
GENERATED_SOURCES += $(GENERATED_SOURCES2)
endif

USER_SOURCES =
USER_HEADERS =

OBJECTS = $(GENERATED_SOURCES:.cc=.o) $(USER_SOURCES:.cc=.o)
DEPS := $(OBJECTS:.o=.d)

TARGET = negtest$(EXESUFFIX)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJECTS) -L$(TTCN3_DIR)/lib -l$(TTCN3_LIB) \
	-L$(OPENSSL_DIR)/lib -lcrypto $($(PLATFORM)_LIBS)

.cc.o:
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

.cc.d .c.d:
	@echo Creating dependency file for '$<'; set -e; \
	$(CXX) $(CXXDEPFLAGS) $(CPPFLAGS) $(CXXFLAGS) $< \
	| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
	[ -s $@ ] || rm -f $@

$(GENERATED_SOURCES) $(GENERATED_HEADERS): compile
	@if [ ! -f $@ ]; then $(RM) compile; $(MAKE) compile; fi

compile: $(TTCN3_MODULES) $(ASN1_MODULES)
	$(filter-out -Nold -E, $(TTCN3_COMPILER)) $(COMPILER_FLAGS) $^ 
	touch compile

clean distclean:
	-rm -f $(TARGET) $(OBJECTS) $(GENERATED_HEADERS) \
	$(GENERATED_SOURCES) compile *.log *~ core

dep: $(DEPS) ;
#	makedepend $(CPPFLAGS) $(USER_SOURCES) $(GENERATED_SOURCES)

run: $(TARGET)
	./$(TARGET) NegTest_all.cfg


run1: $(TARGET)
	./$(TARGET) NegTest.cfg

run2: $(TARGET)
	./$(TARGET) config.cfg

run3: $(TARGET)
	./$(TARGET) NegTest_TEXT.cfg

run4: $(TARGET)
	./$(TARGET) NegTest_XML.cfg

run5: $(TARGET)
	./$(TARGET) NegTest_RAW.cfg || perl -nwle 'if (/->(\w+\.ttcn:\d+)->.*? (.*fail.*)/) { print STDERR "$$1: note: $$2" }' NegTest_RAW.log
	perl -i -pwle 's/!/\n/g' NegTest_RAW.log

run6: $(TARGET)
	./$(TARGET) NegTest_JSON.cfg


ifeq ($(findstring n,$(MAKEFLAGS)),)
ifeq ($(filter clean check compile archive diag,$(MAKECMDGOALS)),)
-include $(DEPFILES)
endif
endif
