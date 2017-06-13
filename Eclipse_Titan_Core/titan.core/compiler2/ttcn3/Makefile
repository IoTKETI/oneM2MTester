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
#   Cserveni, Akos
#   Delic, Adam
#   Forstner, Matyas
#   Kovacs, Ferenc
#   Kremer, Peter
#   Raduly, Csaba
#   Szabados, Kristof
#   Szabo, Janos Zoltan – initial implementation
#   Szalai, Gabor
#   Tatarka, Gabor
#   Zalanyi, Balazs Andor
#
##############################################################################
# Makefile for the TTCN-3 part of the compiler

TOP := ../..
include $(TOP)/Makefile.cfg

ifeq ($(findstring g++, $(CXX)), g++)
  CXXFLAGS += -fno-exceptions
endif

ifdef OPENSSL_DIR
ifneq ($(OPENSSL_DIR), default)
  CPPFLAGS += -I$(OPENSSL_DIR)/include
endif
endif

CPPFLAGS += -I$(SRC_TOP)

TARGETS := libttcn3_compiler.a

ORIGINATORS := charstring_la.l  coding_attrib_la.l  coding_attrib_p.y  \
compiler.l  compiler.y  comptype_attrib_la.l  pstring_la.l  \
rawAST.l  rawAST.y  ttcn3_preparser.l

GENERATED_SOURCES := lex.ttcn3.cc compiler.tab.cc lex.rawAST.cc rawAST.tab.cc \
charstring_la.cc pstring_la.cc comptype_attrib_la.cc coding_attrib_la.cc \
coding_attrib_p.cc ttcn3_preparser.lex.c

GENERATED_HEADERS := compiler.tab.hh rawAST.tab.hh coding_attrib_p.hh

GENERATED_OTHERS := compiler.output rawAST.output coding_attrib_p.output lex.backup

STATIC_SOURCES := ArrayDimensions.cc AST_ttcn3.cc Attributes.cc ILT.cc PatternString.cc \
RawAST.cc Statement.cc TtcnTemplate.cc Templatestuff.cc TextAST.cc Ttcnstuff.cc \
compiler.c port.c signature.c BerAST.cc JsonAST.cc Ttcn2Json.cc profiler.c

SOURCES := $(STATIC_SOURCES) $(GENERATED_SOURCES)
	
COMMON_OBJECTS := $(addprefix ../../common/, JSON_Tokenizer.o)

OBJECTS := $(patsubst %.cc,%.o,$(patsubst %.c,%.o,$(SOURCES))) $(COMMON_OBJECTS)

DEPFILES := $(patsubst %.cc,%.d,$(patsubst %.c,%.d,$(SOURCES)))

SUBDIRS :=

all run: $(TARGETS)

libttcn3_compiler.a: $(OBJECTS)
	$(AR) -r $@ $?

%_la.cc: %_la.l
	$(FLEX) $(FLEXFLAGS) -o$@ $<

%_p.cc %_p.hh: %_p.y
	$(BISON) $(BISONFLAGS) $<

compiler.tab.cc compiler.tab.hh: compiler.y
	$(BISON) $(BISONFLAGS) -p ttcn3_ -o compiler.tab.cc $<

rawAST.tab.cc rawAST.tab.hh: rawAST.y
	$(BISON) $(BISONFLAGS) -p rawAST_ -o rawAST.tab.cc $<

lex.ttcn3.cc: compiler.l
	$(FLEX) $(FLEXFLAGS) -Pttcn3_ -o$@ $<

lex.rawAST.cc: rawAST.l
	$(FLEX) $(FLEXFLAGS) -PrawAST_ -o$@ $<

ttcn3_preparser.lex.c: ttcn3_preparser.l
	$(FLEX) $(FLEXFLAGS) -Pttcn3_preparser_ -o$@ $<

include ../../Makefile.genrules

# pre-seed dependencies
lex.ttcn3.d: compiler.tab.hh
