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
#ifndef _langviz_Grammar_HH
#define _langviz_Grammar_HH

#include "Node.hh"
#include "Symbol.hh"
#include "../compiler2/string.hh"
#include "../compiler2/map.hh"

class Grouping;

/**
 * The grammar...
 */
class Grammar : public Node {
protected:
  SymbolMap ss; /**< every symbol */
  map<string, Symbol> as; /**< aliases */
  map<string, Grouping> gs; /**< groupings (every rule of the grammar) */
  Symbol *startsymbol;
  int max_dist;

  Grammar(const Grammar& p);
public:
  Grammar();
  virtual ~Grammar();
  virtual Grammar* clone() const;
  Symbol* get_symbol(const string& id);
  void add_alias(const string& s1, const string& s2);
  void add_grouping(Grouping *p_grouping);
  void set_startsymbol(Symbol *p_symbol) {startsymbol=p_symbol;}
  void set_firstsymbol(Symbol *p_symbol)
    {if(!startsymbol) startsymbol=p_symbol;}
  Symbol* get_alias(Symbol *p_symbol);
  void replace_aliases();
  size_t get_nof_groupings() {return gs.size();}
  Grouping* get_grouping_byIndex(size_t p_i) {return gs.get_nth_elem(p_i);}
  Grouping* get_grouping_bySymbol(Symbol *p_symbol);
  size_t get_nof_symbols() {return ss.get_nof_ss();}
  Symbol* get_symbol_byIndex(size_t p_i) {return ss.get_s_byIndex(p_i);}
 protected:
  void compute_refs();
  void compute_can_be_empty();
  void compute_dists();
 public:
  void compute_all();
};

#endif // _langviz_Grammar_HH
