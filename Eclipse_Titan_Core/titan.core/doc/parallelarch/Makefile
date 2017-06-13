##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#   Forstner, Matyas
#   Szabados, Kristof
#   Szabo, Janos Zoltan – initial implementation
#
##############################################################################
TOP := ../../
include ../../Makefile.cfg

DOCUMENTS := parallel

TARGETS := $(addsuffix .ps, $(DOCUMENTS)) $(addsuffix .pdf, $(DOCUMENTS))

GENERATED_EPS := parallelarch.eps state_mach_mc.eps state_mach_hc.eps \
  state_mach_hc_mc.eps state_mach_mtc.eps state_mach_ptc_mc.eps \
  state_mach_conn_endpoint.eps state_mach_conn_mc.eps state_mach_mapping_mc.eps

TOBECLEANED := $(addsuffix .aux, $(DOCUMENTS)) $(addsuffix .dvi, $(DOCUMENTS)) $(addsuffix .log, $(DOCUMENTS)) $(addsuffix .toc, $(DOCUMENTS))

all: $(TARGETS)

clean:
	$(RM) $(TARGETS) $(TOBECLEANED)

distclean: clean
	$(RM) $(GENERATED_EPS)

parallel.ps: $(GENERATED_EPS)

%.eps: %.dot
	dot -Tps -o $@ $<

%.eps: %.fig
	fig2dev -L eps $< $@

%.dvi: %.tex
	$(TEXPATH)/latex $<
	$(TEXPATH)/latex $<
	$(TEXPATH)/latex $<

%.ps: %.dvi
	$(TEXPATH)/dvips -o $@ $<

%.pdf: %.ps
	distill -compatlevel 3.0 -pagesize 21 29.7 cm -resolution 600 \
	-colordownsample off -graydownsample off -monodownsample off \
	-colorcompr zip -graycompr zip -monocompr zip -embedallfonts on \
	-compresstext on \
	<$< >$@
