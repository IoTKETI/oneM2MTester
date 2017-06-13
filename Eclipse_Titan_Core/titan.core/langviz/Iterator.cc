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
#include "Iterator.hh"
#include "Grammar.hh"
#include "Rule.hh"
#include "Symbol.hh"

// =================================
// ===== Iterator
// =================================

void Iterator::visitGrammar(Grammar *p_grammar)
{
  grammar=p_grammar;
  actionGrammar();
  size_t n=grammar->get_nof_groupings();
  for(size_t i=0; i<n; i++)
    visitGrouping(grammar->get_grouping_byIndex(i));
}

void Iterator::visitGrouping(Grouping *p_grouping)
{
  grouping=p_grouping;
  lhs=p_grouping->get_lhs();
  actionGrouping();
  size_t n=grouping->get_nof_rules();
  for(size_t i=0; i<n; i++)
    visitRule(grouping->get_rule_byIndex(i));
}

void Iterator::visitRule(Rule *p_rule)
{
  rule=p_rule;
  actionRule();
  size_t n=rule->get_nof_ss();
  for(size_t i=0; i<n; i++)
    visitSymbol(rule->get_s_byIndex(i));
}

void Iterator::visitSymbol(Symbol *p_symbol)
{
  symbol=p_symbol;
  actionSymbol();
}

// =================================
// ===== ItRefBuild
// =================================

void ItRefBuild::actionSymbol()
{
  if(symbol==lhs)
    symbol->set_is_recursive();
  else {
    symbol->add_refd_by(lhs);
    lhs->add_refs(symbol);
  }
}
