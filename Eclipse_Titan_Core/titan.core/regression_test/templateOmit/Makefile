##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#   Baranyi, Botond
#
##############################################################################
TOPDIR := ..
include   ../Makefile.regression
unexport ABS_SRC
unexport SRCDIR

# List of fake targets:
.PHONY: all dep clean run Common NewTests LegacyTests /Common /NewTests /LegacyTests profile

all dep clean distclean run:
	$(MAKE) -C NewTests $@
	$(MAKE) -C LegacyTests $@

new:
	$(MAKE) -C NewTests run
	
legacy:
	$(MAKE) -C LegacyTests run
