##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#   Forstner, Matyas
#   Gecse, Roland
#   Raduly, Csaba
#   Szabo, Janos Zoltan – initial implementation
#
##############################################################################
# Makefile for the license generator program

include ../Makefile.cfg

MYSQL_DIR := /mnt/TTCN/Tools/mysql-5.0.41-solaris8-sparc
TARGET_DIR := /mnt/TTCN/license

CPPFLAGS += -I$(OPENSSL_DIR)/include -I$(MYSQL_DIR)/include

SOLARIS_LIBS := -lsocket -lnsl -lm -lz
SOLARIS8_LIBS := -lsocket -lnsl -lm -lz
LINUX_LIBS :=
FREEBSD_LIBS :=
WIN32_LIBS :=

SOURCES := license_gen.c
DEPFILES := $(patsubst %.cc,%.d,$(patsubst %.c,%.d,$(SOURCES)))
COMMON_OBJECTS := ../common/license.o ../common/memory.o
OBJECTS := $(patsubst %.cc,%.o,$(patsubst %.c,%.o,$(SOURCES))) $(COMMON_OBJECTS)
TARGETS := license_gen$(EXESUFFIX)

license_gen$(EXESUFFIX): $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(MYSQL_DIR)/lib/libmysqlclient.a \
	$(OPENSSL_DIR)/lib/libcrypto.a $($(PLATFORM)_LIBS)
	chmod g+w $(TARGETS)

install: $(TARGETS)
	strip $(TARGETS)
	cp $(TARGETS) $(TARGET_DIR)
	chmod 710 $(TARGET_DIR)/license_gen$(EXESUFFIX)
	chmod u+s $(TARGET_DIR)/license_gen$(EXESUFFIX)

include ../Makefile.genrules
