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
#ifndef _langviz_Symbol_HH
#define _langviz_Symbol_HH

#include "Node.hh"
#include "../compiler2/string.hh"
#include "../compiler2/vector.hh"
#include "../compiler2/map.hh"

class Symbol;
class Grammar;

/**
 * Unique set of symbols.
 */
class SymbolSet : public Node {
protected:
  map<Symbol*, void> ss;

  SymbolSet(const SymbolSet& p);
public:
  SymbolSet() {}
  virtual ~SymbolSet();
  virtual SymbolSet* clone() const {return new SymbolSet(*this);}
  void add_s(Symbol *p_s);
  void add_ss(const SymbolSet& p_ss);
  void remove_s(Symbol *p_s);
  void remove_ss(const SymbolSet& p_ss);
  void remove_all();
  size_t get_nof_ss() const {return ss.size();}
  Symbol* get_s_byIndex(size_t p_i) const {return ss.get_nth_key(p_i);}
};

/**
 * Terminal symbols (tokens) and nonterminal symbols.
 */
class Symbol : public Node {
protected:
  string id;
  string id_dot; /**< this id is used in dot file */
  SymbolSet refd_by; /**< this symbol is referenced directly by these
                          symbols */
  SymbolSet refs; /**< these symbols are referenced directly by this
                       symbol */
  SymbolSet refd_from; /**< this symbol is referenced directly or
                            indirectly from these symbols */
  SymbolSet refs_weight; /**< these NONTERMINAL symbols are referenced
                              directly or indirectly by this symbol;
                              the dist of these symbols are greater
                              than dist of this */
  bool is_terminal; /**< true if is terminal symbol */
  bool is_recursive;
  bool can_be_empty;
  int dist; /**< shortest distance from the start symbol; -1:
                 unreachable */
  int weight; /**< 1 + number of nonterminal symbols that can be
                   reached from this one, and have greater dist than
                   this */

  Symbol(const Symbol& p);
public:
  Symbol(const string& p_id);
  virtual ~Symbol() {}
  virtual Symbol* clone() const {return new Symbol(*this);}
  bool get_is_terminal() {return is_terminal;}
  void set_is_terminal() {is_terminal=true;}
  const string& get_id() const {return id;}
  const string& get_id_dot();
  void add_refd_by(Symbol* p_symbol) {refd_by.add_s(p_symbol);}
  void add_refs(Symbol* p_symbol) {refs.add_s(p_symbol);}
  const SymbolSet& get_refd_by() {return refd_by;}
  const SymbolSet& get_refs() {return refs;}
  void set_is_recursive() {is_recursive=true;}
  void set_can_be_empty() {can_be_empty=true;}
  bool get_can_be_empty() {return can_be_empty;}
  void set_dist(int p_i) {if(dist==-1) dist=p_i;}
  int get_dist() {return dist;}
  int get_weight();
};

/**
 * Ordered list of symbols.
 */
class Symbols : public Node {
protected:
  vector<Symbol> ss; /**< symbols */

  Symbols(const Symbols& p);
public:
  Symbols() {}
  virtual ~Symbols();
  virtual Symbols* clone() const {return new Symbols(*this);}
  void add_s(Symbol *p_s);
  size_t get_nof_ss() const {return ss.size();}
  const Symbol* get_s_byIndex(size_t p_i) const {return ss[p_i];}
  Symbol*& get_s_byIndex(size_t p_i) {return ss[p_i];}
  void replace_aliases(Grammar *grammar);
};

/**
 * Unique map of symbols.
 */
class SymbolMap : public Node {
protected:
  map<string, Symbol> ss;

  SymbolMap(const SymbolMap& p);
public:
  SymbolMap() {}
  virtual ~SymbolMap();
  virtual SymbolMap* clone() const {return new SymbolMap(*this);}
  void add_s(Symbol *p_s);
  size_t get_nof_ss() const {return ss.size();}
  const Symbol* get_s_byIndex(size_t p_i) const {return ss.get_nth_elem(p_i);}
  Symbol*& get_s_byIndex(size_t p_i) {return ss.get_nth_elem(p_i);}
  bool has_s_withId(const string& p_id) const {return ss.has_key(p_id);}
  const Symbol* get_s_byId(const string& p_id) const {return ss[p_id];}
  Symbol*& get_s_byId(const string& p_id) {return ss[p_id];}
  /** Each Symbol instance is owned by the Grammar's SymbolMap. The
   *  only place from where you should destruct them. */
  void destruct_ss();
  /** Used by Grammar after replacing aliases */
  void destruct_symbol(const string& p_id);
};

#endif // _langviz_Symbol_HH
