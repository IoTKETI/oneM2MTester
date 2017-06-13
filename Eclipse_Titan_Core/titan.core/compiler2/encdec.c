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
 *   Delic, Adam
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *
 ******************************************************************************/
#include "../common/memory.h"
#include "datatypes.h"
#include "main.hh"
#include "encdec.h"

void def_encdec(const char *p_classname,
                char **p_classdef, char **p_classsrc,
                boolean ber, boolean raw, boolean text, boolean xer,
                boolean json, boolean is_leaf)
{
  char *def=NULL;
  char *src=NULL;

  def=mputstr
    (def,
     "void encode(const TTCN_Typedescriptor_t&, TTCN_Buffer&,"
     " TTCN_EncDec::coding_t, ...) const;\n"
     "void decode(const TTCN_Typedescriptor_t&, TTCN_Buffer&,"
     " TTCN_EncDec::coding_t, ...);\n");
  if(ber)
    def=mputstr(def,
     "ASN_BER_TLV_t* BER_encode_TLV(const TTCN_Typedescriptor_t& p_td,"
     " unsigned p_coding) const;\n"
     "boolean BER_decode_TLV(const TTCN_Typedescriptor_t& p_td, const"
     " ASN_BER_TLV_t& p_tlv, unsigned L_form);\n"
     );
  if(raw)
    def=mputprintf(def,
     "int RAW_encode(const TTCN_Typedescriptor_t&, RAW_enc_tree&) const;\n"
     "int RAW_decode(const TTCN_Typedescriptor_t&, TTCN_Buffer&,"
     " int, raw_order_t, boolean no_err=FALSE,"
     "int sel_field=-1, boolean first_call=TRUE);\n"
     );
  if(text)
    def=mputprintf(def,
     "int TEXT_encode(const TTCN_Typedescriptor_t&, TTCN_Buffer&) const;\n"
     "int TEXT_decode(const TTCN_Typedescriptor_t&,"
     "TTCN_Buffer&, Limit_Token_List&, boolean no_err=FALSE,"
     "boolean first_call=TRUE);\n"
     );
  if (xer) /* XERSTUFF encdec function headers */
    def=mputprintf(def,
#ifndef NDEBUG
      "// written by %s in " __FILE__ " at %d\n"
#endif
      "int XER_encode(const XERdescriptor_t&, TTCN_Buffer&, unsigned int,"
      "unsigned int, int, embed_values_enc_struct_t*) const;\n"
      "int XER_decode(const XERdescriptor_t&, XmlReaderWrap&, unsigned int, "
      "unsigned int, embed_values_dec_struct_t*);\n"
      "static boolean can_start(const char *name, const char *uri, "
      "XERdescriptor_t const& xd, unsigned int, unsigned int);\n"
      "%s"
#ifndef NDEBUG
      , __FUNCTION__, __LINE__
#endif
      , use_runtime_2 ?
        "boolean can_start_v(const char *name, const char *uri, "
        "XERdescriptor_t const& xd, unsigned int, unsigned int);\n" : ""
    );
  if(json) {
    def = mputprintf(def,
      "int JSON_encode(const TTCN_Typedescriptor_t&, JSON_Tokenizer&) const;\n"
      "int JSON_decode(const TTCN_Typedescriptor_t&, JSON_Tokenizer&, boolean);\n");
  }

  src=mputprintf(src,
     "void %s::encode(const TTCN_Typedescriptor_t& p_td,"
     " TTCN_Buffer& p_buf, TTCN_EncDec::coding_t p_coding, ...) const\n"
     "{\n"
     "  va_list pvar;\n"
     "  va_start(pvar, p_coding);\n"
     "  switch(p_coding) {\n"
     "  case TTCN_EncDec::CT_BER: {\n"
     "    TTCN_EncDec_ErrorContext ec(\"While BER-encoding type"
     " '%%s': \", p_td.name);\n"
     "    unsigned BER_coding=va_arg(pvar, unsigned);\n"
     "    BER_encode_chk_coding(BER_coding);\n"
     "    ASN_BER_TLV_t *tlv=BER_encode_TLV(p_td, BER_coding);\n"
     "    tlv->put_in_buffer(p_buf);\n"
     "    ASN_BER_TLV_t::destruct(tlv);\n"
     "    break;}\n"
     "  case TTCN_EncDec::CT_RAW: {\n"
     "    TTCN_EncDec_ErrorContext ec(\"While RAW-encoding type"
     " '%%s': \", p_td.name);\n"
     "    if(!p_td.raw)\n"
     "      TTCN_EncDec_ErrorContext::error_internal\n"
     "        (\"No RAW descriptor available for type '%%s'.\", p_td.name);\n"
     "    RAW_enc_tr_pos rp;\n"
     "    rp.level=0;\n"
     "    rp.pos=NULL;\n"
     "    RAW_enc_tree root(%s, NULL, &rp, 1, p_td.raw);\n"
     "    RAW_encode(p_td, root);\n"
     "    root.put_to_buf(p_buf);\n"
     "    break;}\n"
     "  case TTCN_EncDec::CT_TEXT: {\n"
     "    TTCN_EncDec_ErrorContext ec("
     "\"While TEXT-encoding type '%%s': \", p_td.name);\n"
     "    if(!p_td.text)\n"
     "      TTCN_EncDec_ErrorContext::error_internal\n"
     "      (\"No TEXT descriptor available for type '%%s'.\", p_td.name);\n"
     "    TEXT_encode(p_td,p_buf);\n"
     "    break;}\n"
     /* XERSTUFF encoder */
     "  case TTCN_EncDec::CT_XER: {\n"
     "    TTCN_EncDec_ErrorContext ec("
     "\"While XER-encoding type '%%s': \", p_td.name);\n"
     "    unsigned XER_coding=va_arg(pvar, unsigned);\n"
     "    XER_encode_chk_coding(XER_coding, p_td);\n"
      /* Do not use %s_xer_ here. It supplies the XER descriptor of oldtype
       * even if encoding newtype for:
       * <ttcn>type newtype oldtype;</ttcn> */
     "    XER_encode(*(p_td.xer),p_buf, XER_coding, 0, 0, 0);\n"
     "    p_buf.put_c('\\n');\n" /* make sure it has a newline */
     "    break;}\n"
     "  case TTCN_EncDec::CT_JSON: {\n"
     "    TTCN_EncDec_ErrorContext ec("
     "\"While JSON-encoding type '%%s': \", p_td.name);\n"
     "    if(!p_td.json)\n"
     "      TTCN_EncDec_ErrorContext::error_internal\n"
     "        (\"No JSON descriptor available for type '%%s'.\", p_td.name);\n"
     "    JSON_Tokenizer tok(va_arg(pvar, int) != 0);\n"
     "    JSON_encode(p_td, tok);\n"
     "    p_buf.put_s(tok.get_buffer_length(), (const unsigned char*)tok.get_buffer());\n"
     "    break;}\n"
     "  default:\n"
     "    TTCN_error(\"Unknown coding method requested to encode"
     " type '%%s'\", p_td.name);\n"
     "  }\n"
     "  va_end(pvar);\n"
     "}\n"
     "\n"
    , p_classname, is_leaf?"TRUE":"FALSE"
    );

  src=mputprintf(src,
#ifndef NDEBUG
     "// written by %s in " __FILE__ " at %d\n"
#endif
     "void %s::decode(const TTCN_Typedescriptor_t& p_td,"
     " TTCN_Buffer& p_buf, TTCN_EncDec::coding_t p_coding, ...)\n"
     "{\n"
     "  va_list pvar;\n"
     "  va_start(pvar, p_coding);\n"
     "  switch(p_coding) {\n"
     "  case TTCN_EncDec::CT_BER: {\n"
     "    TTCN_EncDec_ErrorContext ec(\"While BER-decoding type"
     " '%%s': \", p_td.name);\n"
     "    unsigned L_form=va_arg(pvar, unsigned);\n"
     "    ASN_BER_TLV_t tlv;\n"
     "    BER_decode_str2TLV(p_buf, tlv, L_form);\n"
     "    BER_decode_TLV(p_td, tlv, L_form);\n"
     "    if(tlv.isComplete) p_buf.increase_pos(tlv.get_len());\n"
     "    break;}\n"
     "  case TTCN_EncDec::CT_RAW: {\n"
     "    TTCN_EncDec_ErrorContext ec(\"While RAW-decoding"
     " type '%%s': \", p_td.name);\n"
     "    if(!p_td.raw)\n"
     "      TTCN_EncDec_ErrorContext::error_internal\n"
     "        (\"No RAW descriptor available for type '%%s'.\", p_td.name);\n"
     "    raw_order_t r_order;\n"
     "    switch(p_td.raw->top_bit_order) {\n"
     "    case TOP_BIT_LEFT:\n"
     "      r_order=ORDER_LSB;\n"
     "      break;\n"
     "    case TOP_BIT_RIGHT:\n"
     "    default:\n"
     "      r_order=ORDER_MSB;\n"
     "    }\n"
     "    int rawr = RAW_decode(p_td, p_buf, p_buf.get_len()*8, r_order);\n"
     "    if(rawr<0) switch (-rawr) {\n"
     "    case TTCN_EncDec::ET_INCOMPL_MSG:\n"
     "    case TTCN_EncDec::ET_LEN_ERR:\n"
     "      ec.error((TTCN_EncDec::error_type_t)-rawr, "
     "\"Can not decode type '%%s', because incomplete message was received\", "
     "p_td.name);\n"
     "      break;\n"
     "    case 1:\n" /* from the generic -1 return value */
     "    default:\n"
     "      ec.error(TTCN_EncDec::ET_INVAL_MSG, "
     "\"Can not decode type '%%s', because invalid "
     "message was received\", p_td.name);\n"
     "      break;\n"
     "    }\n"
     "    break;}\n"
     "  case TTCN_EncDec::CT_TEXT: {\n"
     "    Limit_Token_List limit;\n"
     "    TTCN_EncDec_ErrorContext ec(\"While TEXT-decoding type '%%s': \","
     " p_td.name);\n"
     "    if(!p_td.text)\n"
     "      TTCN_EncDec_ErrorContext::error_internal\n"
     "        (\"No TEXT descriptor available for type '%%s'.\", p_td.name);\n"
     "    const unsigned char *b_data=p_buf.get_data();\n"
     "    if(b_data[p_buf.get_len()-1]!='\\0'){\n"
     "      p_buf.set_pos(p_buf.get_len());\n"
     "      p_buf.put_zero(8,ORDER_LSB);\n"
     "      p_buf.rewind();\n"
     "    }\n"
     "    if(TEXT_decode(p_td,p_buf,limit)<0)\n"
     "      ec.error(TTCN_EncDec::ET_INCOMPL_MSG,"
     "\"Can not decode type '%%s', because invalid or incomplete"
     " message was received\", p_td.name);\n"
     "    break;}\n"
     /* XERSTUFF decoder */
     "  case TTCN_EncDec::CT_XER: {\n"
     "    TTCN_EncDec_ErrorContext ec("
     "\"While XER-decoding type '%%s': \", p_td.name);\n"
     "    unsigned XER_coding=va_arg(pvar, unsigned);\n"
     "    XER_encode_chk_coding(XER_coding, p_td);\n"
     "    XmlReaderWrap reader(p_buf);\n"
     "    for (int rd_ok=reader.Read(); rd_ok==1; rd_ok=reader.Read()) {\n"
     "      if (reader.NodeType() == XML_READER_TYPE_ELEMENT) break;\n"
     "    }\n"
     "    XER_decode(*(p_td.xer), reader, XER_coding | XER_TOPLEVEL, XER_NONE, 0);\n"
     "    size_t bytes = reader.ByteConsumed();\n"
     "    p_buf.set_pos(bytes);\n"
     "    break;}\n"
     "  case TTCN_EncDec::CT_JSON: {\n"
     "    TTCN_EncDec_ErrorContext ec(\"While JSON-decoding type '%%s': \","
     " p_td.name);\n"
     "    if(!p_td.json)\n"
     "      TTCN_EncDec_ErrorContext::error_internal\n"
     "        (\"No JSON descriptor available for type '%%s'.\", p_td.name);\n"
     "    JSON_Tokenizer tok((const char*)p_buf.get_data(), p_buf.get_len());\n"
     "    if(JSON_decode(p_td, tok, FALSE)<0)\n"
     "      ec.error(TTCN_EncDec::ET_INCOMPL_MSG,"
     "\"Can not decode type '%%s', because invalid or incomplete"
     " message was received\", p_td.name);\n"
     "    p_buf.set_pos(tok.get_buf_pos());\n"
     "    break;}\n"
     "  default:\n"
     "    TTCN_error(\"Unknown coding method requested to decode"
     " type '%%s'\", p_td.name);\n"
     "  }\n"
     "  va_end(pvar);\n"
     "}\n\n"
#ifndef NDEBUG
     , __FUNCTION__, __LINE__
#endif
     , p_classname);

  *p_classdef=mputstr(*p_classdef, def);
  Free(def);
  *p_classsrc=mputstr(*p_classsrc, src);
  Free(src);
}

char *genRawFieldChecker(char *src, const rawAST_coding_taglist *taglist,
  boolean is_equal)
{
  int i;
  for (i = 0; i < taglist->nElements; i++) {
    rawAST_coding_field_list *fields = taglist->fields + i;
    char *field_name = NULL;
    boolean first_expr = TRUE;
    int j;
    if (i > 0) src = mputstr(src, is_equal ? " || " : " && ");
    for (j = 0; j < fields->nElements; j++) {
      rawAST_coding_fields *field = fields->fields + j;
      if (j == 0) {
        /* this is the first field reference */
        if (field->fieldtype == UNION_FIELD)
          field_name = mputprintf(field_name,"(*field_%s)",field->nthfieldname);
        else
          field_name = mputprintf(field_name,"field_%s", field->nthfieldname);
      }
      else {
        /* this is not the first field reference */
        if (field->fieldtype == UNION_FIELD) {
          /* checking for the right selection within the union */
          if (first_expr) {
            if (taglist->nElements > 1) src = mputc(src, '(');
            first_expr = FALSE;
          }
          else src = mputstr(src, is_equal ? " && " : " || ");
          src = mputprintf(src, "%s.get_selection() %s %s%s%s", field_name,
            is_equal ? "==" : "!=", fields->fields[j - 1].type,
            "::ALT_", field->nthfieldname);
        }
        /* appending the current field name to the field reference */
        field_name = mputprintf(field_name, ".%s()", field->nthfieldname);
      }
      if (j < fields->nElements - 1 && field->fieldtype == OPTIONAL_FIELD) {
        /* this is not the last field in the chain and it is optional */
        if (first_expr) {
          if (taglist->nElements > 1) src = mputc(src, '(');
          first_expr = FALSE;
        }
        else src = mputstr(src, is_equal ? " && " : " || ");
        /* check for the presence */
        if (!is_equal) src = mputc(src, '!');
        src = mputprintf(src, "%s.ispresent()", field_name);
        /* add an extra () to the field reference */
        field_name = mputstr(field_name, "()");
      }
    }
    if (!first_expr) src = mputstr(src, is_equal ? " && " : " || ");
    /* compare the referred field with the given value */
    src = mputprintf(src, "%s %s %s", field_name, is_equal ? "==" : "!=",
      fields->value);
    if (!first_expr && taglist->nElements > 1) src = mputc(src, ')');
    Free(field_name);
  }
  return src;
}

char *genRawTagChecker(char *src, const rawAST_coding_taglist *taglist)
{
  int temp_tag, l;
  rawAST_coding_field_list temp_field;
  src = mputstr(src, "  RAW_enc_tree* temp_leaf;\n");
  for (temp_tag = 0; temp_tag < taglist->nElements; temp_tag++) {
    temp_field = taglist->fields[temp_tag];
    src = mputprintf(src, "  {\n"
      "  RAW_enc_tr_pos pr_pos%d;\n"
      "  pr_pos%d.level=myleaf.curr_pos.level+%d;\n"
      "  int new_pos%d[]={", temp_tag, temp_tag, temp_field.nElements, temp_tag);
    for (l = 0; l < temp_field.nElements; l++) {
      src= mputprintf(src, "%s%d", l ? "," : "", temp_field.fields[l].nthfield);
    }
    src = mputprintf(src, "};\n"
      "  pr_pos%d.pos=init_new_tree_pos(myleaf.curr_pos,%d,new_pos%d);\n"
      "  temp_leaf = myleaf.get_node(pr_pos%d);\n"
      "  if(temp_leaf != NULL){\n", temp_tag, temp_field.nElements, temp_tag, temp_tag);
    if (temp_field.value[0] != ' ') {
      src = mputprintf(src, "  %s new_val = %s;\n"
        "  new_val.RAW_encode(%s_descr_,*temp_leaf);\n",
        temp_field.fields[temp_field.nElements - 1].type, temp_field.value,
        temp_field.fields[temp_field.nElements - 1].typedescr);
    }
    else {
      src = mputprintf(src, "  %s.RAW_encode(%s_descr_,*temp_leaf);\n",
        temp_field.value, temp_field.fields[temp_field.nElements - 1].typedescr);
    }
    src = mputstr(src, "  } else");
  }
  src = mputstr(src, " {\n"
    "    TTCN_EncDec_ErrorContext::error\n"
    "      (TTCN_EncDec::ET_OMITTED_TAG, \"Encoding a tagged, but omitted"
    " value.\");\n"
    "  }\n");
  for (temp_tag = taglist->nElements - 1; temp_tag >= 0 ; temp_tag--) {
    src = mputprintf(src, "  free_tree_pos(pr_pos%d.pos);\n"
      "  }\n", temp_tag);
  }
  return src;
}
