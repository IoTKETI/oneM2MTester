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

CODIRS := 1 2 3 4 5 6 7 8


#The configurations of the TPD files only differ in the name of the working
#directory and the target executable field and the files that are in the project.
#Each project has files, but each configuration excludes other files.
#The tests checks that the correct files are linked into the workind directory.

#This case of the improved linking method: -Z and -H option

all dep clean distclean:
	for dir in $(CODIRS); do $(MAKE) -C $$dir $@ || exit; done
