##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   
#   Balasko, Jeno
#   Godar, Marton
#   Pilisi, Gergely
#   Raduly, Csaba
#   Szabo, Bence Janos
#
##############################################################################
TOP := ..
include $(TOP)/Makefile.cfg

SOURCES :=	converter.cc \
			XMLParser.cc TTCN3Module.cc TTCN3ModuleInventory.cc \
			RootType.cc SimpleType.cc AttributeType.cc ComplexType.cc \
			Annotation.cc ImportStatement.cc \
			PredefinedModules.cc GeneralFunctions.cc \
			Mstring.cc Constant.cc

# No generated sources
STATIC_SOURCES := $(SOURCES)

HEADERS := $(SOURCES:.cc=.hh)


OBJECTS :=     $(SOURCES:.cc=.o) \
                        $(TOP)/common/new.o \
                        $(TOP)/common/memory.o

ifeq ($(LICENSING), yes)
  OBJECTS += $(TOP)/common/license.o
endif


TARGETS := xsd2ttcn$(EXESUFFIX)

DEPFILES := $(patsubst %.cc,%.d,$(patsubst %.c,%.d,$(SOURCES)))

ifdef XMLDIR
ifeq ($(XMLDIR), default)
  # libxml2 headers supplied by the system are usually in /usr/include/libxml2,
  # so a -I is required. Rename XMLDIR for use in the common case.
  XMLDIR := /usr
  # do not add -L/usr/lib to LDFLAGS, it's redundant
else
  LDFLAGS  += -L$(XMLDIR)/lib
endif
  # Always add to CPPFLAGS, even if "default"
  CPPFLAGS += -I$(XMLDIR)/include/libxml2
endif

ifdef OPENSSL_DIR
ifneq ($(OPENSSL_DIR), default)
  CPPFLAGS += -I$(OPENSSL_DIR)/include
  LDFLAGS  += -L$(OPENSSL_DIR)/lib
endif
endif



ifeq ($(PLATFORM),INTERIX)
  # to get the "right" iconv, used by libxml2
  CPPFLAGS += -I/usr/local/include
endif

# libraries for the linker
LDLIBS += -lxml2 -lcrypto

ifdef MINGW
LDLIBS += -lregex
endif

all run: $(TARGETS) $(PROGRAMS)

xsd2ttcn$(EXESUFFIX): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS) $(MINGW_LIBS)

install: $(TARGETS)
ifdef MINGW
	mkdir -p      $(TTCN3_DIR)/programs
	cp $(TARGETS) $(TTCN3_DIR)/programs
else
	mkdir -p $(BINDIR)
	cp -f $(TARGETS) $(BINDIR)
endif

include $(TOP)/Makefile.genrules
