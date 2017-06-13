###############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#   Lovassy, Arpad
#
###############################################################################

1. GENERATING BUILD.XML

build.xml is generated.
The generated build.xml is modified manually:
  target TITAN_Executor_API_test is modified to fail in ant level if any testcase fails to make Jenkins show the failures
  failureproperty="test.failed" added to <junit fork="yes" printsummary="withOutAndErr" HERE>
  <fail if="test.failed" message="TITAN_Executor_API_test FAILED"/> added after </junit>

Steps to generate build.xml from Eclipse:
  1. Right click on TITAN_Executor_API -> Export...
  2. Select General/Ant Buildfiles
JUnit will be included in build.xml

2. RUN JUNIT TESTS

Requirements:
  Java 1.7
    for compiling Titan with JNI Java SDK (JDK) 1.7 is needed
  $TTCN3_DIR is set to Titan install directory
  ${TTCN3_DIR}/lib/libmctrjninative.so exists and ${TTCN3_DIR}/lib is added to $LD_LIBRARY_PATH

The test project depends on these external jars:
  junit.jar (JUnit4)
  Hamcrest core 1.3
    https://code.google.com/p/hamcrest/downloads/list

Test compiling and running from command line is done with this command:
(NOTE: this script also compiles its dependecies)

ant \
-lib <JUnit jar> \
-lib <Hamcrest core jar> \
TITAN_Executor_API_test

For example
ant \
-lib ${LIB_DIR}/org.junit_4.11.0.v201303080030/junit.jar \
-lib ${LIB_DIR}/org.hamcrest.core_1.3.0.v201303031735.jar \
TITAN_Executor_API_test

See build_and_run_test.sh, this script also check the dependencies before test compiling and running.

3. TROUBLESHOOTING
Typical error situations during test running and their solutions

3.1
Error:
java.lang.UnsatisfiedLinkError: org.eclipse.titan.executor.jni.JNIMiddleWare.init(I)J
Reason:
The Titan binaries you use are old (before 2014-12-11 or release before CRL 113 200/5 R1A), and since then the project became open source and thatâs why all the java packages were renamed from com.ericsson.titan.* to org.eclipse.titan.*
Solution:
So you should use the latest release.
You can download a new package from
ttcn.ericsson.se/download/
Search for "TITAN packages", download the latest version
You can extract it locally in your home directory, just make sure, that
TTCN3_DIR is set properly
PATH contains its bin directory
LD_LIBRARY_PATH contains its lib directory

3.2
Error:
.../lib/libmctrjninative.so: wrong ELF class: ELFCLASS64 (Possible cause: architecture word width mismatch)
Reason:
You use a 32-bit JDK on a 64-bit system.
Solution:
So you should download and use a new one.

So download this file:
jdk-7u75-linux-x64.tar.gz
http://download.oracle.com

Extract it to your home directory, you will get a directory like this:
jdk-7u75-linux-x64
create a symlink to it:
ln -s jdk-7u75-linux-x64 jdk

set the following variables in your .bashrc
JDKDIR=$HOME/jdk
export JDKDIR
PATH=$HOME/jdk/bin:${PATH}
export PATH
LD_LIBRARY_PATH=$HOME/jdk/lib:.:${LD_LIBRARY_PATH}
export LD_LIBRARY_PATH

In case of tcsh
.cshrc.user
setenv JDKDIR $HOME/jdk
setenv JAVA_HOME $HOME/jdk
setenv LD_LIBRARY_PATH ${JAVA_HOME}/lib:$LD_LIBRARY_PATH
setenv PATH ${JAVA_HOME}/bin:$PATH

Then start a new terminal and check the result with java -version

3.3
Error:
org.eclipse.titan.executorapi.exception.JniExecutorJniLoadException: JNI dynamic library could not be loaded.
Reason:
libmctrjninative.so is missing or not found
Solution:
TTCN3_DIR must be added to LD_LIBRARY_PATH
Add this line to .bashrc
LD_LIBRARY_PATH=${TTCN3_DIR}/lib:${LD_LIBRARY_PATH}

If Titan is built locally, Makefile.personal must contain the following lines:
JNI := yes
JDKDIR := $HOME/jdk
