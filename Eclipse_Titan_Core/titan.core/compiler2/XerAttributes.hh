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
 *   Raduly, Csaba
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
/*
 * XerAttributes.h
 *
 *  Created on: Oct 17, 2008
 *      Author: ecsardu
 */

#ifndef XERATTRIBUTES_H_
#define XERATTRIBUTES_H_

#include <stddef.h>
#include <limits.h>
#include "../common/memory.h"
#include "datatypes.h"
#include "../compiler2/Setting.hh"

namespace Common {
class Value;
}

struct NamespaceRestriction // union member, no constructor allowed
{
  size_t nElements_;
  /** URIs for ANY-ATTRIBUTES and ANY-ELEMENT. NULL is used to represent
   * ABSENT  */
  char ** uris_;
  enum {
    UNUSED,
    NOTHING, ///< anyElement
    FROM,    ///< anyElement from ...
    EXCEPT   ///< anyElement except ...
  } type_;
};
// #define ABSENT ((char*)5) just use NULL

void FreeNamespaceRestriction(NamespaceRestriction& nsr);


struct NamespaceSpecification
{
  /** How to transform the ASN/TTCN name to XML name */
  enum NameMangling {
    NO_MANGLING,  ///< no change
    CAPITALIZED,  ///< change the first letter to uppercase
    UNCAPITALIZED,///< change the first letter to lowercase
    UPPERCASED,   ///< change all letters to uppercase
    LOWERCASED,   ///< change all letters to lowercase
    // The underlying type of the enum is an integral type that can cover
    // all the enum values. Using a value which doesn't fit into signed int,
    // we try to force the underlying type to be _unsigned_ int.
    FORCE_UNSIGNED = UINT_MAX
  };
  enum { ALL = LOWERCASED+1 }; // should be one bigger than the last of NameMangling

  /** The URI for the NAMESPACE encoding instruction, or TextToBeUsed for TEXT.
   *  May be a valid pointer or any of the NameMangling enum values,
   *  or NULL for "text".
   *  For NAMESPACE, NULL (NO_MANGLING) means that no NAMESPACE instruction
   *  is assigned directly; UNCAPITALIZED means that an explicit "cancel
   *  all namespaces" instruction was applied: "namespace '' prefix ''" */
  union {
  char * uri;      // for NAMESPACE
  char * new_text; // for TEXT
  NameMangling keyword; // for TEXT
  };
  /** The prefix for the NAME encoding instruction or the Target for TEXT.
   * May be XerAttributes::ALL for "text all as ...cased", or 0 for "text" */
  union {
  char * prefix; // for NAMESPACE
  char * target; // for TEXT
  };
};

inline void free_name_or_kw(char *s)
{
  if (s > (char*)NamespaceSpecification::ALL) {
    Free(s);
  }
}

/** XER attributes during compilation.
 *
 * @todo members are ordered alphabetically, this should be changed to
 * be more space efficient (pointers, then integers, then booleans)
 */
class XerAttributes {
private:
  // Compiler-generated copy constructor and assignment NOT safe.
  // Must disable or implement own.
  XerAttributes(const XerAttributes&);
  XerAttributes& operator=(const XerAttributes&);
public:
  //enum PI_Location { NOWHERE, BEFORE_TAG, BEFORE_VALUE, AFTER_TAG, AFTER_VALUE };
  enum WhitespaceAction { PRESERVE, REPLACE, COLLAPSE };
  enum Form { // a bit mask
    UNSET = 0,
    // Global defaults for the module:
    ELEMENT_DEFAULT_QUALIFIED = 1,
    ATTRIBUTE_DEFAULT_QUALIFIED = 2,
    // Locally set values (applied directly to the type or field)
    UNQUALIFIED = 4, //< "form as unqualified"
    QUALIFIED = 8,   //< "form as qualified"
    LOCALLY_SET = (QUALIFIED | UNQUALIFIED)
  };

  /** Name change action.
   *
   * When importing XSD into ASN.1 or TTCN-3, some names might need to be
   * changed to protect the guilty (make them valid ASN.1/TTCN.3).
   * The NAME encoding instruction describes how to get back to the XML
   * tag name. The original name may be retrieved with one of the "canned" actions
   * (change the case of the first or all letters) or by storing the original
   * name in full (usually if characters were appended to remove name clashes).
   *
   * This union and the related code makes the assumption that not only
   * 0, but 1,2,3,4 will not appear as valid pointers. Switch statements
   * operating on the kw_ member must have cases for the enumeration values,
   * the default case means that the nn_ member is in fact active
   * and contains a pointer to the string. Remember to free/alloc new space
   * in this situation. */
  typedef union {
    NamespaceSpecification::NameMangling kw_;
    char* nn_;
  } NameChange;

  XerAttributes();
  ~XerAttributes();

  /// If the NameChange structure contains a string, free it.
  static void FreeNameChange(NameChange& n);
  /// If the NamespaceSpecification contains a string, free it.
  static void FreeNamespace(NamespaceSpecification& ns);
public:
  bool abstract_;
  bool attribute_;
  NamespaceRestriction anyAttributes_;
  NamespaceRestriction anyElement_;
  /// Base64 encoding for string-like types (XSD:base64Binary)
  bool base64_;
  bool block_;
  /// No scientific notation for float
  bool decimal_;
  /// String parsed out from the encoding attribute
  char * defaultForEmpty_;
  /// True if the defaultForEmpty variant's value is a reference.
  bool defaultForEmptyIsRef_;
  /// The reference to the defaultForEmpty's value.
  Common::Reference* defaultForEmptyRef_;
  /// Value object constructed by Type::chk_xer() from defaultForEmpty_
  Common::Value *defaultValue_;
  /// Global element in XSD
  bool element_;
  /// Give values to text nodes. Applied to record containing a record of string
  bool embedValues_;
  /// Qualified or unqualified form for local elements and attributes
  unsigned short form_;
  // True if there was fractionDigits variant
  bool has_fractionDigits_;
  // fractionDigits on float number
  int fractionDigits_;
  /// XSD:hexBinary
  bool hex_;
  /// space-separated values for record-of/set-of
  bool list_;
  /// How to get back the XML name
  NameChange name_;
  NamespaceSpecification namespace_;
  //struct { PI_Location position_; /*const*/ char * value_; } pi_or_comment_;
  /// Number of TEXT instructions stored.
  size_t num_text_;
  /// Pointer to an array for the TEXT encoding instruction
  NamespaceSpecification *text_; // re-use of struct
  /// No XML tag, just the value
  bool untagged_;
  bool useNil_;
  bool useNumber_;
  bool useOrder_;
  bool useQName_;
  bool useType_;
  bool useUnion_;
  WhitespaceAction whitespace_;
  XSD_types xsd_type;

  void print(const char *type_name) const;

  /** Override/merge XER attributes from @p other.
   *
   * Boolean attributes (attribute_, base64_, decimal_, embedValues_,
   * list_, untagged_, useNil_, useNumber_, useOrder_, useQName_, useType_,
   * useUnion_) are merged.
   * In case of "Value" attributes (anyAttributes_, anyElement_, name_,
   * namespace_, text_, whitespace_) the value in @p other overwrites
   * the value in @p this.
   *
   * @param other set of XER attributes
   * @return the merged object (@p *this)
   * @pre @a other must not be @t empty() */
  XerAttributes& operator |= (const XerAttributes& other);

  /** Return true if no attribute is set, false otherwise */
  bool empty() const;
};

inline bool has_ae(const XerAttributes *xa) {
  return xa->anyElement_.type_ != NamespaceRestriction::UNUSED;
}

inline bool has_aa(const XerAttributes *xa) {
  return xa->anyAttributes_.type_ != NamespaceRestriction::UNUSED;
}


#endif /* XERATTRIBUTES_H_ */
