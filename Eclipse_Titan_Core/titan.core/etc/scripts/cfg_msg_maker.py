##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#   Delic, Adam
#
##############################################################################
header = "TITAN"
str = header + """
 ________    _____   ________     ____        __      _
(___  ___)  (_   _) (___  ___)   (    )      /  \    / )
    ) )       | |       ) )      / /\ \     / /\ \  / /
   ( (        | |      ( (      ( (__) )    ) ) ) ) ) )
    ) )       | |       ) )      )    (    ( ( ( ( ( (
   ( (       _| |__    ( (      /  /\  \   / /  \ \/ /
   /__\     /_____(    /__\    /__(  )__\ (_/    \__/
"""
encoded = ""
for c in str:
  code = ord(c)
  for i in range(7,-1,-1):
    if (code & (1<<i)): ch = "\t"
    else: ch = " "
    encoded += ch
cfgfile = open('message.cfg', 'w')
cfgfile.write("[DEFINE]\n");
cfgfile.write("// include this cfg file or copy paste the following whitespaces into your main cfg file\n");
cfgfile.write(encoded);
cfgfile.write("\n// end of message\n");
cfgfile.close();
