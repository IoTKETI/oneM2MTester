##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Baji, Laszlo
#   Balasko, Jeno
#   Beres, Szabolcs
#   Raduly, Csaba
#
##############################################################################
TOPDIR := ../../../
include $(TOPDIR)/Makefile.regression

ifdef LCOV
COVERAGE_FLAG := -C
endif

# ${MAKEPROG} has the same content as the built-in ${MAKE},
# except the special handling of ${MAKE} does not apply.
# If we used ${MAKE} in the rules below, 'make -n' would succeed once,
# then fail on every subsequent invocation until a 'make clean' is done. 
MAKEPROG := ${MAKE}
MFGEN := $(TTCN3_DIR)/bin/ttcn3_makefilegen

ifeq ($(PLATFORM), WIN32)
all: static
else
all: static dynamic
endif

static: CheckLibraryL CheckLibrary2L CheckLibrary3L CheckLibrary4L \
	CheckLibrary CheckLibrary2 CheckLibrary3 CheckLibrary4 CheckHelloTpd CheckHelloTpd2

dynamic: Check_dynamic_central Check_dynamic_central_exe \
	Check_dynamic Check_dynamic2 Check_dynamic3 Check_dynamic4


# ttcn3_makefilegen -L; make
BuildLibraryL: a.ttcn 
	rm -rf ./bin
	mkdir -p bin && cd bin && ln -fs ../a.ttcn \
	&& $(MFGEN) -f $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	 -L a.ttcn && ${MAKEPROG}

CheckLibraryL: BuildLibraryL
	if [ ! -f ./bin/liba.a ]; then exit 1; fi
	if [ -f ./bin/a ]; then exit 1; fi

# ttcn3_makefilegen -L; make executable
BuildLibrary3L: a.ttcn
	rm -rf ./bin3
	mkdir -p bin3 && cd bin3 && ln -fs ../a.ttcn \
	&& $(MFGEN) -f $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	 -L a.ttcn && ${MAKEPROG} executable

CheckLibrary3L: BuildLibrary3L
	if [ -f ./bin3/liba.a ]; then exit 1; fi
	if [ ! -f ./bin3/a ]; then exit 1; fi

# ttcn3_makefilegen -L; make library
BuildLibrary2L: a.ttcn
	rm -rf ./bin2
	mkdir -p bin2 && cd bin2 && ln -fs ../a.ttcn \
	&& $(MFGEN) -f $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	 -L a.ttcn && ${MAKEPROG} library

CheckLibrary2L: BuildLibrary2L
	if [ ! -f ./bin2/liba.a ]; then exit 1; fi
	if [ -f ./bin2/a ]; then exit 1; fi

# ttcn3_makefilegen -L; make run
BuildLibrary4L: a.ttcn
	rm -rf ./bin4
	mkdir -p bin4 && cd bin4 && ln -fs ../a.ttcn \
	&& $(MFGEN) -f $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	 -L a.ttcn && ${MAKEPROG} all

CheckLibrary4L: BuildLibrary4L
	if [ ! -f ./bin4/liba.a ]; then exit 1; fi
	if [ -f ./bin4/a ]; then exit 1; fi

#### Without -L ####

# ttcn3_makefilegen; make
BuildLibrary: a.ttcn 
	rm -rf ./bin
	mkdir -p bin && cd bin && ln -fs ../a.ttcn \
	&& $(MFGEN) -f $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	 a.ttcn && ${MAKEPROG}

CheckLibrary: BuildLibrary
	if [ -f ./bin/liba.a ]; then exit 1; fi
	if [ ! -f ./bin/a ]; then exit 1; fi

# ttcn3_makefilegen; make executable
BuildLibrary3: a.ttcn
	rm -rf ./bin3
	mkdir -p bin3 && cd bin3 && ln -fs ../a.ttcn \
	&& $(MFGEN) -f $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	 a.ttcn && ${MAKEPROG} executable

CheckLibrary3: BuildLibrary3
	if [ -f ./bin3/liba.a ]; then exit 1; fi
	if [ ! -f ./bin3/a ]; then exit 1; fi

# ttcn3_makefilegen; make library
BuildLibrary2: a.ttcn
	rm -rf ./bin2
	mkdir -p bin2 && cd bin2 && ln -fs ../a.ttcn \
	&& $(MFGEN) -f $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	 a.ttcn && ${MAKEPROG} library

CheckLibrary2: BuildLibrary2
	if [ ! -f ./bin2/liba.a ]; then exit 1; fi
	if [ -f ./bin2/a ]; then exit 1; fi

# ttcn3_makefilegen; make all
BuildLibrary4: a.ttcn
	rm -rf ./bin4
	mkdir -p bin4 && cd bin4 && ln -fs ../a.ttcn \
	&& $(MFGEN) -f $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	 a.ttcn && ${MAKEPROG} all

CheckLibrary4: BuildLibrary4
	if [ -f ./bin4/liba.a ]; then exit 1; fi
	if [ ! -f ./bin4/a ]; then exit 1; fi

###############################
#  Dynamic linking
##############################

####### with -L ##############

# ttcn3_makefilegen -L -l; make all
Build_dynamic: a.ttcn
	rm -rf ./bin_dynamic
	mkdir -p bin_dynamic && cd bin_dynamic && ln -fs ../a.ttcn \
	&& $(MFGEN) -flL $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	 a.ttcn && ${MAKEPROG}

Check_dynamic: Build_dynamic
	if [ ! -f ./bin_dynamic/liba.so ]; then exit 1; fi
	if [ -f ./bin_dynamic/a ]; then exit 1; fi

# ttcn3_makefilegen -L -l; make library
Build_dynamic2: a.ttcn
	rm -rf ./bin_dynamic2
	mkdir -p bin_dynamic2 && cd bin_dynamic2 && ln -fs ../a.ttcn \
	&& $(MFGEN) -flL $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	 a.ttcn && ${MAKEPROG} library

Check_dynamic2: Build_dynamic2
	if [ ! -f ./bin_dynamic2/liba.so ]; then exit 1; fi
	if [ -f ./bin_dynamic2/a ]; then exit 1; fi

# ttcn3_makefilegen -L -l; make executable
Build_dynamic3: a.ttcn
	rm -rf ./bin_dynamic3
	mkdir -p bin_dynamic3 && cd bin_dynamic3 && ln -fs ../a.ttcn \
	&& $(MFGEN) -flL $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	 a.ttcn && ${MAKEPROG} executable

Check_dynamic3: Build_dynamic3
	if [ ! -f ./bin_dynamic3/liba.so ]; then exit 1; fi
	if [ -f ./bin_dynamic3/a.so ]; then exit 1; fi
	if [ ! -f ./bin_dynamic3/a ]; then exit 1; fi

####### without -L ##############

# ttcn3_makefilegen -l; make
Build_dynamic4: a.ttcn
	rm -rf ./bin_dynamic4
	mkdir -p bin_dynamic4 && cd bin_dynamic4 && ln -fs ../a.ttcn \
	&& $(MFGEN) -fl $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	 a.ttcn && ${MAKEPROG}

Check_dynamic4: Build_dynamic4
	if [ -f ./bin_dynamic4/liba.so ]; then exit 1; fi
	if [ ! -f ./bin_dynamic4/a ]; then exit 1; fi

# ttcn3_makefilegen -l; make library 
Build_dynamic5: a.ttcn
	rm -rf ./bin_dynamic5
	mkdir -p bin_dynamic5 && cd bin_dynamic5 && ln -fs ../a.ttcn \
	&& $(MFGEN) -fl $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	 a.ttcn && ${MAKEPROG} library

Check_dynamic5: Build_dynamic5
	if [ ! -f ./bin_dynamic5/liba.so ]; then exit 1; fi
	if [ -f ./bin_dynamic5/a ]; then exit 1; fi

# ttcn3_makefilegen -l; make executable 
Build_dynamic6: a.ttcn
	rm -rf ./bin_dynamic6
	mkdir -p bin_dynamic6 && cd bin_dynamic6 && ln -fs ../a.ttcn \
	&& $(MFGEN) -fl $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	 a.ttcn && ${MAKEPROG} executable 

Check_dynamic6: Build_dynamic6
	if [ -f ./bin_dynamic6/liba.so ]; then exit 1; fi
	if [ ! -f ./bin_dynamic6/a ]; then exit 1; fi


##############################
# Some central storage stuff
##############################
# ttcn3_makefilegen  -c -l -L; make
Build_dynamic_central: b.ttcn Central
	mkdir -p bin_central && cd bin_central && ln -fs ../b.ttcn \
	&& $(MFGEN) -cflL $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	 b.ttcn ../central_storage/central.ttcn && ${MAKEPROG}

Check_dynamic_central: Build_dynamic_central
	if [ ! -f ./bin_central/libb.so ]; then exit 1; fi
	if [ -f ./bin_central/b ]; then exit 1; fi

# ttcn3_makefilegen  -c -l; make
Build_dynamic_central_exe: b.ttcn Central
	mkdir -p bin_central_exe && cd bin_central_exe && ln -fs ../b.ttcn \
	&& $(MFGEN) -cfl $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	 b.ttcn ../central_storage/central.ttcn && ${MAKEPROG}

Check_dynamic_central_exe: Build_dynamic_central_exe
	if [ ! -f ./bin_central_exe/b ]; then exit 1; fi
	if [ ! -f ./bin_central_exe/b.so ]; then exit 1; fi

Central:
	cd central_storage && $(MFGEN) $(MFGEN_FLAGS) -lf central.ttcn && ${MAKEPROG} shared_objects


###############################
# Generate makefile from tpd
##############################
# The default target is library in the tpd file
BuildHelloTpd:
	mkdir -p HelloTpd/bin && cd HelloTpd/bin \
	&& $(TTCN3_DIR)/bin/ttcn3_makefilegen -f $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	 -t ../HelloTpd.tpd && ${MAKEPROG}

CheckHelloTpd: BuildHelloTpd
	if [ -f HelloTpd/bin/HelloTpd_test.exe ]; then exit 1; fi
	if [ ! -f HelloTpd/bin/libHelloTpd_test.a ]; then exit 1; fi

# the default target is executable in the tpd file
# check if the command line option overrides the one defined in the tpd
BuildHelloTpd2:
	cd HelloTpd2 && ln -sf ../HelloTpd/src && ln -sf ../HelloTpd/.project && \
		ln -sf ../HelloTpd/.TITAN_properties
	mkdir -p HelloTpd2/bin && cd HelloTpd2/bin \
	&& $(TTCN3_DIR)/bin/ttcn3_makefilegen -fL $(MFGEN_FLAGS) $(COVERAGE_FLAG) \
	 -t ../HelloTpd.tpd && ${MAKEPROG}

CheckHelloTpd2: BuildHelloTpd2
	if [ -f HelloTpd2/bin/HelloTpd_test.exe ]; then exit 1; fi
	if [ ! -f HelloTpd2/bin/libHelloTpd_test.a ]; then exit 1; fi

clean:
	-rm -rf bin* 
	cd central_storage && if [ -f Makefile ] ; then ${MAKEPROG} $@ && rm -f Makefile; fi
	-rm -rf HelloTpd/bin 
	-rm -rf HelloTpd2/bin 

distclean: clean

.PHONY: all clean distclean static dynamic \
	BuildLibrary CheckLibrary BuildLibrary2 CheckLibrary2 BuildLibrary3 CheckLibrary3 BuildLibrary4 CheckLibrary4\
	BuildLibraryL CheckLibraryL BuildLibrary2L CheckLibrary2L BuildLibrary3L CheckLibrary3L BuildLibrary4L CheckLibrary4L\
	Check_dynamic_central Check_dynamic_central_exe Build_dynamic Check_dynamic Build_dynamic2 Check_dynamic2 \
	Build_dynamic3 Check_dynamic3 Build_dynamic4 Check_dynamic4 Build_dynamic5 Check_dynamic5 Build_dynamic6 Check_dynamic6 \
	Central BuildHelloTpd CheckHelloTpd



