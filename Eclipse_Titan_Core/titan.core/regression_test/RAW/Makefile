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
#   Hanko, Matyas
#   Kovacs, Ferenc
#   Raduly, Csaba
#   Szabo, Janos Zoltan – initial implementation
#
##############################################################################
TOPDIR := ..
include $(TOPDIR)/Makefile.regression

RDIRS = Examples HN25015 HQ26535 HQ49956 HS16977 ustr Annex_E_variants

all dep clean run distclean:
	for dir in $(RDIRS); do (cd $$dir && $(MAKE) $@) || exit; done

