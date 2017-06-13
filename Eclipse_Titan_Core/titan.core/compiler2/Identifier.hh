/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Forstner, Matyas
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef _Common_Identifier_HH
#define _Common_Identifier_HH

#include "string.hh"
#include "map.hh"

namespace Common {

  /**
   * \addtogroup AST
   *
   * @{
   */

  /**
   * This is a universal identifier class which can handle all
   * reserved keywords. It contains also the name mappings. It is
   * effective because of using reference counter.
   */
  class Identifier {
  public:
    enum id_t {
      ID_NAME, /**< internal name == C++ (generated) name */
      ID_ASN,  /**< ASN.1 display name */
      ID_TTCN  /**< TTCN-3 display name */
    };
    struct id_data_t;
    /** Returns whether \a p_name is a reserved word in the language
     * identified by \a p_id_t */
    static bool is_reserved_word(const string& p_name, id_t p_id_t);
    /** Converts ASN.1 name \a p_name to internal (C++) name */
    static string asn_2_name(const string& p_name);
    /** Converts internal (C++) name \a p_name to ASN.1 name */
    static string name_2_asn(const string& p_name);
    /** Converts TTCN-3 name \a p_from to internal (C++) name */
    static string ttcn_2_name(const string& p_name);
    /** Converts internal (C++) name \a p_from to TTCN-3 name */
    static string name_2_ttcn(const string& p_name);
  private:
    id_data_t *id_data;
    id_t origin;
  public:
    /** Creates a new identifier with origin \a p_id_t and name \a
     *  p_name. If \a dontreg is true, then this new identifier will
     *  not be registered in the mapping tables. */
    Identifier(id_t p_id_t, const string& p_name, bool dontreg=false);
    Identifier(const Identifier& p);
    ~Identifier();

    Identifier *clone() const { return new Identifier(*this); }
    Identifier& operator=(const Identifier& p);
    bool operator==(const Identifier& p) const;
    inline bool operator!=(const Identifier& p) const
      { return !(*this == p); }
    bool operator<(const Identifier& p) const;

    /** Gets the origin. */
    id_t get_origin() const { return origin; }
    /** Gets the internal (and C++) name. */
    const string& get_name() const;
    /** Gets the display name according to its origin. */
    const string& get_dispname() const;
    /** Gets the ASN display name. */
    const string& get_asnname() const;
    /** Gets the TTCN display name. */
    const string& get_ttcnname() const;
    /** Returns false if this identifier does not have a valid name
     *  for the requested purpose. */
    bool get_has_valid(id_t p_id_t) const;
    /** Gets each valid/set name. */
    string get_names() const;

    /** True if it is a valid ASN modulereference. */
    bool isvalid_asn_modref() const;
    /** True if it is a valid ASN typereference. */
    bool isvalid_asn_typeref() const;
    /** True if it is a valid ASN valuereference. */
    bool isvalid_asn_valref() const;
    /** True if it is a valid ASN valuesetreference. */
    bool isvalid_asn_valsetref() const;
    /** True if it is a valid ASN objectclassreference. */
    bool isvalid_asn_objclassref() const;
    /** True if it is a valid ASN objectreference. */
    bool isvalid_asn_objref() const;
    /** True if it is a valid ASN objectsetreference. */
    bool isvalid_asn_objsetref() const;
    /** True if it is a valid ASN typefieldreference. */
    bool isvalid_asn_typefieldref() const;
    /** True if it is a valid ASN valuefieldreference. */
    bool isvalid_asn_valfieldref() const;
    /** True if it is a valid ASN valuesetfieldreference. */
    bool isvalid_asn_valsetfieldref() const;
    /** True if it is a valid ASN objectfieldreference. */
    bool isvalid_asn_objfieldref() const;
    /** True if it is a valid ASN objectsetfieldreference. */
    bool isvalid_asn_objsetfieldref() const;
    /** True if it is a valid ASN "word". */
    bool isvalid_asn_word() const;

    void dump(unsigned level) const;
  };

  /// Identifier to represent the contained type of a record-of
  /// as a pseudo-component when checking attribute qualifiers.
  extern const Identifier underscore_zero;

  /** @} end of AST group */

} // namespace Common

#endif // _Common_Identifier_HH
