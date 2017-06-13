##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   
#   >
#   Baji, Laszlo
#   Balasko, Jeno
#   Bene, Tamas
#   Beres, Szabolcs
#   Csondes, Tibor
#   Czimbalmos, Eduard
#   Delic, Adam
#   Feher, Csaba
#   Forstner, Matyas
#   Gecse, Roland
#   Kovacs, Ferenc
#   Kremer, Peter
#   Raduly, Csaba
#   Szabados, Kristof
#   Szabo, Janos Zoltan – initial implementation
#   Varga, Vilmos
#
##############################################################################
# Makefile for the TTCN-3 Main Controller

TOP := ../..
include $(TOP)/Makefile.cfg

TARGETS :=

ifeq ($(CLI), yes)
    TARGETS += mctr_cli$(EXESUFFIX)
endif

ORIGINATORS := main.cc

GENERATED_SOURCES := Cli_main.cc

STATIC_SOURCES := MctrError.cc MainController.cc UserInterface.cc config_data.cc

SOURCES := $(STATIC_SOURCES) $(GENERATED_SOURCES)

GENERATED_HEADERS :=

GENERATED_OTHERS :=

OBJECTS := $(patsubst %.cc,%.o,$(patsubst %.c,%.o,$(SOURCES)))

OBJECTS_NOMAIN := $(filter-out Cli_main.o, $(OBJECTS))

COMMON_OBJECTS := $(addprefix ../../common/, memory.o config_preproc.o \
config_preproc_la.o config_preproc_p.tab.o path.o NetworkHandler.o \
Path2.o openssl_version.o) ../../core/RInt.o ../../core/Textbuf.o

ifeq ($(LICENSING), yes)
    COMMON_OBJECTS += ../../common/license.o
endif

COMMON_OBJECTS_CLI := $(COMMON_OBJECTS)

DEPFILES := $(patsubst %.cc,%.d,$(patsubst %.c,%.d,$(SOURCES)))

SUBDIRS :=

CPPFLAGS += -D_REENTRANT -I../../core -I../../common

ifdef OPENSSL_DIR
ifneq ($(OPENSSL_DIR), default)
  CPPFLAGS += -I$(OPENSSL_DIR)/include
  LDFLAGS  += -L$(OPENSSL_DIR)/lib
endif
endif

# Platform specific libraries that are necessary for CLI # versions.
SOLARIS_LIBS := -lpthread -lsocket -lnsl
SOLARIS8_LIBS := -lpthread -lsocket -lnsl
LINUX_LIBS := -lpthread
FREEBSD_LIBS := -lpthread
WIN32_LIBS := -lpthread

SOLARIS_CLI_LIBS := -lcurses
SOLARIS8_CLI_LIBS := -lcurses
LINUX_CLI_LIBS := -lncurses
FREEBSD_CLI_LIBS := -lncurses
WIN32_CLI_LIBS := -lncurses
INTERIX_CLI_LIBS := -lncurses

all run: $(TARGETS)

mctr_cli$(EXESUFFIX): Cli_main.o $(OBJECTS_NOMAIN) $(COMMON_OBJECTS_CLI) \
../cli/libcli.a ../editline/build/src/.libs/libedit.a
	$(CXX) $(LDFLAGS) -o $@ $^ \
	-lcrypto $($(PLATFORM)_CLI_LIBS) $($(PLATFORM)_LIBS) $(RSLICLIB)

install: $(TARGETS)
ifeq ($(DEBUG), no)
	$(STRIP) $(TARGETS)
endif
	mkdir -p $(BINDIR)
	cp $(TARGETS) $(BINDIR)
ifeq ($(CLI), yes)
	ln -s mctr_cli$(EXESUFFIX) $(BINDIR)/mctr$(EXESUFFIX)
endif
	cp ttcn3_start $(BINDIR)
	chmod +x $(BINDIR)/ttcn3_start

Cli_main.cc: main.cc
	@echo "#define CLI" > $@ && cat $< >> $@

include ../../Makefile.genrules
