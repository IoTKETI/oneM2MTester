###############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   >
#   Balasko, Jeno
#
###############################################################################
#!/bin/sh

# source this file on solaris shell to get usable environment

P=/mnt/TTCN/Tools

V_zlib=zlib-1.2.8
V_tar=tar-1.26
V_make=make-3.82
V_m4=m4-1.4.16
V_gmp=gmp-5.1.1
V_mpc=mpc-1.0.1
V_mpfr=mpfr-3.1.2
V_gcc=gcc-4.7.3
V_binutils=binutils-2.23.2
V_bison=bison-2.7
V_flex=flex-2.5.37
V_libiconv=libiconv-1.14
V_libxml2=libxml2-2.7.8
V_openssl=openssl-0.9.8y
V_tcl=tcl8.6.0
V_expect=expect5.45

PATH=$P/$V_binutils/bin:$P/$V_make/bin:$P/$V_tar/bin:$P/$V_m4/bin:$P/$V_gcc/bin:/usr/bin
LD_LIBRARY_PATH=$P/$V_zlib/lib:$P/$V_libiconv/lib:$P/$V_openssl/lib:$P/$V_libxml2/lib:$P/$V_gmp/lib:$P/$V_mpc/lib:$P/$V_mpfr/lib:$P/$V_gcc/lib:/lib:/usr/lib

TTCN3_LICENSE_FILE=$HOME/license.dat

export PATH LD_LIBRARY_PATH TTCN3_LICENSE_FILE
