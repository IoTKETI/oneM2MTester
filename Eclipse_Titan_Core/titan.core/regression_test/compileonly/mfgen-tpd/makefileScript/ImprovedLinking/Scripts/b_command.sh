#!/bin/sh

editcmd='/COMPILER_FLAGS/a\
SCRIPTFLAG=ImprovedLinkingB'

sed -e "$editcmd" <$1 >$2
