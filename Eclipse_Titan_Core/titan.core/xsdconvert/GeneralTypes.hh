/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Godar, Marton
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
#ifndef GENERAL_TYPES_H_
#define GENERAL_TYPES_H_

#include "Mstring.hh"

enum ConstructType {
  c_unknown,
  c_schema,
  c_simpleType,
  c_complexType,
  c_element,
  c_attribute,
  c_attributeGroup,
  c_group,
  c_annotation,
  c_include,
  c_import,
  c_idattrib
};

enum NameConversionMode {
  nameMode,
  typeMode,
  fieldMode
};

enum UseValue {
  optional,
  prohibited,
  required
};

enum FormValue {
  notset,
  qualified,
  unqualified
};

enum BlockValue {
  not_set,
  all,
  substitution,
  restriction,
  extension
};

enum TagName {
  // XSD Elements:
  n_all,
  n_annotation,
  n_any,
  n_anyAttribute,
  n_appinfo,
  n_attribute,
  n_attributeGroup,
  n_choice,
  n_complexContent,
  n_complexType,
  n_documentation,
  n_element,
  n_extension,
  n_field, // Not supported by now
  n_group,
  n_import,
  n_include,
  n_key, // Not supported by now
  n_keyref, // Not supported by now
  n_list,
  n_notation, // Not supported by now
  n_redefine,
  n_restriction,
  n_schema,
  n_selector, // Not supported by now
  n_sequence,
  n_simpleContent,
  n_simpleType,
  n_union,
  n_unique, // Not supported by now

  // XSD Restrictions / Facets for Datatypes:
  n_enumeration,
  n_fractionDigits, // Not supported by now
  n_length,
  n_maxExclusive,
  n_maxInclusive,
  n_maxLength,
  n_minExclusive,
  n_minInclusive,
  n_minLength,
  n_pattern,
  n_totalDigits,
  n_whiteSpace,

  // Others - non-standard, but used:
  n_label, // ???
  n_definition, // ???

  n_NOTSET
};

/** This type just stores the textual information about an XML namespace */
class NamespaceType {
public:
  Mstring prefix;
  Mstring uri;

  NamespaceType()
  : prefix(), uri() {
  }

  NamespaceType(const Mstring& p, const Mstring& u)
  : prefix(p), uri(u) {
  }

  bool operator<(const NamespaceType & rhs) const {
    return uri < rhs.uri;
  }

  bool operator==(const NamespaceType & rhs) const {
    return (uri == rhs.uri) && (prefix == rhs.prefix);
  }
};

class QualifiedName {
public:
  Mstring nsuri;
  Mstring name;
  Mstring orig_name;
  bool dup;

  QualifiedName()
  : nsuri(), name(), orig_name(), dup(false) {
  }

  QualifiedName(const Mstring& ns, const Mstring nm)
  : nsuri(ns), name(nm), orig_name(nm), dup(false) {
  }

  QualifiedName(const Mstring& ns, const Mstring nm, const Mstring orig)
  : nsuri(ns), name(nm), orig_name(orig), dup(false) {
  }

  bool operator<(const QualifiedName& rhs) const {
    return name < rhs.name;
  }

  bool operator==(const QualifiedName& rhs) const {
    return (nsuri == rhs.nsuri) && (name == rhs.name);
  }
  
  bool operator!=(const QualifiedName& rhs) const {
    return (nsuri != rhs.nsuri) || (name != rhs.name);
  }
};

enum wanted {
  want_CT, want_ST, want_BOTH
};

static const char XMLSchema[] = "http://www.w3.org/2001/XMLSchema";

#endif /*GENERAL_TYPES_H_*/
