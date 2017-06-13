##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#   Delic, Adam
#   Koppany, Csaba
#   Kovacs, Ferenc
#   Kulcsar, Endre
#   Raduly, Csaba
#   Szabados, Kristof
#   Szabo, Bence Janos
#   Szabo, Janos Zoltan – initial implementation
#
##############################################################################
TOPDIR := ../..
include $(TOPDIR)/Makefile.regression

.SUFFIXES: .ttcn .asn .hh
.PHONY: all clean dep run

TTCN3_LIB = ttcn3$(RT2_SUFFIX)$(DYNAMIC_SUFFIX)

ASN1_MODULES = Test46.asn Test48.asn Test49.asn Test51.asn Test52.asn Test53.asn Test54.asn Test55.asn Test56.asn Test57.asn Test60.asn Test61.asn Test62.asn Test63.asn Test64.asn Test65.asn Test66.asn Test68.asn Test71.asn Test74.asn Test75.asn Test76.asn Test77.asn Test78.asn Test80.asn Test81.asn Test83.asn Test86.asn Test87.asn

# Tests for REAL values
# Test54.asn Test62.asn Test64.asn Test65.asn

# Selection Type for CHOICE
#  Test71.asn

GENERATED_SOURCES = $(ASN1_MODULES:.asn=.cc)
GENERATED_HEADERS = $(GENERATED_SOURCES:.cc=.hh)
ifdef CODE_SPLIT
GENERATED_SOURCES := $(foreach file, $(GENERATED_SOURCES:.cc=), $(addprefix $(file), .cc _seq.cc _set.cc  _seqof.cc _setof.cc _union.cc))
else ifdef SPLIT_TO_SLICES
POSTFIXES := $(foreach file, $(SPLIT_TO_SLICES), $(addsuffix $(file), _part_))
POSTFIXES := $(foreach file, $(POSTFIXES), $(addprefix $(file), .cc))
GENERATED_SOURCES2 := $(foreach file, $(GENERATED_SOURCES:.cc=), $(addprefix $(file), $(POSTFIXES)))
GENERATED_SOURCES += $(GENERATED_SOURCES2)
endif

all: $(GENERATED_SOURCES)

.asn.cc .asn.hh:
	$(TTCN3_COMPILER) $<

clean distclean:
	$(RM) $(GENERATED_HEADERS) $(GENERATED_SOURCES) Makefile.bak

dep:

run: $(GENERATED_SOURCES)
	$(ABS_SRC)/codeGeneration2
