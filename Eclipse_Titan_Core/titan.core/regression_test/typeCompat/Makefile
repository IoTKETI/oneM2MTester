##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#   Kovacs, Ferenc
#   Raduly, Csaba
#   Szabados, Kristof
#   Szabo, Bence Janos
#
##############################################################################
TOPDIR := ..
include $(TOPDIR)/Makefile.regression

#only works with RT2
ifndef RT2
$(error note: typeCompat requires RT2)
endif

.PHONY: all clean dep run

CXXFLAGS += -Wall -Wextra -Wshadow -g

TTCN3_LIB = ttcn3$(RT2_SUFFIX)$(DYNAMIC_SUFFIX)

TTCN3_MODULES = Compat.ttcn Ext.ttcn Ext2.ttcn Std.ttcn
ASN1_MODULES = ASN_Ext.asn

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

TARGET = Compat$(EXESUFFIX)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJECTS) -L$(TTCN3_DIR)/lib -l$(TTCN3_LIB) \
	-L$(OPENSSL_DIR)/lib -lcrypto $($(PLATFORM)_LIBS)

.cc.o:
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

$(GENERATED_SOURCES) $(GENERATED_HEADERS): compile

compile: $(TTCN3_MODULES) $(ASN1_MODULES)
#	Test uses anytype, which is incompatible with old names
	$(filter-out -Nold -E, $(TTCN3_COMPILER)) $^ 
	touch compile

clean distclean:
	-rm -f $(TARGET) $(OBJECTS) $(GENERATED_HEADERS) \
	$(GENERATED_SOURCES) compile *.log *~ core

dep: $(GENERATED_SOURCES)
	makedepend $(CPPFLAGS) $(USER_SOURCES) $(GENERATED_SOURCES)

run: $(TARGET)
	./$(TARGET) config.cfg
