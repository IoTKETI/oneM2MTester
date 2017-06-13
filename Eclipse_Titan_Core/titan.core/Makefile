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
#   Delic, Adam
#   Forstner, Matyas
#   Godar, Marton
#   Horvath, Gabriella
#   Koppany, Csaba
#   Kovacs, Ferenc
#   Kovacs, Zoltan
#   Kremer, Peter
#   Lovassy, Arpad
#   Ormandi, Matyas
#   Pilisi, Gergely
#   Raduly, Csaba
#   Szabados, Kristof
#   Szabo, Janos Zoltan – initial implementation
#   Torpis, Zsolt
#   Zalanyi, Balazs Andor
#
##############################################################################
# Main Makefile for the TTCN-3 Test Executor
# For use with GNU make only.

TOP := .
include $(TOP)/Makefile.cfg

# Dirlist for "all", "clean", "distclean"
ALLDIRS := common compiler2 repgen xsdconvert

ifndef MINGW
	ALLDIRS += mctr2 core core2 loggerplugins
endif

# JNI not supported on Cygwin or Mingw
ifneq ($(PLATFORM), WIN32)
	ifeq ($(JNI), yes)
		ALLDIRS += JNI
	endif
endif

#ALLDIRS += ctags

# Dirlist for "dep", "tags"
DEPDIRS := $(ALLDIRS)

ALLDIRS += usrguide

# Dirlist for "install"
INSTALLDIRS := $(ALLDIRS)

INSTALLDIRS += etc help hello

###########################################################

all run clean distclean:
	@for dir in $(ALLDIRS); do \
	  $(MAKE) -C $$dir $@ || exit; \
	done

dep tags:
	@for dir in $(DEPDIRS); do \
	  $(MAKE) -C $$dir $@ || exit; \
	done

install:
	if test -h $(TTCN3_DIR); then \
	  echo $(TTCN3_DIR) is a symlink, you cannot be serious; exit 1; fi
ifdef MINGW
	$(info cross your fingers...)
endif
	-rm -rf $(TTCN3_DIR)
ifeq ($(INCLUDE_EXTERNAL_LIBS), yes)
	mkdir -p $(TTCN3_DIR)/lib
	cp -d $(OPENSSL_DIR)/lib/libcrypto.so* $(TTCN3_DIR)/lib
	cp -d $(XMLDIR)/lib/libxml2.so* $(TTCN3_DIR)/lib
endif
	@for dir in $(INSTALLDIRS); do \
	  $(MAKE) -C $$dir $@ || exit; \
	done
ifeq ($(LICENSING), yes)
	mkdir -p $(ETCDIR)/licensegen
	cp licensegen/license.dat $(ETCDIR)/license
endif
	chmod -R +r,go-w $(TTCN3_DIR)
	@echo "***********************************"
	@echo "Installation successfully completed"
	@echo "***********************************"

# check is the standard GNU target for running tests
# (avoids clash with shell builtin 'test')
check:
	compiler2/compiler -v

ifneq (,$(findstring prereq,$(MAKECMDGOALS)))
# Export all variables into the environment for the prereq target
export
endif


# no need to include Makefile.genrules

.PHONY: all clean distclean dep tags install psi dox check confess prereq


Makefile.personal: SHELL:=/bin/bash
Makefile.personal:
	if [ -f $(TOP)/makefiles/$@.$${HOSTNAME:=$${COMPUTERNAME}} ]; then ln -s $(TOP)/makefiles/$@.$${HOSTNAME} $@; else touch $@; fi
