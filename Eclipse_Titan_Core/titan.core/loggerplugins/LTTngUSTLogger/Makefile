##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#   Raduly, Csaba
#   Busque, Antoine
#   Proulx, Philippe
#
##############################################################################

TOP    := ../..
include   ../../Makefile.cfg

LIB_DIR := $(TTCN3_DIR)/lib

MAJOR := 1
MINOR := 0

TPP := lttngust-logger-tp.so
TPP_SRC := tp.cc
TPP_OBJ := $(TPP_SRC:.cc=.o)
TPP_HEADER := $(TPP_SRC:.cc=.hh)

SOURCES := LTTngUSTLogger.cc
STATIC_SOURCES := ${SOURCES} $(LTTNG_UST_SOURCES)
HEADERS := $(SOURCES:.cc=.hh) $(TPP_HEADER)
OBJECTS := $(SOURCES:.cc=.o)
OBJECTS_RT2 := $(addprefix FT/,$(OBJECTS))

SHARED_LIB              := liblttng-ust-logger.so
SHARED_LIB_RT2          := liblttng-ust-logger-rt2.so
SHARED_LIB_PARALLEL     := liblttng-ust-logger-parallel.so
SHARED_LIB_PARALLEL_RT2 := liblttng-ust-logger-parallel-rt2.so

CPPFLAGS += -I../../core -I$(ABS_SRC)/../../common -I$(ABS_SRC)/../../core -I.
# RT2 needs core2 (for RT2/TitanLoggerAPI.hh) in addition to core
CPPFLAGS_RT2 := $(CPPFLAGS) -I$(ABS_SRC)/../../core2 -DTITAN_RUNTIME_2

CXXFLAGS += -Werror
COMMON_LDFLAGS       += -ldl
LDFLAGS_STD          += $(COMMON_LDFLAGS) -g -L$(ABS_SRC)/../../core  -Wl,-soname,$(SHARED_LIB).$(MAJOR)              -o $(SHARED_LIB).$(MAJOR).$(MINOR)
LDFLAGS_RT2          += $(COMMON_LDFLAGS) -g -L$(ABS_SRC)/../../core2 -Wl,-soname,$(SHARED_LIB_RT2).$(MAJOR)          -o $(SHARED_LIB_RT2).$(MAJOR).$(MINOR)
LDFLAGS_PARALLEL     += $(COMMON_LDFLAGS) -g -L$(ABS_SRC)/../../core  -Wl,-soname,$(SHARED_LIB_PARALLEL).$(MAJOR)     -o $(SHARED_LIB_PARALLEL).$(MAJOR).$(MINOR)
LDFLAGS_PARALLEL_RT2 += $(COMMON_LDFLAGS) -g -L$(ABS_SRC)/../../core2 -Wl,-soname,$(SHARED_LIB_PARALLEL_RT2).$(MAJOR) -o $(SHARED_LIB_PARALLEL_RT2).$(MAJOR).$(MINOR)

LIBS              := -lttcn3-dynamic
LIBS_RT2          := -lttcn3-rt2-dynamic
LIBS_PARALLEL     := -lttcn3-parallel-dynamic
LIBS_PARALLEL_RT2 := -lttcn3-rt2-parallel-dynamic

TARGETS := $(TPP) $(SHARED_LIB) $(SHARED_LIB_PARALLEL) $(SHARED_LIB_RT2) $(SHARED_LIB_PARALLEL_RT2)
# .so with .major appended:
TARGETS_MAJOR       := $(addsuffix .$(MAJOR), $(TARGETS))
# .so with .major.minor appended:
TARGETS_MAJOR_MINOR := $(addsuffix .$(MINOR), $(TARGETS_MAJOR))

# OBJECTS_RT2, TARGETS_MAJOR and TARGETS_MAJOR_MINOR are non-standard make variables,
# not taken into account by "clean" in Makefile.genrules
# Delete them as "miscellaneous" files.
TOBECLEANED := $(OBJECTS_RT2) $(TARGETS_MAJOR) $(TARGETS_MAJOR_MINOR) $(TPP_OBJ) $(TPP)

all run: $(TARGETS)

$(SHARED_LIB): $(OBJECTS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS_STD) $^ $(LIBS) -shared
	ln -sf $@.$(MAJOR).$(MINOR) $@.$(MAJOR)
	ln -sf $@.$(MAJOR) $@

$(SHARED_LIB_RT2): $(OBJECTS_RT2)
	$(CXX) $(CPPFLAGS_RT2) $(CXXFLAGS) $(LDFLAGS_RT2) $^ $(LIBS_RT2) -shared
	ln -sf $@.$(MAJOR).$(MINOR) $@.$(MAJOR)
	ln -sf $@.$(MAJOR) $@

$(SHARED_LIB_PARALLEL): $(OBJECTS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS_PARALLEL) $^ $(LIBS_PARALLEL) -shared
	ln -sf $@.$(MAJOR).$(MINOR) $@.$(MAJOR)
	ln -sf $@.$(MAJOR) $@

$(SHARED_LIB_PARALLEL_RT2): $(OBJECTS_RT2)
	$(CXX) $(CPPFLAGS_RT2) $(CXXFLAGS) $(LDFLAGS_PARALLEL_RT2) $^ $(LIBS_PARALLEL_RT2) -shared
	ln -sf $@.$(MAJOR).$(MINOR) $@.$(MAJOR)
	ln -sf $@.$(MAJOR) $@

$(OBJECTS): $(SOURCES)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $?

# The `-o $@' stuff is necessary, otherwise the result will be put into the
# current directory.
$(OBJECTS_RT2): $(SOURCES)
	mkdir -p FT
	$(CXX) $(CPPFLAGS_RT2) $(CXXFLAGS) -c $? -o $@

$(TPP_OBJ): $(TPP_SRC) $(TPP_HEADER)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -I. -c -fpic $(TPP_SRC)

$(TPP): $(TPP_OBJ)
	$(CXX) -o $@ $(LDFLAGS) -shared $^ -llttng-ust

dep:
	@echo Doing nothing...

install: $(SHARED_LIB) $(SHARED_LIB_RT2) $(SHARED_LIB_PARALLEL) $(SHARED_LIB_PARALLEL_RT2) $(TPP)
	mkdir -p $(LIB_DIR)
	cp $(SHARED_LIB)* $(SHARED_LIB_RT2)* $(SHARED_LIB_PARALLEL)* $(SHARED_LIB_PARALLEL_RT2)* $(LIB_DIR)
	cp $(TPP) $(LIB_DIR)

include ../../Makefile.genrules
