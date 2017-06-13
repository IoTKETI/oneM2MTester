/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Baranyi, Botond
 *   Delic, Adam
 *   Forstner, Matyas
 *   Raduly, Csaba
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#ifndef _Ttcn_PatternString_HH
#define _Ttcn_PatternString_HH

#include "../vector.hh"
#include "../Setting.hh"
#include "AST_ttcn3.hh"

#include "../Value.hh"

namespace Ttcn {

  class Module;
  using namespace Common;

  /**
   * The instances of this class can represent a TTCN pattern
   * string. It can built up from different kinds of elements:
   * "regular" RE-elements and "reference" elements ({reference} and
   * \\N{reference}).
   */
  class PatternString : public Node {
  public:
    enum pstr_type_t {
      CSTR_PATTERN,
      USTR_PATTERN
    };
  private:
    Scope *my_scope;
    struct ps_elem_t;
    vector<ps_elem_t> elems;
    PatternString(const PatternString& p);
    /** returns the last element if it contains a regular string or NULL
     * otherwise */
    ps_elem_t *get_last_elem() const;

    Value* cstr_value;

    pstr_type_t pattern_type;
    
    bool nocase;

  public:
    PatternString(): Node(), my_scope(0), cstr_value(0),
      pattern_type(CSTR_PATTERN), nocase(false) { }
    virtual ~PatternString();
    virtual PatternString* clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    void set_code_section(GovernedSimple::code_section_t p_code_section);
    void addChar(char c);
    void addString(const char *p_str);
    void addString(const string& p_str);
    void addStringUSI(char **usi_str, const size_t size);
    void addRef(Ttcn::Reference *p_ref, boolean N);
    string get_full_str() const;

    void set_pattern_type(pstr_type_t p_type);
    pstr_type_t get_pattern_type() const;

    /** Returns whether the pattern contains embedded references */
    bool has_refs() const;
    /** Checks the embedded referenced values */
    void chk_refs(Type::expected_value_t expected_value=Type::EXPECTED_DYNAMIC_VALUE);
    void chk_recursions(ReferenceChain& refch);
    /** Checks the pattern by translating it to POSIX regexp */
    void chk_pattern();
    bool chk_self_ref(Common::Assignment *lhs);
    /** Joins adjacent string components into one for more efficient code
     * generation. */
    void join_strings();
    /** Temporary hack... */
    string create_charstring_literals(Common::Module *p_mod, string& preamble);
    virtual void dump(unsigned level) const;

    /** Called by Value::get_value_refd_last() */
    Value* get_value();
    
    /** Converts this string pattern into a JSON schema string pattern. */
    char* convert_to_json();
    
    void set_nocase(bool p_nocase) { nocase = p_nocase; }
    
    bool get_nocase() const { return nocase; }
  };

} // namespace Ttcn

#endif // _Ttcn_PatternString_HH
