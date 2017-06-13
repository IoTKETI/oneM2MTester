/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Baji, Laszlo
 *   Balasko, Jeno
 *   Baranyi, Botond
 *   Delic, Adam
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Zalanyi, Balazs Andor
 *   Pandi, Krisztian
 *
 ******************************************************************************/
#ifndef _Ttcn_AST_HH
#define _Ttcn_AST_HH

#include "../AST.hh"

namespace Common {
class CodeGenHelper;
}

/**
 * This namespace contains classes unique to TTCN-3 and some specializations
 * of classes from Common.
 */
namespace Ttcn {

  /**
   * \addtogroup AST
   *
   * @{
   */

  using namespace Common;

  class Module;
  class Definition;
  class FriendMod;
  class Imports;
  class ImpMod;
  class ActualPar;
  class ActualParList;
  class FormalParList;
  class ParsedActualParameters;
  class Template;
  class TemplateInstance;
  class TemplateInstances;
  class ArrayDimensions;
  class Timer;
  class StatementBlock;
  class AltGuards;
  class ILT;
  class Group;
  class WithAttribPath;
  class ErrorBehaviorList;
  class ErroneousAttributes;
  class ErroneousAttributeSpec;
  class PrintingType;

  /** Class to represent an actual parameter */
  class ActualPar : public Node {
  public:
    enum ap_selection_t {
      AP_ERROR,    ///< erroneous
      AP_VALUE,    ///< "in" value parameter
      AP_TEMPLATE, ///< "in" template parameter
      AP_REF,      ///< out/inout value or template parameter
      AP_DEFAULT   ///< created from the default value of a formal parameter
    };
  private:
    ap_selection_t selection;
    union {
      Value *val; ///< Value for AP_VALUE. Owned by ActualPar
      TemplateInstance *temp; ///< %Template for AP_TEMPLATE. Owned by ActualPar
      Ref_base  *ref; ///< %Reference for AP_REF. Owned by ActualPar
      ActualPar *act; ///< For AP_DEFAULT. \b Not owned by ActualPar
    };
    Scope *my_scope; ///< %Scope. Not owned
    /** tells what runtime template restriction check to generate,
     *  TR_NONE means that no check is needed because it could be determined
     *  in compile time */
    template_restriction_t gen_restriction_check;
    /** if this is an actual template parameter of an external function add
     *  runtime checks for out and inout parameters after the call */
    template_restriction_t gen_post_restriction_check;
  private:
    /** Copy constructor not implemented */
    ActualPar(const ActualPar& p);
    /** %Assignment disabled */
    ActualPar& operator=(const ActualPar& p);
  public:
    /// Constructor for an erroneous object (fallback)
    ActualPar()
    : Node(), selection(AP_ERROR), my_scope(0),
      gen_restriction_check(TR_NONE), gen_post_restriction_check(TR_NONE) {}
    /// Actual par for an in value parameter
    ActualPar(Value *v);
    /// Actual par for an in template parameter
    ActualPar(TemplateInstance *t);
    /// Actual par for an {out or inout} {value or template} parameter
    ActualPar(Ref_base *r);
    /// Created from the default value of a formal par, at the call site,
    ///
    ActualPar(ActualPar *a);
    virtual ~ActualPar();
    virtual ActualPar *clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    bool is_erroneous() const { return selection == AP_ERROR; }
    ap_selection_t get_selection() const { return selection; }
    Value *get_Value() const;
    TemplateInstance *get_TemplateInstance() const;
    Ref_base *get_Ref() const;
    ActualPar *get_ActualPar() const;
    /** Checks the embedded recursions within the value or template instance. */
    void chk_recursions(ReferenceChain& refch);
    /** Returns whether the actual parameter can be represented by an in-line
     * C++ expression. */
    bool has_single_expr();
    void set_code_section(GovernedSimple::code_section_t p_code_section);
    /** Generates the C++ equivalent of \a this into \a expr.
     * Flag \a copy_needed indicates whether to add an extra copy constructor
     * call if \a this contains a referenced value or template to avoid
     * aliasing problems with other out/inout parameters. */
    void generate_code(expression_struct *expr, bool copy_needed, FormalPar* formal_par) const;
    /** Appends the initialization sequence of all (directly or indirectly)
     * referred non-parameterized templates and the default values of all
     *  parameterized templates to \a str and returns the resulting string. 
     *  Only objects belonging to module \a usage_mod are initialized. */
    char *rearrange_init_code(char *str, Common::Module* usage_mod);
    char *rearrange_init_code_defval(char *str, Common::Module* usage_mod);
    /** Appends the string representation of the actual parameter to \a str. */
    void append_stringRepr(string& str) const;
    virtual void dump(unsigned level) const;
    void set_gen_restriction_check(template_restriction_t tr)
      { gen_restriction_check = tr; }
    template_restriction_t get_gen_restriction_check()
      { return gen_restriction_check; }
    void set_gen_post_restriction_check(
      template_restriction_t p_gen_post_restriction_check)
      { gen_post_restriction_check = p_gen_post_restriction_check; }
  };

  /// A collection of actual parameters (parameter list)
  class ActualParList : public Node {
    vector<ActualPar> params;
  public:
    ActualParList(): Node(), params() { }
    ActualParList(const ActualParList& p);
    ~ActualParList();
    ActualParList* clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    size_t get_nof_pars() const { return params.size(); }
    void add(ActualPar* p) { params.add(p); }
    ActualPar* get_par(size_t i) const { return params[i]; }
    /** Checks the embedded recursions within the values and template
     * instances of actual parameters. */
    void chk_recursions(ReferenceChain& refch);
    /** Generates the C++ equivalent of the actual parameter list without
     * considering any aliasing between variables and 'in' parameters. */
    void generate_code_noalias(expression_struct *expr, FormalParList *p_fpl);
    /** Generates the C++ equivalent of the actual parameter list considering
     * aliasing problems between the 'in' parameters and 'out'/'inout'
     * parameters as well as component variables seen by the called definition.
     * Argument \a p_fpl points to the formal parameter list of the referred
     * definition if it is known, otherwise it is NULL. Argument \a p_comptype
     * points to the component type identified by the 'runs on' clause of the
     * referred definition (if exists and relevant for alias analysis,
     * otherwise NULL).
     * When invoke() is used with FAT types: the special case of 'runs on self'
     * has the argument \a p_compself set to true and \a p_comptype is NULL,
     * but the component is 'self' or any compatible component. */
    void generate_code_alias(expression_struct *expr, FormalParList *p_fpl,
      Type *p_comptype, bool p_compself);
    /** Walks through the parameter list and appends the initialization
     * sequence of all (directly or indirectly) referred non-parameterized
     * templates and the default values of all parameterized templates to
     * \a str and returns the resulting string. 
     * Only objects belonging to module \a usage_mod are initialized. */
    char *rearrange_init_code(char *str, Common::Module* usage_mod);
    virtual void dump(unsigned level) const;
  };

  /** Helper class for the Ttcn::Reference */
  class FieldOrArrayRef : public Node, public Location {
  public:
    enum reftype { FIELD_REF, ARRAY_REF };
  private:
    reftype ref_type; ///< reference type
    /** The stored reference. Owned and destroyed by FieldOrArrayRef */
    union {
       Identifier *id; ///< name of the field, used by FIELD_REF
       Value *arp; ///< value of the index, used by ARRAY_REF
    } u;
    /** Copy constructor for clone() only */
    FieldOrArrayRef(const FieldOrArrayRef& p);
    /** %Assignment disabled */
    FieldOrArrayRef& operator=(const FieldOrArrayRef& p);
  public:
    FieldOrArrayRef(Identifier *p_id);
    FieldOrArrayRef(Value *p_arp);
    ~FieldOrArrayRef();
    virtual FieldOrArrayRef* clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    reftype get_type() const { return ref_type; }
    /** Return the identifier.
     * @pre reftype is FIELD_REF, or else FATAL_ERROR */
    const Identifier* get_id() const;
    /** Returns the value.
     * @pre reftype is ARRAY_REF, or else FATAL_ERROR */
    Value* get_val() const;
    /** Appends the string representation of the sub-reference to \a str. */
    void append_stringRepr(string& str) const;
    /** Sets the first letter in the name of the field to lowercase if it's an
      * uppercase letter.
      * Used on open types (the name of their alternatives can be given with both
      * an uppercase or a lowercase first letter, and the generated code will need
      * to use the lowercase version). */
    void set_field_name_to_lowercase();
  };

  /** A vector of FieldOrArrayRef objects */
  class FieldOrArrayRefs : public Node {
    /** Element holder. This FieldOrArrayRefs object owns the elements
     * and will free them in the destructor. */
    vector<FieldOrArrayRef> refs;
    /** Indicates whether the last array index refers to an element of a
     * string value. */
    bool refs_str_element;
  public:
    FieldOrArrayRefs() : Node(), refs(), refs_str_element(false) { }
    FieldOrArrayRefs(const FieldOrArrayRefs& p);
    ~FieldOrArrayRefs();
    FieldOrArrayRefs *clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    void add(FieldOrArrayRef *p_ref) { refs.add(p_ref); }
    size_t get_nof_refs() const { return refs.size(); }
    FieldOrArrayRef* get_ref(size_t i) const { return refs[i]; }
    bool has_unfoldable_index() const;
    /** Removes (deletes) the first \a n field references. */
    void remove_refs(size_t n);
    Identifier *remove_last_field();
    /** Generates the C++ sub-expression that accesses
     * the given sub-references of definition \a ass. 
     * @param nof_subrefs indicates the number of sub-references
     * to generate code from (UINT_MAX means all of them) */
    void generate_code(expression_struct *expr, Common::Assignment *ass, size_t nof_subrefs = UINT_MAX);
    /** Generates the C++ sub-expression that could access the
      * sub-references of a reference of type \a type
      * @param nof_subrefs indicates the number of sub-references
      * to generate code from (UINT_MAX means all of them) */
    void generate_code(expression_struct *expr, Type *type, bool is_template = false, size_t nof_subrefs = UINT_MAX);
    /** Appends the string representation of sub-references to \a str. */
    void append_stringRepr(string &str) const;
    bool refers_to_string_element() const { return refs_str_element; }
    void set_string_element_ref() { refs_str_element = true; }
    void clear_string_element_ref() { refs_str_element = false; }
  };

  /**
   * Base class for all TTCN-3 references.
   * Includes the common functionality for parameterized and non-parameterized
   * references (e.g. handling of field or array subreferences).
   */
  class Ref_base : public Ref_simple {
  protected: // Ttcn::Reference and Ttcn::Ref_pard need access
    Identifier *modid;
    /** If id is a NULL pointer all components are stored in subrefs */
    Identifier *id;
    FieldOrArrayRefs subrefs;
    /** Indicates whether the consistency of formal and actual parameter lists
     * has been verified. */
    bool params_checked;
    bool usedInIsbound;
    Ref_base(const Ref_base& p);
  private:
    Ref_base& operator=(const Ref_base& p);
  public:
    /** Default constructor: sets \a modid and \a id to NULL. Used by
     * non-parameterized references only. It is automatically guessed whether
     * the first component of \a subrefs is a module id or not. */
    Ref_base() : Ref_simple(), modid(0), id(0), subrefs(), params_checked(0)
      , usedInIsbound(false) {}
    Ref_base(Identifier *p_modid, Identifier *p_id);
    ~Ref_base();
    virtual Ref_base *clone() const = 0;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    /** Sets the scope of the base reference to \a p_scope.
     * The scope of array indices in \a subrefs remains unchanged. */
    void set_base_scope(Scope *p_scope) { Ref_simple::set_my_scope(p_scope); }
    virtual bool getUsedInIsbound() {return usedInIsbound;}
    virtual void setUsedInIsbound() {usedInIsbound = true;}
    Setting *get_refd_setting();
    FieldOrArrayRefs *get_subrefs();
    /** Appends \a p_ref to the sub-references */
    void add(FieldOrArrayRef *p_ref) { subrefs.add(p_ref); }
    virtual bool has_single_expr();
    virtual void set_code_section(
      GovernedSimple::code_section_t p_code_section);
    /** Generates the C++ equivalent of the reference (including the parameter
     * list and sub-references) as an access to a constant resource.
     */
    virtual void generate_code_const_ref(expression_struct_t *expr);
  };

  /**
   * TTCN-3 reference without parameters.
   * Implements the automatic detection whether the first identifier is a
   * module name or not.
   */
  class Reference : public Ref_base {
    ActualParList *parlist;
  public:
    Reference(Identifier *p_id);
    Reference(Identifier *p_modid, Identifier *p_id)
      : Ref_base(p_modid, p_id), parlist(0) { }
    ~Reference();
    virtual Reference *clone() const;
    virtual string get_dispname();
    virtual Common::Assignment *get_refd_assignment(bool check_parlist = true);
    virtual const Identifier* get_modid();
    virtual const Identifier* get_id();
    /** Checks whether \a this points to a variable or value parameter.
     * Returns the type of the respective variable or variable field or NULL
     * in case of error. */
    Type *chk_variable_ref();
    /** Checks if \a this points to a component.
     *  Returns the type of the component if so or NULL in case of error. */
    Type *chk_comptype_ref();
    virtual bool has_single_expr();
    virtual void generate_code(expression_struct_t *expr);
    /** Generates the C++ equivalent of port references within
     * connect/disconnect/map/unmap statements into \a expr.
     * Argument \a p_scope shall point to the scope of the statement. */
    void generate_code_portref(expression_struct_t *expr, Scope *p_scope);
    virtual void generate_code_const_ref(expression_struct_t *expr);
    /**
     * Generates code for checking if the reference
     * and the referred objects are bound or not.*/
    void generate_code_ispresentbound(expression_struct_t *expr,
      bool is_template, const bool isbound);
    /** Lets the referenced assignment object know, that the reference is used
      * at least once (only relevant for formal parameters and external constants). */
    void ref_usage_found();
  private:
    /** Detects whether the first identifier in subrefs is a module id */
    void detect_modid();
  };

  /**
   * Parameterized TTCN-3 reference
   */
  class Ref_pard : public Ref_base {
    /** "Processed" parameter list, after the semantic check. */
    ActualParList parlist;
    /** "Raw" parameter list, before the semantic check. */
    Ttcn::ParsedActualParameters *params;
    /** Used by generate_code_cached(). Stores the generated expression string, 
      * so it doesn't get regenerated every time. */
    char* expr_cache;
    /** Copy constructor. Private, used by Ref_pard::clone() only */
    Ref_pard(const Ref_pard& p);
    /// %Assignment disabled
    Ref_pard& operator=(const Ref_pard& p);
  public:
    /** Constructor
     * \param p_modid the module in which it resides
     * \param p_id the identifier
     * \param p_params parameters. For a function, this is the list constructed
     * for the actual parameters.
     * */
    Ref_pard(Identifier *p_modid, Identifier *p_id,
             ParsedActualParameters *p_params);
    ~Ref_pard();
    virtual Ref_pard *clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    string get_dispname();
    virtual Common::Assignment *get_refd_assignment(bool check_parlist = true);
    virtual const Identifier *get_modid();
    virtual const Identifier *get_id();
    virtual ActualParList *get_parlist();
    /** Checks whether \a this is a correct argument of an activate operation
     * or statement. The reference shall point to an altstep with proper
     * `runs on' clause and the actual parameters that are passed by reference
     * shall not point to local definitions. The function returns true if the
     * altstep reference is correct and false in case of any error. */
    bool chk_activate_argument();
    virtual bool has_single_expr();
    virtual void set_code_section(
      GovernedSimple::code_section_t p_code_section);
    virtual void generate_code          (expression_struct_t *expr);
    virtual void generate_code_const_ref(expression_struct_t *expr);
    
    /** Used when an 'all from' is called on a function or parametrised template,
      * generate_code would generate new temporaries for the function's parameters
      * each call. This method makes sure the same temporaries are used every time
      * the function is called in the generated code.
      * On the first run calls generate_code and stores its result (only expr.expr
      * is cached, since the preamble is only needed after the first call). 
      * On further runs the cached expression is returned.*/
    virtual void generate_code_cached (expression_struct_t *expr);
  };

  /**
  * Class Ttcn::NameBridgingScope.
  * This scope unit is NOT A REAL SCOPE UNIT,
  * its only purpose is to serve as a bridge with a name between two real scope
  * units. All operations are transfered automatically.
  */
  class NameBridgingScope : public Scope {
    virtual string get_scopeMacro_name() const;
    virtual NameBridgingScope* clone() const;
    virtual Common::Assignment* get_ass_bySRef(Ref_simple *p_ref);
  };

  /**
   * Class Ttcn::RunsOnScope.
   * Implements the scoping rules for functions, altsteps and testcases that
   * have a 'runs on' clause. First looks for the definitions in the given
   * component type first then it searches in its parent scope.
   * Note: This scope unit cannot access the parent scope of the component type
   * (which is a module Definitions) unless the component type and the
   * 'runs on' clause is defined in the same module.
   */
  class RunsOnScope : public Scope {
    /** Points to the component type. */
    Type *component_type;
    /** Shortcut to the definitions within \a component_type. */
    ComponentTypeBody *component_defs;

    /** Not implemented. Causes \a FATAL_ERROR. */
    RunsOnScope(const RunsOnScope& p);
    /** %Assignment not implemented */
    RunsOnScope& operator=(const RunsOnScope& p);
  public:
    RunsOnScope(Type *p_comptype);
    virtual RunsOnScope *clone() const;

    Type *get_component_type() const { return component_type; }
    /** Checks the uniqueness of definitions within \a component_defs and
     * reports warnings in case of hiding. */
    void chk_uniq();

    virtual RunsOnScope *get_scope_runs_on();
    virtual Common::Assignment *get_ass_bySRef(Ref_simple *p_ref);
    virtual bool has_ass_withId(const Identifier& p_id);
  };
  
  /**
   * Class Ttcn::PortScope.
   * Implements the scoping rules for functions that
   * have a 'port' clause. First looks for the variable definitions in the given
   * port type first then it searches in its parent scope.
   * Note: This scope unit cannot access the parent scope of the port type.
   */
  class PortScope : public Scope {
    /** Points to the port type. */
    Type *port_type;
    /** Shortcut to the definitions within \a port_type. */
    Definitions *vardefs;

    /** Not implemented. Causes \a FATAL_ERROR. */
    PortScope(const PortScope& p);
    /** %Assignment not implemented */
    PortScope& operator=(const PortScope& p);
  public:
    PortScope(Type *p_porttype);
    virtual PortScope *clone() const;

    Type *get_port_type() const { return port_type; }
    /** Checks the uniqueness of definitions within \a portdefs and
     * reports warnings in case of hiding. */
    void chk_uniq();

    virtual PortScope *get_scope_port();
    virtual Common::Assignment *get_ass_bySRef(Ref_simple *p_ref);
    virtual bool has_ass_withId(const Identifier& p_id);
  };

  /**
   * Class Ttcn::Definitions.
   *
   * Owns the contained Definition objects.
   */
  class Definitions : public Common::Assignments {
  protected:
    /** Searchable map of definitions. Used after chk_uniq. */
    map<string, Definition> ass_m;
    /** Indicates whether the uniqueness of identifiers has been checked. */
    bool checked;
    /** Vector containing all definitions. Used for building. */
    vector<Definition> ass_v;

    Definitions(const Definitions& p);
  public:

    Definitions() : Common::Assignments(), ass_m(), checked(false), ass_v() {}
    ~Definitions();
    Definitions *clone() const;
    virtual void set_fullname(const string& p_fullname);
    /** Adds the assignment p_ass and becomes the owner of it.
     *  The uniqueness of the identifier is not checked. */
    void add_ass(Definition *p_ass);
    virtual bool has_local_ass_withId(const Identifier& p_id);
    virtual Common::Assignment* get_local_ass_byId(const Identifier& p_id);
    virtual size_t get_nof_asss();
    virtual Common::Assignment* get_ass_byIndex(size_t p_i);
    size_t get_nof_raw_asss();
    Definition *get_raw_ass_byIndex(size_t p_i);
    /** Checks the uniqueness of identifiers. */
    void chk_uniq();
    /** Checks all definitions. */
    void chk();
    /** Checks the definitions within the header of a for loop. */
    void chk_for();
    /** Sets the genname of embedded definitions using \a prefix. */
    void set_genname(const string& prefix);
    /** Generates code for all assignments into \a target. */
    void generate_code(output_struct *target);
    void generate_code(CodeGenHelper& cgh);
    char* generate_code_str(char *str);
    void ilt_generate_code(ILT *ilt);
    /** Prints the contents of all assignments. */
    virtual void dump(unsigned level) const;
  };

  /**
   * Class Ttcn::Definitions.
   *
   * Owns the contained Definition objects.
   */
/*  class Definitions : public OtherDefinitions {
  public:
    Definitions() : OtherDefinitions() {}
    ~Definitions();
    Definitions *clone() const;
    void add_ass(Definition *p_ass);
  };*/

  /** Represents a TTCN-3 group
   *
   *  @note a Group is not a Scope */
  class Group : public Node, public Location {
  private:
    Group* parent_group;
    WithAttribPath* w_attrib_path;
    /// Definitions that belong to this group (directly)
    vector<Definition> ass_v;
    /// Map the name to the definition (filled in chk_uniq)
    map<string,Definition> ass_m;
    /// Subgroups
    vector<Group> group_v;
    /// Map the name to the subgroup
    map<string,Group> group_m;
    vector<ImpMod> impmods_v;
    vector<FriendMod> friendmods_v;
    Identifier *id;
    bool checked;
  private:
    /** Copy constructor not implemented */
    Group(const Group& p);
    /** %Assignment not implemented */
    Group& operator=(const Group& p);
  public:
    Group(Identifier *p_id);
    ~Group();
    virtual Group* clone() const;
    virtual void set_fullname(const string& p_fullname);
    void add_ass(Definition* p_ass);
    void add_group(Group* p_group);
    void set_parent_group(Group* p_parent_group);
    Group* get_parent_group() const { return parent_group; }
    void set_attrib_path(WithAttribPath* p_path);
    const Identifier& get_id() const { return *id; }
    void chk_uniq();
    virtual void chk();
    void add_impmod(ImpMod *p_impmod);
    void add_friendmod(FriendMod *p_friendmod);
    virtual void dump(unsigned level) const;
    void set_with_attr(MultiWithAttrib* p_attrib);
    WithAttribPath* get_attrib_path();
    void set_parent_path(WithAttribPath* p_path);
  };

  class ControlPart : public Node, public Location {
  private:
    StatementBlock* block;
    WithAttribPath* w_attrib_path;

    NameBridgingScope bridgeScope;

    /// Copy constructor disabled
    ControlPart(const ControlPart& p);
    /// %Assignment disabled
    ControlPart& operator=(const ControlPart& p);
  public:
    ControlPart(StatementBlock* p_block);
    ~ControlPart();
    virtual ControlPart* clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    void chk();
    void generate_code(output_struct *target, Module *my_module);
    void set_with_attr(MultiWithAttrib* p_attrib);
    WithAttribPath* get_attrib_path();
    void set_parent_path(WithAttribPath* p_path);
    void dump(unsigned level) const;
  };

  /** A TTCN-3 module */
  class Module : public Common::Module {
  private:
    string *language_spec;
    Definitions *asss;
    vector<Group> group_v;
    map<string, Group> group_m;
    WithAttribPath* w_attrib_path;
    Imports *imp;
    ControlPart* controlpart;
    /** For caching the scope objects that are created in
     * \a get_runs_on_scope() and get_port_scope(). */
    vector<RunsOnScope> runs_on_scopes;
    vector<PortScope> port_scopes;
  private:
    /** Copy constructor not implemented */
    Module(const Module& p);
    /** %Assignment not implemented */
    Module& operator=(const Module& p);
  public:
    vector<FriendMod> friendmods_v;
    Module(Identifier *p_modid);
    ~Module();
    void add_group(Group* p_group);
    void add_friendmod(FriendMod *p_friendmod);
    virtual Module* clone() const;
    virtual Common::Assignment* importAssignment(
      const Identifier& p_source_modid, const Identifier& p_id) const;
    virtual void set_fullname(const string& p_fullname);
    virtual Common::Assignments* get_scope_asss();
    virtual bool has_imported_ass_withId(const Identifier& p_id);
    virtual Common::Assignment* get_ass_bySRef(Ref_simple *p_ref);
    virtual bool is_valid_moduleid(const Identifier& p_id);
    virtual Common::Assignments *get_asss();
    virtual bool exports_sym(const Identifier& p_id);
    virtual Type *get_address_type();
    virtual void chk_imp(ReferenceChain& refch, vector<Common::Module>& moduleStack);
    virtual void chk();
  private:
    void chk_friends();
    void chk_groups();
    virtual void get_imported_mods(module_set_t& p_imported_mods);
    virtual void generate_code_internal(CodeGenHelper& cgh);
  public:
    /** Returns a scope that can access the definitions within component type
     * \a comptype (which is imported from another module) and its parent scope
     * is \a asss. Note that this scope cannot see the scope of \a comptype.
     * The function uses \a runs_on_scopes for caching the scope objects: if an
     * object has been created for a component type it will be returned later
     * instead of creating a new one. */
    RunsOnScope *get_runs_on_scope(Type *comptype);
    /* The same as get_runs_on_scope except that the returned scope can access
     * the port types definitions.
     */
    PortScope *get_port_scope(Type *porttype);
    virtual void dump(unsigned level) const;
    void set_language_spec(const char *p_language_spec);
    void add_ass(Definition* p_ass);
    void add_impmod(ImpMod *p_impmod);
    void add_controlpart(ControlPart* p_controlpart);
    void set_with_attr(MultiWithAttrib* p_attrib);
    WithAttribPath* get_attrib_path();
    void set_parent_path(WithAttribPath* p_path);
    Imports& get_imports() { return *imp; }

    bool is_visible(const Identifier& id, visibility_t visibility);
    
    /** Generates JSON schema segments for the types defined in the modules,
      * and references to these types. Information related to the types' 
      * JSON encoding and decoding functions is also inserted after the references.
      *
      * @param json JSON document containing the main schema, schema segments for 
      * the types will be inserted here
      * @param json_refs map of JSON documents containing the references and function
      * info related to each type */
    virtual void generate_json_schema(JSON_Tokenizer& json, map<Type*, JSON_Tokenizer>& json_refs);
    
    /** Generates the debugger initialization function for this module.
      * The function creates the global debug scope associated with this module,
      * and initializes it with all the global variables visible in the module
      * (including imported variables).
      * The debug scopes of all component types defined in the module are also
      * created and initialized with their variables. */
    virtual void generate_debugger_init(output_struct *output);
    
    /** Generates the variable adding code for all global variables defined
      * in this module. This function is called by generate_debugger_init()
      * for both the current module and all imported modules. */
    virtual char* generate_debugger_global_vars(char* str, Common::Module* current_mod);
    
    /** Generates the debugger variable printing function, which can print values
      * and templates of all types defined in this module (excluding subtypes). */
    virtual void generate_debugger_functions(output_struct *output);
  };

  /**
  * Module friendship declaration.
  */
  class FriendMod : public Node, public Location {
  private:
    Module *my_mod;
    Identifier *modid;
    WithAttribPath* w_attrib_path;
    Group* parentgroup;
    /** Indicates whether this friend module declaration was checked */
    bool checked;
  private:
    /** Copy constructor not implemented. */
    FriendMod(const FriendMod&);
    /** %Assignment not implemented. */
    FriendMod& operator=(const FriendMod&);
  public:
    FriendMod(Identifier *p_modid);
    ~FriendMod();
    virtual FriendMod* clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void chk();
    void set_my_mod(Module *p_mod) { my_mod = p_mod; }
    const Identifier& get_modid() const {return *modid;}
    void set_with_attr(MultiWithAttrib* p_attrib);
    WithAttribPath* get_attrib_path();
    void set_parent_path(WithAttribPath* p_path);
    void set_parent_group(Group* p_group);
  };


  /**
   * Imported module. Represents an import statement.
   */
  class ImpMod : public Node, public Location {
  public:
    enum imptype_t {
      I_UNDEF,
      I_ERROR,
      I_ALL,
      I_IMPORTSPEC,
      I_IMPORTIMPORT,
      /** Phantom import for when the 'include' is needed in the generated C++
        * code, but the TTCN-3 'import' statement is optional.
        * Currently this can only happen when using coding values of a type that
        * has custom or PER coder functions set (the module containing the coder
        * function declarations does not have to be imported to the module that
        * contains the coding operation). */
      I_DEPENDENCY
    };
  private:
    /** Points to the target (imported) module. This is initially NULL;
     *  set during semantic analysis by Ttcn::Imports::chk_uniq() */
    Common::Module *mod;
    /** The importing module (indirectly our owner) */
    Module *my_mod;
    /** Import type: "import all", selective import, import of import, etc. */
    imptype_t imptype;
    /** The name given in the import statement;
     *  hopefully an actual module name */
    Identifier *modid;
    /** The text after "import from name language" */
    string *language_spec;
    /** Recursive import (already deprecated in v3.2.1) */
    bool is_recursive;
    WithAttribPath* w_attrib_path;
    /** Group in which the import statement is located, if any */
    Group* parentgroup;
    visibility_t visibility;
  private:
    /** Copy constructor not implemented. */
    ImpMod(const ImpMod&);
    /** %Assignment not implemented */
    ImpMod& operator=(const ImpMod&);
  public:
    ImpMod(Identifier *p_modid);
    ~ImpMod();
    virtual ImpMod* clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void chk();
    /** Checks the existence of imported symbols and checks import definitions
     * in the imported modules recursively. */
    void chk_imp(ReferenceChain& refch, vector<Common::Module>& moduleStack);
    void set_my_mod(Module *p_mod) { my_mod = p_mod; }
    const Identifier& get_modid() const {return *modid;}
    void set_mod(Common::Module *p_mod) { mod = p_mod; }
    Common::Module *get_mod() const { return mod; }
    /** Returns the imported definition with name \a p_id if it is imported or
     * NULL otherwise.
     * \a loc is used to report an error if it is needed */
    Common::Assignment *get_imported_def(const Identifier& p_source_modid,
        const Identifier& p_id, const Location *loc,
        ReferenceChain* refch, vector<ImpMod>& usedImpMods) const;
    bool has_imported_def(const Identifier& p_source_modid,
            const Identifier& p_id, const Location *loc) const;
    void set_imptype(imptype_t p_imptype) { imptype = p_imptype; }
    void set_language_spec(const char *p_language_spec);
    void set_recursive() { is_recursive = true; }
    void generate_code(output_struct *target);
    virtual void dump(unsigned level) const;
    void set_with_attr(MultiWithAttrib* p_attrib);
    WithAttribPath* get_attrib_path();
    void set_parent_path(WithAttribPath* p_path);
    void set_parent_group(Group* p_group);
    imptype_t get_imptype() {
      return imptype;
    }
    void set_visibility(visibility_t p_visibility){
      visibility = p_visibility;
    }
    visibility_t get_visibility() const{
      return visibility;
    }
  };

  /**
   * Class Imports.
   */
  class Imports : public Node {
  private:
    /** my module */
    Module *my_mod;
    /** imported modules */
    vector<ImpMod> impmods_v;
    /** Indicates whether the import list has been checked. */
    bool checked;

    friend class ImpMod;
  private:
    /** Copy constructor not implemented. */
    Imports(const Imports&);
    /** %Assignment not implemented */
    Imports& operator=(const Imports&);
  public:
    Imports() : Node(), my_mod(0), impmods_v(), checked(false) {}
    virtual ~Imports();
    virtual Imports* clone() const;
    void add_impmod(ImpMod *p_impmod);
    void set_my_mod(Module *p_mod);
    size_t get_imports_size() const {return impmods_v.size();}
    ImpMod* get_impmod(size_t index) const {return impmods_v[index];}
    /** Checks the existence of imported modules and detects duplicated imports
     * from the same module. Initializes \a impmods_m. */
    void chk_uniq();
    /** Checks the existence of imported symbols and checks import definitions
     * in the imported modules recursively. */
    void chk_imp(ReferenceChain& refch, vector<Common::Module>& moduleStack);
    /** Returns \p true if an imported module with the given name exists,
     *  else returns \p false. */
    bool has_impmod_withId(const Identifier& p_id) const;
    /** Returns whether a definition with identifier \a p_id is imported
     * from one or more modules */
    bool has_imported_def(const Identifier& p_id, const Location *loc) const;
    /** Returns the imported definition with name \a p_id if it is
     * unambiguous or NULL otherwise.
     * \a loc is used to report an error if it is needed */
    Common::Assignment *get_imported_def(const Identifier& p_source_modid,
        const Identifier& p_id, const Location *loc, ReferenceChain* refch);
    void get_imported_mods(Module::module_set_t& p_imported_mods) const;
    void generate_code(output_struct *target);
    void generate_code(CodeGenHelper& cgh);
    virtual void dump(unsigned level) const;
  };

  class Definition : public Common::Assignment {
  protected: // many derived classes
    /** Contains the C++ identifier of the definition. If empty the C++
     * identifier is generated from \a id. */
    string genname;
    /** The group it's in, if any */
    Group *parentgroup;
    WithAttribPath* w_attrib_path;
    ErroneousAttributes* erroneous_attrs; // set by chk_erroneous_attr() or NULL
    /** True if function/altstep/default scope, not module scope */
    bool local_scope;

    Definition(const Definition& p)
      : Common::Assignment(p), genname(), parentgroup(0),
        w_attrib_path(0), erroneous_attrs(0), local_scope(false)
      { }

    namedbool has_implicit_omit_attr() const;
  private:
    /// %Assignment disabled
    Definition& operator=(const Definition& p);
  public:
    Definition(asstype_t p_asstype, Identifier *p_id)
      : Common::Assignment(p_asstype, p_id), genname(), parentgroup(0),
        w_attrib_path(0), erroneous_attrs(0), local_scope(false)
      { }
    virtual ~Definition();
    virtual Definition* clone() const = 0;
    virtual void set_fullname(const string& p_fullname);
    virtual bool is_local() const;
    /** Sets the visibility type of the definition */
    void set_visibility(const visibility_t p_visibility)
      { visibilitytype = p_visibility; }
    /** Marks the (template) definition as local to a func/altstep/default */
    inline void set_local() { local_scope = true; }

    virtual string get_genname() const;
    void set_genname(const string& p_genname) { genname = p_genname; }
    /** Check if two definitions are (almost) identical, the type and dimensions
     * must always be identical, the initial values can be different depending
     * on the definition type. If error was reported the return value is false.
     * The initial values (if applicable) may be present/absent, different or
     * unfoldable. The function must be overridden to be used.
     */ 
    virtual bool chk_identical(Definition *p_def);
    /** Parse and check the erroneous attribute data,
      * returns erroneous attributes or NULL */
    static ErroneousAttributes* chk_erroneous_attr(WithAttribPath* p_attrib_path,
      Type* p_type, Scope* p_scope, string p_fullname, bool in_update_stmt);
    /** This code generation is used when this definition is embedded
     * in a statement block. */
    virtual char* generate_code_str(char *str);
    virtual void ilt_generate_code(ILT *ilt);
    /** Generates the C++ initializer sequence for a definition of a component
     * type, appends to \a str and returns the resulting string. The function
     * is used when \a this is realized using the C++ objects of definition
     * \a base_defn inherited from another component type. The function is
     * implemented only for those definitions that can appear within component
     * types, the generic version causes \a FATAL_ERROR. */
    virtual char *generate_code_init_comp(char *str, Definition *base_defn);
    virtual void dump_internal(unsigned level) const;
    virtual void dump(unsigned level) const;
    virtual void set_with_attr(MultiWithAttrib* p_attrib);
    virtual WithAttribPath* get_attrib_path();
    virtual void set_parent_path(WithAttribPath* p_path);
    virtual void set_parent_group(Group* p_group);
    virtual Group* get_parent_group();
  };

  /**
   * TTCN-3 type definition (including signatures and port types).
   */
  class Def_Type : public Definition {
  private:
    Type *type;

    NameBridgingScope bridgeScope;

    /** Copy constructor not implemented */
    Def_Type(const Def_Type& p);
    /** %Assignment disabled */
    Def_Type& operator=(const Def_Type& p);
  public:
    Def_Type(Identifier *p_id, Type *p_type);
    virtual ~Def_Type();
    virtual Def_Type* clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    virtual Setting *get_Setting();
    virtual Type *get_Type();
    virtual void chk();
    virtual void generate_code(output_struct *target, bool clean_up = false);
    virtual void generate_code(CodeGenHelper& cgh);
    virtual void dump_internal(unsigned level) const;
    virtual void set_with_attr(MultiWithAttrib* p_attrib);
    virtual WithAttribPath* get_attrib_path();
    virtual void set_parent_path(WithAttribPath* p_path);
  };

  /**
   * TTCN-3 constant definition.
   */
  class Def_Const : public Definition {
  private:
    Type *type;
    Value *value;
    bool value_under_check;

    /// Copy constructor disabled
    Def_Const(const Def_Const& p);
    /// %Assignment disabled
    Def_Const& operator=(const Def_Const& p);
  public:
    Def_Const(Identifier *p_id, Type *p_type, Value *p_value);
    virtual ~Def_Const();
    virtual Def_Const *clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    virtual Setting *get_Setting();
    virtual Type *get_Type();
    virtual Value *get_Value();
    virtual void chk();
    virtual bool chk_identical(Definition *p_def);
    virtual void generate_code(output_struct *target, bool clean_up = false);
    virtual void generate_code(CodeGenHelper& cgh);
    virtual char* generate_code_str(char *str);
    virtual void ilt_generate_code(ILT *ilt);
    virtual char *generate_code_init_comp(char *str, Definition *base_defn);
    virtual void dump_internal(unsigned level) const;
  };

  /**
   * TTCN-3 external constant definition.
   */
  class Def_ExtConst : public Definition {
  private:
    Type *type;
    bool usage_found;

    /// Copy constructor disabled
    Def_ExtConst(const Def_ExtConst& p);
    /// %Assignment disabled
    Def_ExtConst& operator=(const Def_ExtConst& p);
  public:
    Def_ExtConst(Identifier *p_id, Type *p_type);
    virtual ~Def_ExtConst();
    virtual Def_ExtConst *clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    virtual Type *get_Type();
    virtual void chk();
    virtual void generate_code(output_struct *target, bool clean_up = false);
    virtual void generate_code(CodeGenHelper& cgh);
    virtual void dump_internal(unsigned level) const;
    /** Indicates that the parameter is used at least once. */
    void set_usage_found() { usage_found = true; }
    /** Returns true if the external constant is used at least once. */
    bool is_used() const { return usage_found; }
  };

  /**
   * TTCN-3 module parameter definition.
   */
  class Def_Modulepar : public Definition {
  private:
    Type *type;
    Value*    def_value;
    /// Copy constructor disabled
    Def_Modulepar(const Def_Modulepar& p);
    /// %Assignment disabled
    Def_Modulepar& operator=(const Def_Modulepar& p);
  public:
    Def_Modulepar(Identifier *p_id, Type *p_type, Value *p_defval);
    virtual ~Def_Modulepar();
    virtual Def_Modulepar* clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    virtual Type *get_Type();
    virtual void chk();
    virtual void generate_code(output_struct *target, bool clean_up = false);
    virtual void generate_code(CodeGenHelper& cgh);
    virtual void dump_internal(unsigned level) const;
  };

  /**
   * TTCN-3 template module parameter definition.
   */
  class Def_Modulepar_Template : public Definition {
  private:
    Type *type;
    Template* def_template;
    /// Copy constructor disabled
    Def_Modulepar_Template(const Def_Modulepar_Template& p);
    /// %Assignment disabled
    Def_Modulepar_Template& operator=(const Def_Modulepar_Template& p);
  public:
    Def_Modulepar_Template(Identifier *p_id, Type *p_type, Template *p_defval);
    virtual ~Def_Modulepar_Template();
    virtual Def_Modulepar_Template* clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    virtual Type *get_Type();
    virtual void chk();
    virtual void generate_code(output_struct *target, bool clean_up = false);
    virtual void generate_code(CodeGenHelper& cgh);
    virtual void dump_internal(unsigned level) const;
  };

  /**
   * Def_Template class represents a template definition.
   */
  class Def_Template : public Definition {
  private:
    /* the type of the template */
    Type *type;
    /** The formal parameter list of the template. It is NULL in case of
     * non-parameterized templates. */
    FormalParList *fp_list;
    /** points to the base template reference in case of modified templates,
     * otherwise it is NULL */
    Reference *derived_ref;
    /** shortcut to the base template in case of modified templates,
     * otherwise it is NULL */
    Def_Template *base_template;
    /** Indicates whether the circular recursion chain of modified templates
     * has been checked. */
    bool recurs_deriv_checked;
    /** the body of the template */
    Template *body;
    /** template definition level restriction */
    template_restriction_t template_restriction;
    /** set in chk(), used by code generation,
     *  valid if template_restriction!=TR_NONE */
    bool gen_restriction_check;

    NameBridgingScope bridgeScope;

    /// Copy constructor for Def_Template::clone() only
    Def_Template(const Def_Template& p);
    /// %Assignment disabled
    Def_Template& operator=(const Def_Template& p);
  public:
    Def_Template(template_restriction_t p_template_restriction,
      Identifier *p_id, Type *p_type, FormalParList *p_fpl,
      Reference *p_derived_ref, Template *p_body);
    virtual ~Def_Template();
    virtual Def_Template *clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    virtual Setting *get_Setting();
    virtual Type *get_Type();
    virtual Template *get_Template();
    virtual FormalParList *get_FormalParList();
    virtual void chk();
  private:
    void chk_modified();
    void chk_default() const;
    void chk_recursive_derivation();
  public:
    virtual void generate_code(output_struct *target, bool clean_up = false);
    virtual void generate_code(CodeGenHelper& cgh);
    virtual char* generate_code_str(char *str);
    virtual void ilt_generate_code(ILT *ilt);
    virtual void dump_internal(unsigned level) const;
    template_restriction_t get_template_restriction()
      { return template_restriction; }
  };

  /**
   * Def_Var class represents a variable definition.
   */
  class Def_Var : public Definition {
  private:
    Type *type;
    /** the initial value: optional and maybe incomplete */
    Value *initial_value;

    /// Copy constructor disabled
    Def_Var(const Def_Var& p);
    /// %Assignment disabled
    Def_Var& operator=(const Def_Var& p);
  public:
    Def_Var(Identifier *p_id, Type *p_type, Value *p_initial_value);
    virtual ~Def_Var();
    virtual Def_Var *clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    virtual Type *get_Type();
    virtual void chk();
    virtual bool chk_identical(Definition *p_def);
    virtual void generate_code(output_struct *target, bool clean_up = false);
    virtual void generate_code(CodeGenHelper& cgh);
    virtual char* generate_code_str(char *str);
    virtual void ilt_generate_code(ILT *ilt);
    virtual char *generate_code_init_comp(char *str, Definition *base_defn);
    virtual void dump_internal(unsigned level) const;
  };

  /**
   * Def_Var_Template class represents a template variable (dynamic template)
   * definition.
   */
  class Def_Var_Template : public Definition {
  private:
    Type *type;
    /** the initial value: optional and maybe incomplete */
    Template *initial_value;
    /** optional restriction on this variable */
    template_restriction_t template_restriction;
    /** set in chk(), used by code generation,
     *  valid if template_restriction!=TR_NONE */
    bool gen_restriction_check;

    /// Copy constructor disabled
    Def_Var_Template(const Def_Var_Template& p);
    /// %Assignment disabled
    Def_Var_Template& operator=(const Def_Var_Template& p);
  public:
    Def_Var_Template(Identifier *p_id, Type *p_type, Template *p_initial_value,
                     template_restriction_t p_template_restriction);
    virtual ~Def_Var_Template();
    virtual Def_Var_Template *clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    virtual Type *get_Type();
    virtual void chk();
    virtual bool chk_identical(Definition *p_def);
    virtual void generate_code(output_struct *target, bool clean_up = false);
    virtual void generate_code(CodeGenHelper& cgh);
    virtual char* generate_code_str(char *str);
    virtual void ilt_generate_code(ILT *ilt);
    virtual char *generate_code_init_comp(char *str, Definition *base_defn);
    virtual void dump_internal(unsigned level) const;
    template_restriction_t get_template_restriction()
      { return template_restriction; }
  };

  /**
   * Def_Timer class represents a single timer declaration (e.g. in a
   * TTCN component type).
   */
  class Def_Timer : public Definition {
  private:
    /** Describes the dimensions of a timer array. It is NULL in case
     * of single timer instance. */
    ArrayDimensions *dimensions;
    /** Default duration of the timers. It can be either a single
     * float value or an array of floats. If it is NULL the timer(s)
     * has no default duration. */
    Value *default_duration;

    /// Copy constructor disabled
    Def_Timer(const Def_Timer& p);
    /// %Assignment disabled
    Def_Timer& operator=(const Def_Timer& p);
  public:
    Def_Timer(Identifier *p_id, ArrayDimensions *p_dims, Value *p_dur)
      : Definition(A_TIMER, p_id), dimensions(p_dims),
      default_duration(p_dur) { }
    virtual ~Def_Timer();
    virtual Def_Timer *clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    virtual ArrayDimensions *get_Dimensions();
    virtual void chk();
    virtual bool chk_identical(Definition *p_def);
    /** Returns false if it is sure that the timer referred by array
     * indices \a p_subrefs does not have a default
     * duration. Otherwise it returns true. Argument \a p_subrefs
     * might be NULL when examining a single timer. */
    bool has_default_duration(FieldOrArrayRefs *p_subrefs);
  private:
    /** Checks if the value \a dur is suitable as duration for a single
     * timer. */
    void chk_single_duration(Value *dur);
    /** Checks if the value \a dur is suitable as duration for a timer
     * array.  The function calls itself recursively, argument \a
     * start_dim shall be zero when called from outside. */
    void chk_array_duration(Value *dur, size_t start_dim = 0);
  public:
    virtual void generate_code(output_struct *target, bool clean_up = false);
    virtual void generate_code(CodeGenHelper& cgh);
  private:
    /** Generates a C++ code fragment that sets the default duration
     * of a timer array. The generated code is appended to \a str and
     * the resulting string is returned. The equivalent C++ object of
     * timer array is named \a object_name, the array value containing
     * the default durations is \a dur. The function calls itself
     * recursively, argument \a start_dim shall be zero when called
     * from outside. */
    char *generate_code_array_duration(char *str, const char *object_name,
      Value *dur, size_t start_dim = 0);
  public:
    virtual char* generate_code_str(char *str);
    virtual void ilt_generate_code(ILT *ilt);
    virtual char *generate_code_init_comp(char *str, Definition *base_defn);
    virtual void dump_internal(unsigned level) const;
  };

  /**
   * Def_Port class represents a port declaration (in a component type).
   */
  class Def_Port : public Definition {
  private:
    /** Contains a reference to a TTCN-3 port type */
    Reference *type_ref;
    /** Points to the object describing the port type.
     *
     * Derived from \a type_ref during Def_Port::chk().
     * It can be NULL in case of any error (e.g. the reference points to a
     * non-existent port type or the referenced entity is not a port type. */
    Type *port_type;
    /** Describes the dimensions of a port array.
     * It is NULL in case of single port instance. */
    ArrayDimensions *dimensions;

    /// Copy constructor disabled
    Def_Port(const Def_Port& p);
    /// %Assignment disabled
    Def_Port& operator=(const Def_Port& p);
  public:
    /** Constructor
     *
     * @param p_id identifier (must not be NULL), the name of the port
     * @param p_tref type reference (must not be NULL)
     * @param p_dims array dimensions (NULL for a single port instance)
     */
    Def_Port(Identifier *p_id, Reference *p_tref, ArrayDimensions *p_dims);
    virtual ~Def_Port();
    virtual Def_Port *clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    /** Get the \a port_type
     *
     * @pre chk() has been called (because it computes \a port_type)
     */
    virtual Type *get_Type();
    virtual ArrayDimensions *get_Dimensions();
    virtual void chk();
    virtual bool chk_identical(Definition *p_def);
    virtual void generate_code(output_struct *target, bool clean_up = false);
    virtual void generate_code(CodeGenHelper& cgh);
    virtual char *generate_code_init_comp(char *str, Definition *base_defn);
    virtual void dump_internal(unsigned level) const;
  };

  /**
   * Common base class for function and external function definitions.
   */
  class Def_Function_Base : public Definition {
  public:
    enum prototype_t {
      PROTOTYPE_NONE, /**< no prototype(...) attribute */
      PROTOTYPE_CONVERT, /**< attribute prototype(convert) */
      PROTOTYPE_FAST, /**< attribute prototype(fast) */
      PROTOTYPE_BACKTRACK, /**< attribute prototype(backtrack) */
      PROTOTYPE_SLIDING /**< attribute prototype(sliding) */
    };
  protected: // Def_Function and Def_ExtFunction need access
    /** The formal parameter list of the function. It is never NULL even if
     * the function has empty parameter list. Owned. */
    FormalParList *fp_list;
    /** The return type of the function or NULL in case of no return type.
     *  Owned. */
    Type *return_type;
    /** Identifier of the enc/dec API implemented by the function */
    prototype_t prototype;
    /** Shortcut to the input type if the function implements one of the
     * enc/dec APIs or NULL otherwise. Not owned. */
    Type *input_type;
    /** Shortcut to the output type if the function implements one of the
     * enc/dec APIs or NULL otherwise. Not owned */
    Type *output_type;
    /** optional template restriction on return template value */
    template_restriction_t template_restriction;

    static asstype_t determine_asstype(bool is_external, bool has_return_type,
      bool returns_template);
    /// Copy constructor disabled
    Def_Function_Base(const Def_Function_Base& p);
    /// %Assignment disabled
    Def_Function_Base& operator=(const Def_Function_Base& p);
  public:
    /** Constructor.
     *
     * @param is_external true if external function (the derived type is
     *        Def_ExtFunction), false if a TTCN-3 function (Def_Function)
     * @param p_id the name of the function
     * @param p_fpl formal parameter list
     * @param p_return_type return type (may be NULL)
     * @param returns_template true if the return is a template
     * @param p_template_restriction restriction type
     */
    Def_Function_Base(bool is_external, Identifier *p_id,
      FormalParList *p_fpl, Type *p_return_type, bool returns_template,
      template_restriction_t p_template_restriction);
    virtual ~Def_Function_Base();
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    virtual Type *get_Type();
    virtual FormalParList *get_FormalParList();
    prototype_t get_prototype() const { return prototype; }
    void set_prototype(prototype_t p_prototype) { prototype = p_prototype; }
    const char *get_prototype_name() const;
    void chk_prototype();
    Type *get_input_type();
    Type *get_output_type();
    template_restriction_t get_template_restriction()
      { return template_restriction; }
  };

  /**
   * Def_Function class represents a function definition.
   */
  class Def_Function : public Def_Function_Base {
  private:
    /** The 'runs on' clause (i.e. a reference to a TTCN-3 component type)
     * It is NULL if the function has no 'runs on' clause. */
    Reference *runs_on_ref;
    /** Points to the object describing the component type referred by
     * 'runs on' clause.
     * It is NULL if the function has no 'runs on' clause or \a runs_on_ref is
     * erroneous. */
    Type *runs_on_type;
    /** The 'port' clause (i.e. a reference to a TTCN-3 port type)
     * It is NULL if the function has no 'port' clause. */
    Reference *port_ref;
    /** Points to the object describing the port type referred by
     * 'port' clause.
     * It is NULL if the function has no 'port' clause or \a port_ref is
     * erroneous. */
    Type *port_type;
    /** The body of the function */
    StatementBlock *block;
    /** Indicates whether the function is startable. That is, it can be
     * launched as PTC behaviour as argument of a start test component
     * statement. */
    bool is_startable;
    /** Opts out from location information */
    bool transparent;

    NameBridgingScope bridgeScope;

    /// Copy constructor disabled
    Def_Function(const Def_Function& p);
    /// %Assignment disabled
    Def_Function& operator=(const Def_Function& p);
  public:
    /** Constructor for a TTCN-3 function definition
     *
     * Called from a single location in compiler.y
     *
     * @param p_id function name
     * @param p_fpl formal parameter list
     * @param p_runs_on_ref "runs on", else NULL
     * @param p_port_ref "port", else NULL
     * @param p_return_type return type, may be NULL
     * @param returns_template true if the return value is a template
     * @param p_template_restriction restriction type
     * @param p_block the body of the function
     */
    Def_Function(Identifier *p_id, FormalParList *p_fpl,
                 Reference *p_runs_on_ref, Reference *p_port_ref,
                 Type *p_return_type,
                 bool returns_template,
                 template_restriction_t p_template_restriction,
                 StatementBlock *p_block);
    virtual ~Def_Function();
    virtual Def_Function *clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    virtual Type *get_RunsOnType();
    virtual Type *get_PortType();
    /** Returns a scope that can access the definitions within component type
     * \a comptype and its parent is \a parent_scope.*/
    RunsOnScope *get_runs_on_scope(Type *comptype);
    /** Returns a scope that can access the definitions within port type
     * \a porttype and its parent is \a parent_scope.*/
    PortScope *get_port_scope(Type *porttype);
    virtual void chk();
    /** Checks and returns whether the function is startable.
     * Reports the appropriate error message(s) if not. */
    bool chk_startable();

    bool is_transparent() const { return transparent; }

    // Reuse of clean_up variable to allow or disallow the generation.
    // clean_up is true when it is called from PortTypeBody::generate_code()
    virtual void generate_code(output_struct *target, bool clean_up = false);
    virtual void generate_code(CodeGenHelper& cgh);
    virtual void dump_internal(unsigned level) const;

    virtual void set_parent_path(WithAttribPath* p_path);
  };

  /** RAII class for transparent functions.
   *
   *  Calls to TTCN_Location::update_lineno are written by Ttcn::Statement
   *  by calling Common::Location::update_location_object. These calls
   *  need to be removed inside transparent functions.
   *
   *  It is difficult (impossible) for a Statement to navigate up in the AST
   *  to the function/altstep/testcase/control part that contains it;
   *  instead, the Def_Function sets a static flag inside Common::Location
   *  that says "you are inside a transparent function" while
   *  Def_Function::generate_code function runs.
   */
  class transparency_holder {
    bool old_transparency;
  public:
    /** Sets Common::Location::transparency (but remembers its old vaue)
     *
     * @param df the function definition
     */
    transparency_holder(const Def_Function &df)
    : old_transparency(Common::Location::transparency)
    { Common::Location::transparency = df.is_transparent(); }

    /** Restores the value of Common::Location::transparency */
    ~transparency_holder()
    { Common::Location::transparency = old_transparency; }
  };

  /**
   * Def_ExtFunction class represents an external function definition.
   */
  class Def_ExtFunction : public Def_Function_Base {
  public:
    enum ExternalFunctionType_t {
      EXTFUNC_MANUAL, /**< written manually by the user */
      EXTFUNC_ENCODE, /**< automatically generated encoder function */
      EXTFUNC_DECODE /**< automatically generated decoder function */
    };
  private:
    ExternalFunctionType_t function_type;
    Type::MessageEncodingType_t encoding_type;
    string *encoding_options;
    Ttcn::ErrorBehaviorList *eb_list;
    Ttcn::PrintingType *json_printing;
    /// Copy constructor disabled
    Def_ExtFunction(const Def_ExtFunction& p);
    /// %Assignment disabled
    Def_ExtFunction& operator=(const Def_ExtFunction& p);
  public:
    /** Constructor for an external function definition
     *
     * Called from a single location in compiler.y
     *
     * @param p_id the name
     * @param p_fpl formal parameters
     * @param p_return_type the return type
     * @param returns_template true if it returns a template
     * @param p_template_restriction restriction type
     */
    Def_ExtFunction(Identifier *p_id, FormalParList *p_fpl,
      Type *p_return_type, bool returns_template,
      template_restriction_t p_template_restriction)
      : Def_Function_Base(true, p_id, p_fpl, p_return_type, returns_template,
          p_template_restriction),
      function_type(EXTFUNC_MANUAL), encoding_type(Type::CT_UNDEF),
      encoding_options(0), eb_list(0), json_printing(0) { }
    ~Def_ExtFunction();
    virtual Def_ExtFunction *clone() const;
    virtual void set_fullname(const string& p_fullname);
    void set_encode_parameters(Type::MessageEncodingType_t p_encoding_type,
      string *p_encoding_options);
    void set_decode_parameters(Type::MessageEncodingType_t p_encoding_type,
      string *p_encoding_options);
    void add_eb_list(Ttcn::ErrorBehaviorList *p_eb_list);
    ExternalFunctionType_t get_function_type() const { return function_type; }
  private:
    void chk_function_type();
    void chk_allowed_encode();
  public:
    virtual void chk();
  private:
    char *generate_code_encode(char *str);
    char *generate_code_decode(char *str);
  public:
    virtual void generate_code(output_struct *target, bool clean_up = false);
    /// Just a shim for code splitting
    virtual void generate_code(CodeGenHelper& cgh);
    virtual void dump_internal(unsigned level) const;
    
    /** For JSON encoding and decoding functions only
      * Generates a JSON schema segment containing a reference to the encoded
      * type's schema and any information required to recreate this function.
      * If the schema with the reference already exists, the function's info is
      * inserted in the schema. */
    void generate_json_schema_ref(map<Type*, JSON_Tokenizer>& json_refs);
  };

  /**
   * Represents an altstep definition.
   */
  class Def_Altstep : public Definition {
  private:
    /** The formal parameter list of the altstep. It is never NULL even if
     * the altstep has no parameters. */
    FormalParList *fp_list;
    /** The 'runs on' clause (i.e. a reference to a TTCN-3 component type)
     * It is NULL if the altstep has no 'runs on' clause. */
    Reference *runs_on_ref;
    /** Points to the object describing the component type referred by
     * 'runs on' clause.
     * It is NULL if the altstep has no 'runs on' clause or \a runs_on_ref is
     * erroneous. */
    Type *runs_on_type;
    StatementBlock *sb; /**< contains the local definitions */
    AltGuards *ags;

    NameBridgingScope bridgeScope;

    /// Copy constructor disabled
    Def_Altstep(const Def_Altstep& p);
    /// %Assignment disabled
    Def_Altstep& operator=(const Def_Altstep& p);
  public:
    Def_Altstep(Identifier *p_id, FormalParList *p_fpl,
                Reference *p_runs_on_ref, StatementBlock *p_sb,
                AltGuards *p_ags);
    virtual ~Def_Altstep();
    virtual Def_Altstep *clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    virtual Type *get_RunsOnType();
    virtual FormalParList *get_FormalParList();
    /** Returns a scope that can access the definitions within component type
     * \a comptype and its parent is \a parent_scope.*/
    RunsOnScope *get_runs_on_scope(Type *comptype);
    virtual void chk();
    virtual void generate_code(output_struct *target, bool clean_up = false);
    virtual void generate_code(CodeGenHelper& cgh);
    virtual void dump_internal(unsigned level) const;

    virtual void set_parent_path(WithAttribPath* p_path);
  };

  /**
   * Represents an testcase definition.
   */
  class Def_Testcase : public Definition {
  private:
    /** The formal parameter list of the testcase. It is never NULL even if
     * the testcase has no parameters. */
    FormalParList *fp_list;
    /** The 'runs on' clause (i.e. a reference to a TTCN-3 component type)
     *  It is never NULL. */
    Reference *runs_on_ref;
    /** Points to the object describing the component type referred by
     * 'runs on' clause. It is NULL only if \a runs_on_ref is erroneous. */
    Type *runs_on_type;
    /** The 'system' clause (i.e. a reference to a TTCN-3 component type)
     *  It is NULL if the testcase has no 'system' clause. */
    Reference *system_ref;
    /** Points to the object describing the component type referred by
     * 'system' clause. It is NULL if the testcase has no 'system' clause or
     * \a system_ref is erroneous. */
    Type *system_type;
    StatementBlock *block;

    NameBridgingScope bridgeScope;

    /// Copy constructor disabled
    Def_Testcase(const Def_Testcase& p);
    /// %Assignment disabled
    Def_Testcase& operator=(const Def_Testcase& p);
  public:
    Def_Testcase(Identifier *p_id, FormalParList *p_fpl,
                 Reference *p_runs_on_ref, Reference *p_system_ref,
                 StatementBlock *p_block);
    virtual ~Def_Testcase();
    virtual Def_Testcase *clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    virtual Type *get_RunsOnType();
    Type *get_SystemType();
    virtual FormalParList *get_FormalParList();
    /** Returns a scope that can access the definitions within component type
     * \a comptype and its parent is \a parent_scope.*/
    RunsOnScope *get_runs_on_scope(Type *comptype);
    virtual void chk();
    virtual void generate_code(output_struct *target, bool clean_up = false);
    virtual void generate_code(CodeGenHelper& cgh);
    virtual void dump_internal(unsigned level) const;

    virtual void set_parent_path(WithAttribPath* p_path);
  };

  /** General class to represent formal parameters. The inherited
   * attribute asstype carries the kind and direction of the
   * parameter. */
  class FormalPar : public Definition {
  private:
    Type *type;
    /** Default value of the parameter (optional). If \a checked flag is true
     *  then field \a ap is used otherwise \a ti is active. */
    union {
      TemplateInstance *ti;
      ActualPar *ap;
    } defval;
    /** Points to the formal parameter list that \a this belongs to. */
    FormalParList *my_parlist;
    /** Flag that indicates whether the value of the parameter is overwritten
     * within the function/altstep/testcase body.
     * Used only in case of `in' value or template parameters. */
    bool used_as_lvalue;
    /** restriction on template value */
    template_restriction_t template_restriction;
    /** normal, lazy or fuzzy evaluation parametrization should be used */
    param_eval_t eval;
    /** Flag that indicates whether the C++ code for the parameter's default
      * value has been generated or not. */
    bool defval_generated;
    /** Flag that indicates whether the parameter is used in the function/
     * altstep/testcase/parameterized template body. */
    bool usage_found;

    /// Copy constructor disabled
    FormalPar(const FormalPar& p);
    /// %Assignment disabled
    FormalPar& operator=(const FormalPar& p);
  public:
    FormalPar(asstype_t p_asstype, Type *p_type, Identifier* p_name,
      TemplateInstance *p_defval, param_eval_t p_eval = NORMAL_EVAL);
    FormalPar(asstype_t p_asstype,
      template_restriction_t p_template_restriction,
      Type *p_type, Identifier* p_name, TemplateInstance *p_defval,
      param_eval_t p_eval = NORMAL_EVAL);
    FormalPar(asstype_t p_asstype, Identifier* p_name,
      TemplateInstance *p_defval);
    ~FormalPar();
    virtual FormalPar *clone() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    /** Always true. Formal parameters are considered as local definitions
     * even if their scope is the module definitions. */
    virtual bool is_local() const;
    virtual Type *get_Type();
    virtual void chk();
    bool has_defval() const;
    bool has_notused_defval() const;
    /** Get the default value.
     * \pre chk() has been called (checked==true) */
    ActualPar *get_defval() const;
    void set_defval(ActualPar *defpar);
    void set_my_parlist(FormalParList *p_parlist) { my_parlist = p_parlist; }
    FormalParList *get_my_parlist() const { return my_parlist; }
    ActualPar *chk_actual_par(TemplateInstance *actual_par,
      Type::expected_value_t exp_val);
  private:
    ActualPar *chk_actual_par_value(TemplateInstance *actual_par,
      Type::expected_value_t exp_val);
    ActualPar *chk_actual_par_template(TemplateInstance *actual_par,
      Type::expected_value_t exp_val);
    ActualPar *chk_actual_par_by_ref(TemplateInstance *actual_par,
      bool is_template, Type::expected_value_t exp_val);
    ActualPar *chk_actual_par_timer(TemplateInstance *actual_par,
      Type::expected_value_t exp_val);
    ActualPar *chk_actual_par_port(TemplateInstance *actual_par,
      Type::expected_value_t exp_val);
  public:
    /** Checks whether the value of the parameter may be modified in the body
     * of the parameterized definition (in assignment, port redirect or passing
     * it further as 'out' or 'inout' parameter). Applicable to `in' value or
     * template parameters only. Note that formal parameters of templates cannot
     * be modified. If the modification is allowed a flag is set, which is
     * considered at C++ code generation. Argument \a p_loc is used for error
     * reporting. */
    virtual void use_as_lvalue(const Location& p_loc);
    bool get_used_as_lvalue() const { return used_as_lvalue; }
    /** Partially generates the C++ object that represents the default value for
      * the parameter (if present). The object's declaration is not generated,
      * only its value assignment. */
    char* generate_code_defval(char* str);
    /** Generates the C++ object that represents the default value for the
     * parameter (if present). */
    virtual void generate_code_defval(output_struct *target, bool clean_up = false);
    /** Generates the C++ equivalent of the formal parameter, appends it to
     * \a str and returns the resulting string.
     * The name of the parameter is not displayed if the parameter is unused
     * (unless forced).
     * @param display_unused forces the display of the parameter name */
    char *generate_code_fpar(char *str, bool display_unused = false);
    /** Generates a C++ statement that defines an object (variable) for the
     * formal parameter, appends it to \a str and returns the resulting
     * string. The name of the object is constructed from \a p_prefix and the
     * parameter identifier.
     * The \a refch parameter is needed when the code for start_ptc_function is
     * generated, because reference is generated in case of inout parameters. */
    char *generate_code_object(char *str, const char *p_prefix,
      char refch = '&', bool gen_init = false);
    /** Generates a C++ statement that instantiates a shadow object for the
     * parameter when necessary. It is used when the value of an 'in' value or
     * template parameter is overwritten within the function body. */
    char *generate_shadow_object(char *str) const;
   /** Generates a C++ statement that calls a function that sets the parameter unbound
     *  It is used when the value of an 'out' value  */
    char *generate_code_set_unbound(char *str) const;
    virtual void dump_internal(unsigned level) const;
    template_restriction_t get_template_restriction()
      { return template_restriction; }
    virtual param_eval_t get_eval_type() const { return eval; }
    // code generation: get the C++ string that refers to the formal parameter
    // adds a casting to data type if wrapped into a lazy param
    string get_reference_name(Scope* scope) const;
    /** Indicates that the parameter is used at least once. */
    void set_usage_found() { usage_found = true; }
  };

  /** Class to represent a list of formal parameters. Owned by a
   *  Def_Template, Def_Function_Base, Def_Altstep or Def_Testcase. */
  class FormalParList : public Scope, public Location {
  private:
    vector<FormalPar> pars_v;
    /** Map names to the formal parameters in pars_v. Filled by
     *  FormalParList::chk*/
    map<string, FormalPar> pars_m;
    /** Indicates the minimal number of actual parameters that must be present
     * in parameterized references. Could be less than the size of pars_v
     * if some parameters have default values. */
    size_t min_nof_pars;
    /** Points to the definition that the parameter list belongs to. */
    Definition *my_def;
    bool checked;
    bool is_startable;

    /** Copy constructor. For FormalParList::clone() only. */
    FormalParList(const FormalParList& p);
    /** %Assignment disallowed. */
    FormalParList& operator=(const FormalParList& p);
  public:
    FormalParList() : Scope(), Location(), pars_v(), pars_m()
    , min_nof_pars(0), my_def(0), checked(false), is_startable(false) {}
    ~FormalParList();
    virtual FormalParList *clone() const;
    virtual void set_fullname(const string& p_fullname);
    /** Sets the parent scope and the scope of parameters to \a p_scope. */
    virtual void set_my_scope(Scope *p_scope);
    void set_my_def(Definition *p_def) { my_def = p_def; }
    Definition *get_my_def() const { return my_def; }
    void add_fp(FormalPar *p_fp);
    size_t get_nof_fps() const { return pars_v.size(); }
    bool has_notused_defval() const;
    bool has_only_default_values() const;
    bool has_fp_withName(const Identifier& p_name);
    FormalPar *get_fp_byName(const Identifier& p_name);
    FormalPar *get_fp_byIndex(size_t n) const { return pars_v[n]; }
    bool get_startability();
    virtual Common::Assignment *get_ass_bySRef(Common::Ref_simple *p_ref);
    virtual bool has_ass_withId(const Identifier& p_id);
    /** Checks the parameter list, which belongs to definition of type
     * \a deftype. */
    void chk(Definition::asstype_t deftype);
    void chk_noLazyFuzzyParams();
    /** Checks the parameter list for startability: reports error if the owner
     * function cannot be started on a PTC. Used by functions and function
     * types. Parameter \a p_what shall contain "Function" or "Function type",
     * \a p_name shall contain the name of the function or function type. */
    void chk_startability(const char *p_what, const char *p_name);
    /** Checks the compatibility of two formal parameter list.
     *  They are compatible if every parameter is compatible, has the same
     *    attribute, and name */
    void chk_compatibility(FormalParList* p_fp_list, const char* where);
    /** Checks the parsed actual parameters in \a p_tis against \a this.
     * The actual parameters that are the result of transformation are added to
     * \a p_aplist (which is usually empty when this function is called).
     * \return true in case of any error, false after a successful check. */
    bool chk_actual_parlist(TemplateInstances *p_tis, ActualParList *p_aplist);
    /** Fold named parameters into the unnamed (order-based) param list.
     *
     * Named parameters are checked against the formal parameters.
     * Found ones are transferred into the unnamed param list at the
     * appropriate index.
     *
     * @param p_paps actual parameters from the parser (Bison); contains
     *        both named and unnamed parameters
     * @param p_aplist actual parameters
     * @return the result of calling chk_actual_parlist, above:
     *         true if error, false if the check is successful
     */
    bool fold_named_and_chk(ParsedActualParameters *p_paps, ActualParList *p_aplist);
    bool chk_activate_argument(ActualParList *p_aplist,
                               const char* p_description);
    void set_genname(const string& p_prefix);
    /** Generates the C++ equivalent of the formal parameter list, appends it
     * to \a str and returns the resulting string.
     * The names of unused parameters are not displayed (unless forced).
     * @param display_unused forces the display of parameter names (an amount
     * equal to this parameter are forced, starting from the first) */
    char *generate_code(char *str, size_t display_unused = 0);
    /** Partially generates the C++ objects that represent the default value for
      * the parameters (if present). The objects' declarations are not generated,
      * only their value assignments. */
    char* generate_code_defval(char* str);
    /** Generates the C++ objects that represent the default values for the
     * parameters (if present). */
    void generate_code_defval(output_struct *target);
    /** Generates a C++ actual parameter list for wrapper functions, appends it
     * to \a str and returns the resulting string. It contains the
     * comma-separated list of parameter identifiers prefixed by \a p_prefix. */
    char *generate_code_actual_parlist(char *str, const char *p_prefix);
    /** Generates a C++ code sequence that defines an object (variable) for
     * each formal parameter (which is used in wrapper functions), appends it
     * to \a str and returns the resulting string. The name of each object is
     * constructed from \a p_prefix and the parameter identifier.
     * The \a refch parameter is needed when the code for start_ptc_function is
     * generated, because reference is generated in case of inout parameters. */
    char *generate_code_object(char *str, const char *p_prefix,
      char refch = '&', bool gen_init = false);
    /** Generates the C++ shadow objects for all parameters. */
    char *generate_shadow_objects(char *str) const;
    char *generate_code_set_unbound(char *str) const;
    void dump(unsigned level) const;
  };

} // namespace Ttcn

#endif // _Ttcn_AST_HH
