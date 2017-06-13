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
#ifndef _Common_Value_HH
#define _Common_Value_HH

#include "Setting.hh"
#include "Code.hh"
#include "Type.hh"
#include "Int.hh"
#include "../common/ttcn3float.hh"

class ustring;
class JSON_Tokenizer;

namespace Asn {
  class Block;
}

namespace Ttcn {
  class Reference;
  class Ref_base;
  class TemplateInstance;
  class TemplateInstances;
  class ActualParList;
  class ActualPar;
  class ParsedActualParameters;
  class LogArguments;
  class JsonOmitCombination;
  class LengthRestriction;
}

namespace Common {

  // not defined here
  class OID_comp;
  class Values;
  class NamedValue;
  class NamedValues;
  class CharSyms;
  class Assignment;
  class Scope;

  using Asn::Block;
  using Ttcn::TemplateInstance;
  using Ttcn::LogArguments;

  /**
   * \ingroup AST
   *
   * \defgroup AST_Value Value
   *
   * These classes provide a unified interface for values.
   *
   * @{
   */

  /**
   * Represents a value in the AST.
   */
  class Value : public GovernedSimple {
  public:
    /** value type */


    enum valuetype_t {
      V_ERROR, /**< erroneous */
      V_NULL, /**< NULL (for ASN.1 NULL type, also in TTCN-3) */
      V_BOOL, /**< boolean */
      V_NAMEDINT, /**< integer / named number */
      V_NAMEDBITS, /**< named bits (identifiers) */
      V_INT, /**< integer */
      V_REAL, /**< real/float */
      V_ENUM, /**< enumerated */
      V_BSTR, /**< bitstring */
      V_HSTR, /**< hexstring */
      V_OSTR, /**< octetstring */
      V_CSTR, /**< charstring */
      V_USTR, /**< universal charstring */
      V_ISO2022STR, /**< ISO-2022 string (treat as octetstring) */
      V_CHARSYMS, /**< parsed ASN.1 universal string notation */
      V_OID, /**< object identifier */
      V_ROID, /**< relative object identifier */
      V_CHOICE, /**< choice; set directly by the ASN.1 parser */
      V_SEQOF, /**< sequence (record) of */
      V_SETOF, /**< set of */
      V_ARRAY, /**< array */
      V_SEQ, /**< sequence (record) */
      V_SET, /**< set */
      V_OPENTYPE, /**< open type */
      V_REFD, /**< referenced */
      V_UNDEF_LOWERID, /**< undefined loweridentifier */
      V_UNDEF_BLOCK, /**< undefined {block} */
      V_OMIT, /**< special value for optional values */
      V_VERDICT, /**< verdict */
      V_TTCN3_NULL, /**< TTCN-3 null (for component or default references) */
      V_DEFAULT_NULL, /**< null default reference */
      V_FAT_NULL, /**< null for function, altstep and testcase */
      V_EXPR, /**< expressions */
      V_MACRO, /**< macros (%%something) */
      V_NOTUSED, /**< not used symbol ('-') */
      V_FUNCTION, /**< function */
      V_ALTSTEP, /**< altstep */
      V_TESTCASE, /**< testcase */
      V_INVOKE, /**< invoke operation */
      V_REFER, /**< refer(function) */
      V_ANY_VALUE, /**< any value (?) - used by template concatenation */
      V_ANY_OR_OMIT /**< any or omit (*) - used by template concatenation */
    };

    enum verdict_t {
      Verdict_NONE,
      Verdict_PASS,
      Verdict_INCONC,
      Verdict_FAIL,
      Verdict_ERROR
    };

    /** Operation type, for V_EXPR */
    enum operationtype_t {
      OPTYPE_RND, // -
      OPTYPE_TESTCASENAME, // -

      OPTYPE_UNARYPLUS, // v1
      OPTYPE_UNARYMINUS, // v1
      OPTYPE_NOT, // v1
      OPTYPE_NOT4B, // v1

      OPTYPE_BIT2HEX, // v1  6
      OPTYPE_BIT2INT, // v1
      OPTYPE_BIT2OCT, // v1
      OPTYPE_BIT2STR, // v1
      OPTYPE_CHAR2INT, // v1  10
      OPTYPE_CHAR2OCT, // v1
      OPTYPE_FLOAT2INT, // v1
      OPTYPE_FLOAT2STR, // v1
      OPTYPE_HEX2BIT, // v1
      OPTYPE_HEX2INT, // v1
      OPTYPE_HEX2OCT, // v1
      OPTYPE_HEX2STR, // v1
      OPTYPE_INT2CHAR, // v1
      OPTYPE_INT2FLOAT, // v1
      OPTYPE_INT2STR, // v1   20
      OPTYPE_INT2UNICHAR, // v1
      OPTYPE_OCT2BIT, // v1
      OPTYPE_OCT2CHAR, // v1
      OPTYPE_OCT2HEX, // v1
      OPTYPE_OCT2INT, // v1
      OPTYPE_OCT2STR, // v1
      OPTYPE_OCT2UNICHAR, // v1 [v2]
      OPTYPE_STR2BIT, // v1
      OPTYPE_STR2FLOAT, // v1
      OPTYPE_STR2HEX, // v1
      OPTYPE_STR2INT, // v1    30
      OPTYPE_STR2OCT, // v1
      OPTYPE_UNICHAR2INT, // v1
      OPTYPE_UNICHAR2CHAR, // v1
      OPTYPE_UNICHAR2OCT, // v1 [v2]
      OPTYPE_ENUM2INT, // v1

      OPTYPE_ENCODE, // ti1 35

      OPTYPE_RNDWITHVAL, /** \todo -> SEED */ // v1

      OPTYPE_ADD, // v1 v2
      OPTYPE_SUBTRACT, // v1 v2
      OPTYPE_MULTIPLY, // v1 v2
      OPTYPE_DIVIDE, // v1 v2      40
      OPTYPE_MOD, // v1 v2
      OPTYPE_REM, // v1 v2
      OPTYPE_CONCAT, // v1 v2
      OPTYPE_EQ, // v1 v2
      OPTYPE_LT, // v1 v2
      OPTYPE_GT, // v1 v2
      OPTYPE_NE, // v1 v2
      OPTYPE_GE, // v1 v2
      OPTYPE_LE, // v1 v2
      OPTYPE_AND, // v1 v2    50
      OPTYPE_OR, // v1 v2
      OPTYPE_XOR, // v1 v2
      OPTYPE_AND4B, // v1 v2
      OPTYPE_OR4B, // v1 v2
      OPTYPE_XOR4B, // v1 v2
      OPTYPE_SHL, // v1 v2
      OPTYPE_SHR, // v1 v2
      OPTYPE_ROTL, // v1 v2
      OPTYPE_ROTR, // v1 v2

      OPTYPE_INT2BIT, // v1 v2    60
      OPTYPE_INT2HEX, // v1 v2
      OPTYPE_INT2OCT, // v1 v2

      OPTYPE_DECODE, // r1 r2

      OPTYPE_SUBSTR, // ti1 v2 v3
      OPTYPE_REGEXP, // ti1 t2 v3 b4
      OPTYPE_DECOMP, // v1 v2 v3     66

      OPTYPE_REPLACE, // ti1 v2 v3 ti4

      OPTYPE_ISVALUE, // ti1   68
      OPTYPE_ISBOUND, // ti1
      OPTYPE_ISPRESENT, // ti1
      OPTYPE_ISCHOSEN, // r1 i2
      OPTYPE_ISCHOSEN_V, // v1 i2
      OPTYPE_ISCHOSEN_T, // t1 i2

      OPTYPE_LENGTHOF, // ti1
      OPTYPE_SIZEOF, // ti1 
      OPTYPE_VALUEOF, // ti1
      OPTYPE_MATCH, // v1 t2

      OPTYPE_TTCN2STRING, // ti1
      OPTYPE_REMOVE_BOM, //v1
      OPTYPE_GET_STRINGENCODING, //v1
      OPTYPE_ENCODE_BASE64, //v1 [v2]
      OPTYPE_DECODE_BASE64, //v1

      /** cannot distinguish during parsing; can be COMP or TMR */
      OPTYPE_UNDEF_RUNNING, // r1 [r2] b4                   78
      OPTYPE_COMP_NULL, // - (from V_TTCN3_NULL)
      OPTYPE_COMP_MTC, // -
      OPTYPE_COMP_SYSTEM, // -
      OPTYPE_COMP_SELF, // -
      OPTYPE_COMP_CREATE, // r1 [v2] [v3] b4
      OPTYPE_COMP_RUNNING, // v1 [r2] b4
      OPTYPE_COMP_RUNNING_ANY, // -
      OPTYPE_COMP_RUNNING_ALL, // -
      OPTYPE_COMP_ALIVE, // v1
      OPTYPE_COMP_ALIVE_ANY, // -
      OPTYPE_COMP_ALIVE_ALL, // -
      OPTYPE_TMR_READ, // r1     90
      OPTYPE_TMR_RUNNING, // r1 [r2] b4
      OPTYPE_TMR_RUNNING_ANY, // -
      OPTYPE_GETVERDICT, // -
      OPTYPE_ACTIVATE, // r1
      OPTYPE_ACTIVATE_REFD, //v1 t_list2
      OPTYPE_EXECUTE, // r1 [v2]
      OPTYPE_EXECUTE_REFD, // v1 t_list2 [v3]

      OPTYPE_LOG2STR, // logargs
      OPTYPE_PROF_RUNNING, // -     99
      
      OPTYPE_ENCVALUE_UNICHAR, // ti1 [v2]
      OPTYPE_DECVALUE_UNICHAR, // r1 r2 [v3]
      
      OPTYPE_ANY2UNISTR, // logarg, length = 1
      OPTYPE_CHECKSTATE_ANY, // [r1] v2, port or any
      OPTYPE_CHECKSTATE_ALL, // [r1] v2, port or all
      OPTYPE_HOSTID, // [v1]
      OPTYPE_ISTEMPLATEKIND, // ti1 v2

      NUMBER_OF_OPTYPES // must be last
    };

    enum macrotype_t {
      MACRO_MODULEID, /**< module identifier (%moduleId) */
      MACRO_FILENAME, /**< name of input file (%fileName) */
      MACRO_BFILENAME, /**< name of input file (__BFILE__) */
      MACRO_FILEPATH, /**< canonical file name (name with full path) */
      MACRO_LINENUMBER, /**< line number (%lineNumber) */
      MACRO_LINENUMBER_C, /**< line number in C style (__LINE__)
			       gives an integer value */
      MACRO_DEFINITIONID, /**< name of (top level) definition (%definitionId) */
      /* the following macro may be needed in future versions of titan,
       * functionality will be explained by a quick study written by Kristof
      MACRO_NOTSCOPE_NAME_BUT_THE_NAME_OF_SOMETHING_ELSE */
      MACRO_SCOPE,
      MACRO_TESTCASEID /**< name of current testcase (%testcaseId)
			    (evaluated at runtime) */
    };

    enum expr_state_t {EXPR_NOT_CHECKED, EXPR_CHECKING,
                       EXPR_CHECKING_ERR, EXPR_CHECKED};
  private:

    valuetype_t valuetype;
    Type *my_governor;
    union {
      bool val_bool;
      int_val_t  *val_Int;
      Identifier *val_id;
      ttcn3float val_Real;
      struct {
        string *val_str;
	map<size_t, Value> *str_elements;
      } str;
      struct {
        ustring *val_ustr;
	map<size_t, Value> *ustr_elements;
	bool convert_str; /**< Indicates that universal charstring is */
	                   /* initialized with charstring */
      } ustr;
      CharSyms *char_syms;
      vector<OID_comp> *oid_comps;
      struct {
        Identifier *alt_name; /**< The name of the selected alternative */
        Value *alt_value; /**< The value given */
      } choice;
      Values *val_vs;
      NamedValues *val_nvs;
      struct {
        Reference *ref;
        Value *refd_last; /**< cache */
      } ref;
      Reference *refered;
      struct {
        operationtype_t v_optype;
        expr_state_t state;
        union {
          Value *v1;
          Template *t1;
          TemplateInstance *ti1;
          Ttcn::Ref_base *r1; /**< timer or component */
          LogArguments *logargs; /**< arguments of log2str() */
        };
        union {
          Value *v2;
          TemplateInstance *t2;
          Identifier *i2;
          Ttcn::ParsedActualParameters *t_list2;
          Ttcn::ActualParList *ap_list2;
          Ttcn::Ref_base *r2;
        };
        Value *v3;
        union {
          Value *v4;
          TemplateInstance *ti4;
          bool b4;
        };
      } expr;
      macrotype_t macro;
      struct {
        Value *v;
        Ttcn::ParsedActualParameters *t_list;
        Ttcn::ActualParList *ap_list;
      } invoke;
      map<string, Identifier> *ids;
      Block *block;
      verdict_t verdict;
      Common::Assignment *refd_fat; /** function, altstep, testcase */
      Ttcn::LengthRestriction* len_res; /** any value, any or omit */
    } u;
    
    /** Used to avoid infinite recursions of is_unfoldable() */
    class UnfoldabilityCheck {
      static map<Value*, void> running;
      Value* value;
    public:
      static bool is_running(Value* p_value) { return running.has_key(p_value); }
      UnfoldabilityCheck(Value* p_value) : value(p_value) { running.add(value, 0); }
      ~UnfoldabilityCheck() { running.erase(value); }
    };

    /** Copy constructor for Value::clone() only. */
    Value(const Value& p);
    /** Assignment op not implemented. */
    Value& operator=(const Value& p);
    /** Release resources. */
    void clean_up();
    /** Release resources if Value::valuetype == V_EXPR.
     * Called by Value::clean_up(). */
    void clean_up_expr();
    /** Copies (moves) the contents of \a src (valuetype & u) into \a this and
     * after that destroys (deletes) src. Before doing the copy the contents of
     * \a this are destroyed by calling \a clean_up(). */
    void copy_and_destroy(Value *src);
  public:
    Value(valuetype_t p_vt);
    Value(valuetype_t p_vt, bool p_val_bool);
    Value(valuetype_t p_vt, const Int& p_val_Int);
    Value(valuetype_t p_vt, int_val_t *p_val_Int);
    Value(valuetype_t p_vt, string *p_val_str);
    Value(valuetype_t p_vt, ustring *p_val_ustr);
    Value(valuetype_t p_vt, CharSyms *p_char_syms);
    Value(valuetype_t p_vt, Identifier *p_val_id);
    Value(valuetype_t p_vt, Identifier *p_id, Value *p_val);
    Value(valuetype_t p_vt, const Real& p_val_Real);
    Value(valuetype_t p_vt, Values *p_vs);
    Value(valuetype_t p_vt, Value *p_v, Ttcn::ParsedActualParameters *p_t_list);
    /** Constructor used by V_EXPR "-": RND, TESTCASENAME, COMP_NULL, COMP_MTC,
     *  COMP_SYSTEM, COMP_SELF, COMP_RUNNING_ANY, COMP_RUNNING_ALL,
     *  COMP_ALIVE_ALL, COMP_ALIVE_ANY, TMR_RUNNING_ANY, GETVERDICT,
     *  PROF_RUNNING */
    Value(operationtype_t p_optype);
    /** Constructor used by V_EXPR "v1" or [v1] */
    Value(operationtype_t p_optype, Value *p_v1);
    /** Constructor used bt V_EXPR "v1 [r2] b4": COMP_RUNNING, COMP_ALIVE */
    Value(operationtype_t p_optype, Value* p_v1, Ttcn::Ref_base* p_r2, bool p_b4);
    /** Constructor used by V_EXPR "ti1": LENGTHOF, SIZEOF, VALUEOF, TTCN2STRING */
    Value(operationtype_t p_optype, TemplateInstance *p_ti1);
    /** Constructor used by V_EXPR "r1": TMR_READ, ACTIVATE */
    Value(operationtype_t p_optype, Ttcn::Ref_base *p_r1);
    /** Constructor used by V_EXPR "r1 [r2] b4": UNDEF_RUNNING */
    Value(operationtype_t p_optype, Ttcn::Ref_base* p_r1, Ttcn::Ref_base* p_r2,
      bool p_b4);
    /** ACTIVATE_REFD, OPTYPE_INVOKE, OPTYPE_COMP_ALIVE_REFD */
    Value(operationtype_t p_optype, Value *p_v1,
      Ttcn::ParsedActualParameters *p_ap_list);
    /** OPTYPE_EXECUTE_REFD */
    Value(operationtype_t p_optype, Value *p_v1,
      Ttcn::ParsedActualParameters *p_t_list2, Value *p_v3);
    /** Constructor used by V_EXPR "r1 [v2]": EXECUTE or [r1] v2 */
    Value(operationtype_t p_optype, Ttcn::Ref_base *p_r1, Value *v2);
    /** Constructor used by V_EXPR "r1 [v2] [v3] b4": COMP_CREATE */
    Value(operationtype_t p_optype, Ttcn::Ref_base *p_r1, Value *p_v2,
      Value *p_v3, bool p_b4);
    /** Constructor used by V_EXPR "v1 v2" */
    Value(operationtype_t p_optype, Value *p_v1, Value *p_v2);
    /** Constructor used by V_EXPR "v1 v2 v3" */
    Value(operationtype_t p_optype, Value *p_v1, Value *p_v2, Value *p_v3);
    /** Constructor used by encvalue_unichar "ti1 [v2]" */
    Value(operationtype_t p_optype, TemplateInstance *p_ti1, Value *p_v2);
    /** Constructor used by V_EXPR "ti1 v2 v3" */
    Value(operationtype_t p_optype, TemplateInstance *p_ti1, Value *p_v2, Value *p_v3);
    /** Constructor used by V_EXPR "ti1 t2 v3 b4" */
    Value(operationtype_t p_optype, TemplateInstance *p_ti1, TemplateInstance *p_t2,
      Value *p_v3, bool p_b4);
    /** Constructor used by V_EXPR "ti1 v2 v3 ti4" */
    Value(operationtype_t p_optype, TemplateInstance *p_ti1, Value *p_v2,
      Value *p_v3, TemplateInstance *p_ti4);
    /** Constructor used by V_EXPR "v1 t2": MATCH */
    Value(operationtype_t p_optype, Value *p_v1, TemplateInstance *p_t2);
    /** Constructor used by V_EXPR "r1 i2": ISPRESENT, ISCHOSEN */
    Value(operationtype_t p_optype, Ttcn::Reference *p_r1, Identifier *p_i2);
    Value(operationtype_t p_optype, LogArguments *p_logargs);
    /** Constructor used by V_MACRO */
    Value(valuetype_t p_vt, macrotype_t p_macrotype);
    Value(valuetype_t p_vt, NamedValues *p_nvs);
    /** V_REFD, V_REFER */
    Value(valuetype_t p_vt, Reference *p_ref);
    Value(valuetype_t p_vt, Block *p_block);
    Value(valuetype_t p_vt, verdict_t p_verdict);
    /** Constructor used by decode */
    Value(operationtype_t p_optype, Ttcn::Ref_base *p_r1, Ttcn::Ref_base *p_r2);
    /** Constructor used by decvalue_unichar*/
    Value(operationtype_t p_optype, Ttcn::Ref_base *p_r1, Ttcn::Ref_base *p_r2, Value *p_v3);
    /** Constructor used by V_ANY_VALUE and V_ANY_OR_OMIT */
    Value(valuetype_t p_vt, Ttcn::LengthRestriction* p_len_res);
    virtual ~Value();
    virtual Value* clone() const;
    valuetype_t get_valuetype() const {return valuetype;}
    /** it it is not V_EXPR then fatal error. */
    operationtype_t get_optype() const;
    Value* get_concat_operand(bool first_operand) const;
    Ttcn::LengthRestriction* take_length_restriction();
    /** Sets the governor type. */
    virtual void set_my_governor(Type *p_gov);
    /** Gets the governor type. */
    virtual Type *get_my_governor() const;
    /** Returns true if it is a value reference. */
    bool is_ref() const { return valuetype == V_REFD; }
    virtual void set_fullname(const string& p_fullname);
    virtual void set_my_scope(Scope *p_scope);
  private:
    void set_fullname_expr(const string& p_fullname);
    void set_my_scope_expr(Scope *p_scope);
  public:
    void set_genname_recursive(const string& p_genname);
    void set_genname_prefix(const char *p_genname_prefix);
    void set_code_section(code_section_t p_code_section);
    void change_sign();
    void add_oid_comp(OID_comp* p_comp);
    void set_valuetype(valuetype_t p_valuetype);
    void set_valuetype_COMP_NULL();
    void set_valuetype(valuetype_t p_valuetype, const Int& p_val_int);
    void set_valuetype(valuetype_t p_valuetype, string *p_str);
    void set_valuetype(valuetype_t p_valuetype, Identifier *p_id);
    void set_valuetype(valuetype_t p_valuetype, Assignment *p_ass);
    void add_id(Identifier *p_id);
    /** Follows the chain of references if any and returns the last
     *  element of the chain. In other words, the returned value is
     *  the first value which is not a reference. */
    Value* get_value_refd_last
    (ReferenceChain *refch=0,
     Type::expected_value_t exp_val=Type::EXPECTED_DYNAMIC_VALUE);
    Value *get_refd_sub_value(Ttcn::FieldOrArrayRefs *subrefs,
                              size_t start_i, bool usedInIsbound,
                              ReferenceChain *refch,
                              bool silent = false);
    /** Returns true if the value is unknown at compile-time. */
    bool is_unfoldable
    (ReferenceChain *refch=0,
     Type::expected_value_t exp_val=Type::EXPECTED_DYNAMIC_VALUE);

    /** Returns whether the value is a single identifier, which can be either
     * a reference or an enum value. */
    bool is_undef_lowerid();
    /** Returns the single identifier.
     * It can be used only if \a is_undef_lowerid() returned true. */
    const Identifier& get_undef_lowerid();
    /** Converts the single identifier to a referenced value. */
    void set_lowerid_to_ref();
    /** Check if ref++; can be used instead of ref = ref + 1; */
    bool can_use_increment(Reference *ref) const;

    /** If this value is used in an expression, what is its return type? */
    Type::typetype_t get_expr_returntype
    (Type::expected_value_t exp_val=Type::EXPECTED_DYNAMIC_VALUE);
    /** If this value is used in an expression, what is its governor?
     *  If has no governor, but is reference, then tries the
     *  referenced stuff... If not known, returns NULL. */
    Type* get_expr_governor(Type::expected_value_t exp_val);
    Type* get_expr_governor_v1v2(Type::expected_value_t exp_val);
    Type* get_expr_governor_last();
    /** get the type invoked */
    Type *get_invoked_type(Type::expected_value_t exp_val);
  private:
    const char* get_opname() const;
    /** Used to determine whether the reference points to value or
     *  template in ISCHOSEN, then convert to {ISCHOSEN}_{V,T} */
    void chk_expr_ref_ischosen();
    void chk_expr_operandtype_enum(const char *opname, Value *v,
                                   Type::expected_value_t exp_val);
    void chk_expr_operandtype_bool(Type::typetype_t tt, const char *opnum,
                                   const char *opname, const Location *loc);
    void chk_expr_operandtype_int(Type::typetype_t tt, const char *opnum,
                                  const char *opname, const Location *loc);
    void chk_expr_operandtype_float(Type::typetype_t tt, const char *opnum,
                                    const char *opname, const Location *loc);
    void chk_expr_operandtype_int_float(Type::typetype_t tt, const char *opnum,
                                        const char *opname, const Location *loc);
    void chk_expr_operandtype_int_float_enum(Type::typetype_t tt,
                                             const char *opnum,
                                             const char *opname,
                                             const Location *loc);
    /** Check that the operand is a list or string or something like it.
     *
     * This Value is usually an expression like
     * @code lengthof(foo) @endcode
     *
     * @param t the type (governor) of the operand
     * @param opnum describes the operand ("first", "left", ...)
     * @param opname the name of the operation
     * @param loc the operand itself as a Location object
     * @param allow_array \a true if T_ARRAY is acceptable (rotate, lengthof)
     * or \a false if it's not (concat, substr, replace).
     */
    void chk_expr_operandtype_list(Type* t, const char *opnum,
                                  const char *opname, const Location *loc,
                                  bool allow_array);
    // Check that the operand of the operation is a string value
    void chk_expr_operandtype_str(Type::typetype_t tt, const char *opnum,
                                  const char *opname, const Location *loc);
    void chk_expr_operandtype_charstr(Type::typetype_t tt, const char *opnum,
                                      const char *opname, const Location *loc);
    void chk_expr_operandtype_cstr(Type::typetype_t tt, const char *opnum,
                                   const char *opname, const Location *loc);
    void chk_expr_operandtype_binstr(Type::typetype_t tt, const char *opnum,
                                     const char *opname, const Location *loc);
    void chk_expr_operandtype_bstr(Type::typetype_t tt, const char *opnum,
                                   const char *opname, const Location *loc);
    void chk_expr_operandtype_hstr(Type::typetype_t tt, const char *opnum,
                                   const char *opname, const Location *loc);
    void chk_expr_operandtype_ostr(Type::typetype_t tt, const char *opnum,
                                   const char *opname, const Location *loc);
    void chk_expr_operandtypes_same(Type::typetype_t tt1, Type::typetype_t tt2,
                                    const char *opname);
    void chk_expr_operandtypes_same_with_opnum(Type::typetype_t tt1, Type::typetype_t tt2,
                                               const char *opnum1, const char *opnum2,
                                               const char *opname);
    void chk_expr_operandtypes_compat(Type::expected_value_t exp_val,
                                      Value *v1, Value *v2,
                                      const char *opnum1 = "left",
                                      const char *opnum2 = "right");
    void chk_expr_operand_undef_running(Type::expected_value_t exp_val,
                                        Ttcn::Ref_base *ref, bool any_from,
                                        Ttcn::Ref_base* index_ref,
                                        const char *opnum, const char *opname);
    /** Returns the referred component type if it is correct. */
    Type *chk_expr_operand_comptyperef_create();
    /** Checks whether the special component references mtc, system and self
     * have the correct component types (compatible with \a my_governor). */
    void chk_expr_comptype_compat();
    Type* chk_expr_operand_compref(Value *val, const char *opnum,
                                  const char *opname, bool any_from = false);
    void chk_expr_operand_tmrref(Ttcn::Ref_base *ref,
                                 const char *opnum, const char *opname,
                                 bool any_from = false);
    void chk_expr_operand_activate(Ttcn::Ref_base *ref,
                                   const char *opnum, const char *opname);
    void chk_expr_operand_activate_refd(Value *val,
                                        Ttcn::TemplateInstances* t_list2,
                                        Ttcn::ActualParList *&parlist,
                                        const char *opnum, const char *opname);
    void chk_expr_operand_execute(Ttcn::Ref_base *ref, Value *val,
                                  const char *opnum, const char *opname);
    void chk_expr_operand_execute_refd(Value *v1,
                                       Ttcn::TemplateInstances* t_list2,
                                       Ttcn::ActualParList *&parlist,
                                       Value *v3, const char *opnum,
                                       const char *opname);
    void chk_invoke(Type::expected_value_t exp_val);
    void chk_expr_eval_value(Value *val, Type &t,
                             ReferenceChain *refch,
                             Type::expected_value_t exp_val);
    void chk_expr_eval_ti(TemplateInstance *ti, Type *t,
      ReferenceChain *refch, Type::expected_value_t exp_val);
    void chk_expr_val_int_pos0(Value *val, const char *opnum,
                               const char *opname);
    void chk_expr_val_int_pos7bit(Value *val, const char *opnum,
                                  const char *opname);
    void chk_expr_val_int_pos31bit(Value *val, const char *opnum,
                                   const char *opname);
    void chk_expr_val_int_float_not0(Value *val, const char *opnum,
                                     const char *opname);
    void chk_expr_val_large_int(Value *val, const char *opnum,
                                const char *opname);
    void chk_expr_val_len1(Value *val, const char *opnum,
                           const char *opname);
    void chk_expr_val_str_len_even(Value *val, const char *opnum,
                                   const char *opname);
    void chk_expr_val_str_bindigits(Value *val, const char *opnum,
                                    const char *opname);
    void chk_expr_val_str_hexdigits(Value *val, const char *opnum,
                                    const char *opname);
    void chk_expr_val_str_7bitoctets(Value *val, const char *opnum,
                                    const char *opname);
    void chk_expr_val_str_int(Value *val, const char *opnum,
                              const char *opname);
    void chk_expr_val_str_float(Value *val, const char *opnum,
                                const char *opname);
    void chk_expr_val_ustr_7bitchars(Value *val, const char *opnum,
                                    const char *opname);
    void chk_expr_val_bitstr_intsize(Value *val, const char *opnum,
                                     const char *opname);
    void chk_expr_val_hexstr_intsize(Value *val, const char *opnum,
                                     const char *opname);
    void chk_expr_operands_int2binstr();
    void chk_expr_operands_str_samelen();
    void chk_expr_operands_replace();
    void chk_expr_operands_substr();
    void chk_expr_operands_regexp();
    void chk_expr_operands_ischosen(ReferenceChain *refch,
      Type::expected_value_t exp_val);
    void chk_expr_operand_encode(ReferenceChain *refch,
      Type::expected_value_t exp_val);
    /** \a has two possible value: OPTYPE_DECODE | OPTYPE_DECVALUE_UNICHAR */
    void chk_expr_operands_decode(operationtype_t p_optype);
    /** Checks whether \a this can be compared with omit value (i.e. \a this
     * should be a referenced value pointing to a optional record/set field. */
    void chk_expr_omit_comparison(Type::expected_value_t exp_val);
    Int chk_eval_expr_sizeof(ReferenceChain *refch,
                             Type::expected_value_t exp_val);
    /** The governor is returned. */
    Type *chk_expr_operands_ti(TemplateInstance* ti, Type::expected_value_t exp_val);
    void chk_expr_operands_match(Type::expected_value_t exp_val);
    void chk_expr_dynamic_part(Type::expected_value_t exp_val,
			       bool allow_controlpart,
			       bool allow_runs_on = true,
			       bool require_runs_on = false);
    /** Check the operands of an expression */
    void chk_expr_operands(ReferenceChain *refch,
                           Type::expected_value_t exp_val);
    void chk_expr_operand_valid_float(Value* v, const char *opnum, const char *opname);
    /** Evaluate...
     * Called by Value::get_value_refd_last() for V_EXPR */
    void evaluate_value(ReferenceChain *refch, Type::expected_value_t exp_val);
    /** Evaluate macro.
     * Called by Value::get_value_refd_last() for V_MACRO */
    void evaluate_macro(Type::expected_value_t exp_val);
    /** Compile-time evaluation of isvalue()
     *
     * @param from_sequence \c true if called for a member of a sequence/set
     * @return the compile-time result of calling isvalue()
     */
    bool evaluate_isvalue(bool from_sequence);

    Value *get_refd_field_value(const Identifier& field_id, bool usedInIsbound,
                                const Location& loc, bool silent);
    Value *get_refd_array_value(Value *array_index, bool usedInIsbound,
                                ReferenceChain *refch, bool silent);
    Value *get_string_element(const Int& index, const Location& loc);

  public:
    /** Checks whether the value (expression) evaluates to built-in type
     * \a p_tt. Argument \a type_name is the name of that type, it is used in
     * error messages. */
    void chk_expr_type(Type::typetype_t p_tt, const char *type_name,
      Type::expected_value_t exp_val);
    /** Checks that the value (expression) evals to a boolean value */
    inline void chk_expr_bool(Type::expected_value_t exp_val)
      { chk_expr_type(Type::T_BOOL, "boolean", exp_val); }
    /** Checks that the value (expression) evals to an integer value */
    inline void chk_expr_int(Type::expected_value_t exp_val)
      { chk_expr_type(Type::T_INT, "integer", exp_val); }
    /** Checks that the value (expression) evals to a float value */
    inline void chk_expr_float(Type::expected_value_t exp_val)
      { chk_expr_type(Type::T_REAL, "float", exp_val); }
    /** Checks that the value (expression) evals to a verdict value */
    inline void chk_expr_verdict(Type::expected_value_t exp_val)
      { chk_expr_type(Type::T_VERDICT, "verdict", exp_val); }
    /** Checks that the value (expression) evals to a default value */
    inline void chk_expr_default(Type::expected_value_t exp_val)
      { chk_expr_type(Type::T_DEFAULT, "default", exp_val); }
    
    /** Checks that the value is (or evaluates to) a valid universal charstring
      * encoding format. */
    bool chk_string_encoding(Common::Assignment* lhs);

    /* if "infinity" or "-infinity" was parsed then this is a real value or
       a unary - expression containing a real value, where the real value is
       infinity.
       Returns -1 if -infinity was parsed, +1 if infinity, 0 in other cases */
    int is_parsed_infinity();

    bool get_val_bool();
    int_val_t* get_val_Int();
    const Identifier* get_val_id();
    const ttcn3float& get_val_Real();
    string get_val_str();
    ustring get_val_ustr();
    string get_val_iso2022str();
    size_t get_val_strlen();
    verdict_t get_val_verdict();
    size_t get_nof_comps();
    bool is_indexed() const;
    const Identifier& get_alt_name();
    Value *get_alt_value();
    /** Sets the first letter in the name of the alternative to lowercase
      * if it's an uppercase letter.
      * Used on open types (the name of their alternatives can be given with both
      * an uppercase or a lowercase first letter, and the generated code will need
      * to use the lowercase version). */
    void set_alt_name_to_lowercase();
    /** Returns whether the embedded object identifier components
     *  contain any error. Applicable to OID/ROID values only. */
    bool has_oid_error();
    /** Collects all object identifier components of the value into \a
     *  comps.  It follows and expands the embedded references.
     *  Applicable to OID/ROID values and referenced values pointing
     *  to them. 
     *  @return true, if all components can be calculated in compile-time */
    bool get_oid_comps(vector<string>& comps);
    /** Get a component of SEQUENCE/SET, zero-based */
    void add_se_comp(NamedValue* nv); // needed by implicit_omit
    NamedValue* get_se_comp_byIndex(size_t n);
    /** Get a component of an array/REC-OF/SET-OF */
    Value *get_comp_byIndex(size_t n);
    /** Get an index of an array/REC-OF/SET-OF */
    Value *get_index_byIndex(size_t n);
    /** Does a named component exist ? For SEQ/SET/CHOICE */
    bool has_comp_withName(const Identifier& p_name);
    NamedValue* get_se_comp_byName(const Identifier& p_name);
    Value* get_comp_value_byName(const Identifier& p_name);
    bool field_is_present(const Identifier& p_name);
    bool field_is_chosen(const Identifier& p_name);
    void chk_dupl_id();
    size_t get_nof_ids() const;
    Identifier* get_id_byIndex(size_t p_i);
    bool has_id(const Identifier& p_id);
    Reference *get_reference() const;
    Reference *get_refered() const;
    Common::Assignment *get_refd_fat() const;
    /** Usable during AST building. If VariableRef is needed, then use
     *  the return value of this function, then delete this. */
    Ttcn::Reference* steal_ttcn_ref();
    Ttcn::Ref_base* steal_ttcn_ref_base();
    void steal_invoke_data(Value*& p_v, Ttcn::ParsedActualParameters*& p_ti,
                          Ttcn::ActualParList*& p_ap);
    Common::Assignment* get_refd_assignment();

    void chk();
    void chk_OID(ReferenceChain& refch);
    void chk_ROID(ReferenceChain& refch);
    /** Checks for circular references within embedded values */
    void chk_recursions(ReferenceChain& refch);
    /** Checks for circular references within embedded expressions */
    void chk_recursions_expr(ReferenceChain& refch);
    void chk_recursions_expr_decode(Ttcn::Ref_base* ref, ReferenceChain& refch);
    /** Check that the value (a V_EXPR) - being used as the RHS - refers to
     *  the LHS of the assignment.
     *  @return true if self-assignment*/
    bool chk_expr_self_ref(Common::Assignment *lhs);

    virtual string create_stringRepr();
  private:
    static bool chk_expr_self_ref_templ(Ttcn::Template *t, Common::Assignment *lhs);
    static bool chk_expr_self_ref_val  (Common::Value  *v, Common::Assignment *lhs);
    string create_stringRepr_unary(const char *operator_str);
    string create_stringRepr_infix(const char *operator_str);
    string create_stringRepr_predef1(const char *function_name);
    string create_stringRepr_predef2(const char *function_name);
  public:
    bool operator==(Value& val);
    bool operator<(Value& val);
  public:
    /** Returns true if this value is of a string type */
    bool is_string_type(Type::expected_value_t exp_val);
    /** Public entry points for code generation. */
    /** Generates the equivalent C++ code for the value. It is used
     *  when the value is part of a complex expression (e.g. as
     *  operand of a built-in operation, actual parameter, array
     *  index). The generated code fragments are appended to the
     *  fields of visitor \a expr. */
    void generate_code_expr(expression_struct *expr);
    /** Generates the C++ equivalent of \a this into \a expr and adds a "()"
     * to expr->expr if \a this is referenced value that points to an optional
     * field of a record/set value. */
    void generate_code_expr_mandatory(expression_struct *expr);
    /** Generates a C++ code sequence, which initializes the C++
     *  object named \a name with the contents of the value. The code
     *  sequence is appended to argument \a str and the resulting
     *  string is returned. */
    char *generate_code_init(char *str, const char *name);
    /** Appends the initialization sequence of all referred
     *  non-parameterized templates to \a str and returns the
     *  resulting string. Such templates may appear in the actual
     *  parameter list of parameterized value references
     *  (e.g. function calls) and in operands of valueof or match
     *  operations. */
    char *rearrange_init_code(char *str, Common::Module* usage_mod);
    /**
     *  Generates a value for temporary use. Example:
     *
     *  str: // empty
     *  prefix: if(
     *  blockcount: 0
     *
     *  if the value is simple, then returns:
     *
     *  // empty
     *  if(simple
     *
     *  if the value is complex, then returns:
     *
     *  // empty
     *  {
     *    booelan tmp_2;
     *    {
     *      preamble... tmp_1...
     *      tmp_2=func5(tmp_1);
     *      postamble
     *    }
     *    if(tmp_2
     *
     *  and also increments the blockcount because you have to close it...
     */
    char* generate_code_tmp(char *str, const char *prefix, size_t& blockcount);
    char* generate_code_tmp(char *str, char*& init);
    /** Generates the C++ statement that puts the value of \a this into the
     * log. It is used when the value appears in the argument of a log()
     * statement. */
    void generate_code_log(expression_struct *expr);
    /** Generates the C++ equivalent of a match operation if it
     * appears within a log() statement. Applicable only if \a this is
     * an expression containing a match operation. */
    void generate_code_log_match(expression_struct *expr);

  private:
    /** Private helper functions for code generation. */
    /** Helper function for \a generate_code_expr(). It is used when
     *  the value is an expression. */
    void generate_code_expr_expr(expression_struct *expr);
    /** Helper function for \a generate_code_expr_expr(). It handles
     *  unary operators. */
    static void generate_code_expr_unary(expression_struct *expr,
                                         const char *operator_str, Value *v1);
    /** Helper function for \a generate_code_expr_expr().  It handles infix
     *  binary operators.  Flag \a optional_allowed is true when the operation
     *  has different meaning on optional fields (like comparison).  Otherwise
     *  the optional operands are explicitly converted to a regular data
     *  type.  */
    void generate_code_expr_infix(expression_struct *expr,
                                  const char *operator_str, Value *v1,
                                  Value *v2, bool optional_allowed);
    /** Helper function for \a generate_code_expr_expr().  It handles the
     *  logical "and" and "or" operation with short-circuit evaluation
     *  semantics.  */
    void generate_code_expr_and_or(expression_struct *expr);
    /** Helper function for \a generate_code_expr_expr(). It handles
     *  built-in functions with one argument. */
    static void generate_code_expr_predef1(expression_struct *expr,
                                           const char *function_name,
                                           Value *v1);
    /** Helper function for \a generate_code_expr_expr(). It handles
     *  built-in functions with two arguments. */
    static void generate_code_expr_predef2(expression_struct *expr,
                                           const char *function_name,
                                           Value *v1, Value *v2);
    /** Helper function for \a generate_code_expr_expr(). It handles
     *  built-in functions with three arguments. */
    static void generate_code_expr_predef3(expression_struct *expr,
                                           const char *function_name,
                                           Value *v1, Value *v2, Value *v3);
    /** Helper functions for \a generate_code_expr_expr(). */
    void generate_code_expr_substr(expression_struct *expr);
    void generate_code_expr_substr_replace_compat(expression_struct *expr);
    void generate_code_expr_regexp(expression_struct *expr);
    void generate_code_expr_replace(expression_struct *expr);
    /** Helper function for \a generate_code_expr_expr(). It handles
     *  built-in function rnd(). */
    static void generate_code_expr_rnd(expression_struct *expr,
                                       Value *v1);
    /** Helper function for \a generate_code_expr_expr(). It handles
     *  create(). */
    static void generate_code_expr_create(expression_struct *expr,
      Ttcn::Ref_base *type, Value *name, Value *location, bool alive);
    /** Helper function for \a generate_code_expr_expr(). It handles
     *  activate(). */
    void generate_code_expr_activate(expression_struct *expr);
    /** Helper function for \a generate_code_expr_expr(). It handles
     *  activate() with derefers(). */
    void generate_code_expr_activate_refd(expression_struct *expr);
    /** Helper function for \a generate_code_expr_expr(). It handles
     *  execute(). */
    void generate_code_expr_execute(expression_struct *expr);
    /** Helper function for \a generate_code_expr_expr(). It handles
     *  execute() with derefers(). */
    void generate_code_expr_execute_refd(expression_struct *expr);
    /** Helper function for \a generate_code_expr(). It handles invoke */
    void generate_code_expr_invoke(expression_struct *expr);

    /** Adds the character sequence "()" to expr->expr if \a ref points to
     * an optional field of a record/set value. */
    static void generate_code_expr_optional_field_ref(expression_struct *expr,
                                                      Reference *ref);

    void generate_code_expr_encode(expression_struct *expr);

    void generate_code_expr_decode(expression_struct *expr);
    
    void generate_code_expr_encvalue_unichar(expression_struct *expr);
    
    void generate_code_expr_decvalue_unichar(expression_struct *expr);
    
    void generate_code_expr_checkstate(expression_struct *expr);
    
    void generate_code_expr_hostid(expression_struct *expr);
    
    char* generate_code_char_coding_check(expression_struct *expr, Value *v, const char *name);

    /** Helper function for \a generate_code_init(). It handles the
     *  union (CHOICE) values. */
    char *generate_code_init_choice(char *str, const char *name);
    /** Helper function for \a generate_code_init(). It handles the
     *  'record of'/'set of' ('SEQUENCE OF'/'SET OF') values. */
    char *generate_code_init_seof(char *str, const char *name);
    /** Helper function for \a generate_code_init(). It handles the
     *  indexed value notation for 'record of'/'set of' ('SEQUENCE OF'/
     *  'SET OF') values. */
    char *generate_code_init_indexed(char *str, const char *name);
    /** Helper function for \a generate_code_init(). It handles the
     *  array values. */
    char *generate_code_init_array(char *str, const char *name);
    /** Helper function for \a generate_code_init(). It handles the
     *  record/set (SEQUENCE/SET) values. */
    char *generate_code_init_se(char *str, const char *name);
    /** Helper function for \a generate_code_init(). It handles the
     *  referenced values. */
    char *generate_code_init_refd(char *str, const char *name);

  public:
    /** Generates JSON code from this value. Used in JSON schema generation.
      * No code is generated for special float values NaN, INF and -INF if the
      * 2nd parameter is false. */
    void generate_json_value(JSON_Tokenizer& json,
      bool allow_special_float = true, bool union_value_list = false,
      Ttcn::JsonOmitCombination* omit_combo = NULL);
    
    /** Returns whether C++ explicit cast (type conversion) is necessary when
     * \a this is the argument of a send() or log() statement. True is returned
     * when the type of the C++ equivalent is ambiguous or is a built-in type
     * that has to be converted to the respective TTCN-3 value class
     * (e.g. int -> class INTEGER). */
    bool explicit_cast_needed(bool forIsValue = false);
    /** Returns whether the value can be represented by an in-line C++
     *  expression. */
    bool has_single_expr();
    /** Returns the equivalent C++ expression. It can be used only if
     *  \a has_single_expr() returns true. */
    string get_single_expr();
  private:
    /** Helper function for has_single_expr(). Used when the value contains
     * an expression */
    bool has_single_expr_expr();
    /** Helper function for has_single_expr(). Used for invoke operations and
     * for activate and execute combined with derefer */
    static bool has_single_expr_invoke(Value *v, Ttcn::ActualParList *ap_list);
    /** Helper function for \a get_single_expr(). Used for enumerated
     *  values only. It considers the enum-hack option. */
    string get_single_expr_enum();
    /** Helper function for \a get_single_expr(). Used for ISO2022
     *  string values only. The generated code refers to the
     *  user-defined conversion functions of the RTE. */
    string get_single_expr_iso2022str();
    /** Helper function for \a get_single_expr(). Used by literal function,
     * altstep and testcase values denoted by refer(). */
    string get_single_expr_fat();
    /** Returns whether the value is compound (i.e. record, set,
     *  union, record of, set of). */
    bool is_compound();
    /** Returns whether the C++ initialization sequence requires a
     *  temporary variable reference to be introduced for efficiency
     *  reasons. */
    bool needs_temp_ref();
    /** Returns whether the evaluation of \a this has side-effects that shall
     * be eliminated in case of short-circuit evaluation of logical "and" and
     * "or" operations. This function is applied on the second (right) operand
     * of the expression. */
    bool needs_short_circuit();
  public:
    virtual void dump(unsigned level) const;
  private:
    inline void set_val_str(string *p_val_str)
      { u.str.val_str = p_val_str; u.str.str_elements = 0; }
    inline void set_val_ustr(ustring *p_val_ustr)
      { u.ustr.val_ustr = p_val_ustr; u.ustr.ustr_elements = 0; }
    void add_string_element(size_t index, Value *v_element,
      map<size_t, Value>*& string_elements);
  };

  /** @} end of AST_Value group */

  class LazyFuzzyParamData {
    static int depth; // recursive code generation: calling a func. with lazy/fuzzy param inside a lazy/fuzzy param
    static bool used_as_lvalue;
    static vector<string>* type_vec;
    static vector<string>* refd_vec;
  public:
    static bool in_lazy_or_fuzzy();
    static void init(bool p_used_as_lvalue);
    static void clean();
    static string add_ref_genname(Assignment* ass, Scope* scope);
    static string get_member_name(size_t idx);
    static string get_constr_param_name(size_t idx);
    static void generate_code_for_value(expression_struct* expr, Value* val, Scope* my_scope);
    static void generate_code_for_template(expression_struct* expr, TemplateInstance* temp, template_restriction_t gen_restriction_check, Scope* my_scope);
    static void generate_code(expression_struct *expr, Value* value, Scope* scope, boolean lazy);
    static void generate_code(expression_struct *expr, TemplateInstance* temp, template_restriction_t gen_restriction_check, Scope* scope, boolean lazy);
    static void generate_code_param_class(expression_struct *expr, expression_struct& param_expr, const string& param_id, const string& type_name, boolean lazy);
    static void generate_code_ap_default_ref(expression_struct *expr, Ttcn::Ref_base* ref, Scope* scope, boolean lazy);
    static void generate_code_ap_default_value(expression_struct *expr, Value* value, Scope* scope, boolean lazy);
    static void generate_code_ap_default_ti(expression_struct *expr, TemplateInstance* ti, Scope* scope, boolean lazy);
  };
  
} // namespace Common

#endif // _Common_Value_HH
