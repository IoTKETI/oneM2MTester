##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   >
#   Baji, Laszlo
#   Balasko, Jeno
#   Bene, Tamas
#   Delic, Adam
#   Forstner, Matyas
#   Gecse, Roland
#   Raduly, Csaba
#   Szabo, Janos Zoltan – initial implementation
#
##############################################################################
# Main Makefile for the TTCN-3 Main Controller

TOP := ..
include $(TOP)/Makefile.cfg

SUBDIRS := 
ifeq ($(CLI), yes)
  SUBDIRS += editline cli
endif
SUBDIRS += mctr

all run:
	@for i in $(SUBDIRS) ; do \
	  (cd $$i && $(MAKE)) || exit; \
	done

install: all
	cd mctr && $(MAKE) install

include ../Makefile.genrules
