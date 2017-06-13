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
#include "Rule.hh"
#include "Grammar.hh"
#include "Symbol.hh"

// =================================
// ===== Rule
// =================================

Rule::Rule(const Rule& p)
  : Node(p)
{
  rhs=p.rhs->clone();
}

Rule::Rule(Symbols *p_rhs)
  : rhs(p_rhs)
{
  if(!p_rhs)
    FATAL_ERROR("Rule::Rule()");
}

Rule::~Rule()
{
  delete rhs;
}

void Rule::replace_aliases(Grammar *grammar)
{
  rhs->replace_aliases(grammar);
}

// =================================
// ===== Rules
// =================================

Rules::Rules(const Rules& p)
  : Node(p)
{
  for(size_t i=0; i<p.rs.size(); i++)
    add_r(p.rs[i]->clone());
}

Rules::~Rules()
{
  for(size_t i=0; i<rs.size(); i++)
    delete rs[i];
  rs.clear();
}

void Rules::add_r(Rule *p_r)
{
  if(!p_r)
    FATAL_ERROR("NULL parameter: Rules::add_r()");
  rs.add(p_r);
}

void Rules::steal_rules(Rules *p_other)
{
  for(size_t i=0; i<p_other->rs.size(); i++)
    rs.add(p_other->rs[i]);
  p_other->rs.clear();
}

void Rules::replace_aliases(Grammar *grammar)
{
  for(size_t i=0; i<rs.size(); i++)
    rs[i]->replace_aliases(grammar);
}

// =================================
// ===== Grouping
// =================================

Grouping::Grouping(const Grouping& p)
  : Node(p)
{
  lhs=p.lhs;
  rhss=p.rhss->clone();
}

Grouping::Grouping(Symbol *p_lhs, Rules *p_rhss)
  : lhs(p_lhs), rhss(p_rhss)
{
  if(!p_lhs || !p_rhss)
    FATAL_ERROR("Grouping::Grouping()");
}

Grouping::~Grouping()
{
  delete rhss;
}

void Grouping::steal_rules(Grouping *p_other)
{
  rhss->steal_rules(p_other->rhss);
}

void Grouping::replace_aliases(Grammar *grammar)
{
  lhs=grammar->get_alias(lhs);
  rhss->replace_aliases(grammar);
}
