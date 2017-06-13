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

TARGET = Examples$(EXESUFFIX)

XML_SCHEMAS =   Example1.xsd \
				Example2.xsd \
				Example3.xsd \
				Example4.xsd \
				chapter_5_1_1.xsd \
				chapter_5_1_2.xsd \
				chapter_5_1_3.xsd \
				chapter_5_1_4.xsd \
				chapter_5_1_5.xsd \
				chapter_5_1_6.xsd \
				chapter_5_2_1.xsd \
				chapter_5_2_2.xsd \
				chapter_5_2_3.xsd \
				chapter_6.xsd \
				chapter_7_1.xsd \
				chapter_7_3.xsd \
				chapter_7_4.xsd \
				chapter_7_5.xsd \
				chapter_7_6.xsd \
				chapter_7_7.xsd \
				chapter_7_8.xsd \
				chapter_7_9.xsd

GENERATED_TTCN3_MODULES = 	UsefulTtcn3Types.ttcn \
							XSD.ttcn \
							www_chapter_5_1_org.ttcn \
							www_chapter_5_2_org.ttcn \
							www_chapter_6_org.ttcn \
							www_chapter_7_1_org.ttcn \
							www_chapter_7_3_org.ttcn \
							www_chapter_7_4_org.ttcn \
							www_chapter_7_5_org.ttcn \
							www_chapter_7_6_org.ttcn \
							www_chapter_7_7_org.ttcn \
							www_chapter_7_8_org.ttcn \
							www_chapter_7_9_org.ttcn \
							NoTargetNamespace.ttcn \
							www_examples_org_ericsson.ttcn

include ../Makefile.converter

run: $(TARGET)
