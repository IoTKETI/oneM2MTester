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
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#ifndef RAW_AST_HH
#define RAW_AST_HH

#include "rawASTspec.h"
#include "../Setting.hh"
#include "../Identifier.hh"
#include "../../common/CharCoding.hh"

class XerAttributes;
class TextAST;
class BerAST;
class JsonAST;

namespace Ttcn {
  class AttributeSpec;
}

typedef struct{
     int ext_bit;
     Common::Identifier *from;
     Common::Identifier *to;
} rawAST_ext_bit_group;

typedef struct {
    int nElements;
    Common::Identifier **names;
} rawAST_field_list;

typedef struct {
    rawAST_field_list* keyField;
    char* value;
    Common::Value *v_value;
} rawAST_tag_field_value;

typedef struct {
    Common::Identifier* fieldName;
    int nElements;
    rawAST_tag_field_value* keyList;
} rawAST_single_tag;

typedef struct {
    int nElements;
    rawAST_single_tag* tag;
} rawAST_tag_list;

typedef struct{
  char* value;
  Common::Value *v_value;
} rawAST_values;

/** Parsed RAW encoder attributes */
class RawAST{
private:
    void init_rawast(bool int_type);
    /** No copying */
    RawAST(const RawAST&);
    /** No assignment */
    RawAST& operator=(const RawAST&);
public:
    int fieldlength;            /**< Nr of bits per character, hexstring : 4,
                                     octetstring and charstring : 8, etc */
    int comp;                   /**< Handling of sign: no, 2scomp, signbit */
    int byteorder;              /**< XDEFMSB, XDEFLSB */
    int align;                  /**< XDEFLEFT, XDEFRIGHT */
    int bitorderinfield;        /**< XDEFMSB, XDEFLSB */
    int bitorderinoctet;        /**< XDEFMSB, XDEFLSB */
    int extension_bit;          /**< XDEFYES, XDEFNO
                                   can be used for record fields:
                                   variant (field1) EXTENSION_BIT(use)*/
    int ext_bit_goup_num;
    rawAST_ext_bit_group *ext_bit_groups;
    int hexorder;               /**< XDEFLOW, XDEFHIGH */
    int padding;                /**< XDEFYES: next field starts at next octet */
    int prepadding;
    int padding_pattern_length;
    int paddall;
    int repeatable;
    char* padding_pattern;
    int fieldorder;             /**< XDEFMSB, XDEFLSB */
    int lengthto_num;
    Common::Identifier **lengthto;      /**< list of fields to generate length for */
    Common::Identifier *pointerto;      /**< pointer to the specified field is contained
                                   in this field */
    int ptroffset;            /**< offset to the pointer value in bits
                                   Actual position will be:
                                   pointerto*ptrunit + ptroffset */
    Common::Identifier *ptrbase; /**< the identifier in PTROFFSET(identifier) */
    int unit;                   /**< XDEFOCTETS, XDEFBITS */
    rawAST_field_list *lengthindex;          /**< stores subattribute of the lengthto
                                   attribute */
    rawAST_tag_list crosstaglist;
    rawAST_tag_list taglist;           /**< field IDs in form of
                                   [unionField.sub]field_N,
                                   keyField.subfield_M = tagValue
                                   multiple tagValues may be specified */
    rawAST_single_tag presence;    /**< Presence indicator expressions for an
                                   optional field */
    int topleveleind;
    rawAST_toplevel toplevel;      /**< Toplevel attributes */
    int length_restrition;
    bool intx; /**< IntX encoding for integers */
    CharCoding::CharCodingType stringformat; /**< String serialization type for
                                               * universal charstrings */
    /** Default constructor.
     *  Calls \c init_rawast(false).
     *  \todo should be merged with the next one */
    RawAST();
    /** Constructor.
     *  Calls \c init_rawast(int_type). */
    RawAST(bool int_type);
    /** Sort-of copy constructor. */
    RawAST(RawAST *other, bool int_type);
    ~RawAST();
    void print_RawAST();

};

/** The entry point for the RAW attribute parser.
 *
 * This is overloaded with the TEXT and XER attribute parsing.
 *
 * Called from Common::Type::parse_raw()
 *
 * @param[out] par the parsed RAW encoder attributes
 * @param[out] textpar the parsed TEXT encoder attributes
 * @param[out] xerpar the parsed XER encoder settings
 * @param[out] jsonpar the parsed JSON encoder settings
 * @param[in] attrib attribute specification to be parsed
 * @param[in] l_multip length multiplier (4 for \c hexstring,
 *            8 for \c octet \c string and \c charstring,
 *            1 for anything else)
 * @param[in] mod pointer to the current module
 * @param[out] raw_found set to \c true if a RAW attribute was found
 *             (or if none found)
 * @param[out] text_found set to \c true if a TEXT attribute was found
 * @param[out] xer_found set to \c true if a XER attribute was found
 * @param[out] json_found set to \c true if a JSON attribute was found
 * @return the return value of rawAST_parse: 0 for success, 1 for error,
 * 2 for out of memory.
 */
extern int parse_rawAST(RawAST *par, TextAST *textpar, XerAttributes *xerpar,
    BerAST *berpar, JsonAST* jsonpar, const Ttcn::AttributeSpec& attrib, 
    int l_multip, const Common::Module* mod, bool &raw_found, bool &text_found,
    bool &xer_found, bool &ber_found, bool &json_found);
extern void copy_rawAST_to_struct(RawAST *from, raw_attrib_struct *to);
extern void free_raw_attrib_struct(raw_attrib_struct *raw);
extern int compare_raw_attrib(RawAST *a, RawAST *b);

extern void free_rawAST_field_list(rawAST_field_list *ptr);
extern rawAST_single_tag *link_rawAST_single_tag(rawAST_single_tag *dst,
  rawAST_single_tag *src);
extern void free_rawAST_single_tag(rawAST_single_tag *spec);
extern void free_rawAST_tag_field_value(rawAST_tag_field_value *spec);
extern rawAST_tag_field_value *link_rawAST_tag_field_value(
  rawAST_tag_field_value *dst, rawAST_tag_field_value* src);
extern rawAST_tag_field_value *init_rawAST_tag_field_value(
  rawAST_tag_field_value *spec);
extern void free_rawAST_tag_list(rawAST_tag_list *spec);
extern rawAST_tag_list* link_rawAST_tag_list(rawAST_tag_list *dst,
  rawAST_tag_list *src);

#endif
