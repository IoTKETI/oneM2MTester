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
#ifndef _Asn_AST_HH
#define _Asn_AST_HH

#include "../AST.hh"
#include "../Int.hh"
#include "Object0.hh"

namespace Common {
class CodeGenHelper;
}

/**
 * This namespace contains things related to ASN.1 compiler.
 */
namespace Asn {

  /**
   * \addtogroup AST
   *
   * @{
   */

  class Assignments;
  /**
   * This is used when parsing internal stuff (e.g., EXTERNAL,
   * EMBEDDED PDV, etc.).
   */
  extern Assignments *parsed_assignments;

  using namespace Common;

  struct TagDefault;

  class Assignment;
  class Module;
  class Symbols;
  class Exports;
  class ImpMod;
  class Imports;
  class Ass_Undef;
  class Ass_T;
  class Ass_V;
  class Ass_VS;
  class Ass_OC;
  class Ass_O;
  class Ass_OS;

  /* not defined, only declared in this file */

  class Block;
  class TokenBuf;
  class ValueSet;
  class ObjectClass;
  class Object;
  class ObjectSet;

  /**
   * Struct to represent default tagging.
   */
  struct TagDefault {
    enum tagdef_t {
      EXPLICIT, /**< EXPLICIT TAGS */
      IMPLICIT, /**< IMPLICIT TAGS */
      AUTOMATIC /**< AUTOMATIC TAGS */
    };
  };

  /**
   * Helper-class for parameterized assignments.
   */
  class Ass_pard : public Node {
  private:
    Assignment *my_ass; // link to...
    Block *parlist_block; /**< parameter list */
    /** if the block (parlist) is pre-parsed then it is deleted, and
     *  the content is put into these vectors. */
    vector<Identifier> dummyrefs;
    /** contains one tokenbuf for each parameter; e.g. for parameter
     *  "INTEGER : i" it contains "i INTEGER ::=" */
    vector<TokenBuf> governors;
    /** Instance counters: for each target module a separate counter is
     * maintained to get deterministic instance numbers in case of incremental
     * compilation. */
    map<Common::Module*, size_t> inst_cnts;

    Ass_pard(const Ass_pard& p);
    Ass_pard& operator=(const Ass_pard& p);
    /** Split the formal parameterlist */
    void preparse_pars();
  public:
    Ass_pard(Block *p_parlist_block);
    virtual ~Ass_pard();
    virtual Ass_pard* clone() const
    {return new Ass_pard(*this);}
    void set_my_ass(Assignment *p_my_ass) {my_ass=p_my_ass;}
    /** Returns the number of formal parameters. 0 means the parameter
     *  list is erroneous. */
    size_t get_nof_pars();
    const Identifier& get_nth_dummyref(size_t i);
    TokenBuf* clone_nth_governor(size_t i);
    /** Returns the next instance number for target module \a p_mod */
    size_t new_instnum(Common::Module *p_mod);
  };

  /**
   * Abstract class to represent different kinds of assignments.
   */
  class Assignment : public Common::Assignment {
  protected: // many classes derived
    Ass_pard *ass_pard;
    bool dontgen;

    Assignment(const Assignment& p);
    Assignment& operator=(const Assignment& p);
    virtual string get_genname() const;
    virtual Assignment* new_instance0();
  public:
    Assignment(asstype_t p_asstype, Identifier *p_id, Ass_pard *p_ass_pard);
    virtual ~Assignment();
    virtual Assignment* clone() const =0;
    virtual bool is_asstype(asstype_t p_asstype, ReferenceChain* refch=0);
    virtual void set_right_scope(Scope *p_scope) =0;
    void set_dontgen() {dontgen=true;}
    /** Returns 0 if assignment is not parameterized! */
    virtual Ass_pard* get_ass_pard() const { return ass_pard; }
    /** Returns 0 if this assignment is not parameterized! */
    Assignment* new_instance(Common::Module *p_mod);
    virtual Type* get_Type();
    virtual Value* get_Value();
    virtual ValueSet* get_ValueSet();
    virtual ObjectClass* get_ObjectClass();
    virtual Object* get_Object();
    virtual ObjectSet* get_ObjectSet();
    virtual void chk();
    virtual void dump(unsigned level) const;
  };

  /**
   * Class to store assignments. It also contains the ASN-related
   * pre-defined (so-called "useful") stuff.
   */
  class Assignments : public Common::Assignments {
  private:
    /** Special assignments. */
    static Module *_spec_asss;
  public:
    static void create_spec_asss();
    static void destroy_spec_asss();
    /** Returns whether module \a p_mod is the module that contains the
     * special assignments */
    static bool is_spec_asss(Common::Module *p_mod);
  private:
    /** The assignments. */
    map<string, Assignment> asss_m;
    vector<Assignment> asss_v;
    /** Indicates whether the uniqueness of identifiers has been checked. */
    bool checked;

    Assignments(const Assignments&);
  public:
    /** Constructor. */
    Assignments() : Common::Assignments(), asss_m(), asss_v(), checked(false)
    { }
    /** Destructor. */
    virtual ~Assignments();
    virtual Assignments* clone() const;
    /** Sets the fullname. */
    virtual void set_fullname(const string& p_fullname);
    /** Also looks into special assignments. */
    virtual bool has_local_ass_withId(const Identifier& p_id);
    /** Also looks into special assignments. */
    virtual Assignment* get_local_ass_byId(const Identifier& p_id);
    virtual size_t get_nof_asss();
    virtual Common::Assignment* get_ass_byIndex(size_t p_i);
    /** Adds the Assignment. If \a checked flag is true and the id of the new
     * assignment is not unique it reports the error. */
    void add_ass(Assignment *p_ass);
    /** Checks the uniqueness of identifiers. */
    void chk_uniq();
    /** Checks all assignments. */
    void chk();
    /** Sets the scope for the right side of assignments. Used in
     *  parametrization. There is a strange situation that the left
     *  and right sides of assignments can have different parent
     *  scopes. */
    void set_right_scope(Scope *p_scope);
    void generate_code(output_struct* target);
    void generate_code(CodeGenHelper& cgh);
    void dump(unsigned level) const;
  };

  /**
   * Class to represent ASN-modules.
   */
  class Module : public Common::Module {
  private:
    /** default tagging */
    TagDefault::tagdef_t tagdef;
    /** extensibility implied */
    bool extens_impl;
    /** exported stuff */
    Exports *exp;
    /** imported stuff. */
    Imports *imp;
    /** assignments of the module */
    Assignments *asss;
  private:
    /** Copy constructor not implemented. */
    Module(const Module&);
    /** Assignment not implemented */
    Module& operator=(const Module&);
  public:
    Module(Identifier *p_modid, TagDefault::tagdef_t p_tagdef,
           bool p_extens_impl, Exports *p_exp, Imports *p_imp,
	   Assignments *p_asss);
    virtual ~Module();
    virtual Module* clone() const;
    virtual Common::Assignment* importAssignment(
      const Identifier& p_source_modid, const Identifier& p_id) const;
    /** Sets the fullname. */
    virtual void set_fullname(const string& p_fullname);
    virtual Common::Assignments* get_scope_asss();
    virtual bool has_imported_ass_withId(const Identifier& p_id);
    /** Returns the assignment referenced by p_ref. Appends an error
     *  message an returns 0 if assignment is not found. */
    virtual Common::Assignment* get_ass_bySRef(Ref_simple *p_ref);
    virtual Assignments *get_asss();
    virtual bool exports_sym(const Identifier& p_id);
    virtual void chk_imp(ReferenceChain& refch, vector<Common::Module>& moduleStack);
    virtual void chk();
  private:
    virtual void get_imported_mods(module_set_t& p_imported_mods);
    virtual bool has_circular_import();
    virtual void generate_code_internal(CodeGenHelper& cgh);
  public:
    virtual void dump(unsigned level) const;
    void add_ass(Assignment *p_ass);
    TagDefault::tagdef_t get_tagdef() const { return tagdef; }
    
    /** Generates JSON schema segments for the types defined in the modules,
      * and references to these types.
      *
      * @param json JSON document containing the main schema, schema segments for 
      * the types will be inserted here
      * @param json_refs map of JSON documents containing the references to each type */
    virtual void generate_json_schema(JSON_Tokenizer& json, map<Type*, JSON_Tokenizer>& json_refs);
    
    /** Does nothing. Debugger initialization functions are not generated for
      * ASN.1 modules. */
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
   * SymbolList.
   * Used only while parsing a module (and building the AST).
   */
  class Symbols : public Node {
    friend class Exports;
    friend class ImpMod;
    friend class Imports;
  private:
    /** string => Identifier container */
    typedef map<string, Identifier> syms_t;
    /** temporary container for symbols (no uniqueness checks) */
    vector<Identifier> syms_v;
    /** the symbols */
    syms_t syms_m;
  private:
    /** Copy constructor not implemented. */
    Symbols(const Symbols&);
    /** Assignment not implemented. */
    Symbols& operator=(const Symbols&);
  public:
    /** Default constructor. */
    Symbols() : Node(), syms_v(), syms_m() { }
    /** Destructor. */
    virtual ~Symbols();
    /** Not implemented. */
    virtual Symbols* clone() const;
    /** Adds \a p_id to the symlist. */
    void add_sym(Identifier *p_id);
    /** Ensures uniqueness of identifiers in syms */
    void chk_uniq(const Location& p_loc);
  };

  /**
   * Exported symbols of a module.
   */
  class Exports : public Node, public Location {
  private:
    bool checked;
    /** string => Identifier container */
    /** my module */
    Module *my_mod;
    /** exports all */
    bool expall;
    /** exported symbols */
    Symbols *symbols;
  private:
    /** Copy constructor not implemented. */
    Exports(const Exports&);
    /** Assignment not implemented */
    Exports& operator=(const Exports&);
  public:
    /** Constructor used when the module exports all. */
    Exports(bool p_expall=false);
    /** Constructor used when there is a SymbolList. */
    Exports(Symbols *p_symlist);
    /** Destructor. */
    virtual ~Exports();
    /** Not implemented. */
    virtual Exports* clone() const;
    /** Sets the internal pointer to its module. */
    void set_my_mod(Module *p_mod) {my_mod=p_mod;}
    /** Returns true if a symbol with identifier \a p_id is
     *  exported. */
    bool exports_sym(const Identifier& p_id);
    /** Checks whether the exported symbols are defined in the module. */
    void chk_exp();
  };

  /**
   * Imported module.
   */
  class ImpMod : public Node, public Location {
    friend class Imports;
  private:
    /** my module */
    Module *my_mod;
    /** identifier of imported module */
    Identifier *modid;
    /** pointer to the imported module */
    Common::Module *mod;
    /** imported symbols */
    Symbols *symbols;
  private:
    /** Copy constructor not implemented. */
    ImpMod(const ImpMod&);
    /** Assignment not implemented */
    ImpMod& operator=(const ImpMod&);
  public:
    /** Constructor. */
    ImpMod(Identifier *p_modid, Symbols *p_symlist);
    /** Destructor. */
    virtual ~ImpMod();
    /** Not implemented. */
    virtual ImpMod* clone() const;
    /** Sets the internal pointer to its module. */
    void set_my_mod(Module *p_mod) {my_mod=p_mod;}
    /** Gets the module id of imported module. */
    const Identifier& get_modid() const {return *modid;}
    void set_mod(Common::Module *p_mod) { mod = p_mod; }
    Common::Module *get_mod() const { return mod; }
    bool has_sym(const Identifier& p_id) const;
    /** Checks whether the imported symbols exist. Appends error
     *  messages when needed. */
    void chk_imp(ReferenceChain& refch);
    void generate_code(output_struct *target);
  };

  /**
   * Imported modules.
   */
  class Imports : public Node, public Location {
    friend class Module;
  private:
    /** my module */
    Module *my_mod;
    /** imported modules */
    vector<Asn::ImpMod> impmods_v;
    map<string, Asn::ImpMod> impmods;
    /** these symbols are mentioned only once in IMPORTS */
    map<string, Common::Module> impsyms_1;
    /** these symbols are mentioned more than once in IMPORTS. This
     *  map is used as a set. */
    map<string, void> impsyms_m;
    /** Indicates whether the import list has been checked. */
    bool checked;
    /** Indicates whether the owner module is a part of a circular import
     * chain. */
    bool is_circular;
  private:
    /** Copy constructor not implemented. */
    Imports(const Imports&);
    /** Assignment not implemented */
    Imports& operator=(const Imports&);
  public:
    /** Constructor. */
    Imports()
    : Node(), my_mod(0), impmods_v(), impmods(), impsyms_1(), impsyms_m(),
      checked(false), is_circular(false) {}
    /** Destructor. */
    virtual ~Imports();
    /** Not implemented. */
    virtual Imports* clone() const;
    /** Adds the ImpMod. If a module with that id already exists,
     *  throws an Error_AST_uniq exception. */
    void add_impmod(ImpMod *p_impmod);
    /** Sets the internal pointer my_mod to \a p_mod. */
    void set_my_mod(Module *p_mod);
    /** Recursive check. Checks whether the modules with the id exist,
     *  then checks that ImpMod. */
    void chk_imp(ReferenceChain& refch);
    /** Returns whether symbols from module with identifier \a p_id
     *  are imported. */
    bool has_impmod_withId(const Identifier& p_id) const
      { return impmods.has_key(p_id.get_name()); }
    const ImpMod *get_impmod_byId(const Identifier& p_id) const
      { return impmods[p_id.get_name()]; }
    /** Returns whether a symbol with identifier \a p_id is imported from one
     * or more modules */
    bool has_impsym_withId(const Identifier& p_id) const;
    void get_imported_mods(Module::module_set_t& p_imported_mods);
    bool get_is_circular() const { return is_circular; }
    void generate_code(output_struct *target);
  };

  /**
   * Undefined assignment.
   */
  class Ass_Undef : public Assignment {
  private:
    Node *left, *right;
    Scope *right_scope;
    /** the classified assignment */
    Assignment *ass;

    void classify_ass(ReferenceChain *refch=0);
    virtual Assignment* new_instance0();
    Ass_Undef(const Ass_Undef& p);
    /// Assignment disabled
    Ass_Undef& operator=(const Ass_Undef& p);
  public:
    Ass_Undef(Identifier *p_id, Ass_pard *p_ass_pard,
              Node *p_left, Node *p_right);
    virtual ~Ass_Undef();
    virtual Assignment* clone() const;
    virtual asstype_t get_asstype() const;
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_right_scope(Scope *p_scope);
    virtual bool is_asstype(asstype_t p_asstype, ReferenceChain* refch=0);
    virtual Ass_pard* get_ass_pard() const;
    virtual Setting* get_Setting();
    virtual Type* get_Type();
    virtual Value* get_Value();
    virtual ValueSet* get_ValueSet();
    virtual ObjectClass* get_ObjectClass();
    virtual Object* get_Object();
    virtual ObjectSet* get_ObjectSet();
    virtual void chk();
    virtual void generate_code(output_struct *target, bool clean_up = false);
    virtual void generate_code(CodeGenHelper& cgh);
    virtual void dump(unsigned level) const;
  private:
    bool _error_if_pard();
  };

  /**
   * Erroneous assignment.
   */
  class Ass_Error : public Assignment {
  private:
    Setting *setting_error;
    Type *type_error;
    Value *value_error;

    /// Copy constructor not implemented (even though clone works)
    Ass_Error(const Ass_Error& p);
    /// Assignment disabled
    Ass_Error& operator=(const Ass_Error& p);
    virtual Assignment* new_instance0();
  public:
    Ass_Error(Identifier *p_id, Ass_pard *p_ass_pard);
    virtual ~Ass_Error();
    virtual Assignment* clone() const;
    virtual bool is_asstype(asstype_t p_asstype, ReferenceChain* refch=0);
    virtual void set_right_scope(Scope *) {}
    virtual Setting* get_Setting();
    virtual Type* get_Type();
    virtual Value* get_Value();
    virtual ValueSet* get_ValueSet();
    virtual ObjectClass* get_ObjectClass();
    virtual Object* get_Object();
    virtual ObjectSet* get_ObjectSet();
    virtual void chk();
    virtual void dump(unsigned level) const;
  };

  /**
   * Type assignment.
   */
  class Ass_T : public Assignment {
  private:
    Type *right;

    virtual Assignment* new_instance0();
    /// Copy constructor for clone() only
    Ass_T(const Ass_T& p);
    /// Assignment disabled
    Ass_T& operator=(const Ass_T& p);
  public:
    Ass_T(Identifier *p_id, Ass_pard *p_ass_pard, Type *p_right);
    virtual ~Ass_T();
    virtual Ass_T* clone() const
    {return new Ass_T(*this);}
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_right_scope(Scope *p_scope);
    virtual Setting* get_Setting() {return get_Type();}
    virtual Type* get_Type();
    virtual void chk();
    virtual void generate_code(output_struct *target, bool clean_up = false);
    virtual void generate_code(CodeGenHelper& cgh);
    virtual void dump(unsigned level) const;
  };

  /**
   * Value assignment.
   */
  class Ass_V : public Assignment {
  private:
    Type *left;
    Value *right;

    virtual Assignment* new_instance0();
    /// Copy constructor for clone() only
    Ass_V(const Ass_V& p);
    /// Assignment disabled
    Ass_V& operator=(const Ass_V& p);
  public:
    Ass_V(Identifier *p_id, Ass_pard *p_ass_pard,
          Type *p_left, Value *p_right);
    virtual ~Ass_V();
    virtual Ass_V* clone() const
    {return new Ass_V(*this);}
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_right_scope(Scope *p_scope);
    virtual Setting* get_Setting() {return get_Value();}
    virtual Type* get_Type();
    virtual Value* get_Value();
    virtual void chk();
    virtual void generate_code(output_struct *target, bool clean_up = false);
    virtual void generate_code(CodeGenHelper& cgh);
    virtual void dump(unsigned level) const;
  };

  /**
   * ValueSet assignment.
   */
  class Ass_VS : public Assignment {
  private:
    Type *left;
    Block *right;
    Scope *right_scope;

    virtual Assignment* new_instance0();
    /// Copy constructor for clone() only
    Ass_VS(const Ass_VS& p);
    /// Assignment disabled
    Ass_VS& operator=(const Ass_VS& p);
  public:
    Ass_VS(Identifier *p_id, Ass_pard *p_ass_pard,
           Type *p_left, Block *p_right);
    virtual ~Ass_VS();
    virtual Ass_VS* clone() const
    {return new Ass_VS(*this);}
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_right_scope(Scope *p_scope);
    virtual Setting* get_Setting() {return get_Type();}
    virtual Type* get_Type();
    virtual void chk();
    virtual void generate_code(output_struct *target, bool clean_up = false);
    virtual void generate_code(CodeGenHelper& cgh);
    virtual void dump(unsigned level) const;
  };

  /**
   * ObjectClass assignment.
   */
  class Ass_OC : public Assignment {
  private:
    ObjectClass *right;

    virtual Assignment* new_instance0();
    /// Copy constructor for clone() only
    Ass_OC(const Ass_OC& p);
    /// Assignment disabled
    Ass_OC& operator=(const Ass_OC& p);
  public:
    Ass_OC(Identifier *p_id, Ass_pard *p_ass_pard, ObjectClass *p_right);
    virtual ~Ass_OC();
    virtual Ass_OC* clone() const
    {return new Ass_OC(*this);}
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_right_scope(Scope *p_scope);
    virtual void chk();
    virtual Setting* get_Setting() {return get_ObjectClass();}
    virtual ObjectClass* get_ObjectClass();
    virtual void generate_code(output_struct *target, bool clean_up = false);
    virtual void generate_code(CodeGenHelper& cgh);
    virtual void dump(unsigned level) const;
  };

  /**
   * Object assignment.
   */
  class Ass_O : public Assignment {
  private:
    ObjectClass *left;
    Object *right;

    virtual Assignment* new_instance0();
    /// Copy constructor for clone() only
    Ass_O(const Ass_O& p);
    /// Assignment disabled
    Ass_O& operator=(const Ass_O& p);
  public:
    Ass_O(Identifier *p_id, Ass_pard *p_ass_pard,
          ObjectClass *p_left, Object *p_right);
    virtual ~Ass_O();
    virtual Ass_O* clone() const
    {return new Ass_O(*this);}
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_right_scope(Scope *p_scope);
    virtual Setting* get_Setting() {return get_Object();}
    //virtual ObjectClass* get_ObjectClass();
    virtual Object* get_Object();
    virtual void chk();
    virtual void generate_code(output_struct *target, bool clean_up = false);
    virtual void generate_code(CodeGenHelper& cgh);
    virtual void dump(unsigned level) const;
  };

  /**
   * ObjectSet assignment.
   */
  class Ass_OS : public Assignment {
  private:
    ObjectClass *left;
    ObjectSet *right;

    virtual Assignment* new_instance0();
    /// Copy constructor for clone() only
    Ass_OS(const Ass_OS& p);
    /// Assignment disabled
    Ass_OS& operator=(const Ass_OS& p);
  public:
    Ass_OS(Identifier *p_id, Ass_pard *p_ass_pard,
           ObjectClass *p_left, ObjectSet *p_right);
    virtual ~Ass_OS();
    virtual Ass_OS* clone() const
    {return new Ass_OS(*this);}
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_right_scope(Scope *p_scope);
    virtual Setting* get_Setting() {return get_ObjectSet();}
    //virtual ObjectClass* get_ObjectClass();
    virtual ObjectSet* get_ObjectSet();
    virtual void chk();
    virtual void generate_code(output_struct *target, bool clean_up = false);
    virtual void generate_code(CodeGenHelper& cgh);
    virtual void dump(unsigned level) const;
  };

  /** @} end of AST group */

} // namespace Asn

#endif // _Asn_AST_HH
