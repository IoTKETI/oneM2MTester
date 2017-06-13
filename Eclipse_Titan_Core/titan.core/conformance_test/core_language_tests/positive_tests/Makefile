##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Kirjak, Adrien
#
# not ready
##############################################################################
#TOPDIR := ../..
#include $(TOPDIR)/Makefile.regression

#ifdef LCOV
#COVERAGE_FLAG := -C
#endif

MAKE := make

MAKECLEAN := make clean

run:
	$(TTCN3_DIR)/bin/ttcn3_makefilegen -R -f -t pos_conf_tests.tpd && \
	cd bin && \
	$(MAKE) && \
	$(TTCN3_DIR)/bin/ttcn3_start pos_conf_tests pos_conf_tests.cfg && \
	cd ..

clean:
	cd bin && \
	$(MAKECLEAN) && \
	cd .. 

.PHONY: all clean run 



