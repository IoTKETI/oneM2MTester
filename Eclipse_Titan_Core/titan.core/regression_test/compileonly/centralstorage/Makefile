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
#   Forstner, Matyas
#   Kovacs, Ferenc
#   Raduly, Csaba
#   Szabados, Kristof
#   Szabo, Janos Zoltan – initial implementation
#
##############################################################################
# Make sure TOPDIR is an absolute path
TOPDIR := $(shell cd ../.. && pwd)
include $(TOPDIR)/Makefile.regression

MAKEFLAGS += -j1

# This is a test for central storage and makefile generation
MFGEN := $(TTCN3_DIR)/bin/ttcn3_makefilegen
MFGEN_FLAGS := -sfp $(RT2_FLAG) $(SPLIT_FLAG)
ifdef LCOV
  MFGEN_FLAGS += -C
endif

CSDIRS := base_ttcn base_asn base_mixed extended_ttcn all_bases user_ttcn \
    user_all_bases separate_project separate_user

BASE_TTCN_FILES := base_ttcn.ttcnpp base_PT.cc
BASE_TTCN_FILES_RELPATH := $(addprefix ../base_ttcn/, $(BASE_TTCN_FILES))

BASE_ASN_FILES := Base_asn.asn
BASE_ASN_FILES_RELPATH := $(addprefix ../base_asn/, $(BASE_ASN_FILES))

BASE_MIXED_FILES := base_mixed_ttcn.ttcn base_mixed_ttcnpp.ttcnpp \
    base_mixed_PT.cc Base_mixed_asn.asn
BASE_MIXED_FILES_RELPATH := $(addprefix ../base_mixed/, $(BASE_MIXED_FILES))

EXTENDED_TTCN_FILES := extended_ttcnpp.ttcnpp extended_ttcn.ttcn
EXTENDED_TTCN_FILES_RELPATH := $(addprefix ../extended_ttcn/, \
    $(EXTENDED_TTCN_FILES))

SEPARATE_PROJECT_FILES := separate_ttcn.ttcn separate_ttcnpp.ttcnpp \
    separate_PT.cc separate_PT.hh
SEPARATE_PROJECT_FILES_RELPATH := $(addprefix ../separate_project/, \
    $(SEPARATE_PROJECT_FILES))

SEPARATED_FILES_RELPATH := $(addprefix ../separated_files/, \
    $(SEPARATE_PROJECT_FILES))

# Make executable alias.
# Actions containing $(MAKE) are exempt from -n, i.e. are always executed.
# Here they contain a mixture of makefilegen and make. The sub-make inherits
# the -n (do-nothing) flag but makefilegen is unaware of this.
# At extended_ttcn the second invocation of makefilegen fails to find files
# which should have been created by make.
#
# Workaround: use an alias, which is not exempted from -n
# Now all actions are merely printed.
MAKE_PROG := $(MAKE)

all: emptydirs
	#this tests if giving only the ".cc" of a port is enough
	cd base_ttcn && $(MFGEN) $(MFGEN_FLAGS) -c $(BASE_TTCN_FILES) \
		&& $(MAKE_PROG) $@
	#this tests compilation without central storage
	cd base_asn && $(MFGEN) $(MFGEN_FLAGS) $(BASE_ASN_FILES) && \
		$(MAKE_PROG) $@
	#this tests whether we can give the header file of a test port
	cd base_mixed && $(MFGEN) $(MFGEN_FLAGS) $(BASE_MIXED_FILES) \
	base_mixed_PT.hh && $(MAKE_PROG) $@
	#this tests giving in generated files
	cd extended_ttcn && $(MFGEN) $(MFGEN_FLAGS) -c \
		$(EXTENDED_TTCN_FILES) $(BASE_TTCN_FILES_RELPATH) && \
		$(MAKE_PROG) compile && $(MFGEN) $(MFGEN_FLAGS) -c \
		$(EXTENDED_TTCN_FILES) extended_ttcn.cc extended_ttcn.hh \
		$(BASE_TTCN_FILES_RELPATH) && $(MAKE_PROG) $@
	#this tests if we can reach several projects as central storages
	#and also this project has no files on its own
	cd all_bases && $(MFGEN) $(MFGEN_FLAGS) -c \
		$(BASE_TTCN_FILES_RELPATH) $(BASE_ASN_FILES_RELPATH) \
		$(BASE_MIXED_FILES_RELPATH) && $(MAKE_PROG) $@
	#this tests if we can use a project as central storage which also uses
	#a project as central storage
	cd user_ttcn && $(MFGEN) $(MFGEN_FLAGS) -c -O fake.ttcnpp \
		user_ttcn.ttcn $(EXTENDED_TTCN_FILES_RELPATH)\
		$(BASE_TTCN_FILES_RELPATH) && $(MAKE_PROG) $@
	#this tests if we can use a project which is used by all the project we
	#use as central storage.
	cd user_all_bases && $(MFGEN) $(MFGEN_FLAGS) -c \
		user_all_bases.ttcn $(BASE_TTCN_FILES_RELPATH) \
		$(BASE_ASN_FILES_RELPATH) $(BASE_MIXED_FILES_RELPATH) \
		$(EXTENDED_TTCN_FILES_RELPATH) && $(MAKE_PROG) $@
	#this tests if we can give the same file more than once
	cd separate_project && $(MFGEN) $(MFGEN_FLAGS) -c \
		$(SEPARATE_PROJECT_FILES) $(SEPARATE_PROJECT_FILES) \
		$(SEPARATE_PROJECT_FILES_RELPATH) \
		$(SEPARATE_PROJECT_FILES_RELPATH) && $(MAKE_PROG) $@
	#in some cases the project to be used as central storage must be moved
	#without recompiling
	(cd separate_project && tar -cf - *) | (cd separated_files && tar -xf -)
	#this tests if we can use a moved and recompiled project as
	#central storage and generates a gnu Makefile with absolute paths
	#and puts a config file into the makefile which is not compilable
	cd separate_user && $(MFGEN) $(MFGEN_FLAGS) -cga \
		separate_user.ttcn config.cfg $(SEPARATED_FILES_RELPATH) && \
		$(MAKE_PROG) $@

dep clean distclean: emptydirs
	#clean up all the projects and erase their Makefile one by one.
	for dir in $(CSDIRS); do (cd $$dir && if [ -f Makefile ]; then \
		$(MAKE) $(subst dist,,$@); $(RM) Makefile; fi); done
	cd separated_files && $(RM) *.*

emptydirs:
	mkdir -p all_bases separated_files

.NOTPARALLEL:
