##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   
#   Balasko, Jeno
#   Baranyi, Botond
#   Beres, Szabolcs
#   Czerman, Oliver
#   Delic, Adam
#   Dimitrov, Peter
#   Forstner, Matyas
#   Godar, Marton
#   Hanko, Matyas
#   Koppany, Csaba
#   Kovacs, Ferenc
#   Lovassy, Arpad
#   Ormandi, Matyas
#   Raduly, Csaba
#   Szabados, Kristof
#   Szabo, Bence Janos
#   Szabo, Janos Zoltan – initial implementation
#   Szalai, Gabor
#   Tatarka, Gabor
#   Zalanyi, Balazs Andor
#
##############################################################################
# Settings for the regression test

TOPDIR := $(CURDIR)
include $(TOPDIR)/Makefile.regression

DIRS := intOper boolOper floatOper bitstrOper octetstrOper charOper charstrOper \
verdictOper recordOper recofOper setOper setofOper setofMatch unionOper \
enumOper arrayOper anytypeOper templateInt templateBool templateFloat templateBitstr \
templateHexstr templateOctetstr templateChar templateCharstr templateUnicharstr \
templateRec templateRecof templateSet templateSetof templateUnion templateEnum modifiedTemplate \
basicStatem configOper commMessage commProcedure lostTimer controlTimer \
BER BER_x682 ASN1 predefFunction ERC preCompilerFlags functionReference \
nonMandatoryPar logFiles logger_control namedActualParameters \
assignmentNotation omitdef anytype RAW implicitMsgEncoding pattern_quadruples \
macros visibility hexstrOper ucharstrOper objidOper CRTR00015758 slider \
XML ipv6 implicitOmit testcase_defparam transparent HQ16404 cfgFile \
all_from lazyEval tryCatch text2ttcn json ttcn2json profiler templateOmit \
customEncoding makefilegen uidChars checkstate hostid templateIstemplatekind \
selectUnion templateExclusiveRange any_from templatePatternRef indexWithRecofArray \
connectMapOperTest fuzzy portTranslation

ifdef DYN
DIRS += loggerplugin junitlogger 
endif

ifdef RT2
DIRS += typeCompat negativeTest templateConcat
endif

ifeq (LTTNGUSTLOGGER, yes)
DIRS += LTTngUSTLogger
endif

# List of fake targets:
.PHONY: all dep clean run $(DIRS) $(addsuffix /, $(DIRS)) coverage compileonly

# Export all variables to the sub-directories.
#export

all dep clean distclean:
	$(MAKE) -C compileonly $@
	for dir in $(DIRS); do $(MAKE) -C $$dir $@ || exit; done
	$(RM) report.txt
	$(RM) build.txt

run:
	for dir in $(DIRS); do $(MAKE) -C $$dir $@ || exit; done


# It is assumed, that the TITAN sources are available in `..'.  Please note,
# that if TITAN was compiled with coverage enabled, only this target will
# work properly.
coverage:
	$(LCOV_DIR)/bin/lcov -d .. -z
	make report LCOV=1
	$(LCOV_DIR)/bin/lcov -d .. -c -o regtest_coverage.info
	$(LCOV_DIR)/bin/genhtml -o regtest_coverage -t "TITAN Regression Test Coverage" regtest_coverage.info

compileonly $(DIRS) $(addsuffix /, $(DIRS)):
ifeq ($(DEBUG), yes)
	bash -c 'set -o pipefail; $(MAKE) -C $@ 2>&1 | tee current_build.txt'
	@if [ -f build.txt ]; \
	then cat build.txt current_build.txt > build_result.txt; rm current_build.txt; mv build_result.txt build.txt; \
	else mv current_build.txt build.txt; fi
else
	$(MAKE) -C $@
endif

# Build in parallel
parallel build-par: compileonly $(DIRS)
ifeq ($(DEBUG), yes)
	@echo "--------------------------------------"
	@echo "List of memory leaks during the build:"
	@echo "--------------------------------------"
	@perl vcheck.pl build.txt
endif

# To generate a report:
# First, build everything (incl. compileonly); possibly in parallel.
# Then, run the runnable tests in sequence (avoids garbled output).
report.txt: parallel
	bash -c 'set -o pipefail; $(MAKE) run 2>&1 | tee report.txt'

report: report.txt
	@echo "----------------------------------"
	@echo "List of tests not 100% successful:"
	@echo "----------------------------------"
	@perl vcheck.pl report.txt

testclean:
	$(MAKE) -C .. $@

rclean:
	$(MAKE) clean 'DIRS=$(filter-out $(TESTDIRS), $(DIRS))'

prereq:
	./prereq.pl ${MAKELEVEL}

$(TOPDIR)/Makefile.personal:
	ln -s $(TOPDIR)/../$(@F) $(TOPDIR)/

unexport DIRS

include Makefile.regression
export PLATFORM
