/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Delic, Adam
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#ifndef ATTRIBUTES_HH
#define ATTRIBUTES_HH

#include "../string.hh"
#include "../vector.hh"
#include "../Identifier.hh"
#include "../Setting.hh"
#include "AST_ttcn3.hh"

namespace Ttcn {

  using namespace Common;

  class Definition;
  class ImpMod;
  class Group;
  class Def_Template;
  class TemplateInstance;
  class Statement;

  /** Attribute qualifier (DefOrFieldRef). */
  class Qualifier: public FieldOrArrayRefs, public Location
  {
    /// Assignment disabled.
    Qualifier& operator=(const Qualifier& p);
  public:
    Qualifier(): FieldOrArrayRefs(), Location() { }
    Qualifier(const Qualifier& p): FieldOrArrayRefs(p), Location(p) { }
    virtual Qualifier* clone() const;
    /** return array indexes as "_0" identifiers TODO: remove */
    const Identifier* get_identifier(size_t p_index) const;
    /** use with get_identifier() TODO: remove */
    size_t get_nof_identifiers() const { return get_nof_refs(); }
    /** Returns a newly allocated Qualifier, without the first identifier.
     * Used by Common::Type::parse_raw to propagate an attribute
     * with a qualifier from a type (record,union,rec-of) to its component */
    Qualifier* get_qualifier_without_first_id() const;
    string get_stringRepr() const;
    void dump(unsigned level) const;
  };

  /** Attribute qualifiers, when there were multiple qualifiers
   *  in the DefOrFieldRefList.
   *
   *  E.g. the three-element list from below:
   *  @code variant (foo.f00, bar, baz[-]) "something" @endcode */
  class Qualifiers: public Node{
    /// Copy constructor disabled.
    Qualifiers(const Qualifiers& p);
    /// Assignment disabled.
    Qualifiers& operator=(const Qualifiers& p);
  private:
    vector<Qualifier> qualifiers;
  public:
    Qualifiers() : Node() { }
    ~Qualifiers();
    size_t get_nof_qualifiers() const { return qualifiers.size(); }
    void add_qualifier(Qualifier* p_qualifier);
    void delete_qualifier(size_t p_index);
    const Qualifier* get_qualifier(size_t p_index) const;
    virtual Qualifiers* clone() const;
    virtual void set_fullname(const string& p_fullname);
    bool has_qualifier(Qualifier* p_qualifier)const;
    void dump(unsigned level) const;
  };

  /**
   * The errouneous attribute spec. string is parsed into this AST node
   */
  class ErroneousAttributeSpec : public Node, public Location
  {
  public:
    enum indicator_t {
      I_BEFORE,
      I_VALUE,
      I_AFTER,
      I_INVALID
    };
  private:
    ErroneousAttributeSpec(const ErroneousAttributeSpec& p);
    /// Assignment disabled.
    ErroneousAttributeSpec& operator=(const ErroneousAttributeSpec& p);
    bool is_raw;
    bool has_all_keyword;
    indicator_t indicator;
    TemplateInstance* tmpl_inst;
    Type* type; // set by chk or NULL if tmpl_inst is invalid or omit
    Value* value; // set by chk or NULL if tmpl_inst is invalid or omit
    string first_genname; // used by generate_code_str()to avoid generating 
                          // multiple C++ objects for the same TTCN-3 constant value,
                          // after the first object all are only references
  public:
    ErroneousAttributeSpec(bool p_is_raw, indicator_t p_indicator, TemplateInstance* p_tmpl_inst, bool p_has_all_keyword);
    ~ErroneousAttributeSpec();
    ErroneousAttributeSpec *clone() const;
    void set_fullname(const string& p_fullname);
    void set_my_scope(Scope *p_scope);
    void dump(unsigned level) const;
    /** basic check, the qualifier of the field is not known here */
    void chk(bool in_update_stmt);
    indicator_t get_indicator() const { return indicator; }
    Type* get_type() const { return type; }
    bool get_is_raw() const { return is_raw; }
    bool get_is_omit() const;
    static const char* get_indicator_str(indicator_t i);
    char* generate_code_str(char *str, char *& def, string genname);
    char* generate_code_init_str(char *str, string genname);
    string get_typedescriptor_str();
    void chk_recursions(ReferenceChain& refch);
  };

  /**
   * helper to construct the tree of erroneous attributes
   */
  struct ErroneousValues {
    ErroneousAttributeSpec *before, *value, *after; // NULL if not specified
    string field_name; // qualifier string
    ErroneousValues(const string& p_field_name): before(0), value(0), after(0), field_name(p_field_name) {}
    char* generate_code_embedded_str(char *str, char *& def, string genname);
    char* generate_code_init_str(char *str, string genname);
    char* generate_code_embedded_str(char *str, char *& def, string genname, ErroneousAttributeSpec* attr_spec);
    char* generate_code_struct_str(char *str, string genname, int field_index);
    void chk_recursions(ReferenceChain& refch);
  };

  /**
   * helper to construct the tree of erroneous attributes
   */
  struct ErroneousDescriptor {
    int omit_before, omit_after; // -1 if not set
    string omit_before_name, omit_after_name; // qualifier string or empty
    map<size_t,ErroneousDescriptor> descr_m; // descriptors for the fields
    map<size_t,ErroneousValues> values_m; // erroneous values for the fields
    ErroneousDescriptor(const ErroneousDescriptor& p); // disabled
    ErroneousDescriptor& operator=(const ErroneousDescriptor& p); // disabled
  public:
    ErroneousDescriptor(): omit_before(-1), omit_after(-1) {}
    ~ErroneousDescriptor();
    char* generate_code_embedded_str(char *str, char *& def, string genname);
    char* generate_code_init_str(char *str, string genname);
    char* generate_code_struct_str(char *str, char *& def, string genname, int field_index);
    char* generate_code_str(char *str, char *& def, string genname);
    void chk_recursions(ReferenceChain& refch);
  };
  
  /**
   * helper to construct several trees of erroneous attributes
   * (contains the set of erroneous attributes set at initialization and the
   * sets of erroneous attributes set by '@update' statements)
   */
  class ErroneousDescriptors {
    /** Map of erroneous descriptors 
      * Key: pointer to the '@update' statement or NULL (for the erroneous
      * attributes specified at initialization)
      * Values not owned */
    map<Statement*, ErroneousDescriptor> descr_map;
    ErroneousDescriptors(const ErroneousDescriptor& p); // disabled
    ErroneousDescriptors& operator=(const ErroneousDescriptors& p); // disabled
  public:
    ErroneousDescriptors() {}
    ~ErroneousDescriptors();
    void add(Statement* p_update_statement, ErroneousDescriptor* p_descr);
    bool has_descr(Statement* p_update_statement);
    size_t get_descr_index(Statement* p_update_statement);
    char* generate_code_init_str(Statement* p_update_statement, char *str, string genname);
    char* generate_code_str(Statement* p_update_statement, char *str, char *& def, string genname);
    void chk_recursions(ReferenceChain& refch);
    static boolean can_have_err_attribs(Type* t);
  };

  /**
   * Contains erroneous attr. specs and the pointers to the qualifiers
   */
  class ErroneousAttributes : public Node
  {
  public:
    struct field_err_t {
      const Qualifier* qualifier; // not owned
      ErroneousAttributeSpec* err_attr; // not owned
      dynamic_array<size_t> subrefs_array; // used in chk()
      dynamic_array<Type*> type_array; // used in chk()
    };
  private:
    Type* type; // not owned
    vector<ErroneousAttributeSpec> spec_vec; // all elements owned
    dynamic_array<field_err_t> field_array;
    ErroneousDescriptor* err_descr_tree; // owned, constructed in chk()
    ErroneousAttributes(const ErroneousAttributes& p);
    /// Assignment disabled.
    ErroneousAttributes& operator=(const ErroneousAttributes& p);
    /** builds a tree of descriptors and err.values calling itself recursively, reports errors */
    ErroneousDescriptor* build_descr_tree(dynamic_array<field_err_t>& fld_array);
  public:
    ErroneousAttributes(Type* p_type);
    ~ErroneousAttributes();
    ErroneousAttributes *clone() const;
    void set_fullname(const string& p_fullname);
    void dump(unsigned level) const;

    void add_spec(ErroneousAttributeSpec* err_attr_spec);
    void add_pair(const Qualifier* qualifier, ErroneousAttributeSpec* err_attr_spec);
    /** check every field_err_t value */
    void chk();
    ErroneousDescriptor* get_err_descr() const { return err_descr_tree; }    
  };

  /**
   * Stores the attribute specification and its location
   */
  class AttributeSpec : public Node, public Location {
    /// Assignment disabled, even though copy constructor isn't
    AttributeSpec& operator=(const AttributeSpec& p);
  private:
    string spec; ///< The attribute specification (free text)
    /// Copy constructor, for clone() only
    AttributeSpec(const AttributeSpec& p)
      : Node(p), Location(p), spec(p.spec) { }
  public:
    AttributeSpec(const string& p_spec)
      : Node(), Location(), spec(p_spec) { }
    virtual AttributeSpec* clone() const;
    virtual void set_fullname(const string& p_fullname);
    const string& get_spec() const { return spec; }
    virtual void dump(unsigned level) const;
  };

  /**
   * Stores a single attribute
   */
  class SingleWithAttrib : public Node, public Location {
    /// Copy constructor disabled.
    SingleWithAttrib(const SingleWithAttrib& p);
    /// Assignment disabled.
    SingleWithAttrib& operator=(const SingleWithAttrib& p);
  public:
    enum attribtype_t{
          AT_ENCODE,
          AT_VARIANT,
          AT_DISPLAY,
          AT_EXTENSION,
          AT_OPTIONAL,
          AT_ERRONEOUS,
          AT_INVALID /// invalid keyword was used
    };
  private:
    attribtype_t attribKeyword;
    /// True if the \c override keyword was used
    bool hasOverride;
    /// The stuff in parenthesis before the attribute text. Owned.
    Qualifiers *attribQualifiers;
    /// The attribute text (FreeText). Owned.
    AttributeSpec* attribSpec;

  public:
    SingleWithAttrib(attribtype_t p_attribKeyword, bool p_hasOverride,
          Qualifiers *p_attribQualifiers, AttributeSpec *p_attribSpec);
    ~SingleWithAttrib();
    virtual SingleWithAttrib* clone() const;
    virtual void set_fullname(const string& p_fullname);
    attribtype_t get_attribKeyword() const{ return attribKeyword; }
    bool has_override() const { return hasOverride; }
    AttributeSpec const& get_attribSpec() const { return *attribSpec; }
    Qualifiers *get_attribQualifiers() const { return attribQualifiers; }
    virtual void dump(unsigned level) const;
  };

  /**
   * Stores all the attributes found in one \c with statement
   */
  class MultiWithAttrib: public Node, public Location{
    /// Copy constructor disabled.
    MultiWithAttrib(const MultiWithAttrib& p);
    /// Assignment disabled.
    MultiWithAttrib& operator=(const MultiWithAttrib& p);
  private:
    vector<SingleWithAttrib> elements;
  public:
    MultiWithAttrib() : Node(), Location() { }
    ~MultiWithAttrib();
    virtual MultiWithAttrib* clone() const;
    virtual void set_fullname(const string& p_fullname);
    size_t get_nof_elements() const{ return elements.size();}
    void add_element(SingleWithAttrib* p_single){ elements.add(p_single);}
    void add_element_front(SingleWithAttrib* p_single)
      { elements.add_front(p_single);}
    const SingleWithAttrib* get_element(size_t p_index) const;
    SingleWithAttrib* get_element_for_modification(size_t p_index);
    void delete_element(size_t p_index);
    virtual void dump(unsigned level) const;
  };

  /**
   * With this class we create a path from the actual type to the module
   * including all groups.
   *
   * Known owners:
   * - Ttcn::Group
   * - Ttcn::ControlPart
   * - Ttcn::Module
   * - Ttcn::FriendMod
   * - Ttcn::ImpMod
   * - Ttcn::Definition
   * - Common::Type
   */
  class WithAttribPath : public Node {
    /// Copy constructor disabled.
    WithAttribPath(const WithAttribPath& p);
    /// Assignment disabled.
    WithAttribPath& operator=(const WithAttribPath& p);
  private:
    bool had_global_variants;
    bool attributes_checked;
    bool cached;
    bool s_o_encode;
    WithAttribPath* parent;
    MultiWithAttrib* m_w_attrib;
    vector<SingleWithAttrib> cache;

    void qualifierless_attrib_finder(vector<SingleWithAttrib>& p_result,
      bool& stepped_over_encode);
  public:
    WithAttribPath() : Node(), had_global_variants(false),
      attributes_checked(false), cached(false), s_o_encode(false),
      parent(0), m_w_attrib(0) { }
    ~WithAttribPath();
    virtual WithAttribPath* clone() const;
    virtual void set_fullname(const string& p_fullname);
    void set_had_global_variants(bool has) { had_global_variants = has; }
    bool get_had_global_variants() { return had_global_variants; }
    void chk_no_qualif();
    void chk_only_erroneous();
    void chk_global_attrib(bool erroneous_allowed=false);
    void set_parent(WithAttribPath* p_parent) { parent = p_parent; }
    WithAttribPath* get_parent() { return parent; }
    /** Set the contained attribute.
     *
     * @param p_m_w_attr
     * @pre \c add_with_attr has not been called before with a non-NULL arg
     * @post \c attributes_checked is \c false
     */
    void set_with_attr(MultiWithAttrib* p_m_w_attr);
    MultiWithAttrib* get_with_attr() { return m_w_attrib;}
    /** Fills the cache and returns it.
     *
     * @return a reference to the cached list of qualifier-less attributes.
     */
    const vector<SingleWithAttrib>& get_real_attrib();
    const MultiWithAttrib* get_local_attrib() const { return m_w_attrib; }
    bool has_attribs();
    virtual void dump(unsigned int level) const;
  };
}

#endif
