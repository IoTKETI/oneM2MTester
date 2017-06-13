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
 *   Beres, Szabolcs
 *   Bibo, Zoltan
 *   Cserveni, Akos
 *   Delic, Adam
 *   Forstner, Matyas
 *   Gecse, Roland
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Tatarka, Gabor
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#ifndef _Common_Setting_HH
#define _Common_Setting_HH

#include <stdio.h>

#include "error.h"
#include "string.hh"
#include "stack.hh"
#include "vector.hh"
#include "map.hh"

/** YYLTYPE definition from compiler.tab.hh , used by Location class */
#if ! defined (YYLTYPE) && ! defined (YYLTYPE_IS_DECLARED)
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif

struct expression_struct_t;

namespace Ttcn {
  // not defined here
  class FieldOrArrayRef;
  class FieldOrArrayRefs;
  class ActualParList;
  class RunsOnScope;
  class PortScope;
  class StatementBlock;
  struct ErroneousDescriptor;
  class ErroneousDescriptors;
  class transparency_holder;
  class Statement;
} // namespace Ttcn

namespace Common {

  /**
   * \addtogroup AST
   *
   * @{
   */

  class Node;
  class StringRepr;
  class Setting;
  class Governor;
  class Governed;
  class GovdSet;

  class Scope;

  class Reference;
  class Ref_simple;
  class ReferenceChain;

  // not defined here
  class Identifier;
  class Module;
  class Assignment;
  class Assignments;
  class Type;
  class Value;

  /**
   * Base class for the AST-classes that have associated location
   * (file name and line number) information.
   *
   * This class correctly uses the compiler-generated copy constructor
   * and assignment operator, despite having a pointer member.
   */
  class Location {
  private:
    /** map used as set containing one instance of all file names
     * encountered during preprocessing.
     * It is used as long term storage for the filename extracted from
     * C preprocessor line markers (#line directives) */
    static map<string,void> *source_file_names;
    /** Source file name.
     *  Not owned by the Location object. Do not delete.
     */
    const char *filename;
    /** Source file location.
     *  Line and column for start and end (some of which may be zero)
     */
    YYLTYPE yyloc; /* yyloc.first_line == lineno for backward compatibility */
    /** The location is inside a function with extension "transparent" */
    static bool transparency;
    friend class Ttcn::transparency_holder;
  public:

    /** adds new file name, returns pointer to stored file name,
      * names are stored only once */
    static const char* add_source_file_name(const string& file_name);
    static void delete_source_file_names();

    /** Default constructor.
     *  Sets everything to 0/NULL.
     */
    Location();

    /** Constructor with filename and just a line number.
     * Sets \c yyloc.first_line and \c yyloc.last_line to \p p_lineno,
     * and the columns to zero. */
    Location(const char *p_filename, int p_lineno=0)
      { set_location(p_filename, p_lineno); }

    /** Constructor with filename and full location information.
     * Copies \p p_yyloc into \p yyloc.
     */
    Location(const char *p_filename, const YYLTYPE& p_yyloc)
      { set_location(p_filename, p_yyloc); }

    /** Constructor with filename and two location info.
     *
     * @param p_filename contains the name of the file
     *        Stores pointer \p p_filename into \c filename.
     * @param p_firstloc holds the location of the beginning.
     *        Its   \p first_line and       \p first_column are copied into
     *        \c yyloc.first_line and \c yyloc.first_column.
     * @param p_lastloc contains the location of the end.
     *        Its   \p last_line and       \p last_column are copied into
     *        \c yyloc.last_line and \c yyloc.last_column.
     */
    Location(const char *p_filename, const YYLTYPE& p_firstloc,
                                     const YYLTYPE& p_lastloc)
      { set_location(p_filename, p_firstloc, p_lastloc); }

    /** Constructor with filename and full location available separately.
     * Stores the filename, and the line/column info in the appropriate
     * members of \c yyloc.
     */
    Location(const char *p_filename, int p_first_line, int p_first_column,
                                     int p_last_line, int p_last_column)
      { set_location(p_filename, p_first_line, p_first_column,
                                 p_last_line, p_last_column); }

    /** Setter with filename and full location information.
     * Copies \p p_yyloc into \p yyloc.
     */
    void set_location(const char *p_filename, int p_lineno=0);
    /** Setter with filename and full location information.
     * Copies \p p_yyloc into \p yyloc.
     */
    void set_location(const char *p_filename, const YYLTYPE& p_yyloc);
    /** Setter with filename and two location info.
     *
     * @param p_filename contains the name of the file
     *        Stores pointer \p p_filename into \c filename.
     * @param p_firstloc holds the location of the beginning.
     *        Its   \p first_line and       \p first_column are copied into
     *        \c yyloc.first_line and \c yyloc.first_column.
     * @param p_lastloc contains the location of the end.
     *        Its   \p last_line and       \p last_column are copied into
     *        \c yyloc.last_line and \c yyloc.last_column.
     */
    void set_location(const char *p_filename, const YYLTYPE& p_firstloc,
                                              const YYLTYPE& p_lastloc);
    /** Setter with filename and full location available separately.
     * Stores the filename, and the line/column info in the appropriate
     * members of \c yyloc.
     */
    void set_location(const char *p_filename, int p_first_line,
      int p_first_column, int p_last_line, int p_last_column);

    /** Copies the stored location information from \a p. */
    void set_location(const Location& p) { *this = p; }

    /** Joins the coordinates of adjacent location \a p into this. */
    void join_location(const Location& p);

    /** Returns the attribute filename. */
    const char *get_filename() const { return filename; }
    /** Returns the line number, for backward compatibility */
    int get_lineno() const { return yyloc.first_line; }
    /** Returns the first line attribute */
    int get_first_line() const { return yyloc.first_line; }
    /** Returns the last line attribute */
    int get_last_line() const { return yyloc.last_line; }
    /** Returns the first column attribute */
    int get_first_column() const { return yyloc.first_column; }
    /** Returns the last column attribute */
    int get_last_column() const { return yyloc.last_column; }

private:
    /** Prints the line/column information stored in \a this into file \a fp.
     * Used by functions print_location() */
    void print_line_info(FILE *fp) const;
public:
    /** Prints the contents of \a this into file \a fp. Used in error
     * messages */
    void print_location(FILE *fp) const;

    /** Generic error reporting function with printf-like arguments. */
    void error(const char *fmt, ...) const
      __attribute__ ((__format__ (__printf__, 2, 3)));
    /** Generic warning function with printf-like arguments. */
    void warning(const char *fmt, ...) const
      __attribute__ ((__format__ (__printf__, 2, 3)));
    /** Generic note function with printf-like arguments. */
    void note(const char *fmt, ...) const
      __attribute__ ((__format__ (__printf__, 2, 3)));

    /** Generates a C++ code fragment that instantiates a runtime location
     *  object containing the file name and source line information carried by
     * \a this. The C++ code is appended to argument \a str and the resulting
     * string is returned. Arguments \a entitytype and \a entityname determine
     * the attributes of the location object based on the kind (FUNCTION,
     * TESTCASE, etc.) and name of the corresponding TTCN-3 definition.
     * The generation of location objects is optional, it is controlled by a
     * command line switch. */
    char *create_location_object(char *str, const char *entitytype,
      const char *entityname) const;
    /** Generates a C++ code fragment that updates the line number information
     * of the innermost runtime location object. The C++ code is appended to
     * argument \a str and the resulting string is returned. The function is
     * used by subsequent statements of statement blocks. */
    char *update_location_object(char *str) const;
  };

  /**
   * Base class for AST-classes.
   *
   * The AST is an example of the Composite pattern ("Design Patterns", by
   * Gamma et al.): it represents a part-whole hierarchy and allows clients
   * to treat individual objects and compositions of objects uniformly.
   *
   * A container node "distributes" method calls to its subordinates:
   *
   * Common::Modules contains a collection of Asn/Ttcn::Module objects.
   * Common::Modules::generate_code() calls Common::Module::generate_code()
   * for each module that was parsed. Common::Module::generate_code() calls
   * the virtual generate_code_internal(); for a TTCN-3 module this is
   * Ttcn::Module::generate_code_internal().
   *
   * A Ttcn::Module contains (among others) imports, definitions and (maybe)
   * a control part. Consequently Ttcn::Module::generate_code_internal() calls
   * Ttcn::Imports::generate_code(), then Ttcn::Definitions::generate_code(),
   * then (maybe) Ttcn::ControlPart::generate_code().
   */
  class Node {
  private:
    /** To detect missing/duplicated invoking of destructor. */
    static int counter;
#ifdef MEMORY_DEBUG
    /** Linked list for tracking undeleted nodes. */
    Node *prev_node, *next_node;
#endif
    /** Name that contains information about the node's position in
     *  the AST. It is used when reporting things to the user.
     *  Guidelines for names can be found in X.680 clause 14. Main
     *  rules: components are separated by dot ("."). Absolute names
     *  begin with "@" followed by the module name.
     */
    string fullname;
  protected: // Setting needs access to the constructors
    /** Default constructor. */
    Node();
    /** The copy constructor. */
    Node(const Node& p);
  private:
    /** Assignment disabled */
    Node& operator=(const Node& p);
  public:
    /**
     * "Virtual constructor".
     *
     * In AST-classes, you should invoke the clone() member
     * explicitly, instead of using the copy constructor.
     *
     * \note The clone is not an exact copy of the original object.
     * Some information (like cached things, links to parent etc., if
     * any) are not copied; they are updated when adding to parent
     * node, checking the value etc.
     */
    virtual Node* clone() const = 0;
    /** The destructor. */
    virtual ~Node();
    /** Gives an error message if counter is not zero. It can be
     * invoked before the program exits (like check_mem_leak()) to
     * verify that all AST-objects are destroyed. */
    static void chk_counter();
    /** Sets the fullname. In the descendant classes, it sets also the
     * fullname of the node's children, if any. */
    virtual void set_fullname(const string& p_fullname);
    /** Gets the fullname. */
    const string& get_fullname() const { return fullname; }
    virtual void set_my_scope(Scope *p_scope);
    virtual void dump(unsigned level) const;
  };

  /**
   * Class Setting. ObjectClass, Object, ObjectSet, Type, Value or
   * ValueSet.
   */
  class Setting : public Node, public Location {
  public:
    enum settingtype_t {
      S_UNDEF, /**< Reference to non-existent stuff */
      S_ERROR, /**< Reference to non-existent stuff */
      S_OC, /**< ObjectClass */
      S_T, /**< Type */
      S_TEMPLATE, /**< Template */
      S_O, /**< Object */
      S_V, /**< Value */
      S_OS, /**< ObjectSet */
      S_VS /**< ValueSet */
    };
  protected: // Governed and Governor need access to members and copy c-tor
    settingtype_t st;
    Scope *my_scope;
    string genname;
    bool checked;
    /** Indicates whether the circular references within embedded entities
     * has been checked. For instance: a record/SEQUENCE type cannot contain
     * itself as a mandatory field; a field in a value cannot refer to the
     * value itself. */
    bool recurs_checked;

    Setting(const Setting& p)
    : Node(p), Location(p), st(p.st), my_scope(0),
      genname(), checked(false), recurs_checked(false) {}
  private:
    /** Assignment disabled */
    Setting& operator=(const Setting& p);
  public:
    Setting(settingtype_t p_st);
    virtual Setting* clone() const = 0;
    settingtype_t get_st() const { return st; }
    virtual void set_my_scope(Scope *p_scope);
    Scope *get_my_scope() const { return my_scope; }
    /** Returns whether the setting was defined in ASN.1 module. */
    bool is_asn1() const;
    /** Returns a unique temporary C++ identifier, which is obtained from
     *  the module scope. */
    string get_temporary_id() const;
    /** Set \a genname from \p p_genname.
     *
     * @pre p_genname must not be empty. */
    void set_genname(const string& p_genname);
    /** Set \a genname from two parts.
     *
     * Basically concatenates \p p_prefix, an underscore and \p p_suffix,
     * unless p_prefix already ends with, or p_suffix already begins with
     * \b precisely one underscore.
     *
     * @pre p_prefix must not be empty.
     * @pre p_suffix must not be empty. */
    void set_genname(const string& p_prefix, const string& p_suffix);
  public:
    /** Returns a C++ reference that points to the C++ equivalent of this
     * setting from the local module (when such entity exists) */
    const string& get_genname_own() const;
    /** Returns a C++ reference that points to this setting from the module of
     * scope \a p_scope */
    string get_genname_own(Scope *p_scope) const;

  private:
    /** Creates and returns the string representation of the setting. */
    virtual string create_stringRepr();
  public:
    /** Returns the string representation of the setting. Currently it calls
     * \a create_stringRepr(), but it may use some caching in the future. */
    string get_stringRepr() { return create_stringRepr(); }
  };

  /**
   * Ass_Error::get_Setting() returns this.
   */
  class Setting_Error : public Setting {
  private:
    Setting_Error(const Setting_Error& p) : Setting(p) {}
  public:
    Setting_Error() : Setting(S_ERROR) {}
    virtual Setting_Error* clone() const;
  };

  /**
   * Things that can be the governor of something.
   */
  class Governor : public Setting {
  protected: // Derived classes need access to the copy c-tor
    Governor(const Governor& p) : Setting(p) {}
  public:
    Governor(settingtype_t p_st) : Setting(p_st) {}
    virtual Governor* clone() const =0;
    virtual void chk() =0;
  };

  /**
   * Things that have a governor. Object, Value...
   */
  class Governed : public Setting {
  protected: // Derived classes need access to the copy c-tor
    Governed(const Governed& p) : Setting(p) {}
  public:
    Governed(settingtype_t p_st) : Setting(p_st) {}
    virtual Governed* clone() const =0;
    virtual Governor* get_my_governor() const =0;
  };

  /**
   * A governed thing that will be mapped to a C++ entity.
   * (e.g. Value, Template)
   */
  class GovernedSimple : public Governed {
  public:
    enum code_section_t {
      CS_UNKNOWN, /**< Unknown (i.e. not specified). */
      CS_PRE_INIT, /**< Initialized before processing the configuration file
                    * (i.e. module parameters are not known). Example:
		    * constants, default value for module parameters. */
      CS_POST_INIT, /**< Initialized after processing the configuration file
                    * (i.e. module parameters are known). Example:
		    * non-parameterized templates. */
      CS_INIT_COMP, /**< Initialized with the component entities (i.e. when
                     * the component type is known). Example: initial value
		     * for component variables, default duration for timers. */
      CS_INLINE /**< Initialized immediately at the place of definition.
                 * Applicable to local definitions only. Example: initial
		 * value for a local variable. */
    };
  private:
    /** A prefix that shall be inserted before the genname when initializing
     * the C++ object. Without this prefix the genname points to a read-only
     * C++ object reference. For example, `const_c1' is a writable object with
     * limited access (file static), but `c1' is a global const reference
     * pointing to it.
     * Possible values: "const_", "modulepar_", "template_".
     */
    const char *genname_prefix;
    /** Indicates the section of the output code where the initializer C++
     * sequence has to be put. If entity A refers to entity B and both has to
     * be initialized in the same section, the initializer of B must precede
     * the initializer of A. If the initializer of A and B has to be put into
     * different sections the right order is provided automatically by the
     * run-time environment. */
    code_section_t code_section;
    /** A flag that indicates whether the initializer C++ code has been
     * generated for the object (or object field).
     */
    bool code_generated;
  protected: // Derived classes need access to the copy c-tor
    Ttcn::ErroneousDescriptors* err_descrs; // owned, used by negative testing
    GovernedSimple(const GovernedSimple& p) : Governed(p),
      genname_prefix(p.genname_prefix), code_section(p.code_section),
      code_generated(false), err_descrs(NULL), needs_conversion(false) { }
    bool needs_conversion; /**< Type conversion needed. */
  private:
    /** Assignment disabled */
    GovernedSimple& operator=(const GovernedSimple& p);
  public:
    GovernedSimple(settingtype_t p_st) : Governed(p_st), genname_prefix(0),
      code_section(CS_UNKNOWN), code_generated(false), err_descrs(NULL),
      needs_conversion(false) { }
    ~GovernedSimple();

    /** Sets attribute \a genname_prefix to \a p_genname_prefix. For efficiency
     * reasons the string itself is not copied, thus it must point to a
     * permanent memory area. */
    void set_genname_prefix(const char *p_genname_prefix)
      { genname_prefix = p_genname_prefix; }
    /** Returns attribute \a genname_prefix. */
    const char *get_genname_prefix() const { return genname_prefix; }

    /** Returns the attribute \a code_section. */
    code_section_t get_code_section() const { return code_section; }
    /** Sets the attribute \a code_section to \a p_code_section. */
    void set_code_section(code_section_t p_code_section)
      { code_section = p_code_section; }

    /** Returns the flag \a code_generated. */
    bool get_code_generated() const { return code_generated; }
    /** Sets the flag \a code_generated to true. */
    void set_code_generated() { code_generated = true; }

    /** Adds an error descriptor to the template or value (for negative testing) */
    void add_err_descr(Ttcn::Statement* p_update_statement,
      Ttcn::ErroneousDescriptor* p_err_descr);
    Ttcn::ErroneousDescriptors* get_err_descr() const
      { return err_descrs; }

    /** has_single_expr() to return false. */
    inline void set_needs_conversion() { needs_conversion = true; }
    inline bool get_needs_conversion() const { return needs_conversion; }

    /** Returns the C++ expression that refers to the object, which has to be
     * initialized. */
    string get_lhs_name() const;

    /** Returns whether the C++ initialization sequence of \a refd must be
     * inserted before the initialization sequence of \a this. The function is
     * used when \a this refers to \a refd.
     * It returns true if all the following conditions apply:
     * 1) Code has not been generated for \a refd.
     * 2) Both \a this and \a refd are defined in the same module.
     * 3) The initialization sequence for both shall be placed into the same
     * code section. */
    bool needs_init_precede(const GovernedSimple *refd) const;
    /** Returns whether the entity is a top-level one (i.e. it is not embedded
     * into another entity). The function examines whether the genname is a
     * single identifier or not. */
    bool is_toplevel() const;
  };

  /**
   * A set of governed stuff. ObjectSet or ValueSet.
   */
  class GovdSet : public Governed {
  protected: // Asn::ObjectSet needs access
    GovdSet(const GovdSet& p) : Governed(p) {}
  public:
    GovdSet(settingtype_t p_st) : Governed(p_st) {}
    virtual GovdSet* clone() const =0;
  };

  /**
   * An interface-class to represent scopes of references. A scope
   * is an entity from which assignments can be seen; each of these
   * assignments can be referred by a Reference.
   */
  class Scope : public Node {
  protected: // Several derived classes need access
    /** Link to the parent in the scope hierarchy. Can be 0
     * (root-node, in case of the Module scope). */
    Scope *parent_scope;
    /** Some special kind of parent. Used only in conjunction with
     *  ASN.1 parameterized references. */
    Scope *parent_scope_gen;

    /** The name of the scope. */
    string scope_name;

    /** The name of the scope
    * as it should be reported by the __SCOPE__ macro. */
    string scopeMacro_name;

    Scope(const Scope& p)
      : Node(p), parent_scope(0), parent_scope_gen(0), scope_name(),
        scopeMacro_name() {}
  private:
    /** Assignment disabled */
    Scope& operator=(const Scope& p);
  public:
    Scope() : Node(), parent_scope(0), parent_scope_gen(0), scope_name(),
      scopeMacro_name() {}

    /** Sets the parent_scope to the given value. */
    void set_parent_scope(Scope *p_parent_scope)
      { parent_scope = p_parent_scope; }
    /** Sets some othe parent scope.
     * Only called from Ref_pard::get_ref_defd_simple     */
    void set_parent_scope_gen(Scope *p_parent_scope_gen)
      { parent_scope_gen = p_parent_scope_gen; }
    /** Returns the parent_scope or NULL. */
    Scope *get_parent_scope() const { return parent_scope; }
    /** Sets the name of the scope. */
    void set_scope_name(const string& p_scope_name)
      { scope_name = p_scope_name; }
    /** Gets the full-qualified name of the scope. */
    string get_scope_name() const;
    /** Sets the name of the scope. */
    void set_scopeMacro_name(const string& p_scopeMacro_name)
      { scopeMacro_name = p_scopeMacro_name; }
    /** Gets the full-qualified name of the scope. */
    virtual string get_scopeMacro_name() const;
    /** Returns the closest statementblock unit of the hierarchy, or null if the actual scope is not within a
    * statementblock scope. */
    virtual Ttcn::StatementBlock *get_statementblock_scope();
    /** Returns the scope unit of the hierarchy that belongs to a
     * 'runs on' clause. */
    virtual Ttcn::RunsOnScope *get_scope_runs_on();
    /** Returns the scope unit of the hierarchy that belongs to a
     * 'port' clause. */
    virtual Ttcn::PortScope *get_scope_port();
    /** Returns the assignments/module definitions scope. */
    virtual Assignments *get_scope_asss();
    /** Gets the module scope. This function returns this scope or a
     * scope above this or executes FATAL_ERROR if neither is a
     * Module. */
    virtual Module* get_scope_mod();
    virtual Module* get_scope_mod_gen();
    /** Returns the assignment referenced by \a p_ref. If no such
     * node, 0 is returned. */
    virtual Assignment* get_ass_bySRef(Ref_simple *p_ref) =0;
    /** Returns whether the current scope (or its parent) has an
     * assignment with the given \a p_id. Imported symbols are not
     * searched. */
    virtual bool has_ass_withId(const Identifier& p_id);
    virtual bool is_valid_moduleid(const Identifier& p_id);
    /** Returns the TTCN-3 component type that is associated with
     * keywords 'mtc' or 'system'. Returns NULL if the component type
     * cannot be determined (i.e. outside testcase definitions). */
    virtual Type *get_mtc_system_comptype(bool is_system);
    /** Checks the 'runs on' clause of definition \a p_ass that it can
     * be called from this scope unit. Parameters \a p_loc and \a
     * p_what are used in error messages. \a p_what contains "call" or
     * "activate". */
    void chk_runs_on_clause(Assignment *p_ass, const Location& p_loc,
      const char *p_what);
    /** Checks the 'runs on' clause of type \a p_fat that the values of it can
     * be called from this scope unit. Type \a p_fat shall be of type function
     * or altstep. Parameters \a p_loc and \a p_what are used in error messages.
     * \a p_what contains "call" or "activate". */
    void chk_runs_on_clause(Type *p_fat, const Location& p_loc,
      const char *p_what);
  };

  /**
   * A Reference refers to a Setting.
   */
  class Reference : public Node, public Location {
  protected: // Derived classes need access
    Scope *my_scope;
    bool is_erroneous;
    static size_t _Reference_counter;
    static Setting_Error *setting_error;

    Reference() : Node(), Location(), my_scope(0), is_erroneous(false)
      { _Reference_counter++; }
    Reference(const Reference& p) : Node(p), Location(p), my_scope(0),
      is_erroneous(p.is_erroneous) { _Reference_counter++; }
  private:
    /** Assignment disabled */
    Reference& operator=(const Reference& p);
  public:
    virtual ~Reference();
    virtual Reference* clone() const =0;
    /** Creates a display-name for the reference. */
    virtual string get_dispname() =0;
    virtual void set_my_scope(Scope *p_scope);
    Scope *get_my_scope() const { return my_scope; }
    virtual bool get_is_erroneous();
    /** Returns the referenced stuff. Returns NULL if the referenced
     *  stuff does not exists. */
    virtual Setting* get_refd_setting() =0;
    virtual Setting* get_refd_setting_error();
    /** Returns the referred TTCN-3 definition or ASN.1 assignment.
     * If the referenced definition is not found NULL is returned after
     * reporting the error. The function ignores the TTCN-3 field or array
     * sub-references. Flag \a check_parlist indicates whether to verify the
     * actual parameter list of the TTCN-3 reference against the formal
     * parameter list of the referred definition. The parameter checking is
     * done by default, but it can be disabled in certain cases. */
    virtual Assignment* get_refd_assignment(bool check_parlist = true) = 0;
    /** Returns the field or array subreferences of TTCN-3 references or NULL
     * otherwise. */
    virtual Ttcn::FieldOrArrayRefs *get_subrefs();
    /** Returns the actual parameter list for parameterized TTCN-3 references
     * or NULL otherwise. */
    virtual Ttcn::ActualParList *get_parlist();
    /** True if this reference refers to a settingtype \a p_st. */
    virtual bool refers_to_st(Setting::settingtype_t p_st,
                              ReferenceChain* refch=0);
    virtual bool getUsedInIsbound() {return false;}
    virtual void setUsedInIsbound() {}
    /** Returns whether the reference can be represented by an in-line C++
     *  expression. */
    virtual bool has_single_expr() = 0;
    /** Sets the code section of embedded values (parameters, array indices). */
    virtual void set_code_section(
      GovernedSimple::code_section_t p_code_section);
    /** Generates the C++ equivalent of the reference (including the parameter
     * list and sub-references). */
    virtual void generate_code(expression_struct_t *expr) = 0;
    virtual void generate_code_const_ref(expression_struct_t *expr) = 0;
    virtual void dump(unsigned level) const;
  };

  /**
   * Interface-class to refer to entities. %Common entities which can
   * be referred are: modules, types and values.  Each simple
   * reference is formed by one or two identifiers. An optional
   * identifier identifies the module (<em>modid</em>), and another
   * the entity within that module (<em>id</em>). The \a modid is
   * optional; if present, then the reference is \a global. If not
   * present, then the identifier must be seen from the Reference's
   * Scope or one of its parent-scopes not higher (in the
   * scope-hierarchy) than a Module.
   */
  class Ref_simple : public Reference {
  protected: // Derived classes need access
    /** Points to the referred assignment. Used for caching. */
    Assignment *refd_ass;
    Ref_simple() : Reference(), refd_ass(0) {}
    Ref_simple(const Ref_simple& p) : Reference(p), refd_ass(0) {}
  private:
    /** Assignment disabled */
    Ref_simple& operator=(const Ref_simple& p);
  public:
    virtual Reference* clone() const =0;
    /** Returns the \a modid, or 0 if not present. */
    virtual const Identifier* get_modid() =0;
    /** Returns the \a id. */
    virtual const Identifier* get_id() =0;
    /** Creates a display-name for the reference. */
    virtual string get_dispname();
    virtual Setting* get_refd_setting();
    /** \param check_parlist is ignored */
    virtual Assignment* get_refd_assignment(bool check_parlist = true);
    /** Always returns true */
    virtual bool has_single_expr();
  };

  /**
   * Auxiliary class to detect circular references. Stores strings
   * (e.g., the display-name of the references). When adding a string,
   * checks whether it already exists. If exists, throws an
   * Error_AST_uniq with the full chain in the description.
   *
   * @note ReferenceChains should not be reused without calling reset().
   * It's even better not to reuse them at all.
   */
  class ReferenceChain : public Node {
  private:
    /** Stores the strings representing references. */
    vector<string> refs;
    /** Stores a history of the length of the reference chain.
     * Used by mark_state() and prev_state(). */
    stack<size_t> refstack;
    const Location *my_loc;
    /** Operation being performed (for error reporting) */
    const char* err_str;
    dynamic_array<string> errors;
    /** Used to mark states of the errors array. */
    stack<size_t> err_stack;
    /** If report_error is true, the errors will be reported
     *  automatically. Otherwise the string representation
     *  of the errors will be stored in the errors array.*/
    bool report_error;
  private:
    /** Copy constructor disabled */
    ReferenceChain(const ReferenceChain& p);
    /** Assignment disabled */
    ReferenceChain& operator=(const ReferenceChain& p);
  public:
    /** Constructor.
     *
     * @param p_loc the location to report the error to
     *        stores \p p_loc into \c my_loc
     * @param p_str string describing the operation being performed
     */
    ReferenceChain(const Location *p_loc, const char *p_str=0);
    /** Destructor. Calls reset(). */
    virtual ~ReferenceChain();
    /** "Virtual constructor"; calls FATAL_ERROR() */
    virtual ReferenceChain* clone() const;
    /** Returns whether the chain contains string \a s. */
    bool exists(const string& s) const;
    /** Adds a new string. Returns true on success or false if \a s already
     * exists on chain. */
    bool add(const string& s);
    /** Turns on or off the automatic error reporting. */
    void set_error_reporting(bool enable);
    /** Returns the number of errors found after the last
     * marked state (mark_error_state())*/
    size_t nof_errors() const;
    /** Marks the current state in the errors array. prev_error_state()
     * can be used to restore this state.*/
    void mark_error_state();
    void prev_error_state();
    /** Reports all the errors found after the last mark_error_state() call. */
    void report_errors();
    /** Marks the current state, so later prev_state() can restore
     * this state. Useful in recursive checks.
     * It stores the length of the reference chain in \p refstack. */
    void mark_state();
    /** Restores the reference chain to the last saved state.
     * Pops a stored size from \p refstack and truncates the chain
     * to that size (the chain can never be shorter than the saved size). */
    void prev_state();
    /** Cleans the references and the stored sizes. */
    void reset();
    /** Gives a string like this: "`ref1' -> `ref2' -> `ref3' -> `ref1'".
     * It uses the contents of \a refs starting with the first occurrence
     * of string \a s. Finally it appends \a s to close the loop. */
    string get_dispstr(const string& s) const;
  };

  /** @} end of AST group */

} // namespace Common

#endif // _Common_Setting_HH
