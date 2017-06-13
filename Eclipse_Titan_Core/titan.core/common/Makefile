##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   
#   Baji, Laszlo
#   Balasko, Jeno
#   Baranyi, Botond
#   Beres, Szabolcs
#   Csondes, Tibor
#   Delic, Adam
#   Forstner, Matyas
#   Kovacs, Ferenc
#   Ormandi, Matyas
#   Raduly, Csaba
#   Szabo, Janos Zoltan – initial implementation
#   Zalanyi, Balazs Andor
#
##############################################################################
# Makefile for the common directory of the TTCN-3 Test Executor

TOP := ..
include $(TOP)/Makefile.cfg

TARGETS :=

# The origin of GENERATED_SOURCES
ORIGINATORS := pattern_la.l  pattern_p.y  pattern_uni.y  config_preproc_la.l config_preproc_p.y

GENERATED_SOURCES := pattern_la.cc pattern_p.cc pattern_uni.cc config_preproc_la.cc config_preproc_p.tab.cc

# Sources in the CVS

STATIC_SOURCES := memory.c new.cc userinfo.c path.c config_preproc.cc Quadruple.cc \
		  Path2.cc ModuleVersion.cc JSON_Tokenizer.cc UnicharPattern.cc openssl_version.c

ifndef MINGW
STATIC_SOURCES += NetworkHandler.cc
endif

ifeq ($(USAGE_STATS), yes)
  STATIC_SOURCES += usage_stats.cc
endif

ifeq ($(LICENSING), yes)
  STATIC_SOURCES += license.c
  ifeq ($(PLATFORM), WIN32)
    STATIC_SOURCES += hostid.c
    TARGETS += hostid$(EXESUFFIX)
  endif
  ifeq ($(PLATFORM), INTERIX)
    STATIC_SOURCES += hostid.c
    TARGETS += hostid$(EXESUFFIX)
  endif
endif

SOURCES := $(STATIC_SOURCES) $(GENERATED_SOURCES)

GENERATED_HEADERS := pattern_p.hh pattern_uni.hh config_preproc_p.tab.hh

GENERATED_OTHERS := pattern_p.output pattern_uni.output lex.backup

OBJECTS := $(patsubst %.cc,%.o,$(patsubst %.c,%.o,$(SOURCES)))

DEPFILES := $(patsubst %.cc,%.d,$(patsubst %.c,%.d,$(SOURCES)))

SUBDIRS :=

CPPFLAGS += -D_REENTRANT

ifdef REGEX_DIR
  CPPFLAGS += -I$(REGEX_DIR)/include
endif

all run: $(OBJECTS) $(TARGETS)

install: $(OBJECTS) $(TARGETS)
ifdef MINGW
	@echo Skipped ${CURDIR} for MinGW
else
	mkdir -p $(INCDIR)
	cp version.h memory.h platform.h ttcn3float.hh pattern.hh CharCoding.hh JSON_Tokenizer.hh $(INCDIR)
ifeq ($(DEBUG), yes)
	cp dbgnew.hh $(INCDIR)
endif
ifneq ($(TARGETS), )
  ifeq ($(DEBUG), no)
	$(STRIP) $(TARGETS)
  endif
	mkdir -p $(BINDIR)
	cp $(TARGETS) $(BINDIR)
endif
endif

hostid$(EXESUFFIX): hostid.o license.o memory.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LICENSE_LIBS) $(MINGW_LIBS)

pattern_la.cc: pattern_la.l
	$(FLEX) $(FLEXFLAGS) -o$@ $<

pattern_p.cc pattern_p.hh: pattern_p.y
	$(BISON) $(BISONFLAGS) $<

pattern_uni.cc pattern_uni.hh: pattern_uni.y
	$(BISON) $(BISONFLAGS) $<

config_preproc_la.cc: config_preproc_la.l
	$(FLEX) $(FLEXFLAGS) -o$@ $<

config_preproc_p.tab.cc config_preproc_p.tab.hh: config_preproc_p.y
	$(BISON) $(BISONFLAGS) -o config_preproc_p.tab.cc -p config_process_ $<

# Target-specific variable alteration
# Target-specific += causes make 3.79.1 to crash, hence this old style
ifdef OPENSSL_DIR
ifneq ($(OPENSSL_DIR), default)
license.o license.d: CPPFLAGS := $(CPPFLAGS) -I$(OPENSSL_DIR)/include
openssl_version.o openssl_version.d: CPPFLAGS := $(CPPFLAGS) -I$(OPENSSL_DIR)/include
endif
endif

include ../Makefile.genrules


#vpath pattern_la.l        $(ABS_SRC)
#vpath pattern_p.y         $(ABS_SRC)
#vpath pattern_uni_lex.l   $(ABS_SRC)
#vpath pattern_uni.y       $(ABS_SRC)
#vpath config_preproc_la.l $(ABS_SRC)
#vpath config_preproc_p.y  $(ABS_SRC)

# no vpath for GENERATED_SOURCES !

