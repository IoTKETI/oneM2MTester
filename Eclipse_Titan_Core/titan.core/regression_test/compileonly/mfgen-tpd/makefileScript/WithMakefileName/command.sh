#!/bin/sh

editcmd='/COMPILER_FLAGS/a\
SCRIPTFLAG=WithMakefileName'

sed -e "$editcmd" <$1 >$2
