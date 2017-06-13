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
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *   Tatarka, Gabor
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/

#ifndef _Common_Type_HH
#define _Common_Type_HH

#include "Setting.hh"
#include "Code.hh"
#include "Int.hh"
#include "subtype.hh"
#include "ttcn3/rawASTspec.h"
#include "ttcn3/RawAST.hh"
#include "ttcn3/TextAST.hh"
#include "ttcn3/BerAST.hh"
#include "ttcn3/JsonAST.hh"
#include <float.h>

class XerAttributes;
enum namedbool { INCOMPLETE_NOT_ALLOWED = 0, INCOMPLETE_ALLOWED = 1, WARNING_FOR_INCOMPLETE = 2,
  NO_SUB_CHK = 0, SUB_CHK = 3,
  OMIT_NOT_ALLOWED = 0, OMIT_ALLOWED = 4,
  ANY_OR_OMIT_NOT_ALLOWED = 0, ANY_OR_OMIT_ALLOWED = 5,
  NOT_IMPLICIT_OMIT = 0, IMPLICIT_OMIT = 6,
  NOT_STR_ELEM = 0, IS_STR_ELEM = 7
};

namespace Asn {
  // not defined here
  class Tags;
  class Tag;
  class TagCollection;
  class Block;
  class OC_defn;
  class TableConstraint;
} // namespace Asn

namespace Ttcn {
  // not defined here
  class ArrayDimension;
  class FieldOrArrayRefs;
  class Template;
  class Definitions;
  class Definition;
  class SingleWithAttrib;
  class MultiWithAttrib;
  class WithAttribPath;
  class FormalPar;
  class FormalParList;
  class Reference;
  class PortTypeBody;
  class Def_Type;
  class Ref_pard;
} // namespace Ttcn

// not defined here
class JSON_Tokenizer;

namespace Common {

  /**
   * \ingroup AST
   *
   * \defgroup AST_Type Type
   * @{
   */
  using Asn::Tag;
  using Asn::Tags;
  using Asn::TagCollection;
  using Asn::Block;
  using Asn::OC_defn;
  using Asn::TableConstraint;
  using Ttcn::Template;

  class Type;

  // not defined here
  class Identifier;
  class Constraints;
  class Value;
  class CompField;
  class CompFieldMap;
  class EnumItem;
  class EnumItems;
  class ExcSpec;
  class NamedValues;
  class CTs_EE_CTs;
  class TypeSet;
  class TypeChain;
  class TypeCompatInfo;
  class ComponentTypeBody;
  class SignatureParam;
  class SignatureParamList;
  class SignatureExceptions;
  class CodeGenHelper;
  class Assignment;

  /**
   * This is the base class for types.
   */
  class Type : public Governor {
  public:

    /** type of type */
    enum typetype_t {
      /** Undefined.
       *  There is never a Type object with this typetype.
       *  It may be returned by Value::get_expr_returntype() or
       *  ValueRange::get_expr_returntype().        */
      T_UNDEF,
      T_ERROR, /**< erroneous (e.g. nonexistent reference) */
      T_NULL, /**< null (ASN.1) */
      T_BOOL, /**< boolean */
      T_INT, /**< integer */
      T_INT_A, /**< integer / ASN */
      T_REAL, /**< real/float */
      T_ENUM_A, /**< enumerated / ASN */
      T_ENUM_T, /**< enumerated / TTCN */
      T_BSTR, /**< bitstring */
      T_BSTR_A, /**< bitstring */
      T_HSTR, /**< hexstring (TTCN-3) */
      T_OSTR, /**< octetstring */
      T_CSTR, /**< charstring (TTCN-3) */
      T_USTR, /**< universal charstring (TTCN-3) */
      T_UTF8STRING, /**< UTF8String (ASN.1) */
      T_NUMERICSTRING, /**< NumericString (ASN.1) */
      T_PRINTABLESTRING, /**< PrintableString (ASN.1) */
      T_TELETEXSTRING, /**< TeletexString (ASN.1) */
      T_VIDEOTEXSTRING, /**< VideotexString (ASN.1) */
      T_IA5STRING, /**< IA5String (ASN.1) */
      T_GRAPHICSTRING, /**< GraphicString (ASN.1) */
      T_VISIBLESTRING, /**< VisibleString (ASN.1) */
      T_GENERALSTRING, /**< GeneralString (ASN.1) */
      T_UNIVERSALSTRING,  /**< UniversalString (ASN.1) */
      T_BMPSTRING, /**< BMPString (ASN.1) */
      T_UNRESTRICTEDSTRING, /**< UnrestrictedCharacterString (ASN.1) */
      T_UTCTIME, /**< UTCTime (ASN.1) */
      T_GENERALIZEDTIME, /**< GeneralizedTime (ASN.1) */
      T_OBJECTDESCRIPTOR, /** Object descriptor, a kind of string (ASN.1) */
      T_OID, /**< object identifier */
      T_ROID, /**< relative OID (ASN.1) */
      T_CHOICE_A, /**< choice /ASN, uses u.secho */
      T_CHOICE_T, /**< union /TTCN, uses u.secho */
      T_SEQOF, /**< sequence (record) of */
      T_SETOF, /**< set of */
      T_SEQ_A, /**< sequence /ASN, uses u.secho */
      T_SEQ_T, /**< record /TTCN, uses u.secho */
      T_SET_A, /**< set /ASN, uses u.secho */
      T_SET_T, /**< set /TTCN, uses u.secho */
      T_OCFT, /**< ObjectClassFieldType (ASN.1) */
      T_OPENTYPE, /**< open type (ASN.1) */
      T_ANY, /**< ANY (deprecated ASN.1) */
      T_EXTERNAL, /**< %EXTERNAL (ASN.1) */
      T_EMBEDDED_PDV, /**< EMBEDDED PDV (ASN.1) */
      T_REFD, /**< referenced */
      T_REFDSPEC, /**< special referenced (by pointer, not by name) */
      T_SELTYPE, /**< selection type (ASN.1) */
      T_VERDICT, /**< verdict type (TTCN-3) */
      T_PORT, /**< port type (TTCN-3) */
      T_COMPONENT, /**< component type (TTCN-3) */
      T_ADDRESS, /**< address type (TTCN-3) */
      T_DEFAULT, /**< default type (TTCN-3) */
      T_ARRAY, /**< array (TTCN-3), uses u.array */
      T_SIGNATURE, /**< signature (TTCN-3) */
      T_FUNCTION, /**< function reference (TTCN-3) */
      T_ALTSTEP, /**< altstep reference (TTCN-3) */
      T_TESTCASE, /**< testcase reference (TTCN-3) */
      T_ANYTYPE, /**< anytype (TTCN-3) */
      // WRITE new type before this line
      T_LAST
    };       //DO NOT FORGET to update type_as_string[] in Type.cc 

    /**
     * Enumeration to represent message encoding types.
     */
    enum MessageEncodingType_t {
      CT_UNDEF, /**< undefined/unused */
      CT_BER,   /**< ASN.1 BER (built-in) */
      CT_PER,   /**< ASN.1 PER (through user defined coder functions) */
      CT_RAW,   /**< TTCN-3 RAW (built-in) */
      CT_TEXT,  /**< TTCN-3 TEXT (built-in) */
      CT_XER,    /**< TTCN-3 XER (built-in) */
      CT_JSON,   /**< TTCN-3 and ASN.1 JSON (built-in) */
      CT_CUSTOM  /**< user defined encoding (through user defined coder functions) */
    };

    /** selector for value checking algorithms */
    enum expected_value_t {
      /** the value must be known at compile time (i.e. it may refer
       *  to a TTCN-3 constant or an ASN.1 value) */
      EXPECTED_CONSTANT,
      /** the value must be static at execution time, but may be
       *  unknown at compilation time (i.e. it may refer to a TTCN-3
       *  module parameter as well) */
      EXPECTED_STATIC_VALUE,
      /** the value is known only at execution time (i.e. it may refer
       *  to a variable in addition to static values) */
      EXPECTED_DYNAMIC_VALUE,
      /** the reference may point to a dynamic value or a template
       *  (this selector is also used in template bodies where the
       *  variable references are unaccessible because of the scope
       *  hierarchy) */
      EXPECTED_TEMPLATE
    };

    /** Enumeration to represent the owner of the type.
      */
    enum TypeOwner_t {
      OT_UNKNOWN,
      OT_TYPE_ASS, ///< ASN.1 type assignment (Ass_T)
      OT_VAR_ASS,  ///< ASN.1 variable assignment (Ass_V)
      OT_VSET_ASS, ///< ASN.1 value set assignment (Ass_VS)
      OT_TYPE_FLD, ///< ASN.1 TypeFieldSpec (FieldSpec_T)
      OT_FT_V_FLD, ///< ASN.1 FixedTypeValueFieldSpec (FieldSpec_V_FT)
      OT_TYPE_MAP, ///< TTCN-3 TypeMapping
      OT_TYPE_MAP_TARGET, ///< TTCN-3 TypeMappingTarget
      OT_TYPE_DEF, ///< TTCN-3 type definition (Def_Type)
      OT_CONST_DEF, ///< TTCN-3 constant definition (DefConst, Def_ExtCOnst)
      OT_MODPAR_DEF, ///< TTCN-3 module parameter definition (Def_Modulepar)
      OT_VAR_DEF, ///< TTCN-3 variable definition (Def_Var)
      OT_VARTMPL_DEF, ///< TTCN-3 var template definition (Def_Var_Template)
      OT_FUNCTION_DEF, ///< TTCN-3 function (Def_Function, Def_ExtFunction)
      OT_TEMPLATE_DEF, ///< TTCN-3 template definition (Def_Template)
      OT_ARRAY, ///< another Type: TTCN-3 array(T_ARRAY)
      OT_RECORD_OF, ///< another Type (T_SEQOF, T_SETOF), ASN.1 or TTCN-3
      OT_FUNCTION, ///< another Type: TTCN-3 function (T_FUNCTION)
      OT_SIGNATURE, ///< another Type: TTCN-3 signature (T_SIGNATURE)
      OT_REF, ///< another Type (T_REFD)
      OT_REF_SPEC, ///< another Type (T_REFDSPEC)
      OT_COMP_FIELD, ///< a field of a record/set/union (CompField)
      OT_COMPS_OF, ///< ASN.1 "COMPONENTS OF" (CT_CompsOf)
      OT_FORMAL_PAR, ///< formal parameter (FormalPar), TTCN-3
      OT_TYPE_LIST, ///< TypeList for a 'with "extension anytype t1,t2..." '
      OT_FIELDSETTING, ///< ASN.1 FieldSetting_Type
      OT_SELTYPE, ///< another Type (T_SELTYPE), ASN.1 selection type
      OT_OCFT, ///< another Type (T_OCFT), ASN.1 obj.class field type
      OT_TEMPLATE_INST, ///< a TemplateInstance (TTCN-3)
      OT_RUNSON_SCOPE, ///< a RunsOnScope (TTCN-3)
      OT_PORT_SCOPE, ///< a port scope
      OT_EXC_SPEC, ///< exception Specification (ExcSpec)
      OT_SIG_PAR, ///< signature parameter (SignatureParam)
      OT_POOL ///< It's a pool type, owned by the type pool
    };
    
    /**
     * Enumeration to represent the default method of encoding or decoding for a type.
     */
    enum coding_type_t {
      CODING_UNSET, ///< No encoding/decoding method has been set for the type.
      CODING_BUILT_IN, ///< The type uses a built-in codec for encoding/decoding.
      CODING_BY_FUNCTION, ///< The type uses a user defined function for encoding/decoding.
      CODING_MULTIPLE ///< Multiple encoding/decoding methods have been set for the type.
    };
    
    /**
     * Structure containing the default encoding or decoding settings for a type.
     * These settings determine how values of the type are encoded or decoded by
     * the following TTCN-3 language elements:
     * 'encvalue' (encode), 'encvalue_unichar' (encode), 'decvalue' (decode),
     * 'decvalue_unichar' (decode), 'decmatch' (decode) and '@decoded' (decode).
     */
    struct coding_t {
      coding_type_t type; ///< Type of encoding/decoding
      union {
        MessageEncodingType_t built_in_coding; ///< Built-in codec (if type is CODING_BUILT_IN)
        Assignment* function_def; ///< Pointer to external function definition (if type is CODING_BY_FUNCTION)
      };
    };

    /** Returns the display string of \a encoding_type. */
    static const char *get_encoding_name(MessageEncodingType_t encoding_type);
    /** Returns a pool type that represents the encoded stream of the given
     * \a encoding_type. */
    static Type *get_stream_type(MessageEncodingType_t encoding_type, int stream_variant=0);

    enum truth {
      No, Maybe, Yes
    };

  private:
    typetype_t typetype;
    bool tags_checked;
    bool tbl_cons_checked;
    bool text_checked;
    bool json_checked;
    bool raw_parsed;
    bool raw_checked;
    bool xer_checked;
    bool raw_length_calculated;
    bool has_opentypes;
    bool opentype_outermost;
    bool code_generated;
    bool embed_values_possible;
    bool use_nil_possible;
    bool use_order_possible;
    int raw_length;
    Type *parent_type;
    Tags *tags;
    Constraints *constraints;
    /// The type's attributes (with context)
    Ttcn::WithAttribPath* w_attrib_path;
    /** A copy of all the AT_ENCODEs  */
    Ttcn::WithAttribPath* encode_attrib_path;
    RawAST *rawattrib;
    TextAST *textattrib;
    XerAttributes *xerattrib;
    BerAST *berattrib;
    JsonAST *jsonattrib;

    vector<SubTypeParse> *parsed_restr; ///< parsed subtype restrictions are stored here until they are moved to the sub_type member
    SubType *sub_type; ///< effective/aggregate subtype of this type, NULL if neither inherited nor own subtype restrictions exist

    coding_t default_encoding; ///< default settings for encoding values of this type
    coding_t default_decoding; ///< default settings for decoding values of this type
    
    /** What kind of AST element owns the type.
     *  It may not be known at creation type, so it's initially OT_UNKNOWN.
     *  We want this information so we don't have to bother with XER
     *  if the type is an ASN.1 construct, or it's the type in a "runs on" scope,
     *  the type of a variable declaration/module par/const, etc. */
    TypeOwner_t ownertype;
    Node *owner;

    union {
      struct {
        Block *block;
        NamedValues *nvs;
      } namednums;
      struct {
        EnumItems *eis; ///< Final set of enum items
        Int first_unused;  ///< First  unused >=0 value (for UNKNOWN_VALUE)
        Int second_unused; ///< Second unused >=0 value (for UNBOUND_VALUE)
        Block *block; ///< ASN.1 block to be parsed
        EnumItems *eis1; ///< First set of enum items before the ellipsis
        bool ellipsis; ///< true if there was an ellipsis, false otherwise
        ExcSpec *excSpec; ///< Exception specification
        EnumItems *eis2; ///< Second set of enum items after the ellipsis
        map<string, size_t> *eis_by_name;
      } enums;
      struct {
        CompFieldMap *cfm;
        Block *block; ///< Unparsed block; will be 0 after parse_block_...()
        CTs_EE_CTs *ctss;
        bool tr_compsof_ready;
        bool component_internal;
        map<string, size_t> *field_by_name;
        OC_defn *oc_defn; /**< link to...  */
        const Identifier *oc_fieldname; /**< link to...  */
        const TableConstraint *my_tableconstraint; /**< link to...  */
        bool has_single_charenc; /**< Has a single character-encodable field
                                  * with the UNTAGGED encoding instruction.
                                  * X693amd1 32.2.2 applies to the field. */
      } secho; /**< data for T_(SEQUENCE|SET_CHOICE)_[AT] */
      struct {
        Type *ofType;
        bool component_internal;
      } seof; /**< data for SEQUENCE OF/SET OF */
      struct {
        Reference *ref;
        Type *type_refd; /**< link to...  */
        OC_defn *oc_defn; /**< link to...  */
        const Identifier *oc_fieldname; /**< link to...  */
        bool component_internal;
      } ref;
      struct {
        Identifier *id;
        Type *type;
        Type *type_refd;
      } seltype;
      struct {
        Type *element_type;
	Ttcn::ArrayDimension *dimension;
	bool in_typedef;
        bool component_internal;
      } array;
      Ttcn::PortTypeBody *port;
      ComponentTypeBody *component;
      Type *address; /**< link to...  */
      struct {
	SignatureParamList *parameters;
	Type *return_type;
	bool no_block;
        bool component_internal;
	SignatureExceptions *exceptions;
      } signature;
      struct {
        Ttcn::FormalParList *fp_list;
	struct {
          bool self;
          Ttcn::Reference *ref;
          Type *type; // useful only after check
	} runs_on;
        union {
          Type *return_type;
          struct {
            Ttcn::Reference *ref;
            Type *type; // useful only after check
          } system;
        };
        bool returns_template;
        template_restriction_t template_restriction;
        bool is_startable;
      } fatref;
    } u;
    static const char* type_as_string[];
    
    /** True if chk() has finished running. 
      * Prevents force_raw() from running chk_raw(), chk_text() or chk_json() on unchecked types. */
    bool chk_finished;
    
    /** Signifies that this type is an instance of an ASN.1 parameterized type.
      * It will not have its own segment and reference generated in the JSON schema,
      * its schema segment will be generated as an embedded type's would. */
    bool pard_type_instance;
    
    /** Indicates that the component array version (used with the help of the
      * 'any from' clause) of the 'done' function needs to be generated for
      * this type. */
    bool needs_any_from_done;

    /** Copy constructor, for the use of Type::clone() only. */
    Type(const Type& p);
    /** Assignment disabled */
    Type& operator=(const Type& p);
    /** Set fields to their default values */
    void init();
    /** Free up resources */
    void clean_up();
    /** Returns the default tag of the type. */
    Tag *get_default_tag();
    /** Returns the number in UNIVERSAL tag class that belongs to ASN.1 type
     * type \a p_tt. In case of invalid argument -1 is returned */
    static int get_default_tagnumber(typetype_t p_tt);
    /** Container for the allocated tags that do not belong to a particular
     * Type object */
    static map<typetype_t, Tag> *default_tags;
    static void destroy_default_tags();
    /** Container for the allocated pool types */
    static map<typetype_t, Type> *pooltypes;
    /** Drops the elements of \a pooltypes */
    static void destroy_pooltypes();
    /** Returns the TTCN-3 equivalent of \a p_tt. */
    static typetype_t get_typetype_ttcn3(typetype_t p_tt);

  public:
    /** @name Constructors
     *  @{ */
    /// Construct a predefined type (including anytype and address)
    Type(typetype_t p_tt);
    /// Construct a TTCN enumerated type
    Type(typetype_t p_tt, EnumItems *p_eis);
    /** Construct an ASN.1 enum, sequence, set, choice, integer with
     * named numbers, or a bitstring with named bits */
    Type(typetype_t p_tt, Block *p_block);
    /// Construct an ASN.1 enum, with or without extension
    Type(typetype_t p_tt,
         EnumItems *p_eis1, bool p_ellipsis, EnumItems *p_eis2);
    /// Construct a TTCN3 sequence, set or choice
    Type(typetype_t p_tt, CompFieldMap *p_cfm);
    /** Construct a type with an embedded type: a record-of, set-of,
    * or a special reference (involved in: ASN.1 table constraint,
    * TTCN3 (ext)const definition, module parameter, variable instance) */
    Type(typetype_t p_tt, Type *p_type);
    /// Create an ASN.1 selection type
    Type(typetype_t p_tt, Identifier *p_id, Type *p_type);
    /// ASN.1 ObjectClassFieldType
    Type(typetype_t p_tt, Type *p_type, OC_defn *p_oc_defn,
         const Identifier *p_id);
    /// Create a TTCN3 array
    Type(typetype_t p_tt, Type *p_type, Ttcn::ArrayDimension *p_dim,
	 bool p_in_typedef);
    /// Create an ASN.1 open type
    Type(typetype_t p_tt, OC_defn *p_oc_defn, const Identifier *p_id);
    /// Create a reference
    Type(typetype_t p_tt, Reference *p_ref);
    /// Create a TTCN3 port type
    Type(typetype_t p_tt, Ttcn::PortTypeBody *p_pb);
    /// Create a TTCN3 component type
    Type(typetype_t p_tt, ComponentTypeBody *p_cb);
    /// Create a TTCN3 signature
    Type(typetype_t p_tt, SignatureParamList *p_params, Type *p_returntype,
	bool p_noblock, SignatureExceptions *p_exceptions);
    /// Create a TTCN3 function reference
    Type(typetype_t p_tt,Ttcn::FormalParList *p_params,
        Ttcn::Reference* p_runs_on_ref, bool p_runs_on_self,
        Type *p_returntype, bool p_returns_template,
        template_restriction_t p_template_restriction);
    /// Create a TTCN3 altstep
    Type(typetype_t p_tt,Ttcn::FormalParList *p_params,
        Ttcn::Reference* p_runs_on_ref, bool p_runs_on_self);
    /// Create a TTCN3 testcase
    Type(typetype_t p_tt,Ttcn::FormalParList *p_params,
        Ttcn::Reference* p_runs_on_ref, Ttcn::Reference *p_system_ref);
    /** @} */
    virtual ~Type();
    /** This function must be called to clean up the pool types,
     *  tags, etc. It is called by main() */
    static void free_pools();
    /** Create a (partial) copy of the object */
    virtual Type* clone() const;
    /** Return the type of type. */
    typetype_t get_typetype() const { return typetype; }
    /** Returns the TTCN-3 equivalent of \a typetype. */
    typetype_t get_typetype_ttcn3() const
      { return get_typetype_ttcn3(typetype); }
    /** Returns a simple/built-in type from the pool */
    static Type* get_pooltype(typetype_t p_typetype);
    /** Returns whether the type is defined in an ASN.1 module. */
    bool is_asn1() const;
    /** Returns true if it is a type reference. */
    bool is_ref() const;
    /** Returns true if it is a Sequence, Set or Choice. */
    bool is_secho() const;
    /** Returns true if this is a character-encodable type.
     *  Being character-encodable is a requirement for a type to be
     *  encoded as an XML attribute. */
    truth is_charenc();
    /** Return true if at least one abstract value of the type
     *  has an empty "ExtendedXMLValue" encoding (only possible with EXER).
     *  Possible for record/set when all components are optional,
     *  and record-of/set-of when 0 length is not forbidden. */
    bool has_empty_xml();
    /** Returns whether \a this is a sequence-of/record-of or set-of type. */
    bool is_seof() const { return typetype == T_SEQOF || typetype == T_SETOF; }
    /** Returns the \a sub_type member */
    SubType* get_sub_type() const { return sub_type; }
    /** If this is a reference, returns the referenced type.
     *  Otherwise, it's a fatal error. */
    Type* get_type_refd(ReferenceChain *refch=0);
    /** Walk through all references to the referenced type */
    Type* get_type_refd_last(ReferenceChain *refch=0);
    /** Returns the type of the field, which is referenced by \a subrefs from
     * \a this. It checks the array indices against \a expected_index. In case
     * of error NULL is returned.
     * Special case: if \a interrupt_if_optional is true then return NULL if an
     * optional field has been reached. Using this bool parameter it can be
     * checked if a referenced field is on an optional path (used by template
     * restriction checking code) */
    Type *get_field_type(Ttcn::FieldOrArrayRefs *subrefs,
      expected_value_t expected_index, ReferenceChain *refch = 0,
      bool interrupt_if_optional = false);
    /** subrefs must point to an existing field, get_field_type() should be used
      * to check. subrefs_array will be filled with the indexes of the fields,
      * type_array will be filled with types whose field indexes were collected,
      * if an invalid index is encountered false will be returned */
    bool get_subrefs_as_array(const Ttcn::FieldOrArrayRefs *subrefs,
      dynamic_array<size_t>& subrefs_array, dynamic_array<Type*>& type_array);
    /** Returns whether the last field referenced by \a subrefs is an optional
     * record/SEQUENCE or set/SET field. It can be used only after a successful
     * semantic check (e.g. during code generation) or the behaviour will be
     * unpredictable. */
    bool field_is_optional(Ttcn::FieldOrArrayRefs *subrefs);
    /** Returns whether this type instance is the type of an optional field.  */
    bool is_optional_field() const;
    virtual void set_fullname(const string& p_fullname);
    /** Sets the internal pointer my_scope to \a p_scope. */
    virtual void set_my_scope(Scope *p_scope);
    /** Checks the type (including tags). */
    virtual void chk();
    /** Return whether the two typetypes are compatible.  Sometimes, this is
     *  just a question of \p p_tt1 == \p p_tt2.  When there are multiple
     *  typetypes for a type (e.g. T_ENUM_A and T_ENUM_T) then all
     *  combinations of those are compatible. In case of anytype the
     *  module has to be the same  */
    static bool is_compatible_tt_tt(typetype_t p_tt1, typetype_t p_tt2,
                                    bool p_is_asn11, bool p_is_asn12,
                                    bool same_module);
    /** Returns whether the type is compatible with \a p_tt.  Used if the
     *  other value is unfoldable, but we can determine its expr_typetype.
     *  Note: The compatibility relation is asymmetric.  The function returns
     *  true if the set of possible values in \a p_type is a subset of
     *  possible values in \a this.  */
    bool is_compatible_tt(typetype_t p_tt, bool p_is_asn1, Type* p_type = 0);
    /** Returns whether this type is compatible with \a p_type.  Note: The
     *  compatibility relation is asymmetric.  The function returns true if
     *  the set of possible values in \a p_type is a subset of possible values
     *  in \a this.  It returns false if the two types cannot be compatible
     *  ever.  If the two types are compatible, but they need additional
     *  "type" conversion code generated with run-time checks \p p_info will
     *  provide more information.  \p p_info is used to collect type
     *  information to report more precise errors.  \p p_left_chain and
     *  \p p_right_chain are there to prevent infinite recursion.
     *  \p p_left_chain contains the type chain of the left operand from the
     *  "root" type to this point in the type's structure.  \p p_right_chain
     *  is the same for the right operand.
     * \p p_is_inline_template indicates that the conversion is requested for an
     * inline template. Type conversion code is not needed in this case. */
    bool is_compatible(Type *p_type, TypeCompatInfo *p_info,
                       Location* p_loc,
                       TypeChain *p_left_chain = NULL,
                       TypeChain *p_right_chain = NULL,
                       bool p_is_inline_template = false);
    /** Check if the restrictions of a T_SEQOF/T_SETOF are "compatible" with
     *  the given type \a p_type.  Can be called only as a T_SEQOF/T_SETOF.
     *  Currently, used for structured types only.  \a p_type can be any kind
     *  of structured type.  */
    bool is_subtype_length_compatible(Type *p_type);
    /** Check if it's a structured type.  Return true if the current type is a
     *  structured type, false otherwise.  */
    bool is_structured_type() const;
    /** returns the type as a string */
    virtual const char* asString() const;
    static const char* asString(typetype_t type);

  private:
    /** Helper functions for is_compatible().  These functions can be called
     *  only for a Type that the name suggests.  \p p_type is the Type we're
     *  checking compatibility against.  \p p_info is used to collect type
     *  information to report more precise errors.  \p p_left_chain and
     *  \p p_right_chain are there to prevent infinite recursion.
     *  \p p_left_chain contains the type chain of the left operand from the
     *  "root" type to this point in the type's structure.  \p p_right_chain
     *  is the same for the right operand. 
     *  \p p_is_inline_template indicates that the conversion is requested for an
     *  inline template. Type conversion code is not needed in this case. */
    bool is_compatible_record(Type *p_type, TypeCompatInfo *p_info,
                              Location* p_loc,
                              TypeChain *p_left_chain = NULL,
                              TypeChain *p_right_chain = NULL,
                              bool p_is_inline_template = false);
    bool is_compatible_record_of(Type *p_type, TypeCompatInfo *p_info,
                                 Location* p_loc,
                                 TypeChain *p_left_chain = NULL,
                                 TypeChain *p_right_chain = NULL,
                                 bool p_is_inline_template = false);
    bool is_compatible_set(Type *p_type, TypeCompatInfo *p_info,
                           Location* p_loc,
                           TypeChain *p_left_chain = NULL,
                           TypeChain *p_right_chain = NULL,
                           bool p_is_inline_template = false);
    bool is_compatible_set_of(Type *p_type, TypeCompatInfo *p_info,
                              Location* p_loc,
                              TypeChain *p_left_chain = NULL,
                              TypeChain *p_right_chain = NULL,
                              bool p_is_inline_template = false);
    bool is_compatible_array(Type *p_type, TypeCompatInfo *p_info,
                             Location* p_loc,
                             TypeChain *p_left_chain = NULL,
                             TypeChain *p_right_chain = NULL,
                             bool p_is_inline_template = false);
    bool is_compatible_choice_anytype(Type *p_type, TypeCompatInfo *p_info,
                                      Location* p_loc,
                                      TypeChain *p_left_chain = NULL,
                                      TypeChain *p_right_chain = NULL,
                                      bool p_is_inline_template = false);
  public:
    /** Returns whether this type is identical to \a p_type from TTCN-3 point
     *  of view.  Note: This relation is symmetric.  The function returns true
     *  only if the sets of possible values are the same in both types.  */
    bool is_identical(Type *p_type);
    /** Tasks: Decides whether the type needs automatic tagging, performs the
     *  COMPONENTS OF transformations (recursively) and adds the automatic
     *  tags. */
    void tr_compsof(ReferenceChain *refch = 0);
    /** Returns whether the T_FUNCTION is startable.
     *  @pre typetype is T_FUNCTION, or else FATAL_ERROR occurs.  */
    bool is_startable();

    /** Returns true if this is a list type (string, rec.of, set.of or array */
    bool is_list_type(bool allow_array);

    /** Sets the encoding or decoding function for the type (in case of custom
      * or PER encoding). */
    void set_coding_function(bool encode, Assignment* function_def);
    
    /** Sets the codec to use when encoding or decoding the ASN.1 type */
    void set_asn_coding(bool encode, MessageEncodingType_t new_coding);
    
    /** Determines the method of encoding or decoding for values of this type
      * based on its attributes and on encoder or decoder function definitions
      * with this type as their input or output. An error is displayed if the
      * coding method cannot be determined.
      *
      * @note Because this check depends on the checks of other AST elements
      * (external functions), it is sometimes delayed to the end of the semantic
      * analysis. */
    void chk_coding(bool encode, Module* usage_mod, bool delayed = false);
    
    /** Indicates whether the type is encoded/decoded by a function or by a
      * built-in codec. */
    bool is_coding_by_function(bool encode) const;
    
    /** Returns the string representation of the type's default codec.
      * Used during code generation for types encoded/decoded by a built-in
      * codec. */
    string get_coding(bool encode) const;
    
    /** Returns the function definition of the type's encoder/decoder function.
      * Used during code generation for types encoded/decoded by functions. */
    Assignment* get_coding_function(bool encode) const;
    
  private:
    static MessageEncodingType_t get_enc_type(const Ttcn::SingleWithAttrib& enc);

    void chk_Int_A();
    void chk_Enum_A();
    void chk_Enum_item(EnumItem *ei, bool after_ellipsis,
                       map<Int, EnumItem>& value_map);
    void chk_Enum_T();
    void chk_BStr_A();
    void chk_SeCho_T();
    void chk_Choice_A();
    void chk_Se_A();
    void chk_SeOf();
    void chk_refd();
    void chk_seltype();
    void chk_Array();
    void chk_Signature();
    void chk_Fat();
  public:
    /** Checks whether the type can be the TTCN-3 address type. Not allowed:
     * the default type, references pointing to port types, component types
     * and signatures */
    void chk_address();
    /** Checks whether the type can be a component of another type definition
     *  (e.g. field of a structured type, parameter/return type/exception of a
     *  signature).
     *  Not allowed types: ports, signatures.
     *  Default type is allowed only within structured types.
     *  The text for the end of the error message is passed as parameter.
     */
    void chk_embedded(bool default_allowed, const char *error_msg);
    /** Checks for circular references within embedded types */
    void chk_recursions(ReferenceChain& refch);
    /** Checks that the structured type does not have fields
     * with the name of its definition.
     */
    void chk_constructor_name(const Identifier& p_id);
    bool chk_startability();
    /** Checks if it can be a return type */
    void chk_as_return_type(bool as_value, const char* what);
  private:
    void parse_block_Int();
    void parse_block_Enum();
    void parse_block_BStr();
    void parse_block_Choice();
    void parse_block_Se();
    int get_length_multiplier();
    void parse_attributes();
    void chk_raw();
    /** If the type does not have a rawattrib, create one. */
    void force_raw();
    void chk_text();
    void chk_text_matching_values(textAST_matching_values *matching_values,
      const char *attrib_name);
    /** If the type does not have a textattrib, create one. */
    void force_text();
    
    void chk_json();
    void chk_json_default();
    /** If the type does not have a jsonattrib, create one. */
    void force_json();

    void chk_xer();
    void chk_xer_any_attributes();
    void chk_xer_any_element();
    void chk_xer_attribute();
    void chk_xer_dfe();
    Value *new_value_for_dfe(Type *last, const char *dfe_str, Common::Reference* ref = NULL, bool is_ref_dfe = false);
    void target_of_text(string& text);
    void chk_xer_embed_values(int num_attributes);
    void chk_xer_text();
    void chk_xer_untagged();
    void chk_xer_use_nil();
    void chk_xer_use_order(int num_attributes);
    void chk_xer_use_type();
    void chk_xer_use_union();

    bool is_root_basic();
    int get_raw_length();
  public:
    void set_parent_type(Type *p_parent_type) {parent_type=p_parent_type;}
    Type* get_parent_type() const {return parent_type;}
    void set_has_opentypes() {has_opentypes=true;}
    bool get_has_opentypes() const {return has_opentypes;}
    void set_opentype_outermost() {opentype_outermost=true;}
    bool get_is_opentype_outermost() const {return opentype_outermost;}
    /** If \a value is an undef lowerid, then this member decides
     * whether it is a reference or a lowerid value (e.g., enum, named
     * number). */
    void chk_this_value_ref(Value *value);
    bool chk_this_value(Value *value, Common::Assignment *lhs,
      expected_value_t expected_value, namedbool incomplete_allowed,
      namedbool omit_allowed, namedbool sub_chk,
      namedbool implicit_omit = NOT_IMPLICIT_OMIT,
      namedbool str_elem = NOT_STR_ELEM);
    /** Checks the given referenced value */
    bool chk_this_refd_value(Value *value, Common::Assignment *lhs,
      expected_value_t expected_value, ReferenceChain *refch=0,
      namedbool str_elem = NOT_STR_ELEM);
    /** Checks the given invocation */
    void chk_this_invoked_value(Value *value, Common::Assignment *lhs,
      expected_value_t expected_value);
  private:
    void chk_this_value_Null(Value *value);
    void chk_this_value_Bool(Value *value);
    void chk_this_value_Int(Value *value);
    void chk_this_value_Int_A(Value *value);
    void chk_this_value_Real(Value *value);
    void chk_this_value_Enum(Value *value);
    void chk_this_value_BStr(Value *value);
    void chk_this_value_namedbits(Value *value);
    void chk_this_value_BStr_A(Value *value);
    void chk_this_value_HStr(Value *value);
    void chk_this_value_OStr(Value *value);
    void chk_this_value_CStr(Value *value);
    void chk_this_value_OID(Value *value);
    void chk_this_value_ROID(Value *value);
    void chk_this_value_Any(Value *value);
    bool chk_this_value_Choice(Value *value, Common::Assignment *lhs,
      expected_value_t expected_value, namedbool incomplete_allowed,
      namedbool implicit_omit = NOT_IMPLICIT_OMIT);
    bool chk_this_value_Se(Value *value, Common::Assignment *lhs,
      expected_value_t expected_value, namedbool incomplete_allowed,
      namedbool implicit_omit = NOT_IMPLICIT_OMIT);
    bool chk_this_value_Se_T(Value *value, Common::Assignment *lhs,
      expected_value_t expected_value, namedbool incomplete_allowed,
      namedbool implicit_omit = NOT_IMPLICIT_OMIT);
    bool chk_this_value_Seq_T(Value *value, Common::Assignment *lhs,
      expected_value_t expected_value, namedbool incomplete_allowed,
      namedbool implicit_omit = NOT_IMPLICIT_OMIT);
    bool chk_this_value_Set_T(Value *value, Common::Assignment *lhs,
      expected_value_t expected_value, namedbool incomplete_allowed,
      namedbool implicit_omit = NOT_IMPLICIT_OMIT);
    bool chk_this_value_Se_A(Value *value, Common::Assignment *lhs,
      expected_value_t expected_value, namedbool implicit_omit);
    bool chk_this_value_Seq_A(Value *value, Common::Assignment *lhs,
      expected_value_t expected_value, namedbool implicit_omit);
    bool chk_this_value_Set_A(Value *value, Common::Assignment *lhs,
      expected_value_t expected_value, namedbool implicit_omit);
    bool chk_this_value_SeOf(Value *value, Common::Assignment *lhs,
      expected_value_t expected_value, namedbool incomplete_allowed,
      namedbool implicit_omit = NOT_IMPLICIT_OMIT);
    void chk_this_value_Verdict(Value *value);
    void chk_this_value_Default(Value *value);
    bool chk_this_value_Array(Value *value, Common::Assignment *lhs,
      expected_value_t expected_value, namedbool incomplete_allowed,
      namedbool implicit_omit);
    bool chk_this_value_Signature(Value *value, Common::Assignment *lhs,
      expected_value_t expected_value, namedbool incomplete_allowed);
    void chk_this_value_Component(Value *value);
    void chk_this_value_FAT(Value *value);
  public:
    /** Checks whether template \a t is a specific value and the embedded value
     * is a referenced one. If the reference in the value points to a
     * template-like entity then it sets the template type to TEMPLATE_REFD,
     * otherwise it leaves everything as is. */
    void chk_this_template_ref(Template *t);
    /** Checks for self references in functions returning templates (external or otherwise)
      * and in parametrised templates. Returns true if the reference in assignment \a lhs 
      * is found. 
      * Recursive: calls itself incase of multiple embedded functions / parametrised templates.*/
    bool chk_this_template_ref_pard(Ttcn::Ref_pard* ref_pard, Common::Assignment* lhs);
    bool chk_this_template_generic(Template *t, namedbool incomplete_allowed,
     namedbool allow_omit, namedbool allow_any_or_omit, namedbool sub_chk,
     namedbool implicit_omit, Common::Assignment *lhs);
  private:
    bool chk_this_refd_template(Template *t, Common::Assignment *lhs);
    void chk_this_template_length_restriction(Template *t);
    bool chk_this_template_concat_operand(Template* t, namedbool implicit_omit,
      Common::Assignment *lhs);
    bool chk_this_template(Template *t, namedbool incomplete_allowed, namedbool sub_chk,
      namedbool implicit_omit, Common::Assignment *);
    bool chk_this_template_Str(Template *t, namedbool implicit_omit,
      Common::Assignment *lhs);
    /** Checks whether \a v is a correct range boundary for this type.
     * Applicable to the following types: integer, float, charstring,
     * universal charstring.
     * Argument \a v might be NULL in case of + or - infinity.
     * Argument \a which shall contain the word "lower" or "upper".
     * Argument \a loc is used for error reporting if \a v is NULL (it points
     * to the surrounding template).
     * If \a v is correct and it is or refers to a constant the constant value
     * is returned for further checking. Otherwise the return value is NULL. */
    Value *chk_range_boundary(Value *v, const char *which, const Location& loc);
    void chk_range_boundary_infinity(Value *v, bool is_upper);
    void chk_this_template_builtin(Template *t);
    void chk_this_template_Int_Real(Template *t);
    void chk_this_template_Enum(Template *t);
    bool chk_this_template_Choice(Template *t, namedbool incomplete_allowed,
      namedbool implicit_omit, Common::Assignment *lhs);
    bool chk_this_template_Seq(Template *t, namedbool incomplete_allowed,
      namedbool implicit_omit, Common::Assignment *lhs);
    bool chk_this_template_Set(Template *t, namedbool incomplete_allowed,
      namedbool implicit_omit, Common::Assignment *lhs);
    bool chk_this_template_SeqOf(Template *t, namedbool incomplete_allowed,
      namedbool implicit_omit, Common::Assignment *lhs);
    bool chk_this_template_SetOf(Template *t, namedbool incomplete_allowed,
      namedbool implicit_omit, Common::Assignment *lhs);
    bool chk_this_template_array(Template *t, namedbool incomplete_allowed,
      namedbool implicit_omit, Common::Assignment *lhs);
    void chk_this_template_Fat(Template *t);
    void chk_this_template_Signature(Template *t, namedbool incomplete_allowed);
  public:
    /** Check whether there is an enum item with the given name.
     *
     * @pre typetype is T_ENUM_T or T_ENUM_A */
    bool has_ei_withName(const Identifier& p_id) const;

    /** Return the enum item with the given name.
     *
     * @pre typetype is T_ENUM_T or T_ENUM_A */
    EnumItem *get_ei_byName(const Identifier& p_id) const;

    /** Return the enum item with the given index.
     *
     * @pre typetype is T_ENUM_T or T_ENUM_A */
    EnumItem *get_ei_byIndex(size_t i) const;

    /** Get the number of components.
     *
     * @return the number of components from the appropriate alternative
     * depending on the \a typetype.
     *
     * @pre \a typetype is one of T_CHOICE_T, T_SEQ_T, T_SET_T, T_OPENTYPE,
     * T_SEQ_A, T_SET_A, T_CHOICE_A,  T_ARRAY, T_SIGNATURE, T_ANYTYPE */
    size_t get_nof_comps();

    /** Get the name (id) of the component with the given index.
     *
     * @pre \a typetype is one of T_CHOICE_T, T_SEQ_T, T_SET_T, T_OPENTYPE,
     * T_SEQ_A, T_SET_A, T_CHOICE_A, T_SIGNATURE, T_ANYTYPE
     *
     * @note Not valid for T_ARRAY */
    const Identifier& get_comp_id_byIndex(size_t n);

    /** Get the component with the given index.
     *
     * @pre \a typetype is one of T_CHOICE_T, T_SEQ_T, T_SET_T, T_OPENTYPE,
     * T_SEQ_A, T_SET_A, T_CHOICE_A, T_ANYTYPE
     *
     * @note Not valid for T_ARRAY or T_SIGNATURE */
    CompField* get_comp_byIndex(size_t n);

    /** Get the index of the component with the given name
     *
     * @pre \a typetype of the last referenced type is one of
     * T_CHOICE_T, T_SEQ_T, T_SET_T, T_OPENTYPE,
     * T_SEQ_A, T_SET_A, T_CHOICE_A, T_ANYTYPE */
    size_t get_comp_index_byName(const Identifier& p_name);

    /** Get the index of the enum item with the given name
     *
     * @pre typetype is T_ENUM_T or T_ENUM_A */
    size_t get_eis_index_byName(const Identifier& p_name);

    const Int& get_enum_val_byId(const Identifier& p_name);

    size_t get_nof_root_comps();
    CompField* get_root_comp_byIndex(size_t n);

    /** Get the name (id) of the component with the given index.
     *
     * @pre \a typetype is one of T_CHOICE_T, T_SEQ_T, T_SET_T, T_OPENTYPE,
     * T_SEQ_A, T_SET_A, T_CHOICE_A, T_SIGNATURE, T_ANYTYPE
     *
     * @note Not valid for T_ARRAY */
    bool has_comp_withName(const Identifier& p_name);
    CompField* get_comp_byName(const Identifier& p_name);
    void add_comp(CompField *p_cf);

    /** Returns the embedded type of 'sequence/record of', 'set of' or array
     * types. */
    Type *get_ofType();

    OC_defn* get_my_oc();
    const Identifier& get_oc_fieldname();
    void set_my_tableconstraint(const TableConstraint *p_tc);
    const TableConstraint* get_my_tableconstraint();

    /** Returns the array dimension. Applicable only if typetype == T_ARRAY. */
    Ttcn::ArrayDimension *get_dimension() const;

    /** Returns the PortTypeBody if typetype == T_PORT */
    Ttcn::PortTypeBody *get_PortBody() const;

    /** Returns the ComponentTypeBody if typetype == T_COMPONENT */
    ComponentTypeBody *get_CompBody() const;

    /** Returns the parameters of a signature.
     * Applicable only if typetype == T_SIGNATURE. */
    SignatureParamList *get_signature_parameters() const;
    /** Returns the parameters of a signature.
     * Applicable only if typetype == T_SIGNATURE. */
    SignatureExceptions *get_signature_exceptions() const;
    /** Returns the return type of a signature.
     * Applicable only if typetype == T_SIGNATURE. */
    Type *get_signature_return_type() const;
    /** Returns whether the type is a non-blocking signature.
     * Applicable only if typetype == T_SIGNATURE. */
    bool is_nonblocking_signature() const;
    /** Returns the parameters of a functionreference
     * Applicable only if typetype == T_FUNCTION or T_ALTSTEP or T_TESTCASE */
    Ttcn::FormalParList *get_fat_parameters();
    /** Returns the return type of a functionreference
     * Applicable only if typetype == T_FUNCTION */
    Type *get_function_return_type();
    /** Returns the runs on type of a functionreference
     * Appliacable only if typetype == T_FUNCTION or T_ALTSTEP or T_TESTCASE */
    Type *get_fat_runs_on_type();
    /** Returns if a functionreference 'runs on self'
     * Appliacable only if typetype == T_FUNCTION or T_ALTSTEP or T_TESTCASE */
    bool get_fat_runs_on_self();
    /** Applicable only if typetype == T_FUNCTION */
    bool get_returns_template();

    /** Retruns true if it is a tagged type.*/
    bool is_tagged() const {return tags!=0;}
    void add_tag(Tag *p_tag);
    bool is_constrained() const {return constraints!=0;}
    void add_constraints(Constraints *p_constraints);
    Constraints* get_constraints() const {return constraints;}
    Reference * get_Reference();
  private:
    void chk_table_constraints();
  public:
    /** Returns true if the type has multiple alternative tags
      * (i.e. it is a non-tagged CHOICE type). */
    bool has_multiple_tags();
    /** Returns the tag of type. If this type is tagged, then the
     * outermost tag is returned. */
    Tag *get_tag();
    void get_tags(TagCollection& coll, map<Type*, void>& chain);
    /** Returns the smallest possible tag. If !has_multiple_tags()
     * then returns get_tag(). Used is SET CER encoding, see X.690
     * 9.3... */
    Tag *get_smallest_tag();
    /** Returns whether the type needs explicit tagging. */
    bool needs_explicit_tag();
    /** Removes the tag(s) from the type that was added during
     * automatic tagging transformation. Needed for the implementation
     * of COMPONENTS OF transformation. */
    void cut_auto_tags();
    Tags* build_tags_joined(Tags *p_tags=0);
    void set_with_attr(Ttcn::MultiWithAttrib* p_attrib);
    void set_parent_path(Ttcn::WithAttribPath* p_path);
    Ttcn::WithAttribPath* get_attrib_path() const;
    bool hasRawAttrs();
    bool hasNeedofRawAttrs();
    bool hasNeedofTextAttrs();
    bool hasNeedofJsonAttrs();
    bool hasNeedofXerAttrs();
    bool hasVariantAttrs();
    /** Returns whether the type has the encoding attribute specified by
      * the parameter (either in its own 'with' statement or in the module's) */
    bool hasEncodeAttr(const char* encoding_name);
    /** Returns whether \a this can be encoded according to rules
     * \a p_encoding.
     * @note Should be called only during code generation, after the entire
     * AST has been checked, or else the compiler might choke on code like:
     * type MyRecordOfType[-] ElementTypeAlias; */
    bool has_encoding(const MessageEncodingType_t encoding_type, const string* custom_encoding = NULL);
    /** Generates the C++ equivalent class(es) of the type. */
    void generate_code(output_struct *target);
    size_t get_codegen_index(size_t index);
  private:
     /** Generates code for the case when the C++ classes are implemented by
      * hand, includes the header file with name sourcefile and extension hh
      */
     void generate_code_include(const string& sourcefile, output_struct *target);
     /** Generates code for the embedded types the C++ classes of which must be
     * placed before the classes of this type (e.g. the types of mandatory
     * record/set fields). */
    void generate_code_embedded_before(output_struct *target);
    /** Generates code for the embedded types the C++ classes of which can be
     * placed after the classes of this type (e.g. the types of union fields
     * and optional record/set fields). */
    void generate_code_embedded_after(output_struct *target);
    void generate_code_typedescriptor(output_struct *target);
    void generate_code_berdescriptor(output_struct *target);
    void generate_code_rawdescriptor(output_struct *target);
    void generate_code_textdescriptor(output_struct *target);
    void generate_code_jsondescriptor(output_struct *target);
    void generate_code_xerdescriptor(output_struct *target);
    void generate_code_alias(output_struct *target);
    void generate_code_Enum(output_struct *target);
    void generate_code_Choice(output_struct *target);
    Opentype_t *generate_code_ot(stringpool& pool);
    void free_code_ot(Opentype_t* p_ot);
    void generate_code_Se(output_struct *target);
    void generate_code_SeOf(output_struct *target);
    void generate_code_Array(output_struct *target);
    void generate_code_Fat(output_struct *target);
    void generate_code_Signature(output_struct *target);
    /** Returns whether the type needs an explicit C++ typedef alias and/or
     * an alias to a type descriptor of another type. It returns true for those
     * types that are defined in module-level type definitions hence are
     * directly accessible by the users of C++ API (in test ports, external
     * functions). */
    bool needs_alias();
    /** Returns whether this is a pure referenced type that has no tags or
     * encoding attributes. */
    bool is_pure_refd();
    void generate_code_done(output_struct *target);

    /** Helper function used in generate_code_ispresentbound() for the
        ispresent() function in case of template parameter. Returns true
        if the referenced field which is embedded into a "?" is always present,
        otherwise returns false.  **/
    bool ispresent_anyvalue_embedded_field(Type* t,
      Ttcn::FieldOrArrayRefs *subrefs, size_t begin_index);
    
    /** Returns true if the C++ class for this type has already been pre-generated
      * or false if it still needs to be generated */
    bool is_pregenerated();
  public:
    /** Generates type specific call for the reference used in isbound call
     * into argument \a expr. Argument \a subrefs holds the reference path
     * that needs to be checked. Argument \a module is the actual module of
     * the reference and is used to gain access to temporal identifiers.
     * Argument \a global_id is the name of the bool variable where the result
     * of the isbound check is calculated. Argument \a external_id is the name
     * of the assignment where the call chain starts.
     * Argument \a is_template tells if the assignment is a template or not.
     * Argument \a isbound tells if the function is isbound or ispresent.
     */
    void generate_code_ispresentbound(expression_struct *expr,
      Ttcn::FieldOrArrayRefs *subrefs, Common::Module* module,
      const string& global_id, const string& external_id,
      const bool is_template, const bool isbound);

    /** Extension attribute for optimized code generation of structured types:
     *    with { extension "optimize:xxx" }
     * xxx tells what to optimize for (e.g.: memory, performance, etc.)
     * returns empty if none given
     */
    string get_optimize_attribute();
    /** Extension attribute for hand written TTCN-3 types:
     *    with { extension "sourcefile:xxx" }
     * filename is xxx, source files: xxx.hh and xxx.cc
     * returns empty if none given
     */
    string get_sourcefile_attribute();
    bool has_done_attribute();
    /** Generates the declaration and definition of a C++ value or template
     * object governed by \a this into \a cdef. Argument \a p_scope points to
     * the scope of the object to be generated. Argument \a name contains the
     * identifier of the target C++ object. If argument \a prefix is not NULL
     * the real object will be generated as static, its name will be
     * prefixed with \a prefix and a const (read-only) reference will be
     * exported with name \a name. This function shall be used when there is no
     * Value or Template object in the AST (e.g. in case of variables). */
    void generate_code_object(const_def *cdef, Scope* p_scope,
      const string& name, const char *prefix, bool is_template,
      bool has_err_descr);
    /** Generates the declaration and definition of a C++ value or template
     * object governed by \a this into \a cdef based on the attributes of
     * \a p_setting. */
    void generate_code_object(const_def *cdef, GovernedSimple *p_setting);
  private:
    virtual string create_stringRepr();
  public:
    /** If this type is added to an opentype, then this name is used
     *  as the name of the alternative. */
    Identifier get_otaltname(bool& is_strange);
    /** Returns the name of the C++ value class that represents \a this at
     * runtime. The class is either pre-defined (written manually in the Base
     * Library) or generated by the compiler. The reference is valid in the
     * module that \a p_scope belongs to. */
    string get_genname_value(Scope *p_scope);
    /** Returns the name of the C++ class that represents \a this template at
     * runtime. The class is either pre-defined (written manually in the Base
     * Library) or generated by the compiler. The reference is valid in the
     * module that \a p_scope belongs to. */
    string get_genname_template(Scope *p_scope);
    /** Returns a C++ identifier that can be used to distinguish the type in
     * the union that carries signature exceptions. */
    string get_genname_altname();
    /** User visible (not code generator) type name */
    string get_typename();
    /** User visible type name for built-in types */
    static const char * get_typename_builtin(typetype_t tt);
    string get_genname_typedescriptor(Scope *p_scope);
  private:
    /** Returns the name prefix of type descriptors, etc. that belong to the
     * equivalent C++ class referenced from the module of scope \a p_scope.
     * It differs from \a get_genname() only in case of special ASN.1 types
     * like RELATIVE-OID or various string types. */
    string get_genname_typename(Scope *p_scope);
    /** Return the name of the BER descriptor structure.
     * It is either the name of the type itself;
     * one of "ENUMERATED", "CHOICE", "SEQUENCE", "SET";
     * or one of the core classes e.g. ASN_ROID, UTF8String, etc.
     * The _ber_ suffix needs to be appended by the caller. */
    string get_genname_berdescriptor();
    string get_genname_rawdescriptor();
    string get_genname_textdescriptor();
    string get_genname_xerdescriptor();
    string get_genname_jsondescriptor();
    /** Return the ASN base type to be written into the type descriptor */
    const char* get_genname_typedescr_asnbasetype();
    /** parse subtype information and add parent's subtype information to
        create the effective/aggregate subtype of this type */
    void check_subtype_constraints();
  public:
    /** Return the type of subtype that is associated with this type */
    SubtypeConstraint::subtype_t get_subtype_type();
    virtual void dump(unsigned level) const;
    void set_parsed_restrictions(vector<SubTypeParse> *stp);
    /** Returns true if the values of this type can be used only in 'self'.
     * Some types (default, function reference with 'runs on self') are
     * invalid outside of the component they were created in, they should not
     * be sent/received in ports or used in compref.start(). All structured
     * types that may contain such internal types are also internal. */
    bool is_component_internal();
    /** Prints error messages for types that cannot leave the component.
     * Should be called only if is_component_internal() returned true.
     * \a type_chain is used to escape infinite recursion by maintaining a
     * chain of structured types that call this function recursively,
     * \a p_what tells what is the reason of the error. */
    void chk_component_internal(map<Type*,void>& type_chain,
      const char* p_what);
    /** Prints error messages for type that cannot be a type of given ones (parameter: array of typetype_t)
     * \a type_chain is used to escape infinite recursion by maintaining a
     * chain of structured types that call this function recursively,
     * \returns the original type or a not allowed one */
    typetype_t search_for_not_allowed_type(map<Type*, void>& type_chain,
                                           map<typetype_t, void>& not_allowed);
    /** Set the owner and its type type */
    void set_ownertype(TypeOwner_t ot, Node *o) { ownertype = ot; owner = o; }
    
    TypeOwner_t get_ownertype() const { return ownertype; }
    Node* get_owner() const { return owner; }

    bool is_untagged() const;
    
    inline bool is_pard_type_instance() { return pard_type_instance; }
    inline void set_pard_type_instance() { pard_type_instance = true; }
    
    inline void set_needs_any_from_done() { needs_any_from_done = true; }
    
    /** Calculates the type's display name from the genname (replaces double
      * underscore characters with single ones) */
    string get_dispname() const;
    
    /** Generates the JSON schema segment that would validate this type and
      * inserts it into the main schema.
      * @param json JSON tokenizer containing the main JSON schema
      * @param embedded true if the type is embedded in another type's definition;
      * false if it has its own definition
      * @param as_value true if this type is a field of a union with the "as value"
      * coding instruction */
    void generate_json_schema(JSON_Tokenizer& json, bool embedded, bool as_value);
    
    /** Generates the JSON schema segment that would validate a record of, set of
      * or array type and inserts it into the main schema. */
    void generate_json_schema_array(JSON_Tokenizer& json);
    
    /** Generates the JSON schema segment that would validate a record or set type 
      * and inserts it into the main schema. */
    void generate_json_schema_record(JSON_Tokenizer& json);
    
    /** Generates the JSON schema segment that would validate a union type or 
      * an anytype and inserts it into the main schema. */
    void generate_json_schema_union(JSON_Tokenizer& json);
    
    /** Generates a reference to this type's schema segment and inserts it into
      * the given schema. */
    void generate_json_schema_ref(JSON_Tokenizer& json);
    
    JsonAST* get_json_attributes() const { return jsonattrib; }
    
    /** Returns true if the type is a union with the JSON "as value" attribute, or
      * if a union with this attribute is embedded in the type. */
    bool has_as_value_union();
  };

  /** @} end of AST_Type group */

} // namespace Common

#endif // _Common_Type_HH
