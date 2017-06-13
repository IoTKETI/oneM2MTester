##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#   Raduly, Csaba
#   Szabados, Kristof
#
##############################################################################
TOPDIR := ..
include $(TOPDIR)/Makefile.regression

CC := $(CXX)

#CXXFLAGS +=
CPPFLAGS += -I$(TTCN3_DIR)/include
LDFLAGS += -L$(TTCN3_DIR)/lib -L$(XMLDIR)/lib
LDLIBS  += -lttcn3 -lcrypto -lxml2
#LD := g++

EXES := hascontrolpart nocontrolpart twocontrolpart

all: $(EXES)

run: $(EXES)
	@echo -e "*\n* single control part, no config file: should pass\n*"
	./hascontrolpart
	@echo -e "*\n* single control part, config file without EXECUTE: should pass\n*"
	./hascontrolpart config_noexec.cfg
	@echo -e "*\n* single control part, config file with EXECUTE: should pass\n*"
	./hascontrolpart config1.cfg
	@echo -e "*\n* single control part, nonexistent config file: should fail\n*"
	if ./nocontrolpart no_such_thing.cfg; then echo "Should fail"; exit 1; else : ; fi
	@echo -e "*\n* single control part, partially corrupted config file: should fail\n*"
	if ./nocontrolpart ouch.cfg         ; then echo "Should fail"; exit 1; else : ; fi
	@echo -e "*\n* single control part, not a config file: should fail\n*"
	if ./nocontrolpart Makefile         ; then echo "Should fail"; exit 1; else : ; fi

	@echo -e "*\n* no control part, no config file: should fail\n*"
	if ./nocontrolpart; then echo "Should fail"; exit 1; else : ; fi
	@echo -e "*\n* no control part but config file without EXECUTE: should fail\n*"
	if ./nocontrolpart config_noexec.cfg; then echo "Should fail"; exit 1; else : ; fi
	@echo -e "*\n* no control part but config file: should pass\n*"
	./nocontrolpart config.cfg

	@echo -e "*\n* no control, no config => error\n*"
	if ./nocontrolpart  ; then echo No control part should fail; exit 1; else : ; fi
	@echo -e "*\n* no control, config file without EXECUTE => error\n*"
	if ./nocontrolpart config_noexec.cfg ; then echo No control part should fail; exit 1; else : ; fi

	@echo -e "*\n* more than one control part, no config => error\n*"
	if ./twocontrolpart ; then echo Multiple control parts should fail; exit 1; else : ; fi
	@echo -e "*\n* more than one control part, config file without EXECUTE => error\n*"
	if ./twocontrolpart config_noexec.cfg; then echo Multiple control parts should fail; exit 1; else : ; fi
	@echo -e "*\n* more than one control part, config file with EXECUTE => pass\n*"
	./twocontrolpart config2.cfg

	@echo "+++ Success +++"



hascontrolpart: hascontrolpart.o

nocontrolpart: nocontrolpart.cc

twocontrolpart: hascontrolpart.o alsohascontrolpart.cc
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@


%.cc: %.ttcn
	$(TTCN3_COMPILER) -Lg $<

clean distclean:
	rm -f *.o $(EXES) *.cc *.hh *.log
