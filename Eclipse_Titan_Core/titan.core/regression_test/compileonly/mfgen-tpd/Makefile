##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#   Beres, Szabolcs
#   Delic, Adam
#   Ormandi, Matyas
#   Raduly, Csaba
#   Szabados, Kristof
#   Szabo, Bence Janos
#   Szalay, Akos
#
##############################################################################
TOPDIR := ../..
include $(TOPDIR)/Makefile.regression

ifdef LCOV
COVERAGE_FLAG := -C
endif

# ${MAKEPROG} has the same content as the built-in ${MAKE},
# except the special handling of ${MAKE} does not apply.
# If we used ${MAKE} in the rules below, 'make -n' would succeed once,
# then fail on every subsequent invocation until a 'make clean' is done. 
MAKEPROG := ${MAKE}

DIRLIST := buildconfig_param invalid_buildconfig_param invalid_buildconfig_tpd \
	HP79745 HQ56829 HQ56834 HQ56848 library HR30356 flagTest HR30365 dependency_check \
	required_config required_config_v2  disablePredefinedExternalFolder \
	makefileScript diff_tpd_name_error

ifdef DYN
DIRS += required_config_v3 splitting compile_once
endif

# The default target.
ifeq ($(findstring c,$(MFGEN_FLAGS)),) # no -c
#  Build in "reverse" order to reduce false positives
all: BuildConsumer BuildSupplier BuildSubplier HP35732 HQ56829 BuildDirs HQ60308 SearchPaths
else
# Build from supplier to consumer because central dirs must be ready
# when the toplevel is built
all: BuildSubplier BuildSupplier BuildConsumer COMMON NAS HP35732 HQ56829 BuildDirs SearchPaths

.NOTPARALLEL:
endif

WORKING_DIR := $(shell pwd)

# The run target is not normally used in compileonly.
# It can be built manually.
run:
	perl runner.pl

BuildDirs:
	@for dir in $(DIRLIST); do $(MAKE) -C $$dir || exit; done

BuildConsumer: /tmp/issue
	@echo -e "#\n#	Build Consumer\n#"
	mkdir -p consumer/bin && cd consumer/bin \
	&& if [ ! -h handmade.ttcn ]; then ln -s ../../handmade.ttcn ./ ; else : ; fi\
	&& $(TTCN3_DIR)/bin/ttcn3_makefilegen -fD $(MFGEN_FLAGS) $(COVERAGE_FLAG) -t ../consumer-ref-supplier-Default.tpd handmade.ttcn \
	&& $(MAKEPROG)

BuildSupplier:
	@echo -e "#\n#	Build Supplier\n#"
	mkdir -p supplier/bin && cd supplier/bin \
	&& $(TTCN3_DIR)/bin/ttcn3_makefilegen -fD $(MFGEN_FLAGS) $(COVERAGE_FLAG) -t ../supplier24.tpd \
	&& $(MAKEPROG)

BuildSubplier: /tmp/issue
	@echo -e "#\n#	Build Subplier (leaf project)\n#"
	mkdir -p subplier/bin && cd subplier/bin \
	&& $(TTCN3_DIR)/bin/ttcn3_makefilegen -fD $(MFGEN_FLAGS) $(COVERAGE_FLAG) -t ../subplier.tpd \
	&& $(MAKEPROG)

HP35732:
	mkdir -p $@ && cd $@ \
	&& $(TTCN3_DIR)/bin/ttcn3_makefilegen -fD $(MFGEN_FLAGS) $(COVERAGE_FLAG) -t ../NAS_EPS_v9.7.0_CNL113729/test/NAS_EPS_v9.7.0_CNL113729_test.tpd \
	&& $(MAKEPROG)

HQ56829 HQ56834 HQ56848:
	$(MAKEPROG) -C $@

HQ60308:
	cd $@ \
	&& $(TTCN3_DIR)/bin/ttcn3_makefilegen -fD -t counter.tpd \
	&& $(MAKEPROG)

COMMON:
	mkdir -p COMMON/bin && cd COMMON/bin \
	&& $(TTCN3_DIR)/bin/ttcn3_makefilegen -fD $(MFGEN_FLAGS) $(COVERAGE_FLAG) -t ../ProtocolModules_Common.tpd \
	&& $(MAKEPROG)

NAS:
	mkdir -p NAS_EPS_v9.7.0_CNL113729/bin && cd NAS_EPS_v9.7.0_CNL113729/bin \
	&& $(TTCN3_DIR)/bin/ttcn3_makefilegen -fD $(MFGEN_FLAGS) $(COVERAGE_FLAG) -t ../NAS_EPS_v9.7.0_CNL113729.tpd \
	&& $(MAKEPROG)

SearchPaths:
	cd search_paths/main && \
	$(TTCN3_DIR)/bin/ttcn3_makefilegen -fg -t Main.tpd -I/something -I$(WORKING_DIR)/search_paths/dep3/Test3/ -I$(WORKING_DIR)/search_paths/dep2/Test2 -I$(WORKING_DIR)/search_paths/dep2/dep2_1/Test2_1 && \
	cd bin && $(MAKEPROG)

/tmp/issue:
	-touch $@

clean:
	@for dir in $(DIRLIST); do $(MAKE) -C $$dir $@ || exit; done
	if [ -f HQ60308/Makefile ]; then $(MAKEPROG) -C HQ60308 $@ || exit ; else : ; fi
	-rm -rf consumer/bin supplier/bin subplier/bin HP35732 COMMON/bin NAS_EPS_v9.7.0_CNL113729/bin HQ60308/Makefile search_paths/main/bin

distclean: clean
	-rm -f *.out || exit; done
	@for dir in $(DIRLIST); do $(MAKE) -C $$dir $@ || exit; done

.PHONY: all clean distclean run BuildDirs BuildConsumer BuildSupplier BuildSubplier HP35732 HQ56829 HQ56834 HQ56848 COMMON NAS HQ60308 SearchPaths 

