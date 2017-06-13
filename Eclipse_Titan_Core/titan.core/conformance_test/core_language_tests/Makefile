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
#TOPDIR := ..
#include $(TOPDIR)/Makefile.regression

#ifdef LCOV
#COVERAGE_FLAG := -C
#endif

MAKE := make

#DIRS := negative_tests positive_tests
DIRS := positive_tests

WORKING_DIR := $(shell pwd)

# It can be built manually.
run clean:
	echo "hello"
	@for dir in $(DIRS); do make $@ -C $$dir || exit; done

.PHONY: all clean run 



