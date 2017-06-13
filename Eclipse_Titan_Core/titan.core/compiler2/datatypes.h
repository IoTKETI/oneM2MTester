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
 *   Delic, Adam
 *   Forstner, Matyas
 *   Kremer, Peter
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Szabo, Bence Janos
 *   Szalai, Gabor
 *
 ******************************************************************************/
#ifndef DATATYPES_H
#define DATATYPES_H

#include "asn1/asn1p_old.h"
#include "ttcn3/rawASTspec.h"
#include "XSD_Types.hh"
/* Common types */

typedef int boolean;
#define TRUE 1
#define FALSE 0

/* C structures for representing TTCN-3 and ASN.1 types */

typedef enum { RECORD, SET, UNION, ANYTYPE } struct_def_kind_enum;

typedef enum { RECORD_OF, SET_OF } struct_of_def_kind_enum;

typedef enum {
  ANY_ATTRIB_BIT = 1,
  ANY_ELEM_BIT   = 2,
  ANY_FROM_BIT   = 4,
  ANY_EXCEPT_BIT = 8
} any_kind;
/* bit fields, use one of {ANY_ATTRIB_BIT,ANY_ELEM_BIT} and
 * one of {ANY_FROM_BIT, ANY_EXCEPT_BIT} */

/** Flags that determine the type(s) of value an object or field will be encoded
  * as in JSON code */
typedef enum {
  JSON_NONE       = 0x00, // no value type set (default)
  JSON_NUMBER     = 0x01, // integer and float
  JSON_STRING     = 0x02, // all string types, the objid type, the verdict type and enumerated values
  JSON_BOOLEAN    = 0x04, // boolean (true or false)
  JSON_OBJECT     = 0x08, // records, sets, unions and the anytype
  JSON_ARRAY      = 0x10, // record of, set of and array
  JSON_NULL       = 0x20, // ASN.1 null type
  JSON_ANY_VALUE  = 0x3F  // unions with the "as value" coding instruction
} json_value_t;

/* Compound type definitions */

/* record, set, union */

/** Structure field descriptor for code generation */
typedef struct {
  const char *type; /**< The C++ type name of the field */
  const char *typegen; /**< XER descriptor name */
  const char *typedescrname; /**< The name of the TTCN_descriptor variable */
  const char *name; /**< The C++ name of the field (without the prefix) */
  const char *dispname; /**< Display name (user-visible name) */
  boolean isOptional;
  boolean isDefault; /**< does it have a default value */
  boolean of_type; /**< true if the field is a sequence-of or set-of */
  boolean hasRaw;
  raw_attrib_struct raw;
  const char *defvalname; /**< the constant containing the default value */
  size_t   xerAnyNum;
  char **  xerAnyUris;
  /** Conflated field for anyAttributes and anyElement. Use one of
   * {ANY_ATTRIB_BIT,ANY_ELEM_BIT} + one of {ANY_FROM_BIT, ANY_EXCEPT_BIT} */
  unsigned short xerAnyKind;
  unsigned short jsonValueType;
  boolean xerAttribute;
  boolean jsonOmitAsNull;
  boolean jsonMetainfoUnbound;
  const char* jsonAlias;
  const char* jsonDefaultValue;
  /** true if the field is a record-of or set-of with optimized memory allocation */
  boolean optimizedMemAlloc;
  XSD_types xsd_type;
  boolean xerUseUnion;
} struct_field;

/** Structure (record, set, union, anytype) descriptor for code generation */
typedef struct {
  const char *name; /**< C++ name for code generation */
  const char *dispname; /**< Display name (user-visible) */
  struct_def_kind_enum kind; /**< is it a record or a set */
  boolean isASN1; /**< Originating from an ASN.1 module */
  boolean hasRaw;
  boolean hasText;
  boolean hasXer;
  boolean hasJson;
  boolean xerUntagged;
  boolean xerUntaggedOne; /**< from Type::u.secho.has_single_charenc */
  boolean xerUseNilPossible; /* for sequence */
  boolean xerUseOrderPossible; /* for sequence */
  boolean xerUseQName; /* for sequence */
  boolean xerUseTypeAttr; /* for choice */
  boolean xerUseUnion; /* for choice */
  boolean xerHasNamespaces; /* from the module */
  boolean xerEmbedValuesPossible; /* for sequence */
  boolean jsonAsValue; /* for both */
  /** The index of the last field which can generate empty XML, or -1 */
  int exerMaybeEmptyIndex; /* for union */
  const char * control_ns_prefix;
  raw_attrib_struct raw;
  size_t nElements; /**< Number of fields for this class */
  size_t totalElements; /**< Real number of elements; may include
  fields from the last component when USE-NIL and USE-ORDER are both set */
  struct_field *elements;
  boolean has_opentypes;
  boolean opentype_outermost;
  Opentype_t *ot;
  boolean isOptional; /**< this structure is an optional field in a record/set */
} struct_def;

/** record of, set of descriptor for code generation */
typedef struct {
  const char *name; /**< C++ name for code generation */
  const char *dispname; /**< Display name (user-visible) */
  struct_of_def_kind_enum kind; /**< is it a record-of or a set-of */
  boolean isASN1;
  boolean hasRaw;
  boolean hasText;
  boolean hasXer;
  boolean hasJson;
  /** true if this is a record-of BOOLEAN, ENUMERATED or NULL */
  boolean xmlValueList;
  /* * true if this record-of has the LIST encoding instruction */
  /*boolean xerList;*/
  /** true if this record-of has the ATTRIBUTE encoding instruction */
  boolean xerAttribute;
  /** true if this record-of has the ANY-ATTRIBUTE or ANY-ELEMENT encoding instruction */
  boolean xerAnyAttrElem;
  raw_attrib_struct raw;
  boolean has_opentypes;
  const char *type; /**< Type of the elements */
  const char *oftypedescrname; /**< Type descr. variable of the elements */
  size_t nFollowers; /**< number of optional fields following the record-of */
  struct_field *followers; /**< information about following optional siblings */
} struct_of_def;

/* for processing enumerated type definitions */

typedef struct {
    const char *name; /* identifier name */
    const char *dispname; /* identifier TTCN-3 name */
    const char *text; /* modified by TEXT */
    int value;
} enum_field;

typedef struct {
    const char *name;
    const char *dispname; /* fullname */
    boolean isASN1;
    boolean hasRaw;
    boolean hasText;
    boolean hasXer;
    boolean hasJson;
    boolean xerUseNumber;
    boolean xerText; /* A component has the TEXT encoding instruction */
    size_t nElements;
    enum_field *elements;
    int firstUnused, secondUnused;
} enum_def;

/* for function, altstep, testcase reference types */

typedef enum { FUNCTION, ALTSTEP, TESTCASE } fat_type;

typedef struct {
  const char *name;
  const char *dispname;
  char *return_type;
  fat_type type;
  boolean runs_on_self;
  boolean is_startable;
  char *formal_par_list;
  char *actual_par_list;
  size_t nElements;
  const char** parameters;
} funcref_def;

/** for template restrictions */
typedef enum {
  TR_NONE, /* no restriction was given */
  TR_OMIT,
  TR_VALUE,
  TR_PRESENT
} template_restriction_t;

#endif /* DATATYPES_H */
