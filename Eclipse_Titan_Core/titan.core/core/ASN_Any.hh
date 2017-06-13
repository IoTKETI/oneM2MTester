/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Delic, Adam
 *   Forstner, Matyas
 *   Raduly, Csaba
 *
 ******************************************************************************/
#ifndef ASN_ANY_HH
#define ASN_ANY_HH

#include "Octetstring.hh"

class ASN_ANY : public OCTETSTRING
{
public:
  ASN_ANY() : OCTETSTRING() {}
  ASN_ANY(int n_octets, const unsigned char* octets_ptr)
    : OCTETSTRING(n_octets, octets_ptr) {}
  ASN_ANY(const OCTETSTRING_ELEMENT& other_value)
    : OCTETSTRING(other_value) {}
  ASN_ANY(const OCTETSTRING& other_value)
    : OCTETSTRING(other_value) {}
  ASN_ANY(const ASN_ANY& other_value)
    : OCTETSTRING(other_value) {}

#ifdef TITAN_RUNTIME_2
  void set_value(const Base_Type* other_value) { *this = *(static_cast<const ASN_ANY*>(other_value)); }
  Base_Type* clone() const { return new ASN_ANY(*this); }
  const TTCN_Typedescriptor_t* get_descriptor() const { return &ASN_ANY_descr_; }
#endif

  void encode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf,
              TTCN_EncDec::coding_t p_coding, ...) const;

  void decode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf,
              TTCN_EncDec::coding_t p_coding, ...);

  ASN_BER_TLV_t* BER_encode_TLV(const TTCN_Typedescriptor_t& p_td,
                                unsigned p_coding) const;

  boolean BER_decode_TLV(const TTCN_Typedescriptor_t& p_td,
                         const ASN_BER_TLV_t& p_tlv, unsigned L_form);


};

typedef OCTETSTRING_template ASN_ANY_template;

#endif /* ASN_ANY_HH */
