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
#include "Graph.hh"
#include "Symbol.hh"
#include "Grammar.hh"
#include "../common/memory.h"
#include <stdio.h>

void graph_use(Grammar* grammar)
{
  expstring_t out=mcopystr("digraph G {\n");
  size_t n=grammar->get_nof_symbols();
  for(size_t i=0; i<n; i++) {
    Symbol *lhs=grammar->get_symbol_byIndex(i);
    const SymbolSet& ss=lhs->get_refs();
    size_t n2=ss.get_nof_ss();
    if(n2) {
      out=mputprintf(out, "%s -> { ", lhs->get_id_dot().c_str());
      for(size_t i2=0; i2<n2; i2++) {
        Symbol *s=ss.get_s_byIndex(i2);
        if(!s->get_is_terminal())
          out=mputprintf(out, "%s; ", s->get_id_dot().c_str());
      }
      out=mputstr(out, "}\n");
    }
  }
  out=mputstr(out, "}\n");
  puts(out);
  Free(out);
}

