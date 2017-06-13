#!/bin/sh

editcmd='/COMPILER_FLAGS/a\
SCRIPTFLAG=OneMakefileWithAllFilesA'

sed -e "$editcmd" <$1 >$2
