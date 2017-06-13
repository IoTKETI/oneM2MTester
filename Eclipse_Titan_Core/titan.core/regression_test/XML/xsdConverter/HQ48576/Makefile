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
#
##############################################################################
TOPDIR := ../../..
include $(TOPDIR)/Makefile.regression

TARGET := HQ48576Test$(EXESUFFIX)

XML_SCHEMAS :=  hlrla_types.xsd

GENERATED_TTCN3_MODULES := schemas_ericsson_com_pg_hlr_13_5.ttcn \
XSD.ttcn UsefulTtcn3Types.ttcn

USER_TTCN3_MODULES := HQ48576Test.ttcn

include ../Makefile.converter

#
# Add your rules here if necessary...
#

XMLLINT := $(shell which xmllint 2>/dev/null)
ifeq ($(firstword $(XMLLINT)),no)
# stupid /bin/which on Solaris writes errors to stdout instead of stderr
XMLLINT :=
endif

run: $(TARGET)
	./$^

