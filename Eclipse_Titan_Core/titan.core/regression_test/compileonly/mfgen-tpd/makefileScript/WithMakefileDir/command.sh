#!/bin/sh

editcmd='/COMPILER_FLAGS/a\
SCRIPTFLAG=WithMakefileDir'

sed -e "$editcmd" <$1 >$2
