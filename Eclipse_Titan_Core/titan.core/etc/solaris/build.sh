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

set -ex

[ "`uname -sr`" = "SunOS 5.10" ] || {
	echo 'unsupported OS'
	exit 1
}

P=/mnt/TTCN/Tools
LC_ALL=C
export LC_ALL

# version of packages in src/
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

buildcommon() {
	V=$1
	C=$2
	shift
	shift
	if [ -f src/$V.tar.gz ]
	then
		gunzip -c src/$V.tar.gz | tar -xf -
	else
		bunzip2 -c src/$V.tar.bz2 | tar -xf -
	fi
	cd $V
	./$C --prefix=$P/$V "$@"
	{ make || kill $$; } |tee make.log
	{ make install || kill $$; } |tee make.install.log
	cd ..
	rm -rf $V
}

build() {
	V=$1
	shift
	buildcommon $V configure "$@"
}

buildtcl() {
	buildcommon $V_tcl unix/configure "$@"
}

buildopenssl() {
	buildcommon $V_openssl Configure shared --openssldir=$P/$V_openssl solaris-x86-gcc
	chmod u+w $P/$V_openssl/lib/lib*.so.0.9.*
}

# uncomment/comment the build commands of the packages
# should be original gcc and binutils paths
GCC0=$P/$V_gcc
BINUTILS0=$P/$V_binutils
PATH=$BINUTILS0/bin:$GCC0/bin:/usr/bin
LD_LIBRARY_PATH=$BINUTILS0/lib:$GCC0/lib:/lib:/usr/lib
export PATH LD_LIBRARY_PATH

#build $V_zlib
LD_LIBRARY_PATH=$P/$V_zlib/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH

build $V_binutils
PATH=$P/$V_binutils/bin:$PATH
LD_LIBRARY_PATH=$P/$V_binutils/lib:$LD_LIBRARY_PATH
export PATH LD_LIBRARY_PATH

#build $V_libiconv
LD_LIBRARY_PATH=$P/$V_libiconv/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH

# solaris tar had some issues with the gcc and openssl tar.gz
#build $V_tar --with-libiconv-prefix=$P/$V_libiconv --with-gnu-ld
PATH=$P/$V_tar/bin:$PATH
export PATH

#build $V_make
PATH=$P/$V_make/bin:$PATH
export PATH

#build $V_m4
PATH=$P/$V_m4/bin:$PATH
export PATH

#build $V_gmp
LD_LIBRARY_PATH=$P/$V_gmp/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH

#build $V_mpfr --with-gmp=$P/$V_gmp
LD_LIBRARY_PATH=$P/$V_mpfr/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH

#build $V_mpc --with-gmp=$P/$V_gmp --with-mpfr=$P/$V_mpfr
LD_LIBRARY_PATH=$P/$V_mpc/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH

build $V_gcc --enable-languages=c,c++ \
  --disable-multilib \
  --with-gmp=$P/$V_gmp \
  --with-mpfr=$P/$V_mpfr \
  --with-mpc=$P/$V_mpc \
  --with-gnu-as --with-as=$P/$V_binutils/bin/as \
  --with-gnu-ld --with-ld=$P/$V_binutils/bin/ld
PATH=$P/$V_gcc/bin:$PATH
LD_LIBRARY_PATH=$P/$V_gcc/lib:$LD_LIBRARY_PATH
export PATH LD_LIBRARY_PATH

#buildopenssl
PATH=$P/$V_openssl/bin:$PATH
LD_LIBRARY_PATH=$P/$V_openssl/lib:$LD_LIBRARY_PATH
export PATH LD_LIBRARY_PATH

#build bison
#build flex
#buildtcl
#buildexpect --exec-prefix=$P
#build libxml2
#build wget --with-ssl=openssl

echo $PATH
echo $LD_LIBRARY_PATH
