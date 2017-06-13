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
 *   Cserveni, Akos
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef _Ttcn_Templatestuff_HH
#define _Ttcn_Templatestuff_HH

#include "AST_ttcn3.hh"
#include "../Value.hh"
#include "../Type.hh"
#include "../vector.hh"

namespace Ttcn {

  using namespace Common;

  class Template;
  class TemplateInstance;
  class ArrayDimension;
  class ParsedActualParameters;

  /**
   * Class to represent a TTCN-3 ValueRange objects. These are used in
   * value range templates in e.g. (-3.8 .. infinity) format.
   */
  class ValueRange : public Node {
  private:
    Value *min_v;
    Value *max_v;
    bool min_exclusive;
    bool max_exclusive;
    Type::typetype_t type;

    /// Copy constructor for clone() only
    ValueRange(const ValueRange& p);
    /// %Assignment disabled
    ValueRange& operator=(const ValueRange& p);
  public:
    ValueRange(Value *p_min_v, bool lower_excl, Value *p_max_v, bool upper_excl)
      : Node(), min_v(p_min_v), max_v(p_max_v),
        min_exclusive(lower_excl), max_exclusive(upper_excl), type() { }
    virtual ~ValueRange();
    virtual ValueRange* clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    Value *get_min_v() const { return min_v; }
    Value *get_max_v() const { return max_v; }
    void set_typetype(Type::typetype_t t);
    bool is_min_exclusive() const { return min_exclusive; }
    bool is_max_exclusive() const { return max_exclusive; }
    void set_lowerid_to_ref();
    Type::typetype_t get_expr_returntype(Type::expected_value_t exp_val);
    Type *get_expr_governor(Type::expected_value_t exp_val);
    void set_code_section(GovernedSimple::code_section_t p_code_section);
    /** Generates a C++ code sequence, which sets the appropriate value range
     * in C++ object named \a name, which represents the template itself.
     * The code sequence is appended to argument \a str and the
     * resulting string is returned. */
    char *generate_code_init(char *str, const char *name);
    /** Appends the initialization sequence of all (directly or indirectly)
     * referred non-parameterized templates to \a str and returns the resulting
     * string. */
    char *rearrange_init_code(char *str, Common::Module* usage_mod);
    /** Appends the string representation of the value range to \a str. */
    void append_stringRepr(string& str) const;
    virtual void dump(unsigned level) const;
  };

  /**
   * Class to represent TemplateList.
   * Owns and deletes the elements.
   *
   * Note that it doesn't have its own set_fullname() method, only the one
   * inherited from Common::Node. This means that you _also_ have to call
   * set_fullname in a loop on all elements;
   * see Ttcn::Template::set_fullname, case TEMPLATE_LIST.
   */
  class Templates : public Node {
  private:
    vector<Template> ts;

    /// Copy constructor for clone() only
    Templates(const Templates& p);
    /// %Assignment disabled
    Templates& operator=(const Templates& p);
  public:
    Templates() : Node(), ts() { }
    virtual ~Templates();
    virtual Templates *clone() const;
    virtual void set_my_scope(Scope *p_scope);
    /** Appends \a p_t at the end of list. */
    void add_t(Template *p_t);
    /** Adds \a p_t in the front of list and shifts existing elements back by
     * one position. */
    void add_front_t(Template *p_t);
    size_t get_nof_ts() const { return ts.size(); }
    Template*& get_t_byIndex(size_t p_i) { return ts[p_i]; }
    /** Appends the string representation of the template list to \a str. */
    void append_stringRepr(string& str) const;
  };

  /**
   * Class to represent an IndexedTemplate.
   */
  class IndexedTemplate : public Node, public Location {
  private:
    FieldOrArrayRef *index;
    Template *temp;
    /// Copy constructor disabled
    IndexedTemplate(const IndexedTemplate& p);
    /// %Assignment disabled
    IndexedTemplate& operator=(const IndexedTemplate& p);
  public:
    IndexedTemplate(FieldOrArrayRef *p_i, Template *p_t);
    virtual ~IndexedTemplate();
    virtual IndexedTemplate* clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    void set_code_section(GovernedSimple::code_section_t p_code_section);
    const FieldOrArrayRef& get_index() const { return *index; }
    Template *get_template() const { return temp; }
    virtual void dump(unsigned level) const;
  };

  /**
   * Class to represent IndexedTemplateList.
   * Owns and deletes the elements.
   */
  class IndexedTemplates : public Node {
  private:
    /** Indexed templates.  */
    vector<IndexedTemplate> its_v;
    /// Copy constructor disabled
    IndexedTemplates(const IndexedTemplates& p);
    /// %Assignment disabled
    IndexedTemplates& operator=(const IndexedTemplates& p);
  public:
    IndexedTemplates() : Node(), its_v() { }
    virtual ~IndexedTemplates();
    virtual IndexedTemplates* clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    void add_it(IndexedTemplate *p_it);
    size_t get_nof_its() const { return its_v.size(); }
    IndexedTemplate *get_it_byIndex(size_t p_i);
  };

  /**
   * Class to represent a NamedTemplate.
   */
  class NamedTemplate : public Node , public Location {
  private:
    Identifier *name;
    Template *temp;

    /* Copy constructor disabled. */
    NamedTemplate(const NamedTemplate& p);

    /** Copy assignment disabled */
    NamedTemplate& operator=(const NamedTemplate& p);
  public:
    NamedTemplate(Identifier *p_n, Template *p_t);
    virtual ~NamedTemplate();
    virtual NamedTemplate* clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    const Identifier& get_name() const { return *name; }
    /* \todo this should be called get_Template, like in TemplateInstance */
    Template *get_template() const { return temp; }
    /** Sets the first letter in the name of the named template to lowercase
      * if it's an uppercase letter.
      * Used on open types (the name of their alternatives can be given with both
      * an uppercase or a lowercase first letter, and the generated code will need
      * to use the lowercase version). */
    void set_name_to_lowercase();
    /** Remove the template from the ownership of NamedTemplate.
     * @return \a temp
     * @post \a temp == 0 */
    Template *extract_template();
    virtual void dump(unsigned level) const;
  };

  /**
   * Class to represent NamedTemplateList.
   */
  class NamedTemplates : public Node {
  private:
    /** named templates */
    vector<NamedTemplate> nts_v;
    /** Stores the first occurrence of NamedTemplate with id. The string
     * parameter refers to the id of the nt, the size_t param refers
     * to the index in nts. */
    map<string, NamedTemplate> nts_m;
    /** True if NamedTemplates::chk_dupl_id() has been called, else false */
    bool checked;
    /** Copy constructor disabled. */
    NamedTemplates(const NamedTemplates& p);
    /** Copy assignment disabled. */
    NamedTemplates& operator=(const NamedTemplates& p);
  public:
    NamedTemplates() : Node(), nts_v(), nts_m(), checked(false) { }
    virtual ~NamedTemplates();
    virtual NamedTemplates* clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    void add_nt(NamedTemplate *p_nt);
    size_t get_nof_nts() const { return nts_v.size(); }
    NamedTemplate *get_nt_byIndex(size_t p_i) { return nts_v[p_i]; }
    bool has_nt_withName(const Identifier& p_name);
    NamedTemplate *get_nt_byName(const Identifier& p_name);
    void chk_dupl_id(bool report_errors = true);
  };

  /** Class to represent length restrictions associated with string,
   * set of, record of and array values */
  class LengthRestriction : public Node, public Location {
    bool checked;
    bool is_range;
    union {
      Value *single; // owned
      struct {
        Value *lower, *upper; // both owned
      } range;
    };

    LengthRestriction(const LengthRestriction& p);
    /** Copy assignment disabled */
    LengthRestriction& operator=(const LengthRestriction& p);
  public:
    LengthRestriction(Value* p_val);
    LengthRestriction(Value* p_lower, Value* p_upper);
    virtual ~LengthRestriction();
    virtual LengthRestriction* clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    bool get_is_range() const { return is_range; }
    void chk(Type::expected_value_t expected_value);
    /** Checks whether the length restriction contradicts array dimension
     * \a p_dim. If no contradiction is found an error message is
     * displayed.*/
    void chk_array_size(ArrayDimension *p_dim);
    /** Checks the number of elements in the template against the length
     * restriction. Issues a warning if there are too few or too many elements
     * thus the template will not match anything. Argument \a nof_elements
     * shows the minimal number of elements in the template (embedded * symbols
     * are not considered) and flag \a has_anyornone indicates if there is at
     * least one * in the template. Arguments \a p_loc (containing the location
     * of the template that the length restriction belongs to) and \a p_what
     * (containing the word "template", "value" or "string") are used for error
     * reporting . */
    void chk_nof_elements(size_t nof_elements, bool has_anyornone,
      const Location& p_loc, const char *p_what);
    Value *get_single_value();
    Value *get_lower_value();
    Value *get_upper_value();
    void set_code_section(GovernedSimple::code_section_t p_code_section);
    /** Generates a C++ code sequence, which sets the appropriate length
     * restriction attributes in C++ object named \a name, which represents the
     * owner template. The code sequence is appended to argument \a str and the
     * resulting string is returned. */
    char *generate_code_init(char *str, const char *name);
    /** Appends the initialization sequence of all (directly or indirectly)
     * referred non-parameterized templates to \a str and returns the resulting
     * string. */
    char *rearrange_init_code(char *str, Common::Module* usage_mod);
    /** Appends the string representation of the length restriction to
     * \a str. */
    void append_stringRepr(string& str) const;
    virtual void dump(unsigned level) const;
  };

  /**
   * Represents a list of template instances.
   */
  class TemplateInstances : public Node, public Location {
  private:
    vector<TemplateInstance> tis;
    /** Copy constructor disabled. */
    TemplateInstances(const TemplateInstances& p);
    /** Copy assignment not implemented: disabled */
    TemplateInstances& operator=(const TemplateInstances& p);

    friend class ParsedActualParameters;
  public:
    TemplateInstances() : Node(), Location(), tis() { }
    virtual ~TemplateInstances();
    virtual TemplateInstances *clone() const;
    virtual void dump(unsigned level) const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    void add_ti(TemplateInstance *p_ti);
    size_t get_nof_tis() const { return tis.size(); }
    TemplateInstance *get_ti_byIndex(size_t p_i) const { return tis[p_i]; }
    void set_code_section(GovernedSimple::code_section_t p_code_section);
  };

  /** A named actual parameter */
  class NamedParam : public Node, public Location {
    Identifier       *name;
    TemplateInstance *tmpl;

    NamedParam(const NamedParam& p);
    /** Copy assignment disabled. */
    NamedParam& operator=(const NamedParam& p);
  public:
    NamedParam(Identifier *id, TemplateInstance *t);
    virtual ~NamedParam();
    /** "Virtual copy constructor" */
    virtual NamedParam *clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);

    Identifier *get_name() const { return name; }
    TemplateInstance *get_ti() const { return tmpl; }
    TemplateInstance *extract_ti();

    virtual void dump(unsigned int level) const;
  };

  /** A collection of named actual parameters */
  class NamedParams : public Node, public Location {
  private:
    vector<NamedParam> nps;

    NamedParams(const NamedParams& p);
    /** Copy assignment disabled. */
    NamedParams& operator=(const NamedParams& p);
  public:
    NamedParams();
    /** Destructor. Empties \p nps and frees its elements. */
    virtual ~NamedParams();
    /** "Virtual copy constructor" */
    virtual NamedParams *clone() const;
    void set_my_scope(Scope *p_scope);
    void set_fullname(const string& p_fullname);

    /** Append \p p to the list of named actual parameters. */
    void add_np(NamedParam *p);

    size_t get_nof_nps() const;

    /** Replace the named parameter at index \p p_i with NULL and
     * return its previous value. */
    NamedParam *extract_np_byIndex(size_t p_i);

    virtual void dump(unsigned int level) const;
  };

  /** Actual parameters from the parser in "raw form".
   *
   * Contains both positional and named parameters.
   * There is not enough information during parsing to construct a "full"
   * ActualParameters object (information about formal parameters is needed).
   * This object holds the available information until the ActualParameters
   * object can be constructed during semantic analysis.
   *
   * TODO think of a shorter name */
  class ParsedActualParameters : public Node, public Location {
    TemplateInstances *unnamedpart; ///< the "classic" unnamed parameters
    NamedParams       *namedpart; ///< the named parameters

    ParsedActualParameters(const ParsedActualParameters& p);
    /** Copy assignment disabled. */
    ParsedActualParameters& operator=(const ParsedActualParameters& p);
  public:
    ParsedActualParameters(TemplateInstances *p_ti = 0, NamedParams *p_np = 0);
    virtual ~ParsedActualParameters();
    virtual ParsedActualParameters *clone() const;

    // @name Distributors. Pass on the call to \a namedpart and \a unnamedpart.
    // @{
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    void set_location(const char *p_filename, int p_lineno=0);
    void set_location(const char *p_filename, const YYLTYPE& p_yyloc);
    void set_location(const char *p_filename, const YYLTYPE& p_firstloc,
                                              const YYLTYPE& p_lastloc);
    void set_location(const char *p_filename, int p_first_line,
      int p_first_column, int p_last_line, int p_last_column);
    virtual void dump(unsigned int level) const;
    // @}

    /** Return the TemplateInstances member.
     * \note ParsedActualParameters object owns the TemplateInstances object;
     * the caller should clone() it if it wants a copy.
     *
     * The return value should be a const reference to prevent the caller
     * from freeing it. */
    TemplateInstances *get_tis() const { return unnamedpart; }

    TemplateInstances* steal_tis();

    size_t get_nof_tis() const { return unnamedpart->get_nof_tis(); }

    TemplateInstance *get_ti_byIndex(size_t p_i) const {
      return unnamedpart->get_ti_byIndex(p_i); }

    TemplateInstance *set_ti_byIndex(size_t p_i, TemplateInstance *p_np) {
      TemplateInstance *retval = unnamedpart->get_ti_byIndex(p_i);
      unnamedpart->tis[p_i] = p_np;
      return retval;
    }

    /// Append \p p_ti to the internal list of TemplateInstance s
    void add_ti(TemplateInstance *p_ti){
      unnamedpart->add_ti(p_ti);
    }

    /// Calls TemplateInstances::set_code_section() for \p unnamedpart
    void set_code_section(GovernedSimple::code_section_t p_code_section) {
      unnamedpart->set_code_section(p_code_section);
    }

    // Named params section

    /// Append \p np to the list in \p namedpart
    void add_np(NamedParam *np);

    size_t get_nof_nps() const;

    NamedParam *extract_np_byIndex(size_t p_i) {
      return namedpart->extract_np_byIndex(p_i);
    }
  };

} // namespace Ttcn

#endif // _Ttcn_Templatestuff_HH
