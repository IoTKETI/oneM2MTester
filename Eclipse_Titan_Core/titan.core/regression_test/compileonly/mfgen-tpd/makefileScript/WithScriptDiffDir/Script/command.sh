#!/bin/sh

editcmd='/COMPILER_FLAGS/a\
SCRIPTFLAG=WithScriptDiffDir'

sed -e "$editcmd" <$1 >$2
