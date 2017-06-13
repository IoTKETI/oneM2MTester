##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Baranyi, Botond
#
##############################################################################
TOPDIR := ../
include $(TOPDIR)/Makefile.regression

MAKE_PROG := $(MAKE)

TTCN_FILES := FuzzyTypes.ttcn FuzzyFunctions.ttcn FuzzyTestcases.ttcn
CFG := fuzzy.cfg

FILES := $(TTCN_FILES) $(CFG) 

RUNNABLE := fuzzy$(EXE_SUFFIX)

COVERAGE_FLAG := 
ifeq ($(COVERAGE), yes)
  COVERAGE_FLAG += -C
endif

ifdef DYN
ifeq ($(PLATFORM), WIN32)
  export PATH+=:$(TTCN3_DIR)/lib:$(ABS_SRC)/$(DIR_SINGLE):$(ABS_SRC)/$(DIR_PARALLEL):
else
  export LD_LIBRARY_PATH+=:$(ABS_SRC)/$(DIR_SINGLE):$(ABS_SRC)/$(DIR_PARALLEL):
endif
DYNFLAG := -l
endif

DIR_SINGLE := dir_single_mode
DIR_PARALLEL := dir_parallel_mode
GENERATED_DIRS := $(DIR_SINGLE) $(DIR_PARALLEL)

# List of fake targets:
.PHONY: all clean run run_single run_parallel runall

all: $(GENERATED_DIRS)

$(DIR_SINGLE):
	mkdir $@
	cd $@ && for file in $(FILES); do ln -s ../$$file || exit; done
	cd $@ && $(TTCN3_DIR)/bin/ttcn3_makefilegen $(COVERAGE_FLAG) $(SPLIT_FLAG) $(RT2_FLAG) $(DYNFLAG) -s -e $(RUNNABLE) ./*  && $(MAKE_PROG)

$(DIR_PARALLEL):
	mkdir $@
	cd $@ && for file in $(FILES); do ln -s ../$$file || exit; done
	cd $@ && $(TTCN3_DIR)/bin/ttcn3_makefilegen $(COVERAGE_FLAG) $(SPLIT_FLAG) $(RT2_FLAG) $(DYNFLAG) -e $(RUNNABLE) ./*  && $(MAKE_PROG)

run: $(GENERATED_DIRS)
	cd $(DIR_SINGLE) && ./$(RUNNABLE) $(CFG) && grep "Overall verdict: pass" *.log
	cd $(DIR_PARALLEL) && $(TTCN3_DIR)/bin/ttcn3_start $(RUNNABLE) $(CFG) && grep "Overall verdict: pass" *.log

# To run all tests, possibly in parallel
run_single: $(DIR_SINGLE)
	cd $(DIR_SINGLE) && ./$(RUNNABLE) $(CFG) && grep "Overall verdict: pass" *.log

run_parallel: $(DIR_PARALLEL) 
	cd $(DIR_PARALLEL) && $(TTCN3_DIR)/bin/ttcn3_start $(RUNNABLE) $(CFG) && grep "Overall verdict: pass" *.log

runall: run_single run_parallel

clean distclean:
	rm -rf $(GENERATED_DIRS)

