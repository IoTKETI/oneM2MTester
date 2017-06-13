##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   >
#   Balasko, Jeno
#   Bene, Tamas
#   Delic, Adam
#   Forstner, Matyas
#   Gecse, Roland
#   Raduly, Csaba
#   Szabados, Kristof
#   Szabo, Janos Zoltan – initial implementation
#
##############################################################################
# Makefile for the Command Line Interface of the TTCN-3 Main Controller

TOP := ../..
include $(TOP)/Makefile.cfg

TARGETS := libcli.a

ORIGINATORS := config_read.l config_read.y

GENERATED_SOURCES := config_read.lex.cc config_read.tab.cc

STATIC_SOURCES := Cli.cc

SOURCES := $(STATIC_SOURCES) $(GENERATED_SOURCES)

GENERATED_HEADERS := config_read.tab.hh

GENERATED_OTHERS := config_read.output lex.backup

OBJECTS := $(patsubst %.cc,%.o,$(patsubst %.c,%.o,$(SOURCES)))

DEPFILES := $(patsubst %.cc,%.d,$(patsubst %.c,%.d,$(SOURCES)))

CPPFLAGS += -D_REENTRANT

ifdef OPENSSL_DIR
ifneq ($(OPENSSL_DIR), default)
  CPPFLAGS += -I$(OPENSSL_DIR)/include
  LDFLAGS  += -L$(OPENSSL_DIR)/lib
endif
endif

ifeq ($(findstring g++, $(CXX)), g++)
  CXXFLAGS += -fno-exceptions
endif

all run: $(TARGETS)

libcli.a: $(OBJECTS)
	$(AR) -r $@ $?

config_read.lex.cc: config_read.l
	$(FLEX) $(FLEXFLAGS) -Pconfig_read_ -o$@ $<

config_read.tab.cc config_read.tab.hh: config_read.y
	$(BISON) $(BISONFLAGS) -o config_read.tab.cc -p config_read_ $<

include ../../Makefile.genrules
