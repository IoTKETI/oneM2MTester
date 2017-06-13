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
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *
 ******************************************************************************/
#include <string.h>
#include "../common/memory.h"
#include "Types.h"
#include "Encdec.hh"
#include "RAW.hh"
#include "Basetype.hh"
#include "Integer.hh"

#include <openssl/bn.h>

const unsigned char BitReverseTable[256] =
{
0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};

const unsigned char BitMaskTable[9]={
0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff
};

/**
 * Initialize the RAW encoding tree. The tree representations makes it easier
 * to encode/decode structured types with various attributes. Each node in the
 * tree stores information about the node's parent, position, attributes,
 * child nodes etc. The root of the tree is on the first level and its ``par''
 * is always NULL. That's why there's a ``par_pos'' parameter, but it could be
 * omitted. The first part of the position route in ``curr_pos.pos'' is
 * inherited from ``par''. The last element is the number of the current
 * element. Only the leaves carry data. Other nodes are just for construction
 * purposes.
 *
 * @param is_leaf true if it's a node with no children
 * @param par the parent of the current node
 * @param par_pos the parent's position
 * @param my_pos the child node's number of ``par''
 * @param raw_attr encoding attributes
 */
RAW_enc_tree::RAW_enc_tree(boolean is_leaf, RAW_enc_tree *par,
  RAW_enc_tr_pos *par_pos, int my_pos, const TTCN_RAWdescriptor_t *raw_attr)
{
  boolean orders = FALSE;
  isleaf = is_leaf;
  must_free = FALSE;
  data_ptr_used = FALSE;
  rec_of = FALSE;
  parent = par;
  curr_pos.pos = (int*) Malloc((par_pos->level+1)*sizeof(int));
  if (par_pos->level) memcpy((void*) curr_pos.pos, (void*) par_pos->pos,
    par_pos->level * sizeof(int));
  curr_pos.level = par_pos->level + 1;
  curr_pos.pos[curr_pos.level - 1] = my_pos;
  length = 0;
  padding = raw_attr->padding;
  prepadding = raw_attr->prepadding;
  padding_pattern_length = raw_attr->padding_pattern_length;
  padding_pattern = raw_attr->padding_pattern;
  startpos = 0;
  padlength = 0;
  prepadlength = 0;
  align = 0;
  ext_bit_handling = 0;
  coding_descr = NULL;
  ext_bit = raw_attr->extension_bit;
  top_bit_order = raw_attr->top_bit_order;
  calc = CALC_NO;
  if (raw_attr->byteorder == ORDER_MSB) orders = TRUE;
  if (raw_attr->bitorderinfield == ORDER_MSB) orders = !orders;
  coding_par.byteorder = orders ? ORDER_MSB : ORDER_LSB;
  orders = FALSE;
  if (raw_attr->bitorderinoctet == ORDER_MSB) orders = TRUE;
  if (raw_attr->bitorderinfield == ORDER_MSB) orders = !orders;
  coding_par.bitorder = orders ? ORDER_MSB : ORDER_LSB;
  coding_par.hexorder = raw_attr->hexorder;
  coding_par.fieldorder = raw_attr->fieldorder;
  if (isleaf) {
    body.leaf.data_ptr = NULL;
  }
  else {
    body.node.num_of_nodes = 0;
    body.node.nodes = NULL;
  }
}

RAW_enc_tree::~RAW_enc_tree()
{
  if (isleaf) {
    if (must_free) Free(body.leaf.data_ptr);
  }
  else {
    for (int a = 0; a < body.node.num_of_nodes; a++) {
      if (body.node.nodes[a] != NULL) delete body.node.nodes[a];
    }
    Free(body.node.nodes);
  }
  switch (calc) {
  case CALC_LENGTH:
    Free(calcof.lengthto.fields);
    break;
  case CALC_POINTER:
    break;
  default:
    break;
  }
  Free(curr_pos.pos);
}

void RAW_enc_tree::put_to_buf(TTCN_Buffer &buf){
//printf("Start put_to_buf\n\r");
  calc_padding(0);
//printf("End padding\n\r");
  calc_fields();
//printf("End calc\n\r");
  fill_buf(buf);
//printf("End fill\n\r");
}

void RAW_enc_tree::calc_fields()
{
  if (isleaf) {
    int szumm = 0;
    RAW_enc_tree *atm;
    switch (calc) {
    case CALC_LENGTH: {
      if (calcof.lengthto.unit != -1) {
        for (int a = 0; a < calcof.lengthto.num_of_fields; a++) {
          atm = get_node(calcof.lengthto.fields[a]);
          if (atm) szumm += atm->length + atm->padlength + atm->prepadlength;
        }
        szumm = (szumm + calcof.lengthto.unit - 1) / calcof.lengthto.unit;
      }
      else {
        atm = get_node(calcof.lengthto.fields[0]);
        if (atm) szumm = atm->body.node.num_of_nodes;
      }
      INTEGER temp(szumm);
      temp.RAW_encode(*coding_descr, *this);
      break; }
    case CALC_POINTER: {
      int cl = curr_pos.pos[curr_pos.level - 1];
      curr_pos.pos[curr_pos.level - 1] = calcof.pointerto.ptr_base;
      int base = calcof.pointerto.ptr_base;
      RAW_enc_tree *b = get_node(curr_pos);
      while (b == NULL) {
        base++;
        curr_pos.pos[curr_pos.level - 1] = base;
        b = get_node(curr_pos);
      }
      curr_pos.pos[curr_pos.level - 1] = cl;
      atm = get_node(calcof.pointerto.target);
      if (atm) szumm = (atm->startpos - b->startpos + calcof.pointerto.unit - 1
        - calcof.pointerto.ptr_offset) / calcof.pointerto.unit;
      INTEGER temp(szumm);
      temp.RAW_encode(*coding_descr, *this);
      break; }
    default:
      break;
    }
  }
  else {
    for (int a = 0; a < body.node.num_of_nodes; a++)
      if (body.node.nodes[a]) body.node.nodes[a]->calc_fields();
  }
}

int RAW_enc_tree::calc_padding(int position)
{
  int current_pos = position;
  startpos = position;
  if (prepadding) {
    int new_pos = ((current_pos + prepadding - 1) / prepadding) * prepadding;
    prepadlength = new_pos - position;
    current_pos = new_pos;
  }
  if (!isleaf) {
    for (int a = 0; a < body.node.num_of_nodes; a++) {
      if (body.node.nodes[a]) {
        current_pos = body.node.nodes[a]->calc_padding(current_pos);
      }
    }
    length = current_pos - position - prepadlength;
  }
  else current_pos += length;
  if (padding) {
    int new_pos = ((current_pos + padding - 1) / padding) * padding;
    padlength = new_pos - length - position - prepadlength;
    current_pos = new_pos;
  }
  return current_pos;
}

void RAW_enc_tree::fill_buf(TTCN_Buffer &buf)
{
  boolean old_order = buf.get_order();
  if (top_bit_order != TOP_BIT_INHERITED) buf.set_order(top_bit_order
    != TOP_BIT_RIGHT);
  buf.put_pad(prepadlength, padding_pattern, padding_pattern_length,
    coding_par.fieldorder);
  if (isleaf) {
//printf("align: %d, length: %d\n\r",align,length);
    int align_length = align < 0 ? -align : align;
    if (ext_bit != EXT_BIT_NO) buf.start_ext_bit(ext_bit == EXT_BIT_REVERSE);
//      if(ext_bit_handling%2) buf.start_ext_bit(ext_bit==EXT_BIT_REVERSE);
    if (data_ptr_used)
      buf.put_b(length - align_length, body.leaf.data_ptr, coding_par, align);
    else
      buf.put_b(length - align_length, body.leaf.data_array, coding_par, align);
    if (ext_bit_handling > 1)
      buf.stop_ext_bit();
    else if (ext_bit != EXT_BIT_NO && !ext_bit_handling) buf.stop_ext_bit();
  }
  else {
    if (ext_bit != EXT_BIT_NO && (!rec_of || ext_bit_handling % 2))
      buf.start_ext_bit(ext_bit == EXT_BIT_REVERSE);
    for (int a = 0; a < body.node.num_of_nodes; a++) {
      if (body.node.nodes[a]) body.node.nodes[a]->fill_buf(buf);
      if (ext_bit != EXT_BIT_NO && rec_of && !ext_bit_handling)
        buf.set_last_bit(ext_bit != EXT_BIT_YES);
    }
    if (!ext_bit_handling) {
      if (ext_bit != EXT_BIT_NO && !rec_of)
        buf.stop_ext_bit();
      else if (ext_bit != EXT_BIT_NO && rec_of)
        buf.set_last_bit(ext_bit == EXT_BIT_YES);
    }
    else if (ext_bit_handling > 1) buf.stop_ext_bit();
  }
  buf.put_pad(padlength, padding_pattern, padding_pattern_length,
    coding_par.fieldorder);
  buf.set_order(old_order);
}

/**
 * Return the element at ``req_pos'' from the RAW encoding tree. At first get
 * the root of the whole tree at the first level. Then go down in the tree
 * following the route in the ``req_pos.pos'' array. If the element was not
 * found NULL is returned.
 *
 * @param req_pos the position of the element
 * @return the element at the given position
 */
RAW_enc_tree* RAW_enc_tree::get_node(RAW_enc_tr_pos &req_pos)
{
  if (req_pos.level == 0) return NULL;
  RAW_enc_tree* t = this;
  int cur_level = curr_pos.level;
  for (int b = 1; b < cur_level; b++) t = t->parent;
  for (cur_level = 1; cur_level < req_pos.level; cur_level++) {
    if (!t || t->isleaf || t->body.node.num_of_nodes <= req_pos.pos[cur_level]) return NULL;
    t = t->body.node.nodes[req_pos.pos[cur_level]];
  }
  return t;
}

/**
 * Return the number of bits needed to represent an integer value `a'.  The
 * sign bit is added for negative values.  It has a different implementation
 * for `BIGNUM' values.
 *
 * @param a the integer in question
 * @return the number of bits needed to represent the given integer
 * in sign+magnitude
 */
int min_bits(int a)
{
  register int bits = 0;
  register int tmp = a;
  if (a < 0) {
    bits = 1;
    tmp = -a;
  }
  while (tmp != 0) {
    bits++;
    tmp /= 2;
  }
  return bits;
}

int min_bits(BIGNUM *a)
{
  if (!a) return 0;
  register int bits = BN_num_bits(a) + BN_is_negative(a);
  return bits;
}

// Called from code generated by enum.c (defEnumClass)
int RAW_encode_enum_type(const TTCN_Typedescriptor_t& p_td,
  RAW_enc_tree& myleaf, int integer_value, int min_bits_enum)
{
  int fl = p_td.raw->fieldlength ? p_td.raw->fieldlength : min_bits_enum;
  TTCN_RAWdescriptor_t my_raw;
  my_raw.fieldlength     = fl;
  my_raw.comp            = p_td.raw->comp;
  my_raw.byteorder       = p_td.raw->byteorder;
  my_raw.endianness      = p_td.raw->endianness;
  my_raw.bitorderinfield = p_td.raw->bitorderinfield;
  my_raw.bitorderinoctet = p_td.raw->bitorderinoctet;
  my_raw.extension_bit   = p_td.raw->extension_bit;
  my_raw.hexorder        = p_td.raw->hexorder;
  my_raw.fieldorder      = p_td.raw->fieldorder;
  my_raw.top_bit_order   = p_td.raw->top_bit_order;
  my_raw.padding         = p_td.raw->padding;
  my_raw.prepadding      = p_td.raw->prepadding;
  my_raw.ptroffset       = p_td.raw->ptroffset;
  my_raw.unit            = p_td.raw->unit;
  TTCN_Typedescriptor_t my_descr = { p_td.name, 0, &my_raw, NULL, NULL, NULL,
    NULL, TTCN_Typedescriptor_t::DONTCARE };
  INTEGER i(integer_value);
  i.RAW_encode(my_descr, myleaf);
  //  myleaf.align=0;//p_td.raw->endianness==ORDER_MSB?min_bits_enum-fl:fl-min_bits_enum;
  return myleaf.length = fl;
}

// Called from code generated by enum.c (defEnumClass)
int RAW_decode_enum_type(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& buff,
  int limit, raw_order_t top_bit_ord, int& value, int min_bits_enum,
  boolean no_err)
{
  int fl = p_td.raw->fieldlength ? p_td.raw->fieldlength : min_bits_enum;
  TTCN_RAWdescriptor_t my_raw;
  my_raw.fieldlength     = fl;
  my_raw.comp            = p_td.raw->comp;
  my_raw.byteorder       = p_td.raw->byteorder;
  my_raw.endianness      = p_td.raw->endianness;
  my_raw.bitorderinfield = p_td.raw->bitorderinfield;
  my_raw.bitorderinoctet = p_td.raw->bitorderinoctet;
  my_raw.extension_bit   = p_td.raw->extension_bit;
  my_raw.hexorder        = p_td.raw->hexorder;
  my_raw.fieldorder      = p_td.raw->fieldorder;
  my_raw.top_bit_order   = p_td.raw->top_bit_order;
  my_raw.padding         = p_td.raw->padding;
  my_raw.prepadding      = p_td.raw->prepadding;
  my_raw.ptroffset       = p_td.raw->ptroffset;
  my_raw.unit            = p_td.raw->unit;
  TTCN_Typedescriptor_t my_descr = { p_td.name, 0, &my_raw, NULL, NULL, NULL,
    NULL, TTCN_Typedescriptor_t::DONTCARE };
  INTEGER i;
  /*  if(p_td.raw->endianness==ORDER_MSB)
   buff.increase_pos_bit(fl-min_bits_enum);*/
  fl = i.RAW_decode(my_descr, buff, limit, top_bit_ord, no_err);
  if (fl < 0) return fl;
  value = (int) i;
  /*  if(p_td.raw->endianness==ORDER_LSB)
   buff.increase_pos_bit(fl-min_bits_enum);*/
  return fl + buff.increase_pos_padd(p_td.raw->padding);
}

RAW_enc_tree** init_nodes_of_enc_tree(int nodes_num)
{
  RAW_enc_tree ** ret_val=(RAW_enc_tree **) Malloc(nodes_num*sizeof(RAW_enc_tree*));
  memset(ret_val,0,nodes_num*sizeof(RAW_enc_tree*));  
  return ret_val;
}

RAW_enc_tr_pos* init_lengthto_fields_list(int num){
  return (RAW_enc_tr_pos *)Malloc(num*sizeof(RAW_enc_tr_pos));
}

int* init_new_tree_pos(RAW_enc_tr_pos &old_pos,int new_levels, int* new_pos){
  int *new_position=(int*)Malloc((old_pos.level+new_levels)*sizeof(int));
  memcpy(new_position,old_pos.pos,old_pos.level*sizeof(int));
  memcpy(new_position+old_pos.level,new_pos,new_levels*sizeof(int));
  return new_position;
}

void free_tree_pos(int* ptr){
  Free(ptr);
}

int min_of_ints(int num_of_int,...)
{
  va_list pvar;
  va_start(pvar,num_of_int);
  int min_val = 0;
  if (num_of_int > 0) {
    min_val = va_arg(pvar, int);
    for (int a = 1; a < num_of_int; a++) {
      int b = va_arg(pvar, int);
      if (b < min_val) min_val = b;
    }
  }
  va_end(pvar);
  return min_val;
}

/** Default descriptors of RAW encoding for primitive types.                                                                                             padding
 *                                                                                                                                                       | prepadding
 *                                                                                                                                                       |   ptroffset
 *                                                                                                                                                       |     unit
 *                                                                                                                                                       |     | padding_pattern_length
 *                                                                                                                                                       |     |   padding_pattern
 *                                                 length,comp ,byteorder,align    ,ord_field,ord_octet,ext_bit   ,hexorder,fieldorder,top_bit,          |     |        length_restriction*/
const TTCN_RAWdescriptor_t INTEGER_raw_=               {8,SG_NO,ORDER_LSB,ORDER_LSB,ORDER_LSB,ORDER_LSB,EXT_BIT_NO,ORDER_LSB,ORDER_LSB,TOP_BIT_INHERITED,0,0,0,8,0,NULL,-1,CharCoding::UNKNOWN};
const TTCN_RAWdescriptor_t BOOLEAN_raw_=               {1,SG_NO,ORDER_LSB,ORDER_LSB,ORDER_LSB,ORDER_LSB,EXT_BIT_NO,ORDER_LSB,ORDER_LSB,TOP_BIT_INHERITED,0,0,0,8,0,NULL,-1,CharCoding::UNKNOWN};
const TTCN_RAWdescriptor_t BITSTRING_raw_=             {0,SG_NO,ORDER_LSB,ORDER_LSB,ORDER_LSB,ORDER_LSB,EXT_BIT_NO,ORDER_LSB,ORDER_LSB,TOP_BIT_INHERITED,0,0,0,8,0,NULL,-1,CharCoding::UNKNOWN};
const TTCN_RAWdescriptor_t OCTETSTRING_raw_=           {0,SG_NO,ORDER_LSB,ORDER_LSB,ORDER_LSB,ORDER_LSB,EXT_BIT_NO,ORDER_LSB,ORDER_LSB,TOP_BIT_INHERITED,0,0,0,8,0,NULL,-1,CharCoding::UNKNOWN};
const TTCN_RAWdescriptor_t HEXSTRING_raw_=             {0,SG_NO,ORDER_LSB,ORDER_LSB,ORDER_LSB,ORDER_LSB,EXT_BIT_NO,ORDER_LSB,ORDER_LSB,TOP_BIT_INHERITED,0,0,0,8,0,NULL,-1,CharCoding::UNKNOWN};
const TTCN_RAWdescriptor_t CHARSTRING_raw_=            {0,SG_NO,ORDER_LSB,ORDER_LSB,ORDER_LSB,ORDER_LSB,EXT_BIT_NO,ORDER_LSB,ORDER_LSB,TOP_BIT_INHERITED,0,0,0,8,0,NULL,-1,CharCoding::UNKNOWN};
const TTCN_RAWdescriptor_t FLOAT_raw_=                {64,SG_NO,ORDER_LSB,ORDER_LSB,ORDER_LSB,ORDER_LSB,EXT_BIT_NO,ORDER_LSB,ORDER_LSB,TOP_BIT_INHERITED,0,0,0,8,0,NULL,-1,CharCoding::UNKNOWN};
const TTCN_RAWdescriptor_t UNIVERSAL_CHARSTRING_raw_ = {0,SG_NO,ORDER_LSB,ORDER_LSB,ORDER_LSB,ORDER_LSB,EXT_BIT_NO,ORDER_LSB,ORDER_LSB,TOP_BIT_INHERITED,0,0,0,8,0,NULL,-1,CharCoding::UNKNOWN};
