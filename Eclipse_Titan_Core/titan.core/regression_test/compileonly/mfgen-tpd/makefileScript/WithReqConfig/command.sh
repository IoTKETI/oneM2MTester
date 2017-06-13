#!/bin/sh

editcmd='/COMPILER_FLAGS/a\
SCRIPTFLAG=WithReqConfig'

sed -e "$editcmd" <$1 >$2
