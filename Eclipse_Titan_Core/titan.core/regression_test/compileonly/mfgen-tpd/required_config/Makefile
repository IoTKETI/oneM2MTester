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

CODIRS := 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 \
	  26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 \
	  48 49 50 51 52 53 54 55 56 57 58


#The configurations of the TPD files only differ in the name of the working
#directory and the target executable field. This two field is enough to
#determine that the correct configuration is used.
#If a makefilegen command finishes without error, then we check the 
#makefiles in the project's working directory if it has the correct EXECUTABLE
#value.
#If a makefilegen command finishes with error, then we check it's output for
#the error message and we match it with an expected error message.

#This is the case of -r switch being used (hierarchical build)

all dep clean distclean:
	for dir in $(CODIRS); do $(MAKE) -C $$dir $@ || exit; done
