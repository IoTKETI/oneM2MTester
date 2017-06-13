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

TARGET := HN21072Test$(EXESUFFIX)

XML_SCHEMAS :=  W3-WSDL-chop.xsd

GENERATED_TTCN3_MODULES := schemas_xmlsoap_org_wsdl.ttcn \
XSD.ttcn UsefulTtcn3Types.ttcn

USER_TTCN3_MODULES := HN21072Test.ttcn

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
ifneq ($(XMLLINT),) # xmllint found in path
	perl $(ABS_SRC)/log2xml.pl $(TARGET:.exe=).log > result.xml
	xmllint --noout --schema $(ABS_SRC)/W3-WSDL-chop.xsd result.xml
endif

