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
#   Szabados, Kristof
#
# TODO: this should be in the function_test folder together with the other encoders/decoders
##############################################################################
TOPDIR := ../
include   $(TOPDIR)/Makefile.regression

MAKE_PROG := $(MAKE)

TTCN_FILES := array_test.ttcn bitstring_test.ttcn boolean_test.ttcn charstring_test.ttcn enum_test.ttcn float_test.ttcn functions.ttcn hexstring_test.ttcn integer_test.ttcn octetstring_test.ttcn record_test.ttcn recordof_test.ttcn set_test.ttcn setof_test.ttcn types.ttcn ucharstring_test.ttcn union_test.ttcn objid_test.ttcn
TTCN_PARALLEL_FILES := component_test.ttcn

SINGLE_CFG := text2ttcn_1.cfg
PARALLEL_CFG := text2ttcn_2.cfg

SINGLE_FILES := $(TTCN_FILES) $(SINGLE_CFG) 
PARALLEL_FILES := $(TTCN_FILES) $(TTCN_PARALLEL_FILES) $(PARALLEL_CFG)

RUNNABLE := text2ttcn

ifeq ($(PLATFORM), WIN32)
RUNNABLE := $(RUNNABLE).exe
endif

COVERAGE_FLAG := 
ifeq ($(COVERAGE), yes)
  COVERAGE_FLAG += -C
endif

DIR_SINGLE := dir_single_mode
DIR_PARALLEL := dir_parallel_mode
GENERATED_DIRS := $(DIR_SINGLE) $(DIR_PARALLEL)

# List of fake targets:
.PHONY: all clean run run_single run_parallel runall

all: $(GENERATED_DIRS)

$(DIR_SINGLE):
	mkdir $@
	cd $@ && for file in $(SINGLE_FILES); do ln -s ../$$file || exit; done
	cd $@ && $(TTCN3_DIR)/bin/ttcn3_makefilegen $(COVERAGE_FLAG) $(SPLIT_FLAG) -sM -e $(RUNNABLE) ./*  && $(MAKE_PROG)

$(DIR_PARALLEL):
	mkdir $@
	cd $@ && for file in $(PARALLEL_FILES); do ln -s ../$$file || exit; done
	cd $@ && $(TTCN3_DIR)/bin/ttcn3_makefilegen $(COVERAGE_FLAG) $(SPLIT_FLAG) -M -e $(RUNNABLE) ./*  && $(MAKE_PROG)

run: $(GENERATED_DIRS)
	cd $(DIR_SINGLE) && ./$(RUNNABLE) $(SINGLE_CFG) && grep "Overall verdict: pass" *.log
	cd $(DIR_PARALLEL) && $(TTCN3_DIR)/bin/ttcn3_start $(RUNNABLE) $(PARALLEL_CFG) && grep "Overall verdict: pass" *.log

# To run all tests, possibly in parallel
run_single: $(DIR_SINGLE)
	cd $(DIR_SINGLE) && ./$(RUNNABLE) $(SINGLE_CFG) && grep "Overall verdict: pass" *.log

run_parallel: $(DIR_PARALLEL) 
	cd $(DIR_PARALLEL) && $(TTCN3_DIR)/bin/ttcn3_start $(RUNNABLE) $(PARALLEL_CFG) && grep "Overall verdict: pass" *.log

runall: run_single run_parallel

clean distclean:
	rm -rf $(GENERATED_DIRS)

