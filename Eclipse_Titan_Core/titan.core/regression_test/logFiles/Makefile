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
#   Szabo, Bence Janos
#
##############################################################################
TOPDIR := ..
include $(TOPDIR)/Makefile.regression

.SUFFIXES: .ttcn .hh
.PHONY: all clean dep run

# Remove any swtches incorporated into the name of the compiler
# to avoid "duplicate switch" errors.
TTCN3_COMPILER := $(filter %compiler, $(TTCN3_COMPILER))

ifndef COMPILER_FLAGS
COMPILER_FLAGS :=
endif

# There is a testcase which requires -L (location information)
COMPILER_FLAGS += -L -g $(RT2_FLAG)

TTCN3_LIB = ttcn3$(RT2_SUFFIX)-parallel$(DYNAMIC_SUFFIX)

TTCN3_MODULES = LogFiles.ttcn CommonStuff.ttcn TitanLogTest.ttcn

GENERATED_SOURCES = $(TTCN3_MODULES:.ttcn=.cc)
GENERATED_HEADERS = $(GENERATED_SOURCES:.cc=.hh)

ifdef CODE_SPLIT
GENERATED_SOURCES := $(foreach file, $(GENERATED_SOURCES:.cc=), $(addprefix $(file), .cc _seq.cc _set.cc  _seqof.cc _setof.cc _union.cc))
else ifdef SPLIT_TO_SLICES
COMPILER_FLAGS += $(SPLIT_FLAG)
POSTFIXES := $(foreach file, $(SPLIT_TO_SLICES), $(addsuffix $(file), _part_))
POSTFIXES := $(foreach file, $(POSTFIXES), $(addprefix $(file), .cc))
GENERATED_SOURCES2 := $(foreach file, $(GENERATED_SOURCES:.cc=), $(addprefix $(file), $(POSTFIXES)))
GENERATED_SOURCES += $(GENERATED_SOURCES2)
endif

USER_SOURCES := $(ABS_SRC)/extfunc.cc

OBJECTS = $(GENERATED_SOURCES:.cc=.o) $(USER_SOURCES:.cc=.o)

TARGET = LogFiles$(EXESUFFIX)

all: $(TARGET)

$(GENERATED_SOURCES) $(GENERATED_HEADERS): compile
	@if [ ! -f $@ ]; then $(RM) compile; $(MAKE) compile; fi

ifeq ($(findstring dynamic,$(TTCN3_LIB)),)
# not dynamic
CORELIB_BINARY := $(TTCN3_DIR)/lib/lib$(TTCN3_LIB).a
else
CORELIB_BINARY := $(TTCN3_DIR)/lib/lib$(TTCN3_LIB).so
endif



$(TARGET): $(GENERATED_SOURCES) $(USER_SOURCES) $(CORELIB_BINARY)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -g $(LDFLAGS) -o $@ $(GENERATED_SOURCES) $(USER_SOURCES) -L$(TTCN3_DIR)/lib -l$(TTCN3_LIB) -L$(OPENSSL_DIR)/lib -lcrypto $($(PLATFORM)_LIBS)

compile: $(TTCN3_MODULES)
	$(TTCN3_COMPILER) $(COMPILER_FLAGS) $^ - $?
	touch $@

clean distclean:
	-rm -f $(TARGET) $(OBJECTS) \
	compile $(GENERATED_HEADERS) $(GENERATED_SOURCES) \
	*.log baseline_logs/*.lo2 Makefile.bak
	rm -rf new_logs

dep: $(GENERATED_SOURCES)
	makedepend $(CPPFLAGS) $(GENERATED_SOURCES) extfunc.cc

run: $(TARGET)
	./logfilter.sh
	$(TTCN3_DIR)/bin/ttcn3_start $(TARGET) LogFiles.cfg
	$(TTCN3_DIR)/bin/ttcn3_start $(TARGET) LogFiles2.cfg
	$(TTCN3_DIR)/bin/ttcn3_start $(TARGET) LogFilesProc.cfg
	$(TTCN3_DIR)/bin/ttcn3_start $(TARGET) TitanLogTest.cfg
	for l in *MTC.log *mtc.log; do grep 'pass (100\.00 %)' >/dev/null $$l || echo "!!!! $$l not 100% !!!!"; done
	# Bangjohansen produces incomplete xml logs sometimes; delete them
	# Check that $j exists: unmatched wildcards are returned as-is
	if xmllint --version 2>/dev/null; then \
	  for j in junit-xml*.log; do \
	    if [ -f $$j ]; then \
	      xmllint --noout $$j || sh -xc "rm $$j"; \
	    else :; fi \
	  done; \
	else :; fi
#./diffLogs new_logs/ baseline_logs/

#LogFiles.o: LogFiles.hh 
#extfunc.o: LogFiles.hh

.NOTPARALLEL:

#ifdef SRCDIR
#$(foreach src, $(USER_SOURCES), $(eval vpath $(src) $(ABS_SRC)))
#endif
# vpath doesn't work with direct cc -> exe
