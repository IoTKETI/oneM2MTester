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
 *   Beres, Szabolcs
 *   Cserveni, Akos
 *   Forstner, Matyas
 *   Gecse, Roland
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef _Common_Valuestuff_HH
#define _Common_Valuestuff_HH

#include "Setting.hh"
#include "Int.hh"
#include "ttcn3/AST_ttcn3.hh"

class ustring;

namespace Asn {
  class Block;
}

namespace Common {

  // not defined here
  class Value;
  using Asn::Block;

  /**
   * \addtogroup AST_Value
   *
   * @{
   */

  /**
   * Class to represent an IndexedValue.
   */
  class IndexedValue : public Node, public Location {
  private:
    Ttcn::FieldOrArrayRef *index;
    Value *value;
    IndexedValue(const IndexedValue& p);
  public:
    IndexedValue(Ttcn::FieldOrArrayRef *p_index, Value *p_value);
    virtual ~IndexedValue();
    virtual IndexedValue* clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    void set_code_section(GovernedSimple::code_section_t p_code_section);
    inline Value *get_index() const { return index->get_val(); }
    inline Value *get_value() const { return value; }
    Value *steal_value();
    virtual void dump(unsigned level) const;
    virtual bool is_unfoldable(ReferenceChain *refch=0,
    		Type::expected_value_t exp_val=Type::EXPECTED_DYNAMIC_VALUE);

  };

  /**
   * Class to represent ValueList.
   */
  class Values : public Node {
  private:
    bool indexed;
    union {
      vector<Value> *vs;
      vector<IndexedValue> *ivs;
    };
    Values(const Values& p);
  public:
    Values(bool p_indexed = false);
    virtual ~Values();
    virtual Values *clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    void add_v(Value *p_v);
    void add_iv(IndexedValue *p_iv);
    size_t get_nof_vs() const;
    size_t get_nof_ivs() const;
    Value *get_v_byIndex(size_t p_i) const;
    IndexedValue *get_iv_byIndex(size_t p_i) const;
    Value *steal_v_byIndex(size_t p_i);
    IndexedValue *steal_iv_byIndex(size_t p_i);
    virtual void dump(unsigned p_level) const;
    inline bool is_indexed() const { return indexed; }
  };

  /**
   * Class to represent a NamedValue.
   */
  class NamedValue : public Node, public Location {
  private:
    Identifier *name;
    Value *value;
    NamedValue(const NamedValue& p);
  public:
    NamedValue(Identifier *p_name, Value *p_value);
    virtual ~NamedValue();
    virtual NamedValue* clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    const Identifier& get_name() const { return *name; }
    Value *get_value() const { return value; }
    Value *steal_value();
    virtual void dump(unsigned level) const;
  };

  /**
   * Class to represent NamedValueList.
   */
  class NamedValues : public Node {
  private:
    /** named values */
    vector<NamedValue> nvs_v;
    /** Stores the first occurence of NamedValue with id. The string
     * parameter refers to the id of the nv. */
    map<string, NamedValue> nvs_m;
    bool checked;
    NamedValues(const NamedValues& p);
  public:
    NamedValues() : Node(), checked(false) { }
    virtual ~NamedValues();
    virtual NamedValues *clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    void add_nv(NamedValue *p_nv);
    /** Returns the vector's size! */
    size_t get_nof_nvs() const { return nvs_v.size(); }
    NamedValue *get_nv_byIndex(const size_t p_i) const { return nvs_v[p_i]; }
    bool has_nv_withName(const Identifier& p_name);
    NamedValue *get_nv_byName(const Identifier& p_name);
    void chk_dupl_id(bool report_errors = true);
    virtual void dump(unsigned level) const;
  };

  /**
   * Object identifier component.
   */
  class OID_comp : public Node, public Location {
  public:
    /** Which form was used. */
    enum formtype_t {
      NAMEFORM, /**< NameForm */
      NUMBERFORM, /**< NumberForm */
      NAMEANDNUMBERFORM, /**< NameAndNumberForm */
      DEFDVALUE, /**< DefinedValue */
      VARIABLE /**< Variable */
    };
    /** Which state the checking is in. */
    enum oidstate_t {
      START,   /**< at the beginning */
      ITU,     /**< after itu-t */
      ISO,     /**< after iso */
      JOINT,   /**< after joint-iso-itu-t */
      ITU_REC, /**< after itu-t recommendation */
      LATER    /**< later anywhere */
    };
  private:
    struct nameform_t {
      const char *name;
      unsigned int value;
    };
    static const nameform_t names_root[], names_itu[], names_iso[],
      names_joint[];
  private:
    formtype_t formtype;
    /** OID, ROID or INTEGER */
    Value *defdval;
    /** maybe a reference to an INTEGER or R/OID (if NAMEFORM) */
    Identifier *name;
    /** Integer (or reference to an integer) */
    Value *number;
    /** Integer variable (cannot be calculated in compile-time) */
    Value *var;

    OID_comp(const OID_comp& p);
  public:
    OID_comp(Identifier *p_name, Value *p_number);
    OID_comp(Value *p_defdval);
    virtual ~OID_comp();
    virtual OID_comp *clone() const;
    /** Checking function for OID components */
    void chk_OID(ReferenceChain& refch, Value *parent, size_t index,
      oidstate_t& state);
    /** checking function for ROID components */
    void chk_ROID(ReferenceChain& refch, size_t index);
    /** Returns whether the component contains any error. */
    bool has_error();
    /** Appends the integer value of number or the components of the referenced
     * OID/ROID value to \a comps. */
    void get_comps(vector<string>& comps);
    
    /** Returns true, if the component's value is unknown at compile-time */
    bool is_variable();
  private:
    /** Detects whether the identifier in \a name is a valid name form in
     * state \a state. If yes the equivalent non-negative numeric value is
     * returned and \a state is updated. Otherwise -1 is returned and
     * \a state remains unchanged. */
    Int detect_nameform(oidstate_t& state);
    /** Checks the defined value in \a defdval in an OID component and
     * updates \a state accordingly. */
    void chk_defdvalue_OID(ReferenceChain& refch, oidstate_t& state);
    /** Checks the NumberForm (or the number part of NameAndNumberForm) in an
     * OID component and updates \a state accordingly. */
    void chk_numberform_OID(oidstate_t& state);
    /** Checks the NameAndNumberForm: checks the \a number, updates \a state
     * and verifies whether the name is in accordance with \a name and the
     * initial \a state. */
    void chk_nameandnumberform(oidstate_t& state);
    /** Returns whether \a p_name is a correct name for \a p_number according
     * to the table \a p_names. */
    static bool is_valid_name_for_number(const string& p_name,
      const Int& p_number, const nameform_t *p_names);
    /** Returns the expected name(s) for number \a p_number according to table
     * \a p_names. Flag \a p_asn1 indicates the the ASN.1 or TTCN-3 notation
     * shall be used in the identifiers. */
    static string get_expected_name_for_number(const Int& p_number,
      bool p_asn1, const nameform_t *p_names);
    /** Checks the defined value in \a defdval in a ROID component. */
    void chk_defdvalue_ROID(ReferenceChain& refch);
    /** Checks the NumberForm (or the number part of NameAndNumberForm) in a
     * ROID component. */
    void chk_numberform_ROID();
  public:
    virtual void set_fullname(const string& p_fullname);
    void set_my_scope(Scope *p_scope);
    /** Appends the string representation of the OID component to \a str. */
    void append_stringRepr(string& str) const;
  };

  /**
   * Universal charstring element.
   */
  class CharsDefn : public Node, public Location {
  public:
    enum selector_t {
      CD_CSTRING,
      CD_QUADRUPLE,
      CD_TUPLE,
      CD_VALUE,
      CD_BLOCK
    } selector;
    bool checked;
  private:
    union {
      string *str; /**< cstring */
      struct {Int g, p, r, c;} quadruple; /**< quadruple */
      struct {Int c, r;} tuple; /**< tuple */
      Value *val; /**< defined val => referenced val */
      Block *block; /**< quadruple or tuple block */
    } u;

    CharsDefn(const CharsDefn& p);
  public:
    /** ctor for cstr */
    CharsDefn(string *p_str);
    /** ctor for val */
    CharsDefn(Value *p_val);
    /** ctor for quadruple */
    CharsDefn(Int p_g, Int p_p, Int p_r, Int p_c);
    /** ctor for tuple */
    CharsDefn(Int p_c, Int p_r);
    /** ctor for block */
    CharsDefn(Block *p_block);
    virtual ~CharsDefn();
    virtual CharsDefn* clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    selector_t get_selector() {return selector;}
    void chk();
    string get_string(ReferenceChain *refch=0);
    ustring get_ustring(ReferenceChain *refch=0);
    string get_iso2022string(ReferenceChain *refch=0);
    size_t get_len(ReferenceChain *refch);
    virtual void dump(unsigned level) const;
  private:
    void parse_block();
  };

  /**
   * Universal charstring elements.
   */
  class CharSyms : public Node, public Location {
  private:
    vector <CharsDefn> cds;
    enum selector_t {
      CS_UNDEF,
      CS_CHECKING,
      CS_CSTRING,
      CS_USTRING,
      CS_ISO2022STRING
    } selector;
    union {
      string *str;
      ustring *ustr;
    } u;
    CharSyms(const CharSyms& p);
  public:
    CharSyms();
    virtual ~CharSyms();
    virtual CharSyms* clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    /** \a p_cd can be 0 in case of error => drop it */
    void add_cd(CharsDefn *p_cd);
    void chk(bool tuple_enabled, bool quadruple_enabled);
    size_t get_nof_cds() const {return cds.size();}
    CharsDefn*& get_cd_byIndex(size_t p_i) {return cds[p_i];}
    string get_string(ReferenceChain *refch=0);
    ustring get_ustring(ReferenceChain *refch=0);
    string get_iso2022string(ReferenceChain *refch=0);
    size_t get_len(ReferenceChain *refch=0);
    virtual void dump(unsigned level) const;
  };

  /** @} end of AST_Value group */

} // namespace Common

#endif // _Common_Value_HH
