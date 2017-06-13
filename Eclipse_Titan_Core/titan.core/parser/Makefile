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
#   Szabo, Janos Zoltan – initial implementation
#
##############################################################################
# Makefile for the TTCN-3 parser

OBJECTS= parser.tab.o lex.yy.o
TARGETS= parser

.c.o:
	gcc -c -O2 -Wall $<

all:	$(TARGETS)

clean:
	-rm $(OBJECTS) $(TARGETS) lex.yy.c parser.tab.c parser.output parser.tab.h

parser.tab.c: parser.y
	bison -dv $<

lex.yy.c: parser.l parser.tab.c
	flex parser.l

parser: $(OBJECTS)
	gcc -o $@ $(OBJECTS)
