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

XML_SCHEMAS := $(ABS_SRC)/HO12250.xsd

all run:
	if $(TTCN3_DIR)/bin/xsd2ttcn $(XML_SCHEMAS) 2>stderr; then \
	echo Failed to report error; exit 1; \
else \
	if ! grep 'maxOccurs must be 1' stderr >/dev/null ; then echo Expected message not found; exit 2; fi; \
fi
#	@echo OK
	
clean distclean:
	rm -f stderr

.PHONY: all run clean distclean
