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
#   Cserveni, Akos
#   Delic, Adam
#   Feher, Csaba
#   Forstner, Matyas
#   Kovacs, Ferenc
#   Kremer, Peter
#   Ormandi, Matyas
#   Raduly, Csaba
#   Szabados, Kristof
#   Szabo, Janos Zoltan – initial implementation
#   Szalai, Gabor
#   Tatarka, Gabor
#   Zalanyi, Balazs Andor
#
##############################################################################
# Makefile for the integrated (TTCN-3 and ASN.1) compiler
# and the makefile generator

TOP := ..
include $(TOP)/Makefile.cfg

ifeq ($(USAGE_STATS), yes)
SOLARIS_LIBS  := -lresolv -lsocket -lnsl
SOLARIS8_LIBS := -lresolv -lsocket -lnsl
LINUX_LIBS := -lpthread -lrt
endif

ifeq ($(findstring g++, $(CXX)), g++)
  CXXFLAGS += -fno-exceptions
endif

ifdef REGEX_DIR
  CPPFLAGS += -I$(REGEX_DIR)/include
endif

ifdef OPENSSL_DIR
ifneq ($(OPENSSL_DIR), default)
  CPPFLAGS += -I$(OPENSSL_DIR)/include
  LDFLAGS  += -L$(OPENSSL_DIR)/lib
endif
endif

ifdef XMLDIR
ifeq ($(XMLDIR), default)
  XMLDIR := /usr
  # do not add -L/usr/lib to LDFLAGS, it's redundant
else
  LDFLAGS  += -L$(XMLDIR)/lib
endif

  CPPFLAGS += -I$(XMLDIR)/include/libxml2
endif

ifdef SRCDIR
  CPPFLAGS += -Iasn1
endif

# Too many "comparison between signed and unsigned"
CCFLAGS += -Wno-sign-compare

ifndef MINGW
TARGETS := compiler$(EXESUFFIX) ttcn3_makefilegen$(EXESUFFIX) tcov2lcov$(EXESUFFIX)
MANPAGES := compiler.1 ttcn3_makefilegen.1
else
TARGETS := compiler$(EXESUFFIX)
MANPAGES := compiler.1
MAYBE_PDFS := $(MANPAGES:.1=.pdf)
endif

PDFS := $(MANPAGES:.1=.pdf)

COMPILER_SOURCES := encdec.c enum.c functionref.c record.c record_of.c union.c \
PredefFunc.cc AST.cc Code.cc Constraint.cc CompilerError.cc \
CompField.cc CompType.cc EnumItem.cc Identifier.cc Int.cc \
main.cc Real.cc Setting.cc SigParam.cc string.cc subtype.cc Stopwatch.cc \
Type.cc Type_chk.cc Type_codegen.cc TypeCompat.cc \
Typestuff.cc ustring.cc Value.cc Valuestuff.cc XerAttributes.cc subtypestuff.cc \
CodeGenHelper.cc DebuggerStuff.cc XSD_Types.cc

MFGEN_SOURCES := makefile.c xpather.cc ProjectGenHelper.cc

TCOV2LCOV_SOURCES := tcov2lcov.cc

SOURCES := $(COMPILER_SOURCES) $(MFGEN_SOURCES) $(TCOV2LCOV_SOURCES)

# All sources are static in this directory
STATIC_SOURCES := $(SOURCES)

COMPILER_OBJECTS := $(patsubst %.cc,%.o,$(patsubst %.c,%.o,$(COMPILER_SOURCES)))

MFGEN_OBJECTS := $(patsubst %.cc,%.o,$(patsubst %.c,%.o,$(MFGEN_SOURCES)))

TCOV2LCOV_OBJECTS := $(patsubst %.cc,%.o,$(patsubst %.c,%.o,$(TCOV2LCOV_SOURCES)))

OBJECTS := $(COMPILER_OBJECTS) $(MFGEN_OBJECTS) $(TCOV2LCOV_OBJECTS)

# Used by both the compiler and makefilegen
COMMON_OBJECTS := $(addprefix ../common/, memory.o path.o userinfo.o)

TCOV2LCOV_COMMON_OBJECTS := ../common/memory.o

ifeq ($(LICENSING), yes)
	COMMON_OBJECTS += ../common/license.o
	TCOV2LCOV_COMMON_OBJECTS += ../common/license.o
endif

ifdef REGEX_DIR
  COMMON_OBJECTS += $(REGEX_DIR)/lib/regex.o
endif

COMPILER_COMMON_OBJECTS := $(COMMON_OBJECTS) \
$(addprefix ../common/, new.o pattern_la.o pattern_p.o pattern_uni.o Quadruple.o \
ModuleVersion.o JSON_Tokenizer.o UnicharPattern.o openssl_version.o)

ifeq ($(USAGE_STATS), yes)
COMPILER_COMMON_OBJECTS += ../common/usage_stats.o
endif

MFGEN_COMMON_OBJECTS := $(COMMON_OBJECTS) ../common/new.o

DEPFILES := $(patsubst %.cc,%.d,$(patsubst %.c,%.d,$(COMPILER_SOURCES) \
$(MFGEN_SOURCES)))

SUBDIRS := asn1 ttcn3

.PHONY: asn1_dir ttcn3_dir

all run: $(TARGETS)

asn1/libasn1_compiler.a: asn1_dir ;

ttcn3/libttcn3_compiler.a: ttcn3_dir ;

asn1_dir:
	$(MAKE) -C asn1

ttcn3_dir:
	$(MAKE) -C ttcn3

pdf: $(PDFS)

install: $(TARGETS) $(MAYBE_PDFS)
ifeq ($(DEBUG), no)
	$(STRIP) $(TARGETS)
endif
	mkdir -p $(BINDIR)
	cp $(TARGETS) $(BINDIR)
ifdef MINGW
	mkdir -p $(MANDIR)
	cp $(PDFS)    $(MANDIR)
else
	cp tcov.xsd tcov.xsl $(INCDIR)
	cp titanver $(BINDIR)
	chmod a+x $(BINDIR)/titanver
	$(RM) $(BINDIR)/ttcn3_compiler$(EXESUFFIX) \
	$(BINDIR)/asn1_compiler$(EXESUFFIX) $(BINDIR)/makefilegen$(EXESUFFIX)
	ln -s compiler$(EXESUFFIX) $(BINDIR)/ttcn3_compiler$(EXESUFFIX)
	ln -s compiler$(EXESUFFIX) $(BINDIR)/asn1_compiler$(EXESUFFIX)
	ln -s ttcn3_makefilegen$(EXESUFFIX) $(BINDIR)/makefilegen$(EXESUFFIX)
	mkdir -p $(MANDIR)/man1
	cp $(MANPAGES) $(MANDIR)/man1
	$(RM) $(MANDIR)/man1/ttcn3_compiler.1 $(MANDIR)/man1/asn1_compiler.1 \
	$(MANDIR)/man1/makefilegen.1
	ln -s compiler.1 $(MANDIR)/man1/ttcn3_compiler.1
	ln -s compiler.1 $(MANDIR)/man1/asn1_compiler.1
	ln -s ttcn3_makefilegen.1 $(MANDIR)/man1/makefilegen.1
endif

# The compiler always links to OpenSSL (even without LICENSING)
# because it uses bignums.
compiler$(EXESUFFIX): asn1/libasn1_compiler.a ttcn3/libttcn3_compiler.a \
$(COMPILER_OBJECTS) $(COMPILER_COMMON_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $(COMPILER_OBJECTS) $(COMPILER_COMMON_OBJECTS) \
	asn1/libasn1_compiler.a ttcn3/libttcn3_compiler.a \
	-lcrypto $(RSLICLIB) $(MINGW_LIBS) $($(PLATFORM)_LIBS)

ttcn3_makefilegen$(EXESUFFIX): $(MFGEN_OBJECTS) $(MFGEN_COMMON_OBJECTS) \
ttcn3/ttcn3_preparser.lex.o asn1/asn1_preparser.lex.o
	$(CXX) $(LDFLAGS) -o $@ $^ -lxml2 $(LICENSE_LIBS) $(MINGW_LIBS)

tcov2lcov$(EXESUFFIX): $(TCOV2LCOV_OBJECTS) $(TCOV2LCOV_COMMON_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -lxml2 $(LICENSE_LIBS) $(MINGW_LIBS)

include ../Makefile.genrules

# Target-specific variable alteration
# Target-specific += causes make 3.79.1 to crash, hence this old style
makefile.o: CPPFLAGS :=  $(CPPFLAGS) -DCXX='"$(CXX)"'

# Pre-seed dependencies. Value.cc and Valuestuff.cc include asn1/asn1p.tab.hh
# However, if asn1/asn1p.y is modified, these files are not rebuilt because
# the Value.d contains a dependency on asn1/asn1p.tab.hh, which is not yet
# regenerated.
#
# 1. asn1/asn1p.y is modified
# 2. "make" checks dependencies of Value.o; decides nothing to do
# 3. "make -C asn1" rebuilds asn1p.y, updating asn1p.tab.hh
# 4. Value.o is now out of date
# 5. "make" issued a second time decides that Value.o is out of date
Value.d      Value.o      : asn1/asn1p.tab.hh
Valuestuff.d Valuestuff.o : asn1/asn1p.tab.hh
Constraint.d Constraint.o : asn1/asn1p.tab.hh
