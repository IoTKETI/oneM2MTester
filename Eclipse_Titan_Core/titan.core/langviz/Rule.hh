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
#ifndef _langviz_Rule_HH
#define _langviz_Rule_HH

#include "Symbol.hh"
#include "Node.hh"
#include "../compiler2/string.hh"

class Grammar;

/**
 * Represents a rule.
 */
class Rule : public Node {
protected:
  Symbols *rhs;

  Rule(const Rule& p);
public:
  Rule(Symbols *p_rhs);
  virtual ~Rule();
  virtual Rule* clone() const {return new Rule(*this);}
  size_t get_nof_ss() {return rhs->get_nof_ss();}
  Symbol* get_s_byIndex(size_t p_i) {return rhs->get_s_byIndex(p_i);}
  void replace_aliases(Grammar *grammar);
};

/**
 * Ordered list of rules.
 */
class Rules : public Node {
protected:
  vector<Rule> rs; /**< rules */

  Rules(const Rules& p);
public:
  Rules() {}
  virtual ~Rules();
  virtual Rules* clone() const {return new Rules(*this);}
  void add_r(Rule *p_r);
  size_t get_nof_rs() const {return rs.size();}
  const Rule* get_r_byIndex(size_t p_i) const {return rs[p_i];}
  Rule*& get_r_byIndex(size_t p_i) {return rs[p_i];}
  void steal_rules(Rules *p_other);
  void replace_aliases(Grammar *grammar);
};

/**
 * Represents a grouping (rules of a single nonterminal symbol).
 */
class Grouping : public Node {
protected:
  Symbol *lhs;
  Rules *rhss;

  Grouping(const Grouping& p);
public:
  Grouping(Symbol *p_lhs, Rules *p_rhss);
  virtual ~Grouping();
  virtual Grouping* clone() const {return new Grouping(*this);}
  const string& get_id() const {return lhs->get_id();}
  void steal_rules(Grouping *p_other);
  void replace_aliases(Grammar *grammar);
  Symbol* get_lhs() {return lhs;}
  size_t get_nof_rules() const {return rhss->get_nof_rs();}
  Rule* get_rule_byIndex(size_t p_i) {return rhss->get_r_byIndex(p_i);}
};

#endif // _langviz_Rule_HH

