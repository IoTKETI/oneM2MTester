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
TOPDIR := ../..
include $(TOPDIR)/Makefile.regression

XDIRS := Examples HN21072 HN65402 HO16426 HO12250 HO18151 HO21968 HQ48576

ifneq (,$(findstring -Nold, $(TTCN3_COMPILER) $(COMPILER_FLAGS)))
XDIRS := $(filter-out New2 RLP_INIT RLP_RESULT, $(XDIRS))
endif


# List of fake targets:
.PHONY: all dep clean run convert compile $(XDIRS)

all dep clean run convert compile distclean:
	for dir in $(XDIRS); do $(MAKE) -C $$dir $@ || exit; done

# To run just one test
$(XDIRS):
	$(MAKE) -C $@ run

testclean:
	$(MAKE) -C .. $@

-include $(TOPDIR)/Makefile.personal

