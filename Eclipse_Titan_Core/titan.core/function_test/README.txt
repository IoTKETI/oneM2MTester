###############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#
###############################################################################
// README to function test
All command shall be started from this folder (function_test)

1. How to start function test?
make

2. Ho to start function test for one subfolder?
Do it with the subfolder name,
e.g:
make Text_EncDec

Note: It is not the same as 
>cd Text_EncDec; ./run_test
because the environment is not prepared with the latter method.

3.How to clean all subfolder?
make clean

4. Which scripts will be executed in the function test?

function_test/Tools/SAtester for:
BER_EncDec
RAW_EncDec
Text_EncDec

function_test/Tools/SAtester.pl for:
Config_Parser
Semantic_Analyser
