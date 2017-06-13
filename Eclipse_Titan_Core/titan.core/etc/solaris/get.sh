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

# download package sources with given version into src/

set -ex

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

mkdir -p src
cd src

[ -f $V_make.tar.gz ] || wget http://mirrors.kernel.org/gnu/make/$V_make.tar.gz
[ -f $V_tar.tar.gz ] || wget http://mirrors.kernel.org/gnu/tar/$V_tar.tar.gz
[ -f $V_zlib.tar.gz ] || wget http://zlib.net/$V_zlib.tar.gz
[ -f $V_binutils.tar.gz ] || wget http://mirrors.kernel.org/gnu/binutils/$V_binutils.tar.gz
[ -f $V_mpfr.tar.gz ] || wget http://mirrors.kernel.org/gnu/mpfr/$V_mpfr.tar.gz
[ -f $V_mpc.tar.gz ] || wget http://mirrors.kernel.org/gnu/mpc/$V_mpc.tar.gz
[ -f $V_gmp.tar.bz2 ] || wget http://mirrors.kernel.org/gnu/gmp/$V_gmp.tar.bz2
[ -f $V_gcc.tar.gz ] || wget http://mirrors.kernel.org/gnu/gcc/$V_gcc/$V_gcc.tar.gz
[ -f $V_libiconv.tar.gz ] || wget http://mirrors.kernel.org/gnu/libiconv/$V_libiconv.tar.gz
[ -f $V_bison.tar.gz ] || wget http://mirrors.kernel.org/gnu/bison/$V_bison.tar.gz
[ -f $V_flex.tar.gz ] || wget http://prdownloads.sourceforge.net/flex/$V_flex.tar.gz
[ -f $V_m4.tar.gz ] || wget http://mirrors.kernel.org/gnu/m4/$V_m4.tar.gz
[ -f $V_libxml2.tar.gz ] || wget ftp://xmlsoft.org/libxml2/$V_libxml2.tar.gz
[ -f $V_openssl.tar.gz ] || wget http://www.openssl.org/source/$V_openssl.tar.gz
[ -f $V_expect.tar.gz ] || wget http://downloads.sourceforge.net/project/expect/Expect/`echo $V |sed 's/expect//'`/$V.tar.gz
[ -f $V_tcl.tar.gz ] || wget -O $V_tcl.tar.gz http://prdownloads.sourceforge.net/tcl/$V_tcl-src.tar.gz

cd ..
