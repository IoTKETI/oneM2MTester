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
#   Kovacs, Ferenc
#   Raduly, Csaba
#   Szabados, Kristof
#   Szabo, Bence Janos
#   Szabo, Janos Zoltan – initial implementation
#   Tatarka, Gabor
#
##############################################################################
TOPDIR := ..
include $(TOPDIR)/Makefile.regression

.SUFFIXES: .ttcn .hh
.PHONY: all clean dep run

TTCN3_LIB := ttcn3$(RT2_SUFFIX)-parallel$(DYNAMIC_SUFFIX)

TTCN3_MODULES := ProcPort.ttcn

ifdef RT2
TTCN3_MODULES += AdvancedRedirects.ttcn
endif

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
GENERATED_SOURCES += CompileOnlyPort.cc CompileOnlyPortAddress.cc
GENERATED_HEADERS += CompileOnlyPort.hh CompileOnlyPortAddress.hh

USER_SOURCES := ExtProcPort.cc PortAddress.cc
USER_HEADERS := $(USER_SOURCES:.cc=.hh)

OBJECTS := $(GENERATED_SOURCES:.cc=.o) $(USER_SOURCES:.cc=.o)

TARGET := ProcPort$(EXESUFFIX)

COMPILER_FLAGS := -t

# Generated code has many shadowed definitions.
# Also, skeletons have many unused parameters.
# This will override any previous -Wshadow / -Wunused-parameter
CXXFLAGS += -Wno-shadow -Wno-unused-parameter

all: $(TARGET) ;

$(TARGET): $(GENERATED_SOURCES) $(USER_SOURCES)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ \
	-L$(TTCN3_DIR)/lib -l$(TTCN3_LIB) \
	-L$(OPENSSL_DIR)/lib -lcrypto $($(PLATFORM)_LIBS)

$(GENERATED_SOURCES) $(GENERATED_HEADERS) $(USER_SOURCES) $(USER_HEADERS): compile
	@if [ ! -f $@ ]; then $(RM) compile; $(MAKE) compile; fi

compile: $(TTCN3_MODULES)
	$(TTCN3_COMPILER) $(COMPILER_FLAGS) $^ - $?
	touch $@

clean distclean:
	-rm -f $(TARGET) $(OBJECTS) $(GENERATED_HEADERS) \
	$(GENERATED_SOURCES) compile *.log Makefile.bak

dep: $(GENERATED_SOURCES)
	makedepend $(CPPFLAGS) $(USER_SOURCES) $(GENERATED_SOURCES)

run: $(TARGET)
	$(TTCN3_DIR)/bin/ttcn3_start $(TARGET) config$(RT2_SUFFIX).cfg


ExtProcPort.o: ExtProcPort.hh ProcPort.hh
ExtProcPort.o: PortAddress.hh CompileOnlyPort.hh CompileOnlyPortAddress.hh
PortAddress.o: PortAddress.hh ProcPort.hh
PortAddress.o: ExtProcPort.hh CompileOnlyPort.hh CompileOnlyPortAddress.hh
ProcPort.o: ProcPort.hh  PortAddress.hh ExtProcPort.hh
ProcPort.o: CompileOnlyPort.hh CompileOnlyPortAddress.hh
CompileOnlyPort.o: CompileOnlyPort.hh ProcPort.hh
CompileOnlyPort.o: PortAddress.hh
CompileOnlyPort.o: ExtProcPort.hh CompileOnlyPortAddress.hh
CompileOnlyPortAddress.o: CompileOnlyPortAddress.hh ProcPort.hh
CompileOnlyPortAddress.o: PortAddress.hh
CompileOnlyPortAddress.o: ExtProcPort.hh CompileOnlyPort.hh
