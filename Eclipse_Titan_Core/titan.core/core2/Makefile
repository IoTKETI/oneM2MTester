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
#
##############################################################################
# Makefile for the function test version Base Library of TTCN-3 Test Executor

FUNCTION_TEST_RUNTIME := yes

RT2_SOURCES := Basetype2.cc

ifndef SRCDIR
SRCDIR := ..
endif

include ../core/Makefile

CPPFLAGS += -I../core

ifdef SRCDIR
CPPFLAGS += -I$(SRCDIR)/core
endif

# Tell make that .c and .cc files can be found in another directory.
# Selective (lowercase) vpath is a GNU make feature.

vpath %.c  $(SRCDIR)/core
vpath %.cc ../core:$(SRCDIR)/core
vpath %.xsd $(SRCDIR)/core
vpath %.ttcn $(SRCDIR)/core
vpath cversion.h $(SRCDIR)/core
