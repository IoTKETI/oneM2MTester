#!/bin/sh

editcmd='/COMPILER_FLAGS/a\
SCRIPTFLAG=ImprovedLinkingA'

sed -e "$editcmd" <$1 >$2
