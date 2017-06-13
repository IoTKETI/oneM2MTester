##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Baji, Laszlo
#   Balasko, Jeno
#   Baranyi, Botond
#   Czimbalmos, Eduard
#   Delic, Adam
#   Kovacs, Ferenc
#   Kovacs, Zoltan
#   Ormandi, Matyas
#   Raduly, Csaba
#   Szabados, Kristof
#   Szabo, Janos Zoltan – initial implementation
#   Szalai, Endre
#   Torpis, Zsolt
#
##############################################################################
# Makefile for installing the nedit and xemacs additions

TOP := ..
include ../Makefile.cfg

NEDIT_FILES := $(addprefix nedit/, $(addsuffix .pats, \
ttcn3 ttcn3_config ttcn3_log asn1))

XEMACS_FILES := $(addprefix xemacs/, ttcn3.el ttcn3-indent.el ttcn3-mode-init.el ttcn3-check-init.el ttcn3-cfg.el \
asn1-mode.el asn1-mode-1.0-pkg.tar.gz clearcase-1.08-pkg.tar.gz README.xemacs)

SKELETON_FILES := $(addprefix skeleton/, $(addsuffix .skeleton, \
altstep asn1_module component_type constant control_part data_type function \
import_part module_param port_type signature template testcase ttcn3_module))

XSD_FILES := $(addprefix xsd/, TPD.xsd)

CASE_FOLDINGS := CaseFolding.txt

install:
ifdef MINGW
	@echo Skipped ${CURDIR} for MinGW
else
	mkdir -p $(ETCDIR)/xsd
	cp $(XSD_FILES) $(ETCDIR)/xsd
	mkdir -p $(ETCDIR)/asciiart
#	cp asciiart/*.txt $(ETCDIR)/asciiart
	mkdir -p $(ETCDIR)/scripts
	cp scripts/*.py $(ETCDIR)/scripts
	cp $(CASE_FOLDINGS) $(ETCDIR)
endif
