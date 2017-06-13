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
#ifndef _langviz_Iterator_HH
#define _langviz_Iterator_HH

class Grammar;
class Grouping;
class Rule;
class Symbol;

class Iterator {
protected:
  Grammar *grammar;
  Grouping *grouping;
  Symbol *lhs;
  Rule *rule;
  Symbol *symbol;
  
public:
  virtual ~Iterator() {}
  virtual void visitGrammar(Grammar *p_grammar);
  virtual void visitGrouping(Grouping *p_grouping);
  virtual void visitRule(Rule *p_rule);
  virtual void visitSymbol(Symbol *p_symbol);
  virtual void actionGrammar() {}
  virtual void actionGrouping() {}
  virtual void actionRule() {}
  virtual void actionSymbol() {}
};

/**
 * Used to build the references/referenced by structure of symbols
 */
class ItRefBuild : public Iterator {
  virtual void actionSymbol();
};

#endif // _langviz_Iterator_HH
