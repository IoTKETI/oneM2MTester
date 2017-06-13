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
#   Feher, Csaba
#   Forstner, Matyas
#   Kovacs, Ferenc
#   Kovacs, Zoltan
#   Pilisi, Gergely
#   Raduly, Csaba
#   Szabados, Kristof
#   Szabo, Janos Zoltan – initial implementation
#   Szalai, Endre
#
##############################################################################
# Makefile for installing the docs and html help pages

TOP := ..
include ../Makefile.cfg

CHMDIR := chm

MAIN_HTML_FILES := titan_main.html titan_index.html

MAIN_CHM_FILES := titan_index.html titan_online.html titan.hhp titan.hhc

INFO_FILES := $(addprefix info/, $(addsuffix .html, \
BNF action activate address alive all alt altstep and and4b any        \
anytype bit2hex bit2int bit2oct bit2str bitstring boolean break call   \
case catch char char2int char2oct charstring check clear complement    \
component connect const continue control create deactivate decode_base64 decvalue default     \
disconnect display do done else encode encode_base64 encvalue enumerated error except         \
exception execute extension external fail false float float2int        \
float2str for from function friend get_stringencoding getcall getreply getverdict goto group \
hex2bit hex2int hex2oct hex2str hexstring if ifpresent import in       \
inconc infinity inout int2bit int2char int2float int2hex int2oct       \
int2str int2unichar integer interleave isbound ischosen ispresent isvalue kill killed \
label language length lengthof log map match message mixed mod         \
modifies module modulepar mtc noblock none not not4b nowait null objid \
oct2bit oct2char oct2hex oct2int oct2str oct2unichar octetstring of omit on        \
operators optional or or4b out override param pass pattern permutation \
port public private procedure raise read receive record recursive regexp rem remove_bom repeat \
replace reply return rnd running runs select self send sender set setverdict   \
signature sizeof start stop str2bit str2float str2hex \
str2int str2oct subset substr superset system template testcase testcasename \
timeout timer to trigger true type unichar2char unichar2int unichar2oct union universal unmap   \
value valueof var variant verdicttype while with xor xor4b profiler))

IMAGE_FILES := $(addprefix images/, $(addsuffix .jpg, ao left right up)) images/titan_transparent.gif

DOC_FILES := $(addprefix docs/, $(addsuffix .pdf, \
naming))

USERGUIDES := $(addprefix ../../doc/, $(addsuffix .pdf, \
apiguide userguide installationguide referenceguide releasenotes))

all run: ;

install:
ifdef MINGW
	@echo Skipped ${CURDIR} for MinGW
else
	mkdir -p $(BINDIR)
	cp ttcn3_help $(BINDIR)
	chmod +x $(BINDIR)/ttcn3_help
	mkdir -p $(HELPDIR)
	cp $(MAIN_HTML_FILES) $(HELPDIR)
	mkdir -p $(HELPDIR)/info
	cp $(INFO_FILES) $(HELPDIR)/info
	mkdir -p $(HELPDIR)/images
	cp $(IMAGE_FILES) $(HELPDIR)/images
	mkdir -p $(HELPDIR)/docs
	cp $(DOC_FILES) $(HELPDIR)/docs
ifeq (GEN_PDF,yes)
	ln -s $(USERGUIDES) $(HELPDIR)/docs
endif
endif

chm.tgz:
	$(RM) -r $(CHMDIR)
	mkdir -p $(CHMDIR)
	cp $(MAIN_CHM_FILES) $(CHMDIR)
	mkdir $(CHMDIR)/info
	cp $(INFO_FILES) $(CHMDIR)/info
	mkdir -p $(CHMDIR)/images
	cp $(IMAGE_FILES) $(CHMDIR)/images
	./chm_refact.sh
	tar cfz $@ chm

clean:
	$(RM) -rf $(CHMDIR) chm.tgz
