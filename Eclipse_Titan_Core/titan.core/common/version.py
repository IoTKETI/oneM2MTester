##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#   Pandi, Krisztian
#
##############################################################################
import datetime
 
from tempfile import mkstemp
from shutil import move
from os import remove, close

from subprocess import call
from sys import exit

def getnumberfromfileline(line):
    lineasstring = str(line);
    numberfromfile = lineasstring[-3:];
    number = int(numberfromfile) + 1;
    print number;
    if number >= 99:
	print 'Number is over the limit: >=99. File is not modified!'
        exit();
    return number
    

def replace(file_path):
    #Create temp file
    fh, abs_path = mkstemp()
    new_file = open(abs_path,'w')
    old_file = open(file_path)
    for line in old_file:
		if '#define TTCN3_PATCHLEVEL' in line:
				newline = str('#define TTCN3_PATCHLEVEL ') + str(getnumberfromfileline(line)) + str('\n');
				new_file.write(newline);
		elif '#define TTCN3_VERSION 30' in line:
		                number = getnumberfromfileline(line);
				if number <= 9:
					newline = str('#define TTCN3_VERSION 302') +'0' + str(number) + str('\n');
				else:
					newline = str('#define TTCN3_VERSION 302') + str(number) + str('\n');
				new_file.write(newline);
		else:
			new_file.write(line)
    #close temp file
    new_file.close()
    close(fh)
    old_file.close()
    #Remove original file
    remove(file_path)
    #Move new file
    move(abs_path, file_path)
 
#( d.isoweekday() in range(1, 6)
#d = datetime.datetime.now();
#if d.isoweekday() == 2 or d.isoweekday() == 4 :
replace ("version.h");  
#call(["git", "commit", "-m 'TTCN3_PATCHLEVEL update'" ,"version.h"]);
#	call(["git", "push"]);

