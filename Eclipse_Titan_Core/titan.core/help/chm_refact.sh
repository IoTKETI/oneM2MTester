#!/bin/bash
###############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#   Forstner, Matyas
#
###############################################################################

cd chm

for i in info/*.html titan_index.html
do
  echo Processing $i
  if [[ $i = 'info/BNF.html' ]]
  then
    sed -e 's|<a href="http://www\.inet\.com/">|<a href="http://www.inet.com/" target="_blank">|g' $i >tmp
    mv tmp $i
  elif [[ $i = 'titan_index.html' ]]
  then
    sed -e '11,17d;20d' $i >tmp
    mv tmp $i
  else
    line21=$(sed -n -e '21p' $i)
    if [[ -n $(echo $line21 | grep -e 'ao\.jpg') ]] 
    then echo "$i is 1 (ao)"
      sed \
       -e '11,18d' \
       -e '21d' \
       $i >tmp
      mv tmp $i
    elif [[ -n $(echo $line21 | grep -e 'up\.jpg') ]] 
    then echo "$i is 2 (up)"
      sed \
       -e '11,17d' \
       -e '20d' \
       $i >tmp
      mv tmp $i
    elif [[ -n $(echo $line21 | grep -e '<table border="0" align="right" cellpadding="0" cellspacing="0">') ]] 
    then echo "$i is 3 (table)"
      sed \
       -e '11,20d' \
       -e '23d' \
       $i >tmp
      mv tmp $i
    else echo "$i is ? : $line21" 
    fi
  fi

  sed \
   -e 's|\(<a href="\(\.\./info/\)\?BNF\.html[^"]*"\) *target="_blank">|\1>|gI' \
   -e 's|\(<a href="[a-zA-Z0-9_]*\.html[^"]*"\) *target="_blank">|\1>|gI' \
   -e '
    :t
      /<[aA] [^>]*\.\.\/docs\// {
        /<\/[aA]>/!{
	  N;
	  bt
	}
	s/<[aA] [^>]*\.\.\/docs\/[^>]*>\([^<]*\)<\/[Aa]>/\1/g;
      }
   ' \
   $i >tmp
  mv tmp $i
done
