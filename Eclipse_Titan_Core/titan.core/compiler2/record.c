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
 *   Cserveni, Akos
 *   Delic, Adam
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Kremer, Peter
 *   Ormandi, Matyas
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *
 ******************************************************************************/
#include <string.h>
#include "datatypes.h"
#include "../common/memory.h"
#include "record.h"
#include "encdec.h"

#include "main.hh"
#include "error.h"
#include "ttcn3/compiler.h"

static void defEmptyRecordClass(const struct_def *sdef,
                                output_struct *output);

static void defEmptyRecordTemplate(const char *name, const char *dispname,
  output_struct *output);

/** this is common code for both empty and non-empty cases, called from
 *  both functions for template generation */
static void defCommonRecordTemplate(const char *name,
  char **def, char **src);

/** code generation for original runtime */
static void defRecordClass1(const struct_def *sdef, output_struct *output);
static void defRecordTemplate1(const struct_def *sdef, output_struct *output);
/** code generation for alternative runtime (TITAN_RUNTIME_2) */
static void defRecordClass2(const struct_def *sdef, output_struct *output);
static void defRecordTemplate2(const struct_def *sdef, output_struct *output);

void defRecordClass(const struct_def *sdef, output_struct *output)
{
  if (use_runtime_2) defRecordClass2(sdef, output);
  else defRecordClass1(sdef, output);
}

void defRecordTemplate(const struct_def *sdef, output_struct *output)
{
  if (use_runtime_2) defRecordTemplate2(sdef, output);
  else defRecordTemplate1(sdef, output);
}

struct raw_option_struct {
  boolean lengthto; /* indicates whether this field contains a length */
  int lengthof; /* how many length indicators this field counted in */
  int *lengthoffield; /* the list of length indicator field indices */
  boolean pointerto;
  int pointerof;
  boolean ptrbase;
  int extbitgroup;
  int tag_type;
  boolean delayed_decode; /* indicates whether the field has to be decoded
			 out of order (later) */
  int nof_dependent_fields; /* list of fields that are to be decoded */
  int *dependent_fields; /* after this field */
};

static char *genRawFieldDecodeLimit(char *src, const struct_def *sdef,
  int i, const struct raw_option_struct *raw_options);

static char *genRawDecodeRecordField(char *src, const struct_def *sdef,
  int i, const struct raw_option_struct *raw_options, boolean delayed_decode,
  int *prev_ext_group);

static void set_raw_options(const struct_def *sdef,
  struct raw_option_struct *raw_options, boolean* haslengthto,
  boolean* haspointer, boolean* hascrosstag, boolean* has_ext_bit);

static char *generate_raw_coding(char *src,
  const struct_def *sdef, struct raw_option_struct *raw_options,
  boolean haspointer, boolean hascrosstag, boolean has_ext_bit);
static char *generate_raw_coding_negtest(char *src,
  const struct_def *sdef, struct raw_option_struct *raw_options);

void set_raw_options(const struct_def *sdef,
  struct raw_option_struct *raw_options, boolean *haslengthto,
  boolean *haspointer, boolean *hascrosstag, boolean *has_ext_bit)
{
    size_t i;
    for (i = 0; i < sdef->nElements; i++) {
      raw_options[i].lengthto = FALSE;
      raw_options[i].lengthof = 0;
      raw_options[i].lengthoffield = NULL;
      raw_options[i].pointerto = FALSE;
      raw_options[i].pointerof = 0;
      raw_options[i].ptrbase = FALSE;
      raw_options[i].extbitgroup = 0;
      raw_options[i].tag_type = 0;
      raw_options[i].delayed_decode = FALSE;
      raw_options[i].nof_dependent_fields = 0;
      raw_options[i].dependent_fields = NULL;
    }
    *haslengthto = FALSE;
    *haspointer = FALSE;
    *hascrosstag = FALSE;
    *has_ext_bit = sdef->hasRaw && sdef->raw.extension_bit!=XDEFNO &&
                                  sdef->raw.extension_bit!=XDEFDEFAULT;
    for(i=0;i<sdef->nElements;i++){
      if(sdef->elements[i].hasRaw &&
                                 sdef->elements[i].raw.crosstaglist.nElements){
        *hascrosstag = TRUE;
        break;
      }
    }
    if(sdef->hasRaw){ /* fill tag_type. 0-No tag, >0 index of the tag + 1 */
      for(i=0;i<sdef->raw.taglist.nElements;i++){
        raw_options[sdef->raw.taglist.list[i].fieldnum].tag_type= i+1;
      }
      for(i=0;i<sdef->raw.ext_bit_goup_num;i++){
        int k;
        for(k=sdef->raw.ext_bit_groups[i].from;
                                    k<=sdef->raw.ext_bit_groups[i].to;k++)
            raw_options[k].extbitgroup=i+1;
      }
    }
    for(i=0;i<sdef->nElements;i++){
      if (sdef->elements[i].hasRaw && sdef->elements[i].raw.lengthto_num > 0) {
        int j;
        *haslengthto = TRUE;
        raw_options[i].lengthto = TRUE;
        for(j = 0; j < sdef->elements[i].raw.lengthto_num; j++) {
	  int field_index = sdef->elements[i].raw.lengthto[j];
	  raw_options[field_index].lengthoffield = (int*)
	    Realloc(raw_options[field_index].lengthoffield,
	      (raw_options[field_index].lengthof + 1) * sizeof(int));
	  raw_options[field_index].lengthoffield[
	    raw_options[field_index].lengthof] = i;
	  raw_options[field_index].lengthof++;
        }
      }
      if(sdef->elements[i].hasRaw && sdef->elements[i].raw.pointerto!=-1){
        raw_options[i].pointerto = TRUE;
        raw_options[sdef->elements[i].raw.pointerto].pointerof=i+1;
        *haspointer = TRUE;
        raw_options[sdef->elements[i].raw.pointerbase].ptrbase = TRUE;
      }
    }
    if (sdef->kind == RECORD && *hascrosstag) {
      /* looking for fields that require delayed decoding because of
	 forward references in CROSSTAG */
      for (i = 0; i < sdef->nElements; i++) {
        int j;
	/* we are looking for a field index that is greater than i */
	size_t max_index = i;
	if (!sdef->elements[i].hasRaw) continue;
	for (j = 0; j < sdef->elements[i].raw.crosstaglist.nElements; j++) {
	  int k;
	  rawAST_coding_taglist *crosstag =
	    sdef->elements[i].raw.crosstaglist.list + j;
	  for (k = 0; k < crosstag->nElements; k++) {
	    rawAST_coding_field_list *keyid = crosstag->fields + k;
	    if (keyid->nElements >= 1) {
	      int field_index = keyid->fields[0].nthfield;
	      if (field_index > max_index) max_index = field_index;
	    }
	  }
	}
	if (max_index > i) {
	  raw_options[i].delayed_decode = TRUE;
	  raw_options[max_index].nof_dependent_fields++;
	  raw_options[max_index].dependent_fields = (int*)
	    Realloc(raw_options[max_index].dependent_fields,
	      raw_options[max_index].nof_dependent_fields *
	      sizeof(*raw_options[max_index].dependent_fields));
	  raw_options[max_index].dependent_fields[
	    raw_options[max_index].nof_dependent_fields - 1] = i;
	}
      }
    }
}

char* generate_raw_coding(char* src,
  const struct_def *sdef, struct raw_option_struct *raw_options,
    boolean haspointer, boolean hascrosstag, boolean has_ext_bit)
{
  int i;
  const char *name = sdef->name;

  if (sdef->kind == SET) { /* set decoder start */
    size_t mand_num = 0;
    for (i = 0; i < sdef->nElements; i++) {
      if (!sdef->elements[i].isOptional) mand_num++;
    }
    src = mputprintf(src, "int %s::RAW_decode(const TTCN_Typedescriptor_t& "
      "p_td, TTCN_Buffer& p_buf, int limit, raw_order_t top_bit_ord, "
      "boolean, int, boolean)\n"
      "{\n"
        "int prepaddlength = p_buf.increase_pos_padd(p_td.raw->prepadding);\n"
        "limit -= prepaddlength;\n"
        "int decoded_length = 0;\n"
        "int field_map[%lu];\n"
        "memset(field_map, 0, sizeof(field_map));\n",
        name, (unsigned long) sdef->nElements);
    if (mand_num > 0)
      src = mputstr(src, "size_t nof_mand_fields = 0;\n");
    for (i = 0; i < sdef->nElements; i++) {
      if (sdef->elements[i].isOptional)
        src = mputprintf(src, "field_%s = OMIT_VALUE;\n",
                         sdef->elements[i].name);
    }
      src = mputstr(src,
	"raw_order_t local_top_order;\n"
      "if (p_td.raw->top_bit_order == TOP_BIT_INHERITED) "
	"local_top_order = top_bit_ord;\n"
      "else if (p_td.raw->top_bit_order == TOP_BIT_RIGHT) "
	"local_top_order = ORDER_MSB;\n"
      "else local_top_order = ORDER_LSB;\n"
      "while (limit > 0) {\n"
      "size_t fl_start_pos = p_buf.get_pos_bit();\n"
      );
      for (i = 0; i < sdef->nElements; i++) { /* decoding fields with tag */
	if (raw_options[i].tag_type &&
	    sdef->raw.taglist.list[raw_options[i].tag_type - 1].nElements > 0) {
	  rawAST_coding_taglist* cur_choice =
	    sdef->raw.taglist.list + raw_options[i].tag_type - 1;
	  size_t j;
	  boolean has_fixed = FALSE, has_variable = FALSE, flag_needed = FALSE;
	  for (j = 0; j < cur_choice->nElements; j++) {
	    if (cur_choice->fields[j].start_pos >= 0) {
	      if (has_fixed || has_variable) flag_needed = TRUE;
	      has_fixed = TRUE;
	    } else {
	      if (has_fixed) flag_needed = TRUE;
	      has_variable = TRUE;
	    }
	    if (has_fixed && has_variable) break;
	  }
	  src = mputprintf(src, "if (field_map[%lu] == 0) {\n",
            (unsigned long) i);
	  if (flag_needed)
	    src = mputstr(src, "boolean already_failed = FALSE;\n");
	  if (has_fixed) {
	    /* first check the fields we can precode
	     * try to decode those key variables whose position we know
	     * this way we might be able to step over bad values faster
	     */
	    boolean first_fixed = TRUE;
	    src = mputstr(src, "raw_order_t temporal_top_order;\n"
	      "int temporal_decoded_length;\n");
	    for (j = 0; j < cur_choice->nElements; j++) {
	      size_t k;
	      rawAST_coding_field_list *cur_field_list = cur_choice->fields + j;
	      if (cur_field_list->start_pos < 0) continue;
	      if (!first_fixed) src = mputstr(src, "if (!already_failed) {\n");
	      for (k = cur_field_list->nElements - 1; k > 0; k--) {
		src = mputprintf(src, "if (%s_descr_.raw->top_bit_order == "
		  "TOP_BIT_RIGHT) temporal_top_order = ORDER_MSB;\n"
		"else if (%s_descr_.raw->top_bit_order == TOP_BIT_LEFT) "
		  "temporal_top_order = ORDER_LSB;\n"
		"else ", cur_field_list->fields[k - 1].typedescr,
		cur_field_list->fields[k - 1].typedescr);
	      }
	      src = mputprintf(src, "temporal_top_order = top_bit_ord;\n"
		"%s temporal_%lu;\n"
		"p_buf.set_pos_bit(fl_start_pos + %d);\n"
		"temporal_decoded_length = temporal_%lu.RAW_decode(%s_descr_, "
		  "p_buf, limit, temporal_top_order, TRUE);\n"
		"p_buf.set_pos_bit(fl_start_pos);\n"
		"if (temporal_decoded_length > 0 && temporal_%lu == %s) {\n"
		"int decoded_field_length = field_%s%s.RAW_decode(%s_descr_, "
		  "p_buf, limit, local_top_order, TRUE);\n"
		"if (decoded_field_length %s 0 && (",
		cur_field_list->fields[cur_field_list->nElements - 1].type,
		(unsigned long) j, cur_field_list->start_pos, (unsigned long) j,
		cur_field_list->fields[cur_field_list->nElements - 1].typedescr,
		(unsigned long) j, cur_field_list->value,
                sdef->elements[i].name,
		sdef->elements[i].isOptional ? "()" : "",
		sdef->elements[i].typedescrname,
		sdef->elements[i].isOptional ? ">" : ">=");
	      src = genRawFieldChecker(src, cur_choice, TRUE);
	      src = mputstr(src, ")) {\n"
		"decoded_length += decoded_field_length;\n"
		"limit -= decoded_field_length;\n");
	      if (!sdef->elements[i].isOptional)
		src = mputstr(src, "nof_mand_fields++;\n");
	      src = mputprintf(src, "field_map[%lu] = 1;\n"
		"continue;\n"
		"} else {\n"
		"p_buf.set_pos_bit(fl_start_pos);\n", (unsigned long) i);
	      if (sdef->elements[i].isOptional)
		src = mputprintf(src, "field_%s = OMIT_VALUE;\n",
		  sdef->elements[i].name);
	      if (flag_needed) src = mputstr(src, "already_failed = TRUE;\n");
	      src = mputstr(src, "}\n"
		"}\n");
	      if (first_fixed) first_fixed = FALSE;
	      else src = mputstr(src, "}\n");
	    }
	  }
	  if (has_variable) {
	    /* if there is one tag key whose position we don't know
	     * and we couldn't decide yet if element can be decoded or not
	     * than we have to decode it.
	     */
	    if (flag_needed) src = mputstr(src, "if (!already_failed) {\n");
	    src = mputprintf(src, "int decoded_field_length = "
		"field_%s%s.RAW_decode(%s_descr_, p_buf, limit, "
		"local_top_order, TRUE);\n"
	      "if (decoded_field_length %s 0 && (",
	      sdef->elements[i].name, sdef->elements[i].isOptional ? "()" : "",
	      sdef->elements[i].typedescrname,
	      sdef->elements[i].isOptional ? ">" : ">=");
	    src = genRawFieldChecker(src, cur_choice, TRUE);
	    src = mputstr(src, ")) {\n"
	      "decoded_length += decoded_field_length;\n"
	      "limit -= decoded_field_length;\n");
	    if (!sdef->elements[i].isOptional)
	      src = mputstr(src, "nof_mand_fields++;\n");
	    src = mputprintf(src, "field_map[%lu] = 1;\n"
	      "continue;\n"
	      "} else {\n"
	      "p_buf.set_pos_bit(fl_start_pos);\n", (unsigned long) i);
	    if (sdef->elements[i].isOptional)
	      src = mputprintf(src, "field_%s = OMIT_VALUE;\n",
		sdef->elements[i].name);
	    src = mputstr(src, "}\n");
	    if (flag_needed) src = mputstr(src, "}\n");
	  }
	  src = mputstr(src, "}\n");
	}
      }
      for (i = 0; i < sdef->nElements; i++) {
	/* decoding fields without TAG */
	if (!raw_options[i].tag_type) {
	  boolean repeatable;
	  if (sdef->elements[i].of_type && sdef->elements[i].hasRaw &&
	      sdef->elements[i].raw.repeatable == XDEFYES) repeatable = TRUE;
	  else {
	    repeatable = FALSE;
	    src = mputprintf(src, "if (field_map[%lu] == 0) ",
              (unsigned long) i);
	  }
	  src = mputprintf(src, "{\n"
	    "int decoded_field_length = field_%s%s.RAW_decode(%s_descr_, "
	      "p_buf, limit, local_top_order, TRUE",
	    sdef->elements[i].name, sdef->elements[i].isOptional ? "()" : "",
	    sdef->elements[i].typedescrname);
	  if (repeatable)
            src = mputprintf(src, ", -1, field_map[%lu] == 0",
              (unsigned long) i);
	  src = mputprintf(src, ");\n"
	    "if (decoded_field_length %s 0) {\n"
	    "decoded_length += decoded_field_length;\n"
	    "limit -= decoded_field_length;\n",
	    sdef->elements[i].isOptional ? ">" : ">=");
	  if (repeatable) {
	    if (!sdef->elements[i].isOptional) src = mputprintf(src,
	      "if (field_map[%lu] == 0) nof_mand_fields++;\n",
              (unsigned long) i);
	    src = mputprintf(src, "field_map[%lu]++;\n", (unsigned long) i);
	  } else {
	    if (!sdef->elements[i].isOptional)
	      src = mputstr(src, "nof_mand_fields++;\n");
	    src = mputprintf(src, "field_map[%lu] = 1;\n", (unsigned long) i);
	  }
	  src = mputstr(src, "continue;\n"
	    "} else {\n"
	    "p_buf.set_pos_bit(fl_start_pos);\n");
	  if (sdef->elements[i].isOptional) {
	    if (repeatable)
	      src = mputprintf(src, "if (field_map[%lu] == 0) ",
                (unsigned long) i);
	    src = mputprintf(src, "field_%s = OMIT_VALUE;\n",
	      sdef->elements[i].name);
	  }
	  src = mputstr(src, "}\n"
	    "}\n");
	}
      }
      for (i = 0; i < sdef->nElements; i++){
	/* decoding fields with tag OTHERWISE */
	if (raw_options[i].tag_type &&
	    sdef->raw.taglist.list[raw_options[i].tag_type-1].nElements == 0) {
	  src = mputprintf(src, "if (field_map[%lu] == 0) {\n"
	    "int decoded_field_length = field_%s%s.RAW_decode(%s_descr_, "
	      "p_buf, limit, local_top_order, TRUE);\n"
	    "if (decoded_field_length %s 0) {\n"
	    "decoded_length += decoded_field_length;\n"
	    "limit -= decoded_field_length;\n", (unsigned long) i,
	    sdef->elements[i].name, sdef->elements[i].isOptional ? "()" : "",
	    sdef->elements[i].typedescrname,
	    sdef->elements[i].isOptional ? ">" : ">=");
	  if (!sdef->elements[i].isOptional)
	    src = mputstr(src, "nof_mand_fields++;\n");
	  src = mputprintf(src, "field_map[%lu] = 1;\n"
	    "continue;\n"
	    "} else {\n"
	    "p_buf.set_pos_bit(fl_start_pos);\n", (unsigned long) i);
	  if (sdef->elements[i].isOptional)
	    src = mputprintf(src, "field_%s = OMIT_VALUE;\n",
	      sdef->elements[i].name);
	  src = mputstr(src, "}\n"
	    "}\n");
	}
      }
      src = mputstr(src, "break;\n"  /* no field decoded successfully, quit */
        "}\n");
      if (mand_num > 0) src = mputprintf(src,
        "if (nof_mand_fields != %lu) return limit ? -1 : -TTCN_EncDec::ET_INCOMPL_MSG;\n",
        (unsigned long) mand_num);
      /* If not all required fields were decoded and there are no bits left,
       * that means that the last field was decoded successfully but used up
       * the buffer. Signal "incomplete". If there were bits left, that means that
       * no field could be decoded from them; signal an error.
       */
      src = mputstr(src, "return decoded_length + prepaddlength + "
	"p_buf.increase_pos_padd(p_td.raw->padding);\n"
      "}\n\n");
  } else {
    /* set decoder end, record decoder start */
    int prev_ext_group = 0;
    src = mputprintf(src,
      "int %s::RAW_decode(const TTCN_Typedescriptor_t& p_td, "
      "TTCN_Buffer& p_buf, int limit, raw_order_t top_bit_ord, boolean no_err, "
      "int, boolean)\n"
      "{ (void)no_err;\n"
	"  int prepaddlength=p_buf.increase_pos_padd(p_td.raw->prepadding);\n"
	"  limit-=prepaddlength;\n"
	"  size_t last_decoded_pos = p_buf.get_pos_bit();\n"
	"  int decoded_length = 0;\n"
	"  int decoded_field_length = 0;\n"
	"  raw_order_t local_top_order;\n"
	, name);
      if (hascrosstag) {
	src = mputstr(src, "  int selected_field = -1;\n");
      }
      if (sdef->raw.ext_bit_goup_num) {
        src=mputstr(src, "  int group_limit = 0;\n");
      }
      src=mputstr(src,
      "  if(p_td.raw->top_bit_order==TOP_BIT_INHERITED)"
      "local_top_order=top_bit_ord;\n"
      "  else if(p_td.raw->top_bit_order==TOP_BIT_RIGHT)local_top_order"
      "=ORDER_MSB;\n"
      "  else local_top_order=ORDER_LSB;\n"
      );
      if(has_ext_bit){
        src=mputstr(src,
          "  {\n"
          "  cbyte* data=p_buf.get_read_data();\n"
          "  int count=1;\n"
          "  unsigned mask = 1 << (local_top_order==ORDER_LSB ? 0 : 7);\n"
          "  if(p_td.raw->extension_bit==EXT_BIT_YES){\n"
          "    while((data[count-1] & mask)==0 && count*8<(int)limit) count++;\n"
          "  }\n"
          "  else{\n"
          "    while((data[count-1] & mask)!=0 && count*8<(int)limit) count++;\n"
          "  }\n"
          "  if(limit) limit=count*8;\n"
          "  }\n"
        );
      }
      if(haspointer)
       src=mputstr(src,
         "  int end_of_available_data=last_decoded_pos+limit;\n");
      for(i=0;i<sdef->nElements;i++){
        if(raw_options[i].pointerof)
          src=mputprintf(src,
          "  int start_of_field%lu=-1;\n"
          ,(unsigned long) i
          );
        if(raw_options[i].ptrbase)
          src=mputprintf(src,
          "  int start_pos_of_field%lu=-1;\n"
          ,(unsigned long) i
          );
        if(raw_options[i].lengthto)
          src=mputprintf(src,
          "  int value_of_length_field%lu = 0;\n"
          ,(unsigned long) i
          );
      }
      for(i=0;i<sdef->nElements;i++){ /* decoding fields */
	if (raw_options[i].delayed_decode) {
	  int j;
	  /* checking whether there are enough bits in the buffer to decode
	     the field */
	  src = mputstr(src, "  if (");
	  src = genRawFieldDecodeLimit(src, sdef, i, raw_options);
	  src = mputprintf(src, " < %d) return -TTCN_EncDec::ET_LEN_ERR;\n",
	    sdef->elements[i].raw.length);
	  /* skipping over the field that has to be decoded later because of
	     forward referencing in CROSSTAG */
	  src = mputprintf(src,
	    "  size_t start_of_field%lu = p_buf.get_pos_bit();\n"
	    "  p_buf.set_pos_bit(start_of_field%lu + %d);\n"
	    "  decoded_length += %d;\n"
	    "  last_decoded_pos += %d;\n"
	    "  limit -= %d;\n",
	    (unsigned long) i, (unsigned long) i, sdef->elements[i].raw.length,
            sdef->elements[i].raw.length,
	    sdef->elements[i].raw.length, sdef->elements[i].raw.length);
	  for (j = 0; j < raw_options[i].lengthof; j++) {
	    src = mputprintf(src,
	      "  value_of_length_field%d -= %d;\n",
	      raw_options[i].lengthoffield[j], sdef->elements[i].raw.length);
	  }
	} else {
	  src = genRawDecodeRecordField(src, sdef, i, raw_options, FALSE,
	   &prev_ext_group);
	  if (raw_options[i].nof_dependent_fields > 0) {
	    int j;
	    for (j = 0; j < raw_options[i].nof_dependent_fields; j++) {
	      int dependent_field_index = raw_options[i].dependent_fields[j];
	      /* seek to the beginning of the dependent field */
	      src = mputprintf(src,
		"  p_buf.set_pos_bit(start_of_field%d);\n",
		dependent_field_index);
	      /* decode the dependent field */
	      src = genRawDecodeRecordField(src, sdef, dependent_field_index,
		raw_options, TRUE, &prev_ext_group);
	    }
	    if (i < sdef->nElements - 1) {
	      /* seek back if there are more regular fields to decode */
	      src = mputstr(src,
		"  p_buf.set_pos_bit(last_decoded_pos);\n");
	    }
	  }
	}
      } /* decoding fields*/

      if(sdef->hasRaw && sdef->raw.presence.nElements > 0)
      {
	src = mputstr(src, "  if (");
	src = genRawFieldChecker(src, &sdef->raw.presence, FALSE);
	src = mputstr(src, ") return -1;\n");
      }

      src=mputstr(src,
      "  p_buf.set_pos_bit(last_decoded_pos);\n"
      "  return decoded_length+prepaddlength+"
      "p_buf.increase_pos_padd(p_td.raw->padding);\n}\n\n");
  } /* record decoder end */

  src = mputprintf(src, /* encoder */
    "int %s::RAW_encode(const TTCN_Typedescriptor_t&%s, "
    "RAW_enc_tree& myleaf) const {\n", name,
    use_runtime_2 ? " p_td" : "");
  if (use_runtime_2) {
    src = mputstr(src, "  if (err_descr) return RAW_encode_negtest(err_descr, p_td, myleaf);\n");
  }
  src = mputprintf(src,
    "  if (!is_bound()) TTCN_EncDec_ErrorContext::error"
    "(TTCN_EncDec::ET_UNBOUND, \"Encoding an unbound value.\");\n"
    "  int encoded_length = 0;\n"
    "  myleaf.isleaf = FALSE;\n"
    "  myleaf.body.node.num_of_nodes = %lu;\n"
    "  myleaf.body.node.nodes = init_nodes_of_enc_tree(%lu);\n",
    (unsigned long)sdef->nElements, (unsigned long)sdef->nElements);
  /* init nodes */
  for (i = 0; i < sdef->nElements; i++) {
    if (sdef->elements[i].isOptional) {
      src = mputprintf(src,
        "  if (field_%s.ispresent()) {\n", sdef->elements[i].name);
    }
    src = mputprintf(src,
      "  myleaf.body.node.nodes[%lu] = new RAW_enc_tree(TRUE, &myleaf, "
      "&(myleaf.curr_pos), %lu, %s_descr_.raw);\n",
      (unsigned long)i, (unsigned long)i, sdef->elements[i].typedescrname);
    if (sdef->elements[i].isOptional) {
      src = mputprintf(src,
        "  }\n"
        "  else myleaf.body.node.nodes[%lu] = NULL;\n", (unsigned long)i);
    }
  }
  for (i = 0; i < sdef->raw.ext_bit_goup_num; i++) {
    if (sdef->raw.ext_bit_groups[i].ext_bit != XDEFNO) {
      src = mputprintf(src,
        "  {\n"
        "   int node_idx = %d;\n"
        "   while (node_idx <= %d && myleaf.body.node.nodes[node_idx] == NULL) node_idx++;\n"
        "   if (myleaf.body.node.nodes[node_idx]) {\n"
        "     myleaf.body.node.nodes[node_idx]->ext_bit_handling = 1;\n"
        "     myleaf.body.node.nodes[node_idx]->ext_bit = %s;\n  }\n"
        "   node_idx = %d;\n"
        "   while (node_idx >= %d && myleaf.body.node.nodes[node_idx] == NULL) node_idx--;\n"
        "   if (myleaf.body.node.nodes[node_idx]) myleaf.body.node.nodes[node_idx]"
        "->ext_bit_handling += 2;\n"
        "  }\n",
        sdef->raw.ext_bit_groups[i].from,
        sdef->raw.ext_bit_groups[i].to,
        sdef->raw.ext_bit_groups[i].ext_bit == XDEFYES ? "EXT_BIT_YES" : "EXT_BIT_REVERSE",
        sdef->raw.ext_bit_groups[i].to,
        sdef->raw.ext_bit_groups[i].from);
    }
  }
  for (i = 0; i < sdef->nElements; i++) {
    /* encoding fields */
    if (sdef->elements[i].isOptional) {
      src = mputprintf(src,
        "  if (field_%s.ispresent()) {\n", sdef->elements[i].name);
    }
    if (raw_options[i].lengthto && sdef->elements[i].raw.lengthindex == NULL
        && sdef->elements[i].raw.union_member_num == 0) {
      /* encoding of lenghto fields */
      int a;
      src = mputprintf(src,
        "  encoded_length += %d;\n"
        "  myleaf.body.node.nodes[%lu]->calc = CALC_LENGTH;\n"
        "  myleaf.body.node.nodes[%lu]->coding_descr = &%s_descr_;\n"
        "  myleaf.body.node.nodes[%lu]->calcof.lengthto.num_of_fields = %d;\n"
        "  myleaf.body.node.nodes[%lu]->calcof.lengthto.unit = %d;\n"
        "  myleaf.body.node.nodes[%lu]->calcof.lengthto.fields = "
        "init_lengthto_fields_list(%d);\n"
        "  myleaf.body.node.nodes[%lu]->length = %d;\n",
        sdef->elements[i].raw.fieldlength, (unsigned long)i,
        (unsigned long)i, sdef->elements[i].typedescrname,
        (unsigned long)i, sdef->elements[i].raw.lengthto_num,
        (unsigned long)i, sdef->elements[i].raw.unit,
        (unsigned long)i, sdef->elements[i].raw.lengthto_num,
        (unsigned long)i, sdef->elements[i].raw.fieldlength);
      for (a = 0; a < sdef->elements[i].raw.lengthto_num; a++) {
        if (sdef->elements[sdef->elements[i].raw.lengthto[a]].isOptional) {
          src = mputprintf(src,
            "  if (field_%s.ispresent()) {\n",
            sdef->elements[sdef->elements[i].raw.lengthto[a]].name);
        }
        src = mputprintf(src,
          "  myleaf.body.node.nodes[%lu]->calcof.lengthto.fields[%d].level = "
          "myleaf.body.node.nodes[%d]->curr_pos.level;\n"
          "  myleaf.body.node.nodes[%lu]->calcof.lengthto.fields[%d].pos = "
          "myleaf.body.node.nodes[%d]->curr_pos.pos;\n",
          (unsigned long)i, a, sdef->elements[i].raw.lengthto[a],
          (unsigned long)i, a, sdef->elements[i].raw.lengthto[a]);
        if (sdef->elements[sdef->elements[i].raw.lengthto[a]].isOptional) {
          src = mputprintf(src,
            "  } else {\n"
            "  myleaf.body.node.nodes[%lu]->calcof.lengthto.fields[%d].level = 0;\n"
            "  myleaf.body.node.nodes[%lu]->calcof.lengthto.fields[%d].pos = 0;\n"
            "  }\n", (unsigned long)i, a, (unsigned long)i, a);
        }
      }
    } else if (raw_options[i].pointerto) {
      /* encoding of pointerto fields */
      if (sdef->elements[sdef->elements[i].raw.pointerto].isOptional) {
        src = mputprintf(src,
          "  if (field_%s.ispresent()) {\n",
          sdef->elements[sdef->elements[i].raw.pointerto].name);
      }
      src = mputprintf(src,
        "  encoded_length += %d;\n"
        "  myleaf.body.node.nodes[%lu]->calc = CALC_POINTER;\n"
        "  myleaf.body.node.nodes[%lu]->coding_descr = &%s_descr_;\n"
        "  myleaf.body.node.nodes[%lu]->calcof.pointerto.unit = %d;\n"
        "  myleaf.body.node.nodes[%lu]->calcof.pointerto.ptr_offset = %d;\n"
        "  myleaf.body.node.nodes[%lu]->calcof.pointerto.ptr_base = %d;\n"
        "  myleaf.body.node.nodes[%lu]->calcof.pointerto.target.level = "
        "myleaf.body.node.nodes[%d]->curr_pos.level;\n"
        "  myleaf.body.node.nodes[%lu]->calcof.pointerto.target.pos = "
        "myleaf.body.node.nodes[%d]->curr_pos.pos;\n"
        "  myleaf.body.node.nodes[%lu]->length = %d;\n",
        sdef->elements[i].raw.fieldlength,(unsigned long)i,
        (unsigned long)i, sdef->elements[i].typedescrname,
        (unsigned long)i, sdef->elements[i].raw.unit,
        (unsigned long)i, sdef->elements[i].raw.ptroffset,
        (unsigned long)i, sdef->elements[i].raw.pointerbase,
        (unsigned long)i, sdef->elements[i].raw.pointerto,
        (unsigned long)i, sdef->elements[i].raw.pointerto,
        (unsigned long)i, sdef->elements[i].raw.fieldlength);
      if (sdef->elements[sdef->elements[i].raw.pointerto].isOptional) {
        src = mputprintf(src,
          "  } else {\n"
          "    INTEGER atm;\n"
          "    atm = 0;\n"
          "    encoded_length += atm.RAW_encode(%s_descr_"
          ", *myleaf.body.node.nodes[%lu]);\n"
          "  }\n",
          sdef->elements[i].typedescrname, (unsigned long)i);
      }
    } else {
      /* encoding of normal fields */
      src = mputprintf(src,
        "  encoded_length += field_%s%s.RAW_encode(%s_descr_"
        ", *myleaf.body.node.nodes[%lu]);\n",
        sdef->elements[i].name, sdef->elements[i].isOptional ? "()" : "",
        sdef->elements[i].typedescrname, (unsigned long)i);
    }
    if (sdef->elements[i].isOptional) {
      src = mputstr(src, "  }\n");
    }
  }
  for (i = 0; i < sdef->nElements; i++) {
    /* fill presence, tag and crosstag */
    if (raw_options[i].lengthto && sdef->elements[i].raw.lengthindex) {
      /* encoding of lenghto fields */
      int a;
      if (sdef->elements[i].isOptional) {
        src = mputprintf(src,
          "  if (field_%s.ispresent()) {\n",
          sdef->elements[i].name);
      }
      src = mputprintf(src,
        "  if (myleaf.body.node.nodes[%lu]->body.node.nodes[%d]) {\n"
        "  myleaf.body.node.nodes[%lu]->body.node.nodes[%d]->"
        "calc = CALC_LENGTH;\n"
        "  myleaf.body.node.nodes[%lu]->body.node.nodes[%d]->"
        "coding_descr = &%s_descr_;\n"
        "  myleaf.body.node.nodes[%lu]->body.node.nodes[%d]->"
        "calcof.lengthto.num_of_fields = %d;\n"
        "  myleaf.body.node.nodes[%lu]->body.node.nodes[%d]->"
        "calcof.lengthto.unit = %d;\n"
        "  myleaf.body.node.nodes[%lu]->body.node.nodes[%d]->"
        "calcof.lengthto.fields = "
        "init_lengthto_fields_list(%d);\n",
        (unsigned long)i, sdef->elements[i].raw.lengthindex->nthfield,
        (unsigned long)i, sdef->elements[i].raw.lengthindex->nthfield,
        (unsigned long)i, sdef->elements[i].raw.lengthindex->nthfield,
        sdef->elements[i].raw.lengthindex->typedescr,
        (unsigned long)i, sdef->elements[i].raw.lengthindex->nthfield,
        sdef->elements[i].raw.lengthto_num,
        (unsigned long)i, sdef->elements[i].raw.lengthindex->nthfield,
        sdef->elements[i].raw.unit,
        (unsigned long)i, sdef->elements[i].raw.lengthindex->nthfield,
        sdef->elements[i].raw.lengthto_num);
      for (a = 0; a < sdef->elements[i].raw.lengthto_num; a++) {
        if (sdef->elements[sdef->elements[i].raw.lengthto[a]].isOptional) {
          src = mputprintf(src,
            "  if (field_%s.ispresent()) {\n",
            sdef->elements[sdef->elements[i].raw.lengthto[a]].name);
        }
        src = mputprintf(src,
          "  myleaf.body.node.nodes[%lu]->body.node.nodes[%d]"
          "->calcof.lengthto.fields[%d].level = "
          "myleaf.body.node.nodes[%d]->curr_pos.level;\n"
          "  myleaf.body.node.nodes[%lu]->body.node.nodes[%d]"
          "->calcof.lengthto.fields[%d].pos = "
          "myleaf.body.node.nodes[%d]->curr_pos.pos;\n",
          (unsigned long)i,sdef->elements[i].raw.lengthindex->nthfield,
          a, sdef->elements[i].raw.lengthto[a],
          (unsigned long)i,sdef->elements[i].raw.lengthindex->nthfield,
          a, sdef->elements[i].raw.lengthto[a]);
        if (sdef->elements[sdef->elements[i].raw.lengthto[a]].isOptional) {
           src = mputprintf(src,
           "  } else {\n"
           "  myleaf.body.node.nodes[%lu]->body.node.nodes[%d]->"
           "calcof.lengthto.fields[%d].level = 0;\n"
           "  myleaf.body.node.nodes[%lu]->body.node.nodes[%d]->"
           "calcof.lengthto.fields[%d].pos = 0;\n"
           "  }\n",
           (unsigned long)i, sdef->elements[i].raw.lengthindex->nthfield, a,
           (unsigned long)i, sdef->elements[i].raw.lengthindex->nthfield, a);
        }
      }
      src = mputstr(src, "  }\n");
      if (sdef->elements[i].isOptional) {
        src = mputstr(src, "  }\n");
      }
    }
    if (raw_options[i].lengthto && sdef->elements[i].raw.union_member_num) {
      /* encoding of lenghto fields */
      int a;
      if (sdef->elements[i].isOptional) {
        src = mputprintf(src,
          "  if (field_%s.ispresent()) ", sdef->elements[i].name);
      }

      src = mputprintf(src,
        "  {\n"
        "  int sel_field = 0;\n"
        "  while (myleaf.body.node.nodes[%lu]->body.node.nodes[sel_field] == NULL) "
        "{ sel_field++; }\n"
        "  myleaf.body.node.nodes[%lu]->body.node.nodes[sel_field]->"
        "calc = CALC_LENGTH;\n"
        "  myleaf.body.node.nodes[%lu]->body.node.nodes[sel_field]->"
        "calcof.lengthto.num_of_fields = %d;\n"
        "  myleaf.body.node.nodes[%lu]->body.node.nodes[sel_field]->"
        "calcof.lengthto.unit = %d;\n"
        "  myleaf.body.node.nodes[%lu]->body.node.nodes[sel_field]->"
        "calcof.lengthto.fields = init_lengthto_fields_list(%d);\n",
        (unsigned long)i,(unsigned long)i,
        (unsigned long)i,sdef->elements[i].raw.lengthto_num,
        (unsigned long)i,sdef->elements[i].raw.unit,
        (unsigned long)i,sdef->elements[i].raw.lengthto_num);
      for (a = 0; a < sdef->elements[i].raw.lengthto_num; a++) {
        if (sdef->elements[sdef->elements[i].raw.lengthto[a]].isOptional) {
          src = mputprintf(src,
            "  if (field_%s.ispresent()) {\n",
            sdef->elements[sdef->elements[i].raw.lengthto[a]].name);
        }
        src = mputprintf(src,
          "  myleaf.body.node.nodes[%lu]->body.node.nodes[sel_field]"
          "->calcof.lengthto.fields[%d].level = "
          "myleaf.body.node.nodes[%d]->curr_pos.level;\n"
          "  myleaf.body.node.nodes[%lu]->body.node.nodes[sel_field]"
          "->calcof.lengthto.fields[%d].pos = "
          "myleaf.body.node.nodes[%d]->curr_pos.pos;\n",
          (unsigned long)i, a, sdef->elements[i].raw.lengthto[a],
          (unsigned long)i, a, sdef->elements[i].raw.lengthto[a]);
        if (sdef->elements[sdef->elements[i].raw.lengthto[a]].isOptional) {
          src = mputprintf(src,
            "  }else{\n"
            "  myleaf.body.node.nodes[%lu]->body.node.nodes[sel_field]"
            "->calcof.lengthto.fields[%d].level = 0;\n"
            "  myleaf.body.node.nodes[%lu]->body.node.nodes[sel_field]"
            "->calcof.lengthto.fields[%d].pos = 0;\n"
            "  }\n",
            (unsigned long)i, a,
            (unsigned long)i, a);
        }
      }
      src = mputstr(src,
        "  }\n");
    }
    if (raw_options[i].tag_type && sdef->raw.taglist.list[raw_options[i].tag_type - 1].nElements) {
      /* tag */
      rawAST_coding_taglist *cur_choice =
        sdef->raw.taglist.list + raw_options[i].tag_type - 1;
      src = mputstr(src, "  if (");
      if (sdef->elements[i].isOptional) {
        src = mputprintf(src, "field_%s.ispresent() && (", sdef->elements[i].name);
      }
      src = genRawFieldChecker(src, cur_choice, FALSE);
      if (sdef->elements[i].isOptional) src = mputstr(src, ")");
      src = mputstr(src,") {\n");
      src = genRawTagChecker(src, cur_choice);
      src=mputstr(src,"  }\n");
    }
    if (sdef->elements[i].hasRaw && sdef->elements[i].raw.presence.nElements) {
      /* presence */
      src = mputstr(src, "  if (");
      if (sdef->elements[i].isOptional) {
        src = mputprintf(src, "field_%s.ispresent() && (", sdef->elements[i].name);
      }
      src = genRawFieldChecker(src, &sdef->elements[i].raw.presence, FALSE);
      if (sdef->elements[i].isOptional) src = mputstr(src, ")");
      src = mputstr(src, ") {\n");
      src = genRawTagChecker(src, &sdef->elements[i].raw.presence);
      src = mputstr(src,"  }\n");
    }
    if (sdef->elements[i].hasRaw &&
        sdef->elements[i].raw.crosstaglist.nElements) {
      /* crosstag */
      int a;
      if (sdef->elements[i].isOptional) {
        src = mputprintf(src,
          "  if (field_%s.ispresent()) {\n",
          sdef->elements[i].name);
      }
      src = mputprintf(src,
        "  switch (field_%s%s.get_selection()) {\n",
        sdef->elements[i].name,sdef->elements[i].isOptional ? "()" : "");
      for (a = 0; a < sdef->elements[i].raw.crosstaglist.nElements; a++) {
        rawAST_coding_taglist *cur_choice =
	  sdef->elements[i].raw.crosstaglist.list + a;
        if (cur_choice->nElements > 0) {
	    src = mputprintf(src, "  case %s%s%s:\n"
	      "  if (", sdef->elements[i].type,
	      "::ALT_", cur_choice->fieldName);
	    src = genRawFieldChecker(src, cur_choice, FALSE);
	    src = mputstr(src, ") {\n");
	    if (!strcmp(cur_choice->fields[0].value, "OMIT_VALUE")) {
	      if (cur_choice->fields[0].nElements != 1)
		NOTSUPP("omit value with multiple fields in CROSSTAG");
	      /* eliminating the corresponding encoded leaf, which will have
	         the same effect as if the referred field had been omit */
	      src = mputprintf(src,
		"  encoded_length -= myleaf.body.node.nodes[%d]->length;\n"
		"  delete myleaf.body.node.nodes[%d];\n"
		"  myleaf.body.node.nodes[%d] = NULL;\n",
		cur_choice->fields[0].fields[0].nthfield,
		cur_choice->fields[0].fields[0].nthfield,
		cur_choice->fields[0].fields[0].nthfield);
	    } else {
	      int ll;
	      src = mputprintf(src,
		"  RAW_enc_tr_pos pr_pos;\n"
		"  pr_pos.level = myleaf.curr_pos.level + %d;\n"
		"  int new_pos[] = { ",
		cur_choice->fields[0].nElements);
              for (ll = 0; ll < cur_choice->fields[0].nElements; ll++) {
		if (ll > 0) src = mputstr(src, ", ");
		src = mputprintf(src, "%d",
		  cur_choice->fields[0].fields[ll].nthfield);
              }
	      src = mputprintf(src, " };\n"
		"  pr_pos.pos = init_new_tree_pos(myleaf.curr_pos, %d, "
		  "new_pos);\n",
		cur_choice->fields[0].nElements);
              if (cur_choice->fields[0].value[0] == ' ') {
	        /* the value is a string literal (the encoder can be called
		   on that object directly */
		src = mputprintf(src,
		  "  RAW_enc_tree* temp_leaf = myleaf.get_node(pr_pos);\n"
                  "  if (temp_leaf != NULL)\n"
                  "    %s.RAW_encode(%s_descr_,*temp_leaf);\n"
                  "  else\n"
                  "    TTCN_EncDec_ErrorContext::error\n"
                  "      (TTCN_EncDec::ET_OMITTED_TAG, \"Encoding a tagged,"
                  " but omitted value.\");\n",
		  cur_choice->fields[0].value,
		  cur_choice->fields[0].fields[
		    cur_choice->fields[0].nElements - 1].typedescr);
	      } else {
	        /* a temporary object needs to be created for encoding */
		src = mputprintf(src,
		  "  %s new_val(%s);\n"
                  "  RAW_enc_tree* temp_leaf = myleaf.get_node(pr_pos);\n"
                  "  if (temp_leaf != NULL)\n"
                  "    new_val.RAW_encode(%s_descr_,*temp_leaf);\n"
                  "  else\n"
                  "    TTCN_EncDec_ErrorContext::error\n"
                  "      (TTCN_EncDec::ET_OMITTED_TAG, \"Encoding a tagged,"
                  " but omitted value.\");\n",
		  cur_choice->fields[0].fields[
		    cur_choice->fields[0].nElements - 1].type,
		  cur_choice->fields[0].value,
		  cur_choice->fields[0].fields[
		    cur_choice->fields[0].nElements - 1].typedescr);
              }
              src = mputstr(src, "  free_tree_pos(pr_pos.pos);\n");
	    }
	    src = mputstr(src, "  }\n"
	      "  break;\n");
          }
        }
        src = mputstr(src, "  default:;\n"
          "  }\n");
        if (sdef->elements[i].isOptional) src = mputstr(src, "  }\n");
      }
    }
  /* presence */
  if (sdef->hasRaw && sdef->raw.presence.nElements > 0) {
    src = mputstr(src, "  if (");
    src = genRawFieldChecker(src, &sdef->raw.presence, FALSE);
    src = mputstr(src,") {\n");
    src = genRawTagChecker(src, &sdef->raw.presence);
    src = mputstr(src,"  }\n");
  }
  src = mputstr(src, "  return myleaf.length = encoded_length;\n}\n\n");

  if (use_runtime_2)
    src = generate_raw_coding_negtest(src, sdef, raw_options);
  return src;
}

char *generate_raw_coding_negtest(char *src, const struct_def *sdef,
  struct raw_option_struct *raw_options)
{
  int i;
  const char *name = sdef->name;
  src = mputprintf(src,
    /* Count the nodes and discover the new node order only.  No node creation
       or encoding.  */
    "int %s::RAW_encode_negtest(const Erroneous_descriptor_t *p_err_descr, "
    "const TTCN_Typedescriptor_t& /*p_td*/, RAW_enc_tree& myleaf) const\n"
    "{\n"
    "  if (!is_bound()) TTCN_EncDec_ErrorContext::error"
    "(TTCN_EncDec::ET_UNBOUND, \"Encoding an unbound value.\");\n"
    "  int idx_map[%lu];\n"
    "  for (int idx_map_idx = 0; idx_map_idx < %lu; ++idx_map_idx)\n"
    "    idx_map[idx_map_idx] = -1;\n"
    "  (void)idx_map;\n"
    "  int encoded_length = 0;\n"
    "  int num_fields = get_count();\n"
    "  myleaf.isleaf = FALSE;\n"
    "  myleaf.body.node.num_of_nodes = 0;\n"
    "  for (int field_idx = 0; field_idx < num_fields; ++field_idx) {\n"
    "    if ((p_err_descr->omit_before != -1) &&\n"
    "        (field_idx < p_err_descr->omit_before))\n"
    "      continue;\n"
    "    const Erroneous_values_t *err_vals = p_err_descr->get_field_err_values(field_idx);\n"
    "    const Erroneous_descriptor_t *emb_descr = p_err_descr->get_field_emb_descr(field_idx);\n"
    "    if (err_vals && err_vals->before)\n"
    "      ++myleaf.body.node.num_of_nodes;\n"
    "    if (err_vals && err_vals->value) {\n"
    "      if (err_vals->value->errval) {\n"
    "        // Field is modified, but it's still there.  Otherwise, it's\n"
    "        // initialized to `-1'.\n"
    "        idx_map[field_idx] = -2;\n"
    "        ++myleaf.body.node.num_of_nodes;\n"
    "      }\n"
    "    } else {\n"
    "      if (emb_descr) idx_map[field_idx] = -2;\n"
    "      else idx_map[field_idx] = myleaf.body.node.num_of_nodes;\n"
    "      ++myleaf.body.node.num_of_nodes;\n"
    "    }\n"
    "    if (err_vals && err_vals->after)\n"
    "      ++myleaf.body.node.num_of_nodes;\n"
    "    if ((p_err_descr->omit_after != -1) &&\n"
    "        (field_idx >= p_err_descr->omit_after))\n"
    "      break;\n"
    "  }\n",
    name, (unsigned long)sdef->nElements, (unsigned long)sdef->nElements);
  /* Init nodes.  */
  src = mputprintf(src,
    "  myleaf.body.node.nodes =\n"
    "    init_nodes_of_enc_tree(myleaf.body.node.num_of_nodes);\n"
    "  TTCN_EncDec_ErrorContext e_c;\n"
    "  int node_pos = 0;\n"
    "  int next_optional_idx = 0;\n"
    "  const int *my_optional_indexes = get_optional_indexes();\n"
    "  for (int field_idx = 0; field_idx < num_fields; ++field_idx) {\n"
    "    boolean is_optional_field = my_optional_indexes &&\n"
    "      (my_optional_indexes[next_optional_idx] == field_idx);\n"
    "    if ((p_err_descr->omit_before != -1) &&\n"
    "        (field_idx < p_err_descr->omit_before)) {\n"
    "      if (is_optional_field) ++next_optional_idx;\n"
    "      continue;\n"
    "    }\n"
    "    const Erroneous_values_t *err_vals =\n"
    "      p_err_descr->get_field_err_values(field_idx);\n"
    "    if (err_vals && err_vals->before) {\n"
    "      if (err_vals->before->errval == NULL)\n"
    "        TTCN_error(\"internal error: erroneous before value missing\");\n"
    "      if (err_vals->before->raw) {\n"
    "        myleaf.body.node.nodes[node_pos] =\n"
    "          new RAW_enc_tree(TRUE, &myleaf, &(myleaf.curr_pos),\n"
    "                           node_pos, err_vals->before->errval"
    "->get_descriptor()->raw);\n"
    "      } else {\n"
    "        if (err_vals->before->type_descr == NULL)\n"
    "          TTCN_error(\"internal error: erroneous before typedescriptor "
    "missing\");\n"
    "        myleaf.body.node.nodes[node_pos] =\n"
    "          new RAW_enc_tree(TRUE, &myleaf, &(myleaf.curr_pos),\n"
    "            node_pos, err_vals->before->type_descr->raw);\n"
    "      }\n"
    "      ++node_pos;\n"
    "    }\n"
    "    if (err_vals && err_vals->value) {\n"
    "      if (err_vals->value->errval) {\n"
    "        e_c.set_msg(\"'%%s'(erroneous value): \", fld_name(field_idx));\n"
    "        if (err_vals->value->raw) {\n"
    "          myleaf.body.node.nodes[node_pos] =\n"
    "            new RAW_enc_tree(TRUE, &myleaf, &(myleaf.curr_pos),\n"
    "                             node_pos, err_vals->value->errval"
    "->get_descriptor()->raw);\n"
    "        } else {\n"
    "          if (err_vals->value->type_descr == NULL)\n"
    "            TTCN_error(\"internal error: erroneous value typedescriptor "
    "missing\");\n"
    "          myleaf.body.node.nodes[node_pos] =\n"
    "            new RAW_enc_tree(TRUE, &myleaf, &(myleaf.curr_pos),\n"
    "              node_pos, err_vals->value->type_descr->raw);\n"
    "        }\n"
    "        ++node_pos;\n"
    "      }\n"
    "    } else {\n"
    "      e_c.set_msg(\"'%%s': \", fld_name(field_idx));\n"
    "      if (!is_optional_field || get_at(field_idx)->ispresent()) {\n"
    "        myleaf.body.node.nodes[node_pos] =\n"
    "          new RAW_enc_tree(TRUE, &myleaf, &(myleaf.curr_pos),\n"
    "                           node_pos, fld_descr(field_idx)->raw);\n"
    "      } else {\n"
    "        // `omitted' field.\n"
    "        myleaf.body.node.nodes[node_pos] = NULL;\n"
    "      }\n"
    "      ++node_pos;\n"
    "    }\n"
    "    if (err_vals && err_vals->after) {\n"
    "      if (err_vals->after->errval == NULL)\n"
    "        TTCN_error(\"internal error: erroneous before value missing\");\n"
    "      if (err_vals->after->raw) {\n"
    "        myleaf.body.node.nodes[node_pos] =\n"
    "          new RAW_enc_tree(TRUE, &myleaf, &(myleaf.curr_pos),\n"
    "                           node_pos, err_vals->after->errval"
    "->get_descriptor()->raw);\n"
    "      } else {\n"
    "        if (err_vals->after->type_descr == NULL)\n"
    "          TTCN_error(\"internal error: erroneous after typedescriptor "
    "missing\");\n"
    "        myleaf.body.node.nodes[node_pos] =\n"
    "          new RAW_enc_tree(TRUE, &myleaf, &(myleaf.curr_pos),\n"
    "            node_pos, err_vals->after->type_descr->raw);\n"
    "      }\n"
    "      ++node_pos;\n"
    "    }\n"
    "    if (is_optional_field) ++next_optional_idx;\n"
    "    if ((p_err_descr->omit_after != -1) &&\n"
    "        (field_idx >= p_err_descr->omit_after))\n"
    "      break;\n"
    "  }\n");
  /* Handling of the tricky attributes.  A simple integer array will be used
     to track changes (field positions) in the record.
     int idx_map[n_fields] = {
       -1 (= field 0. was omitted),
        1 (= field 1. can be found at index 1.),
       -2 (= field 2. is still at index 2., but its value has changed) };  */
  for (i = 0; i < sdef->raw.ext_bit_goup_num; i++) {
    if (sdef->raw.ext_bit_groups[i].ext_bit != XDEFNO) {
      src = mputprintf(src,
        "  {\n"
        "    boolean in_between_modified = FALSE;\n"
        "    for (int idx_map_idx = %d; idx_map_idx <= %d; ++idx_map_idx)\n"
        "      if (idx_map[idx_map_idx] < 0) {\n"
        "        in_between_modified = TRUE;\n"
        "        break;\n"
        "      }\n"
        "    if (idx_map[%d] < 0 || idx_map[%d] < 0 ||\n"
        "        %d != idx_map[%d] - idx_map[%d] || in_between_modified) {\n"
        "      e_c.set_msg(\"Field #%d and/or #%d: \");\n"
        "      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_NEGTEST_CONFL,\n"
        "        \"Conflicting negative testing attributes, extension bit "
        "group #%d will be ignored\");\n"
        "    } else {\n"
        "      int node_idx = idx_map[%d];\n"
        "      while (node_idx <= idx_map[%d] &&\n"
        "             myleaf.body.node.nodes[node_idx] == NULL)\n"
        "        node_idx++;\n"
        "      if (myleaf.body.node.nodes[node_idx]) {\n"
        "        myleaf.body.node.nodes[node_idx]->ext_bit_handling = 1;\n"
        "        myleaf.body.node.nodes[node_idx]->ext_bit = %s;\n"
        "      }\n"
        "      node_idx = idx_map[%d];\n"
        "      while (node_idx >= idx_map[%d] &&\n"
        "             myleaf.body.node.nodes[node_idx] == NULL)\n"
        "        node_idx--;\n"
        "      if (myleaf.body.node.nodes[node_idx])\n"
        "        myleaf.body.node.nodes[node_idx]->ext_bit_handling += 2;\n"
        "    }\n"
        "  }\n",
        sdef->raw.ext_bit_groups[i].from,
        sdef->raw.ext_bit_groups[i].to,
        sdef->raw.ext_bit_groups[i].from,
        sdef->raw.ext_bit_groups[i].to,
        sdef->raw.ext_bit_groups[i].to - sdef->raw.ext_bit_groups[i].from,
        sdef->raw.ext_bit_groups[i].to,
        sdef->raw.ext_bit_groups[i].from,
        sdef->raw.ext_bit_groups[i].from,
        sdef->raw.ext_bit_groups[i].to, i,
        sdef->raw.ext_bit_groups[i].from,
        sdef->raw.ext_bit_groups[i].to,
        sdef->raw.ext_bit_groups[i].ext_bit == XDEFYES ? "EXT_BIT_YES" : "EXT_BIT_REVERSE",
        sdef->raw.ext_bit_groups[i].to,
        sdef->raw.ext_bit_groups[i].from);
    }
  }
  for (i = 0; i < sdef->nElements; i++) {
    if (sdef->elements[i].isOptional) {
      src = mputprintf(src,
        "  if (field_%s.ispresent()) {\n",
        sdef->elements[i].name);
    }
    if (raw_options[i].lengthto && sdef->elements[i].raw.lengthindex == NULL
        && sdef->elements[i].raw.union_member_num == 0) {
      /* Encoding of lenghto fields.  */
      int a;
      src = mputprintf(src,
        "  if (idx_map[%lu] < 0) {\n"
        "    e_c.set_msg(\"Field '%s': \");\n"
        "    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_NEGTEST_CONFL,\n"
        "      \"Conflicting negative testing attributes, LENGTHTO "
        "attribute will be ignored\");\n"
        "  } else {\n"
        "    boolean negtest_confl_lengthto = FALSE;\n",
        (unsigned long)i, sdef->elements[i].name);
      for (a = 0; a < sdef->elements[i].raw.lengthto_num; a++) {
        src = mputprintf(src,
          "    if (idx_map[%lu] < 0) {\n"
          "      negtest_confl_lengthto = TRUE;\n"
          "      e_c.set_msg(\"Field '%s': \");\n"
          "      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_NEGTEST_CONFL,\n"
          "        \"Conflicting negative testing attributes, LENGTHTO "
          "attribute will be ignored\");\n"
          "    }\n",
          (unsigned long)sdef->elements[i].raw.lengthto[a],
          sdef->elements[sdef->elements[i].raw.lengthto[a]].name);
      }
      src = mputprintf(src,
        "    if (!negtest_confl_lengthto) {\n"
        "      encoded_length += %d;\n"
        "      myleaf.body.node.nodes[idx_map[%lu]]->calc = CALC_LENGTH;\n"
        "      myleaf.body.node.nodes[idx_map[%lu]]->coding_descr = &%s_descr_;\n"
        "      myleaf.body.node.nodes[idx_map[%lu]]->calcof.lengthto.num_of_fields = %d;\n"
        "      myleaf.body.node.nodes[idx_map[%lu]]->calcof.lengthto.unit = %d;\n"
        "      myleaf.body.node.nodes[idx_map[%lu]]->calcof.lengthto.fields = "
        "init_lengthto_fields_list(%d);\n"
        "      myleaf.body.node.nodes[idx_map[%lu]]->length = %d;\n",
        sdef->elements[i].raw.fieldlength, (unsigned long)i,
        (unsigned long)i, sdef->elements[i].typedescrname,
        (unsigned long)i, sdef->elements[i].raw.lengthto_num,
        (unsigned long)i, sdef->elements[i].raw.unit,
        (unsigned long)i, sdef->elements[i].raw.lengthto_num,
        (unsigned long)i, sdef->elements[i].raw.fieldlength);
      for (a = 0; a < sdef->elements[i].raw.lengthto_num; a++) {
        if (sdef->elements[sdef->elements[i].raw.lengthto[a]].isOptional) {
          src = mputprintf(src,
            "      if (field_%s.ispresent()) {\n",
            sdef->elements[sdef->elements[i].raw.lengthto[a]].name);
        }
        src = mputprintf(src,
          "      myleaf.body.node.nodes[idx_map[%lu]]->calcof.lengthto.fields[%lu].level = "
          "myleaf.body.node.nodes[idx_map[%lu]]->curr_pos.level;\n"
          "      myleaf.body.node.nodes[idx_map[%lu]]->calcof.lengthto.fields[%lu].pos = "
          "myleaf.body.node.nodes[idx_map[%lu]]->curr_pos.pos;\n",
          (unsigned long)i, (unsigned long)a,
          (unsigned long)sdef->elements[i].raw.lengthto[a],
          (unsigned long)i, (unsigned long)a,
          (unsigned long)sdef->elements[i].raw.lengthto[a]);
        if (sdef->elements[sdef->elements[i].raw.lengthto[a]].isOptional) {
          src = mputprintf(src,
            "      } else {\n"
            "      myleaf.body.node.nodes[idx_map[%lu]]->"
            "calcof.lengthto.fields[%lu].level = 0;\n"
            "      myleaf.body.node.nodes[idx_map[%lu]]->"
            "calcof.lengthto.fields[%lu].pos = 0;\n"
            "      }\n",
            (unsigned long)i, (unsigned long)a,
            (unsigned long)i, (unsigned long)a);
        }
      }
      /* Closing inner index check.  */
      src = mputstr(src,
        "    }\n");
      /* Closing outer index check.  */
      src = mputstr(src,
        "  }\n");
    } else if (raw_options[i].pointerto) {
      /* Encoding of pointerto fields.  */
      if (sdef->elements[sdef->elements[i].raw.pointerto].isOptional) {
        src = mputprintf(src,
          "  if (field_%s.ispresent()) {\n",
          sdef->elements[sdef->elements[i].raw.pointerto].name);
      }
      src = mputprintf(src,
        "  boolean in_between_modified_pointerto_%s = FALSE;\n"
        "  for (int idx_map_idx = %d; idx_map_idx <= %d; ++idx_map_idx) {\n"
        "    if (idx_map[idx_map_idx] < 0) {\n"
        "      in_between_modified_pointerto_%s = TRUE;\n"
        "      break;\n"
        "    }\n"
        "  }\n"
        "  if (idx_map[%lu] < 0 || idx_map[%lu] < 0 ||\n"
        "      %lu != idx_map[%lu] - idx_map[%lu] || in_between_modified_pointerto_%s) {\n"
        "    e_c.set_msg(\"Field '%s' and/or '%s': \");\n"
        "    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_NEGTEST_CONFL,\n"
        "      \"Conflicting negative testing attributes, POINTERTO "
        "attribute will be ignored\");\n"
        "  } else {\n"
        "    encoded_length += %d;\n"
        "    myleaf.body.node.nodes[idx_map[%lu]]->calc = CALC_POINTER;\n"
        "    myleaf.body.node.nodes[idx_map[%lu]]->coding_descr = &%s_descr_;\n"
        "    myleaf.body.node.nodes[idx_map[%lu]]->calcof.pointerto.unit = %d;\n"
        "    myleaf.body.node.nodes[idx_map[%lu]]->calcof.pointerto.ptr_offset = %d;\n"
        "    myleaf.body.node.nodes[idx_map[%lu]]->calcof.pointerto.ptr_base = %d;\n"
        "    myleaf.body.node.nodes[idx_map[%lu]]->calcof.pointerto.target.level = "
        "myleaf.body.node.nodes[%d]->curr_pos.level;\n"
        "    myleaf.body.node.nodes[idx_map[%lu]]->calcof.pointerto.target.pos = "
        "myleaf.body.node.nodes[%d]->curr_pos.pos;\n"
        "    myleaf.body.node.nodes[idx_map[%lu]]->length = %d;\n"
        "  }\n",
        sdef->elements[i].name,
        i, sdef->elements[i].raw.pointerto,
        sdef->elements[i].name, (unsigned long)i,
        (unsigned long)sdef->elements[i].raw.pointerto,
        (unsigned long)sdef->elements[i].raw.pointerto - (unsigned long)i,
        (unsigned long)sdef->elements[i].raw.pointerto, (unsigned long)i,
        sdef->elements[i].name, sdef->elements[i].name,
        sdef->elements[sdef->elements[i].raw.pointerto].name,
        sdef->elements[i].raw.fieldlength, (unsigned long)i,
        (unsigned long)i, sdef->elements[i].typedescrname,
        (unsigned long)i, sdef->elements[i].raw.unit,
        (unsigned long)i, sdef->elements[i].raw.ptroffset,
        (unsigned long)i, sdef->elements[i].raw.pointerbase,
        (unsigned long)i, sdef->elements[i].raw.pointerto,
        (unsigned long)i, sdef->elements[i].raw.pointerto,
        (unsigned long)i, sdef->elements[i].raw.fieldlength);
      if (sdef->elements[sdef->elements[i].raw.pointerto].isOptional) {
        src = mputprintf(src,
        "  } else {\n"
        "    INTEGER atm;\n"
        "    atm = 0;\n"
        "    encoded_length += atm.RAW_encode(%s_descr_, "
        "*myleaf.body.node.nodes[idx_map[%lu]]);\n"
        "  }\n",
        sdef->elements[i].typedescrname, (unsigned long)i);
      }
    } else {
      /* Encoding of normal fields with no negative testing.  Needed for the
         later phase.  */
      src = mputprintf(src,
        "  if (idx_map[%lu] >= 0) "
        "encoded_length += field_%s%s.RAW_encode(%s_descr_, "
        "*myleaf.body.node.nodes[idx_map[%lu]]);\n",
        (unsigned long)i, sdef->elements[i].name,
        sdef->elements[i].isOptional ? "()" : "",
        sdef->elements[i].typedescrname, (unsigned long)i);
    }
    if (sdef->elements[i].isOptional) {
      src = mputstr(src,
        "  }\n");
    }
  }
  for (i = 0; i < sdef->nElements; i++) {
    /* Fill presence, tag and crosstag.  */
    if (raw_options[i].lengthto && sdef->elements[i].raw.lengthindex) {
      /* Encoding of lenghto fields with lengthindex.  */
      int a;
      if (sdef->elements[i].isOptional) {
        src = mputprintf(src,
          "  if (field_%s.ispresent()) {\n",
          sdef->elements[i].name);
      }
      src = mputprintf(src,
        "  if (idx_map[%lu] < 0) {\n"
        "    e_c.set_msg(\"Field '%s': \");\n"
        "    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_NEGTEST_CONFL,\n"
        "      \"Conflicting negative testing attributes, LENGTHTO/LENGTHINDEX "
        "attribute will be ignored\");\n"
        "  } else {\n"
        "    if (myleaf.body.node.nodes[idx_map[%lu]]->body.node.nodes[%d]) {\n"
        "      boolean negtest_confl_lengthto = FALSE;\n",
        (unsigned long)i, sdef->elements[i].name, (unsigned long)i,
        sdef->elements[i].raw.lengthindex->nthfield);
      for (a = 0; a < sdef->elements[i].raw.lengthto_num; a++) {
        src = mputprintf(src,
          "      if (idx_map[%lu] < 0) {\n"
          "        negtest_confl_lengthto = TRUE;\n"
          "        e_c.set_msg(\"Field '%s': \");\n"
          "        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_NEGTEST_CONFL,\n"
          "          \"Conflicting negative testing attributes, LENGTHTO/LENGTHINDEX "
          "attribute will be ignored\");\n"
          "      }\n",
          (unsigned long)sdef->elements[i].raw.lengthto[a],
          sdef->elements[sdef->elements[i].raw.lengthto[a]].name);
      }
      src = mputprintf(src,
        "      if (!negtest_confl_lengthto) {\n"
        "        myleaf.body.node.nodes[idx_map[%lu]]->body.node.nodes[%d]->"
        "calc = CALC_LENGTH;\n"
        "        myleaf.body.node.nodes[idx_map[%lu]]->body.node.nodes[%d]->"
        "coding_descr = &%s_descr_;\n"
        "        myleaf.body.node.nodes[idx_map[%lu]]->body.node.nodes[%d]->"
        "calcof.lengthto.num_of_fields = %d;\n"
        "        myleaf.body.node.nodes[idx_map[%lu]]->body.node.nodes[%d]->"
        "calcof.lengthto.unit = %d;\n"
        "        myleaf.body.node.nodes[idx_map[%lu]]->body.node.nodes[%d]->"
        "calcof.lengthto.fields = init_lengthto_fields_list(%d);\n",
        (unsigned long)i, sdef->elements[i].raw.lengthindex->nthfield,
        (unsigned long)i, sdef->elements[i].raw.lengthindex->nthfield,
        sdef->elements[i].raw.lengthindex->typedescr,
        (unsigned long)i, sdef->elements[i].raw.lengthindex->nthfield,
        sdef->elements[i].raw.lengthto_num,
        (unsigned long)i, sdef->elements[i].raw.lengthindex->nthfield,
        sdef->elements[i].raw.unit,
        (unsigned long)i, sdef->elements[i].raw.lengthindex->nthfield,
        sdef->elements[i].raw.lengthto_num);
      for (a = 0; a < sdef->elements[i].raw.lengthto_num; a++) {
        if (sdef->elements[sdef->elements[i].raw.lengthto[a]].isOptional) {
          src = mputprintf(src,
            "        if (field_%s.ispresent()) {\n",
            sdef->elements[sdef->elements[i].raw.lengthto[a]].name);
        }
        src = mputprintf(src,
          "        myleaf.body.node.nodes[idx_map[%lu]]->body.node.nodes[%d]"
          "->calcof.lengthto.fields[%d].level = "
          "myleaf.body.node.nodes[idx_map[%d]]->curr_pos.level;\n"
          "        myleaf.body.node.nodes[idx_map[%lu]]->body.node.nodes[%d]"
          "->calcof.lengthto.fields[%d].pos = "
          "myleaf.body.node.nodes[idx_map[%d]]->curr_pos.pos;\n",
          (unsigned long)i, sdef->elements[i].raw.lengthindex->nthfield, a,
          sdef->elements[i].raw.lengthto[a], (unsigned long)i,
          sdef->elements[i].raw.lengthindex->nthfield, a,
          sdef->elements[i].raw.lengthto[a]);
        if (sdef->elements[sdef->elements[i].raw.lengthto[a]].isOptional) {
          src = mputprintf(src,
            "        } else {\n"
            "        myleaf.body.node.nodes[idx_map[%lu]]->body.node.nodes[%d]->"
            "calcof.lengthto.fields[%d].level = 0;\n"
            "        myleaf.body.node.nodes[idx_map[%lu]]->body.node.nodes[%d]->"
            "calcof.lengthto.fields[%d].pos = 0;\n"
            "        }\n",
           (unsigned long)i, sdef->elements[i].raw.lengthindex->nthfield, a,
           (unsigned long)i, sdef->elements[i].raw.lengthindex->nthfield, a);
        }
      }
      /* Closing index check.  */
      src = mputstr(src,
        "      }\n");
      src = mputstr(src, "    }\n  }\n");
      if (sdef->elements[i].isOptional) {
        src = mputstr(src, "  }\n");
      }
    }
    if (raw_options[i].lengthto && sdef->elements[i].raw.union_member_num) {
      /* Encoding of lenghto fields.  */
      int a;
      if (sdef->elements[i].isOptional) {
        src = mputprintf(src,
          "  if (field_%s.ispresent()) ", sdef->elements[i].name);
      }
      src = mputprintf(src,
        "  {\n"
        "  if (idx_map[%lu] < 0) {\n"
        "    e_c.set_msg(\"Field '%s': \");\n"
        "    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_NEGTEST_CONFL,\n"
        "      \"Conflicting negative testing attributes, LENGTHTO/LENGTHINDEX "
        "attribute will be ignored\");\n"
        "  } else {\n"
        "    boolean negtest_confl_lengthto = FALSE;\n",
        (unsigned long)i, sdef->elements[i].name);
      for (a = 0; a < sdef->elements[i].raw.lengthto_num; a++) {
        src = mputprintf(src,
          "  if (idx_map[%lu] < 0) {\n"
          "    negtest_confl_lengthto = TRUE;\n"
          "    e_c.set_msg(\"Field '%s': \");\n"
          "      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_NEGTEST_CONFL,\n"
          "        \"Conflicting negative testing attributes, LENGTHTO/LENGTHINDEX "
          "attribute will be ignored\");\n"
          "  }\n",
          (unsigned long)sdef->elements[i].raw.lengthto[a],
          sdef->elements[sdef->elements[i].raw.lengthto[a]].name);
      }
      src = mputprintf(src,
        "    if (!negtest_confl_lengthto) {\n"
        "      int sel_field = 0;\n"
        "      while (myleaf.body.node.nodes[idx_map[%lu]]->body.node.nodes[sel_field] == NULL) "
        "{ sel_field++; }\n"
        "      myleaf.body.node.nodes[idx_map[%lu]]->body.node.nodes[sel_field]->"
        "calc = CALC_LENGTH;\n"
        "      myleaf.body.node.nodes[idx_map[%lu]]->body.node.nodes[sel_field]->"
        "calcof.lengthto.num_of_fields = %d;\n"
        "      myleaf.body.node.nodes[idx_map[%lu]]->body.node.nodes[sel_field]->"
        "calcof.lengthto.unit = %d;\n"
        "      myleaf.body.node.nodes[idx_map[%lu]]->body.node.nodes[sel_field]->"
        "calcof.lengthto.fields = init_lengthto_fields_list(%d);\n",
        (unsigned long)i, (unsigned long)i,
        (unsigned long)i, sdef->elements[i].raw.lengthto_num,
        (unsigned long)i, sdef->elements[i].raw.unit,
        (unsigned long)i, sdef->elements[i].raw.lengthto_num);
      for (a = 0; a < sdef->elements[i].raw.lengthto_num; a++) {
        if (sdef->elements[sdef->elements[i].raw.lengthto[a]].isOptional) {
          src = mputprintf(src,
            "      if (field_%s.ispresent()) {\n",
            sdef->elements[sdef->elements[i].raw.lengthto[a]].name);
        }
        src = mputprintf(src,
          "      myleaf.body.node.nodes[idx_map[%lu]]->body.node.nodes[sel_field]"
          "->calcof.lengthto.fields[%d].level = "
          "myleaf.body.node.nodes[%d]->curr_pos.level;\n"
          "      myleaf.body.node.nodes[idx_map[%lu]]->body.node.nodes[sel_field]"
          "->calcof.lengthto.fields[%d].pos = "
          "myleaf.body.node.nodes[%d]->curr_pos.pos;\n",
          (unsigned long)i, a, sdef->elements[i].raw.lengthto[a],
          (unsigned long)i, a, sdef->elements[i].raw.lengthto[a]);
        if (sdef->elements[sdef->elements[i].raw.lengthto[a]].isOptional) {
          src = mputprintf(src,
            "      } else {\n"
            "      myleaf.body.node.nodes[idx_map[%lu]]->body.node.nodes[sel_field]"
            "->calcof.lengthto.fields[%d].level = 0;\n"
            "      myleaf.body.node.nodes[idx_map[%lu]]->body.node.nodes[sel_field]"
            "->calcof.lengthto.fields[%d].pos = 0;\n"
            "      }\n",
            (unsigned long)i, a,
            (unsigned long)i, a);
        }
      }
      /* Closing inner index check.  */
      src = mputstr(src,
        "    }\n");
      /* Closing outer index check.  */
      src = mputstr(src,
        "  }\n");
      /* Closing ispresent or local block.  */
      src = mputstr(src,
        "  }\n");
    }
    if (raw_options[i].tag_type &&
        sdef->raw.taglist.list[raw_options[i].tag_type - 1].nElements) {
      /* Plain, old tag.  */
      rawAST_coding_taglist *cur_choice =
        sdef->raw.taglist.list + raw_options[i].tag_type - 1;
      src = mputprintf(src,
        "  boolean negtest_confl_tag_%s = FALSE;\n"
        "  if (idx_map[%lu] < 0) {\n"
        "    negtest_confl_tag_%s = TRUE;\n"
        "    e_c.set_msg(\"Field '%s': \");\n"
        "    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_NEGTEST_CONFL,\n"
        "      \"Conflicting negative testing attributes, TAG attribute will "
        "be ignored\");\n"
        "  }\n",
        sdef->elements[i].name, (unsigned long)i, sdef->elements[i].name,
        sdef->elements[i].name);
      src = mputprintf(src, "  if (!negtest_confl_tag_%s && (",
                       sdef->elements[i].name);
      if (sdef->elements[i].isOptional) {
        src = mputprintf(src, "field_%s.ispresent() && (", sdef->elements[i].name);
      }
      src = genRawFieldChecker(src, cur_choice, FALSE);
      if (sdef->elements[i].isOptional) src = mputstr(src, ")");
      src = mputstr(src,")) {\n");
      src = genRawTagChecker(src, cur_choice);
      src = mputstr(src,"  }\n");
    }
    if (sdef->elements[i].hasRaw && sdef->elements[i].raw.presence.nElements) {
      /* Presence attribute for fields.  */
      int taglist_idx, field_idx;
      /* The optional field itself.  */
      src = mputprintf(src,
        "  boolean negtest_confl_presence_%s = FALSE;\n"
        "  if (idx_map[%lu] < 0) {\n"
        "    negtest_confl_presence_%s = TRUE;\n"
        "    e_c.set_msg(\"Field '%s': \");\n"
        "    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_NEGTEST_CONFL,\n"
        "      \"Conflicting negative testing attributes, PRESENCE attribute "
        "will be ignored\");\n"
        "  }\n",
        sdef->elements[i].name, (unsigned long)i, sdef->elements[i].name,
        sdef->elements[i].name);
      /* Check the referenced fields.  */
      for (taglist_idx = 0;
           taglist_idx < sdef->elements[i].raw.presence.nElements; ++taglist_idx) {
        rawAST_coding_field_list *fields =
          sdef->elements[i].raw.presence.fields + taglist_idx;
        for (field_idx = 0; field_idx < fields->nElements; ++field_idx) {
          /* The top level field is enough.  >0 index is for subrefs.  */
          if (field_idx == 0) {
            rawAST_coding_fields *field = fields->fields + field_idx;
            src = mputprintf(src,
              "  if (idx_map[%d] < 0) {\n"
              "    negtest_confl_presence_%s = TRUE;\n"
              "    e_c.set_msg(\"Field '%s': \");\n"
              "    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_NEGTEST_CONFL,\n"
              "      \"Conflicting negative testing attributes, PRESENCE attribute "
              "will be ignored\");\n"
              "  }\n",
              field->nthfield, sdef->elements[i].name, field->nthfieldname);
          } else {
            /* Subrefs.  */
            break;
          }
        }
      }
      src = mputprintf(src, "  if (!negtest_confl_presence_%s && (",
                       sdef->elements[i].name);
      if (sdef->elements[i].isOptional) {
        src = mputprintf(src, "field_%s.ispresent() && (", sdef->elements[i].name);
      }
      src = genRawFieldChecker(src, &sdef->elements[i].raw.presence, FALSE);
      if (sdef->elements[i].isOptional) src = mputstr(src, ")");
      src = mputstr(src, ")) {\n");
      src = genRawTagChecker(src, &sdef->elements[i].raw.presence);
      src = mputstr(src, "  }\n");
    }
    if (sdef->elements[i].hasRaw &&
        sdef->elements[i].raw.crosstaglist.nElements) {
      /* crosstag */
      int a;
      /* The union field itself.  */
      src = mputprintf(src,
        "  if (idx_map[%lu] < 0) {\n"
        "    e_c.set_msg(\"Field '%s': \");\n"
        "    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_NEGTEST_CONFL,\n"
        "      \"Conflicting negative testing attributes, CROSSTAG attribute "
        "will be ignored\");\n"
        "  } else {\n",
        (unsigned long)i, sdef->elements[i].name);
      if (sdef->elements[i].isOptional) {
        src = mputprintf(src,
          "  if (field_%s.ispresent()) {\n",
          sdef->elements[i].name);
      }
      src = mputprintf(src,
        "  switch (field_%s%s.get_selection()) {\n",
        sdef->elements[i].name,sdef->elements[i].isOptional ? "()" : "");
      for (a = 0; a < sdef->elements[i].raw.crosstaglist.nElements; a++) {
        rawAST_coding_taglist *cur_choice =
          sdef->elements[i].raw.crosstaglist.list + a;
        if (cur_choice->nElements > 0) {
          int taglist_idx, field_idx;
          src = mputprintf(src,
            "  case %s%s%s: {\n"
            "  boolean negtest_confl_crosstag = FALSE;\n",
            sdef->elements[i].type, "::ALT_",
            cur_choice->fieldName);
          for (taglist_idx = 0;
               taglist_idx < cur_choice->nElements; ++taglist_idx) {
            rawAST_coding_field_list *fields = cur_choice->fields + taglist_idx;
            for (field_idx = 0; field_idx < fields->nElements; ++field_idx) {
              if (field_idx == 0) {
                rawAST_coding_fields *field = fields->fields + field_idx;
                src = mputprintf(src,
                  "  if (idx_map[%d] < 0) {\n"
                  "    negtest_confl_crosstag = TRUE;\n"
                  "    e_c.set_msg(\"Field '%s': \");\n"
                  "    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_NEGTEST_CONFL,\n"
                  "      \"Conflicting negative testing attributes, CROSSTAG attribute "
                  "will be ignored\");\n"
                  "  }\n",
                  field->nthfield, field->nthfieldname);
              } else {
                break;
              }
            }
          }
          src = mputstr(src, "  if (!negtest_confl_crosstag && (");
          src = genRawFieldChecker(src, cur_choice, FALSE);
          src = mputstr(src, ")) {\n");
          if (!strcmp(cur_choice->fields[0].value, "OMIT_VALUE")) {
            if (cur_choice->fields[0].nElements != 1)
              NOTSUPP("omit value with multiple fields in CROSSTAG");
            /* eliminating the corresponding encoded leaf, which will have
               the same effect as if the referred field had been omit */
            src = mputprintf(src,
              "  encoded_length -= myleaf.body.node.nodes[%d]->length;\n"
              "  delete myleaf.body.node.nodes[%d];\n"
              "  myleaf.body.node.nodes[%d] = NULL;\n",
              cur_choice->fields[0].fields[0].nthfield,
              cur_choice->fields[0].fields[0].nthfield,
              cur_choice->fields[0].fields[0].nthfield);
          } else {
            int ll;
            src = mputprintf(src,
              "  RAW_enc_tr_pos pr_pos;\n"
              "  pr_pos.level = myleaf.curr_pos.level + %d;\n"
              "  int new_pos[] = { ",
              cur_choice->fields[0].nElements);
            for (ll = 0; ll < cur_choice->fields[0].nElements; ll++) {
              if (ll > 0) src = mputstr(src, ", ");
              src = mputprintf(src, "%d",
                cur_choice->fields[0].fields[ll].nthfield);
            }
            src = mputprintf(src, " };\n"
              "  pr_pos.pos = init_new_tree_pos(myleaf.curr_pos, %d, "
              "new_pos);\n",
            cur_choice->fields[0].nElements);
            if (cur_choice->fields[0].value[0] == ' ') {
              /* the value is a string literal (the encoder can be called
                 on that object directly */
              src = mputprintf(src,
                "  RAW_enc_tree* temp_leaf = myleaf.get_node(pr_pos);\n"
                "  if (temp_leaf != NULL)\n"
                "    %s.RAW_encode(%s_descr_, *temp_leaf);\n"
                "  else\n"
                "    TTCN_EncDec_ErrorContext::error\n"
                "      (TTCN_EncDec::ET_OMITTED_TAG, \"Encoding a tagged, "
                "but omitted value.\");\n",
                cur_choice->fields[0].value,
                cur_choice->fields[0].fields[
                  cur_choice->fields[0].nElements - 1].typedescr);
            } else {
              /* a temporary object needs to be created for encoding */
              src = mputprintf(src,
                "  %s new_val(%s);\n"
                "  RAW_enc_tree* temp_leaf = myleaf.get_node(pr_pos);\n"
                "  if (temp_leaf != NULL)\n"
                "    new_val.RAW_encode(%s_descr_, *temp_leaf);\n"
                "  else\n"
                "    TTCN_EncDec_ErrorContext::error\n"
                "      (TTCN_EncDec::ET_OMITTED_TAG, \"Encoding a tagged, "
                "but omitted value.\");\n",
                cur_choice->fields[0].fields[
                  cur_choice->fields[0].nElements - 1].type,
                cur_choice->fields[0].value,
                cur_choice->fields[0].fields[
                  cur_choice->fields[0].nElements - 1].typedescr);
            }
            src = mputstr(src, "  free_tree_pos(pr_pos.pos);\n");
          }
          src = mputstr(src, "  }\n"
            "  break; }\n");
        }
      }
      src = mputstr(src, "  default:;\n"
        "  }\n");
      if (sdef->elements[i].isOptional) src = mputstr(src, "  }\n");
      src = mputstr(src, "  }\n");
    }
  }
  src = mputprintf(src,
    "  node_pos = 0;\n"
    "  next_optional_idx = 0;\n"
    "  for (int field_idx = 0; field_idx < num_fields; ++field_idx) {\n"
    "    boolean is_optional_field = my_optional_indexes &&\n"
    "      (my_optional_indexes[next_optional_idx] == field_idx);\n"
    "    if ((p_err_descr->omit_before != -1) &&\n"
    "        (field_idx < p_err_descr->omit_before)) {\n"
    "      if (is_optional_field) ++next_optional_idx;\n"
    "      continue;\n"
    "    }\n"
    "    const Erroneous_values_t *err_vals = p_err_descr->get_field_err_values(field_idx);\n"
    "    const Erroneous_descriptor_t *emb_descr = p_err_descr->get_field_emb_descr(field_idx);\n"
    "    if (err_vals && err_vals->before) {\n"
    "      if (err_vals->before->errval == NULL)\n"
    "        TTCN_error(\"internal error: erroneous before value missing\");\n"
    "      if (err_vals->before->raw) {\n"
    "        encoded_length += err_vals->before->errval->"
    "RAW_encode_negtest_raw(*myleaf.body.node.nodes[node_pos++]);\n"
    "      } else {\n"
    "        if (err_vals->before->type_descr == NULL)\n"
    "          TTCN_error(\"internal error: erroneous before typedescriptor missing\");\n"
    "        encoded_length += err_vals->before->errval->RAW_encode(*(err_vals->before->type_descr),\n"
    "          *myleaf.body.node.nodes[node_pos++]);\n"
    "      }\n"
    "    }\n"
    "    if (err_vals && err_vals->value) {\n"
    "      if (err_vals->value->errval) {\n"
    "        e_c.set_msg(\"'%%s'(erroneous value): \", fld_name(field_idx));\n"
    "        if (err_vals->value->raw) {\n"
    "          encoded_length += err_vals->value->errval->"
    "RAW_encode_negtest_raw(*myleaf.body.node.nodes[node_pos++]);\n"
    "        } else {\n"
    "          if (err_vals->value->type_descr == NULL)\n"
    "            TTCN_error(\"internal error: erroneous value typedescriptor missing\");\n"
    "          encoded_length += err_vals->value->errval->"
    "RAW_encode(*(err_vals->value->type_descr), *myleaf.body.node.nodes[node_pos++]);\n"
    "        }\n"
    "      }\n"
    "    } else {\n"
    "      e_c.set_msg(\"'%%s': \", fld_name(field_idx));\n"
    "      if (!is_optional_field || get_at(field_idx)->ispresent()) {\n"
    "        const Base_Type *curr_field = is_optional_field "
    "? get_at(field_idx)->get_opt_value() : get_at(field_idx);\n"
    "        if (emb_descr) {\n"
    "          encoded_length += curr_field\n"
    "            ->RAW_encode_negtest(emb_descr, *fld_descr(field_idx),\n"
    "                                 *myleaf.body.node.nodes[node_pos]);\n"
    "        }\n"
    "      } else {\n"
    "        // `omitted' field.\n"
    "        myleaf.body.node.nodes[node_pos] = NULL;\n"
    "      }\n"
    "      ++node_pos;\n"
    "    }\n"
    "    if (err_vals && err_vals->after) {\n"
    "      if (err_vals->after->errval == NULL)\n"
    "        TTCN_error(\"internal error: erroneous before value missing\");\n"
    "      if (err_vals->after->raw) {\n"
    "        encoded_length += err_vals->after->errval->"
    "RAW_encode_negtest_raw(*myleaf.body.node.nodes[node_pos++]);\n"
    "      } else {\n"
    "        if (err_vals->after->type_descr == NULL)\n"
    "          TTCN_error(\"internal error: erroneous after typedescriptor missing\");\n"
    "        encoded_length += err_vals->after->errval->"
    "RAW_encode(*(err_vals->after->type_descr), *myleaf.body.node.nodes[node_pos++]);\n"
    "      }\n"
    "    }\n"
    "    if (is_optional_field) ++next_optional_idx;\n"
    "    if ((p_err_descr->omit_after != -1) &&\n"
    "        (field_idx >= p_err_descr->omit_after))\n"
    "      break;\n"
    "  }\n");
  /* Presence for the whole record.  */
  if (sdef->hasRaw && sdef->raw.presence.nElements > 0) {
    /* Check the referenced fields.  */
    int taglist_idx, field_idx;
    src = mputstr(src, "  boolean negtest_confl_presence = FALSE;\n");
    for (taglist_idx = 0; taglist_idx < sdef->raw.presence.nElements; ++taglist_idx) {
      rawAST_coding_field_list *fields =
        sdef->raw.presence.fields + taglist_idx;
      for (field_idx = 0; field_idx < fields->nElements; ++field_idx) {
        if (field_idx == 0) {
          rawAST_coding_fields *field = fields->fields + field_idx;
          src = mputprintf(src,
            "  if (idx_map[%d] < 0) {\n"
            "    negtest_confl_presence = TRUE;\n"
            "    e_c.set_msg(\"Field '%s': \");\n"
            "    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_NEGTEST_CONFL,\n"
            "      \"Conflicting negative testing attributes, PRESENCE attribute "
            "will be ignored\");\n"
            "  }\n",
            field->nthfield, field->nthfieldname);
        } else {
          break;
        }
      }
    }
    src = mputstr(src, "  if (!negtest_confl_presence && (");
    src = genRawFieldChecker(src, &sdef->raw.presence, FALSE);
    src = mputstr(src,")) {\n");
    src = genRawTagChecker(src, &sdef->raw.presence);
    src = mputstr(src,"  }\n");
  }
  src = mputstr(src,
    "  return myleaf.length = encoded_length;\n"
    "}\n\n");
  return src;
}

void gen_xer(const struct_def *sdef, char **pdef, char **psrc)
{ /* XERSTUFF codegen for record/SEQUENCE */
  size_t i;
  char *def = *pdef, *src = *psrc;
  const char * const name = sdef->name;

  /*
   * NOTE! Decisions based on the coding instructions of the components
   * (e.g. number of attributes) can be made at compile time.
   * Decisions on coding instructions of the record itself (e.g. D-F-E,
   * EMBED-VALUES, USE-NIL, USE-ORDER) __must__ be postponed to run-time,
   * because these can be applied to type references,
   * which will not be visible when the code is generated.
   */

  /* Number of fields with ATTRIBUTE, includes ANY-ATTRIBUTES */
  size_t num_attributes = 0;

  /* index of USE-ORDER member, which may be bumped into second place by EMBED-VALUES */
  size_t uo = (sdef->xerEmbedValuesPossible !=0);

  /* start_at is the index of the first "real" member of the record */
  size_t start_at = uo + (sdef->xerUseOrderPossible != 0);

  /* Number of optional non-attributes */
  size_t n_opt_elements = 0;

  const boolean want_namespaces = !sdef->isASN1 /* TODO remove this when ASN.1 gets EXER */;

  int aa_index = -1;
  /* Find the number of attributes (compile time) */
  for (i = 0; i < sdef->nElements; ++i) {
    if (sdef->elements[i].xerAttribute) {
      ++num_attributes;
    }
    else if (sdef->elements[i].xerAnyKind & ANY_ATTRIB_BIT) {
      ++num_attributes; /* ANY-ATTRIBUTES also included with attributes */
      aa_index = i;
      /* If the first member is ANY-ATTRIBUTES, then xerEmbedValuesPossible
       * is merely an illusion and USE-ORDER is not possible. */
      if (i==0) {
        start_at = uo = 0;
      }
    }
    else /* not attribute */ if (sdef->elements[i].isOptional) {
      ++n_opt_elements;
    }
  }

  /* Write some helper functions */
  def = mputstr(def,
    "char **collect_ns(const XERdescriptor_t& p_td, size_t& num_ns, bool& def_ns, unsigned int flavor = 0) const;\n");

  src = mputprintf(src,
    "char ** %s::collect_ns(const XERdescriptor_t& p_td, size_t& num_ns, bool& def_ns, unsigned int flavor) const {\n"
    "  size_t num_collected;\n"
    "  char **collected_ns = Base_Type::collect_ns(p_td, num_collected, def_ns, flavor);\n"
    /* The above might throw but then nothing was allocated. */
    , name);

  /* Collect namespaces from the "non-special" members (incl. attributes) */
  if (start_at < sdef->nElements) src = mputstr(src,
    "  try {\n"
    "  char **new_ns;\n"
    "  size_t num_new;\n"
    "  boolean def_ns_1 = FALSE;\n");
  for (i = start_at; i < sdef->nElements; ++i) {
    src = mputprintf(src,
      "  new_ns = field_%s.collect_ns(%s_xer_, num_new, def_ns, flavor);\n"
      "  merge_ns(collected_ns, num_collected, new_ns, num_new);\n"
      "  def_ns = def_ns || def_ns_1;\n" /* alas, no ||= */
      , sdef->elements[i].name, sdef->elements[i].typegen
    );
  }
  if (sdef->xerUseNilPossible) {
    src = mputprintf(src,
      "  if((p_td.xer_bits & USE_NIL) && !field_%s.ispresent()) {\n" /* "nil" attribute to be written*/
      "    collected_ns = (char**)Realloc(collected_ns, sizeof(char*) * ++num_collected);\n"
      "    const namespace_t *c_ns = p_td.my_module->get_controlns();\n"
      "    collected_ns[num_collected-1] = mprintf(\" xmlns:%%s='%%s'\", c_ns->px, c_ns->ns);\n"
      "  }\n"
      , sdef->elements[sdef->nElements-1].name
    );
  }
  if (start_at < sdef->nElements) src = mputstr(src,
    "  }\n"
    "  catch (...) {\n"
    /* Probably a TC_Error thrown from field_%s->collect_ns() if e.g.
     * encoding an unbound value. */
    "    while (num_collected > 0) Free(collected_ns[--num_collected]);\n"
    "    Free(collected_ns);\n"
    "    throw;\n"
    "  }\n"
  );

  src = mputstr(src,
    "  num_ns = num_collected;\n"
    "  return collected_ns;\n"
    "}\n\n"
  );

  src = mputprintf(src,
    "boolean %s::can_start(const char *name, const char *uri, const XERdescriptor_t& xd, unsigned int flavor, unsigned int flavor2) {\n"
    "  boolean e_xer = is_exer(flavor &= ~XER_RECOF);\n"
    "  if (!e_xer || !((xd.xer_bits & UNTAGGED) || (flavor & (USE_NIL|XER_RECOF)))) return check_name(name, xd, e_xer) && (!e_xer || check_namespace(uri, xd));\n"
    , name
  );
  for (i = start_at; i < sdef->nElements; ++i) {
    src = mputprintf(src,
      "  else if (%s::can_start(name, uri, %s_xer_, flavor, flavor2)) return TRUE;\n"
      /* Here we know for sure it's exer */
      , sdef->elements[i].type
      , sdef->elements[i].typegen
    );
    if (!sdef->elements[i].isOptional) break;
    /* The last component with which it can begin is the first non-optional.
     * Does that sentence makes sense to you ? */
  }
  src = mputstr(src, "  return FALSE;\n}\n\n");

  /* * * * * * * * * * XER_encode * * * * * * * * * * * * * * */
  src = mputprintf(src,
    "int %s::XER_encode(const XERdescriptor_t& p_td, "
    "TTCN_Buffer& p_buf, unsigned int p_flavor, unsigned int p_flavor2, int p_indent, embed_values_enc_struct_t* emb_val_parent) const\n"
    "{\n"
    "  if (!is_bound()) TTCN_EncDec_ErrorContext::error"
    "(TTCN_EncDec::ET_UNBOUND, \"Encoding an unbound value.\");\n"
    "  TTCN_EncDec_ErrorContext ec_0(\"Component '\");\n"
    "  TTCN_EncDec_ErrorContext ec_1;\n"
    "  int encoded_length=(int)p_buf.get_len();\n"
    "  boolean e_xer = is_exer(p_flavor);\n"
    "  const boolean omit_tag = e_xer && p_indent "
    "&& ((p_td.xer_bits & (UNTAGGED|XER_ATTRIBUTE)) || (p_flavor & (USE_NIL|USE_TYPE_ATTR)));\n"
    "  if (e_xer && (p_td.xer_bits & EMBED_VALUES)) p_flavor |= XER_CANONICAL;\n"
    "  int is_indented = !is_canonical(p_flavor);\n"
    , name);

  if (want_namespaces && !(num_attributes|sdef->xerUseQName)) src = mputstr(src,
    "  const boolean need_control_ns = (p_td.xer_bits & (USE_NIL));\n");

  if (want_namespaces && (start_at < sdef->nElements)) { /* there _are_ non-special members */
    src = mputstr(src,
      "  size_t num_collected = 0;\n"
      "  char **collected_ns = NULL;\n"
      "  boolean def_ns = FALSE;\n"
      "  if (e_xer) {\n"
      "    if (p_indent == 0) {\n" /* top-level */
      "      collected_ns = collect_ns(p_td, num_collected, def_ns, p_flavor2);\n" /* our own ns */
      "    }\n"
      "    else if ((p_flavor & DEF_NS_SQUASHED) && p_td.my_module && p_td.ns_index != -1){\n"
      "      const namespace_t * ns = p_td.my_module->get_ns((size_t)p_td.ns_index);\n"
      "      if (*ns->px == '\\0') {\n"
      "        collected_ns = Base_Type::collect_ns(p_td, num_collected, def_ns, p_flavor2);\n"
      "      }\n"
      "    }\n"
      "  }\n"
    );
  }

  if (want_namespaces) {
    src = mputstr(src,
      "  const boolean empty_ns_hack = e_xer && !omit_tag && (p_indent > 0)\n"
      "    && (p_td.xer_bits & FORM_UNQUALIFIED)\n"
      "    && p_td.my_module && p_td.ns_index != -1\n"
      "    && *p_td.my_module->get_ns((size_t)p_td.ns_index)->px == '\\0';\n"
    );

    src = mputstr(src, "  boolean delay_close = e_xer");
    if (!(num_attributes | sdef->xerUseQName)) {
      src = mputprintf(src, " && (need_control_ns%s || empty_ns_hack)",
        (start_at < sdef->nElements) ? " || num_collected" : "");
    }
    src = mputstr(src, ";\n");
  }


  { /* write start tag */
    /* From "name>\n":
     * lose the \n if : not indenting or (single untagged(*) or attributes (*)) AND e_xer
     * lose the > if attributes are present (*) AND e_xer
     */
    src = mputprintf(src,
      "  size_t chopped_chars = 0;\n"
      "  if (!omit_tag) {\n"
      "    if (is_indented) do_indent(p_buf, p_indent);\n"
      "    p_buf.put_c('<');\n"
      "    if (e_xer) write_ns_prefix(p_td, p_buf);\n"
      "    p_buf.put_s((size_t)p_td.namelens[e_xer]%s-(!is_indented%s), "
      "(cbyte*)p_td.names[e_xer]);\n"
      "  }\n"
      "  else if (p_flavor & (USE_NIL|USE_TYPE_ATTR)) {\n"
      "    size_t buf_len = p_buf.get_len();\n"
      "    const unsigned char * const buf_data = p_buf.get_data();\n"
      "    if (buf_data[buf_len-1-chopped_chars] == '\\n') ++chopped_chars;\n"
      "    if (buf_data[buf_len-1-chopped_chars] == '>' ) ++chopped_chars;\n"
      "    if (chopped_chars) {\n"
      "      p_buf.increase_length(-chopped_chars);\n"
      "    }\n"
      "%s"
      "  }\n"
      , (want_namespaces ?   "-(delay_close || (e_xer && (p_td.xer_bits & HAS_1UNTAGGED)))" : "")
      , (want_namespaces ? " || delay_close" : "")
      , (want_namespaces ? "    delay_close = TRUE;\n" : ""));
  }

  src = mputprintf(src,
    "  int sub_len=0%s;\n"
    "  p_flavor &= XER_MASK;\n",
    num_attributes ? ", tmp_len; (void)tmp_len" : "");

  if (sdef->xerUseQName) {
    src = mputprintf(src,
      "  if (e_xer && (p_td.xer_bits & USE_QNAME)) {\n"
      "    if (field_%s.is_value()) {\n"
      "      p_buf.put_s(11, (cbyte*)\" xmlns:b0='\");\n"
      "      field_%s.XER_encode(%s_xer_, p_buf, p_flavor | XER_LIST, p_flavor2, p_indent+1, 0);\n"
      "      p_buf.put_c('\\'');\n"
      "    }\n"
      "    if (p_td.xer_bits & XER_ATTRIBUTE) begin_attribute(p_td, p_buf);\n"
      "    else  p_buf.put_c('>');\n"
      "    if (field_%s.is_value()) {\n"
      "      p_buf.put_s(3, (cbyte*)\"b0:\");\n"
      "      sub_len += 3;\n"
      "    }\n"
      "    sub_len += field_%s.XER_encode(%s_xer_, p_buf, p_flavor | XER_LIST, p_flavor2, p_indent+1, 0);\n"
      "    if (p_td.xer_bits & XER_ATTRIBUTE) p_buf.put_c('\\'');\n"
      "  } else" /* no newline */
      , sdef->elements[0].name
      , sdef->elements[0].name, sdef->elements[0].typegen
      , sdef->elements[0].name
      , sdef->elements[1].name, sdef->elements[1].typegen
    );
  }
  src = mputstr(src, "  { // !QN\n");
 
 /* First, the EMBED-VALUES member as an ordinary member if not doing EXER */
  if (sdef->xerEmbedValuesPossible) {
    src = mputprintf(src,
      "  if (!e_xer && (p_td.xer_bits & EMBED_VALUES)) {\n"
      "    ec_1.set_msg(\"%s': \");\n"
      "    sub_len += field_%s.XER_encode(%s_xer_, p_buf, p_flavor, p_flavor2, p_indent+1, 0);\n"
      "  }\n"
      , sdef->elements[0].dispname
      , sdef->elements[0].name, sdef->elements[0].typegen
    );
  }
  /* Then, the USE-ORDER member as an ordinary member when !EXER */
  if (sdef->xerUseOrderPossible) {
    src = mputprintf(src,
      "  if (!e_xer && (p_td.xer_bits & USE_ORDER)) {\n"
      "    sub_len += field_%s.XER_encode(%s_xer_, p_buf, p_flavor, p_flavor2, p_indent+1, 0);\n"
      "  }\n"
      , sdef->elements[uo].name, sdef->elements[uo].typegen
    );
  }

  if (start_at+num_attributes > sdef->nElements) FATAL_ERROR("defRecordClass1");

  if (want_namespaces && (start_at < sdef->nElements)) {
    src = mputstr(src,
      "  if (e_xer && num_collected) {\n"
      "    size_t num_ns;\n"
      "    for (num_ns = 0; num_ns < num_collected; ++num_ns) {\n"
      "      p_buf.put_s(strlen(collected_ns[num_ns]), (cbyte*)collected_ns[num_ns]);\n"
      "      Free(collected_ns[num_ns]);\n"
      "    }\n"
      "    Free(collected_ns);\n"
      "  }\n\n"
      "  if (def_ns) {\n"
      "    p_flavor &= ~DEF_NS_SQUASHED;\n"
      "    p_flavor |=  DEF_NS_PRESENT;\n"
      "  }\n"
      "  else if (empty_ns_hack) {\n"
      "    p_buf.put_s(9, (cbyte*)\" xmlns=''\");\n"
      "    p_flavor &= ~DEF_NS_PRESENT;\n"
      "    p_flavor |=  DEF_NS_SQUASHED;\n"
      "  }\n");
  }

  /* Then, all the attributes (not added to sub_len) */
  for ( i = start_at; i < start_at + num_attributes; ++i ) {
    /* If the first component is a record-of with ANY-ATTRIBUTES,
     * it has been taken care of because it looked like an EMBED-VALUES */
    if (i==0 && sdef->xerEmbedValuesPossible && (sdef->elements[i].xerAnyKind & ANY_ATTRIB_BIT)) continue ;
    src = mputprintf(src, 
      "  ec_1.set_msg(\"%s': \");\n"
      "  tmp_len = field_%s.XER_encode(%s_xer_, p_buf, p_flavor, p_flavor2, p_indent+1, 0);\n"
      "  %ssub_len += tmp_len;\n" /* do not add if attribute and EXER */
      , sdef->elements[i].dispname
      , sdef->elements[i].name, sdef->elements[i].typegen
      , sdef->elements[i].xerAttribute || (sdef->elements[i].xerAnyKind & ANY_ATTRIB_BIT) ? "if (!e_xer) " : ""
    );
  }

  if (sdef->xerUseNilPossible) {
    src = mputprintf(src,
      "  boolean nil_attribute = e_xer && (p_td.xer_bits & USE_NIL) && !field_%s.ispresent();\n"
      "  if (nil_attribute) {\n"
      "    const namespace_t *control_ns = p_td.my_module->get_controlns();\n"
      "    p_buf.put_c(' ');\n"
      "    p_buf.put_s(strlen(control_ns->px), (cbyte*)control_ns->px);\n"
      "    p_buf.put_c(':');\n"
      "    p_buf.put_s(10, (cbyte*)\"nil='true'\");\n"
      "  }\n"
      , sdef->elements[sdef->nElements-1].name
    );
  }
  
  if (want_namespaces) {
    /* there were some attributes. close the start tag left open */
    src = mputprintf(src,
      "  if (delay_close && (!omit_tag || chopped_chars)) p_buf.put_s(1%s, (cbyte*)\">\\n\");\n", /* close the start tag */
      (sdef->xerUntaggedOne /*|| sdef->xerUseNil*/) ? "" : "+is_indented"
    );
  }

  if (sdef->xerEmbedValuesPossible) {
    src = mputprintf(src,
      "  ec_1.set_msg(\"%s': \");\n"
      "  if (e_xer && (p_td.xer_bits & EMBED_VALUES)) {\n"
    /* write the first string (must come AFTER the attributes) */
      "    if (%s%s%s field_%s%s.size_of() > 0) {\n"
      "      sub_len += field_%s%s[0].XER_encode(UNIVERSAL_CHARSTRING_xer_, p_buf, p_flavor | EMBED_VALUES, p_flavor2, p_indent+1, 0);\n"
      "    }\n"
      "  }\n"
      , sdef->elements[0].dispname
      , sdef->elements[0].isOptional ? "field_" : ""
      , sdef->elements[0].isOptional ? sdef->elements[0].name : ""
      , sdef->elements[0].isOptional ? ".ispresent() &&" : ""
      , sdef->elements[0].name
      , sdef->elements[0].isOptional ? "()" : "", sdef->elements[0].name
      , sdef->elements[0].isOptional ? "()" : "");
    if (want_namespaces) { /* here's another chance */
      src = mputprintf(src,
        "  else if ( !(p_td.xer_bits & EMBED_VALUES)) {\n"
        "    %sfield_%s.XER_encode(%s_xer_, p_buf, p_flavor, p_flavor2, p_indent+1, 0);\n"
        "  }\n"
        , ((sdef->elements[0].xerAnyKind & ANY_ATTRIB_BIT) ? "" : "sub_len += " )
        , sdef->elements[0].name, sdef->elements[0].typegen
      );
    }
  }

  /* Then, all the non-attributes. Structuring the code like this depends on
   * all attributes appearing before all non-attributes (excluding
   * special members for EMBED-VALUES, USE-ORDER, etc.) */
  if (sdef->xerEmbedValuesPossible) {
    src = mputprintf(src,
      "  embed_values_enc_struct_t* emb_val = 0;\n"
      "  if (e_xer && (p_td.xer_bits & EMBED_VALUES) && "
      "    %s%s%s field_%s%s.size_of() > 1) {\n"
      "    emb_val = new embed_values_enc_struct_t;\n"
      /* If the first field is a record of ANY-ELEMENTs, then it won't be a pre-generated
       * record of universal charstring, so it needs a cast to avoid a compilation error */
      "    emb_val->embval_array%s = (const PreGenRecordOf::PREGEN__RECORD__OF__UNIVERSAL__CHARSTRING%s*)&field_%s%s;\n"
      "    emb_val->embval_array%s = NULL;\n"
      "    emb_val->embval_index = 1;\n"
      "  }\n"
      , sdef->elements[0].isOptional ? "field_" : ""
      , sdef->elements[0].isOptional ? sdef->elements[0].name : ""
      , sdef->elements[0].isOptional ? ".ispresent() &&" : ""
      , sdef->elements[0].name
      , sdef->elements[0].isOptional ? "()" : ""
      , sdef->elements[0].optimizedMemAlloc ? "_opt" : "_reg"
      , sdef->elements[0].optimizedMemAlloc ? "__OPTIMIZED" : ""
      , sdef->elements[0].name
      , sdef->elements[0].isOptional ? "()" : ""
      , sdef->elements[0].optimizedMemAlloc ? "_reg" : "_opt");
  }
  
  if (sdef->xerUseOrderPossible) {
    int max_ordered = sdef->nElements - start_at - num_attributes;
    int min_ordered = max_ordered - n_opt_elements;
    size_t offset = 0;
    size_t base   = i;
    size_t limit  = sdef->nElements;
    /* base and limit are indexes into sdef->elements[] */
    src = mputprintf(src,
      "  int dis_order = e_xer && (p_td.xer_bits & USE_ORDER);\n"
      "  int to_send = field_%s.lengthof();\n"
      "  int uo_limit = dis_order ? to_send : %lu;\n"
      , sdef->elements[start_at-1].name
      , (unsigned long)(limit - base)
    );
    if (sdef->xerUseNilPossible) { /* USE-NIL on top of USE-ORDER */
      base  = sdef->nElements;
      limit = sdef->totalElements;
      min_ordered = max_ordered = limit - base;
      src = mputprintf(src,
        "  if (!nil_attribute) {\n"
        "%s"
        "  if (!e_xer) sub_len += field_%s.XER_encode(%s_xer_, p_buf, p_flavor, p_flavor2, p_indent+!omit_tag, 0);\n"
        "  else" /* no newline */
        , (sdef->xerUseNilPossible ? "  if (!(p_td.xer_bits & USE_ORDER)) p_flavor |= (p_td.xer_bits & USE_NIL);\n" : "")
        /* If USE-ORDER is on, the tag-removing effect of USE-NIL has been
         * performed by calling the sub-fields directly. */
        , sdef->elements[sdef->nElements-1].name
        , sdef->elements[sdef->nElements-1].typegen
      );
    }

    /* check incorrect data */
    src = mputprintf(src,
      "  {\n"
      "  if (to_send < %d || to_send > %d) {\n"
      "    ec_1.set_msg(\"%s': \");\n"
      "    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_CONSTRAINT, "
      "\"Wrong number of USE-ORDER, %%d instead of %d..%d\", to_send);\n"
      "    uo_limit = -1;\n" /* squash the loop */
      "  }\n"
      "  else {\n" /* check duplicates */
      "    int *seen = new int [to_send];\n"
      "    int num_seen = 0;\n"
      "    for (int ei = 0; ei < to_send; ++ei) {\n"
      "      int val = field_%s[ei];\n"
      "      for (int x = 0; x < num_seen; ++x) {\n"
      "        if (val == seen[x]) { // complain\n"
      "          ec_1.set_msg(\"%s': \");\n"
      "          TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_CONSTRAINT,\n"
      "            \"Duplicate value for USE-ORDER\");\n"
      "          uo_limit = -1; // don't bother sending anything\n"
      "          goto trouble;\n"
      "        }\n"
      "      }\n"
      "      seen[num_seen++] = val;\n"
      "    }\n"
      "trouble:\n"
      "    delete [] seen;"
      "  }\n"
      "  "
      , min_ordered, max_ordered
      , sdef->elements[start_at-1].dispname
      , min_ordered, max_ordered
      , sdef->elements[start_at-1].name
      , sdef->elements[start_at-1].dispname
    );

    src = mputprintf(src,
      "  for (int i = 0; i < uo_limit; ++i) {\n"
      "    switch (dis_order ? (int)field_%s[i] : i) {\n"
      , sdef->elements[start_at-1].name
    );
    for ( i = base; i < limit; ++i ) {
      src = mputprintf(src,
        "    case %lu:\n"
        "      ec_1.set_msg(\"%s': \");\n"
        "      sub_len += field_%s%s%s%s.XER_encode(%s_xer_, p_buf, p_flavor, p_flavor2, p_indent+!omit_tag, %s);\n"

        , (unsigned long)offset++
        , sdef->elements[i].dispname
        , (sdef->xerUseNilPossible ? sdef->elements[sdef->nElements-1].name : sdef->elements[i].name)
        , (sdef->xerUseNilPossible ? "()." : "")
        , (sdef->xerUseNilPossible ? sdef->elements[i].name : "")
        , (sdef->xerUseNilPossible ? "()" : "")
        , sdef->elements[i].typegen
        , sdef->xerEmbedValuesPossible ? "emb_val" : "0"
      );

      src = mputstr(src, "    break;\n");
    } /* next element */
    src = mputstr(src,
      "    default:\n"
      "      TTCN_error(\"Runaway value while encoding USE-ORDER\");\n"
      "      break;\n" /* cannot happen, complain */
      "    }\n");

    if (sdef->xerEmbedValuesPossible) {
      src = mputprintf(src,
        "    if (e_xer && (p_td.xer_bits & EMBED_VALUES) && 0 != emb_val &&\n"
        "        %s%s%s emb_val->embval_index < field_%s%s.size_of()) { // embed-val\n"
        "      field_%s%s[emb_val->embval_index].XER_encode(\n"
        "        UNIVERSAL_CHARSTRING_xer_, p_buf, p_flavor | EMBED_VALUES, p_flavor2, p_indent+1, 0);\n"
        "      ++emb_val->embval_index;\n"
        "    }\n"
        , sdef->elements[0].isOptional ? "field_" : ""
        , sdef->elements[0].isOptional ? sdef->elements[0].name : ""
        , sdef->elements[0].isOptional ? ".ispresent() &&" : ""
        , sdef->elements[0].name
        , sdef->elements[0].isOptional ? "()" : "", sdef->elements[0].name
        , sdef->elements[0].isOptional ? "()" : "");
    }


    src = mputstr(src,
      "  }\n" /* end of loop */
      "  } // exer\n");
    if (sdef->xerUseNilPossible) {
      src = mputstr(src, "  } // nil_attr\n");
    }
  }
  else /* not USE-ORDER */
    for ( /* continue with i */; i < sdef->nElements; ++i ) {
      src = mputprintf(src,
        "  ec_1.set_msg(\"%s': \");\n"
        , sdef->elements[i].dispname
      );
      if (i > start_at + num_attributes) { // First field's embed value is already processed
        src = mputprintf(src, 
          "if (e_xer && ((p_td.xer_bits & UNTAGGED) && !(p_td.xer_bits & EMBED_VALUES) && 0 != emb_val_parent\n"
          "  %s%s%s)) {\n"
          "  if (0 != emb_val_parent->embval_array_reg) {\n"
          "    if (emb_val_parent->embval_index < emb_val_parent->embval_array_reg->size_of()) {\n"
          "      (*emb_val_parent->embval_array_reg)[emb_val_parent->embval_index].XER_encode(UNIVERSAL_CHARSTRING_xer_"
          "      , p_buf, p_flavor | EMBED_VALUES, p_flavor2, p_indent+1, 0);\n"
          "      ++emb_val_parent->embval_index;\n"
          "    }\n"
          "  } else {\n"
           "   if (emb_val_parent->embval_index < emb_val_parent->embval_array_opt->size_of()) {\n"
          "      (*emb_val_parent->embval_array_opt)[emb_val_parent->embval_index].XER_encode(UNIVERSAL_CHARSTRING_xer_"
          "      , p_buf, p_flavor | EMBED_VALUES, p_flavor2, p_indent+1, 0);\n"
          "      ++emb_val_parent->embval_index;\n"
          "    }\n"
          "  }\n"
          "}\n"
          , sdef->elements[0].isOptional ? "&& field_" : ""
          , sdef->elements[0].isOptional ? sdef->elements[0].name : ""
          , sdef->elements[0].isOptional ? ".ispresent()" : ""
        );
      }
      src = mputprintf(src,
        "  sub_len += field_%s.XER_encode(%s_xer_, p_buf, p_flavor%s, p_flavor2, p_indent+!omit_tag, %s);\n"
        , sdef->elements[i].name, sdef->elements[i].typegen
        , sdef->xerUseNilPossible ? "| (p_td.xer_bits & USE_NIL)" : ""
        , sdef->xerEmbedValuesPossible ? "emb_val" : "0"
      );

      if (sdef->xerEmbedValuesPossible) {
        src = mputprintf(src,
          "  if (e_xer && (p_td.xer_bits & EMBED_VALUES) && 0 != emb_val &&\n"
          "      %s%s%s %s%s%s emb_val->embval_index < field_%s%s.size_of()) {\n"
          "    field_%s%s[emb_val->embval_index].XER_encode(\n"
          "      UNIVERSAL_CHARSTRING_xer_, p_buf, p_flavor | EMBED_VALUES, p_flavor2, p_indent+1, 0);\n"
          "    ++emb_val->embval_index;\n"
          "  }\n"
          , sdef->elements[0].isOptional ? "field_" : ""
          , sdef->elements[0].isOptional ? sdef->elements[0].name : ""
          , sdef->elements[0].isOptional ? ".ispresent() &&" : ""
          , sdef->elements[i].isOptional ? "field_" : ""
          , sdef->elements[i].isOptional ? sdef->elements[i].name : ""
          , sdef->elements[i].isOptional ? ".ispresent() &&" : ""
          , sdef->elements[0].name
          , sdef->elements[0].isOptional ? "()" : "", sdef->elements[0].name
          , sdef->elements[0].isOptional ? "()" : "");
      }
    } /* next field when not USE-ORDER */
  
  if (i <= start_at + num_attributes + 1 || sdef->xerUseOrderPossible) {
    src = mputprintf(src, "  (void)emb_val_parent;\n"); // Silence unused warning
  }
  
  if (sdef->xerEmbedValuesPossible) {
    src = mputprintf(src,
      "  if (0 != emb_val) {\n"
      "    if (%s%s%s emb_val->embval_index < field_%s%s.size_of()) {\n"
      "      ec_1.set_msg(\"%s': \");\n"
      "      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_CONSTRAINT,\n"
      "        \"Too many EMBED-VALUEs specified: %%d (expected %%d or less)\",\n"
      "        field_%s%s.size_of(), emb_val->embval_index);\n"
      "    }\n"
      "    delete emb_val;\n"
      "  }\n"
      , sdef->elements[0].isOptional ? "field_" : ""
      , sdef->elements[0].isOptional ? sdef->elements[0].name : ""
      , sdef->elements[0].isOptional ? ".ispresent() &&" : ""
      , sdef->elements[0].name, sdef->elements[0].isOptional ? "()" : ""
      , sdef->elements[0].name, sdef->elements[0].name
      , sdef->elements[0].isOptional ? "()" : "");
  }

  src = mputstr(src, "  } // QN?\n");

  {
    src = mputprintf(src,
      "  if (!omit_tag) {\n"
      "  if (sub_len) {\n" /* something was written, now an end tag */
      "    if (is_indented && !(e_xer && (p_td.xer_bits & (HAS_1UNTAGGED | USE_QNAME)))) {\n"
      "      switch ((int)(e_xer && (p_td.xer_bits & USE_NIL))) {\n"
      "      case 1: {\n"
      "        const unsigned char *buf_end = p_buf.get_data() + (p_buf.get_len()-1);\n"
      "        if (buf_end[-1] != '>' || *buf_end != '\\n') break;\n"
      /*       else fall through */
      "      }\n"
      "      case 0:\n"
      "        do_indent(p_buf, p_indent);\n"
      "        break;\n"
      "      }\n"
      "    }\n"
      "    p_buf.put_c('<');\n"
      "    p_buf.put_c('/');\n"
      "    if (e_xer) write_ns_prefix(p_td, p_buf);\n"
      "    p_buf.put_s((size_t)p_td.namelens[e_xer]-!is_indented, (cbyte*)p_td.names[e_xer]);\n"
      "  } else {\n" /* need to generate an empty element tag */
      "    p_buf.increase_length(%s-1);\n" /* decrease length */
      "    p_buf.put_s((size_t)2+is_indented, (cbyte*)\"/>\\n\");\n"
      "  }}\n"
      , (sdef->xerUntaggedOne /*|| sdef->xerUseNil*/) ? "" : "-is_indented"
    );
  }
  src = mputstr(src,
    "  return (int)p_buf.get_len() - encoded_length;\n"
    "}\n\n");

#ifndef NDEBUG
  src = mputprintf(src, "// %s has%s%s%s%s%s%s%s%s%s\n"
    "// written by %s in " __FILE__ " at %d\n"
    , name
    , (sdef->xerUntagged            ? " UNTAGGED" : "")
    , (sdef->xerUntaggedOne         ? "1" : "")
    , (sdef->xerUseNilPossible      ? " USE_NIL?" : "")
    , (sdef->xerUseOrderPossible    ? " USE_ORDER?" : "")
    , (sdef->xerUseQName            ? " USE_QNAME" : "")
    , (sdef->xerUseTypeAttr         ? " USE_TYPE_ATTR" : "")
    , (sdef->xerUseUnion            ? " USE-UNION" : "")
    , (sdef->xerHasNamespaces       ? " namespace" : "")
    , (sdef->xerEmbedValuesPossible ? " EMBED?" : "")
    , __FUNCTION__, __LINE__
  );
#endif

  src = mputprintf(src, /* XERSTUFF decodegen for record/SEQUENCE*/
    "int %s::XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap& p_reader,"
    " unsigned int p_flavor, unsigned int p_flavor2, embed_values_dec_struct_t* emb_val_parent)\n"
    "{\n"
    /* Remove XER_LIST, XER_RECOF from p_flavor. This is not required
     * for is_exer (which tests another bit), but for subsequent code. */
    "  boolean e_xer = is_exer(p_flavor);\n"
    "  unsigned long xerbits = p_td.xer_bits;\n"
    "  if (p_flavor & XER_TOPLEVEL) xerbits &= ~UNTAGGED;\n"
    "  const boolean omit_tag = e_xer && ((xerbits & (UNTAGGED|XER_ATTRIBUTE)) "
    "|| (p_flavor & (USE_NIL|USE_TYPE_ATTR)));\n"
    "%s"
    "  const boolean parent_tag = e_xer && ((p_flavor & USE_TYPE_ATTR)|| (p_flavor2 & USE_NIL_PARENT_TAG));\n"
    "  (void)parent_tag;\n"
    "  p_flavor &= XER_MASK;\n" /* also removes "toplevel" bit */
    "  p_flavor2 = XER_NONE;\n" /* Remove only bit: USE_NIL_PARENT_TAG (for now) */
    "  int rd_ok, xml_depth=-1, type;\n"
    "  {\n" /* scope for the error contexts */
    , name
    , (start_at + num_attributes < sdef->nElements || (sdef->xerEmbedValuesPossible && num_attributes==0)
      ? "  boolean tag_closed = (p_flavor & PARENT_CLOSED) != 0;\n" : "")
    /* tag_closed needed for non-attribute normal members only,
     * or if EMBED-VALUES is possible, but isn't. */
  );

  if (sdef->xerUseNilPossible) { src = mputstr(src,
    "  boolean nil_attribute = FALSE;\n"
    "  boolean already_processed = FALSE;\n");
  }

  src = mputprintf(src,
    "  TTCN_EncDec_ErrorContext ec_0(\"Component '\");\n"
    "  TTCN_EncDec_ErrorContext ec_1;\n"
    "  if (!omit_tag) for (rd_ok=p_reader.Ok(); rd_ok==1; rd_ok=p_reader.Read()) {\n"
    "    type = p_reader.NodeType();\n"
    "    if (type==XML_READER_TYPE_ELEMENT) {\n"
    "      verify_name(p_reader, p_td, e_xer);\n"
    "      xml_depth = p_reader.Depth();\n"
    "%s"
    "      break;\n"
    "    }\n"
    "  }\n"
    , (start_at + num_attributes < sdef->nElements || (sdef->xerEmbedValuesPossible && num_attributes==0)
      ?  "      tag_closed = p_reader.IsEmptyElement();\n": "")
  );


  if (sdef->xerUseQName) {
    src = mputprintf(src,
      "  if (e_xer && (p_td.xer_bits & USE_QNAME)) {\n"
      "    if (p_td.xer_bits & XER_ATTRIBUTE) rd_ok = 1;\n"
      "    else for (rd_ok = p_reader.Read(); rd_ok == 1; rd_ok = p_reader.Read()) {\n"
      "      type = p_reader.NodeType();\n"
      "      if (type == XML_READER_TYPE_TEXT) break;\n"
      "    }\n"
      "    if (rd_ok==1) {\n"
      "      xmlChar *new_val = p_reader.NewValue();\n"
      "      xmlChar *v_npfx = (xmlChar*)strchr((char*)new_val, ':');\n"
      "      xmlChar *v_pfx;\n"
      "      if (v_npfx != NULL) {\n"
      "        *v_npfx++ = '\\0';\n"
      "        v_pfx = new_val;\n"
      "      }\n"
      "      else {\n"
      "        v_npfx = new_val;\n"
      "        v_pfx  = NULL;\n"
      "      }\n"
      "      xmlChar *q_ns = p_reader.LookupNamespace(v_pfx);\n"
      "      if (q_ns) field_%s = (const char*)q_ns;\n"
      "      else      field_%s = OMIT_VALUE;\n"
      "      field_%s = (const char*)v_npfx;\n"
      "      xmlFree(q_ns);\n"
      "      xmlFree(new_val);\n"
      "    }\n"
      "  } else {\n"
      , sdef->elements[0].name
      , sdef->elements[0].name
      , sdef->elements[1].name
    );
  } /* QName */

  if (num_attributes || sdef->xerEmbedValuesPossible || sdef->xerUseOrderPossible) {
    src = mputstr(src, "if (e_xer) {\n");
  }

  /* ********************************************************************
   * ATTRIBUTES
   ***************************/ 
  if (num_attributes || sdef->xerUseNilPossible /* maybe QNAME too ? */) {
    size_t aaa;

     /* Prepare for attributes not present in the XML.
     * Set all attributes with defaultForEmpty to the D-F-E value.
     * Set all optional components with ATTRIBUTE to omit.
     *
     * If EMBED-VALUES is possible, the first component can't be optional.
     * The same is true for the USE-ORDER component. Hence start_at. */
    for (aaa = start_at; aaa < start_at + num_attributes; ++aaa) {
      if (sdef->elements[aaa].xerAttribute) { /* "proper" ATTRIBUTE */
        src = mputprintf(src,
          "  if (%s_xer_.dfeValue) field_%s = "
          "*static_cast<const %s*>(%s_xer_.dfeValue);\n"
          , sdef->elements[aaa].typegen, sdef->elements[aaa].name
          , sdef->elements[aaa].type   , sdef->elements[aaa].typegen);
        if (sdef->elements[aaa].isOptional) src = mputprintf(src,
          "  else field_%s = OMIT_VALUE;\n", sdef->elements[aaa].name);
      }
      else { /* must be the ANY-ATTRIBUTES */
        src = mputprintf(src,
          "  field_%s%s;\n"
        , sdef->elements[aaa].name
        , sdef->elements[aaa].isOptional ? " = OMIT_VALUE" : ".set_size(0)");
      }
    }
    
    src = mputstr(src, " if (!omit_tag || parent_tag) {\n");

    if (num_attributes==0 /* therefore sdef->xerUseNilPossible is true */ ) {
      /* Only the "nil" attribute may be present. If there is no USE-NIL,
       * then there can be no attributes at all. */
      src = mputstr(src,
        "  if (e_xer && (p_td.xer_bits & (USE_NIL|USE_TYPE_ATTR))) {\n");
    }

    if (aa_index > -1) src = mputstr(src, "  size_t num_aa = 0;\n");
    if (sdef->xerUseNilPossible) {
      src = mputstr(src,
        "  static const namespace_t *control_ns = p_td.my_module->get_controlns();\n");
    }

    src = mputstr(src,
      "  if(parent_tag && p_reader.NodeType() == XML_READER_TYPE_ATTRIBUTE) {\n"
      "    rd_ok = p_reader.Ok();\n"
      "  } else {\n"
      "    rd_ok = p_reader.MoveToFirstAttribute();\n"
      "  }\n"
      "  for (; rd_ok==1 && "
      "p_reader.NodeType()==XML_READER_TYPE_ATTRIBUTE; "
      "rd_ok = p_reader.AdvanceAttribute()) {\n"
      "    if (p_reader.IsNamespaceDecl()) continue;\n");
    /* if the only attribute is ANY-ATTRIBUTE, it doesn't need attr_name */
    if (num_attributes==1 && aa_index!=-1 /*&& !sdef->xerUseNilPossible*/) {}
    else {
      src = mputstr(src,
        "    const char *attr_name = (const char*)p_reader.LocalName();\n"
        "    const char *ns_uri    = (const char*)p_reader.NamespaceUri();\n");
    }

    if (sdef->xerUseNilPossible) {
      src = mputprintf(src,
        "    const char *prefix = (const char*)p_reader.Prefix();\n"
        /* prefix may be NULL, control_ns->px is never NULL or empty */
        "    if (prefix && !strcmp(prefix, control_ns->px)\n"
        "      && !strcmp((const char*)p_reader.LocalName(), \"nil\")){\n"
        "      const char *value = (const char*)p_reader.Value();\n"
        "      if (!strcmp(value, \"1\") || !strcmp(value, \"true\")) {\n"
        "        field_%s = OMIT_VALUE;\n"
        "        nil_attribute = TRUE;\n"
        /* found the "nil" attribute */
        "      }\n"
        "    } else"
        , sdef->elements[sdef->nElements-1].name);
    }

    for (i = start_at; i < start_at + num_attributes; ++i) {
      if (i == aa_index) continue; /* ANY_ATTR. is handled below */
      src = mputprintf(src,
        "    if (check_name(attr_name, %s_xer_, 1) && check_namespace(ns_uri, %s_xer_)) {\n"
        "      ec_1.set_msg(\"%s': \");\n"
        "      field_%s.XER_decode(%s_xer_, p_reader, p_flavor | (p_td.xer_bits & USE_NIL), p_flavor2, 0);\n"
        "    } else"
        , sdef->elements[i].typegen, sdef->elements[i].typegen
        , sdef->elements[i].dispname /* set_msg */
        , sdef->elements[i].name, sdef->elements[i].typegen
      );
    }
    
    if(sdef->xerUseNilPossible) {
      src = mputprintf(src,
        "    if(p_td.xer_bits & USE_NIL) {\n"
        "      field_%s.XER_decode(%s_xer_, p_reader, p_flavor | USE_NIL, p_flavor2 | USE_NIL_PARENT_TAG, 0);\n"
        "      already_processed = TRUE;\n"
        "      break;\n"
        "    } else"
        , sdef->elements[sdef->nElements-1].name
        , sdef->elements[sdef->nElements-1].typegen
      );
    }

    if (sdef->control_ns_prefix && !(num_attributes==1 && aa_index!=-1)) {
      src = mputprintf(src,
        "    if (/*parent_tag &&*/ !strcmp(attr_name, \"type\") "
        "&& !strcmp((const char*)p_reader.Prefix(), \"%s\")) {} else\n"
        , sdef->control_ns_prefix);
      /* xsi:type; already processed by parent with USE-UNION or USE-TYPE */
    }


    if (aa_index >= 0) {
      /* we are at a dangling else */
      src = mputprintf(src,
        "    {\n"
        "      TTCN_EncDec_ErrorContext ec_0(\"Attribute %%d: \", (int)num_aa);\n"
        "      UNIVERSAL_CHARSTRING& new_elem = field_%s%s[num_aa++];\n"
        /* Construct the AnyAttributeFormat (X.693amd1, 18.2.6) */
        "      TTCN_Buffer aabuf;\n"
        "      const xmlChar *x_name = p_reader.LocalName();\n"
        "      const xmlChar *x_val  = p_reader.Value();\n"
        "      const xmlChar *x_uri  = p_reader.NamespaceUri();\n"
        "      if (%s_xer_.xer_bits & (ANY_FROM | ANY_EXCEPT)) {\n"
        "        check_namespace_restrictions(%s_xer_, (const char*)x_uri);\n"
        "      }\n"
        /* We don't care about p_reader.Prefix() */
        /* Using strlen to count bytes */
        "      aabuf.put_s(x_uri ? strlen((const char*)x_uri) : 0, x_uri);\n"
        "      if (x_uri && *x_uri) aabuf.put_c(' ');\n"
        "      aabuf.put_s(x_name ? strlen((const char*)x_name) : 0, x_name);\n"
        "      aabuf.put_c('=');\n"
        "      aabuf.put_c('\"');\n"
        "      aabuf.put_s(x_val ? strlen((const char*)x_val) : 0, x_val);\n"
        "      aabuf.put_c('\"');\n"
        "      new_elem.decode_utf8(aabuf.get_len(), aabuf.get_data());\n"
        "    } \n"
        , sdef->elements[aa_index].name
        , sdef->elements[aa_index].isOptional ? "()" : ""
        , sdef->elements[aa_index].typegen, sdef->elements[aa_index].typegen
      );
    }
    else {
      /* we are at a dangling else */
      src = mputstr(src, "    {\n");
      if (sdef->control_ns_prefix) {
        src = mputprintf(src,
          // Lastly check for the xsi:schemaLocation attribute, this does not
          // affect TTCN-3, but it shouldn't cause a DTE
          "      if (!p_reader.LocalName() || strcmp((const char*)p_reader.LocalName(), \"schemaLocation\") ||\n"
          "          !p_reader.Prefix() || strcmp((const char*)p_reader.Prefix(), \"%s\"))\n"
          , sdef->control_ns_prefix);
      }
      src = mputstr(src,
        "      {\n"
        "        ec_0.set_msg(\" \"); ec_1.set_msg(\" \");\n" /* we have no component */
        "        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG, "
        "\"Unexpected attribute '%s', ns '%s'\", attr_name, ns_uri ? ns_uri : \"\");\n"
        "      }\n"
        "    }\n");
    }
    src = mputstr(src, "  }\n"); /* for */

    for (i = start_at; i < start_at + num_attributes; ++i) {
      if (sdef->elements[i].isOptional) continue; /* allowed to be missing */
      src = mputprintf(src, "  if (!field_%s.is_bound()) "
        "TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG, "
        "\"Missing attribute '%s'\");\n"
        , sdef->elements[i].name, sdef->elements[i].dispname);
    }

    if (num_attributes==0) src = mputstr(src,  "  }\n"); /* if USE_NIL or USE_TYPE_ATTR */
    src = mputstr(src,  " }\n"); /* process (attributes) in (own or parent) tag */
  } /* * * * * * * * * end if(attributes...) * * * * * * * * * * * * */

  src = mputprintf(src,
    "  if ((!omit_tag || parent_tag) && !p_reader.IsEmptyElement()%s) "
    "rd_ok = p_reader.Read();\n"
    , sdef->xerUseNilPossible ? " && !already_processed" : "");

  if (sdef->xerEmbedValuesPossible && num_attributes==0) {
    /* EMBED-VALUES possible, but isn't: the first component is a non-special
     * record of strings. This means there are no attributes, unless
     * the first component is an ATTRIBUTE or ANY-ATTRIBUTES. */
    src = mputprintf(src,
      "  if (!(p_td.xer_bits & EMBED_VALUES)) {\n"
      "    ec_1.set_msg(\"%s': \");\n"
      "    field_%s.XER_decode(%s_xer_, p_reader, "
      "p_flavor | (p_td.xer_bits & USE_NIL)| (tag_closed ? PARENT_CLOSED : XER_NONE), p_flavor2, 0);\n"
      "  }\n"
      , sdef->elements[0].dispname
      , sdef->elements[0].name, sdef->elements[0].typegen
    );
  }

  if (num_attributes || sdef->xerEmbedValuesPossible || sdef->xerUseOrderPossible) {
    src = mputstr(src,
      "} else {\n" /* now the non-EXER processing of (would-be) attributes */
      "  if (!p_reader.IsEmptyElement()) p_reader.Read();\n"
    );
  }

  if (sdef->xerUseOrderPossible) {
    src = mputstr(src,
      "  if (e_xer && p_td.xer_bits & USE_ORDER) ; else {");
  }

  if (start_at + num_attributes > 0)
  {
    /* No attributes nor USE-NIL:
     * Just decode the specials and would-be attributes. */

    // FIXME: WTF is this?
    //if (sdef->xerEmbedValuesPossible) {
    //  /* Only decode the first member if it really is EMBED-VALUES */
    //  src = mputprintf(src, "if (p_td.xer_bits & EMBED_VALUES) ");
    //}
    for (i = 0; i < start_at + num_attributes; ++i) {
      /* Decode the field */
      src = mputprintf(src,
        "  {\n"
        "    ec_1.set_msg(\"%s': \");\n"
        "    field_%s.XER_decode(%s_xer_, p_reader, p_flavor | (p_td.xer_bits & USE_NIL), p_flavor2, 0);\n"
        "  }\n"
        , sdef->elements[i].dispname
        , sdef->elements[i].name, sdef->elements[i].typegen
      );
    } /* next field */

    if (sdef->xerUseOrderPossible) {
      src = mputstr(src, "  }\n");
    }
  }

  if (num_attributes || sdef->xerEmbedValuesPossible || sdef->xerUseOrderPossible) {
    src = mputstr(src, "}\n");
  }
  
  if (sdef->xerEmbedValuesPossible) {
    src = mputprintf(src,
      "  embed_values_dec_struct_t* emb_val = 0;\n"
      "  if (e_xer && (p_td.xer_bits & EMBED_VALUES)) {\n"
      "    emb_val = new embed_values_dec_struct_t;\n"
      /* If the first field is a record of ANY-ELEMENTs, then it won't be a pre-generated
       * record of universal charstring, so it needs a cast to avoid a compilation error */
      "    emb_val->embval_array%s = (PreGenRecordOf::PREGEN__RECORD__OF__UNIVERSAL__CHARSTRING%s*)&field_%s%s;\n"
      "    emb_val->embval_array%s = NULL;\n"
      "    emb_val->embval_index = 0;\n"
      "    field_%s%s.set_size(0);\n"
      "  }\n"
      , sdef->elements[0].optimizedMemAlloc ? "_opt" : "_reg"
      , sdef->elements[0].optimizedMemAlloc ? "__OPTIMIZED" : ""
      , sdef->elements[0].name
      , sdef->elements[0].isOptional ? "()" : ""
      , sdef->elements[0].optimizedMemAlloc ? "_reg" : "_opt"
      , sdef->elements[0].name
      , sdef->elements[0].isOptional ? "()" : "");
  }

  if (sdef->xerUseOrderPossible) {
    size_t begin = start_at + num_attributes; /* first non-attribute */
    size_t end   = sdef->nElements;
    size_t n_optionals = 0, n_embed;
    int max_ordered = sdef->nElements - start_at - num_attributes;
    int min_ordered = max_ordered - n_opt_elements;
    if (sdef->xerUseNilPossible) { /* USE-NIL on top of USE-ORDER */
      begin = sdef->nElements;
      end   = sdef->totalElements;
    }
    n_embed = end - begin;

    src = mputstr(src,
      "  if (e_xer && (p_td.xer_bits & USE_ORDER)) {\n"
    );
    for (i = begin; i < end; ++i) {
      if (sdef->elements[i].isOptional) {
        src = mputprintf(src, "    field_%s = OMIT_VALUE;\n",
          sdef->elements[i].name);
        ++n_optionals;
      }
    }

    if (sdef->xerUseNilPossible) { /* USE-NIL and USE-ORDER */
      src = mputprintf(src,
        "    if (nil_attribute) field_%s.set_size(0);\n    else"
        , sdef->elements[uo].name);
    }

    src = mputprintf(src,
      "    {\n"
      "    field_%s.set_size(0);\n"
      "    if (!tag_closed) {\n" /* Nothing to order if there are no child elements */
      "    int e_val, num_seen = 0, *seen_f = new int[%lu];\n"
      , sdef->elements[uo].name
      , (unsigned long)(n_embed)
    );
    if (sdef->xerEmbedValuesPossible) {
      // The index of the latest embedded value can change outside of this function
      // (if the field is a untagged record of), in this case the next value should
      // be ignored, as it's already been handled by the record of
      src = mputstr(src, "    int last_embval_index = 0;\n");
    }
    src = mputprintf(src,
      "    boolean early_exit = FALSE;\n"
      "    for (int i=0; i < %lu; ++i) {\n"
      "      for (rd_ok=p_reader.Ok(); rd_ok==1; rd_ok=p_reader.Read()) {\n"
      , (unsigned long)(n_embed));

    if (sdef->xerEmbedValuesPossible) {
      /* read and store embedValues text if present */
      src = mputprintf(src,
        "        if (0 != emb_val && p_reader.NodeType()==XML_READER_TYPE_TEXT) {\n"
        "          UNIVERSAL_CHARSTRING emb_ustr((const char*)p_reader.Value());\n"
        "          field_%s%s[emb_val->embval_index] = emb_ustr;\n"
        "        }\n"
        , sdef->elements[0].name
        , sdef->elements[0].isOptional ? "()" : "");
    }

    src = mputstr(src,
      "        type = p_reader.NodeType();\n"
      "        if (type==XML_READER_TYPE_ELEMENT) break;\n"
      "        if (type == XML_READER_TYPE_END_ELEMENT) {\n"
      "          early_exit = TRUE;\n"
      "          break;\n"
      "        }\n"
      "      }\n"
      "      if (rd_ok != 1 || early_exit) break;\n"
      "      const char * x_name = (const char*)p_reader.LocalName();\n" /* Name or LocalName ? */);
    
    if (sdef->xerEmbedValuesPossible) {
      src = mputstr(src,
        "      if (0 != emb_val) {\n"
        "        if (last_embval_index == emb_val->embval_index) {\n"
        "          ++emb_val->embval_index;\n"
        "        }\n"
        "        last_embval_index = emb_val->embval_index;\n"
        "      }\n");
    }

    /* * * * * code for USE-ORDER * * * * */

    for (i = begin; i < end; ++i) {
      // Check non-anyElement fields first
      if (!(sdef->elements[i].xerAnyKind & ANY_ELEM_BIT)) {
        src = mputprintf(src,
          "      if (check_name(x_name, %s_xer_, 1)) {\n"
          "        ec_1.set_msg(\"%s': \");\n"
          "        field_%s%s%s%s.XER_decode(%s_xer_, p_reader, p_flavor, p_flavor2, %s);\n"
          , sdef->elements[i].typegen
          , sdef->elements[i].dispname
          , (sdef->xerUseNilPossible ? sdef->elements[sdef->nElements-1].name: sdef->elements[i].name)
          , (sdef->xerUseNilPossible ? "()." : "")
          , (sdef->xerUseNilPossible ? sdef->elements[i].name : "")
          , (sdef->xerUseNilPossible ? "()" : "")
          , sdef->elements[i].typegen
          , sdef->xerEmbedValuesPossible ? "emb_val" : "0"
        );
        src = mputprintf(src,
        "        field_%s[i] = e_val = %s::of_type::%s;\n"
        , sdef->elements[uo].name
        , sdef->elements[uo].typegen, sdef->elements[i].name);
        src = mputstr(src, "      }\n      else");
      }
    }
    src = mputstr(src, 
      " {\n"
      "        boolean any_found = FALSE;\n"
      "        if (!any_found)");
    for (i = begin; i < end; ++i) {
      // Check anyElement fields after all other fields
      if (sdef->elements[i].xerAnyKind & ANY_ELEM_BIT) {
        src = mputstr(src, " {\n");
        src = mputprintf(src,
          "          e_val = %s::of_type::%s;\n"
          , sdef->elements[uo].typegen, sdef->elements[i].name);
        src = mputprintf(src,
          "          boolean next_any = FALSE;\n"
          "          for (int d_f = 0; d_f < num_seen; ++d_f) {\n"
          "            if (e_val == seen_f[d_f]) {\n"
          "              next_any = TRUE;\n"
          "            }\n"
          "          }\n"
          "          if (!next_any) {\n"
          "            ec_1.set_msg(\"%s': \");\n"
          "            field_%s%s%s%s.XER_decode(%s_xer_, p_reader, p_flavor, p_flavor2, 0);\n"
          "            field_%s[i] = e_val;\n"
          "            any_found = TRUE;\n"
          "          }\n"
          "        }\n"
          "        if (!any_found)"
          , sdef->elements[i].dispname
          , (sdef->xerUseNilPossible ? sdef->elements[sdef->nElements-1].name: sdef->elements[i].name)
          , (sdef->xerUseNilPossible ? "()." : "")
          , (sdef->xerUseNilPossible ? sdef->elements[i].name : "")
          , (sdef->xerUseNilPossible ? "()" : "")
          , sdef->elements[i].typegen, sdef->elements[uo].name
        );
      }
    }

    src = mputstr(src,
      " {\n" /* take care of the dangling else */
      "          TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG,\n"
      "            \"Bad XML tag '%s' instead of a valid field\", x_name);\n"
      "          break;\n"
      "        }\n"
      "      }\n"
      "      for (int d_f = 0; d_f < num_seen; ++d_f)\n"
      "        if (e_val == seen_f[d_f])\n"
      "          TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_CONSTRAINT, \"Duplicate field\");\n"
      "      seen_f[num_seen++] = e_val;\n"
      "    } // next i\n");

    if (sdef->xerEmbedValuesPossible) {
      /* read and store embedValues text if present */
      src = mputprintf(src,
        "    if (0 != emb_val) {\n"
        "      if (p_reader.NodeType()==XML_READER_TYPE_TEXT) {\n"
        "        UNIVERSAL_CHARSTRING emb_ustr((const char*)p_reader.Value());\n"
        "        field_%s%s[emb_val->embval_index] = emb_ustr;\n"
        "      }\n"
        "      if (last_embval_index == emb_val->embval_index) {\n"
        "        ++emb_val->embval_index;\n"
        "      }\n"
        "    }\n"
        , sdef->elements[0].name
        , sdef->elements[0].isOptional ? "()" : "");
    }

    src = mputprintf(src,
      "    delete [] seen_f;\n"
      "    }\n"
      "    int n_collected = field_%s.size_of();\n"
      "    if (p_td.xer_bits & USE_NIL) {\n"
      "      ;\n"
      "    } else {\n"
      "      if (n_collected < %d || n_collected > %d) {\n"
      "        ec_0.set_msg(\" \");\n"
      "        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_CONSTRAINT, \"Wrong number of elements\");\n"
      "      }\n"
      "    }\n"
      "  } // !tag_closed\n"
      "  } else { // !uo\n"
      , sdef->elements[uo].dispname
      , min_ordered, max_ordered
    );
  } /* end if(UseOrder possible) */

  /* * * * * non-UseOrder code always written, executed when * * * *
   * * * * * UseOrder not possible or in the "else" clause * * * * */

  if (sdef->xerUseNilPossible) {
    /* value absent, nothing more to do */
    src = mputstr(src, 
      "  if (nil_attribute) {\n"
      "    p_reader.MoveToElement();\n"
      "  } else {\n");
  }
  if (sdef->xerEmbedValuesPossible) {
    // The index of the latest embedded value can change outside of this function
    // (if the field is a untagged record of), in this case the next value should
    // be ignored, as it's already been handled by the record of
    // Omitted fields can also reset this value
    src = mputstr(src, "  int last_embval_index = 0;\n");
  }
  /* for all the non-attribute fields... */
  for (i = start_at + num_attributes; i < sdef->nElements; ++i) {
    if (sdef->xerEmbedValuesPossible) {
      /* read and store embedValues text if present */
      src = mputprintf(src,
        "  if (0 != emb_val) {\n"
        "    if (p_reader.NodeType()==XML_READER_TYPE_TEXT) {\n"
        "      UNIVERSAL_CHARSTRING emb_ustr((const char*)p_reader.Value());\n"
        "      field_%s%s[emb_val->embval_index] = emb_ustr;\n"
        "    }\n"
        "    if (last_embval_index == emb_val->embval_index) {\n"
        "      ++emb_val->embval_index;\n"
        "    }\n"
        "    last_embval_index = emb_val->embval_index;\n"
        "  }\n"
        , sdef->elements[0].name
        , sdef->elements[0].isOptional ? "()" : "");
    }
    /* The DEFAULT-FOR-EMPTY member can not be involved with EMBED-VALUES,
     * so we can use the same pattern: optional "if(...) else" before {}
     * XXX check with single-element record where nElements-1 == 0 !! */
    if (sdef->kind == RECORD && i == sdef->nElements-1) {
      src = mputprintf(src,
        "  if (e_xer && p_td.dfeValue && p_reader.IsEmptyElement()) {\n"
        "    field_%s = *static_cast<const %s*>(p_td.dfeValue);\n"
        "  }\n"
        "  else%s"
        , sdef->elements[i].name, sdef->elements[i].type
        , sdef->xerUseNilPossible ? " if (!already_processed)" : "");
    }
    /* Decode the field */
    src = mputprintf(src,
      "  {\n"
      "    ec_1.set_msg(\"%s': \");\n"
      , sdef->elements[i].dispname);
    
    src = mputprintf(src, 
      "  if ((p_td.xer_bits & UNTAGGED) && 0 != emb_val_parent) {\n"
      "    if (p_reader.NodeType() == XML_READER_TYPE_TEXT) {\n"
      "      UNIVERSAL_CHARSTRING emb_ustr((const char*)p_reader.Value());\n"
      "      if (0 != emb_val_parent->embval_array_reg) {\n"
      "        (*emb_val_parent->embval_array_reg)[emb_val_parent->embval_index] = emb_ustr;\n"
      "      } else {\n"
      "        (*emb_val_parent->embval_array_opt)[emb_val_parent->embval_index] = emb_ustr;\n"
      "      }\n"
      "      ++emb_val_parent->embval_index;\n"
      "    }\n"
      "  }\n");
    
    if ( (sdef->elements[i].xerAnyKind & ANY_ELEM_BIT)
         && !strcmp(sdef->elements[i].type,"UNIVERSAL_CHARSTRING") ) {
      // In case the field is an optional anyElement -> check if it should be omitted
      if (sdef->elements[i].isOptional) {
        src = mputprintf(src,
          "    if (field_%s.XER_check_any_elem(p_reader, \"%s\", tag_closed))\n  "
          , sdef->elements[i].name
          , (i == sdef->nElements - 1) ? "NULL" : sdef->elements[i + 1].name);
      } else {
        // If the record is emptyElement, there's no way it will have an anyElement field
        src = mputstr(src, "    if (tag_closed) p_reader.Read(); \n");
      }
    }
    
    src = mputprintf(src, 
      "    field_%s.XER_decode(%s_xer_, p_reader, p_flavor"
      " | (p_td.xer_bits & USE_NIL)| (tag_closed ? PARENT_CLOSED : XER_NONE), p_flavor2, %s);\n"
      "  }\n"
      , sdef->elements[i].name, sdef->elements[i].typegen
      , sdef->xerEmbedValuesPossible ? "emb_val" : "0");
    if (sdef->xerEmbedValuesPossible) {
      src = mputprintf(src,
        "  if (!field_%s.is_present()) {\n"
        // there was no new element, the last embedded value is for the next field
        // (or the end of the record if this is the last field) 
        "    last_embval_index = -1;\n"
        "  }\n"
        , sdef->elements[i].name);
    }
    if (!sdef->elements[i].isOptional) {
      src = mputprintf(src,
        "  if (field_%s.is_bound()) {\n"
        "    p_flavor &= ~XER_OPTIONAL;\n"
        "  }\n"
        , sdef->elements[i].name);
    }
  } /* next field */
  
  if (i == start_at + num_attributes) {
    src = mputprintf(src, "  (void)emb_val_parent;\n"); // Silence unused warning
  }
  
  if (sdef->xerEmbedValuesPossible) {
    /* read and store embedValues text if present */
    src = mputprintf(src,
      "  if (0 != emb_val) {\n"
      "    if (p_reader.NodeType()==XML_READER_TYPE_TEXT) {\n"
      "      UNIVERSAL_CHARSTRING emb_ustr((const char*)p_reader.Value());\n"
      "      field_%s%s[emb_val->embval_index] = emb_ustr;\n"
      "    }\n"
      "    if (last_embval_index == emb_val->embval_index) {\n"
      "      ++emb_val->embval_index;\n"
      "    }\n"
      "  }\n"
      , sdef->elements[0].name
      , sdef->elements[0].isOptional ? "()" : "");
  }
  
  if (sdef->xerUseNilPossible) {
    src = mputstr(src, "  } // use_nil\n");
  }

  if (sdef->xerUseOrderPossible) {
    src = mputstr(src,  "  } // uo\n");
  }
  
  if (sdef->xerEmbedValuesPossible) {
    src = mputprintf(src,
      "%s"
      "  if (0 != emb_val) {\n"
      "    %s::of_type empty_string(\"\");\n"
      "    for (int j_j = 0; j_j < emb_val->embval_index; ++j_j) {\n"
      "      if (!field_%s%s[j_j].is_bound()) {\n"
      "        field_%s%s[j_j] = empty_string;\n"
      "      }%s%s%s%s"
      "    }\n"
      "    delete emb_val;\n"
      "  }\n"
      , sdef->elements[0].isOptional ? "  boolean all_unbound = TRUE;\n" : ""
      , sdef->elements[0].type
      , sdef->elements[0].name
      , sdef->elements[0].isOptional ? "()" : ""
      , sdef->elements[0].name
      , sdef->elements[0].isOptional ? "()" : ""
      , sdef->elements[0].isOptional ? " else if(field_" : "\n"
      , sdef->elements[0].isOptional ? sdef->elements[0].name : ""
      , sdef->elements[0].isOptional ? "()" : ""
      , sdef->elements[0].isOptional ? "[j_j] != empty_string) {\n        all_unbound = FALSE;\n      }\n" : ""
    );
    
    if(sdef->elements[0].isOptional) {
      src = mputprintf(src,
        "  if (e_xer && (p_td.xer_bits & EMBED_VALUES) && all_unbound) {\n"
        "    field_%s = OMIT_VALUE;\n"
        "  }\n"
        , sdef->elements[0].name
      );
    }
  }

  if (sdef->xerUseQName) {
    src = mputstr(src, "  } // qn\n");
  }
  
  src = mputstr(src,
    "  } // errorcontext\n"); /* End scope for error context objects */

  /* Check if every non-optional field has been set */
  for (i = 0; i < sdef->nElements; ++i) {
    if (!sdef->elements[i].isOptional) {
      src = mputprintf(src,
        "  if (!field_%s.is_bound()) {\n"
        "    if (p_flavor & XER_OPTIONAL) {\n"
        "      clean_up();\n"
        "      return -1;\n"
        "    }\n"
        "    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INCOMPL_MSG,\n"
        "      \"No data found for non-optional field '%s'\");\n"
        "  }\n"
        , sdef->elements[i].name, sdef->elements[i].dispname);
    }
  }
  src = mputstr(src,
    "  if (!omit_tag) {\n"
    "    int current_depth;\n"
    "    for (rd_ok = p_reader.Ok(); rd_ok == 1; rd_ok = p_reader.Read()) {\n"
    "      type = p_reader.NodeType();\n"
    "      if ((current_depth = p_reader.Depth()) > xml_depth) {\n"
    "        if (XML_READER_TYPE_ELEMENT == type) {\n"
    /*           An element deeper than our start tag has not been processed */
    "          TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TAG,\n"
    "              \"Unprocessed XML tag `%s'\", (const char *)p_reader.Name());\n"
    "        }\n"
    "        continue;\n"
    "      }\n"
    "      else if (current_depth == xml_depth) {\n"
    "        if (XML_READER_TYPE_ELEMENT == type) {\n"
    "          verify_name(p_reader, p_td, e_xer);\n"
    "          if (p_reader.IsEmptyElement()) {\n"
    "            p_reader.Read();\n" /* advance one more time */
    "            break;\n"
    "          }\n"
    "        }\n"
    "        else if (XML_READER_TYPE_END_ELEMENT == type) {\n"
    "          verify_end(p_reader, p_td, xml_depth, e_xer);\n"
    "          rd_ok = p_reader.Read();\n"
    "          break;\n"
    "        }\n"
    "      }\n"
    "      else break;" /* depth is less, we are in trouble... */
    "    }\n"
    "  }\n"
    "  return 1;\n}\n\n");

  *pdef = def;
  *psrc = src;
}

void defRecordClass1(const struct_def *sdef, output_struct *output)
{
  size_t i;
  size_t mandatory_fields_count;
  const char *name = sdef->name, *dispname = sdef->dispname;
  const char* kind_str = sdef->kind == SET ? "set" : "record";
  char *def = NULL, *src = NULL;
  boolean ber_needed = sdef->isASN1 && enable_ber();
  boolean raw_needed = sdef->hasRaw && enable_raw();
  boolean text_needed = sdef->hasText && enable_text();
  boolean xer_needed = sdef->hasXer && enable_xer();
  boolean json_needed = sdef->hasJson && enable_json();

  /* class declaration code */
  output->header.class_decls = mputprintf(output->header.class_decls,
                                          "class %s;\n", name);

  if (sdef->nElements <= 0) {
    defEmptyRecordClass(sdef, output);
    return;
  }

  /* class definition and source code */
  if(ber_needed || raw_needed || text_needed || xer_needed || json_needed) {
    def=mputprintf
      (def,
#ifndef NDEBUG
        "// written by %s in " __FILE__ " at line %d\n"
#endif
        "class %s : public Base_Type {\n"
#ifndef NDEBUG
        , __FUNCTION__, __LINE__
#endif
        , name);
  } else {
    def=mputprintf(def, "class %s {\n", name);
  }

  for (i = 0; i < sdef->nElements; i++) {
    if(sdef->elements[i].isOptional)
      def = mputprintf(def, "  OPTIONAL<%s> field_%s;\n",
                       sdef->elements[i].type,
                       sdef->elements[i].name);
    else
      def = mputprintf(def, "  %s field_%s;\n",
                       sdef->elements[i].type, sdef->elements[i].name);
  }

  /* default constructor */
  def = mputprintf(def, "public:\n"
    "  %s();\n", name);
  src = mputprintf(src, "%s::%s()\n"
    "{\n"
    "}\n\n", name, name);

  /* constructor by fields */
  def = mputprintf(def, "  %s(", name);
  src = mputprintf(src, "%s::%s(", name, name);

  for (i = 0; i < sdef->nElements; i++) {
    char *tmp = NULL;
    if (i > 0) tmp = mputstr(tmp, ",\n    ");
    if (sdef->elements[i].isOptional)
      tmp = mputprintf
        (tmp,
         "const OPTIONAL<%s>& par_%s",
         sdef->elements[i].type, sdef->elements[i].name);
    else
      tmp = mputprintf
        (tmp,
         "const %s& par_%s",
         sdef->elements[i].type, sdef->elements[i].name);
    def = mputstr(def, tmp);
    src = mputstr(src, tmp);
    Free(tmp);
  }
  def = mputstr(def, ");\n");
  src = mputstr(src, ")\n"
                "  : ");
  for (i = 0; i < sdef->nElements; i++) {
    if (i > 0) src = mputstr(src, ",\n");
    src = mputprintf(src, "  field_%s(par_%s)", sdef->elements[i].name,
                     sdef->elements[i].name);
  }
  src = mputstr(src, "\n"
                "{\n"
                "}\n\n");

  /* copy constructor */
  def = mputprintf(def, "  %s(const %s& other_value);\n", name, name);
  src = mputprintf(src, "%s::%s(const %s& other_value)\n"
      "{\n"
      "if(!other_value.is_bound()) "
      "TTCN_error(\"Copying an unbound value of type %s.\");\n",
      name, name, name, dispname);
  for (i = 0; i < sdef->nElements; i++) {
    src = mputprintf(src,
      "if (other_value.%s().is_bound()) field_%s = other_value.%s();\n"
      "else field_%s.clean_up();\n",
      sdef->elements[i].name, sdef->elements[i].name, sdef->elements[i].name,
      sdef->elements[i].name);
  }
  src = mputstr(src, "}\n\n");
  
  /* not a component */
  def = mputstr(def, "  inline boolean is_component() { return FALSE; }\n");

  /* clean_up */
  def = mputstr(def, "  void clean_up();\n");
  src = mputprintf(src, "void %s::clean_up()\n"
      "{\n", name);
  for (i = 0; i < sdef->nElements; i++) {
    src = mputprintf(src,
      "field_%s.clean_up();\n", sdef->elements[i].name);
  }
  src = mputstr(src,
      "}\n\n");

  /* = operator */
  def = mputprintf(def, "  %s& operator=(const %s& other_value);\n", name, name);
  src = mputprintf(src, "%s& %s::operator=(const %s& other_value)\n"
      "{\n"
      "if (this != &other_value) {\n"
      "  if(!other_value.is_bound()) "
      "TTCN_error(\"Assignment of an unbound value of type %s.\");\n",
      name, name, name, dispname);
  for (i = 0; i < sdef->nElements; i++) {
    src = mputprintf(src,
      "  if (other_value.%s().is_bound()) field_%s = other_value.%s();\n"
      "  else field_%s.clean_up();\n",
      sdef->elements[i].name, sdef->elements[i].name, sdef->elements[i].name,
      sdef->elements[i].name);
  }
  src = mputstr(src,
      "}\n"
      "return *this;\n"
      "}\n\n");

  /* == operator */
  def = mputprintf(def, "  boolean operator==(const %s& other_value) "
                   "const;\n", name);
  src = mputprintf(src,
                   "boolean %s::operator==(const %s& other_value) const\n"
                   "{\n"
                   "return ", name, name);
  for (i = 0; i < sdef->nElements; i++) {
    if (i > 0) src = mputstr(src, "\n  && ");
    src = mputprintf(src, "field_%s==other_value.field_%s",
                     sdef->elements[i].name, sdef->elements[i].name);
  }
  src = mputstr(src, ";\n}\n\n");

  /* != and () operator */
  def = mputprintf(def,
     "  inline boolean operator!=(const %s& other_value) const\n"
     "    { return !(*this == other_value); }\n\n", name);

  /* is_bound */
  def = mputprintf(def,
      "  boolean is_bound() const;\n\n");
  src = mputprintf(src,
      "boolean %s::is_bound() const\n"
      "{\n", name);
      for(i=0; i < sdef->nElements; i++) {
        if(sdef->elements[i].isOptional) {
          src = mputprintf(src,
            "if(OPTIONAL_OMIT == field_%s.get_selection() || field_%s.is_bound()) return TRUE;\n",
            sdef->elements[i].name, sdef->elements[i].name);
        } else {
          src = mputprintf(src,
            "if(field_%s.is_bound()) return TRUE;\n",
            sdef->elements[i].name);
        }
      }
  src = mputprintf(src,
      "return FALSE;\n"
      "}\n");

  /* is_present */
  def = mputstr(def,
      "inline boolean is_present() const { return is_bound(); }\n");

  /* is_value */
  def = mputprintf(def,
      "  boolean is_value() const;\n\n");
  src = mputprintf(src,
      "boolean %s::is_value() const\n"
      "{\n", name);
      for(i=0; i < sdef->nElements; i++) {
        if(sdef->elements[i].isOptional) {
          src = mputprintf(src,
            "if(OPTIONAL_OMIT != field_%s.get_selection() && !field_%s.is_value()) return FALSE;\n",
            sdef->elements[i].name, sdef->elements[i].name);
        } else {
          src = mputprintf(src,
            "if(!field_%s.is_value()) return FALSE;\n",
            sdef->elements[i].name);
        }
      }
  src = mputprintf(src,
      "return TRUE;\n}\n");

  /* field accessor methods */
  for (i = 0; i < sdef->nElements; i++) {
    if(sdef->elements[i].isOptional)
      def = mputprintf
        (def,
         "  inline OPTIONAL<%s>& %s()\n"
         "    {return field_%s;}\n"
         "  inline const OPTIONAL<%s>& %s() const\n"
         "    {return field_%s;}\n",
         sdef->elements[i].type, sdef->elements[i].name,
         sdef->elements[i].name,
         sdef->elements[i].type, sdef->elements[i].name,
         sdef->elements[i].name);
    else def = mputprintf
           (def,
            "  inline %s& %s()\n"
            "    {return field_%s;}\n"
            "  inline const %s& %s() const\n"
            "    {return field_%s;}\n",
            sdef->elements[i].type, sdef->elements[i].name,
            sdef->elements[i].name, sdef->elements[i].type,
            sdef->elements[i].name, sdef->elements[i].name);

  }

  /* sizeof operation */
  mandatory_fields_count = 0;
  for (i = 0; i < sdef->nElements; i++)
    if (!sdef->elements[i].isOptional) mandatory_fields_count++;

  if(sdef->nElements == mandatory_fields_count){
    def = mputprintf(def,
        "  inline int size_of() const\n"
        "    {return %lu;}\n", (unsigned long) mandatory_fields_count);
  }else{
    def = mputstr(def, "  int size_of() const;\n");
    src = mputprintf(src,
      "int %s::size_of() const\n"
      "{\n",
      name);
    src = mputprintf(src, "  int ret_val = %lu;\n",
      (unsigned long) mandatory_fields_count);
    for (i = 0; i < sdef->nElements; i++)
      if (sdef->elements[i].isOptional)
        src = mputprintf(src,
          "  if (field_%s.ispresent()) ret_val++;\n",
          sdef->elements[i].name);
    src = mputstr(src,
      "  return ret_val;\n"
      "}\n\n");
  }

  /* log function */
  def = mputstr(def, "  void log() const;\n");
  src = mputprintf(src, 
    "void %s::log() const\n{\n"
    "if (!is_bound()) {\n"
    "TTCN_Logger::log_event_unbound();\n"
    "return;\n"
    "}\n", name);
  for (i = 0; i < sdef->nElements; i++) {
    src = mputstr(src, "TTCN_Logger::log_event_str(\"");
    if (i == 0) src = mputc(src, '{');
    else src = mputc(src, ',');
    src = mputprintf(src, " %s := \");\n"
      "field_%s.log();\n",
      sdef->elements[i].dispname, sdef->elements[i].name);
  }
  src = mputstr(src, "TTCN_Logger::log_event_str(\" }\");\n}\n\n");

  /* set param function */
  def = mputstr(def, "  void set_param(Module_Param& param);\n");
  src = mputprintf
    (src,
     "void %s::set_param(Module_Param& param)\n{\n"
     "  param.basic_check(Module_Param::BC_VALUE, \"%s value\");\n"
     "  switch (param.get_type()) {\n"
     "  case Module_Param::MP_Value_List:\n"
     "    if (%lu<param.get_size()) {\n"
     "      param.error(\"%s value of type %s has %lu fields but list value has %%d fields\", (int)param.get_size());\n"
     "    }\n",
     name, kind_str, (unsigned long)sdef->nElements, kind_str, dispname, (unsigned long)sdef->nElements);

  for (i = 0; i < sdef->nElements; ++i) {
    src = mputprintf(src,
      "    if (param.get_size()>%lu && param.get_elem(%lu)->get_type()!=Module_Param::MP_NotUsed) %s().set_param(*param.get_elem(%lu));\n",
      (unsigned long)i, (unsigned long)i, sdef->elements[i].name, (unsigned long)i);
  } 
  src = mputstr(src,
      "    break;\n"
      "  case Module_Param::MP_Assignment_List: {\n"
      "    Vector<bool> value_used(param.get_size());\n"
      "    value_used.resize(param.get_size(), FALSE);\n");
  for (i = 0; i < sdef->nElements; ++i) {
    src = mputprintf(src,
      "    for (size_t val_idx=0; val_idx<param.get_size(); val_idx++) {\n"
      "      Module_Param* const curr_param = param.get_elem(val_idx);\n"
      "      if (!strcmp(curr_param->get_id()->get_name(), \"%s\")) {\n"
      "        if (curr_param->get_type()!=Module_Param::MP_NotUsed) {\n"
      "          %s().set_param(*curr_param);\n"
      "        }\n"
      "        value_used[val_idx]=TRUE;\n"
      "      }\n"
      "    }\n"
      , sdef->elements[i].dispname, sdef->elements[i].name);
  }
  src = mputprintf(src,
      "    for (size_t val_idx=0; val_idx<param.get_size(); val_idx++) if (!value_used[val_idx]) {\n"
      "      param.get_elem(val_idx)->error(\"Non existent field name in type %s: %%s\", param.get_elem(val_idx)->get_id()->get_name());\n"
      "      break;\n"
      "    }\n"
      "  } break;\n"
      "  default:\n"
      "    param.type_error(\"%s value\", \"%s\");\n"
      "  }\n"
      "}\n\n", dispname, kind_str, dispname);

  /* set implicit omit function, recursive */
  def = mputstr(def, "  void set_implicit_omit();\n");
  src = mputprintf(src, "void %s::set_implicit_omit()\n{\n", name);
  for (i = 0; i < sdef->nElements; i++) {
    if (sdef->elements[i].isOptional) {
      src = mputprintf(src,
        "if (!%s().is_bound()) %s() = OMIT_VALUE;\n"
        "else %s().set_implicit_omit();\n",
        sdef->elements[i].name, sdef->elements[i].name, sdef->elements[i].name);
    } else {
      src = mputprintf(src,
        "if (%s().is_bound()) %s().set_implicit_omit();\n",
        sdef->elements[i].name, sdef->elements[i].name);
    }
  }
  src = mputstr(src, "}\n\n");

  /* text encoder function */
  def = mputstr(def, "  void encode_text(Text_Buf& text_buf) const;\n");
  src = mputprintf(src,"void %s::encode_text(Text_Buf& text_buf) const\n{\n",
                   name);
  for (i = 0; i < sdef->nElements; i++) {
    src = mputprintf(src, "field_%s.encode_text(text_buf);\n",
                     sdef->elements[i].name);
  }
  src = mputstr(src, "}\n\n");

  /* text decoder function */
  def = mputstr(def, "  void decode_text(Text_Buf& text_buf);\n");
  src = mputprintf(src, "void %s::decode_text(Text_Buf& text_buf)\n{\n",
                   name);
  for (i = 0; i < sdef->nElements; i++) {
    src = mputprintf(src, "field_%s.decode_text(text_buf);\n",
                     sdef->elements[i].name);
  }
  src = mputstr(src, "}\n\n");

  /* The common "classname::encode()" and "classname::decode()" functions */
  if(ber_needed || raw_needed || text_needed || xer_needed || json_needed)
    def_encdec(name, &def, &src, ber_needed, raw_needed,
               text_needed, xer_needed, json_needed, FALSE);

  /* BER functions */
  if(ber_needed) {
    /* BER_encode_TLV() */
    src=mputprintf
      (src,
       "ASN_BER_TLV_t* %s::BER_encode_TLV(const TTCN_Typedescriptor_t& p_td,"
       " unsigned p_coding) const\n"
       "{\n"
       "  if (!is_bound()) TTCN_EncDec_ErrorContext::error"
       "(TTCN_EncDec::ET_UNBOUND, \"Encoding an unbound value.\");\n"
       "  BER_chk_descr(p_td);\n"
       "  ASN_BER_TLV_t *new_tlv=ASN_BER_TLV_t::construct(NULL);\n"
       "  TTCN_EncDec_ErrorContext ec_0(\"Component '\");\n"
       "  TTCN_EncDec_ErrorContext ec_1;\n"
       , name
       );
    for(i=0; i<sdef->nElements; i++) {
      if(!default_as_optional && sdef->elements[i].isDefault) { /* is DEFAULT */
        src=mputprintf
          (src,
           "  if(field_%s!=%s) {\n"
           "    ec_1.set_msg(\"%s': \");\n"
           "    new_tlv->add_TLV(field_%s.BER_encode_TLV(%s_descr_,"
           " p_coding));\n"
           "  }\n"
           , sdef->elements[i].name, sdef->elements[i].defvalname
           , sdef->elements[i].dispname
           , sdef->elements[i].name, sdef->elements[i].typedescrname
           );
      }
      else { /* is not DEFAULT */
        src=mputprintf
          (src,
           "  ec_1.set_msg(\"%s': \");\n"
           "  new_tlv->add_TLV(field_%s.BER_encode_TLV(%s_descr_,"
           " p_coding));\n"
           , sdef->elements[i].dispname
           , sdef->elements[i].name, sdef->elements[i].typedescrname
           );
      } /* !isDefault */
    } /* for i */
    if(sdef->kind==SET) {
    src=mputstr
      (src,
       "  if(p_coding==BER_ENCODE_DER)\n"
       "    new_tlv->sort_tlvs_tag();\n"
       );
    }
    src=mputstr
      (src,
       "  new_tlv=ASN_BER_V2TLV(new_tlv, p_td, p_coding);\n"
       "  return new_tlv;\n"
       "}\n"
       "\n"
       );

    /* BER_decode_TLV() */
    src=mputprintf
      (src,
       "boolean %s::BER_decode_TLV(const TTCN_Typedescriptor_t& p_td,"
       " const ASN_BER_TLV_t& p_tlv, unsigned L_form)\n"
       "{\n"
       "  BER_chk_descr(p_td);\n"
       "  ASN_BER_TLV_t stripped_tlv;\n"
       "  BER_decode_strip_tags(*p_td.ber, p_tlv, L_form, stripped_tlv);\n"
       "  TTCN_EncDec_ErrorContext ec_0(\"While decoding '%s' type: \");\n"
       "  stripped_tlv.chk_constructed_flag(TRUE);\n"
       "  size_t V_pos=0;\n"
       "  ASN_BER_TLV_t tmp_tlv;\n"
       , name, sdef->dispname
       );

    if(sdef->kind==RECORD)
    { /* SEQUENCE decoding */
      src=mputstr
        (src,
         "  boolean tlv_present=FALSE;\n"
         "  {\n"
         "    TTCN_EncDec_ErrorContext ec_1(\"Component '\");\n"
         "    TTCN_EncDec_ErrorContext ec_2;\n"
         );
      for(i=0; i<sdef->nElements; i++) {
        src=mputprintf
          (src,
           "    ec_2.set_msg(\"%s': \");\n"
           "    if(!tlv_present) tlv_present=BER_decode_constdTLV_next"
           "(stripped_tlv, V_pos, L_form, tmp_tlv);\n"
           , sdef->elements[i].dispname
           );
        if(sdef->elements[i].isDefault) { /* is DEFAULT */
          src=mputprintf
            (src,
             "    if(!tlv_present || !field_%s.BER_decode_isMyMsg(%s_descr_,"
             " tmp_tlv))\n"
             "      field_%s=%s;\n"
             "    else {\n"
             "      field_%s.BER_decode_TLV(%s_descr_, tmp_tlv,"
             " L_form);\n"
             "      tlv_present=FALSE;\n"
             "    }\n"
             , sdef->elements[i].name, sdef->elements[i].typedescrname
             , sdef->elements[i].name, sdef->elements[i].defvalname
             , sdef->elements[i].name, sdef->elements[i].typedescrname
             );
        }
        else if(sdef->elements[i].isOptional) { /* is OPTIONAL */
          src=mputprintf
            (src,
             "    if(!tlv_present) field_%s=OMIT_VALUE;\n"
             "    else {\n"
             "      field_%s.BER_decode_TLV(%s_descr_, tmp_tlv, L_form);\n"
             "      if(field_%s.ispresent()) tlv_present=FALSE;\n"
             "    }\n"
             , sdef->elements[i].name
             , sdef->elements[i].name, sdef->elements[i].typedescrname
             , sdef->elements[i].name
             );
        }
        else { /* is not DEFAULT OPTIONAL */
          src=mputprintf
            (src,
             "    if(!tlv_present){\n"
             "      ec_2.error(TTCN_EncDec::ET_INCOMPL_MSG,\"Invalid or incomplete message was received.\");\n"
             "      return FALSE;\n"
             "    }\n"
             "    field_%s.BER_decode_TLV(%s_descr_, tmp_tlv, L_form);\n"
             "    tlv_present=FALSE;\n"
             , sdef->elements[i].name, sdef->elements[i].typedescrname
             );
        } /* !isDefault */
      } /* for i */
      src=mputstr
        (src,
         "  }\n"
         "  BER_decode_constdTLV_end(stripped_tlv, V_pos, L_form, tmp_tlv,"
         " tlv_present);\n"
         );
    } /* SEQUENCE decoding */
    else { /* SET decoding */
      src=mputprintf
        (src,
         "  const char *fld_name[%lu]={"
         , (unsigned long) sdef->nElements
         );
      for(i=0; i<sdef->nElements; i++)
        src=mputprintf
          (src,
           "%s\"%s\""
           , i?", ":""
           , sdef->elements[i].dispname
           );
      /* field indicator:
       *   0x01: value arrived
       *   0x02: is optional / not used :)
       *   0x04: has default / not used :)
       */
      src=mputprintf
        (src,
         "};\n"
         "  unsigned char fld_indctr[%lu]={"
         , (unsigned long) sdef->nElements
         );
      for(i=0; i<sdef->nElements; i++)
        src=mputprintf
          (src,
           "%s0"
           , i?", ":""
           );
      src=mputstr
        (src,
         "};\n"
         "  size_t fld_curr;\n"
         "  while(BER_decode_constdTLV_next(stripped_tlv, V_pos,"
         " L_form, tmp_tlv)) {\n"
         );
      for(i=0; i<sdef->nElements; i++)
        src=mputprintf
          (src,
           "    %sif(field_%s.BER_decode_isMyMsg(%s_descr_, tmp_tlv)) {\n"
           "      fld_curr=%lu;\n"
           "      TTCN_EncDec_ErrorContext ec_1(\"Component '%%s': \","
           " fld_name[%lu]);\n"
           "      field_%s.BER_decode_TLV(%s_descr_, tmp_tlv, L_form);\n"
           "    }\n"
           , i?"else ":""
           , sdef->elements[i].name, sdef->elements[i].typedescrname
           , (unsigned long) i, (unsigned long) i
           , sdef->elements[i].name, sdef->elements[i].typedescrname
           );
      src=mputprintf
        (src,
         "    else {\n"
         "      /* ellipsis or error... */\n"
         "      fld_curr=static_cast<size_t>(-1);\n"
         "    }\n"
         "    if(fld_curr!=static_cast<size_t>(-1)) {\n"
         "      if(fld_indctr[fld_curr])\n"
         "        ec_0.error(TTCN_EncDec::ET_DEC_DUPFLD, \"Duplicated"
         " value for component '%%s'.\", fld_name[fld_curr]);\n"
         "      fld_indctr[fld_curr]=1;\n"
         "    }\n" /* if != -1 */
         "  }\n" /* while */
         "  for(fld_curr=0; fld_curr<%lu; fld_curr++) {\n"
         "    if(fld_indctr[fld_curr]) continue;\n"
         "    switch(fld_curr) {\n"
         , (unsigned long) sdef->nElements
         );
      for(i=0; i<sdef->nElements; i++) {
        if(sdef->elements[i].isDefault)
          src=mputprintf
            (src,
             "    case %lu:\n"
             "      field_%s=%s;\n"
             "      break;\n"
             , (unsigned long) i
             , sdef->elements[i].name, sdef->elements[i].defvalname
           );
        else if(sdef->elements[i].isOptional)
          src=mputprintf
            (src,
             "    case %lu:\n"
             "      field_%s=OMIT_VALUE;\n"
             "      break;\n"
             , (unsigned long) i
             , sdef->elements[i].name
           );
      } /* for i */
      src=mputstr
        (src,
         "    default:\n"
         "      ec_0.error(TTCN_EncDec::ET_DEC_MISSFLD, \"Missing"
         " value for component '%s'.\", fld_name[fld_curr]);\n"
         "    }\n" /* switch */
         "  }\n" /* for fld_curr */
         );
    } /* SET decoding */

    if(sdef->opentype_outermost) {
      src=mputstr
        (src,
         "  TTCN_EncDec_ErrorContext ec_1(\"While decoding opentypes: \");\n"
         "  TTCN_Type_list p_typelist;\n"
         "  BER_decode_opentypes(p_typelist, L_form);\n"
         );
    } /* if sdef->opentype_outermost */
    src=mputstr
      (src,
       "  return TRUE;\n"
       "}\n"
       "\n"
       );

    if(sdef->has_opentypes) {
      /* BER_decode_opentypes() */
      def=mputstr
        (def,
         "void BER_decode_opentypes(TTCN_Type_list& p_typelist,"
         " unsigned L_form);\n");
      src=mputprintf
        (src,
         "void %s::BER_decode_opentypes(TTCN_Type_list& p_typelist,"
         " unsigned L_form)\n"
         "{\n"
         "  p_typelist.push(this);\n"
         "  TTCN_EncDec_ErrorContext ec_0(\"Component '\");\n"
         "  TTCN_EncDec_ErrorContext ec_1;\n"
         , name
         );
      for(i=0; i<sdef->nElements; i++) {
        src=mputprintf
          (src,
           "  ec_1.set_msg(\"%s': \");\n"
           "  field_%s.BER_decode_opentypes(p_typelist, L_form);\n"
           , sdef->elements[i].dispname
           , sdef->elements[i].name
           );
      } /* for i */
      src=mputstr
        (src,
         "  p_typelist.pop();\n"
         "}\n"
         "\n"
         );
    } /* if sdef->has_opentypes */

  } /* if ber_needed */

  if(text_needed){
    int man_num=0;
    int opt_number=0;
    int seof=0;
    size_t last_man_index=0;
    for(i=0;i<sdef->nElements;i++){
      if(sdef->elements[i].isOptional) opt_number++;
      else {man_num++;last_man_index=i+1;}
      if(sdef->elements[i].of_type) seof++;
    }

    src=mputprintf(src,
      "int %s::TEXT_encode(const TTCN_Typedescriptor_t& p_td,"
      " TTCN_Buffer& p_buf) const{\n"
      "  if (!is_bound()) TTCN_EncDec_ErrorContext::error"
      "(TTCN_EncDec::ET_UNBOUND, \"Encoding an unbound value.\");\n"
      "  boolean need_separator=FALSE;\n"
      "  int encoded_length=0;\n"
      "  if(p_td.text->begin_encode){\n"
      "    p_buf.put_cs(*p_td.text->begin_encode);\n"
      "    encoded_length+=p_td.text->begin_encode->lengthof();\n"
      "  }\n"
      ,name
     );

    for(i=0;i<sdef->nElements;i++){
      if(sdef->elements[i].isOptional){
        src=mputprintf(src,
        "  if(field_%s.ispresent()){\n"
        ,sdef->elements[i].name
        );
      }
      src=mputprintf(src,
        " if(need_separator && p_td.text->separator_encode){\n"
        "    p_buf.put_cs(*p_td.text->separator_encode);\n"
        "    encoded_length+=p_td.text->separator_encode->lengthof();\n"
        " }\n"
        " encoded_length+=field_%s%s.TEXT_encode(%s_descr_,p_buf);\n"
        " need_separator=TRUE;\n"
        ,sdef->elements[i].name
        ,sdef->elements[i].isOptional?"()":"",sdef->elements[i].typedescrname
       );
      if(sdef->elements[i].isOptional){
        src=mputstr(src,
        "  }\n"
        );
      }
    }

    src=mputstr(src,
      "  if(p_td.text->end_encode){\n"
      "    p_buf.put_cs(*p_td.text->end_encode);\n"
      "    encoded_length+=p_td.text->end_encode->lengthof();\n"
      "  }\n"
      "  return encoded_length;\n"
      "}\n"
     );

    if(sdef->kind==SET){ /* set decoder start*/
      src = mputprintf(src,
      "int %s::TEXT_decode(const TTCN_Typedescriptor_t& p_td,"
      " TTCN_Buffer& p_buf, Limit_Token_List& limit, boolean no_err, boolean){\n"
      "  int decoded_length=0;\n"
      "  int decoded_field_length=0;\n"
      "  size_t pos=p_buf.get_pos();\n"
      "  boolean sep_found=FALSE;\n"
      "  int ml=0;\n"
      "  int sep_length=0;\n"
      "  int mand_field_num=%d;\n"
      "  int opt_field_num=%d;\n"
      "  int loop_detector=1;\n"
      "  int last_field_num=-1;\n"
      "  int field_map[%lu];\n"
      "  memset(field_map, 0, sizeof(field_map));\n"
      "  if(p_td.text->begin_decode){\n"
      "    int tl;\n"
      "    if((tl=p_td.text->begin_decode->match_begin(p_buf))<0){\n"
      "          if(no_err)return -1;\n"
      "          TTCN_EncDec_ErrorContext::error\n"
      "              (TTCN_EncDec::ET_TOKEN_ERR, \"The specified token '%%s'"
      " not found for '%%s': \",(const char*)*(p_td.text->begin_decode)"
      ", p_td.name);\n"
      "          return 0;\n"
      "        }\n"
      "    decoded_length+=tl;\n"
      "    p_buf.increase_pos(tl);\n"
      "  }\n"
      "  if(p_td.text->end_decode){\n"
      "    limit.add_token(p_td.text->end_decode);\n"
      "    ml++;\n"
      "  }\n"
      "  if(p_td.text->separator_decode){\n"
      "    limit.add_token(p_td.text->separator_decode);\n"
      "    ml++;\n"
      "  }\n"
        ,name,man_num,opt_number,(unsigned long) sdef->nElements
       );

      if(seof){
          int first=0;
          src=mputstr(src,
          "  int has_repeatable=0;\n"
          "  if("
          );
          for(i=0;i<sdef->nElements;i++){
            if(sdef->elements[i].of_type){
              src=mputprintf(src,
              "%s%s_descr_.text->val.parameters->decoding_params.repeatable"
              ,first?" && ":""
              ,sdef->elements[i].typedescrname
              );
              first=1;
            }
          }
          src=mputstr(src,
          ") has_repeatable=1;\n"
          );

      }
      for(i=0;i<sdef->nElements;i++){
        if(sdef->elements[i].isOptional){
          src=mputprintf(src,
          "  field_%s=OMIT_VALUE;\n"
          ,sdef->elements[i].name
          );
        }
      }
      src = mputprintf(src,
      "  while(mand_field_num+opt_field_num%s){\n"
      "    loop_detector=1;\n"
      "    while(TRUE){\n"
      ,seof?"+has_repeatable":""
      );

      for(i=0;i<sdef->nElements;i++){
        if(sdef->elements[i].of_type){
          src=mputprintf(src,
          "  if( (%s_descr_.text->val.parameters->decoding_params.repeatable"
          " && field_map[%lu]<3) || !field_map[%lu]){\n"
          "    pos=p_buf.get_pos();\n"
          "    decoded_field_length=field_%s%s.TEXT_decode(%s_descr_,p_buf,"
          "limit,TRUE,!field_map[%lu]);\n"
          "    if(decoded_field_length<0){\n"
          "      p_buf.set_pos(pos);\n"
          ,sdef->elements[i].typedescrname,(unsigned long) i,(unsigned long) i
          ,sdef->elements[i].name,sdef->elements[i].isOptional?"()":""
          ,sdef->elements[i].typedescrname
          ,(unsigned long) i
          );

          if(sdef->elements[i].isOptional){
            src=mputprintf(src,
             "   if(!field_map[%lu]) field_%s=OMIT_VALUE;\n"
             ,(unsigned long) i
             ,sdef->elements[i].name
            );
          }
          src=mputprintf(src,
          "    }else{\n"
          "     loop_detector=0;\n"
          "     if(!field_map[%lu]) {%s--;field_map[%lu]=1;}\n"
          "     else field_map[%lu]=2;\n"
          "     last_field_num=%lu;\n"
          "     break;\n"
          "    }\n"
          "    }\n"
          ,(unsigned long) i
          ,sdef->elements[i].isOptional?"opt_field_num":"mand_field_num"
          ,(unsigned long) i,(unsigned long) i,(unsigned long) i
          );
        } else {
          src=mputprintf(src,
          "  if(!field_map[%lu]){\n"
          "    pos=p_buf.get_pos();\n"
          "    decoded_field_length=field_%s%s.TEXT_decode(%s_descr_,p_buf,"
          "limit,TRUE);\n"
          "    if(decoded_field_length<0){\n"
          "      p_buf.set_pos(pos);\n"
          "%s%s%s"
          "    }else{\n"
          "     loop_detector=0;\n"
          "     field_map[%lu]=1;\n"
          "     %s--;\n"
          "     last_field_num=%lu;\n"
          "     break;\n"
          "    }\n"
          "    }\n"
          ,(unsigned long) i
          ,sdef->elements[i].name,sdef->elements[i].isOptional?"()":""
          ,sdef->elements[i].typedescrname
          ,sdef->elements[i].isOptional?"    field_":""
          ,sdef->elements[i].isOptional?sdef->elements[i].name:""
          ,sdef->elements[i].isOptional?"=OMIT_VALUE;\n":"",(unsigned long) i
          ,sdef->elements[i].isOptional?"opt_field_num":"mand_field_num"
          ,(unsigned long) i
          );
        }
     }

      src = mputstr(src,
      "    break;\n"
      "    }\n"
      "    if(loop_detector) break;\n"
      "    if(p_td.text->separator_decode){\n"
      "      int tl;\n"
      "      if((tl=p_td.text->separator_decode->match_begin(p_buf))<0){\n"
       "      if(p_td.text->end_decode){\n"
       "      int tl;\n"
       "      if((tl=p_td.text->end_decode->match_begin(p_buf))!=-1){\n"
       "        sep_found=FALSE;\n"
       "        break;\n"
       "      }\n"
       "    } else if(limit.has_token(ml)){\n"
       "      int tl;\n"
       "      if((tl=limit.match(p_buf,ml))==0){\n"
       "        sep_found=FALSE;\n"
       "        break;\n"
       "      }\n"
      "     } else break;\n"
       "    p_buf.set_pos(pos);\n"
       "    decoded_length-=decoded_field_length;\n"
       "    field_map[last_field_num]+=2;\n"
      );

      if(opt_number){
       src=mputstr(src,
        "switch(last_field_num){\n"
       );
      for(i=0;i<sdef->nElements;i++){
        if(sdef->elements[i].of_type){
           if(sdef->elements[i].isOptional){
             src=mputprintf(src,
              "  case %lu:\n"
              "   if(field_map[%lu]==3){\n"
              "   field_%s=OMIT_VALUE;\n"
              "   opt_field_num++;\n"
              "   }\n"
              "   break;\n"
              ,(unsigned long) i,(unsigned long) i,sdef->elements[i].name
             );
          } else {
             src=mputprintf(src,
              "  case %lu:\n"
              "   if(field_map[%lu]==3){\n"
              "   mand_field_num++;\n"
              "   }\n"
              "   break;\n"
              ,(unsigned long) i,(unsigned long) i
             );
          }
        } else if(sdef->elements[i].isOptional){
          src=mputprintf(src,
          "  case %lu:\n"
          "   field_%s=OMIT_VALUE;\n"
          "   opt_field_num++;\n"
          "   break;\n"
          ,(unsigned long) i,sdef->elements[i].name
          );
        }
      }
      src=mputstr(src,
        "  default:\n"
        "   mand_field_num++;\n"
        "   break;\n"
        "}\n"
       );
      }

      src = mputprintf(src,
      "        } else {\n"
      "      sep_length=tl;\n"
      "      decoded_length+=tl;\n"
      "      p_buf.increase_pos(tl);\n"
      "      for(int a=0;a<%lu;a++) if(field_map[a]>2) field_map[a]-=3;\n"
      "      sep_found=TRUE;}\n"
      "    } else if(p_td.text->end_decode){\n"
      "      int tl;\n"
      "      if((tl=p_td.text->end_decode->match_begin(p_buf))!=-1){\n"
      "        decoded_length+=tl;\n"
      "        p_buf.increase_pos(tl);\n"
      "        limit.remove_tokens(ml);\n"
      "        if(mand_field_num) return -1;\n"
      "        return decoded_length;\n"
      "      }\n"
      "    } else if(limit.has_token(ml)){\n"
      "      int tl;\n"
      "      if((tl=limit.match(p_buf,ml))==0){\n"
      "        sep_found=FALSE;\n"
      "        break;\n"
      "      }\n"
      "    }\n"
      "  }\n"
      "  limit.remove_tokens(ml);\n"
      "  if(sep_found){\n"
      "    if(mand_field_num){\n"
      "    if(no_err)return -1;\n"
      "    TTCN_EncDec_ErrorContext::error"
      "(TTCN_EncDec::ET_TOKEN_ERR, \"Error during decoding '%%s': \","
      "p_td.name);\n"
      "    return decoded_length;\n"
      "    } else {\n"
      "      decoded_length-=sep_length;\n"
      "      p_buf.set_pos(p_buf.get_pos()-sep_length);\n"
      "    }\n"
      "  }\n"
      "  if(p_td.text->end_decode){\n"
      "    int tl;\n"
      "    if((tl=p_td.text->end_decode->match_begin(p_buf))<0){\n"
      "          if(no_err)return -1;\n"
      "          TTCN_EncDec_ErrorContext::error"
      "(TTCN_EncDec::ET_TOKEN_ERR, \"The specified token '%%s'"
      " not found for '%%s': \",(const char*)*(p_td.text->end_decode)"
      ",p_td.name);\n"
      "          return decoded_length;\n"
      "        }\n"
      "    decoded_length+=tl;\n"
      "    p_buf.increase_pos(tl);\n"
      "  }\n"
      "  if(mand_field_num) return -1;\n"
      "  return decoded_length;\n"
      "}\n"
      ,(unsigned long) sdef->nElements
       );
    } else { /* record decoder start */
      src = mputprintf(src,
      "int %s::TEXT_decode(const TTCN_Typedescriptor_t& p_td,"
      " TTCN_Buffer& p_buf, Limit_Token_List& limit, boolean no_err, boolean){\n"
      "  int decoded_length=0;\n"
      "  int decoded_field_length=0;\n"
      "%s"
      "  boolean sep_found=FALSE;\n"
      "  int sep_length=0;\n"
      "  int ml=0;\n"
      "  if(p_td.text->begin_decode){\n"
      "    int tl;\n"
      "    if((tl=p_td.text->begin_decode->match_begin(p_buf))<0){\n"
      "          if(no_err)return -1;\n"
      "          TTCN_EncDec_ErrorContext::error\n"
      "              (TTCN_EncDec::ET_TOKEN_ERR, \"The specified token '%%s'"
      " not found for '%%s': \",(const char*)*(p_td.text->begin_decode)"
      ", p_td.name);\n"
      "          return 0;\n"
      "        }\n"
      "    decoded_length+=tl;\n"
      "    p_buf.increase_pos(tl);\n"
      "  }\n"
      "  if(p_td.text->end_decode){\n"
      "    limit.add_token(p_td.text->end_decode);\n"
      "    ml++;\n"
      "  }\n"
      "  if(p_td.text->separator_decode){\n"
      "    limit.add_token(p_td.text->separator_decode);\n"
      "    ml++;\n"
      "  }\n"
         ,name,opt_number?"  size_t pos=p_buf.get_pos();\n":""
       );
      for(i=0;i<sdef->nElements;i++){
        if(sdef->elements[i].isOptional){
          src=mputprintf(src,
           "    field_%s=OMIT_VALUE;\n"
          ,sdef->elements[i].name
          );
        }      }
        src=mputstr(src,
         "    while(TRUE){\n"
        );
      for(i=0;i<sdef->nElements;i++){
        if(sdef->elements[i].isOptional){
          src=mputstr(src,
           "    pos=p_buf.get_pos();\n"
          );
        }
        src=mputprintf(src,
        "    decoded_field_length=field_%s%s.TEXT_decode(%s_descr_,p_buf"
        ",limit,TRUE);\n"
        "    if(decoded_field_length<0){\n"
        ,sdef->elements[i].name,sdef->elements[i].isOptional?"()":""
        ,sdef->elements[i].typedescrname
        );
        if(sdef->elements[i].isOptional){
          src=mputprintf(src,
           "    field_%s=OMIT_VALUE;\n"
           "    p_buf.set_pos(pos);\n"
           "    } else {\n"
          ,sdef->elements[i].name
          );
        } else {
          src=mputprintf(src,
           "     limit.remove_tokens(ml);\n"
           "     if(no_err)return -1;\n"
           "     TTCN_EncDec_ErrorContext::error"
           "(TTCN_EncDec::ET_TOKEN_ERR, \"Error during decoding "
           "field '%s' for '%%s': \", p_td.name);\n"
           "     return decoded_length;\n"
           "   } else {\n"
           ,sdef->elements[i].name
          );
        }
        src=mputstr(src,
         "  decoded_length+=decoded_field_length;\n"
        );
        if(last_man_index>(i+1)){
          src=mputstr(src,
          "    if(p_td.text->separator_decode){\n"
          "      int tl;\n"
          "      if((tl=p_td.text->separator_decode->match_begin(p_buf))<0){\n"
          );

          if(sdef->elements[i].isOptional){
            src=mputprintf(src,
             "    field_%s=OMIT_VALUE;\n"
             "    p_buf.set_pos(pos);\n"
             "    decoded_length-=decoded_field_length;\n"
           ,sdef->elements[i].name
            );
          } else {
            src=mputstr(src,
             "          limit.remove_tokens(ml);\n"
             "          if(no_err)return -1;\n"
             "          TTCN_EncDec_ErrorContext::error"
             "(TTCN_EncDec::ET_TOKEN_ERR, \"The specified token '%s'"
             " not found for '%s': \",(const char*)*(p_td.text->"
             "separator_decode),p_td.name);\n"
             "          return decoded_length;\n"
            );
          }

          src=mputstr(src,
          "      } else {\n"
          "      decoded_length+=tl;\n"
          "      p_buf.increase_pos(tl);\n"
          "      sep_length=tl;\n"
          "      sep_found=TRUE;}\n"
          "    } else sep_found=FALSE;\n"
          );
        } else if(i==(sdef->nElements-1)){
          src=mputstr(src,
          "    sep_found=FALSE;\n"
          );
        } else {
          src=mputstr(src,
          "    if(p_td.text->separator_decode){\n"
          "      int tl;\n"
          "      if((tl=p_td.text->separator_decode->match_begin(p_buf))<0){\n"
          );
          if(sdef->elements[i].isOptional){
            src=mputprintf(src,
             "      if(p_td.text->end_decode){\n"
             "      int tl;\n"
             "     if((tl=p_td.text->end_decode->match_begin(p_buf))!=-1){\n"
             "        decoded_length+=tl;\n"
             "        p_buf.increase_pos(tl);\n"
             "        limit.remove_tokens(ml);\n"
             "        return decoded_length;\n"
             "      }\n"
             "    } else if(limit.has_token(ml)){\n"
             "      if((tl=limit.match(p_buf,ml))==0){\n"
             "        sep_found=FALSE;\n"
             "        break;\n"
             "      }\n"
             "    } else break;\n"
             "    field_%s=OMIT_VALUE;\n"
             "    p_buf.set_pos(pos);\n"
             "    decoded_length-=decoded_field_length;\n"
           ,sdef->elements[i].name
            );
          } else {
            src=mputstr(src,
            "      sep_found=FALSE;\n"
            "      break;\n"
            );
          }
          src=mputstr(src,
          "      } else {\n"
          "      decoded_length+=tl;\n"
          "      p_buf.increase_pos(tl);\n"
          "      sep_length=tl;\n"
          "      sep_found=TRUE;}\n"
          "    } else {\n"
          "      sep_found=FALSE;\n"
          "      if(p_td.text->end_decode){\n"
          "      int tl;\n"
          "      if((tl=p_td.text->end_decode->match_begin(p_buf))!=-1){\n"
          "        decoded_length+=tl;\n"
          "        p_buf.increase_pos(tl);\n"
          "        limit.remove_tokens(ml);\n"
          "        return decoded_length;\n"
          "      }\n"
          "    } else if(limit.has_token(ml)){\n"
          "      int tl;\n"
          "      if((tl=limit.match(p_buf,ml))==0){\n"
          "        sep_found=FALSE;\n"
          "        break;\n"
          "      }\n"
          "    }\n"
          "    }\n"
          );
        }
        src=mputstr(src,
         "  }\n"
        );
      }

      src = mputstr(src,
      "   break;\n"
      "  }\n"
      "  limit.remove_tokens(ml);\n"
      "  if(sep_found){\n"
      "      p_buf.set_pos(p_buf.get_pos()-sep_length);\n"
      "      decoded_length-=sep_length;\n"
      "  }\n"
      "  if(p_td.text->end_decode){\n"
      "    int tl;\n"
          "      if((tl=p_td.text->end_decode->match_begin(p_buf))<0){\n"
      "          if(no_err)return -1;\n"
      "          TTCN_EncDec_ErrorContext::error"
      "(TTCN_EncDec::ET_TOKEN_ERR, \"The specified token '%s'"
      " not found for '%s': \",(const char*)*(p_td.text->end_decode)"
      ",p_td.name);\n"
      "          return decoded_length;\n"
      "        }\n"
      "    decoded_length+=tl;\n"
      "    p_buf.increase_pos(tl);\n"
      "  }\n"
      "  return decoded_length;\n"
      "}\n"
       );
    }
  } /* text_needed */

  /* RAW functions */
  if(raw_needed){
    struct raw_option_struct *raw_options;
    boolean haslengthto, haspointer, hascrosstag, has_ext_bit;
    raw_options = (struct raw_option_struct*)
      Malloc(sdef->nElements * sizeof(*raw_options));

    set_raw_options(sdef, raw_options, &haslengthto,
                    &haspointer, &hascrosstag, &has_ext_bit);

    src = generate_raw_coding(src, sdef, raw_options, haspointer, hascrosstag,
                              has_ext_bit);

    for (i = 0; i < sdef->nElements; i++) {
      Free(raw_options[i].lengthoffield);
      Free(raw_options[i].dependent_fields);
    }
    Free(raw_options);
  } /* if raw_needed */

  if (xer_needed) gen_xer(sdef, &def, &src);
  
  if (json_needed) {
    // JSON encode, RT1
    src = mputprintf(src,
      "int %s::JSON_encode(const TTCN_Typedescriptor_t&%s, JSON_Tokenizer& p_tok) const\n"
      "{\n"
      "  if (!is_bound()) {\n"
      "    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,\n"
      "      \"Encoding an unbound value of type %s.\");\n"
      "    return -1;\n"
      "  }\n\n", name, !sdef->jsonAsValue ? " p_td" : "", dispname);
    if (sdef->nElements == 1) {
      if (!sdef->jsonAsValue) {
        src = mputstr(src, "  if (NULL != p_td.json && p_td.json->as_value) {\n");
      }
      src = mputprintf(src, "  %sreturn field_%s.JSON_encode(%s_descr_, p_tok);\n",
        sdef->jsonAsValue ? "" : "  ", sdef->elements[0].name, sdef->elements[0].typedescrname);
      if (!sdef->jsonAsValue) {
        src = mputstr(src, "  }\n");
      }
    }
    if (!sdef->jsonAsValue) {
      src = mputstr(src, "  int enc_len = p_tok.put_next_token(JSON_TOKEN_OBJECT_START, NULL);\n\n");
      for (i = 0; i < sdef->nElements; ++i) {
        if (sdef->elements[i].isOptional && !sdef->elements[i].jsonOmitAsNull &&
            !sdef->elements[i].jsonMetainfoUnbound) {
          src = mputprintf(src,
            "  if (field_%s.is_present())\n"
            , sdef->elements[i].name);
        }
        src = mputprintf(src,
          "  {\n"
          "    enc_len += p_tok.put_next_token(JSON_TOKEN_NAME, \"%s\");\n    "
          , sdef->elements[i].jsonAlias ? sdef->elements[i].jsonAlias : sdef->elements[i].dispname);
        if (sdef->elements[i].jsonMetainfoUnbound) {
          src = mputprintf(src,
            "if (!field_%s.is_bound()) {\n"
            "      enc_len += p_tok.put_next_token(JSON_TOKEN_LITERAL_NULL);\n"
            "      enc_len += p_tok.put_next_token(JSON_TOKEN_NAME, \"metainfo %s\");\n"
            "      enc_len += p_tok.put_next_token(JSON_TOKEN_STRING, \"\\\"unbound\\\"\");\n"
            "    }\n"
            "    else "
            , sdef->elements[i].name
            , sdef->elements[i].jsonAlias ? sdef->elements[i].jsonAlias : sdef->elements[i].dispname);
        }
        src = mputprintf(src,
          "enc_len += field_%s.JSON_encode(%s_descr_, p_tok);\n"
          "  }\n\n"
          , sdef->elements[i].name, sdef->elements[i].typedescrname);
      }
      src = mputstr(src, 
        "  enc_len += p_tok.put_next_token(JSON_TOKEN_OBJECT_END, NULL);\n"
        "  return enc_len;\n");
    }
    src = mputstr(src, "}\n\n");
    
    // JSON decode, RT1
    src = mputprintf(src,
      "int %s::JSON_decode(const TTCN_Typedescriptor_t&%s, JSON_Tokenizer& p_tok, boolean p_silent)\n"
      "{\n", name, !sdef->jsonAsValue ? " p_td" : "");
    
    if (sdef->nElements == 1) {
      if (!sdef->jsonAsValue) {
        src = mputstr(src, "  if (NULL != p_td.json && p_td.json->as_value) {\n");
      }
      src = mputprintf(src, "  %sreturn field_%s.JSON_decode(%s_descr_, p_tok, p_silent);\n",
        sdef->jsonAsValue ? "" : "  ", sdef->elements[0].name, sdef->elements[0].typedescrname);
      if (!sdef->jsonAsValue) {
        src = mputstr(src, "  }\n");
      }
    }
    if (!sdef->jsonAsValue) {
      src = mputstr(src,
        "  json_token_t j_token = JSON_TOKEN_NONE;\n"
        "  size_t dec_len = p_tok.get_next_token(&j_token, NULL, NULL);\n"
        "  if (JSON_TOKEN_ERROR == j_token) {\n"
        "    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_BAD_TOKEN_ERROR, \"\");\n"
        "    return JSON_ERROR_FATAL;\n"
        "  }\n"
        "  else if (JSON_TOKEN_OBJECT_START != j_token) {\n"
        "    return JSON_ERROR_INVALID_TOKEN;\n"
        "  }\n");

      boolean has_metainfo_enabled = FALSE;
      for (i = 0; i < sdef->nElements; ++i) {
        src = mputprintf(src, "  boolean %s_found = FALSE;\n", sdef->elements[i].name);
        if (sdef->elements[i].jsonMetainfoUnbound) {
          // initialize meta info states
          src = mputprintf(src, 
            "  int metainfo_%s = JSON_METAINFO_NONE;\n"
            , sdef->elements[i].name);
          has_metainfo_enabled = TRUE;
        }
      }
      src = mputstr(src,
        // Read name - value token pairs until we reach some other token
        "\n  while (TRUE) {\n"
        "    char* fld_name = 0;\n"
        "    size_t name_len = 0;\n"
        "    size_t buf_pos = p_tok.get_buf_pos();\n"
        "    dec_len += p_tok.get_next_token(&j_token, &fld_name, &name_len);\n"
        "    if (JSON_TOKEN_ERROR == j_token) {\n"
        "      JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_NAME_TOKEN_ERROR);\n"
        "      return JSON_ERROR_FATAL;\n"
        "    }\n"
             // undo the last action on the buffer
        "    else if (JSON_TOKEN_NAME != j_token) {\n"
        "      p_tok.set_buf_pos(buf_pos);\n"
        "      break;\n"
        "    }\n"
        "    else {\n      ");
      if (has_metainfo_enabled) {
        // check for meta info
        src = mputstr(src,
          "boolean is_metainfo = FALSE;\n"
          "      if (name_len > 9 && 0 == strncmp(fld_name, \"metainfo \", 9)) {\n"
          "        fld_name += 9;\n"
          "        name_len -= 9;\n"
          "        is_metainfo = TRUE;\n"
          "      }\n      ");
      }
      for (i = 0; i < sdef->nElements; ++i) {
        src = mputprintf(src,
          // check field name
          "if (%d == name_len && 0 == strncmp(fld_name, \"%s\", name_len)) {\n"
          "        %s_found = TRUE;\n"
          , (int)strlen(sdef->elements[i].jsonAlias ? sdef->elements[i].jsonAlias : sdef->elements[i].dispname)
          , sdef->elements[i].jsonAlias ? sdef->elements[i].jsonAlias : sdef->elements[i].dispname
          , sdef->elements[i].name);
        if (has_metainfo_enabled) {
          src = mputstr(src, "        if (is_metainfo) {\n");
          if (sdef->elements[i].jsonMetainfoUnbound) {
            src = mputprintf(src,
              // check meta info
              "          char* info_value = 0;\n"
              "          size_t info_len = 0;\n"
              "          dec_len += p_tok.get_next_token(&j_token, &info_value, &info_len);\n"
              "          if (JSON_TOKEN_STRING == j_token && 9 == info_len &&\n"
              "              0 == strncmp(info_value, \"\\\"unbound\\\"\", 9)) {\n"
              "            metainfo_%s = JSON_METAINFO_UNBOUND;\n"
              "          }\n"
              "          else {\n"
              "            JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_METAINFO_VALUE_ERROR, \"%s\");\n"
              "            return JSON_ERROR_FATAL;\n"
              "          }\n"
              , sdef->elements[i].name, sdef->elements[i].dispname);
          }
          else {
            src = mputprintf(src,
              "          JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_METAINFO_NOT_APPLICABLE, \"%s\");\n"
              "          return JSON_ERROR_FATAL;\n"
              , sdef->elements[i].dispname);
          }
          src = mputstr(src,
            "        }\n"
            "        else {\n");
          if (sdef->elements[i].jsonMetainfoUnbound) {
            src = mputstr(src, "         buf_pos = p_tok.get_buf_pos();\n");
          }
        }
        src = mputprintf(src,
          "         int ret_val = field_%s.JSON_decode(%s_descr_, p_tok, p_silent);\n"
          "         if (0 > ret_val) {\n"
          "           if (JSON_ERROR_INVALID_TOKEN == ret_val) {\n"
          , sdef->elements[i].name, sdef->elements[i].typedescrname);
        if (sdef->elements[i].jsonMetainfoUnbound) {
          src = mputprintf(src,
            // undo the last action on the buffer, check if the invalid token was a null token 
            "             p_tok.set_buf_pos(buf_pos);\n"
            "             p_tok.get_next_token(&j_token, NULL, NULL);\n"
            "             if (JSON_TOKEN_LITERAL_NULL == j_token) {\n"
            "               if (JSON_METAINFO_NONE == metainfo_%s) {\n"
            // delay reporting an error for now, there might be meta info later
            "                 metainfo_%s = JSON_METAINFO_NEEDED;\n"
            "                 continue;\n"
            "               }\n"
            "               else if (JSON_METAINFO_UNBOUND == metainfo_%s) {\n"
            // meta info already found
            "                 continue;\n"
            "               }\n"
            "             }\n"
            , sdef->elements[i].name, sdef->elements[i].name, sdef->elements[i].name);
        }
        src = mputprintf(src,
          "             JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_FIELD_TOKEN_ERROR, %lu, \"%s\");\n"
          "           }\n"
          "           return JSON_ERROR_FATAL;\n"
          "         }\n"
          "         dec_len += (size_t)ret_val;\n"
          , (unsigned long) strlen(sdef->elements[i].dispname), sdef->elements[i].dispname);
        if (has_metainfo_enabled) {
          src = mputstr(src, "        }\n");
        }
        src = mputstr(src,
          "      }\n"
          "      else ");
      }
      src = mputprintf(src,
        "{\n"
                 // invalid field name
        "        JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, %sJSON_DEC_INVALID_NAME_ERROR, (int)name_len, fld_name);\n"
                 // if this is set to a warning, skip the value of the field
        "        dec_len += p_tok.get_next_token(&j_token, NULL, NULL);\n"
        "        if (JSON_TOKEN_NUMBER != j_token && JSON_TOKEN_STRING != j_token &&\n"
        "            JSON_TOKEN_LITERAL_TRUE != j_token && JSON_TOKEN_LITERAL_FALSE != j_token &&\n"
        "            JSON_TOKEN_LITERAL_NULL != j_token) {\n"
        "          JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_FIELD_TOKEN_ERROR, (int)name_len, fld_name);\n"
        "          return JSON_ERROR_FATAL;\n"
        "        }\n"
        "      }\n"
        "    }\n"
        "  }\n\n"
        "  dec_len += p_tok.get_next_token(&j_token, NULL, NULL);\n"
        "  if (JSON_TOKEN_OBJECT_END != j_token) {\n"
        "    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_OBJECT_END_TOKEN_ERROR, \"\");\n"
        "    return JSON_ERROR_FATAL;\n"
        "  }\n\n  "
        , has_metainfo_enabled ? "is_metainfo ?\n          JSON_DEC_METAINFO_NAME_ERROR : " : "");
      // Check if every field has been set and handle meta info
      for (i = 0; i < sdef->nElements; ++i) {
        if (sdef->elements[i].jsonMetainfoUnbound) {
          src = mputprintf(src,
            "if (JSON_METAINFO_UNBOUND == metainfo_%s) {\n"
            "    field_%s.clean_up();\n"
            "  }\n"
            "  else if (JSON_METAINFO_NEEDED == metainfo_%s) {\n"
            // no meta info was found for this field, report the delayed error
            "    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_FIELD_TOKEN_ERROR, %lu, \"%s\");\n"
            "  }\n"
            "  else "
            , sdef->elements[i].name, sdef->elements[i].name
            , sdef->elements[i].name
            , (unsigned long) strlen(sdef->elements[i].dispname)
            , sdef->elements[i].dispname);
        }
        src = mputprintf(src,
          "  if (!%s_found) {\n"
          , sdef->elements[i].name);
        if (sdef->elements[i].jsonDefaultValue) {
          src = mputprintf(src,
            "    field_%s.JSON_decode(%s_descr_, DUMMY_BUFFER, p_silent);\n"
            , sdef->elements[i].name, sdef->elements[i].typedescrname);
        }
        else if (sdef->elements[i].isOptional) {
          src = mputprintf(src,
            "    field_%s = OMIT_VALUE;\n"
            , sdef->elements[i].name);
        } else {
          src = mputprintf(src,
            "    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_MISSING_FIELD_ERROR, \"%s\");\n"
            "    return JSON_ERROR_FATAL;\n"
            , sdef->elements[i].dispname);
        }
        src = mputstr(src,
          "  }\n");
      }
      src = mputstr(src,
        "\n  return (int)dec_len;\n");
    }
    src = mputstr(src, "}\n\n");
  }
  
  /* end of class definition */
  def = mputstr(def, "};\n\n");

  output->header.class_defs = mputstr(output->header.class_defs, def);
  Free(def);

  output->source.methods = mputstr(output->source.methods, src);
  Free(src);
}


static char *genRawFieldDecodeLimit(char *src, const struct_def *sdef,
  int i, const struct raw_option_struct *raw_options)
{
  /* number of arguments passed to min_of_ints() */
  int nof_args = 1;
  int j;
  for (j = 0; j < raw_options[i].lengthof; j++) {
    int field_index = raw_options[i].lengthoffield[j];
    if (i > field_index && sdef->elements[field_index].raw.unit!=-1)
      nof_args++;
  }
  if (raw_options[i].extbitgroup && sdef->raw.ext_bit_groups[
      raw_options[i].extbitgroup-1].ext_bit != XDEFNO) nof_args++;
  if (nof_args > 1) {
    src = mputprintf(src, "min_of_ints(%d, limit", nof_args);
    for (j = 0; j < raw_options[i].lengthof; j++) {
      int field_index = raw_options[i].lengthoffield[j];
      if (i > field_index && sdef->elements[field_index].raw.unit != -1)
	src = mputprintf(src, ", value_of_length_field%d", field_index);
    }
    if (raw_options[i].extbitgroup && sdef->raw.ext_bit_groups[
	raw_options[i].extbitgroup-1].ext_bit != XDEFNO)
      src = mputstr(src, ", group_limit");
    src = mputc(src, ')');
  } else {
    src = mputstr(src, "limit");
  }
  return src;
}

static char *genRawDecodeRecordField(char *src, const struct_def *sdef,
  int i, const struct raw_option_struct *raw_options, boolean delayed_decode,
  int *prev_ext_group)
{
  int a;
  if(raw_options[i].ptrbase)
    src=mputprintf(src,
    "  start_pos_of_field%d=p_buf.get_pos_bit();\n"
    ,i
    );
  if (*prev_ext_group != raw_options[i].extbitgroup) {
    *prev_ext_group = raw_options[i].extbitgroup;
    if(prev_ext_group &&
          sdef->raw.ext_bit_groups[raw_options[i].extbitgroup-1].ext_bit!=XDEFNO){
       if(raw_options[i].pointerof)  /* pointed field */
         src=mputprintf(src,
         "  {size_t old_pos=p_buf.get_pos_bit();\n"
         "  if(start_of_field%d!=-1 && start_pos_of_field%d!=-1){\n"
         "  start_of_field%d=start_pos_of_field%d"
         "+(int)field_%s%s*%d+%d;\n"
         "  p_buf.set_pos_bit(start_of_field%d);\n"
         "  limit=end_of_available_data-start_of_field%d;\n"
         ,i
         ,sdef->elements[raw_options[i].pointerof-1].raw.pointerbase
         ,i
         ,sdef->elements[raw_options[i].pointerof-1].raw.pointerbase
         ,sdef->elements[raw_options[i].pointerof-1].name
         ,sdef->elements[raw_options[i].pointerof-1].isOptional?"()":""
         ,sdef->elements[raw_options[i].pointerof-1].raw.unit
         ,sdef->elements[raw_options[i].pointerof-1].raw.ptroffset
          ,i,i
         );
       src=mputprintf(src,
         "  {\n"
         "  cbyte* data=p_buf.get_read_data();\n"
         "  int count=1;\n"
         "  int rot=local_top_order==ORDER_LSB?0:7;\n"
         "    while(((data[count-1]>>rot)&0x01)==%d && count*8<"
         "(int)limit) count++;\n"
          "  if(limit) group_limit=count*8;\n"
         "  }\n"
       ,sdef->raw.ext_bit_groups[raw_options[i].extbitgroup-1].ext_bit==
                                                               XDEFYES?0:1
       );
       if(raw_options[i].pointerof)  /* pointed field */
         src=mputstr(src,
         "  } else group_limit=0;\n"
         "  p_buf.set_pos_bit(old_pos);\n"
         "  limit=end_of_available_data-old_pos;}\n"
         );
    }
  }
  if(sdef->elements[i].hasRaw &&  /* check the crosstag */
                            sdef->elements[i].raw.crosstaglist.nElements){
    /* field index of the otherwise rule */
    int other = -1;
    boolean first_value = TRUE;
    int j;
    for (j = 0; j < sdef->elements[i].raw.crosstaglist.nElements; j++) {
      rawAST_coding_taglist* cur_choice =
	sdef->elements[i].raw.crosstaglist.list + j;
      if (cur_choice->nElements > 0) {
        /* this is a normal rule */
	if (first_value) {
	  src = mputstr(src, "  if (");
	  first_value = FALSE;
	} else src = mputstr(src, "  else if (");
	src = genRawFieldChecker(src, cur_choice, TRUE);
	/* set selected_field in the if's body */
	src = mputprintf(src, ") selected_field = %d;\n",
	  cur_choice->fieldnum);
      } else {
        /* this is an otherwise rule */
	other = cur_choice->fieldnum;
      }
    }
    /* set selected_field to the field index of the otherwise rule or -1 */
    src = mputprintf(src, "  else selected_field = %d;\n", other);
  }
  /* check the presence of optional field*/
  if(sdef->elements[i].isOptional){
    /* check if enough bits to decode the field*/
    src = mputstr(src, "  if (limit > 0");
    for (a = 0; a < raw_options[i].lengthof; a++){
      int field_index = raw_options[i].lengthoffield[a];
      if (i > field_index) src = mputprintf(src,
	  " && value_of_length_field%d > 0", field_index);
    }
    if (raw_options[i].extbitgroup &&
        sdef->raw.ext_bit_groups[raw_options[i].extbitgroup-1].ext_bit!=XDEFNO){
      src = mputstr(src, " && group_limit > 0");
    }
    if(raw_options[i].pointerof){ /* valid pointer?*/
      src=mputprintf(src,
      " && start_of_field%d!=-1 && start_pos_of_field%d!=-1"
      ,i,sdef->elements[raw_options[i].pointerof-1].raw.pointerbase
      );
    }
    if(sdef->elements[i].hasRaw &&
                                 sdef->elements[i].raw.presence.nElements)
    {
      src = mputstr(src, " && ");
      if (sdef->elements[i].raw.presence.nElements > 1) src = mputc(src, '(');
      src = genRawFieldChecker(src, &sdef->elements[i].raw.presence, TRUE);
      if (sdef->elements[i].raw.presence.nElements > 1) src = mputc(src, ')');
    }
    if(sdef->elements[i].hasRaw &&
                             sdef->elements[i].raw.crosstaglist.nElements)
    {
      src=mputstr(src,
      "&& selected_field!=-1"
      );
    }
    src=mputstr(src,
    "){\n"
    );
  }
  if(raw_options[i].pointerof){  /* pointed field */
   src=mputprintf(src,
     "  start_of_field%d=start_pos_of_field%d"
     "+(int)field_%s%s*%d+%d;\n"
     ,i
     ,sdef->elements[raw_options[i].pointerof-1].raw.pointerbase
     ,sdef->elements[raw_options[i].pointerof-1].name
     ,sdef->elements[raw_options[i].pointerof-1].isOptional?"()":""
     ,sdef->elements[raw_options[i].pointerof-1].raw.unit
     ,sdef->elements[raw_options[i].pointerof-1].raw.ptroffset
     );
    src=mputprintf(src,
    "  p_buf.set_pos_bit(start_of_field%d);\n"
    "  limit=end_of_available_data-start_of_field%d;\n"
    ,i,i
    );
  }
  /* decoding of normal field */
  if (sdef->elements[i].isOptional) src = mputstr(src,
    "  size_t fl_start_pos = p_buf.get_pos_bit();\n");
  src = mputprintf(src,
    "  decoded_field_length = field_%s%s.RAW_decode(%s_descr_, p_buf, ",
    sdef->elements[i].name, sdef->elements[i].isOptional ? "()" : "",
    sdef->elements[i].typedescrname);
  if (delayed_decode) {
    /* the fixed field length is used as limit in case of delayed decoding */
    src = mputprintf(src, "%d", sdef->elements[i].raw.length);
  } else {
    src = genRawFieldDecodeLimit(src, sdef, i, raw_options);
  }
  src = mputprintf(src, ", local_top_order, %s",
    sdef->elements[i].isOptional ? "TRUE" : "no_err");
  if (sdef->elements[i].hasRaw &&
      sdef->elements[i].raw.crosstaglist.nElements > 0)
    src = mputstr(src, ", selected_field");
  for (a = 0; a < raw_options[i].lengthof; a++) {
    int field_index = raw_options[i].lengthoffield[a];
    /* number of elements in 'record of' or 'set of' */
    if (sdef->elements[field_index].raw.unit == -1) {
      src = mputprintf(src, ", value_of_length_field%d", field_index);
      break;
    }
  }
  src = mputstr(src, ");\n");
  if (delayed_decode) {
    src = mputprintf(src, "  if (decoded_field_length != %d) return -1;\n",
      sdef->elements[i].raw.length);
  } else if (sdef->elements[i].isOptional) {
    src = mputprintf(src, "  if (decoded_field_length < 1) {\n"
      "  field_%s = OMIT_VALUE;\n" /* transform errors into omit */
      "  p_buf.set_pos_bit(fl_start_pos);\n"
      "  } else {\n", sdef->elements[i].name);
  } else {
    src = mputstr(src, "  if (decoded_field_length < 0) return decoded_field_length;\n");
  }
  if(raw_options[i].tag_type && sdef->raw.taglist.list[raw_options[i].tag_type-1].nElements){
    rawAST_coding_taglist* cur_choice=
                               sdef->raw.taglist.list+raw_options[i].tag_type-1;
    src=mputstr(src,
      "       if(");
    src=genRawFieldChecker(src,cur_choice,FALSE);
    src=mputstr(src,
      ")");
    if(sdef->elements[i].isOptional){
      src=mputprintf(src,
      "{\n  field_%s=OMIT_VALUE;\n"
      "  p_buf.set_pos_bit(fl_start_pos);\n  }\n"
      "  else {\n"
      ,sdef->elements[i].name
      );
    }else src=mputstr(src, " return -1;\n");
  }
  if (!delayed_decode) {
    src=mputstr(src,
      "  decoded_length+=decoded_field_length;\n"
      "  limit-=decoded_field_length;\n"
      "  last_decoded_pos=bigger(last_decoded_pos, p_buf.get_pos_bit());\n"
      );
  }
  if(raw_options[i].extbitgroup &&
          sdef->raw.ext_bit_groups[raw_options[i].extbitgroup-1].ext_bit!=XDEFNO){
    src=mputstr(src,
    "group_limit-=decoded_field_length;\n"
    );
  }
  if(raw_options[i].lengthto){   /* if the field is lengthto store the value*/
    if(sdef->elements[i].raw.lengthindex){
      if(sdef->elements[i].raw.lengthindex->fieldtype == OPTIONAL_FIELD){
        src=mputprintf(src,
        "  if(field_%s%s.%s().ispresent()){\n"
        ,sdef->elements[i].name,sdef->elements[i].isOptional?"()":""
        ,sdef->elements[i].raw.lengthindex->nthfieldname
        );
      }
      src=mputprintf(src,
        "  value_of_length_field%d+=(int)field_%s%s.%s%s()*%d;\n"
        ,i,sdef->elements[i].name,sdef->elements[i].isOptional?"()":""
        ,sdef->elements[i].raw.lengthindex->nthfieldname
        ,sdef->elements[i].raw.lengthindex->fieldtype == OPTIONAL_FIELD?"()":""
        ,sdef->elements[i].raw.unit==-1?1:sdef->elements[i].raw.unit
      );
      if(sdef->elements[i].raw.lengthindex->fieldtype == OPTIONAL_FIELD){
        src=mputstr(src,
        "  }\n"
        );
      }
    }
    else if(sdef->elements[i].raw.union_member_num){
      int m;
      src = mputprintf(src, "  switch (field_%s%s.get_selection()) {\n",
	sdef->elements[i].name, sdef->elements[i].isOptional ? "()" : "");
      for (m = 1; m < sdef->elements[i].raw.union_member_num + 1; m++) {
	src = mputprintf(src, "  case %s%s%s:\n"
	  "    value_of_length_field%d += (int)field_%s%s.%s() * %d;\n"
	  "    break;\n", sdef->elements[i].raw.member_name[0],
	  "::ALT_",
	  sdef->elements[i].raw.member_name[m], i, sdef->elements[i].name,
	  sdef->elements[i].isOptional ? "()" : "",
	  sdef->elements[i].raw.member_name[m],
	  sdef->elements[i].raw.unit == -1 ? 1 : sdef->elements[i].raw.unit);
      }
      src = mputprintf(src, "  default:\n"
	"    value_of_length_field%d = 0;\n"
	"  }\n", i);
    }
    else{
      src=mputprintf(src,
        "  value_of_length_field%d+=(int)field_%s%s*%d;\n"
        ,i,sdef->elements[i].name,sdef->elements[i].isOptional?"()":""
        ,sdef->elements[i].raw.unit==-1?1:sdef->elements[i].raw.unit
      );
    }
  }
  if(raw_options[i].pointerto){   /* store the start of pointed field*/
    src=mputprintf(src,
      "  start_of_field%d=(int)field_%s%s+%d?1:-1;\n"
      ,sdef->elements[i].raw.pointerto
      ,sdef->elements[i].name,sdef->elements[i].isOptional?"()":""
      ,sdef->elements[i].raw.ptroffset
    );
  }
  if (!delayed_decode) {
    /* mark the used bits in length area*/
    for (a = 0; a < raw_options[i].lengthof; a++) {
      int field_index = raw_options[i].lengthoffield[a];
      src = mputprintf(src,
	"  value_of_length_field%d -= decoded_field_length;\n", field_index);
      if (i == field_index) src = mputprintf(src,
	  "  if (value_of_length_field%d < 0) return -1;\n", field_index);
    }
  }
  if(sdef->elements[i].isOptional){
    src=mputprintf(src,
    "  }\n  }\n%s"
    "  else field_%s=OMIT_VALUE;\n"
    ,raw_options[i].tag_type?"  }\n":"",sdef->elements[i].name
    );
  }
  return src;
}

#ifdef __SUNPRO_C
#define SUNPRO_PUBLIC  "public:\n"
#define SUNPRO_PRIVATE "private:\n"
#else
#define SUNPRO_PUBLIC
#define SUNPRO_PRIVATE
#endif

void defRecordTemplate1(const struct_def *sdef, output_struct *output)
{
    int i;
    const char *name = sdef->name, *dispname = sdef->dispname;
    const char* kind_str = sdef->kind == SET ? "set" : "record";
    char *def = NULL, *src = NULL;

    size_t mandatory_fields_count;

    /* class declaration */
    output->header.class_decls = mputprintf(output->header.class_decls,
	"class %s_template;\n", name);

    if(sdef->nElements <= 0) {
	defEmptyRecordTemplate(name, dispname, output);
	return;
    }

    /* template class definition */
    def = mputprintf(def, "class %s_template : public Base_Template {\n", name);

    /* data members */
    def = mputprintf(def,
	SUNPRO_PUBLIC
	"struct single_value_struct;\n"
	SUNPRO_PRIVATE
	"union {\n"
	"single_value_struct *single_value;\n"
	"struct {\n"
	"unsigned int n_values;\n"
	"%s_template *list_value;\n"
	"} value_list;\n"
	"};\n\n", name);
    /* the definition of single_value_struct must be put into the source file
     * because the types of optional fields may be incomplete in this position
     * of header file (e.g. due to type recursion) */
    src = mputprintf(src, "struct %s_template::single_value_struct {\n", name);
    for (i = 0; i < sdef->nElements; i++) {
	src = mputprintf(src, "%s_template field_%s;\n",
	    sdef->elements[i].type, sdef->elements[i].name);
    }
    src = mputstr(src, "};\n\n");

    /* set_specific function (used in field access members) */
    def = mputstr(def, "void set_specific();\n");
    src = mputprintf(src, "void %s_template::set_specific()\n"
	"{\n"
	"if (template_selection != SPECIFIC_VALUE) {\n"
	"template_sel old_selection = template_selection;\n"
	"clean_up();\n"
	"single_value = new single_value_struct;\n"
	"set_selection(SPECIFIC_VALUE);\n"
	"if (old_selection == ANY_VALUE || old_selection == ANY_OR_OMIT) {\n",
	name);
    for (i = 0; i < sdef->nElements; i++) {
	src = mputprintf(src, "single_value->field_%s = %s;\n",
	    sdef->elements[i].name,
	    sdef->elements[i].isOptional ? "ANY_OR_OMIT" : "ANY_VALUE");
    }
    src = mputstr(src, "}\n"
	"}\n"
	"}\n\n");

    /* copy_value function */
    def = mputprintf(def, "void copy_value(const %s& other_value);\n", name);
    src = mputprintf(src,
	"void %s_template::copy_value(const %s& other_value)\n"
	"{\n"
	"single_value = new single_value_struct;\n", name, name);
    for (i = 0; i < sdef->nElements; i++) {
        src = mputprintf(src,
          "if (other_value.%s().is_bound()) {\n", sdef->elements[i].name);
        if (sdef->elements[i].isOptional) {
          /* If the value is present, "reach into" the OPTIONAL with operator()
           * and pass the contained object to the template's constructor.
           * Else set the template to omit. */
          src = mputprintf(src,
	    "  if (other_value.%s().ispresent()) single_value->field_%s = "
		"other_value.%s()();\n"
	    "  else single_value->field_%s = OMIT_VALUE;\n",
	    sdef->elements[i].name, sdef->elements[i].name,
	    sdef->elements[i].name, sdef->elements[i].name);
        } else {
           src = mputprintf(src,
	    "  single_value->field_%s = other_value.%s();\n",
	    sdef->elements[i].name, sdef->elements[i].name);
        }
        src = mputprintf(src, "} else {\n"
          "  single_value->field_%s.clean_up();\n"
          "}\n", sdef->elements[i].name);
    }
    src = mputstr(src, "set_selection(SPECIFIC_VALUE);\n"
	"}\n\n");

    /* copy_template function */
    def = mputprintf(def, "void copy_template(const %s_template& "
	"other_value);\n\n", name);
    src = mputprintf(src,
	"void %s_template::copy_template(const %s_template& other_value)\n"
	"{\n"
	"switch (other_value.template_selection) {\n"
	"case SPECIFIC_VALUE:\n", name, name);
    src = mputstr(src,
        "single_value = new single_value_struct;\n");
    for (i = 0; i < sdef->nElements; i++) {
        src = mputprintf(src,
          "if (UNINITIALIZED_TEMPLATE != other_value.%s().get_selection()) {\n",
          sdef->elements[i].name);
        src = mputprintf(src,
          "single_value->field_%s = other_value.%s();\n",
          sdef->elements[i].name, sdef->elements[i].name);
        src = mputprintf(src, "} else {\n"
          "single_value->field_%s.clean_up();\n"
          "}\n", sdef->elements[i].name);
    }
    src = mputprintf(src,
	"case OMIT_VALUE:\n"
	"case ANY_VALUE:\n"
	"case ANY_OR_OMIT:\n"
	"break;\n"
	"case VALUE_LIST:\n"
	"case COMPLEMENTED_LIST:\n"
	"value_list.n_values = other_value.value_list.n_values;\n"
	"value_list.list_value = new %s_template[value_list.n_values];\n"
	"for (unsigned int list_count = 0; list_count < value_list.n_values; "
	    "list_count++)\n"
	"value_list.list_value[list_count].copy_template("
	    "other_value.value_list.list_value[list_count]);\n"
	"break;\n"
	"default:\n"
	"TTCN_error(\"Copying an uninitialized/unsupported template of type "
	    "%s.\");\n"
	"break;\n"
	"}\n"
	"set_selection(other_value);\n"
	"}\n\n", name, dispname);

    /* default constructor */
    def = mputprintf(def, "public:\n"
	"%s_template();\n", name);
    src = mputprintf(src, "%s_template::%s_template()\n"
	"{\n"
	"}\n\n", name, name);

    /* constructor t1_template(template_sel other_value) */
    def = mputprintf(def, "%s_template(template_sel other_value);\n", name);
    src = mputprintf(src, "%s_template::%s_template(template_sel other_value)\n"
	" : Base_Template(other_value)\n"
	"{\n"
	"check_single_selection(other_value);\n"
	"}\n\n", name, name);

    /* constructor t1_template(const t1& other_value) */
    def = mputprintf(def, "%s_template(const %s& other_value);\n", name, name);
    src = mputprintf(src, "%s_template::%s_template(const %s& other_value)\n"
	"{\n"
	"copy_value(other_value);\n"
	"}\n\n", name, name, name);

    /* constructor t1_template(const OPTIONAL<t1>& other_value) */
    def = mputprintf(def, "%s_template(const OPTIONAL<%s>& other_value);\n",
	name, name);
    src = mputprintf(src,
	"%s_template::%s_template(const OPTIONAL<%s>& other_value)\n"
	"{\n"
	"switch (other_value.get_selection()) {\n"
	"case OPTIONAL_PRESENT:\n"
	"copy_value((const %s&)other_value);\n"
	"break;\n"
	"case OPTIONAL_OMIT:\n"
	"set_selection(OMIT_VALUE);\n"
	"break;\n"
	"default:\n"
	"TTCN_error(\"Creating a template of type %s from an unbound optional "
	    "field.\");\n"
	"}\n"
	"}\n\n", name, name, name, name, dispname);

    /* copy constructor */
    def = mputprintf(def, "%s_template(const %s_template& other_value);\n",
	name, name);
    src = mputprintf(src, "%s_template::%s_template(const %s_template& "
	"other_value)\n"
	": Base_Template()\n" /* yes, the base class _default_ constructor */
	"{\n"
	"copy_template(other_value);\n"
	"}\n\n", name, name, name);

    /* destructor */
    def = mputprintf(def, "~%s_template();\n", name);
    src = mputprintf(src, "%s_template::~%s_template()\n"
	"{\n"
	"clean_up();\n"
	"}\n\n", name, name);

    /* assignment operator <- template_sel */
    def = mputprintf(def, "%s_template& operator=(template_sel other_value);\n",
	name);
    src = mputprintf(src,
	"%s_template& %s_template::operator=(template_sel other_value)\n"
	"{\n"
	"check_single_selection(other_value);\n"
	"clean_up();\n"
	"set_selection(other_value);\n"
	"return *this;\n"
	"}\n\n", name, name);

    /* assignment operator <- value */
    def = mputprintf(def, "%s_template& operator=(const %s& other_value);\n",
	name, name);

    src = mputprintf(src,
	"%s_template& %s_template::operator=(const %s& other_value)\n"
	"{\n"
	"clean_up();\n"
	"copy_value(other_value);\n"
	"return *this;\n"
	"}\n\n", name, name, name);

    /* assignment operator <- optional value */
    def = mputprintf(def, "%s_template& operator=(const OPTIONAL<%s>& "
	"other_value);\n", name, name);

    src = mputprintf(src,
	"%s_template& %s_template::operator=(const OPTIONAL<%s>& other_value)\n"
	"{\n"
	"clean_up();\n"
	"switch (other_value.get_selection()) {\n"
	"case OPTIONAL_PRESENT:\n"
	"copy_value((const %s&)other_value);\n"
	"break;\n"
	"case OPTIONAL_OMIT:\n"
	"set_selection(OMIT_VALUE);\n"
	"break;\n"
	"default:\n"
	"TTCN_error(\"Assignment of an unbound optional field to a template "
	  "of type %s.\");\n"
	"}\n"
	"return *this;\n"
	"}\n\n", name, name, name, name, dispname);

    /* assignment operator <- template*/
    def = mputprintf(def,
	"%s_template& operator=(const %s_template& other_value);\n",
	name, name);

    src = mputprintf(src,
	"%s_template& %s_template::operator=(const %s_template& other_value)\n"
	"{\n"
	"if (&other_value != this) {\n"
	"clean_up();\n"
	"copy_template(other_value);\n"
	"}\n"
	"return *this;\n"
	"}\n\n", name, name, name);

    /* match operation (template matching) */
    def = mputprintf(def, "boolean match(const %s& other_value, boolean legacy "
      "= FALSE) const;\n", name);

    src = mputprintf(src,
	"boolean %s_template::match(const %s& other_value, boolean legacy) const\n"
	"{\n"
    "if (!other_value.is_bound()) return FALSE;\n"
	"switch (template_selection) {\n"
	"case ANY_VALUE:\n"
	"case ANY_OR_OMIT:\n"
	"return TRUE;\n"
	"case OMIT_VALUE:\n"
	"return FALSE;\n"
	"case SPECIFIC_VALUE:\n", name, name);
    for (i = 0; i < sdef->nElements; i++) {
      src = mputprintf(src,"if(!other_value.%s().is_bound()) return FALSE;\n", sdef->elements[i].name);
	if (sdef->elements[i].isOptional) src = mputprintf(src,
	    "if((other_value.%s().ispresent() ? "
	    "!single_value->field_%s.match((const %s&)other_value.%s(), legacy) : "
	    "!single_value->field_%s.match_omit(legacy)))",
	    sdef->elements[i].name, sdef->elements[i].name,
	    sdef->elements[i].type, sdef->elements[i].name,
	    sdef->elements[i].name);
	else src = mputprintf(src,
	    "if(!single_value->field_%s.match(other_value.%s(), legacy))",
	    sdef->elements[i].name, sdef->elements[i].name);
	  src = mputstr(src, "return FALSE;\n");
    }
    src = mputprintf(src,
	"return TRUE;\n"
	"case VALUE_LIST:\n"
	"case COMPLEMENTED_LIST:\n"
	"for (unsigned int list_count = 0; list_count < value_list.n_values; "
	    "list_count++)\n"
	"if (value_list.list_value[list_count].match(other_value, legacy)) "
	    "return template_selection == VALUE_LIST;\n"
	"return template_selection == COMPLEMENTED_LIST;\n"
	"default:\n"
	"TTCN_error(\"Matching an uninitialized/unsupported template of "
	    "type %s.\");\n"
	"}\n"
	"return FALSE;\n"
	"}\n\n", dispname);

    /* is_bound */
    def = mputstr(def, "boolean is_bound() const;\n");

    src = mputprintf(src, "boolean %s_template::is_bound() const\n"
        "{\n"
        "if (template_selection == UNINITIALIZED_TEMPLATE && !is_ifpresent) "
        "return FALSE;\n", name);
    src = mputstr(src, "if (template_selection != SPECIFIC_VALUE) return TRUE;\n");
    for (i = 0; i < sdef->nElements; i++) {
        if (sdef->elements[i].isOptional) {
            src = mputprintf(src,
                "if (single_value->field_%s.is_omit() ||"
                    " single_value->field_%s.is_bound()) return TRUE;\n",
                sdef->elements[i].name, sdef->elements[i].name);
        } else {
            src = mputprintf(src,
                "if (single_value->field_%s.is_bound()) return TRUE;\n",
                sdef->elements[i].name);
        }
    }
    src = mputstr(src, "return FALSE;\n"
        "}\n\n");

    /* is_value */
    def = mputstr(def, "boolean is_value() const;\n");

    src = mputprintf(src, "boolean %s_template::is_value() const\n"
        "{\n"
        "if (template_selection != SPECIFIC_VALUE || is_ifpresent) "
        "return FALSE;\n", name);
    for (i = 0; i < sdef->nElements; i++) {
        if (sdef->elements[i].isOptional) {
            src = mputprintf(src,
                "if (!single_value->field_%s.is_omit() &&"
                    " !single_value->field_%s.is_value()) return FALSE;\n",
                sdef->elements[i].name, sdef->elements[i].name);
        } else {
            src = mputprintf(src,
                "if (!single_value->field_%s.is_value()) return FALSE;\n",
                sdef->elements[i].name);
        }
    }
    src = mputstr(src, "return TRUE;\n"
        "}\n\n");

    /* clean_up function */
    def = mputstr(def, "void clean_up();\n");
    src = mputprintf(src,
	"void %s_template::clean_up()\n"
	"{\n"
	"switch (template_selection) {\n"
	"case SPECIFIC_VALUE:\n"
	"delete single_value;\n"
	"break;\n"
	"case VALUE_LIST:\n"
	"case COMPLEMENTED_LIST:\n"
	"delete [] value_list.list_value;\n"
	"default:\n"
	"break;\n"
	"}\n"
	"template_selection = UNINITIALIZED_TEMPLATE;\n"
	"}\n\n", name);

    /* valueof operation */
    def = mputprintf(def, "%s valueof() const;\n", name);

    src = mputprintf(src, "%s %s_template::valueof() const\n"
	"{\n"
	"if (template_selection != SPECIFIC_VALUE || is_ifpresent)\n"
	"TTCN_error(\"Performing a valueof or send operation on a non-specific "
	"template of type %s.\");\n"
	"%s ret_val;\n", name, name, dispname, name);
    for (i = 0; i < sdef->nElements; i++) {
    	if (sdef->elements[i].isOptional) {
    	  src = mputprintf(src,
		  "if (single_value->field_%s.is_omit()) "
		    "ret_val.%s() = OMIT_VALUE;\n"
    	   "else ",
		  sdef->elements[i].name, sdef->elements[i].name);
    	}
    	src = mputprintf(src, "if (single_value->field_%s.is_bound()) {\n"
    		"ret_val.%s() = single_value->field_%s.valueof();\n"
    		"}\n",
    	  sdef->elements[i].name, sdef->elements[i].name, sdef->elements[i].name);
    }
    src = mputstr(src, "return ret_val;\n"
    "}\n\n");

    /* void set_type(template_sel, int) function */
    def = mputstr(def,
	"void set_type(template_sel template_type, "
	    "unsigned int list_length);\n");

    src = mputprintf(src,
	"void %s_template::set_type(template_sel template_type, "
	    "unsigned int list_length)\n"
	"{\n"
	"if (template_type != VALUE_LIST "
	    "&& template_type != COMPLEMENTED_LIST)\n"
	"TTCN_error(\"Setting an invalid list for a template of type %s.\");\n"
	"clean_up();\n"
	"set_selection(template_type);\n"
	"value_list.n_values = list_length;\n"
	"value_list.list_value = new %s_template[list_length];\n"
	"}\n\n", name, dispname, name);

    /* list_item(int) function */

    def = mputprintf(def,
	"%s_template& list_item(unsigned int list_index) const;\n", name);

    src = mputprintf(src,
	"%s_template& %s_template::list_item(unsigned int list_index) const\n"
	"{\n"
	"if (template_selection != VALUE_LIST "
	    "&& template_selection != COMPLEMENTED_LIST)\n"
	"TTCN_error(\"Accessing a list element of a non-list template of "
	"type %s.\");\n"
	"if (list_index >= value_list.n_values)\n"
	"TTCN_error(\"Index overflow in a value list template of type "
	"%s.\");\n"
	"return value_list.list_value[list_index];\n"
	"}\n\n", name, name, dispname, dispname);

    /* template field access functions (non-const & const) */
    for (i = 0; i < sdef->nElements; i++) {
	def = mputprintf(def, "%s_template& %s();\n",
	    sdef->elements[i].type, sdef->elements[i].name);
	src = mputprintf(src, "%s_template& %s_template::%s()\n"
	    "{\n"
	    "set_specific();\n"
	    "return single_value->field_%s;\n"
	    "}\n\n",
	    sdef->elements[i].type, name, sdef->elements[i].name,
	    sdef->elements[i].name);
	def = mputprintf(def, "const %s_template& %s() const;\n",
	    sdef->elements[i].type, sdef->elements[i].name);
	src = mputprintf(src, "const %s_template& %s_template::%s() const\n"
	    "{\n"
	    "if (template_selection != SPECIFIC_VALUE)\n"
	    "TTCN_error(\"Accessing field %s of a non-specific "
		"template of type %s.\");\n"
	    "return single_value->field_%s;\n"
	    "}\n\n",
	    sdef->elements[i].type, name, sdef->elements[i].name,
	    sdef->elements[i].dispname, dispname, sdef->elements[i].name);
    }

    /* sizeof operation */
    def = mputstr(def, "int size_of() const;\n");
    src = mputprintf(src,
      "int %s_template::size_of() const\n"
      "{\n"
      "  if (is_ifpresent) TTCN_error(\"Performing sizeof() operation on a "
        "template of type %s which has an ifpresent attribute.\");\n"
      "  switch (template_selection)\n"
      "  {\n"
      "  case SPECIFIC_VALUE:\n",
      name, dispname);
    mandatory_fields_count = 0;
    for (i = 0; i < sdef->nElements; i++)
      if (!sdef->elements[i].isOptional) mandatory_fields_count++;
    if(sdef->nElements == mandatory_fields_count){
      src = mputprintf(src,"    return %lu;\n",
        (unsigned long) mandatory_fields_count);
    }else{
      src = mputprintf(src,
        "  {"
        "    int ret_val = %lu;\n", (unsigned long) mandatory_fields_count);
      for (i = 0; i < sdef->nElements; i++)
        if (sdef->elements[i].isOptional)
          src = mputprintf(src,
            "      if (single_value->field_%s.is_present()) ret_val++;\n",
            sdef->elements[i].name);
      src = mputstr(src,
        "      return ret_val;\n"
        "    }\n");
    }
    src = mputprintf(src,

      "  case VALUE_LIST:\n"
      "   {\n"
      "     if (value_list.n_values<1)\n"
      "       TTCN_error(\"Internal error: Performing sizeof() operation on a "
      "template of type %s containing an empty list.\");\n"
      "      int item_size = value_list.list_value[0].size_of();\n"
      "      for (unsigned int l_idx = 1; l_idx < value_list.n_values; l_idx++)\n"
      "      {\n"
      "        if (value_list.list_value[l_idx].size_of()!=item_size)\n"
      "          TTCN_error(\"Performing sizeof() operation on a template "
      "of type %s containing a value list with different sizes.\");\n"
      "      }\n"
      "      return item_size;\n"
      "    }\n"
      "  case OMIT_VALUE:\n"
      "    TTCN_error(\"Performing sizeof() operation on a template of type %s "
      "containing omit value.\");\n"
      "  case ANY_VALUE:\n"
      "  case ANY_OR_OMIT:\n"
      "    TTCN_error(\"Performing sizeof() operation on a template of type %s "
      "containing */? value.\");\n"
      "  case COMPLEMENTED_LIST:\n"
      "    TTCN_error(\"Performing sizeof() operation on a template of type %s "
      "containing complemented list.\");\n"
      "  default:\n"
      "    TTCN_error(\"Performing sizeof() operation on an "
      "uninitialized/unsupported template of type %s.\");\n"
      "  }\n"
      "  return 0;\n"
      "}\n\n",
      dispname,dispname,dispname,dispname,dispname,dispname);

    /* log function */
    def = mputstr(def, "void log() const;\n");
    src = mputprintf(src,
	"void %s_template::log() const\n"
	"{\n"
	"switch (template_selection) {\n"
	"case SPECIFIC_VALUE:\n", name);
    for (i = 0; i < sdef->nElements; i++) {
	src = mputstr(src, "TTCN_Logger::log_event_str(\"");
	if (i == 0) src = mputc(src, '{');
	else src = mputc(src, ',');
	src = mputprintf(src, " %s := \");\n"
	    "single_value->field_%s.log();\n",
	    sdef->elements[i].dispname, sdef->elements[i].name);
    }
    src = mputstr(src,
	"TTCN_Logger::log_event_str(\" }\");\n"
	"break;\n"
	"case COMPLEMENTED_LIST:\n"
	"TTCN_Logger::log_event_str(\"complement \");\n"
	"case VALUE_LIST:\n"
	"TTCN_Logger::log_char('(');\n"
	"for (unsigned int list_count = 0; list_count < value_list.n_values; "
	    "list_count++) {\n"
	"if (list_count > 0) TTCN_Logger::log_event_str(\", \");\n"
	"value_list.list_value[list_count].log();\n"
	"}\n"
	"TTCN_Logger::log_char(')');\n"
	"break;\n"
	"default:\n"
	"log_generic();\n"
	"}\n"
	"log_ifpresent();\n"
	"}\n\n");

    /* log_match function */
    def = mputprintf(def, "void log_match(const %s& match_value, "
      "boolean legacy = FALSE) const;\n", name);
    src = mputprintf(src,
    "void %s_template::log_match(const %s& match_value, boolean legacy) const\n"
    "{\n"
    "if(TTCN_Logger::VERBOSITY_COMPACT"
      " == TTCN_Logger::get_matching_verbosity()){\n"
    "if(match(match_value, legacy)){\n"
    "TTCN_Logger::print_logmatch_buffer();\n"
    "TTCN_Logger::log_event_str(\" matched\");\n"
    "} else{\n"
    "if (template_selection == SPECIFIC_VALUE) {\n"
    "size_t previous_size = TTCN_Logger::get_logmatch_buffer_len();\n"
    , name, name);
    for (i = 0; i < sdef->nElements; i++) {
      if (sdef->elements[i].isOptional){
        src = mputprintf(src,
        "if (match_value.%s().ispresent()){\n"
        "if(!single_value->field_%s.match(match_value.%s(), legacy)){\n"
        "TTCN_Logger::log_logmatch_info(\".%s\");\n"
        "single_value->field_%s.log_match(match_value.%s(), legacy);\n"
        "TTCN_Logger::set_logmatch_buffer_len(previous_size);\n"
        "}\n"
        "} else {\n"
        "if (!single_value->field_%s.match_omit(legacy)){\n "
        "TTCN_Logger::log_logmatch_info(\".%s := omit with \");\n"
        "TTCN_Logger::print_logmatch_buffer();\n"
        "single_value->field_%s.log();\n"
        "TTCN_Logger::log_event_str(\" unmatched\");\n"
        "TTCN_Logger::set_logmatch_buffer_len(previous_size);\n"
        "}\n"
        "}\n"
        ,  sdef->elements[i].name, sdef->elements[i].name,
        sdef->elements[i].name, sdef->elements[i].dispname,
        sdef->elements[i].name, sdef->elements[i].name,
        sdef->elements[i].name, sdef->elements[i].dispname,
        sdef->elements[i].name);
      }else{
        src = mputprintf(src,
        "if(!single_value->field_%s.match(match_value.%s(), legacy)){\n"
        "TTCN_Logger::log_logmatch_info(\".%s\");\n"
        "single_value->field_%s.log_match(match_value.%s(), legacy);\n"
        "TTCN_Logger::set_logmatch_buffer_len(previous_size);\n"
        "}\n",sdef->elements[i].name, sdef->elements[i].name,
        sdef->elements[i].dispname, sdef->elements[i].name,
        sdef->elements[i].name);
      }
    }

    src = mputstr(src,"}else {\n"
    "TTCN_Logger::print_logmatch_buffer();\n"
    "match_value.log();\n"
    "TTCN_Logger::log_event_str(\" with \");\n"
    "log();\n"
    "TTCN_Logger::log_event_str(\" unmatched\");\n"
    "}\n"
    "}\n"
    "return;\n"
    "}\n"
    "if (template_selection == SPECIFIC_VALUE) {\n");
    for (i = 0; i < sdef->nElements; i++) {
	src = mputstr(src, "TTCN_Logger::log_event_str(\"");
	if (i == 0) src = mputc(src, '{');
	else src = mputc(src, ',');
	src = mputprintf(src, " %s := \");\n", sdef->elements[i].dispname);
	if (sdef->elements[i].isOptional) src = mputprintf(src,
	    "if (match_value.%s().ispresent()) "
	    "single_value->field_%s.log_match(match_value.%s(), legacy);\n"
	    "else {\n"
	    "TTCN_Logger::log_event_str(\"omit with \");\n"
	    "single_value->field_%s.log();\n"
            "if (single_value->field_%s.match_omit(legacy)) "
              "TTCN_Logger::log_event_str(\" matched\");\n"
            "else TTCN_Logger::log_event_str(\" unmatched\");\n"
	    "}\n",
	    sdef->elements[i].name, sdef->elements[i].name,
	    sdef->elements[i].name, sdef->elements[i].name,
	    sdef->elements[i].name);
	else src = mputprintf(src,
	    "single_value->field_%s.log_match(match_value.%s(), legacy);\n",
	    sdef->elements[i].name, sdef->elements[i].name);
    }
    src = mputstr(src,
	"TTCN_Logger::log_event_str(\" }\");\n"
	"} else {\n"
	"match_value.log();\n"
	"TTCN_Logger::log_event_str(\" with \");\n"
	"log();\n"
	"if (match(match_value, legacy)) TTCN_Logger::log_event_str(\" matched\");\n"
	"else TTCN_Logger::log_event_str(\" unmatched\");\n"
	"}\n"
	"}\n\n");

    /*encode_text function*/
    def = mputstr(def, "void encode_text(Text_Buf& text_buf) const;\n");
    src = mputprintf(src,
	"void %s_template::encode_text(Text_Buf& text_buf) const\n"
	"{\n"
	"encode_text_base(text_buf);\n"
	"switch (template_selection) {\n"
	"case SPECIFIC_VALUE:\n", name);
    for (i = 0; i < sdef->nElements; i++) {
	src = mputprintf(src, "single_value->field_%s.encode_text(text_buf);\n",
	    sdef->elements[i].name);
    }
    src = mputprintf(src,
	"case OMIT_VALUE:\n"
	"case ANY_VALUE:\n"
	"case ANY_OR_OMIT:\n"
	"break;\n"
	"case VALUE_LIST:\n"
	"case COMPLEMENTED_LIST:\n"
	"text_buf.push_int(value_list.n_values);\n"
	"for (unsigned int list_count = 0; list_count < value_list.n_values; "
	    "list_count++)\n"
	"value_list.list_value[list_count].encode_text(text_buf);\n"
	"break;\n"
	"default:\n"
	"TTCN_error(\"Text encoder: Encoding an uninitialized/unsupported "
	    "template of type %s.\");\n"
	"}\n"
	"}\n\n", dispname);

    /*decode_text function*/
    def = mputstr(def, "void decode_text(Text_Buf& text_buf);\n");
    src = mputprintf(src,
	"void %s_template::decode_text(Text_Buf& text_buf)\n"
	"{\n"
	"clean_up();\n"
	"decode_text_base(text_buf);\n"
	"switch (template_selection) {\n"
	"case SPECIFIC_VALUE:\n"
	"single_value = new single_value_struct;\n", name);
    for (i = 0; i < sdef->nElements; i++) {
	src = mputprintf(src, "single_value->field_%s.decode_text(text_buf);\n",
	    sdef->elements[i].name);
    }
    src = mputprintf(src,
	"case OMIT_VALUE:\n"
	"case ANY_VALUE:\n"
	"case ANY_OR_OMIT:\n"
	"break;\n"
	"case VALUE_LIST:\n"
	"case COMPLEMENTED_LIST:\n"
	"value_list.n_values = text_buf.pull_int().get_val();\n"
	"value_list.list_value = new %s_template[value_list.n_values];\n"
	"for (unsigned int list_count = 0; list_count < value_list.n_values; "
	    "list_count++)\n"
	"value_list.list_value[list_count].decode_text(text_buf);\n"
	"break;\n"
	"default:\n"
	"TTCN_error(\"Text decoder: An unknown/unsupported selection was "
	    "received in a template of type %s.\");\n"
	"}\n"
	"}\n\n", name, dispname);

  /* set_param() */
  def = mputstr(def, "void set_param(Module_Param& param);\n");
  src = mputprintf(src,
    "void %s_template::set_param(Module_Param& param)\n"
    "{\n"
    "  param.basic_check(Module_Param::BC_TEMPLATE, \"%s template\");\n"
    "  switch (param.get_type()) {\n"
    "  case Module_Param::MP_Omit:\n"
    "    *this = OMIT_VALUE;\n"
    "    break;\n"
    "  case Module_Param::MP_Any:\n"
    "    *this = ANY_VALUE;\n"
    "    break;\n"
    "  case Module_Param::MP_AnyOrNone:\n"
    "    *this = ANY_OR_OMIT;\n"
    "    break;\n"
    "  case Module_Param::MP_List_Template:\n"
    "  case Module_Param::MP_ComplementList_Template: {\n"
    "    %s_template new_temp;\n"
    "    new_temp.set_type(param.get_type()==Module_Param::MP_List_Template ? "
    "VALUE_LIST : COMPLEMENTED_LIST, param.get_size());\n"
    "    for (size_t p_i=0; p_i<param.get_size(); p_i++) {\n"
    "      new_temp.list_item(p_i).set_param(*param.get_elem(p_i));\n"
    "    }\n"
    "    *this = new_temp;\n"
    "    break; }\n"
    "  case Module_Param::MP_Value_List:\n"
    "    if (%lu<param.get_size()) {\n"
    "      param.error(\"%s template of type %s has %lu fields but list value has %%d fields\", (int)param.get_size());\n"
    "    }\n",
    name, kind_str, name, (unsigned long)sdef->nElements, kind_str, dispname, (unsigned long)sdef->nElements);
  for (i = 0; i < sdef->nElements; ++i) {
    src = mputprintf(src,
      "    if (param.get_size()>%lu && param.get_elem(%lu)->get_type()!=Module_Param::MP_NotUsed) %s().set_param(*param.get_elem(%lu));\n",
      (unsigned long)i, (unsigned long)i, sdef->elements[i].name, (unsigned long)i);
  }
  src = mputstr(src,
    "    break;\n"
    "  case Module_Param::MP_Assignment_List: {\n"
    "    Vector<bool> value_used(param.get_size());\n"
    "    value_used.resize(param.get_size(), FALSE);\n");
  for (i = 0; i < sdef->nElements; ++i) {
    src = mputprintf(src,
      "    for (size_t val_idx=0; val_idx<param.get_size(); val_idx++) {\n"
      "      Module_Param* const curr_param = param.get_elem(val_idx);\n"
      "      if (!strcmp(curr_param->get_id()->get_name(), \"%s\")) {\n"
      "        if (curr_param->get_type()!=Module_Param::MP_NotUsed) {\n"
      "          %s().set_param(*curr_param);\n"
      "        }\n"
      "        value_used[val_idx]=TRUE;\n"
      "      }\n"
      "    }\n"
      , sdef->elements[i].dispname, sdef->elements[i].name);
  }
  src = mputprintf(src,
    "    for (size_t val_idx=0; val_idx<param.get_size(); val_idx++) if (!value_used[val_idx]) {\n"
    "      param.get_elem(val_idx)->error(\"Non existent field name in type %s: %%s\", param.get_elem(val_idx)->get_id()->get_name());\n"
    "      break;\n"
    "    }\n"
    "  } break;\n"
    "  default:\n"
    "    param.type_error(\"%s template\", \"%s\");\n"
    "  }\n"
    "  is_ifpresent = param.get_ifpresent();\n"
    "}\n\n", dispname, kind_str, dispname);

    /* check template restriction */
    def = mputstr(def, "void check_restriction(template_res t_res, "
        "const char* t_name=NULL, boolean legacy = FALSE) const;\n");
    src = mputprintf(src,
        "void %s_template::check_restriction("
          "template_res t_res, const char* t_name, boolean legacy) const\n"
        "{\n"
        "if (template_selection==UNINITIALIZED_TEMPLATE) return;\n"
        "switch ((t_name&&(t_res==TR_VALUE))?TR_OMIT:t_res) {\n"
        "case TR_OMIT:\n"
        "if (template_selection==OMIT_VALUE) return;\n"
        "case TR_VALUE:\n"
        "if (template_selection!=SPECIFIC_VALUE || is_ifpresent) break;\n",
        name);
    for (i = 0; i < sdef->nElements; i++) {
	    src = mputprintf(src, "single_value->field_%s.check_restriction("
        "t_res, t_name ? t_name : \"%s\");\n",
	      sdef->elements[i].name, dispname);
    }
    src = mputprintf(src,
        "return;\n"
        "case TR_PRESENT:\n"
        "if (!match_omit(legacy)) return;\n"
        "break;\n"
        "default:\n"
        "return;\n"
        "}\n"
        "TTCN_error(\"Restriction `%%s' on template of type %%s "
          "violated.\", get_res_name(t_res), t_name ? t_name : \"%s\");\n"
        "}\n\n", dispname);

    defCommonRecordTemplate(name, &def, &src);

    def = mputstr(def, "};\n\n");

    output->header.class_defs = mputstr(output->header.class_defs, def);
    Free(def);

    output->source.methods = mputstr(output->source.methods, src);
    Free(src);
}

static void defEmptyRecordClass(const struct_def *sdef,
                                output_struct *output)
{
    const char *name = sdef->name, *dispname = sdef->dispname;
    char *def = NULL, *src = NULL;
    boolean ber_needed = sdef->isASN1 && enable_ber();
    boolean raw_needed = sdef->hasRaw && enable_raw();
    boolean text_needed = sdef->hasText && enable_text();
    boolean xer_needed = sdef->hasXer && enable_xer();
    boolean json_needed = sdef->hasJson && enable_json();

    def = mputprintf(def,
#ifndef NDEBUG
        "// written by %s in " __FILE__ " at line %d\n"
#endif
        "class %s : public Base_Type {\n"
	"boolean bound_flag;\n"
	"public:\n"
#ifndef NDEBUG
        , __FUNCTION__, __LINE__
#endif
        , name);

    /* default ctor */
    def = mputprintf(def, "%s();\n", name);
    src = mprintf("%s::%s()\n"
	"{\n"
	"bound_flag = FALSE;\n"
	"}\n\n", name, name);

    /* ctor from NULL_VALUE (i.e. {}) */
    def = mputprintf(def, "%s(null_type other_value);\n", name);
    src = mputprintf(src, "%s::%s(null_type)\n"
	"{\n"
	"bound_flag = TRUE;\n"
	"}\n\n", name, name);

    /* copy ctor */
    def = mputprintf(def, "%s(const %s& other_value);\n", name, name);
    src = mputprintf(src, "%s::%s(const %s& other_value)\n"
	"{\n"
	"other_value.must_bound(\"Copying an unbound value of "
	    "type %s.\");\n"
	"bound_flag = TRUE;\n"
	"}\n\n", name, name, name, dispname);

    /* assignment op: from NULL_VALUE */
    def = mputprintf(def, "%s& operator=(null_type other_value);\n", name);
    src = mputprintf(src, "%s& %s::operator=(null_type)\n"
	"{\n"
	"bound_flag = TRUE;\n"
	"return *this;\n"
	"}\n\n", name, name);

    /* assignment op: from itself */
    def = mputprintf(def, "%s& operator=(const %s& other_value);\n", name,
	name);
    src = mputprintf(src, "%s& %s::operator=(const %s& other_value)\n"
	"{\n"
	"other_value.must_bound(\"Assignment of an unbound value of type "
	    "%s.\");\n"
	"bound_flag = TRUE;\n"
	"return *this;\n"
	"}\n\n", name, name, name, dispname);

    /* comparison op: with NULL_VALUE */
    def = mputstr(def, "boolean operator==(null_type other_value) const;\n");
    src = mputprintf(src,
	"boolean %s::operator==(null_type) const\n"
	"{\n"
	"must_bound(\"Comparison of an unbound value of type %s.\");\n"
	"return TRUE;\n"
	"}\n\n", name, dispname);

    /* comparison op: with itself */
    def = mputprintf(def, "boolean operator==(const %s& other_value) const;\n",
	name);
    src = mputprintf(src,
	"boolean %s::operator==(const %s& other_value) const\n"
	"{\n"
	"must_bound(\"Comparison of an unbound value of type %s.\");\n"
	"other_value.must_bound(\"Comparison of an unbound value of type "
	    "%s.\");\n"
	"return TRUE;\n"
	"}\n\n", name, name, dispname, dispname);

    /* non-equal operators */
    def = mputprintf(def,
	"inline boolean operator!=(null_type other_value) const "
	"{ return !(*this == other_value); }\n"
	"inline boolean operator!=(const %s& other_value) const "
	"{ return !(*this == other_value); }\n", name);


    /* is_bound function */
    def = mputstr(def, "inline boolean is_bound() const "
	"{ return bound_flag; }\n");

    /* is_present function */
    def = mputstr(def,
      "inline boolean is_present() const { return is_bound(); }\n");

    /* is_value function */
    def = mputstr(def, "inline boolean is_value() const "
        "{ return bound_flag; }\n");

    /* clean_up function */
    def = mputstr(def, "inline void clean_up() "
        "{ bound_flag = FALSE; }\n");

    /* must_bound function */
    def = mputstr(def, "inline void must_bound(const char *err_msg) const "
	"{ if (!bound_flag) TTCN_error(\"%s\", err_msg); }\n");

    /* log function */
    def = mputstr(def, "void log() const;\n");
    src = mputprintf(src, "void %s::log() const\n"
	"{\n"
	"if (bound_flag) TTCN_Logger::log_event_str(\"{ }\");\n"
	"else TTCN_Logger::log_event_unbound();\n"
	"}\n\n", name);

    /* set_param function */
    def = mputstr(def, "void set_param(Module_Param& param);\n");
    src = mputprintf(src, "void %s::set_param(Module_Param& param)\n"
      "{\n"
      "  param.basic_check(Module_Param::BC_VALUE, \"empty record/set value (i.e. { })\");\n"
      "  if (param.get_type()!=Module_Param::MP_Value_List || param.get_size()>0) {\n"
      "    param.type_error(\"empty record/set value (i.e. { })\", \"%s\");\n"
      "  }\n"
      "  bound_flag = TRUE;\n"
      "}\n\n", name, dispname);

    /* encode_text function */
    def = mputstr(def, "void encode_text(Text_Buf& text_buf) const;\n");
    src = mputprintf(src, "void %s::encode_text(Text_Buf& /*text_buf*/) const\n"
	"{\n"
	"must_bound(\"Text encoder: Encoding an unbound value of type %s.\");\n"
	"}\n\n", name, dispname);

    /* decode_text function */
    def = mputstr(def, "void decode_text(Text_Buf& text_buf);\n");
    src = mputprintf(src, "void %s::decode_text(Text_Buf& /*text_buf*/)\n"
	"{\n"
	"bound_flag = TRUE;\n"
	"}\n\n", name);

    if(ber_needed || raw_needed || text_needed || xer_needed || json_needed)
      def_encdec(name, &def, &src, ber_needed, raw_needed,
                 text_needed, xer_needed, json_needed, FALSE);

  /* BER functions */
  if(ber_needed) {
    /* BER_encode_TLV() */
    src=mputprintf
      (src,
       "ASN_BER_TLV_t* %s::BER_encode_TLV(const TTCN_Typedescriptor_t& p_td,"
       " unsigned p_coding) const\n"
       "{\n"
       "  BER_chk_descr(p_td);\n"
       "  ASN_BER_TLV_t *new_tlv=ASN_BER_TLV_t::construct(NULL);\n"
       "  new_tlv=ASN_BER_V2TLV(new_tlv, p_td, p_coding);\n"
       "  return new_tlv;\n"
       "}\n"
       "\n"
       , name
       );

    /* BER_decode_TLV() */
    src=mputprintf
      (src,
       "boolean %s::BER_decode_TLV(const TTCN_Typedescriptor_t& p_td,"
       " const ASN_BER_TLV_t& p_tlv, unsigned L_form)\n"
       "{\n"
       "  BER_chk_descr(p_td);\n"
       "  ASN_BER_TLV_t stripped_tlv;\n"
       "  BER_decode_strip_tags(*p_td.ber, p_tlv, L_form, stripped_tlv);\n"
       "  TTCN_EncDec_ErrorContext ec_0(\"While decoding '%s' type: \");\n"
       "  stripped_tlv.chk_constructed_flag(TRUE);\n"
       "  bound_flag=TRUE;\n"
       "  return TRUE;\n"
       "}\n"
       "\n"
       , name, sdef->dispname
       );
  } /* if ber_needed */
  if(text_needed){
    src = mputprintf(src,
      "int %s::TEXT_encode(const TTCN_Typedescriptor_t& p_td,"
      "TTCN_Buffer& p_buf) const{\n"
      "  int encoded_length=0;\n"
      "  if(p_td.text->begin_encode){\n"
      "    p_buf.put_cs(*p_td.text->begin_encode);\n"
      "    encoded_length+=p_td.text->begin_encode->lengthof();\n"
      "  }\n"
      "  if(!bound_flag) {\n"
      "    TTCN_EncDec_ErrorContext::error\n"
      "      (TTCN_EncDec::ET_UNBOUND, \"Encoding an unbound value.\");\n"
      "  }\n"
      "  if(p_td.text->end_encode){\n"
      "    p_buf.put_cs(*p_td.text->end_encode);\n"
      "    encoded_length+=p_td.text->end_encode->lengthof();\n"
      "  }\n"
      "  return encoded_length;\n"
      "}\n"
      ,name
     );
    src = mputprintf(src,
      "int %s::TEXT_decode(const TTCN_Typedescriptor_t& p_td,"
      " TTCN_Buffer& p_buf, Limit_Token_List& limit, boolean no_err, boolean){\n"
      "  bound_flag = TRUE;\n"
      "  int decoded_length=0;\n"
      "  if(p_td.text->begin_decode){\n"
      "    int tl;\n"
      "    if((tl=p_td.text->begin_decode->match_begin(p_buf))<0){\n"
      "          if(no_err)return -1;\n"
      "          TTCN_EncDec_ErrorContext::error\n"
      "              (TTCN_EncDec::ET_TOKEN_ERR, \"The specified token '%%s'"
      " not found for '%%s': \",(const char*)*(p_td.text->begin_decode)"
      ", p_td.name);\n"
      "          return 0;\n"
      "        }\n"
      "    decoded_length+=tl;\n"
      "    p_buf.increase_pos(tl);\n"
      "  }\n"
      "  if(p_td.text->end_decode){\n"
      "    int tl;\n"
      "    if((tl=p_td.text->end_decode->match_begin(p_buf))<0){\n"
      "          if(no_err)return -1;\n"
      "          TTCN_EncDec_ErrorContext::error\n"
      "              (TTCN_EncDec::ET_TOKEN_ERR, \"The specified token '%%s'"
      " not found for '%%s': \",(const char*)*(p_td.text->end_decode)"
      ", p_td.name);\n"
      "          return 0;\n"
      "        }\n"
      "    decoded_length+=tl;\n"
      "    p_buf.increase_pos(tl);\n"
      "  }\n"
      "  return decoded_length;\n"
      "}\n"
      ,name
     );

  }
    /* RAW functions */
    if (raw_needed) {
	src = mputprintf(src,
	    "int %s::RAW_encode(const TTCN_Typedescriptor_t& p_td, "
		"RAW_enc_tree& /*myleaf*/) const\n"
	    "{\n"
	    "if (!bound_flag) TTCN_EncDec_ErrorContext::error"
		"(TTCN_EncDec::ET_UNBOUND, \"Encoding an unbound value of "
		"type %%s.\", p_td.name);\n"
	    "return 0;\n"
	    "}\n\n", name);

	src = mputprintf(src,
	    "int %s::RAW_decode(const TTCN_Typedescriptor_t& p_td, "
		"TTCN_Buffer& p_buf, int, raw_order_t, boolean, int, boolean)\n"
	    "{\n"
	    "bound_flag = TRUE;\n"
	    "return p_buf.increase_pos_padd(p_td.raw->prepadding) + "
		"p_buf.increase_pos_padd(p_td.raw->padding);\n"
	    "}\n\n", name);
    }

    if (xer_needed) { /* XERSTUFF codegen for empty record/SEQUENCE */
      src=mputprintf(src,
        "boolean %s::can_start(const char *p_name, const char *p_uri, "
        "const XERdescriptor_t& p_td, unsigned int p_flavor, unsigned int /*p_flavor2*/) {\n"
        "  boolean e_xer = is_exer(p_flavor);\n"
        "  if (e_xer && (p_td.xer_bits & UNTAGGED)) return FALSE;\n"
        "  else return check_name(p_name, p_td, e_xer) && (!e_xer || check_namespace(p_uri, p_td));\n"
        "}\n\n"
        , name

        );
      src = mputprintf(src,
        "int %s::XER_encode(const XERdescriptor_t& p_td, TTCN_Buffer& p_buf, "
        "unsigned int p_flavor, unsigned int, int p_indent, embed_values_enc_struct_t*) const{\n"
        "  int encoded_length=(int)p_buf.get_len();\n"
        "  int is_indented = !is_canonical(p_flavor);\n"
        "  boolean e_xer = is_exer(p_flavor);\n"
        "  if (is_indented) do_indent(p_buf, p_indent);\n"
        "  p_buf.put_c('<');\n"
        "  if (e_xer) write_ns_prefix(p_td, p_buf);\n"
        "  p_buf.put_s((size_t)p_td.namelens[e_xer]-2, (cbyte*)p_td.names[e_xer]);\n"
        "  p_buf.put_s(2 + is_indented, (cbyte*)\"/>\\n\");\n"
        "  return (int)p_buf.get_len() - encoded_length;\n"
        "}\n\n"
        , name);
      src = mputprintf(src,
#ifndef NDEBUG
        "// written by %s in " __FILE__ " at %d\n"
#endif
        "int %s::XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap& p_reader, "
        "unsigned int p_flavor, unsigned int /*p_flavor2*/, embed_values_dec_struct_t*)\n"
        "{\n"
        "  boolean e_xer = is_exer(p_flavor);\n"
        "  bound_flag = TRUE;\n"
        "  int rd_ok, depth=-1;\n"
        "  for (rd_ok=p_reader.Ok(); rd_ok==1; rd_ok=p_reader.Read()) {\n"
        "    int type = p_reader.NodeType();\n"
        "    if (type==XML_READER_TYPE_ELEMENT) {\n"
        "      verify_name(p_reader, p_td, e_xer);\n"
        "      depth=p_reader.Depth();\n"
        "      if (p_reader.IsEmptyElement()) {\n"
        "        rd_ok = p_reader.Read(); break;\n"
        "      }\n"
        "      else if ((p_flavor & XER_MASK) == XER_CANONICAL) {\n"
        "        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG, "
        "\"Expected an empty element tag\");\n"
        "      }\n"
        "    }\n"
        "    else if (type == XML_READER_TYPE_END_ELEMENT && depth != -1) {\n"
        "      verify_end(p_reader, p_td, depth, e_xer);\n"
        "      rd_ok = p_reader.Read(); break;\n"
        "    }\n"
        "  }\n"
        "  return 1;\n"
        "}\n\n"
#ifndef NDEBUG
        , __FUNCTION__, __LINE__
#endif
        , name);
    }
  if (json_needed) {
    // JSON encode, RT1
    src = mputprintf(src,
      "int %s::JSON_encode(const TTCN_Typedescriptor_t&, JSON_Tokenizer& p_tok) const\n"
      "{\n"
      "  if (!is_bound()) {\n"
      "    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,\n"
      "      \"Encoding an unbound value of type %s.\");\n"
      "    return -1;\n"
      "  }\n\n"
      "  return p_tok.put_next_token(JSON_TOKEN_OBJECT_START, NULL) + \n"
      "    p_tok.put_next_token(JSON_TOKEN_OBJECT_END, NULL);\n"
      "}\n\n"
      , name, dispname);
    
    // JSON decode, RT1
    src = mputprintf(src,
      "int %s::JSON_decode(const TTCN_Typedescriptor_t&, JSON_Tokenizer& p_tok, boolean p_silent)\n"
      "{\n"
      "  json_token_t token = JSON_TOKEN_NONE;\n"
      "  size_t dec_len = p_tok.get_next_token(&token, NULL, NULL);\n"
      "  if (JSON_TOKEN_ERROR == token) {\n"
      "    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_BAD_TOKEN_ERROR, \"\");\n"
      "    return JSON_ERROR_FATAL;\n"
      "  }\n"
      "  else if (JSON_TOKEN_OBJECT_START != token) {\n"
      "    return JSON_ERROR_INVALID_TOKEN;\n"
      "  }\n\n"
      "  dec_len += p_tok.get_next_token(&token, NULL, NULL);\n"
      "  if (JSON_TOKEN_OBJECT_END != token) {\n"
      "    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_STATIC_OBJECT_END_TOKEN_ERROR, \"\");\n"
      "    return JSON_ERROR_FATAL;\n"
      "  }\n\n"
      "  bound_flag = TRUE;\n\n"
      "  return (int)dec_len;\n"
      "}\n\n"
      , name);
  }

    /* closing class definition */
    def = mputstr(def, "};\n\n");

    output->header.class_defs = mputstr(output->header.class_defs, def);
    Free(def);

    output->source.methods = mputstr(output->source.methods, src);
    Free(src);

    output->header.function_prototypes =
	mputprintf(output->header.function_prototypes,
	"extern boolean operator==(null_type null_value, const %s& "
	    "other_value);\n", name);
    output->source.function_bodies =
	mputprintf(output->source.function_bodies,
	"boolean operator==(null_type, const %s& other_value)\n"
	"{\n"
	"other_value.must_bound(\"Comparison of an unbound value of type "
	"%s.\");\n"
	"return TRUE;\n"
	"}\n\n", name, dispname);

    output->header.function_prototypes =
	mputprintf(output->header.function_prototypes,
	"inline boolean operator!=(null_type null_value, const %s& "
	    "other_value) "
	"{ return !(null_value == other_value); }\n", name);
}

static void defEmptyRecordTemplate(const char *name, const char *dispname,
    output_struct *output)
{
    char *def = NULL, *src = NULL;

    /* class definition */
    def = mprintf("class %s_template : public Base_Template {\n"
	"struct {\n"
	"unsigned int n_values;\n"
	"%s_template *list_value;\n"
	"} value_list;\n", name, name);

    /* copy_template function */
    def = mputprintf(def,  "void copy_template(const %s_template& "
	"other_value);\n\n", name);
    src = mputprintf(src,
	"void %s_template::copy_template(const %s_template& other_value)\n"
	"{\n"
	"set_selection(other_value);\n"
	"switch (template_selection) {\n"
	"case OMIT_VALUE:\n"
	"case ANY_VALUE:\n"
	"case ANY_OR_OMIT:\n"
	"case SPECIFIC_VALUE:\n"
	"break;\n"
	"case VALUE_LIST:\n"
	"case COMPLEMENTED_LIST:\n"
	"value_list.n_values = other_value.value_list.n_values;\n"
	"value_list.list_value = new %s_template[value_list.n_values];\n"
	"for (unsigned int list_count = 0; list_count < value_list.n_values; "
	    "list_count++)\n"
	"value_list.list_value[list_count].copy_template("
	    "other_value.value_list.list_value[list_count]);\n"
	"break;\n"
	"default:\n"
	"TTCN_error(\"Copying an uninitialized/unsupported template of type "
	"%s.\");\n"
	"break;\n"
	"}\n"
	"}\n\n", name, name, name, dispname);

    /* default ctor */
    def = mputprintf(def, "public:\n"
	"%s_template();\n", name);
    src = mputprintf(src, "%s_template::%s_template()\n"
	"{\n"
	"}\n\n", name, name);

    /* ctor for generic wildcards */
    def = mputprintf(def, "%s_template(template_sel other_value);\n", name);
    src = mputprintf(src, "%s_template::%s_template(template_sel other_value)\n"
	" : Base_Template(other_value)\n"
	"{\n"
	"check_single_selection(other_value);\n"
	"}\n\n", name, name);

    /* ctor for value {} */
    def = mputprintf(def, "%s_template(null_type other_value);\n", name);
    src = mputprintf(src, "%s_template::%s_template(null_type)\n"
	" : Base_Template(SPECIFIC_VALUE)\n"
	"{\n"
	"}\n\n", name, name);

    /* ctor for specific value */
    def = mputprintf(def, "%s_template(const %s& other_value);\n", name, name);
    src = mputprintf(src, "%s_template::%s_template(const %s& other_value)\n"
	" : Base_Template(SPECIFIC_VALUE)\n"
	"{\n"
	"other_value.must_bound(\"Creating a template from an unbound value of "
	    "type %s.\");\n"
	"}\n\n", name, name, name, dispname);

    /* ctor for optional value */
    def = mputprintf(def, "%s_template(const OPTIONAL<%s>& other_value);\n",
	name, name);
    src = mputprintf(src, "%s_template::%s_template(const OPTIONAL<%s>& "
	"other_value)\n"
	"{\n"
	"switch (other_value.get_selection()) {\n"
	"case OPTIONAL_PRESENT:\n"
	"set_selection(SPECIFIC_VALUE);\n"
	"break;\n"
	"case OPTIONAL_OMIT:\n"
	"set_selection(OMIT_VALUE);\n"
	"break;\n"
	"default:\n"
	"TTCN_error(\"Creating a template of type %s from an unbound optional "
	    "field.\");\n"
	"}\n"
	"}\n\n", name, name, name, dispname);

    /* copy ctor */
    def = mputprintf(def, "%s_template(const %s_template& other_value);\n",
	name, name);
    src = mputprintf(src, "%s_template::%s_template(const %s_template& "
	"other_value)\n"
	": Base_Template()" /* yes, the base class _default_ constructor */
	"{\n"
	"copy_template(other_value);\n"
	"}\n\n", name, name, name);

    /* dtor */
    def = mputprintf(def, "~%s_template();\n", name);
    src = mputprintf(src, "%s_template::~%s_template()\n"
	"{\n"
	"clean_up();\n"
	"}\n\n", name, name);

    /* clean_up function */
    def = mputstr(def,  "void clean_up();\n");
    src = mputprintf(src, "void %s_template::clean_up()\n"
	"{\n"
	"if (template_selection == VALUE_LIST || "
	"template_selection == COMPLEMENTED_LIST)\n"
	"delete [] value_list.list_value;\n"
	"template_selection = UNINITIALIZED_TEMPLATE;\n"
	"}\n\n", name);

    /* assignment op for generic wildcards */
    def = mputprintf(def, "%s_template& operator=(template_sel other_value);\n",
	name);
    src = mputprintf(src, "%s_template& %s_template::operator=(template_sel "
	"other_value)\n"
	"{\n"
	"check_single_selection(other_value);\n"
	"clean_up();\n"
	"set_selection(other_value);\n"
	"return *this;\n"
	"}\n\n", name, name);

    /* assignment op for value {} */
    def = mputprintf(def, "%s_template& operator=(null_type other_value);\n",
	name);
    src = mputprintf(src, "%s_template& %s_template::operator=(null_type)\n"
	"{\n"
	"clean_up();\n"
	"set_selection(SPECIFIC_VALUE);\n"
	"return *this;\n"
	"}\n\n", name, name);

    /* assignment op for specific value */
    def = mputprintf(def, "%s_template& operator=(const %s& other_value);\n",
	name, name);
    src = mputprintf(src, "%s_template& %s_template::operator=(const %s& "
	"other_value)\n"
	"{\n"
	"other_value.must_bound(\"Assignment of an unbound value of type %s "
	    "to a template.\");\n"
	"clean_up();\n"
	"set_selection(SPECIFIC_VALUE);\n"
	"return *this;\n"
	"}\n\n", name, name, name, dispname);

    /* assignment op for optional value */
    def = mputprintf(def, "%s_template& operator=(const OPTIONAL<%s>& "
	"other_value);\n", name, name);
    src = mputprintf(src, "%s_template& %s_template::operator="
	"(const OPTIONAL<%s>& other_value)\n"
	"{\n"
	"clean_up();\n"
	"switch (other_value.get_selection()) {\n"
	"case OPTIONAL_PRESENT:\n"
	"set_selection(SPECIFIC_VALUE);\n"
	"break;\n"
	"case OPTIONAL_OMIT:\n"
	"set_selection(OMIT_VALUE);\n"
	"break;\n"
	"default:\n"
	"TTCN_error(\"Assignment of an unbound optional field to a template "
	  "of type %s.\");\n"
	"}\n"
	"return *this;\n"
	"}\n\n", name, name, name, dispname);

    /* assignment op for itself */
    def = mputprintf(def, "%s_template& operator=(const %s_template& "
	"other_value);\n", name, name);
    src = mputprintf(src, "%s_template& %s_template::operator="
	"(const %s_template& other_value)\n"
	"{\n"
	"if (&other_value != this) {\n"
	"clean_up();\n"
	"set_selection(other_value);\n"
	"}\n"
	"return *this;\n"
	"}\n\n", name, name, name);

    /* match operation with {} */
    def = mputstr(def, "boolean match(null_type other_value, boolean legacy "
      "= FALSE) const;\n");
    src = mputprintf(src, "boolean %s_template::match(null_type other_value,"
      "boolean) const\n"
	"{\n"
	"switch (template_selection) {\n"
	"case ANY_VALUE:\n"
	"case ANY_OR_OMIT:\n"
	"case SPECIFIC_VALUE:\n"
	"return TRUE;\n"
	"case OMIT_VALUE:\n"
	"return FALSE;\n"
	"case VALUE_LIST:\n"
	"case COMPLEMENTED_LIST:\n"
	"for (unsigned int list_count = 0; list_count < value_list.n_values; "
	    "list_count++)\n"
	"if (value_list.list_value[list_count].match(other_value)) "
	    "return template_selection == VALUE_LIST;\n"
	"return template_selection == COMPLEMENTED_LIST;\n"
	"default:\n"
	"TTCN_error(\"Matching an uninitialized/unsupported template of "
	    "type %s.\");\n"
	"}\n"
	"return FALSE;\n"
	"}\n\n", name, dispname);

    /* match operation with specific value */
    def = mputprintf(def, "boolean match(const %s& other_value, boolean legacy "
      "= FALSE) const;\n", name);
    src = mputprintf(src, "boolean %s_template::match(const %s& other_value, "
      "boolean) const\n"
	"{\n"
    "if (!other_value.is_bound()) return FALSE;"
	"return match(NULL_VALUE);\n"
	"}\n\n", name, name);

    /* valueof operation */
    def = mputprintf(def, "%s valueof() const;\n", name);
    src = mputprintf(src, "%s %s_template::valueof() const\n"
	"{\n"
	"if (template_selection != SPECIFIC_VALUE || is_ifpresent) "
            "TTCN_error(\"Performing a valueof or send operation on a "
            "non-specific template of type %s.\");\n"
	"return NULL_VALUE;\n"
	"}\n\n", name, name, dispname);

    /* void set_type(template_sel, int) function */
    def = mputstr(def,
	"void set_type(template_sel template_type, "
	    "unsigned int list_length);\n");

    src = mputprintf(src,
	"void %s_template::set_type(template_sel template_type, "
	    "unsigned int list_length)\n"
	"{\n"
	"if (template_type != VALUE_LIST "
	    "&& template_type != COMPLEMENTED_LIST)\n"
	"TTCN_error(\"Setting an invalid list for a template of type %s.\");\n"
	"clean_up();\n"
	"set_selection(template_type);\n"
	"value_list.n_values = list_length;\n"
	"value_list.list_value = new %s_template[list_length];\n"
	"}\n\n", name, dispname, name);

    /* list_item(int) function */

    def = mputprintf(def,
	"%s_template& list_item(unsigned int list_index) const;\n", name);

    src = mputprintf(src,
	"%s_template& %s_template::list_item(unsigned int list_index) const\n"
	"{\n"
	"if (template_selection != VALUE_LIST "
	    "&& template_selection != COMPLEMENTED_LIST)\n"
	"TTCN_error(\"Accessing a list element of a non-list template of "
	"type %s.\");\n"
	"if (list_index >= value_list.n_values)\n"
	"TTCN_error(\"Index overflow in a value list template of type "
	"%s.\");\n"
	"return value_list.list_value[list_index];\n"
	"}\n\n", name, name, dispname, dispname);

    /* log function */
    def = mputstr(def, "void log() const;\n");
    src = mputprintf(src, "void %s_template::log() const\n"
	"{\n"
	"switch (template_selection) {\n"
	"case SPECIFIC_VALUE:\n"
	"TTCN_Logger::log_event_str(\"{ }\");\n"
	"break;\n"
	"case COMPLEMENTED_LIST:\n"
	"TTCN_Logger::log_event_str(\"complement \");\n"
	"case VALUE_LIST:\n"
	"TTCN_Logger::log_char('(');\n"
	"for (unsigned int list_count = 0; list_count < value_list.n_values; "
	    "list_count++) {\n"
	"if (list_count > 0) TTCN_Logger::log_event_str(\", \");\n"
	"value_list.list_value[list_count].log();\n"
	"}\n"
	"TTCN_Logger::log_char(')');\n"
	"break;\n"
	"default:\n"
	"log_generic();\n"
	"}\n"
	"log_ifpresent();\n"
	"}\n\n", name);

    /* log_match function */
    def = mputprintf(def, "void log_match(const %s& match_value, "
      "boolean legacy = FALSE) const;\n", name);
    src = mputprintf(src, "void %s_template::log_match(const %s& match_value, "
      "boolean) const\n"
	"{\n"
	"match_value.log();\n"
	"TTCN_Logger::log_event_str(\" with \");\n"
	"log();\n"
	"if (match(match_value)) TTCN_Logger::log_event_str(\" matched\");\n"
	"else TTCN_Logger::log_event_str(\" unmatched\");\n"
	"}\n\n", name, name);

    /* encode_text function */
    def = mputstr(def, "void encode_text(Text_Buf& text_buf) const;\n");
    src = mputprintf(src, "void %s_template::encode_text(Text_Buf& text_buf) "
	    "const\n"
	"{\n"
	"encode_text_base(text_buf);\n"
	"switch (template_selection) {\n"
	"case OMIT_VALUE:\n"
	"case ANY_VALUE:\n"
	"case ANY_OR_OMIT:\n"
	"case SPECIFIC_VALUE:\n"
	"break;\n"
	"case VALUE_LIST:\n"
	"case COMPLEMENTED_LIST:\n"
	"text_buf.push_int(value_list.n_values);\n"
	"for (unsigned int list_count = 0; list_count < value_list.n_values; "
	    "list_count++)\n"
	"value_list.list_value[list_count].encode_text(text_buf);\n"
	"break;\n"
	"default:\n"
	"TTCN_error(\"Text encoder: Encoding an uninitialized/unsupported "
	    "template of type %s.\");\n"
	"}\n"
	"}\n\n", name, dispname);

    /* decode_text function */
    def = mputstr(def, "void decode_text(Text_Buf& text_buf);\n");
    src = mputprintf(src, "void %s_template::decode_text(Text_Buf& text_buf)\n"
	"{\n"
	"clean_up();\n"
	"decode_text_base(text_buf);\n"
	"switch (template_selection) {\n"
	"case OMIT_VALUE:\n"
	"case ANY_VALUE:\n"
	"case ANY_OR_OMIT:\n"
	"case SPECIFIC_VALUE:\n"
	"break;\n"
	"case VALUE_LIST:\n"
	"case COMPLEMENTED_LIST:\n"
	"value_list.n_values = text_buf.pull_int().get_val();\n"
	"value_list.list_value = new %s_template[value_list.n_values];\n"
	"for (unsigned int list_count = 0; list_count < value_list.n_values; "
	    "list_count++)\n"
	"value_list.list_value[list_count].decode_text(text_buf);\n"
	"break;\n"
	"default:\n"
	"TTCN_error(\"Text decoder: An unknown/unsupported selection was "
	    "received in a template of type %s.\");\n"
	"}\n"
	"}\n\n", name, name, dispname);

  /* set_param() */
  def = mputstr(def, "void set_param(Module_Param& param);\n");
  src = mputprintf(src,
    "void %s_template::set_param(Module_Param& param)\n"
    "{\n"
    "  param.basic_check(Module_Param::BC_TEMPLATE, \"empty record/set template\");\n"
    "  switch (param.get_type()) {\n"
    "  case Module_Param::MP_Omit:\n"
    "    *this = OMIT_VALUE;\n"
    "    break;\n"
    "  case Module_Param::MP_Any:\n"
    "    *this = ANY_VALUE;\n"
    "    break;\n"
    "  case Module_Param::MP_AnyOrNone:\n"
    "    *this = ANY_OR_OMIT;\n"
    "    break;\n"
    "  case Module_Param::MP_List_Template:\n"
    "  case Module_Param::MP_ComplementList_Template: {\n"
    "    %s_template temp;\n"
    "    temp.set_type(param.get_type()==Module_Param::MP_List_Template ? "
    "VALUE_LIST : COMPLEMENTED_LIST, param.get_size());\n"
    "    for (size_t p_i=0; p_i<param.get_size(); p_i++) {\n"
    "      temp.list_item(p_i).set_param(*param.get_elem(p_i));\n"
    "    }\n"
    "    *this = temp;\n"
    "    break; }\n"
    "  case Module_Param::MP_Value_List:\n"
    "    if (param.get_size()>0) param.type_error(\"empty record/set template\", \"%s\");\n"
    "    *this = NULL_VALUE;\n"
    "    break;\n"
    "  default:\n"
    "    param.type_error(\"empty record/set template\", \"%s\");\n"
    "  }\n"
    "  is_ifpresent = param.get_ifpresent();\n"
    "}\n\n", name, name, dispname, dispname);

    /* check template restriction */
    def = mputstr(def, "void check_restriction(template_res t_res, "
        "const char* t_name=NULL, boolean legacy = FALSE) const;\n");
    src = mputprintf(src,
        "void %s_template::check_restriction("
          "template_res t_res, const char* t_name, boolean legacy) const\n"
        "{\n"
        "if (template_selection==UNINITIALIZED_TEMPLATE) return;\n"
        "switch ((t_name&&(t_res==TR_VALUE))?TR_OMIT:t_res) {\n"
        "case TR_OMIT:\n"
        "if (template_selection==OMIT_VALUE) return;\n"
        "case TR_VALUE:\n"
        "if (template_selection!=SPECIFIC_VALUE || is_ifpresent) break;\n"
        "return;\n"
        "case TR_PRESENT:\n"
        "if (!match_omit(legacy)) return;\n"
        "break;\n"
        "default:\n"
        "return;\n"
        "}\n"
        "TTCN_error(\"Restriction `%%s' on template of type %%s "
          "violated.\", get_res_name(t_res), t_name ? t_name : \"%s\");\n"
        "}\n\n", name, dispname);

    defCommonRecordTemplate(name, &def, &src);

    def = mputstr(def, "};\n\n");

    output->header.class_defs = mputstr(output->header.class_defs, def);
    Free(def);

    output->source.methods = mputstr(output->source.methods, src);
    Free(src);
}

static void defCommonRecordTemplate(const char *name,
  char **def, char **src)
{
    /* TTCN-3 ispresent() function */
    *def = mputstr(*def, "boolean is_present(boolean legacy = FALSE) const;\n");
    *src = mputprintf(*src,
        "boolean %s_template::is_present(boolean legacy) const\n"
        "{\n"
        "if (template_selection==UNINITIALIZED_TEMPLATE) return FALSE;\n"
        "return !match_omit(legacy);\n"
        "}\n\n", name);

    /* match_omit() */
    *def = mputstr(*def, "boolean match_omit(boolean legacy = FALSE) const;\n");
    *src = mputprintf(*src,
        "boolean %s_template::match_omit(boolean legacy) const\n"
        "{\n"
        "if (is_ifpresent) return TRUE;\n"
        "switch (template_selection) {\n"
        "case OMIT_VALUE:\n"
        "case ANY_OR_OMIT:\n"
        "return TRUE;\n"
        "case VALUE_LIST:\n"
        "case COMPLEMENTED_LIST:\n"
        "if (legacy) {\n"
        "for (unsigned int l_idx=0; l_idx<value_list.n_values; l_idx++)\n"
        "if (value_list.list_value[l_idx].match_omit())\n"
        "return template_selection==VALUE_LIST;\n"
        "return template_selection==COMPLEMENTED_LIST;\n"
        "} // else fall through\n"
        "default:\n"
        "return FALSE;\n"
        "}\n"
        "return FALSE;\n"
        "}\n\n", name);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void defRecordClass2(const struct_def *sdef, output_struct *output)
{
  size_t i;
  size_t optional_num = 0;
  const char *name = sdef->name;
  char *def = NULL, *src = NULL;

  boolean xer_needed = sdef->hasXer && enable_xer();
  boolean raw_needed = sdef->hasRaw && enable_raw();
  boolean has_optional = FALSE;
  boolean has_default  = FALSE;

  const char* base_class = (sdef->nElements==0) ? "Empty_Record_Type" : "Record_Type";

  /* class declaration code */
  output->header.class_decls = mputprintf(output->header.class_decls,
                                          "class %s;\n", name);

  /* class definition and source code */
  def=mputprintf(def, "class %s : public %s {\n", name, base_class);

  /* data members */
  for (i = 0; i < sdef->nElements; i++) {
    if(sdef->elements[i].isOptional) {
      optional_num++;
      def = mputprintf(def, "  OPTIONAL<%s> field_%s;\n",
                       sdef->elements[i].type,
                       sdef->elements[i].name);
    } else {
      def = mputprintf(def, "  %s field_%s;\n",
                       sdef->elements[i].type, sdef->elements[i].name);
    }
  }
  if (sdef->nElements) {
    def = mputprintf(def, "  Base_Type* fld_vec[%lu];\n", (unsigned long)sdef->nElements);
  }

  /* default constructor */
  if (sdef->nElements==0) {
    def = mputprintf(def, "public:\n"
    "  %s();\n", name);
    src = mputprintf(src,
      "%s::%s() : Empty_Record_Type() {}\n\n",
      name, name);
  } else {
    def = mputstr(def, "  void init_vec();\n");
    src = mputprintf(src, "void %s::init_vec() { ", name);
    for (i = 0; i < sdef->nElements; i++) {
      src = mputprintf(src, "fld_vec[%lu]=&field_%s; ",
        (unsigned long)i, sdef->elements[i].name);
    }
    src = mputstr(src, " }\n\n");

    def = mputprintf(def, "public:\n"
        "  %s();\n", name);
    src = mputprintf(src,
        "%s::%s() : Record_Type() { init_vec(); }\n\n", name, name);
  }

  /* copy constructor */
  if (sdef->nElements) {
    def = mputprintf(def, "  %s(const %s& other_value);\n", name, name);
    src = mputprintf(src, "%s::%s(const %s& other_value) : Record_Type(other_value)\n", name, name, name);
    src = mputstr(src, "{\n"
      "  if(!other_value.is_bound()) "
      "TTCN_error(\"Copying an unbound record/set value.\");\n");
    for (i = 0; i < sdef->nElements; i++) {
      src = mputprintf(src,
          "if (other_value.field_%s.is_bound() )\n"
          "  field_%s = other_value.field_%s;\n",
          sdef->elements[i].name, sdef->elements[i].name,
          sdef->elements[i].name);
    }

    src = mputstr(src, "init_vec();\n"
                  "}\n\n");
  } else {
    def = mputprintf(def, "  %s(const %s& other_value): %s(other_value) {}\n", name, name, base_class);
  }

  if (sdef->nElements>0) { /* constructor by fields */
    def = mputprintf(def, "  %s(", name);
    src = mputprintf(src, "%s::%s(", name, name);
    for (i = 0; i < sdef->nElements; i++) {
      char *tmp = NULL;
      if (i > 0) tmp = mputstr(tmp, ",\n    ");
      if (sdef->elements[i].isOptional)
        tmp = mputprintf
          (tmp,
           "const OPTIONAL<%s>& par_%s",
           sdef->elements[i].type, sdef->elements[i].name);
      else
        tmp = mputprintf
          (tmp,
           "const %s& par_%s",
           sdef->elements[i].type, sdef->elements[i].name);
      def = mputstr(def, tmp);
      src = mputstr(src, tmp);
      Free(tmp);
    }
    def = mputstr(def, ");\n");
    src = mputprintf(src, ") : ");
    for (i = 0; i < sdef->nElements; i++) {
      if (i > 0) src = mputstr(src, ",\n    ");
      src = mputprintf(src, "field_%s(par_%s)", sdef->elements[i].name,
                       sdef->elements[i].name);
    }
    src = mputstr(src, "\n"
                  "{\n"
                  "init_vec();\n"
                  "}\n\n");
  } else { /* constructor from null */
    def = mputprintf(def, "  %s(null_type) {bound_flag = TRUE;}\n", name);
  }

  /* assignment operators */
  def = mputprintf(def, "inline %s& operator=(const %s& other_value) "
    "{ set_value(&other_value); return *this; }\n\n", name, name);
  if (sdef->nElements == 0) {
    def = mputprintf(def, "inline %s& operator=(null_type) "
      "{ bound_flag = TRUE; return *this; }\n", name);
  }

  /* == operator */
  def = mputprintf(def, "inline boolean operator==(const %s& other_value) "
                   "const { return is_equal(&other_value); }\n", name);
  /* != operator */
  def = mputprintf(def,
     "  inline boolean operator!=(const %s& other_value) const\n"
     "    { return !is_equal(&other_value); }\n\n", name);

  for (i = 0; i < sdef->nElements; i++) {
    if(sdef->elements[i].isOptional)
      def = mputprintf
        (def,
         "  inline OPTIONAL<%s>& %s()\n"
         "    {return field_%s;}\n"
         "  inline const OPTIONAL<%s>& %s() const\n"
         "    {return field_%s;}\n",
         sdef->elements[i].type, sdef->elements[i].name,
         sdef->elements[i].name,
         sdef->elements[i].type, sdef->elements[i].name,
         sdef->elements[i].name);
    else def = mputprintf
           (def,
            "  inline %s& %s()\n"
            "    {return field_%s;}\n"
            "  inline const %s& %s() const\n"
            "    {return field_%s;}\n",
	    sdef->elements[i].type, sdef->elements[i].name,
	    sdef->elements[i].name, sdef->elements[i].type,
	    sdef->elements[i].name, sdef->elements[i].name);

  }

  /* override virtual functions where needed */
  def = mputprintf(def,
    "Base_Type* clone() const { return new %s(*this); }\n"
    "const TTCN_Typedescriptor_t* get_descriptor() const;\n"
    "boolean is_set() const { return %s; }\n",
    name,
    (sdef->kind==SET) ? "TRUE" : "FALSE");
  src = mputprintf(src,
    "const TTCN_Typedescriptor_t* %s::get_descriptor() const { return &%s_descr_; }\n",
    name, name);

  if (sdef->nElements > 0) {

    /* field access functions */
    def = mputprintf(def,
      "Base_Type* get_at(int index_value) { return fld_vec[index_value]; }\n"
      "const Base_Type* get_at(int index_value) const { return fld_vec[index_value]; }\n\n"
      "int get_count() const { return %lu; }\n", (unsigned long)sdef->nElements);

    /* override if there are optional fields */
    if (optional_num) {
      def = mputprintf(def,
        "int optional_count() const { return %lu; }\n", (unsigned long)optional_num);
    }

    if (sdef->opentype_outermost)
      def = mputstr(def, "boolean is_opentype_outermost() const { return TRUE; }\n");

    /* FIXME: use static member in Record_Type and initialize somewhere */
    if (default_as_optional)
      def = mputstr(def, "boolean default_as_optional() const { return TRUE; }\n");

    for (i = 0; i < sdef->nElements; i++) {
      if (sdef->elements[i].isOptional) {
        has_optional = TRUE;
        break;
      }
    }

    for (i = 0; i < sdef->nElements; i++) {
      if (sdef->elements[i].isDefault) {
        has_default = TRUE;
        break;
      }
    }
    def = mputstr(def,
      "static const TTCN_Typedescriptor_t* fld_descriptors[];\n"
      "const TTCN_Typedescriptor_t* fld_descr(int p_index) const;\n\n"
      "static const char* fld_names[];\n"
      "const char* fld_name(int p_index) const;\n\n");

    src = mputprintf(src, "const TTCN_Typedescriptor_t* %s::fld_descriptors[] = ", name);
    for (i = 0; i < sdef->nElements; i++) {
      src = mputprintf(src, "%c &%s_descr_",
                       (i ? ',' : '{'), sdef->elements[i].typedescrname);
    }
    src = mputstr(src, " };\n");
    src = mputprintf(src,
      "const TTCN_Typedescriptor_t* %s::fld_descr(int p_index) const "
      "{ return fld_descriptors[p_index]; }\n\n", name);

    src = mputprintf(src, "const char* %s::fld_names[] = ", name);
    for (i = 0; i < sdef->nElements; i++) {
      src = mputprintf(src, "%c \"%s\"",
                       (i ? ',' : '{'), sdef->elements[i].dispname);
    }
    src = mputstr(src, " };\n");

    src = mputprintf(src,
      "const char* %s::fld_name(int p_index) const "
      "{ return fld_names[p_index]; }\n\n", name);

    if (has_optional) {
      def = mputstr(def,
        "static const int optional_indexes[];\n"
        "const int* get_optional_indexes() const;\n\n");
      src = mputprintf(src, "const int %s::optional_indexes[] = { ", name);
      for (i = 0; i < sdef->nElements; i++) {
        if (sdef->elements[i].isOptional) {
          src = mputprintf(src, "%lu, ", (unsigned long)i);
        }
      }
      src = mputstr(src, "-1 };\n");
      src = mputprintf(src,
        "const int* %s::get_optional_indexes() const "
        "{ return optional_indexes; }\n\n", name);
    }

    if (has_default) {
      def = mputstr(def,
        "static const default_struct default_indexes[];\n"
        "const default_struct* get_default_indexes() const;\n");
      src = mputprintf(src, "const Record_Type::default_struct %s::default_indexes[] = { ", name);
      for (i = 0; i < sdef->nElements; i++) {
        if (sdef->elements[i].isDefault) {
          src = mputprintf(src, "{%lu,&%s}, ",
            (unsigned long)i, sdef->elements[i].defvalname);
        }
      }
      src = mputstr(src, "{-1,NULL} };\n");
      src = mputprintf(src,
        "const Record_Type::default_struct* %s::get_default_indexes() const "
          "{ return default_indexes; }\n", name);
    }

    if (raw_needed) {
      struct raw_option_struct *raw_options;
      boolean haslengthto, haspointer, hascrosstag, has_ext_bit;
      boolean generate_raw_function = FALSE;

      raw_options = (struct raw_option_struct*)
                    Malloc(sdef->nElements * sizeof(*raw_options));

      set_raw_options(sdef, raw_options, &haslengthto,
                      &haspointer, &hascrosstag, &has_ext_bit);

      /* set the value of generate_raw_function: if the coding/decoding is too
         complex for this type then generate the functions, otherwise generate
         helper functions only which are called by the default RAW enc/dec
         functions */
      if (haslengthto || haspointer || hascrosstag) {
        generate_raw_function = TRUE;
        goto check_generate_end;
      }
      for (i = 0; i < sdef->nElements; i++) {
        if (raw_options[i].lengthto || raw_options[i].lengthof ||
            raw_options[i].lengthoffield || raw_options[i].pointerto ||
            raw_options[i].pointerof || raw_options[i].ptrbase ||
            raw_options[i].extbitgroup || raw_options[i].tag_type ||
            raw_options[i].delayed_decode || raw_options[i].nof_dependent_fields
            || raw_options[i].dependent_fields) {
          generate_raw_function = TRUE;
          goto check_generate_end;
        }
      }
      for (i = 0; i < sdef->nElements; i++) {
        if (raw_options[i].lengthof || raw_options[i].lengthoffield ||
            raw_options[i].nof_dependent_fields ||
            raw_options[i].dependent_fields) {
          generate_raw_function = TRUE;
          goto check_generate_end;
        }
      }
      if (sdef->raw.ext_bit_goup_num || sdef->raw.ext_bit_groups ||
          sdef->raw.lengthto || sdef->raw.lengthindex ||
          sdef->raw.taglist.nElements || sdef->raw.crosstaglist.nElements ||
          sdef->raw.presence.fieldnum || sdef->raw.presence.fields ||
          sdef->raw.member_name) {
        generate_raw_function = TRUE;
        goto check_generate_end;
      }
      for (i = 0; i < sdef->nElements; i++) {
        if (sdef->elements[i].hasRaw) {
          generate_raw_function = TRUE;
          goto check_generate_end;
        }
      }
check_generate_end:

      if (generate_raw_function) {
        def = mputprintf(def,
         "int RAW_encode(const TTCN_Typedescriptor_t&, RAW_enc_tree&) const;\n"
         "virtual int RAW_encode_negtest(const Erroneous_descriptor_t *, const TTCN_Typedescriptor_t&, RAW_enc_tree&) const;\n"
         "int RAW_decode(const TTCN_Typedescriptor_t&, TTCN_Buffer&, "
         "int, raw_order_t, boolean no_err = FALSE, "
         "int sel_field = -1, boolean first_call = TRUE);\n");
        src = generate_raw_coding(src, sdef, raw_options, haspointer,
                                  hascrosstag, has_ext_bit);
      } else { /* generate helper functions for the default RAW enc/dec */
        if (has_ext_bit)
          def = mputstr(def,"boolean raw_has_ext_bit() const { return TRUE; }\n");
      }
      for (i = 0; i < sdef->nElements; i++) {
        Free(raw_options[i].lengthoffield);
        Free(raw_options[i].dependent_fields);
      }
      Free(raw_options);
    } /* if (raw_needed) */

    if (xer_needed) { /* XERSTUFF codegen for record/SEQUENCE in RT2 */
      size_t num_attributes = 0;

      /* XER descriptors needed because of the xer antipattern */
      def = mputstr(def,
        "static const XERdescriptor_t* xer_descriptors[];\n"
        "const XERdescriptor_t* xer_descr(int p_index) const;\n"
        "virtual boolean can_start_v(const char *name, const char *prefix, "
        "XERdescriptor_t const& xd, unsigned int flavor, unsigned int flavor2);\n"
        "static  boolean can_start  (const char *name, const char *prefix, "
        "XERdescriptor_t const& xd, unsigned int flavor, unsigned int flavor2);\n");
      src = mputprintf(src, "const XERdescriptor_t* %s::xer_descriptors[] = ", name);
      for (i = 0; i < sdef->nElements; i++) {
        src = mputprintf(src, "%c &%s_xer_",
                         (i ? ',' : '{'), sdef->elements[i].typegen);
      }
      src = mputstr(src, " };\n");
      src = mputprintf(src,
        "const XERdescriptor_t* %s::xer_descr(int p_index) const "
        "{ return xer_descriptors[p_index]; }\n"
        /* The virtual can_start_v hands off to the static can_start.
         * We must make a virtual call in Record_Type::XER_decode because
         * we don't know the actual type (derived from Record_Type) */
        "boolean %s::can_start_v(const char *p_name, const char *p_uri, "
        "XERdescriptor_t const& p_td, unsigned int p_flavor, unsigned int p_flavor2)\n"
        "{ return can_start(p_name, p_uri, p_td, p_flavor, p_flavor2); }\n"
        "boolean %s::can_start(const char *p_name, const char *p_uri, "
        "XERdescriptor_t const& p_td, unsigned int p_flavor, unsigned int p_flavor2) {\n"
        "  boolean e_xer = is_exer(p_flavor);\n"
        "  if (!e_xer || (!(p_td.xer_bits & UNTAGGED) && !(p_flavor & USE_NIL))) return check_name(p_name, p_td, e_xer) && (!e_xer || check_namespace(p_uri, p_td));\n"
        , name , name, name);
      for (i = 0; i < sdef->nElements; i++) {
        src = mputprintf(src,
          "  else if (%s::can_start(p_name, p_uri, %s_xer_, p_flavor, p_flavor2)) return TRUE;\n"
          , sdef->elements[i].type, sdef->elements[i].typegen);
      }
      src = mputstr(src,
        "  return FALSE;\n"
        "}\n\n");
      /* end of antipattern */

      /* calculate num_attributes in compile time */
      for ( i=0; i < sdef->nElements; ++i ) {
        if (sdef->elements[i].xerAttribute
          ||(sdef->elements[i].xerAnyKind & ANY_ATTRIB_BIT)) ++num_attributes;
        else if (num_attributes) break;
      }

      /* generate helper virtual functions for XER encdec */
      if (num_attributes) {
        def = mputprintf(def,
          "int get_xer_num_attr() const { return %lu; }\n",
          (unsigned long)num_attributes);
      }
    }
    else { /* XER not needed */
      def = mputstr(def,
        "boolean can_start_v(const char *, const char *, XERdescriptor_t const&, unsigned int, unsigned int)\n"
        "{ return FALSE; }\n"
        );
    } /* if (xer_needed) */
  } /* if (sdef->nElements > 0) */

  /* end of class definition */
  def = mputstr(def, "};\n\n");

  output->header.class_defs = mputstr(output->header.class_defs, def);
  Free(def);

  output->source.methods = mputstr(output->source.methods, src);
  Free(src);
}

void defRecordTemplate2(const struct_def *sdef, output_struct *output)
{
    int i;
    const char *name = sdef->name;
    char *def = NULL, *src = NULL;

    const char* base_class = (sdef->nElements==0) ? "Empty_Record_Template" : "Record_Template";

    /* class declaration */
    output->header.class_decls = mputprintf(output->header.class_decls,
	"class %s_template;\n", name);

    /* template class definition */
    def = mputprintf(def, "class %s_template : public %s {\n", name, base_class);

    if (sdef->nElements>0) {
      /* set_specific function (used in field access members) */
      def = mputstr(def, "void set_specific();\n");
      src = mputprintf(src, "void %s_template::set_specific()\n"
        "{\n"
        "if (template_selection != SPECIFIC_VALUE) {\n"
        "%s"
        "clean_up();\n"
        "single_value.n_elements = %lu;\n"
        "single_value.value_elements = (Base_Template**)allocate_pointers(single_value.n_elements);\n"
        "set_selection(SPECIFIC_VALUE);\n",
        name, sdef->nElements ? "boolean was_any = (template_selection == ANY_VALUE || template_selection == ANY_OR_OMIT);\n" : "",
        (unsigned long)sdef->nElements);
      for (i = 0; i < sdef->nElements; i++) {
      src = mputprintf(src,
        "single_value.value_elements[%d] = was_any ? new %s_template(%s) : new %s_template;\n",
        i, sdef->elements[i].type,
        sdef->elements[i].isOptional ? "ANY_OR_OMIT" : "ANY_VALUE",
        sdef->elements[i].type);
      }
      src = mputstr(src,
      "}\n"
      "}\n\n");
    }

    /* default constructor */
    def = mputprintf(def, "public:\n"
	"%s_template(): %s() {}\n", name, base_class);

    if (sdef->nElements==0) {
      /* ctor for value {} */
      def = mputprintf(def, "%s_template(null_type) : "
        "Empty_Record_Template() { set_selection(SPECIFIC_VALUE); }\n",
        name);
    }

    /* constructor t1_template(template_sel other_value) */
    def = mputprintf(def, "%s_template(template_sel other_value): "
                          "%s(other_value) {}\n", name, base_class);

    /* constructor t1_template(const t1& other_value) */
    def = mputprintf(def, "%s_template(const %s& other_value): "
                          "%s() { copy_value(&other_value); }\n",
                          name, name, base_class);

    /* constructor t1_template(const OPTIONAL<t1>& other_value) */
    def = mputprintf(def, "%s_template(const OPTIONAL<%s>& other_value): "
                          "%s() { copy_optional(&other_value); }\n",
                          name, name, base_class);

    /* copy constructor */
    def = mputprintf(def, "%s_template(const %s_template& other_value): %s() "
      "{ copy_template(other_value); }\n", name, name, base_class);

    /* assignment operator <- template_sel */
    def = mputprintf(def, "%s_template& operator=(template_sel other_value);\n",
	name);
    src = mputprintf(src,
	"%s_template& %s_template::operator=(template_sel other_value)\n"
	"{\n"
	"check_single_selection(other_value);\n"
	"clean_up();\n"
	"set_selection(other_value);\n"
	"return *this;\n"
	"}\n\n", name, name);

    /* assignment operator <- value */
    def = mputprintf(def, "%s_template& operator=(const %s& other_value);\n",
	name, name);
    src = mputprintf(src,
	"%s_template& %s_template::operator=(const %s& other_value)\n"
	"{\n"
	"clean_up();\n"
	"copy_value(&other_value);\n"
	"return *this;\n"
	"}\n\n", name, name, name);

    /* assignment operator <- optional value */
    def = mputprintf(def, "%s_template& operator=(const OPTIONAL<%s>& "
	"other_value);\n", name, name);
    src = mputprintf(src,
	"%s_template& %s_template::operator=(const OPTIONAL<%s>& other_value)\n"
	"{\n"
	"clean_up();\n"
	"copy_optional(&other_value);\n"
	"return *this;\n"
	"}\n\n", name, name, name);

    /* assignment operator <- template*/
    def = mputprintf(def,
	"%s_template& operator=(const %s_template& other_value);\n",
	name, name);
    src = mputprintf(src,
	"%s_template& %s_template::operator=(const %s_template& other_value)\n"
	"{\n"
	"if (&other_value != this) {\n"
	"clean_up();\n"
	"copy_template(other_value);\n"
	"}\n"
	"return *this;\n"
	"}\n\n", name, name, name);

    if (sdef->nElements==0) {
      /* assignment op for value {} */
      def = mputprintf(def, "%s_template& operator=(null_type other_value);\n",
      name);
      src = mputprintf(src, "%s_template& %s_template::operator=(null_type)\n"
      "{\n"
      "clean_up();\n"
      "set_selection(SPECIFIC_VALUE);\n"
      "return *this;\n"
      "}\n\n", name, name);
    }

    /* match operation (template matching) */
    def = mputprintf(def, "inline boolean match(const %s& other_value, "
      "boolean legacy = FALSE) const "
      "{ return matchv(&other_value, legacy); }\n", name);

    /* log_match */
    def = mputprintf(def, "inline void log_match(const %s& match_value, "
      "boolean legacy = FALSE) const "
      "{ log_matchv(&match_value, legacy); }\n", name);

    /* valueof operation */
    def = mputprintf(def, "%s valueof() const;\n", name);

    src = mputprintf(src, "%s %s_template::valueof() const\n"
	"{\n"
	"%s ret_val;\n"
    "valueofv(&ret_val);\n"
    "return ret_val;\n"
    "}\n\n", name, name, name);

    /* list_item(int) function */

    def = mputprintf(def,
	"inline %s_template& list_item(int list_index) const "
      "{ return *(static_cast<%s_template*>(get_list_item(list_index))); }\n", name, name);

    if (sdef->nElements>0) {
      /* template field access functions (non-const & const) */
      for (i = 0; i < sdef->nElements; i++) {
        def = mputprintf(def,
          "%s_template& %s();\n"
          "const %s_template& %s() const;\n",
          sdef->elements[i].type, sdef->elements[i].name,
          sdef->elements[i].type, sdef->elements[i].name);
        src = mputprintf(src,
          "%s_template& %s_template::%s() { return *(static_cast<%s_template*>(get_at(%d))); }\n"
          "const %s_template& %s_template::%s() const { return *(static_cast<const %s_template*>(get_at(%d))); }\n",
          sdef->elements[i].type, name, sdef->elements[i].name, sdef->elements[i].type, i,
          sdef->elements[i].type, name, sdef->elements[i].name, sdef->elements[i].type, i);
      }
    }

    /* virtual functions */
    def = mputprintf(def,
      "%s* create() const { return new %s_template; }\n"
      "const TTCN_Typedescriptor_t* get_descriptor() const;\n",
      base_class, name);
    src = mputprintf(src,
    "const TTCN_Typedescriptor_t* %s_template::get_descriptor() const { return &%s_descr_; }\n",
    name, name);

    if (sdef->nElements>0) {
      def = mputprintf(def, "const char* fld_name(int p_index) const;\n");
      src = mputprintf(src,
        "const char* %s_template::fld_name(int p_index) const { return %s::fld_names[p_index]; }\n",
        name, name);
    }

    def = mputstr(def, "};\n\n");

    output->header.class_defs = mputstr(output->header.class_defs, def);
    Free(def);

    output->source.methods = mputstr(output->source.methods, src);
    Free(src);
}

