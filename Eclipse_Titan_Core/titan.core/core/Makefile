##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   
#   Baji, Laszlo
#   Balasko, Jeno
#   Baranyi, Botond
#   Beres, Szabolcs
#   Delic, Adam
#   Feher, Csaba
#   Forstner, Matyas
#   Horvath, Gabriella
#   Koppany, Csaba
#   Kovacs, Ferenc
#   Raduly, Csaba
#   Szabados, Kristof
#   Szabo, Janos Zoltan – initial implementation
#   Szalai, Gabor
#   Zalanyi, Balazs Andor
#
##############################################################################
# Makefile for the Base Library of TTCN-3 Test Executor
TOP := ..
include $(TOP)/Makefile.cfg

SCHEMAS := TitanLoggerApi.xsd
# Generated from `TitanLoggerApi.xsd'.
GENERATED_MODULES := TitanLoggerApi.ttcn
	
PREGENERATOR_MODULES := PreGenRecordOf.ttcn

GENERATED_LOGGERAPI_SOURCES := $(GENERATED_MODULES:.ttcn=.cc)
GENERATED_LOGGERCONTROL_SOURCES := TitanLoggerControl.cc
	
PREGENERATED_SOURCES := $(PREGENERATOR_MODULES:.ttcn=.cc)

ORIGINATORS := TitanLoggerApi.xsd TitanLoggerApi.ttcn TitanLoggerControl.ttcn gccversion.c \
config_process.l config_process.y

ifneq ($(FUNCTION_TEST_RUNTIME), yes)
APIDIR := RT1
else
APIDIR := RT2
endif

# RT1 (load test) generates cc/hh files into the RT1/ directory,
# RT2 (function test) into the RT2/ directory. This has two purposes:
# * To prevent the vpath directive in core2/Makefile from finding files in RT1.
#   This way, RT2 has to generate these files for itself (with RT2 settings).
# * To allow TTCN3.hh to include the right kind of TitanLoggerApi.hh
# These directories (RT1/RT2) are replicated inside $(INCDIR)
GENERATED_LOGGERAPI_SOURCES     := $(addprefix $(APIDIR)/,$(GENERATED_LOGGERAPI_SOURCES))
GENERATED_LOGGERCONTROL_SOURCES := $(addprefix $(APIDIR)/,$(GENERATED_LOGGERCONTROL_SOURCES))
PREGENERATED_SOURCES		:= $(addprefix $(APIDIR)/,$(PREGENERATED_SOURCES))

GENERATED_SOURCES := $(GENERATED_LOGGERAPI_SOURCES)          $(GENERATED_LOGGERCONTROL_SOURCES)         config_process.lex.cc config_process.tab.cc
GENERATED_SOURCES += $(PREGENERATED_SOURCES)
GENERATED_HEADERS := $(GENERATED_LOGGERAPI_SOURCES:.cc=.hh)  $(GENERATED_LOGGERCONTROL_SOURCES:.cc=.hh) config_process.tab.hh config_process.lex.hh
GENERATED_HEADERS += $(GENERATED_LOGGERAPI_SOURCES:.cc=Simple.hh) $(PREGENERATED_SOURCES:.cc=.hh)
GENERATED_OTHERS  := $(GENERATED_LOGGERAPI_SOURCES).compiled $(GENERATED_LOGGERCONTROL_SOURCES).compiled
GENERATED_OTHERS  += $(PREGENERATED_SOURCES).compiled config_process.output lex.backup

TTCN_COMPILER_FLAGS :=
ifeq ($(FUNCTION_TEST_RUNTIME), yes)
  TTCN_COMPILER_FLAGS += -R
endif

TTCN_COMPILER_FLAGS += -o $(APIDIR)

# -I. is needed because TitanLoggerApi.hh (generated) does #include <TTCN3.hh>
CPPFLAGS += -I. -I$(APIDIR)

SOLARIS_LIBS   := -lsocket -lnsl -lxml2
SOLARIS8_LIBS  := -lsocket -lnsl -lxml2
LINUX_LIBS     := -lxml2
FREEBSD_LIBS   := -lxml2
WIN32_LIBS     := -lxml2
INTERIX_LIBS   := -lxml2
	
ifeq ($(ADVANCED_DEBUGGER_UI), yes)
  SOLARIS_LIBS   += -lcurses
  SOLARIS8_LIBS  += -lcurses
  LINUX_LIBS     += -lncurses
  FREEBSD_LIBS   += -lncurses
  WIN32_LIBS     += -lncurses
  INTERIX_LIBS   += -lncurses
endif

ifeq ($(USAGE_STATS), yes)
  SOLARIS_LIBS  += -lresolv
  SOLARIS8_LIBS += -lresolv
  LINUX_LIBS += -lpthread -lrt
endif

# No need for `-Wl,-export-dynamic' until we're using shared objects.  All
# `dl*()' related stuff is disabled for now.
ifeq ($(DYNAMIC_LOGGER_PLUGINS), yes)
  $(PLATFORM)_LIBS += -ldl
endif

# Note: RT2_SOURCES is set in core2/Makefile
STATIC_SOURCES := Addfunc.cc Array.cc ASN_Any.cc ASN_CharacterString.cc \
ASN_EmbeddedPDV.cc ASN_External.cc ASN_Null.cc Basetype.cc BER.cc Bitstring.cc \
Boolean.cc Charstring.cc Communication.cc Component.cc Default.cc Encdec.cc \
Error.cc Float.cc Hexstring.cc RInt.cc Integer.cc Logger.cc LoggerPlugin.cc \
LoggerPluginManager.cc LegacyLogger.cc LoggingBits.cc \
Module_list.cc Objid.cc Octetstring.cc Parallel_main.cc Port.cc RAW.cc \
Runtime.cc Single_main.cc Snapshot.cc Struct_of.cc Template.cc TEXT.cc \
Textbuf.cc Timer.cc Param_Types.cc Universal_charstring.cc \
Verdicttype.cc XER.cc XmlReader.cc TitanLoggerControlImpl.cc TCov.cc JSON.cc \
Profiler.cc ProfilerTools.cc ProfMerge_main.cc Debugger.cc DebuggerUI.cc \
$(RT2_SOURCES)

# Keep GENERATED_SOURCES at the beginning. This may speed up parallel builds
# by starting early the compilation of the largest files.
SOURCES := $(GENERATED_SOURCES) $(STATIC_SOURCES)
	

ifeq ($(FUNCTION_TEST_RUNTIME), yes)
  RT2_SUFFIX := -rt2
  CPPFLAGS += -DTITAN_RUNTIME_2
endif

ifdef XMLDIR
ifeq ($(XMLDIR), default)
  # Do not change XMLDIR !
  ACTUAL_XMLDIR := /usr
  # do not add -L/usr/lib to LDFLAGS, it's redundant
else
  ACTUAL_XMLDIR := $(XMLDIR)
  LDFLAGS  += -L$(ACTUAL_XMLDIR)/lib
endif
  # This addition to CPPFLAGS is always needed
  CPPFLAGS += -I$(ACTUAL_XMLDIR)/include/libxml2
endif

ifdef OPENSSL_DIR
ifneq ($(OPENSSL_DIR), default)
  CPPFLAGS += -I$(OPENSSL_DIR)/include
  LDFLAGS  += -L$(OPENSSL_DIR)/lib
endif
endif

ifeq ($(PLATFORM),INTERIX)
  # to get the "right" iconv, used by libxml2
  CPPFLAGS += -I/usr/local/include
endif


OBJECTS := $(patsubst %.cc,%.o,$(patsubst %.c,%.o,$(SOURCES)))
	
PROFMERGE_OBJECTS := ProfMerge_main.o ProfilerTools.profmerge.o \
	../common/JSON_Tokenizer.o ../common/memory.o

COMMON_OBJECTS := $(addprefix ../common/, memory.o pattern_la.o pattern_p.o \
	            config_preproc.o config_preproc_la.o config_preproc_p.tab.o \
	            path.o pattern_uni.o Quadruple.o NetworkHandler.o Path2.o \
	            ModuleVersion.o JSON_Tokenizer.o UnicharPattern.o)

ifeq ($(DEBUG), yes)
  COMMON_OBJECTS += ../common/new.o
endif

ifeq ($(LICENSING), yes)
  COMMON_OBJECTS += ../common/license.o
  PROFMERGE_OBJECTS += ../common/license.o
endif

ifdef REGEX_DIR
  COMMON_OBJECTS += $(REGEX_DIR)/lib/regex.o
endif

ifeq ($(USAGE_STATS), yes)
  COMMON_OBJECTS += ../common/usage_stats.o
endif

ifeq ($(ADVANCED_DEBUGGER_UI), yes)
EDITLINE_OBJECTS := $(addprefix ../mctr2/editline/build/src/.libs/, chared.o common.o \
emacs.o fgetln.o help.o history.o map.o prompt.o read.o search.o strlcat.o term.o tty.o \
vi.o wcsdup.o chartype.o el.o fcns.o filecomplete.o hist.o key.o parse.o readline.o \
refresh.o sig.o strlcpy.o tokenizer.o unvis.o vis.o)
endif

LIBRARY_OBJECTS_NOMAIN := $(filter-out Single_main.o Parallel_main.o ProfMerge_main.o, \
  $(OBJECTS)) $(COMMON_OBJECTS) $(EDITLINE_OBJECTS)

TOBECLEANED := LoggerPlugin_static.o LoggerPlugin_dynamic.o ProfilerTools.profmerge.o

DEPFILES := $(patsubst %.cc,%.d,$(patsubst %.c,%.d,$(SOURCES)))

SUBDIRS :=

HEADERS := TTCN3.hh Types.h Param_Types.hh Basetype.hh Template.hh RInt.hh \
Integer.hh Float.hh Boolean.hh Objid.hh Verdicttype.hh Bitstring.hh \
Hexstring.hh Octetstring.hh Charstring.hh Universal_charstring.hh Component.hh \
Logger.hh ILoggerPlugin.hh Error.hh Timer.hh Runtime.hh Snapshot.hh Default.hh \
Port.hh Event_Handler.hh Struct_of.hh Array.hh Optional.hh Textbuf.hh Encdec.hh \
Module_list.hh Parameters.h Addfunc.hh RAW.hh BER.hh TEXT.hh ASN_Null.hh \
ASN_Any.hh ASN_External.hh ASN_EmbeddedPDV.hh ASN_CharacterString.hh XER.hh \
XmlReader.hh cversion.h TitanLoggerControl.ttcn TitanLoggerApi.xsd Vector.hh \
JSON.hh Profiler.hh RefdIndex.hh ProfilerTools.hh Debugger.hh DebugCommands.hh \
DebuggerUI.hh
# Copied during "make install"

ifdef REGEX_DIR
  HEADERS += $(REGEX_DIR)/include/regex.h
endif

# # # # Library and shared lib names
LIBRARY := libttcn3$(RT2_SUFFIX).a
PARALLEL_LIBRARY := libttcn3$(RT2_SUFFIX)-parallel.a
SHAREDLIB := libttcn3$(RT2_SUFFIX)-dynamic.so
PARALLEL_SHAREDLIB := libttcn3$(RT2_SUFFIX)-parallel-dynamic.so
	
# Executables
ifneq ($(FUNCTION_TEST_RUNTIME), yes)
PROGRAMS := ttcn3_profmerge$(EXESUFFIX)
endif

TARGETS := $(LIBRARY) $(PARALLEL_LIBRARY)

# Determine whether to build shared libraries:
DYNAMIC_LINKING := yes
ifeq ($(PLATFORM), WIN32)
  LDFLAGS += -Xlinker --out-implib -Xlinker $(basename $@).a
endif
ifeq ($(PLATFORM), INTERIX)
  DYNAMIC_LINKING := no
endif

ifeq ($(DYNAMIC_LINKING), yes)
  TARGETS += $(SHAREDLIB) $(PARALLEL_SHAREDLIB)
  ifeq ($(PLATFORM), WIN32)
    # import libraries are generated as a side-effect; need to be cleaned
    TOBECLEANED += $(SHAREDLIB:.so=.a) $(PARALLEL_SHAREDLIB:.so=.a)
  endif
endif

ifneq ($(FUNCTION_TEST_RUNTIME), yes)
  TARGETS += cversion.h
endif

# Only for testing the consistency of libraries:
TEST_PROGRAMS := single$(RT2_SUFFIX)$(EXESUFFIX) parallel$(RT2_SUFFIX)$(EXESUFFIX)
ifeq ($(DYNAMIC_LINKING), yes)
TEST_PROGRAMS += single$(RT2_SUFFIX)-dynamic$(EXESUFFIX) parallel$(RT2_SUFFIX)-dynamic$(EXESUFFIX)
endif
ifneq ($(FUNCTION_TEST_RUNTIME), yes)
TEST_PROGRAMS += gccversion$(EXESUFFIX)
endif


TOBECLEANED += $(TEST_PROGRAMS) cversion.h $(GENERATED_MODULES)

ifdef REGEX_DIR
  CPPFLAGS += -I$(REGEX_DIR)/include
endif


ifdef SRCDIR
COMMON_DIR := $(SRCDIR)/common
else
COMMON_DIR := ../common
endif

CPPFLAGS += -I$(COMMON_DIR)

MANPAGES := ttcn3_profmerge.1

# # # # # # # # # targets
all: $(TARGETS) $(PROGRAMS) $(TEST_PROGRAMS)

run: all
# Verify that the executables are runnable.
	for p in $(filter-out gccversion%,$(TEST_PROGRAMS)); do echo $$p; LD_LIBRARY_PATH=.:$$LD_LIBRARY_PATH ./$$p -l; if [ $$? -ne 0 ] ; then exit $$?; fi done

# xsd -> ttcn
$(GENERATED_MODULES): $(SCHEMAS) ../xsdconvert/xsd2ttcn$(EXESUFFIX)
	../xsdconvert/xsd2ttcn$(EXESUFFIX) -p $<
	sed -e 's/XSD.String/charstring/g;s/XSD.AnySimpleType/charstring/g;s/XSD.Integer/integer/g;s/XSD.Float/float/g;s/XSD.Double/float/g;s/XSD.Boolean/boolean/g;s/import from XSD all;//g' TitanLoggerApi.ttcn >TitanLoggerApi.ttcn_
	mv TitanLoggerApi.ttcn_ TitanLoggerApi.ttcn

install: $(TARGETS) $(PROGRAMS)
ifeq ($(DYNAMIC_LINKING), yes)
  ifeq ($(DEBUG), no)
	$(STRIP) $(SHAREDLIB) $(PARALLEL_SHAREDLIB)
  endif
endif
	mkdir -p $(LIBDIR)
	cp $(TARGETS) $(LIBDIR)
ifneq ($(FUNCTION_TEST_RUNTIME), yes)
	mkdir -p $(INCDIR)
	cp $(HEADERS) $(INCDIR)
	mkdir -p $(BINDIR)
	cp $(PROGRAMS) $(BINDIR)
	mkdir -p $(MANDIR)/man1
	cp $(MANPAGES) $(MANDIR)/man1
ifeq ($(DYNAMIC_LINKING), yes)
ifneq ($(OPENSSL_DIR), default)
	(cd $(OPENSSL_DIR)/lib && tar -cf - libcrypto.so*) |\
	(cd $(LIBDIR) && tar -xf -)
endif
ifneq ($(XMLDIR), default)
	(cd $(XMLDIR)/lib && tar -cf - libxml2.so*) |\
	(cd $(LIBDIR) && tar -xf -)
endif
endif
endif
	mkdir -p $(INCDIR)/$(APIDIR)
	cp $(APIDIR)/TitanLoggerApi*.hh $(INCDIR)/$(APIDIR)
	cp $(APIDIR)/PreGenRecordOf.hh $(INCDIR)/$(APIDIR)

# # # # executables
single$(RT2_SUFFIX)$(EXESUFFIX): $(LIBRARY)
	$(CXX) $(LDFLAGS) -o $@ $^ -lcrypto $($(PLATFORM)_LIBS)

single$(RT2_SUFFIX)-dynamic$(EXESUFFIX): Single_main.o $(SHAREDLIB)
	$(CXX) $(LDFLAGS) -o $@ $^ -lcrypto $($(PLATFORM)_LIBS)

parallel$(RT2_SUFFIX)$(EXESUFFIX): $(PARALLEL_LIBRARY)
	$(CXX) $(LDFLAGS) -o $@ $^ -lcrypto $($(PLATFORM)_LIBS)

parallel$(RT2_SUFFIX)-dynamic$(EXESUFFIX): Parallel_main.o $(PARALLEL_SHAREDLIB)
	$(CXX) $(LDFLAGS) -o $@ $^ -lcrypto $($(PLATFORM)_LIBS)
	
ttcn3_profmerge$(EXESUFFIX): $(PROFMERGE_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LICENSE_LIBS) $($(PLATFORM)_LIBS)

# # # # static link libraries
$(LIBRARY): $(LIBRARY_OBJECTS_NOMAIN) LoggerPlugin_static.o Single_main.o
	$(AR) -r $@ $?

$(PARALLEL_LIBRARY): $(LIBRARY_OBJECTS_NOMAIN) LoggerPlugin_static.o Parallel_main.o
	$(AR) -r $@ $?

# # # # dynamic link libraries

$(SHAREDLIB): $(LIBRARY_OBJECTS_NOMAIN) LoggerPlugin_dynamic.o Single_main.o
	$(LD) $(LDFLAGS) -o $@ $^ -L$(OPENSSL_DIR)/lib -lcrypto $($(PLATFORM)_LIBS)

$(PARALLEL_SHAREDLIB): $(LIBRARY_OBJECTS_NOMAIN) LoggerPlugin_dynamic.o Parallel_main.o
	$(LD) $(LDFLAGS) -o $@ $^ -L$(OPENSSL_DIR)/lib -lcrypto $($(PLATFORM)_LIBS)

ifneq ($(FUNCTION_TEST_RUNTIME), yes)
# core2 -> core dependency, core2 does not run flex and bison
# core must be made before core2

%.lex.cc %.lex.hh: %.l
	$(FLEX) $(FLEXFLAGS) -P$(subst .l,_,$<) --header-file=$(subst .l,.lex.hh,$<) \
		-o$(subst .l,.lex.cc,$<) $<

%.tab.cc %.tab.hh: %.y %.lex.hh
	$(BISON) $(BISONFLAGS) -o $(subst .y,.tab.cc,$<) -p $(subst .y,_,$<) $<

cversion.h: gccversion$(EXESUFFIX)
	./$< > $@.tmp
	if [ -f $@ ] ; then \
	  if diff $@.tmp $@ >/dev/null ; then \
	    rm $@.tmp ; \
	  else \
	    mv $@.tmp $@ ; \
	  fi \
	else \
	  mv $@.tmp $@ ; \
	fi

gccversion$(EXESUFFIX): gccversion.c ../Makefile.cfg
# The line before the backslash is $(LINK.c), except it uses CCFLAGS
# instead of CFLAGS. TARGET_ARCH is not used by Titan.
	$(CC) $(CCFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH) \
	$< $(LOADLIBES) $(LDLIBS) -o $@
endif

#LegacyLogger.d: cversion.h
$(DEPFILES): cversion.h

#TitanLoggerControlImpl.d Single_main.d Runtime.d Port.d LegacyLogger.d \
#LoggerPlugin.d LoggerPluginManager.d Error.d Default.d Communication.d \
#Addfunc.d: $(GENERATED_LOGGERAPI_SOURCES:.cc=.cc)

TitanLoggerControlImpl.d: $(GENERATED_LOGGERCONTROL_SOURCES)


$(GENERATED_LOGGERAPI_SOURCES): $(GENERATED_LOGGERAPI_SOURCES).compiled
	@if [ ! -f $@ ]; then $(RM) $<; $(MAKE) $<; fi

$(GENERATED_LOGGERAPI_SOURCES).compiled:: ../compiler2/compiler$(EXESUFFIX)
	@if [ -f $@ ]; then $(RM) $@; $(MAKE) $@; fi

$(GENERATED_LOGGERAPI_SOURCES).compiled:: $(GENERATED_MODULES)
	mkdir -p $(APIDIR)
# The input (TitanLoggerApi.ttcn) is a generated file and might get corrupted
# by parallel execution of xsd2ttcn. Get it out of the way but save it for later.
	if ../compiler2/compiler$(EXESUFFIX) $(TTCN_COMPILER_FLAGS) $^ - $?; then :; else mv $? $?.$$$$.bad; exit 1; fi
	perl $(ABS_SRC)/../core/simplify.pl $(GENERATED_LOGGERAPI_SOURCES:.cc=.hh) > $(GENERATED_LOGGERAPI_SOURCES:.cc=Simple.hh) 
	touch $@

$(GENERATED_LOGGERCONTROL_SOURCES): $(GENERATED_LOGGERCONTROL_SOURCES).compiled
	@if [ ! -f $@ ]; then $(RM) $<; $(MAKE) $<; fi

$(GENERATED_LOGGERCONTROL_SOURCES).compiled:: ../compiler2/compiler$(EXESUFFIX)
	@if [ -f $@ ]; then $(RM) $@; $(MAKE) $@; fi

$(GENERATED_LOGGERCONTROL_SOURCES).compiled:: TitanLoggerControl.ttcn
	mkdir -p $(APIDIR)
	../compiler2/compiler$(EXESUFFIX) $(TTCN_COMPILER_FLAGS) $^ - $?
	touch $@
	
$(PREGENERATED_SOURCES): $(PREGENERATED_SOURCES).compiled
	@if [ ! -f $@ ]; then $(RM) $<; $(MAKE) $<; fi

$(PREGENERATED_SOURCES).compiled:: ../compiler2/compiler$(EXESUFFIX)
	@if [ -f $@ ]; then $(RM) $@; $(MAKE) $@; fi

$(PREGENERATED_SOURCES).compiled:: $(PREGENERATOR_MODULES)
	mkdir -p $(APIDIR)
	../compiler2/compiler$(EXESUFFIX) -F $(TTCN_COMPILER_FLAGS) $^ - $?
	touch $@
	
include ../Makefile.genrules

# These are not in STATIC_SOURCES, so vpath is not created for them
vpath LoggerPlugin_dynamic.cc $(ABS_SRC)
vpath LoggerPlugin_static.cc  $(ABS_SRC)
