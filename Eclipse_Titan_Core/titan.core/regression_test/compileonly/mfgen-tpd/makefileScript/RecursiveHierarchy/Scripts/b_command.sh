#!/bin/sh

editcmd='/COMPILER_FLAGS/a\
SCRIPTFLAG=RecursiveHierarchyB'

sed -e "$editcmd" <$1 >$2
