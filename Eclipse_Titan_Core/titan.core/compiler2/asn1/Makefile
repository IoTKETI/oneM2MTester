##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#   Cserveni, Akos
#   Forstner, Matyas
#   Kovacs, Ferenc
#   Kremer, Peter
#   Raduly, Csaba
#   Szabo, Janos Zoltan – initial implementation
#
##############################################################################
# Makefile for the ASN.1 Compiler

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

TARGETS := libasn1_compiler.a

ORIGINATORS := asn1_preparser.l asn1la.l asn1p.y

GENERATED_SOURCES := asn1la.yy.cc asn1p.tab.cc asn1_preparser.lex.c

STATIC_SOURCES := AST_asn1.cc Ref.cc Block.cc Object.cc OCSV.cc \
Tag.cc TableConstraint.cc TokenBuf.cc Type_parse.cc

SOURCES := $(STATIC_SOURCES) $(GENERATED_SOURCES)

GENERATED_HEADERS := asn1p.tab.hh

GENERATED_OTHERS := asn1p.output lex.backup

OBJECTS := $(patsubst %.cc,%.o,$(patsubst %.c,%.o,$(SOURCES)))

DEPFILES := $(patsubst %.cc,%.d,$(patsubst %.c,%.d,$(SOURCES)))

SUBDIRS :=

all run: $(TARGETS)

libasn1_compiler.a: $(OBJECTS)
	$(AR) -r $@ $?

asn1_preparser.lex.c: asn1_preparser.l
	$(FLEX) $(FLEXFLAGS) -Pasn1_preparser_ -o$@ $<

asn1la.yy.cc: asn1la.l
	$(FLEX) $(FLEXFLAGS) -o$@ $<

asn1p.tab.cc asn1p.tab.hh: asn1p.y
	$(BISON) $(BISONFLAGS) $<

include ../../Makefile.genrules

# pre-seed dependencies
Type_parse.d TokenBuf.d TableConstraint.d OCSV.d Object.d Block.d Ref.d AST_asn1.d asn1la.yy.d: asn1p.tab.hh
