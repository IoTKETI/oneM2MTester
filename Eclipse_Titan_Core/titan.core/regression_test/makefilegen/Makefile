##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#   Szabo, Bence Janos
#
##############################################################################
TOPDIR := ..
include $(TOPDIR)/Makefile.regression

.PHONY: all clean dep run

all: $(TARGET)

envvar:
	./makefilegen_envvar_test.sh

split:
	./makefilegen_split.sh

run: envvar split

clean distclean:
	rm -f TempMakefile

dep:
