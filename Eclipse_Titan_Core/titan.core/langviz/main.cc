/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Forstner, Matyas
 *
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "../common/memory.h"
#include "error.h"
#include "Node.hh"
#include "Grammar.hh"
#include "Iterator.hh"
#include "Graph.hh"

extern int bison_parse_file(const char* filename);

Grammar *grammar=0;

void show_usage()
{
  fprintf(stderr, "Usage: %s bison_filename\n", argv0);
}

int main(int argc, char *argv[])
{
  argv0 = argv[0];

  if(argc!=2) {
    show_usage();
    return 1;
  }

  char *infilename=argv[1];
  grammar=new Grammar();

  bison_parse_file(infilename);

  grammar->compute_all();
  graph_use(grammar);

  delete grammar;
  Node::chk_counter();
  check_mem_leak(argv0); 
}
