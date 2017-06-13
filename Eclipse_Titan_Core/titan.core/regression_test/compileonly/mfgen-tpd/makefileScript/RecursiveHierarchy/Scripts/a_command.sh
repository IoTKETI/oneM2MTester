#!/bin/sh

editcmd='/COMPILER_FLAGS/a\
SCRIPTFLAG=RecursiveHierarchyA'

sed -e "$editcmd" <$1 >$2
