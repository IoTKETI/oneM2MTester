# Common makefile for all the TTCN_* and ASN_* subdirectories

# WARNING! This Makefile can be used with GNU make only.
# Other versions of make may report syntax errors in it.

#
# Do NOT touch this line...
#
.PHONY: all archive check clean dep objects run

.SUFFIXES: .d

#
# Set these variables...
#


# Flags for the TTCN-3 and ASN.1 compiler:
COMPILER_FLAGS := -s -g
ifeq ($(RT2), yes)
COMPILER_FLAGS += -R
endif

# TTCN-3 modules of this project:
TTCN3_MODULES := $(sort $(wildcard *A.ttcn *S[WE].ttcn *OK.ttcn))

# ASN.1 modules of this project:
ASN1_MODULES := $(wildcard *A.asn)

# Remember, this makefile runs in one of the subdirectories.

# C++ source & header files generated from the TTCN-3 & ASN.1 modules of
# this project:
GENERATED_SOURCES := $(TTCN3_MODULES:.ttcn=.cc) $(ASN1_MODULES:.asn=.cc)
GENERATED_HEADERS := $(GENERATED_SOURCES:.cc=.hh)

# C/C++ Source & header files of Test Ports, external functions and
# other modules:
USER_SOURCES :=
USER_HEADERS := $(USER_SOURCES:.cc=.hh)

# Object files (will not be built)
OBJECTS := $(GENERATED_OBJECTS) $(USER_OBJECTS)

GENERATED_OBJECTS := $(GENERATED_SOURCES:.cc=.o)

USER_OBJECTS := $(USER_SOURCES:.cc=.o)

DEPFILES := $(USER_OBJECTS:.o=.d)  $(GENERATED_OBJECTS:.o=.d)

# Other files of the project (Makefile, configuration files, etc.)
# that will be added to the archived source files:
OTHER_FILES := Makefile

# lastword only in GNU make 3.81 and above :(
COMMON_MK := $(word $(words $(MAKEFILE_LIST)), $(MAKEFILE_LIST))
CW := $(COMMON_MK:common.mk=cw.pl)

include ../../Makefile.personal
ifeq ($(DEBUG), yes)
CPPFLAGS += -DMEMORY_DEBUG
endif

#
# Rules for building the executable...
#

all: $(GENERATED_SOURCES) ;

.cc.o .c.o:
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $@ $<

.cc.d .c.d:
	@echo Creating dependency file for '$<'; set -e; \
	$(CXX) $(CXXDEPFLAGS) $(CPPFLAGS) $(CXXFLAGS) $< \
	| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
	[ -s $@ ] || rm -f $@


#
# This is the "Run the compiler on every source you can find" target.
#
run check: $(TTCN3_MODULES) $(ASN1_MODULES)
	@$(CW) $(COMPILER_FLAGS) $(subst A.,?.,$^)


clean:
	-$(RM) $(TARGET) $(OBJECTS) $(GENERATED_HEADERS) \
	$(GENERATED_SOURCES) compile $(DEPFILES) \
	tags *.log

dep: $(GENERATED_SOURCES) $(USER_SOURCES) ;

ifeq ($(findstring n,$(MAKEFLAGS)),)
ifeq ($(filter clean distclean check compile archive diag,$(MAKECMDGOALS)),)
# -include $(DEPFILES)
endif
endif


diag:
	$(TTCN3_DIR)/bin/compiler -v 2>&1
	$(TTCN3_DIR)/bin/mctr_cli -v 2>&1
	$(CXX) -v 2>&1
	@echo TTCN3_DIR=$(TTCN3_DIR)
	@echo OPENSSL_DIR=$(OPENSSL_DIR)
	@echo XMLDIR=$(XMLDIR)
	@echo PLATFORM=$(PLATFORM)

#
# Add your rules here if necessary...
#

%.hh: %.ttcn
	mycompiler $(COMPILER_FLAGS) $<

%.cc: %.ttcn
	$(CW) $(COMPILER_FLAGS) $(subst A.,?.,$<)
# || ( mycompiler $(COMPILER_FLAGS) $(subst A.,?.,$<) && fail )

s: $(GENERATED_SOURCES)
h: $(GENERATED_HEADERS)

%.cc: %.asn
	$(CW) $(COMPILER_FLAGS) $(subst A.,?.,$<)
# || mycompiler $(COMPILER_FLAGS) $(subst A.,?.,$<)

export HARNESS_ACTIVE=1
