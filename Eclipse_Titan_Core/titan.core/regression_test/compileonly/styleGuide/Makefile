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
#   Kovacs, Ferenc
#   Raduly, Csaba
#   Szabados, Kristof
#   Szabo, Bence Janos
#   Szabo, Janos Zoltan – initial implementation
#
##############################################################################
TOPDIR := ../..
include $(TOPDIR)/Makefile.regression

.PHONY: all clean dep

TTCN3_LIB := ttcn3$(RT2_SUFFIX)-parallel$(DYNAMIC_SUFFIX)
COMPILER_FLAGS += -t
# Sort also performs uniq
COMPILER_FLAGS := $(sort $(COMPILER_FLAGS))

TTCN3_MODULES := MySample_module.ttcn Another_module.ttcn
ASN1_MODULES := MyASN1_module.asn
TESTPORT_SOURCES := MyTestPortmsg_PT.cc MyTestPortproc_PT.cc

GENERATED_SOURCES := $(TTCN3_MODULES:.ttcn=.cc) $(ASN1_MODULES:.asn=.cc)

GENERATED_HEADERS := $(GENERATED_SOURCES:.cc=.hh)
ifdef CODE_SPLIT
GENERATED_SOURCES := $(foreach file, $(GENERATED_SOURCES:.cc=), $(addprefix $(file), .cc _seq.cc _set.cc  _seqof.cc _setof.cc _union.cc))
else ifdef SPLIT_TO_SLICES
POSTFIXES := $(foreach file, $(SPLIT_TO_SLICES), $(addsuffix $(file), _part_))
POSTFIXES := $(foreach file, $(POSTFIXES), $(addprefix $(file), .cc))
GENERATED_SOURCES2 := $(foreach file, $(GENERATED_SOURCES:.cc=), $(addprefix $(file), $(POSTFIXES)))
GENERATED_SOURCES += $(GENERATED_SOURCES2)
endif
GENERATED_SOURCES += $(TESTPORT_SOURCES)
GENERATED_HEADERS += $(TESTPORT_SOURCES:.cc=.hh)

USER_SOURCES = external.cc

OBJECTS := $(GENERATED_SOURCES:.cc=.o) $(USER_SOURCES:.cc=.o)

TARGET := styleGuide$(EXESUFFIX)

all run: $(TARGET) ;

$(TARGET): $(GENERATED_SOURCES) $(USER_SOURCES)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ -L$(TTCN3_DIR)/lib -l$(TTCN3_LIB) \
	-L$(OPENSSL_DIR)/lib -lcrypto $($(PLATFORM)_LIBS)

$(GENERATED_SOURCES) $(GENERATED_HEADERS): compile
	@if [ ! -f $@ ]; then $(RM) compile; $(MAKE) compile; fi

compile: $(TTCN3_MODULES) $(ASN1_MODULES)
	$(TTCN3_COMPILER) $(COMPILER_FLAGS) $^ - $?
	touch $@

clean distclean:
	-$(RM) $(TARGET) $(OBJECTS) $(GENERATED_HEADERS) \
	$(GENERATED_SOURCES) compile *.log Makefile.bak

dep: $(GENERATED_SOURCES)
	makedepend $(CPPFLAGS) $(USER_SOURCES) $(GENERATED_SOURCES)

external.o: MySample_module.hh Another_module.hh
external.o: MyASN1_module.hh
external.o: MyTestPortmsg_PT.hh MyTestPortproc_PT.hh
MySample_module.o: MySample_module.hh Another_module.hh
MySample_module.o: MyASN1_module.hh
MySample_module.o: MyTestPortmsg_PT.hh MyTestPortproc_PT.hh
Another_module.o: Another_module.hh
MyASN1_module.o: MyASN1_module.hh
MyTestPortmsg_PT.o: MyTestPortmsg_PT.hh MySample_module.hh Another_module.hh
MyTestPortmsg_PT.o: MyASN1_module.hh
MyTestPortmsg_PT.o: MyTestPortproc_PT.hh
MyTestPortproc_PT.o: MyTestPortproc_PT.hh MySample_module.hh MyASN1_module.hh
MyTestPortproc_PT.o: MyTestPortmsg_PT.hh

vpath $(USER_SOURCES) $(ABS_SRC)

