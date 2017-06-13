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
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
/*
 * XerAttributes.cpp
 *
 *  Created on: Oct 17, 2008
 *      Author: ecsardu
 */

#include "XerAttributes.hh"
// The above line must be the first in this file
#include "../common/memory.h"

#include "Value.hh"

static const NamespaceRestriction empty_nsr = {
  0, 0, NamespaceRestriction::UNUSED
};

static const XerAttributes::NameChange nochange= { NamespaceSpecification::NO_MANGLING };

XerAttributes::XerAttributes()
: abstract_(false)
, attribute_(false)
, anyAttributes_(empty_nsr)
, anyElement_(empty_nsr)
, base64_(false)
, block_(false)
, decimal_(false)
, defaultForEmpty_(0)
, defaultForEmptyIsRef_(false)
, defaultForEmptyRef_(0)
, defaultValue_(0)
, element_(false)
, embedValues_(false)
, form_(UNSET)
, has_fractionDigits_(false)
, fractionDigits_(0)
, hex_(false)
, list_(false)
, name_(nochange)
, namespace_()
//, pi_or_comment_()
, num_text_(0)
, text_(0)
, untagged_(false)
, useNil_(false)
, useNumber_(false)
, useOrder_(false)
, useQName_(false)
, useType_(false)
, useUnion_(false)
, whitespace_(PRESERVE)
, xsd_type(XSD_NONE)
{
  //__asm("int3");
  //fprintf(stderr, "XER attributes(%p) new\n", (void*)this);
}

void FreeNamespaceRestriction(NamespaceRestriction& nsr)
{
  for (size_t i=0; i < nsr.nElements_; ++i) {
    Free(nsr.uris_[i]);
  }
  Free(nsr.uris_);
}

XerAttributes::~XerAttributes()
{
  FreeNamespaceRestriction(anyAttributes_);
  FreeNamespaceRestriction(anyElement_);

  Free(defaultForEmpty_);
  delete defaultValue_;
  delete defaultForEmptyRef_;

  FreeNameChange(name_);
  FreeNamespace(namespace_);

  //if (pi_or_comment_.position_ > AFTER_VALUE) {
  //  Free(pi_or_comment_.value_);
  //}

  for (size_t i=0; i < num_text_; ++i) {
    if ((unsigned long)text_[i].keyword
      > (unsigned long)NamespaceSpecification::LOWERCASED) {
      Free(text_[i].uri);
    }
    if (text_[i].prefix > (char*)NamespaceSpecification::ALL) {
      Free(text_[i].prefix);
    }
  }
  Free(text_);
}

void XerAttributes::FreeNamespace(NamespaceSpecification &ns) {
  switch (ns.keyword) {
  case NamespaceSpecification::NO_MANGLING:
  case NamespaceSpecification::CAPITALIZED:
  case NamespaceSpecification::UNCAPITALIZED:
  case NamespaceSpecification::UPPERCASED:
  case NamespaceSpecification::LOWERCASED:
    break; // nothing to do
  default: // real string, must be freed
    Free(ns.uri);
    break;
  }
  ns.uri = 0;
  Free(ns.prefix);
  ns.prefix = 0;
}

void XerAttributes::FreeNameChange(XerAttributes::NameChange& n) {
  switch (n.kw_) {
  case NamespaceSpecification::NO_MANGLING:
  case NamespaceSpecification::CAPITALIZED:
  case NamespaceSpecification::UNCAPITALIZED:
  case NamespaceSpecification::UPPERCASED:
  case NamespaceSpecification::LOWERCASED:
    break; // nothing to do
  default: // real string, must be freed
    Free(n.nn_);
    break;
  }
  n.kw_ = NamespaceSpecification::NO_MANGLING;
}


void XerAttributes::print(const char *type_name) const {
  fprintf(stderr, "XER attributes(%p) for %s:\n", (const void*)this, type_name);
  if (empty()) fputs("...Empty...\n", stderr);
  else {
    fputs(abstract_ ? "ABSTRACT\n" : "", stderr);
    fputs(attribute_ ? "ATTRIBUTE\n" : "", stderr);

    if (has_aa(this)) {
      if (anyAttributes_.type_ == NamespaceRestriction::NOTHING) {
        fputs("ANY-ATTRIBUTES\n", stderr);
      }
      else for (size_t i = 0; i < anyAttributes_.nElements_; ++i) {
        fprintf(stderr, "ANY-ATTRIBUTES %s %s\n",
          anyAttributes_.type_ == NamespaceRestriction::FROM ? "EXCEPT" : "FROM",
            (anyAttributes_.uris_[i] && *anyAttributes_.uris_[i]) ?
              anyAttributes_.uris_[i] : "ABSENT");
      }
    }

    if (has_ae(this)) {
      if (anyElement_.type_ == NamespaceRestriction::NOTHING) {
        fputs("ANY-ELEMENT\n", stderr);
      }
      else for (size_t i = 0; i < anyElement_.nElements_; ++i) {
        fprintf(stderr, "ANY-ELEMENT %s %s\n",
          anyElement_.type_ == NamespaceRestriction::FROM ? "EXCEPT" : "FROM",
            (anyElement_.uris_[i] && *anyElement_.uris_[i]) ?
              anyElement_.uris_[i] : "ABSENT");
      }
    }
    fputs(base64_ ? "BASE64\n" : "", stderr);
    fputs(block_ ? "BLOCK\n" : "", stderr);
    fputs(decimal_ ? "DECIMAL\n" : "", stderr);

    if (defaultForEmpty_)
      fprintf(stderr, "DEFAULT-FOR-EMPTY '%s' %s\n",
        defaultForEmptyIsRef_ ? defaultForEmptyRef_->get_dispname().c_str() : defaultForEmpty_,
        defaultForEmptyIsRef_ ? "(reference) " : "");

    if (element_) fputs("ELEMENT\n", stderr);
    fputs(embedValues_ ? "EMBED-VALUES\n" : "", stderr);
    fputs((form_ & QUALIFIED) ? "FORM AS QUALIFIED\n" : "", stderr);
    if (has_fractionDigits_) fprintf(stderr, "FRACTIONDIGITS '%i'\n", fractionDigits_);
    fputs(hex_ ? "hexBinary" : "", stderr);
    fputs(list_ ? "LIST\n" : "", stderr);

    static const char * xforms[] = {
      "CAPITALIZED", "UNCAPITALIZED", "UPPERCASED", "LOWERCASED"
    };
    switch (name_.kw_) {
    case NamespaceSpecification::NO_MANGLING: // nothing to do
      break;
    default: // a string
      fprintf(stderr, "NAME AS '%s'\n", name_.nn_);
      break;
    case NamespaceSpecification::CAPITALIZED:
    case NamespaceSpecification::UNCAPITALIZED:
    case NamespaceSpecification::LOWERCASED:
    case NamespaceSpecification::UPPERCASED:
      fprintf(stderr, "NAME AS %s\n",
        xforms[name_.kw_ - NamespaceSpecification::CAPITALIZED]);
      break;
    }

    if (namespace_.uri) {
      fprintf(stderr, "NAMESPACE '%s' %s %s\n", namespace_.uri,
        (namespace_.prefix ? "PREFIX" : ""),
        (namespace_.prefix ? namespace_.prefix : ""));
    }

    //if (pi_or_comment_.position_ != NOWHERE) {
    //  fputs("PI-OR-COMMENT\n", stderr);
    //}
    if (num_text_) {
      fputs("TEXT\n", stderr);
      for (size_t t=0; t < num_text_; ++t) {
        const char* who = 0, *action = 0;
        switch ((unsigned long)(text_[t].uri) ) {
        case NamespaceSpecification::LOWERCASED:
          action = "LOWERCASED"; break;
        case NamespaceSpecification::UPPERCASED:
          action = "UPPERCASED"; break;
        case NamespaceSpecification::CAPITALIZED:
          action = "CAPITALIZED"; break;
        case NamespaceSpecification::UNCAPITALIZED:
          action = "UNCAPITALIZED"; break;
        case 0:
          action = "text"; break;
        default:
          action = text_[t].uri; break;
        }

        switch ((unsigned long)text_[t].prefix) {
        case 0: who = ""; break;
        case NamespaceSpecification::ALL: who = "ALL"; break;
        default: who = text_[t].prefix; break;
        }
        fprintf(stderr, "   %s as %s\n", who, action);
      }
    }
    fputs(untagged_ ? "UNTAGGED\n" : "", stderr);
    fputs(useNil_ ? "USE-NIL\n" : "", stderr);
    fputs(useNumber_ ? "USE-NUMBER\n" : "", stderr);
    fputs(useOrder_ ? "USE-ORDER\n" : "", stderr);
    fputs(useQName_ ? "USE-QNAME\n" : "", stderr);
    fputs(useType_ ? "USE-TYPE\n" : "", stderr);
    fputs(useUnion_ ? "USE-UNION\n" : "", stderr);
    if (whitespace_ != PRESERVE) fprintf(stderr, "WHITESPACE %s\n",
      whitespace_ == COLLAPSE ? "COLLAPSE" : "REPLACE");
    fputs(". . . . .\n", stderr);
  }
}

XerAttributes& XerAttributes::operator |= (const XerAttributes& other)
{
  if (other.empty()) FATAL_ERROR("XerAttributes::operator |=");
/*
fprintf(stderr, "@@@ replacing:\n");
print("orig.");
other.print("other");
*/
  abstract_ |= other.abstract_;
  attribute_ |= other.attribute_;
  if (has_aa(&other)) {
    FreeNamespaceRestriction(anyAttributes_);
    anyAttributes_.nElements_ = other.anyAttributes_.nElements_;
    anyAttributes_.type_      = other.anyAttributes_.type_;
    anyAttributes_.uris_      = (char**)Malloc(anyAttributes_.nElements_
      * sizeof(char*));
    for (size_t i=0; i < anyAttributes_.nElements_; ++i) {
      anyAttributes_.uris_[i] = mcopystr(other.anyAttributes_.uris_[i]);
    }
  }
  if (has_ae(&other)) {
    FreeNamespaceRestriction(anyElement_);
    anyElement_.nElements_ = other.anyElement_.nElements_;
    anyElement_.type_      = other.anyElement_.type_;
    anyElement_.uris_      = (char**)Malloc(anyElement_.nElements_
      * sizeof(char*));
    for (size_t i=0; i < anyElement_.nElements_; ++i) {
      anyElement_.uris_[i] = mcopystr(other.anyElement_.uris_[i]);
    }
  }
  base64_ |= other.base64_;
  block_ |= other.block_;
  decimal_ |= other.decimal_;

  if (other.defaultForEmpty_ != 0) {
    Free(defaultForEmpty_);
    defaultForEmpty_ = mcopystr(other.defaultForEmpty_);
    defaultForEmptyIsRef_ = other.defaultForEmptyIsRef_;
  }
  
  defaultForEmptyIsRef_ |= other.defaultForEmptyIsRef_;
  if (other.defaultForEmptyRef_ != 0) {
    Free(defaultForEmptyRef_);
    defaultForEmptyRef_ = other.defaultForEmptyRef_->clone();
  }

  element_ |= other.element_;
  embedValues_ |= other.embedValues_;
  form_ = other.form_;
  if (other.has_fractionDigits_) {
    has_fractionDigits_ = other.has_fractionDigits_;
    fractionDigits_ = other.fractionDigits_;
  }
  hex_  |= other.hex_;
  list_ |= other.list_;
  if (other.name_.kw_ != NamespaceSpecification::NO_MANGLING) {
    FreeNameChange(name_);
    switch (other.name_.kw_) {
    case NamespaceSpecification::NO_MANGLING:
      break; // not possible inside the if
    case NamespaceSpecification::CAPITALIZED:
    case NamespaceSpecification::UNCAPITALIZED:
    case NamespaceSpecification::UPPERCASED:
    case NamespaceSpecification::LOWERCASED:
      name_.kw_ = other.name_.kw_;
      break;
    default: // a real string
      name_.nn_ = mcopystr(other.name_.nn_);
      break;
    }
  }

  if (other.namespace_.uri != 0) {
    switch (namespace_.keyword) {
    case NamespaceSpecification::NO_MANGLING:
    case NamespaceSpecification::CAPITALIZED:
    case NamespaceSpecification::UNCAPITALIZED:
    case NamespaceSpecification::UPPERCASED:
    case NamespaceSpecification::LOWERCASED:
      break; // nothing to do
    default: // real string, must be freed
      Free(namespace_.uri);
      break;
    }
    switch (other.namespace_.keyword) {
    case NamespaceSpecification::NO_MANGLING:
    case NamespaceSpecification::CAPITALIZED:
    case NamespaceSpecification::UNCAPITALIZED:
    case NamespaceSpecification::UPPERCASED:
    case NamespaceSpecification::LOWERCASED:
      namespace_.uri = other.namespace_.uri;
      break;
    default: // real string
      namespace_.uri = mcopystr(other.namespace_.uri);
      break;
    }
    Free(namespace_.prefix);
    namespace_.prefix = mcopystr(other.namespace_.prefix);
  }
  //pi_or_comment_;

  if (other.num_text_) {
    // Append the other TEXT. No attempt is made to eliminate duplicates.
    // This will be done in Type::chk_xer_text().
    size_t old_num = num_text_;
    num_text_ += other.num_text_;
    text_ = (NamespaceSpecification *)Realloc(
      text_, num_text_ * sizeof(NamespaceSpecification));
    for (size_t t = 0; t < other.num_text_; ++t) {
      switch ((unsigned long)(other.text_[t].uri) ) {
      case NamespaceSpecification::LOWERCASED:
      case NamespaceSpecification::UPPERCASED:
      case NamespaceSpecification::CAPITALIZED:
      case NamespaceSpecification::UNCAPITALIZED:
      case NamespaceSpecification::NO_MANGLING:
        text_[old_num + t].uri = other.text_[t].uri;
        break;
      default:
        text_[old_num + t].uri = mcopystr(other.text_[t].uri);
        break;
      }

      switch ((unsigned long)other.text_[t].prefix) {
      case 0: case NamespaceSpecification::ALL:
        text_[old_num + t].prefix = other.text_[t].prefix;
        break;
      default:
        text_[old_num + t].prefix = mcopystr(other.text_[t].prefix);
        break;
      }
    }
  }
  untagged_ |= other.untagged_;
  useNil_ |= other.useNil_;
  useNumber_ |= other.useNumber_;
  useOrder_ |= other.useOrder_;
  useQName_ |= other.useQName_;
  useType_ |= other.useType_;
  useUnion_ |= other.useUnion_;
  whitespace_ = other.whitespace_;

  if (other.xsd_type != XSD_NONE) {
    xsd_type = other.xsd_type;
  }
  return *this;
}

bool XerAttributes::empty() const
{
  return !abstract_
  && !attribute_
  && !has_aa(this)
  && !has_ae(this)
  && !base64_
  && !block_
  && !decimal_
  && defaultForEmpty_ == 0
  && !defaultForEmptyIsRef_
  && !defaultForEmptyRef_
  && !element_
  && !embedValues_
  && !(form_ & LOCALLY_SET)
  && !has_fractionDigits_
  && !hex_
  && !list_
  && name_.kw_ == NamespaceSpecification::NO_MANGLING
  && namespace_.uri == 0
  && num_text_ == 0
  && !untagged_
  && !useNil_
  && !useNumber_
  && !useOrder_
  && !useQName_
  && !useType_
  && !useUnion_
  && whitespace_ == PRESERVE
  && xsd_type == XSD_NONE;
}

