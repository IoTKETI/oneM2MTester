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
#!/usr/bin/make -f

TOPDIR := ../..
include $(TOPDIR)/Makefile.regression

#
# Makefile for isbound() test
#
# Files named *OK.ttcn are expected to pass syntax and semantic check
# Files named *SY.ttcn are expected to fail syntax check
# Files named *SE.ttcn are expected to pass syntax check,
# but fail semantic check
#
# Adding new files conforming to this naming convention
# automatically includes them in the test.
#


# for debugging:
#SHELL := /bin/sh -x




GOOD_TTCN := $(wildcard *OK.ttcn)
SYNT_TTCN := $(wildcard *SY.ttcn)
SEMA_TTCN := $(wildcard *SE.ttcn)

GOOD_CC := $(GOOD_TTCN:.ttcn=.cc)
#SEMA_CC := $(SEMA_TTCN:.ttcn=.cc)
#SYNT_CC := $(SYNT_TTCN:.ttcn=.cc)

GOOD_HH := $(GOOD_TTCN:.ttcn=.hh)
#SEMA_HH := $(SEMA_TTCN:.ttcn=.hh)
#SYNT_HH := $(SYNT_TTCN:.ttcn=.hh)

ifdef CODE_SPLIT
GOOD_CC := $(foreach file, $(GOOD_CC:.cc=), $(addprefix $(file), .cc _seq.cc _set.cc  _seqof.cc _setof.cc _union.cc))
else ifdef SPLIT_TO_SLICES
POSTFIXES := $(foreach file, $(SPLIT_TO_SLICES), $(addsuffix $(file), _part_))
POSTFIXES := $(foreach file, $(POSTFIXES), $(addprefix $(file), .cc))
GENERATED_SOURCES2 := $(foreach file, $(GOOD_CC:.cc=), $(addprefix $(file), $(POSTFIXES)))
GOOD_CC += $(GENERATED_SOURCES2)
endif


# The flags for the TTCN3 compiler
# Don't pass -i. Error lines with line *and* column
# are not recognised by Eclipse (which is what we want).
#F := -L

run: the good the bad and the ugly
#	for ttcn in $(GOOD_TTCN); do if $(TTCN3_COMPILER) $F $$ttcn ; then true; else exit 1; fi; done


define comp_ok
@for f in $^; do \
  echo $(TTCN3_COMPILER) $F -s $$f; 
  if   $(TTCN3_COMPILER) $F -s $$f; \
  then \
    true; \
  else \
    echo $$f:1: error: should have passed syntax and sema; exit 1; \
  fi; \
done
endef

define syntax_fail
@for f in $^; do \
  echo $(TTCN3_COMPILER) $F -p $$f; 
  if   $(TTCN3_COMPILER) $F -p $$f; \
  then \
    echo $$f:1: error: should have failed syntax; exit 1; \
  else \
    true; \
  fi; \
done
endef

define sema_fail
@for f in $^; do \
  echo $(TTCN3_COMPILER) $F -p $$f; 
  if   $(TTCN3_COMPILER) $F -p $$f; \
  then \
    true; \
  else \
    echo $$f:1: error: should have passed syntax; exit 1; \
  fi; \
  echo $(TTCN3_COMPILER) $F -s $$f; 
  if   $(TTCN3_COMPILER) $F -s $$f; \
  then \
    echo $$f:1: error: should have failed semantic check; exit 1; \
  else \
    true; \
  fi; \
done
endef


good: $(GOOD_TTCN)
	@echo +++++ The good +++++
	$(comp_ok)

bad: $(SYNT_TTCN)
	@echo ----- The bad -----
	@$(syntax_fail)

ugly: $(SEMA_TTCN)
	@echo \##### The ugly \#####
	@$(sema_fail)

####################

TARGET := goodprog$(EXESUFFIX)

GOOD_OBJ := $(GOOD_CC:.cc=.o)

# Execution mode: (either ttcn3 or ttcn3-parallel)
TTCN3_LIB = ttcn3$(RT2_SUFFIX)$(DYNAMIC_SUFFIX)

$(TARGET) : $(GOOD_CC)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ \
	-L$(TTCN3_DIR)/lib -l$(TTCN3_LIB) \
	-L$(OPENSSL_DIR)/lib -lcrypto $($(PLATFORM)_LIBS)

all: $(TARGET)

####################

%.cc : %.ttcn
	$(TTCN3_COMPILER) $F $<

clean distclean:
	rm -f $(GOOD_OBJ) $(TARGET)  *.stackdump *.log \
	$(GOOD_CC) $(SYNT_CC) $(SEMA_CC) \
	$(GOOD_HH) $(SYNT_HH) $(SEMA_HH)


.PHONY: good bad ugly all clean run the and

