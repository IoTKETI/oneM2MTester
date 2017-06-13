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
#   Raduly, Csaba
#   Szabados, Kristof
#
##############################################################################
# Makefile for build and install nedit and xemacs additions
#
#

TOP :=  ../..
include ../../Makefile.cfg

SUBDIR = build
DEPFILES =  $(SUBDIR)/Makefile $(SUBDIR)/config.h $(SUBDIR)/config.status
CONFIGURING = configuring
UPPERCFG = ../../Makefile.cfg

ifeq ($(DEBUG), no)
INSTALL = install-strip
else
INSTALL = install
endif

# Variable wrappers
# make sure "configure" doesn't get -Werror
CFLAGS=$(filter-out -Werror,$(CCFLAGS) $(CPPFLAGS)) -Wno-unused-value
EXEEXT=$(EXESUFFIX)
prefix=$(TTCN3_DIR)
mandir=$(MANDIR)
# For MinGW only
bindir=$(BINDIR)

ifeq ($(PLATFORM), WIN32)
# Help "configure" find ncurses headers on Cygwin 1.7
CPPFLAGS+=-I/usr/include/ncurses
endif

# Override make options for ctags' autoconf
OPTIONS =MINGW CPPFLAGS CC CCFLAGS CCDEPFLAG CXXDEPFLAG EXEEXT CFLAGS LDFLAGS prefix mandir
ifdef MINGW
OPTIONS+=bindir
endif

# path and name of 'config.site'
#config_site = config.site

CONFIGOPTIONS = $(foreach m, $(OPTIONS), $(m)='$($m)')

ifdef SRCDIR
CONFIG_PARAMS := --srcdir=$(SRCDIR)/mctr2/editline/libedit
else
SRCDIR := $(ABS_TOP)
endif

all run: $(DEPFILES)
	$(MAKE) -C $(SUBDIR) all

clean:
	- cd $(SUBDIR) && $(MAKE) $@ || exit

distclean:
	rm -rf $(SUBDIR)
	rm -f $(CONFIGURING) $(DEPFILES)

dep: $(DEPFILES)

$(DEPFILES): $(UPPERCFG) $(CONFIGURING)

$(CONFIGURING): $(UPPERCFG)
	@touch $(SRCDIR)/mctr2/editline/libedit/*.m4
	@touch $(SRCDIR)/mctr2/editline/libedit/configure
	@touch $(SRCDIR)/mctr2/editline/libedit/config.h.in
	mkdir -p $(SUBDIR)
	@echo Running 'configure' script...
	cd $(SUBDIR) && $(CONFIGOPTIONS) $(SRCDIR)/mctr2/editline/libedit/configure $(CONFIG_PARAMS)
	@touch $(CONFIGURING)

.PHONY: all install tags dep clean distclean
