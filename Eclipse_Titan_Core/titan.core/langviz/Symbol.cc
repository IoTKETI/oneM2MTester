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
#include "Symbol.hh"
#include "Grammar.hh"
#include "../common/memory.h"

// =================================
// ===== Symbol
// =================================

Symbol::Symbol(const Symbol& p)
  : Node(p), id(p.id), id_dot(p.id),
    is_terminal(p.is_terminal), is_recursive(p.is_recursive),
    can_be_empty(p.can_be_empty), dist(p.dist), weight(p.weight)
{
}

Symbol::Symbol(const string& p_id)
  : id(p_id), is_terminal(false), is_recursive(false), can_be_empty(false),
    dist(-1), weight(-1)
{
}

const string& Symbol::get_id_dot()
{
  if(id_dot.empty()) {
    string s=id;
    for(size_t pos=s.find('\\', 0); pos<s.size();
        pos=s.find('\\', pos)) {
      s.replace(pos, 1, "\\\\");
      pos+=2;
    }
    for(size_t pos=s.find('"', 0); pos<s.size();
        pos=s.find('"', pos)) {
      s.replace(pos, 1, "\\\"");
      pos+=2;
    }
    id_dot="\"";
    id_dot+=s;
    if(can_be_empty) id_dot+="?";
    if(is_recursive) id_dot+="*";

    {
      expstring_t tmp_s=mprintf(" (%d)", dist);
      id_dot+=tmp_s;
      Free(tmp_s);
    }

    id_dot+="\"";
  }
  return id_dot;
}

int Symbol::get_weight()
{
  if(weight>=0) return weight;
  if(is_terminal) return weight=0;
  size_t n=refs.get_nof_ss();
  for(size_t i=0; i<n; i++) {
    Symbol *s=refs.get_s_byIndex(i);
    if(s->dist>dist) refs_weight.add_s(s);
    
  }
}

// =================================
// ===== Symbols
// =================================

Symbols::Symbols(const Symbols& p)
  : Node(p)
{
  for(size_t i=0; i<p.ss.size(); i++)
    add_s(p.ss[i]);
}

Symbols::~Symbols()
{
  ss.clear();
}

void Symbols::add_s(Symbol *p_s)
{
  if(!p_s)
    FATAL_ERROR("NULL parameter: Symbols::add_s()");
  ss.add(p_s);
}

void Symbols::replace_aliases(Grammar *grammar)
{
  for(size_t i=0; i<ss.size(); i++)
    ss[i]=grammar->get_alias(ss[i]);
}

// =================================
// ===== SymbolMap
// =================================

SymbolMap::SymbolMap(const SymbolMap& p)
  : Node(p)
{
  for(size_t i=0; i<p.ss.size(); i++) {
    Symbol *p_s=p.ss.get_nth_elem(i);
    ss.add(p_s->get_id(), p_s);
  }
}

SymbolMap::~SymbolMap()
{
  ss.clear();
}

void SymbolMap::add_s(Symbol *p_s)
{
  if(!p_s)
    FATAL_ERROR("NULL parameter: SymbolMap::add_s()");
  ss.add(p_s->get_id(), p_s);
}

void SymbolMap::destruct_ss()
{
  for(size_t i=0; i<ss.size(); i++)
    delete ss.get_nth_elem(i);
}

void SymbolMap::destruct_symbol(const string& p_id)
{
  delete ss[p_id];
  ss.erase(p_id);
}

// =================================
// ===== SymbolSet
// =================================

SymbolSet::SymbolSet(const SymbolSet& p)
{
  FATAL_ERROR("SymbolSet::SymbolSet");
}

SymbolSet::~SymbolSet()
{
  ss.clear();
}

void SymbolSet::add_s(Symbol *p_s)
{
  if(ss.has_key(p_s)) return;
  ss.add(p_s, 0);
}

void SymbolSet::add_ss(const SymbolSet& p_ss)
{
  size_t n=p_ss.ss.size();
  for(size_t i=0; i<n; i++) {
    Symbol *s=p_ss.ss.get_nth_key(i);
    if(!ss.has_key(s)) ss.add(s, 0);
  }
}

void SymbolSet::remove_s(Symbol *p_s)
{
  if(ss.has_key(p_s)) ss.erase(p_s);
}

void SymbolSet::remove_ss(const SymbolSet& p_ss)
{
  size_t n=p_ss.ss.size();
  for(size_t i=0; i<n; i++) {
    Symbol *s=p_ss.ss.get_nth_key(i);
    if(ss.has_key(s)) ss.erase(s);
  }
}

void SymbolSet::remove_all()
{
  ss.clear();
}

