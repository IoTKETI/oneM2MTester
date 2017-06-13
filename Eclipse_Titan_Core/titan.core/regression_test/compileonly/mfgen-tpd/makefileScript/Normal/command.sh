#!/bin/sh

editcmd='/COMPILER_FLAGS/a\
SCRIPTFLAG=Normal'

sed -e "$editcmd" <$1 >$2
