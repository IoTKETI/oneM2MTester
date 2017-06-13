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

# EDIT THESE LINES TO SET CORRECT JAR LOCATIONS

JUNIT_FULL=$HOME/lib/org.junit_4.11.0.v201303080030/junit.jar
HAMCREST_FULL=$HOME/lib/org.hamcrest.core_1.3.0.v201303031735.jar

#---------------------------------------------------------------------
# DO NOT EDIT AFTER THIS LINE

# check if JAVA exists
# http://stackoverflow.com/questions/7334754/correct-way-to-check-java-version-from-bash-script
if type -p java; then
    echo found java executable in PATH
    _java=java
elif [[ -n "$JAVA_HOME" ]] && [[ -x "$JAVA_HOME/bin/java" ]];  then
    echo found java executable in JAVA_HOME
    _java="$JAVA_HOME/bin/java"
else
    echo "no java, exiting"; exit 1;
fi

# check java version (>=1.7)
if [[ "$_java" ]]; then
    version=$("$_java" -version 2>&1 | awk -F '"' '/version/ {print $2}')
    echo version "$version"
    if [[ "$version" > "1.7" ]]; then
        echo "version is at least 1.7, OK";
    else
        echo "version is less than 1.7, NOT OK, exiting"; exit 1;
    fi
fi

# check java version (>=1.7) in another way
[ $(java -version 2>&1 | sed 's/java version "\(.*\)\.\(.*\)\..*"/\1\2/; 1q') -ge 17 ] && echo "version is at least 1.7, OK" || { echo "version is less than 1.7, NOT OK, exiting"; exit 1; }

# checks if file exists, exits if not
# @param $1 file full path
function file_exist {
	[ -f "$1" ] && echo "$1 FOUND, OK" || { echo "$1 NOT FOUND, exiting"; exit 1; }
}

# checks if directory exists, exits if not
# @param $1 directory full path
function dir_exist {
	[ -d "$1" ] && echo "$1 DIRECTORY FOUND, OK" || { echo "$1 DIRECTORY NOT FOUND, exiting"; exit 1; }
}

# check TITAN dependencies
[ ! -z "${TTCN3_DIR}" ] && echo "\$TTCN3_DIR is set to ${TTCN3_DIR}, OK" \
|| { echo "\$TTCN3_DIR is not set, NOT OK, exiting"; exit 1; }
dir_exist ${TTCN3_DIR}
file_exist ${TTCN3_DIR}/lib/libmctrjninative.so
[[ "${LD_LIBRARY_PATH}" == *"${TTCN3_DIR}/lib"* ]] && echo "\$TTCN3_DIR/lib is added to \$LD_LIBRARY_PATH=${LD_LIBRARY_PATH}, OK" \
|| { echo "\$TTCN3_DIR is NOT added to \$LD_LIBRARY_PATH=${LD_LIBRARY_PATH}, NOT OK, exiting"; exit 1; }

# make sure, that the demo is compiled, which is cleaned in make install
pushd ${TTCN3_DIR}/demo
make
popd
file_exist ${TTCN3_DIR}/demo/MyExample

# Check if HelloWorld demo binary is compiled in parallel mode: output of MyExample -v contains "(parallel mode)"
[ `${TTCN3_DIR}/demo/MyExample -v 2>&1 | grep "(parallel mode)" | wc -l` != 0 ] && echo "${TTCN3_DIR}/demo/MyExample is compiled in parallel mode, OK" \
|| { echo "${TTCN3_DIR}/demo/MyExample is compiled in single mode, NOT in parallel mode, NOT OK, exiting"; exit 1; }

# check JAR dependencies
file_exist ${JUNIT_FULL}
file_exist ${HAMCREST_FULL}

# run test (and build its dependencies if needed)
BASEDIR=$(dirname $0)
echo BASEDIR: $BASEDIR
ant \
-f $BASEDIR/build.xml \
-Djunit.full=${JUNIT_FULL} \
-Dhamcrest.full=${HAMCREST_FULL} \
TITAN_Executor_API_test

