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
#   Bartha, Norbert
#   Delic, Adam
#   Kovacs, Ferenc
#   Ormandi, Matyas
#   Raduly, Csaba
#
##############################################################################
# Makefile to run the non-interactive tests
DIRS := BER_EncDec RAW_EncDec Text_EncDec XER_EncDec Semantic_Analyser Semantic_Analyser_Csaba Config_Parser

SCRIPTFLAGS :=

# Coverage settings
LCOVDIR := /mnt/TTCN/Tools/lcov-1.7/usr
LCOV := no
ifeq ($(LCOV), yes)
SCRIPTFLAGS += -coverage
endif
# Runtime selection
#RT2 := yes
ifeq ($(RT2), yes)
SCRIPTFLAGS += -rt2
endif

# Your platform. Allowed values: SOLARIS, SOLARIS8, LINUX, FREEBSD, WIN32
ifndef PLATFORM
PLATFORM1 := $(shell uname -s)
PLATFORM2 := $(shell uname -r)
PLATFORM3 := $(shell uname -m)

ifeq ($(PLATFORM1), SunOS)
  ifeq ($(PLATFORM2), 5.6)
    PLATFORM := SOLARIS
  else
    PLATFORM := SOLARIS8
  endif # 5.6
endif # SunOS
ifeq ($(PLATFORM1), Linux)
  PLATFORM := LINUX
endif # Linux
ifeq ($(PLATFORM1), FreeBSD)
  PLATFORM := FREEBSD
endif # FreeBSD
ifeq ($(findstring CYGWIN, $(PLATFORM1)), CYGWIN)
  PLATFORM := WIN32
endif # CYGWIN
ifeq ($(PLATFORM1), Interix)
  PLATFORM := INTERIX
endif # Interix
endif # ifndef PLATFORM
ifndef PLATFORM
	PLEASE CHECK THE ABOVE PLATFORM SETTINGS!!!
endif # ifndef PLATFORM

export PLATFORM
export RT2

all check run: $(DIRS)

.PHONY: $(DIRS)

BER_EncDec:
	cd $@; ./run_test $(SCRIPTFLAGS)

RAW_EncDec:
	cd $@; ./run_test $(SCRIPTFLAGS)

Text_EncDec:
	cd $@; if [ ! -f perl ] ; then ln -s `which perl` perl; fi; ./run_test $(SCRIPTFLAGS)

XER_EncDec:
	cd $@; ./run_test $(SCRIPTFLAGS)

Semantic_Analyser:
	cd $@; if [ ! -f perl ] ; then ln -s `which perl` perl; fi; ./run_test_all $(SCRIPTFLAGS)

Semantic_Analyser_Csaba:
	make run -f Makefile.semantic -C Semantic_Analyser

Config_Parser:
	cd $@; if [ ! -f perl ] ; then ln -s `which perl` perl; fi; ./run_test $(SCRIPTFLAGS)

profile: $(DIRS)
	make all
	$(LCOVDIR)/bin/lcov -d .. -c -o functest_coverage.info
	$(LCOVDIR)/bin/genhtml -o functest_coverage -t "TITAN Function Test Coverage" functest_coverage.info

# It's a 50-50
clean:
	for d in $(DIRS); do if [ -f $$d/Makefile ]; then $(MAKE) -C $$d clean; else : ; fi done

$(CURDIR)/Makefile.personal:
	ln -s $(CURDIR)/../$(@F) $(CURDIR)/
include $(CURDIR)/Makefile.personal
