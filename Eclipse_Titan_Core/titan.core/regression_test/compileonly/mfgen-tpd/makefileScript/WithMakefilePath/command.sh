#!/bin/sh

editcmd='/COMPILER_FLAGS/a\
SCRIPTFLAG=WithMakefilePath'

sed -e "$editcmd" <$1 >$2
