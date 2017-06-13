#!/bin/sh

editcmd='/COMPILER_FLAGS/a\
SCRIPTFLAG=OneMakefileWithAllFilesB'

sed -e "$editcmd" <$1 >$2
