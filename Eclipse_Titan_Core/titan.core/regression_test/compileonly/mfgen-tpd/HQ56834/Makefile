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

#
# Makefilegen is supposed to fail
#
all:
	cd bin && \
	if $(TTCN3_DIR)/bin/ttcn3_makefilegen -t ../counter.tpd > output.txt 2>&1 ; \
	then echo makefilegen was supposed to fail!; exit 1; \
	else	if [ -h counter.ttcn ]; then echo symlink should not be created; exit 1; \
		else echo OK; fi  ; \
	fi

clean distclean:
	$(RM) bin/output.txt
