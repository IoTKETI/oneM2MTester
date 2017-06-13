##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#   Delic, Adam
#   Raduly, Csaba
#   Susanszky, Eszter
#
##############################################################################
TOP    := ..
include   ../Makefile.cfg

DIRS := \
JUnitLogger TSTLogger JUnitLogger2

ifeq ($(LTTNGUSTLOGGER),yes)
	DIRS += LTTngUSTLogger
endif

all run dep clean distclean install:
	for d in $(DIRS); do $(MAKE) -C $$d $@ || exit 1; done
