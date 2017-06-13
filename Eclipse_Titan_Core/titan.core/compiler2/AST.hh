/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   
 *   Baji, Laszlo
 *   Balasko, Jeno
 *   Baranyi, Botond
 *   Cserveni, Akos
 *   Czerman, Oliver
 *   Delic, Adam
 *   Forstner, Matyas
 *   Gecse, Roland
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *   Zalanyi, Balazs Andor
 *   Pandi, Krisztian
 *
 ******************************************************************************/
#ifndef _Common_AST_HH
#define _Common_AST_HH


#undef new
#include <new>
#include <set>
#include <string>
#include "../common/dbgnew.hh"

#include "../common/ModuleVersion.hh"
#include "CompilerError.hh"
#include "Setting.hh"
#include "Type.hh"
#include "Value.hh"
#include "vector.hh"

namespace Ttcn {
  // not defined here
  class Template;
  class FormalParList;
  class ArrayDimensions;
  class Group;
  class Module;
} // namespace Ttcn

// not defined here
class JSON_Tokenizer;

/**
 * This namespace contains things used both in TTCN-3
 * and ASN.1 compiler.
 */
namespace Common {

  /**
   * \defgroup AST AST
   *
   * These classes provide a unified interface for language entities
   * present both in TTCN and ASN. (Most of) the classes defined here
   * are abstract, and have a descendant (with the same name) in the
   * namespaces Asn and Ttcn.
   *
   * @{
   */

  class Identifier;
  class Modules;
  class Module;
  class Assignments;
  class Assignment;
  class TypeConv;
  class CodeGenHelper;

  enum visibility_t {
    NOCHANGE, /* for parser usage only; default visibility */
    PUBLIC,   /* public visibility */
    PRIVATE,  /* private visibility */
    FRIEND    /* friend visibility */
  };

  /**
   * Class for storing modules.
   *
   * This is, in effect, the "root" of the AST: there is only one instance
   * of this class (declared in main.cc).
   */
  class Modules : public Node {
  private:
    /** Containers to store the modules. */
    vector<Module> mods_v;
    map<string, Module> mods_m;
    
    /** Contains info needed for delayed type encoding checks */
    struct type_enc_t {
      Type* t;
      Module* mod;
      bool enc;
    };
    static vector<type_enc_t> delayed_type_enc_v;

    /** Not implemented */
    Modules(const Modules& p);
    Modules& operator=(const Modules& p);
  public:
    /** The default constructor. */
    Modules();
    /** The destructor. */
    ~Modules();
    /** Not implemented: generates FATAL_ERROR */
    virtual Modules *clone() const;
    /** Adds a new module. The uniqueness of identifier is not checked. */
    void add_mod(Module *p_mod);
    /** Returns whether a module with the given \a modid exists. */
    bool has_mod_withId(const Identifier& p_modid);
    /** Gets the module with the given \a modid. Returns 0 if the
     *  module does not exist (w/o error message). See also:
     *  has_mod_withId */
    Module* get_mod_byId(const Identifier& p_modid);
    /** Returns the referenced assignment or 0 if it does not exist.
     *  Append an error message when necessary. */
    Assignment* get_ass_bySRef(Ref_simple *p_ref);
    /** Checks the uniqueness of module identifiers. Prints an error
     *  message if <tt>get_modid()</tt> returns the same for two
     *  modules. */
    void chk_uniq();
    /** Performs the semantic analysis on modules of the ATS.
     * Generates error/warning messages when necessary */
    void chk();
    void write_checksums();
    std::set<ModuleVersion> getVersionsWithProductNumber() const;
  private:
    /* Checks the ASN.1 top-level PDU types that were passed as command line
     * arguments */
    void chk_top_level_pdus();
  public:
    void generate_code(CodeGenHelper& cgh);
    void dump(unsigned level=1) const;
    
    /** Generates JSON schema segments for the types defined in the modules,
      * and references to these types. Information related to the types' 
      * JSON encoding and decoding functions is also inserted after the references.
      *
      * @param json JSON document containing the main schema, schema segments for 
      * the types will be inserted here
      * @param json_refs map of JSON documents containing the references and function
      * info related to each type */
    void generate_json_schema(JSON_Tokenizer& json, map<Type*, JSON_Tokenizer>& json_refs);
    
    /** Called if a Type::chk_coding() call could not be resolved (the check for
      * custom and PER coding requires the checks of all external functions to
      * be completed).
      * This stores the info needed to call the function again after everything
      * else has been checked. */
    static void delay_type_encode_check(Type* p_type, Module* p_module, bool p_encode);
  };

  /**
   * Interface-class to represent a module.
   *
   * Abstract class because of clone() and a bunch of other methods.
   */
  class Module : public Scope, public Location {
  public:
    enum moduletype_t { MOD_UNKNOWN, MOD_ASN, MOD_TTCN };
    /** The map is used as a set. */
    typedef map <Module*, void> module_set_t;
  protected: // *::Module need access
    moduletype_t moduletype;
    /** Module identifier ("module name") */
    Identifier *modid;
    /** The \c Modules collection which contains this module. Not owned. */
    /** Indicates whether the import list has been checked. */
    bool imp_checked;
    /** Indicates whether to generate C++ code for the module. */
    bool gen_code;
    /** Indicates whether the module has a checksum. */
    bool has_checksum;
    /** Contains the set of modules that are visible through imports (i.e. the
     * module is imported either directly or indirectly by an imported module).
     * The set is complete if \a this is added to the set. */
    module_set_t visible_mods;
    /** Contains the checksum (more correctly, a hash) of the module.
     *  It is an MD5 hash of the TTCN source, computed by the lexer. */
    unsigned char module_checksum[16];

    /** @name Containers to store string literals.
     *  @{ */
    map<string, string> bs_literals; ///< bitstring values
    map<string, string> bp_literals; ///< bitstring patterns
    map<string, string> hs_literals; ///< hexstring values
    map<string, string> hp_literals; ///< hexstring patterns
    map<string, string> os_literals; ///< octetstring values
    map<string, string> op_literals; ///< octetstring patterns
    map<string, string> cs_literals; ///< charstring values
    map<ustring, string> us_literals; ///< universal charstring values
    map<string, string> pp_literals; ///< padding patterns (RAW codec)
    map<string, string> mp_literals; ///< matching patterns (TEXT codec)
    /** @} */

    struct OID_literal {
      size_t nof_elems;
      string oid_id;
    };
    map<string, OID_literal> oid_literals;

    /** Counter for the temporary identifiers */
    size_t tmp_id_count;

    /** Control namespace and prefix.
     *  The module owns these strings. */
    char * control_ns;
    char * control_ns_prefix;

    /** Namespace declarations encountered (global for the entire program).
     *  Keys are the prefixes. Values are the URIs. */
    static map<string, const char> namespaces;

    /** The index of the replacement for the empty prefix, or -1. Returned by
     *  \p get_ns_index if the empty prefix has indeed been replaced. */
    static size_t replacement_for_empty_prefix;

    /** Namespace prefixes made up to avoid clashes.
     *  Keys are the URIs (!), values are the prefixes.
     *  (This is a reverse of a subset of the \p namespaces member).
     *  This allows reuse of made-up prefixes if the same namespace URI
     *  appears again. */
    static map<string, const char> invented_prefixes;

    /** Indexes of the namespaces used by this module.
     *  Written while Common::Type::generate_code_xerdescriptor() makes calls
     *  to Common::Module::get_ns_index(). */
    map<size_t, void> used_namespaces;

    /** How many different namespace URIs with empty prefix have been added */
    static size_t default_namespace_attempt;

    /** Type conversions found in this module */
    vector<TypeConv> type_conv_v;

    /** @name Module version fields
     *  @{ */
    char* product_number;
    unsigned int suffix;
    unsigned int release;
    unsigned int patch;
    unsigned int build;
    char* extra;
    /** @} */

    friend class Ttcn::Module;

    /* * Value of GLOBAL-DEFAULTS MODIFIED-ENCODINGS */
    //TODO: introduce bool modified_encodings;

    /** Generates code for all string literals that belong to the
     *  module into \a target */
    void generate_literals(output_struct *target);
    /** Generates the module level entry functions based on
     * \a output->functions. The resulting functions are placed into
     * \a output->source.function_bodies.
     * Also writes the control part, module checksum, XML namespaces,
     * module object to output->source.global_vars */
    void generate_functions(output_struct *output);
    void generate_conversion_functions(output_struct *output);
    
    /** Generates the debugger initialization function for this module.
      * The function creates the global debug scope associated with this module,
      * and initializes it with all the global variables visible in the module
      * (including imported variables).
      * The debug scopes of all component types defined in the module are also
      * created and initialized with their variables. */
    virtual void generate_debugger_init(output_struct *output) = 0;
    
    /** Generates the variable adding code for all global variables defined
      * in this module. This function is called by generate_debugger_init()
      * for both the current module and all imported modules. */
    virtual char* generate_debugger_global_vars(char* str, Common::Module* current_mod) = 0;
    
    /** Generates the debugger variable printing function, which can print values
      * and templates of all types defined in this module (excluding subtypes). */
    virtual void generate_debugger_functions(output_struct *output) = 0;
    
  private:
    /** Copy constructor not implemented */
    Module(const Module& p);
    /** Assignment disabled */
    Module& operator=(const Module& p);

    /** Adds \a str to container \a literals with prefix \a prefix */
    static string add_literal(map<string, string>& literals, const string& str,
      const char *prefix);

    void generate_bs_literals(char *&src, char *&hdr);
    void generate_bp_literals(char *&src, char *&hdr);
    void generate_hs_literals(char *&src, char *&hdr);
    void generate_hp_literals(char *&src, char *&hdr);
    void generate_os_literals(char *&src, char *&hdr);
    void generate_op_literals(char *&src, char *&hdr);
    void generate_cs_literals(char *&src, char *&hdr);
    void generate_us_literals(char *&src, char *&hdr);
    void generate_oid_literals(char *&src, char *&hdr);
    void generate_pp_literals(char *&src, char *&hdr);
    void generate_mp_literals(char *&src, char *&hdr);

    /** Clears the container \a literals */
    static void clear_literals(map<string, string>& literals);

  public:
    Module(moduletype_t p_mt, Identifier *modid);
    virtual ~Module();
    /** Adds type conversion \p p_conv to this module.  */
    void add_type_conv(TypeConv *p_conv);
    /** Checks if \p p_from_type and \p p_to_type types need conversion in
     *  this module. */
    bool needs_type_conv(Type *p_from_type, Type *p_to_type) const;
    /** Returns the module type (either TTCN-3 or ASN.1). */
    moduletype_t get_moduletype() const {return moduletype;}
    /** Sets the flag to generate C++ code for this module. */
    void set_gen_code() {gen_code = true;}
    /** Returns whether to generate C++ code for this module. */
    bool get_gen_code() const {return gen_code;}
    /** Returns the module-identifier. */
    const Identifier& get_modid() const {return *modid;}
    /** Gets the module scope. This function returns this scope or a
     *  scope above this or 0 if neither is a Module. */
    virtual Module* get_scope_mod() {return this;}
    virtual Module* get_scope_mod_gen() {return this;}
    virtual Assignments *get_asss() =0;
    virtual Common::Assignment* importAssignment(
      const Identifier& p_source_modid, const Identifier& p_id) const =0;
    /** Returns true if a symbol with identifier \a p_id is
     *  exported. */
    virtual bool exports_sym(const Identifier& p_id) =0;
    /** Returns whether the module has an imported definition/assignment with
     * identifier \a p_id */
    virtual bool has_imported_ass_withId(const Identifier& p_id) = 0;
    /** Returns a pointer to the TTCN-3 special address type that is defined in
     * the TTCN-3 module. A NULL pointer is returned if the address type is not
     * defined in this module. This function is applicable to TTCN-3 modules
     * only (otherwise a FATAL_ERROR will occur). */
    virtual Type *get_address_type();
    /** Checks imports (and propagates the code generation
     *  flags). */
    virtual void chk_imp(ReferenceChain& refch, vector<Module>& moduleStack) = 0;
    /** Checks everything (imports, exports, assignments) */
    virtual void chk() = 0;
    /** Checks this module and all imported modules in bottom-up order.
     * Argument \a checked_modules contains the list of modules that are
     * already checked */
    void chk_recursive(module_set_t& checked_modules);
    /** Returns whether \a m is visible from \a this through imports. */
    bool is_visible(Module *m);
    /** Extends \a p_visible_mods with the set of visible modules. It uses the
     * cache \a visible_mods if possible or calls \a get_imported_mods()
     * otherwise. */
    void get_visible_mods(module_set_t& p_visible_mods);
    /** Walks through the import list and collects the imported modules into
         * \a p_imported_mods recursively. */
    virtual void get_imported_mods(module_set_t& p_imported_mods) = 0;
    void write_checksum();
    static char* get_product_identifier(const char* product_number,
        const unsigned int suffix, unsigned int release, unsigned int patch,
        unsigned int build, const char* extra=NULL);
    ModuleVersion getVersion() const;
  protected: // *::Module need access
    /** Collects the set of visible modules into \a visible_mods. */
    void collect_visible_mods();
    virtual void generate_code_internal(CodeGenHelper& cgh) = 0;
  public:
    /** Adds a string to the module's bitstring container. Returns a
     *  string like "bs_xx", where xx is the index of the literal in
     *  the container. */
    inline string add_bitstring_literal(const string& bstr)
      { return add_literal(bs_literals, bstr, "bs_"); }
    inline string add_bitstring_pattern(const string& bpat)
      { return add_literal(bp_literals, bpat, "bp_"); }
    inline string add_hexstring_literal(const string& hstr)
      { return add_literal(hs_literals, hstr, "hs_"); }
    inline string add_hexstring_pattern(const string& hpat)
      { return add_literal(hp_literals, hpat, "hp_"); }
    inline string add_octetstring_literal(const string& ostr)
      { return add_literal(os_literals, ostr, "os_"); }
    inline string add_octetstring_pattern(const string& opat)
      { return add_literal(op_literals, opat, "op_"); }
    inline string add_charstring_literal(const string& cstr)
      { return add_literal(cs_literals, cstr, "cs_"); }
    inline string add_padding_pattern(const string& ppat)
      { return add_literal(pp_literals, ppat, "pp_"); }
    inline string add_matching_literal(const string& mpat)
      { return add_literal(mp_literals, mpat, "mp_"); }
    string add_ustring_literal(const ustring& ustr);
    string add_objid_literal(const string& oi_str, const size_t nof_elems);

    /** Sets the module checksum. Parameter \a checksum_ptr points to the
     * checksum to be set, which consists of \a checksum_len bytes. */
    void set_checksum(size_t checksum_len, const unsigned char* checksum_ptr);

    /** Returns an identifier used for temporary C++ objects,
     *  which is unique in the module */
    string get_temporary_id();

    /** Sets the control namespace and its prefix.
     * Any previous value is overwritten.
     * Takes ownership of the strings (must be allocated on the heap). */
    void set_controlns(char *ns, char *prefix);

    /** Gets the control namespace components.
     * The caller must not free the strings. */
    void get_controlns(const char * &ns, const char * &prefix);

    /** Adds a namespace to the list of known namespaces.
     *  No effect if the namespace is already in the map.
     *  @param new_uri namespace URI
     *  @param new_prefix namespace prefix; NULL means "make up a prefix"
     *  @note If \p new_prefix is empty and there is already a namespace with
     *  an empty prefix, a new, non-empty prefix is invented for this URI;
     *  in this case \p new_prefix is modified to contain the "made-up" value.
     *  @note \p new_prefix \b MUST be expstring_t or allocated by Malloc
     *  (add_namespace may call Free() on it) */
    static void add_namespace(const char *new_uri, char * &new_prefix);

    /** Returns the number of XML namespaces */
    static inline size_t get_nof_ns()
    { return namespaces.size(); }

    /** Return the index of the given namespace prefix in the list of namespaces.
     *  If the namespace is not found, FATAL_ERROR occurs.
     *  @note also records the index in the per-instance member used_namespaces
     *  (which is why it cannot be static) */
    size_t get_ns_index(const char *prefix);

    /** Rename the default namespace, but only if there were two or more
     *  namespaces with empty prefixes */
    static void rename_default_namespace();

    /** Generates C++ code for the module */
    void generate_code(CodeGenHelper& cgh);
    virtual void dump(unsigned level) const;
    
    /** Generates JSON schema segments for the types defined in the modules,
      * and references to these types. Information related to the types' 
      * JSON encoding and decoding functions is also inserted after the references.
      *
      * @param json JSON document containing the main schema, schema segments for 
      * the types will be inserted here
      * @param json_refs map of JSON documents containing the references and function
      * info related to each type */
    virtual void generate_json_schema(JSON_Tokenizer& json, map<Type*, JSON_Tokenizer>& json_refs) = 0;
  };

  /**
   * Class to store assignments.
   */
  class Assignments : public Scope {
  protected:  // Ttcn::Definitions and Asn::Assignments need access

    Assignments(const Assignments& p): Scope(p) {}
  public:
    /** Constructor. */
    Assignments() : Scope() {}

    virtual Assignments* get_scope_asss();
    /** Returns the referenced assignment or 0 if it does not exist.
     *  An error message is generated when necessary. */
    virtual Assignment* get_ass_bySRef(Ref_simple *p_ref);
    /** Returns whether an assignment with id \a p_id exists;
     * either in the current scope or its parent (recursively). */
    virtual bool has_ass_withId(const Identifier& p_id);
    /** Returns whether an assignment with id \a p_id exists.
     * Unlike \a has_ass_withId() this function does not look into the
     * parent scope. */
    virtual bool has_local_ass_withId(const Identifier& p_id) = 0;
    /** Returns the locally defined assignment with the given id,
     * or NULL if it does not exist. */
    virtual Assignment* get_local_ass_byId(const Identifier& p_id) = 0;
    /** Returns the number of assignments. Only the uniquely named
     *  assignments are visible. */
    virtual size_t get_nof_asss() = 0;
    /** Returns the assignment with the given index. Only the uniquely
     *  named assignments are visible. */
    virtual Assignment* get_ass_byIndex(size_t p_i) = 0;
  };
  
  enum param_eval_t {
    NORMAL_EVAL,
    LAZY_EVAL,
    FUZZY_EVAL
  };

  /**
   * Abstract class to represent different kinds of assignments.
   */
  class Assignment : public Node, public Location {
  public:
    enum asstype_t {
      A_TYPE,           /**< type */
      A_CONST,          /**< value (const) */
      A_UNDEF,          /**< undefined/undecided (ASN.1) */
      A_ERROR,          /**< erroneous; the kind cannot be deduced (ASN.1) */
      A_OC,             /**< information object class (ASN.1) */
      A_OBJECT,         /**< information object (ASN.1) */
      A_OS,             /**< information object set (ASN.1) */
      A_VS,             /**< value set (ASN.1) */
      A_EXT_CONST,      /**< external constant (TTCN-3) */
      A_MODULEPAR,      /**< module parameter (TTCN-3) */
      A_MODULEPAR_TEMP, /**< template module parameter */
      A_TEMPLATE,       /**< template (TTCN-3) */
      A_VAR,            /**< variable (TTCN-3) */
      A_VAR_TEMPLATE,   /**< template variable, dynamic template (TTCN-3) */
      A_TIMER,          /**< timer (TTCN-3) */
      A_PORT,           /**< port (TTCN-3) */
      A_FUNCTION,       /**< function without return type (TTCN-3) */
      A_FUNCTION_RVAL,  /**< function that returns a value (TTCN-3) */
      A_FUNCTION_RTEMP, /**< function that returns a template (TTCN-3) */
      A_EXT_FUNCTION,   /**< external function without return type (TTCN-3) */
      A_EXT_FUNCTION_RVAL,  /**< ext. func that returns a value (TTCN-3) */
      A_EXT_FUNCTION_RTEMP, /**< ext. func that returns a template (TTCN-3) */
      A_ALTSTEP,        /**< altstep (TTCN-3) */
      A_TESTCASE,       /**< testcase (TTCN-3) */
      A_PAR_VAL,        /**< formal parameter (value) (TTCN-3) */
      A_PAR_VAL_IN,     /**< formal parameter (in value) (TTCN-3) */
      A_PAR_VAL_OUT,    /**< formal parameter (out value) (TTCN-3) */
      A_PAR_VAL_INOUT,  /**< formal parameter (inout value) (TTCN-3) */
      A_PAR_TEMPL_IN,   /**< formal parameter ([in] template) (TTCN-3) */
      A_PAR_TEMPL_OUT,  /**< formal parameter (out template) (TTCN-3) */
      A_PAR_TEMPL_INOUT,/**< formal parameter (inout template) (TTCN-3) */
      A_PAR_TIMER,      /**< formal parameter (timer) (TTCN-3) */
      A_PAR_PORT        /**< formal parameter (port) (TTCN-3) */
    };
  protected: // Ttcn::Definition and Asn::Assignment need access
    asstype_t asstype;
    Identifier *id; /**< the name of the assignment */
    Scope *my_scope; /**< the scope this assignment belongs to */
    bool checked;
    visibility_t visibilitytype;

    /// Copy constructor disabled
    Assignment(const Assignment& p);
    /// Assignment disabled
    Assignment& operator=(const Assignment& p);
    virtual string get_genname() const = 0;
  public:
    Assignment(asstype_t p_asstype, Identifier *p_id);
    virtual ~Assignment();
    virtual Assignment* clone() const =0;
    virtual asstype_t get_asstype() const;
    /** Returns the string representation of the assignment type */
    const char *get_assname() const;
    /** Returns the description of the definition, which consists of the
     * assignment type and name. The name is either the fullname or only
     * the id. It depends on \a asstype and \a my_scope. */
    string get_description();
    /** Gets the id of the assignment. */
    const Identifier& get_id() const {return *id;}
    /** Sets the internal pointer my_scope to \a p_scope. */
    virtual void set_my_scope(Scope *p_scope);
    Scope* get_my_scope() const { return my_scope; }
    bool get_checked() const { return checked; }
    /** Return the visibility type of the assignment */
    visibility_t get_visibility() const { return visibilitytype; }
    /** Returns whether the definition belongs to a TTCN-3 statement block
     * (i.e. it is defined in the body of a function, testcase, altstep or
     * control part). */
    virtual bool is_local() const;
    /** @name Need to be overridden and implemented in derived classes.
     *  Calling these methods causes a FATAL_ERROR.
     *
     * @{
     */
    virtual Setting* get_Setting();
    virtual Type *get_Type();
    virtual Value *get_Value();
    virtual Ttcn::Template *get_Template();
    virtual param_eval_t get_eval_type() const;
    /** @} */
    /** Returns the formal parameter list of a TTCN-3 definition */
    virtual Ttcn::FormalParList *get_FormalParList();
    /** Returns the dimensions of TTCN-3 port and timer arrays or NULL
     * otherwise. */
    virtual Ttcn::ArrayDimensions *get_Dimensions();
    /** Returns the component type referred by the 'runs on' clause of a
     * TTCN-3 definition */
    virtual Type *get_RunsOnType();
    /** Returns the port type referred by the 'port' clause of a
     * TTCN-3 function definition */
    virtual Type *get_PortType();
    /** Semantic check */
    virtual void chk() = 0;
    /** Checks whether the assignment has a valid TTCN-3 identifier,
     *  i.e. is reachable from TTCN. */
    void chk_ttcn_id();
    /** Returns a string containing the C++ reference pointing to this
     * definition from the C++ equivalent of scope \a p_scope. The reference
     * is a simple identifier qualified with a namespace when necessary.
     * If \a p_prefix is not NULL it is inserted before the string returned by
     * function \a get_genname(). */
    string get_genname_from_scope(Scope *p_scope, const char *p_prefix = 0);
    /** Returns the name of the C++ object in the RTE that contains the common
     * entry points for the module that the definition belongs to */
    const char *get_module_object_name();
    /** A stub function to avoid dynamic_cast's. It causes FATAL_ERROR unless
     * \a this is an `in' value or template parameter. The real implementation
     * is in class Ttcn::FormalPar. */
    virtual void use_as_lvalue(const Location& p_loc);
    virtual void generate_code(output_struct *target, bool clean_up = false);
    virtual void generate_code(CodeGenHelper& cgh); // FIXME: this should be pure virtual
    virtual void dump(unsigned level) const;
    virtual Ttcn::Group* get_parent_group();
  };

  /** @} end of AST group */

} // namespace Common

#endif // _Common_AST_HH
