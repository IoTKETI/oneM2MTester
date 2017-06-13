##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Szabo, Bence Janos – initial implementation
#
##############################################################################
TOPDIR := ../../..
include $(TOPDIR)/Makefile.regression

CODIRS := Normal WithReqConfig WithMakefileName WithMakefileDir \
		WithMakefilePath WithScriptDiffDir RecursiveHierarchy \
		OneMakefileWithAllFiles ImprovedLinking


# All dirs have a test in them. The dir's makefile will check the
# generated makefile if the script inserted a line into it.

all dep clean distclean:
	for dir in $(CODIRS); do $(MAKE) -C $$dir $@ || exit; done
