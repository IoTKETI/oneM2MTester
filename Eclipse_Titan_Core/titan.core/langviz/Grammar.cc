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
#include "Grammar.hh"
#include "Rule.hh"
#include "Iterator.hh"
#include "Symbol.hh"

// =================================
// ===== Grammar
// =================================

Grammar::Grammar(const Grammar& p)
  : Node(p)
{
  FATAL_ERROR("Grammar::Grammar()");
}

Grammar::Grammar()
  : max_dist(-1)
{
}

Grammar::~Grammar()
{
  for(size_t i=0; i<gs.size(); i++)
    delete gs.get_nth_elem(i);
  gs.clear();
  as.clear();
  ss.destruct_ss();
}

Grammar* Grammar::clone() const
{
  FATAL_ERROR("Grammar::clone()");
}

Symbol* Grammar::get_symbol(const string& id)
{
  if(ss.has_s_withId(id)) return ss.get_s_byId(id);
  Symbol *s=new Symbol(id);
  if(id[0]=='\'' || id[0]=='"' || id=="error") s->set_is_terminal();
  ss.add_s(s);
  return s;
}

void Grammar::add_alias(const string& s1, const string& s2)
{
  if(!as.has_key(s1))
    as.add(s1, get_symbol(s2));
  get_symbol(s1)->set_is_terminal();
}

void Grammar::add_grouping(Grouping *p_grouping)
{
  const string& id=p_grouping->get_id();
  if(gs.has_key(id)) {
    gs[id]->steal_rules(p_grouping);
    delete p_grouping;
  }
  else gs.add(id, p_grouping);
}

Symbol* Grammar::get_alias(Symbol *p_symbol)
{
  const string& id=p_symbol->get_id();
  return as.has_key(id)?as[id]:p_symbol;
}

void Grammar::replace_aliases()
{
  startsymbol=get_alias(startsymbol);
  for(size_t i=0; i<gs.size(); i++)
    gs.get_nth_elem(i)->replace_aliases(this);
  for(size_t i=0; i<as.size(); i++)
    ss.destruct_symbol(as.get_nth_key(i));
}

Grouping* Grammar::get_grouping_bySymbol(Symbol *p_symbol)
{
  const string& id=p_symbol->get_id();
  if(!gs.has_key(id)) FATAL_ERROR("Grammar::get_grouping()");
  return gs[id];
}

void Grammar::compute_refs()
{
  ItRefBuild it;
  it.visitGrammar(this);
}

void Grammar::compute_can_be_empty()
{
  bool changed;
  do {
    changed=false;
    size_t ng=get_nof_groupings();
    for(size_t ig=0; ig<ng; ig++) {
      Grouping *grp=get_grouping_byIndex(ig);
      Symbol *lhs=grp->get_lhs();
      if(lhs->get_can_be_empty()) continue;
      size_t nr=grp->get_nof_rules();
      for(size_t ir=0; ir<nr; ir++) {
        if(lhs->get_can_be_empty()) continue;
        Rule *rule=grp->get_rule_byIndex(ir);
        size_t ns=rule->get_nof_ss();
        bool b=true;
        for(size_t is=0; is<ns; is++)
          if(!rule->get_s_byIndex(is)->get_can_be_empty()) {b=false; break;}
        if(b) {lhs->set_can_be_empty(); changed=true;}
      } // for ir
    } // for ig
  } while(changed);
}

void Grammar::compute_dists()
{
  SymbolSet *prev_set, *next_set;
  prev_set=new SymbolSet();
  next_set=new SymbolSet();
  int dist=0;
  if(startsymbol) {
    prev_set->add_s(startsymbol);
    startsymbol->set_dist(dist);
  }
  size_t n=prev_set->get_nof_ss();
  while(n) {
    dist++;
    for(size_t i=0; i<n; i++)
      next_set->add_ss(prev_set->get_s_byIndex(i)->get_refs());
    n=next_set->get_nof_ss();
    SymbolSet *tmp_set=new SymbolSet();
    for(size_t i=0; i<n; i++) {
      Symbol *s=next_set->get_s_byIndex(i);
      if(s->get_dist()==-1) s->set_dist(dist);
      else tmp_set->add_s(s);
    }
    next_set->remove_ss(*tmp_set);
    delete prev_set;
    prev_set=next_set;
    next_set=tmp_set;
    next_set->remove_all();
    n=prev_set->get_nof_ss();
  } // while n
  delete prev_set;
  delete next_set;
  max_dist=dist;
}

void Grammar::compute_all()
{
  compute_refs();
  compute_can_be_empty();
  compute_dists();
}
