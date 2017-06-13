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
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *
 ******************************************************************************/
#include "../../common/memory.h"
#include "RawAST.hh"
#include <stdio.h>
#include "../main.hh"

RawAST::RawAST(){
    init_rawast(false);
}

RawAST::RawAST(bool int_type){
    init_rawast(int_type);
}

RawAST::RawAST(RawAST *other,bool int_type){
    if(other){
    fieldlength=other->fieldlength;
    comp=other->comp;
    byteorder=other->byteorder;
    align=other->align;
    bitorderinfield=other->bitorderinfield;
    bitorderinoctet=other->bitorderinoctet;
    extension_bit=other->extension_bit;
    hexorder=other->hexorder;
    padding=other->padding;
    prepadding=other->prepadding;
    padding_pattern_length=other->padding_pattern_length;
    if (other->padding_pattern != NULL)
      padding_pattern = mcopystr(other->padding_pattern);
    else padding_pattern = NULL;
    fieldorder=other->fieldorder;
    paddall=XDEFDEFAULT;
    repeatable=other->repeatable;
    ext_bit_goup_num=0;
    ext_bit_groups=NULL;
    lengthto_num=0;
    lengthto=NULL;
    pointerto=NULL;
    ptrbase=NULL;
    ptroffset=other->ptroffset;
    unit=other->unit;
    lengthindex=NULL;
    taglist.nElements=0;
    taglist.tag=NULL;
    crosstaglist.nElements=0;
    crosstaglist.tag=NULL;
    presence.fieldName=NULL;
    presence.nElements=0;
    presence.keyList=NULL;
    topleveleind=other->topleveleind;
    toplevel.bitorder=other->toplevel.bitorder;
    length_restrition=other->length_restrition;
    intx = other->intx;
    stringformat = other->stringformat;
    }
    else init_rawast(int_type);
}

void RawAST::init_rawast(bool int_type){
    fieldlength=int_type?8:0;
    comp=XDEFDEFAULT;
    byteorder=XDEFDEFAULT;
    align=XDEFDEFAULT;
    bitorderinfield=XDEFDEFAULT;
    bitorderinoctet=XDEFDEFAULT;
    extension_bit=XDEFDEFAULT;
    ext_bit_goup_num=0;
    ext_bit_groups=NULL;
    hexorder=XDEFDEFAULT;
    repeatable=XDEFDEFAULT;
    paddall=XDEFDEFAULT;
    padding=0;
    prepadding=0;
    padding_pattern_length=0;
    padding_pattern=NULL;
    fieldorder=XDEFDEFAULT;
    lengthto_num=0;
    lengthto=NULL;
    pointerto=NULL;
    ptrbase=NULL;
    ptroffset=0;
    unit=8;
    lengthindex=NULL;
    length_restrition=-1;
    taglist.nElements=0;
    taglist.tag=NULL;
    crosstaglist.nElements=0;
    crosstaglist.tag=NULL;
    presence.fieldName=NULL;
    presence.nElements=0;
    presence.keyList=NULL;
    topleveleind=0;
    intx = false;
    stringformat = CharCoding::UNKNOWN;
}

RawAST::~RawAST(){
    if(lengthto_num){
        for(int a=0;a<lengthto_num;a++){delete lengthto[a];}
        Free(lengthto);
    }
    if(pointerto!=NULL){delete pointerto;}
    if(ptrbase!=NULL){delete ptrbase;}
    if(padding_pattern!=NULL){Free(padding_pattern);}
    free_rawAST_tag_list(&taglist);
    free_rawAST_tag_list(&crosstaglist);
    free_rawAST_single_tag(&presence);
    if(lengthindex!=NULL) {
        for(int a=0;a<lengthindex->nElements;a++) delete lengthindex->names[a];
	Free(lengthindex->names);
        Free(lengthindex);
    }
    if(ext_bit_goup_num){
        for(int a=0;a<ext_bit_goup_num;a++){
          delete ext_bit_groups[a].from;
          delete ext_bit_groups[a].to;
        }
        Free(ext_bit_groups);
    }

}

void RawAST::print_RawAST(){
    printf("Fieldlength: %d\n\r",fieldlength);
    printf("comp: %d\n\r",comp);
    printf("byteorder: %d\n\r",byteorder);
    printf("align: %d\n\r",align);
    printf("bitorderinfield: %d\n\r",bitorderinfield);
    printf("bitorderinoctet: %d\n\r",bitorderinoctet);
    printf("extension_bit: %d\n\r",extension_bit);
    printf("hexorder: %d\n\r",hexorder);
    printf("fieldorder: %d\n\r",fieldorder);
    printf("ptroffset: %d\n\r",ptroffset);
    printf("unit: %d\n\r",unit);
    printf("repeatable: %d\n\r",repeatable);
    printf("presence:\n\r");
    printf("  nElements:%d \n\r",presence.nElements);
    for(int a=0;a<presence.nElements;a++){
      printf("  Element%d:\n\r",a);
      printf("    value:%s\n\r",presence.keyList[a].value);
      printf("    field:");
      for(int b=0;b<presence.keyList[a].keyField->nElements;b++){
       printf("%s.",presence.keyList[a].keyField->names[b]->get_name().c_str());
      }
      printf("\n\r");
    }
    printf("crosstag:\n\r");
    printf("  nElements:%d \n\r",crosstaglist.nElements);
    for(int a=0;a<crosstaglist.nElements;a++){
      printf("  Element%d:\n\r",a);
      printf("    fieldname:%s\n\r",crosstaglist.tag[a].fieldName
                                                          ->get_name().c_str());
      printf("    nElements:%d\n\r",crosstaglist.tag[a].nElements);
      for(int c=0;c<crosstaglist.tag[a].nElements;c++){
        printf("    Element%d:\n\r",c);
        printf("      value:%s\n\r",crosstaglist.tag[a].keyList[c].value);
        printf("      field:");
        for(int b=0;b<crosstaglist.tag[a].keyList[c].keyField->nElements;b++){
          printf("%s.",crosstaglist.tag[a].keyList[c].keyField->names[b]
                                                          ->get_name().c_str());
        }
        printf("\n\r");
      }
    }
    printf("Tag:\n\r");
    printf("  nElements:%d \n\r",taglist.nElements);
    for(int a=0;a<taglist.nElements;a++){
      printf("  Element%d:\n\r",a);
      printf("    fieldname:%s\n\r",taglist.tag[a].fieldName
                                                          ->get_name().c_str());
      printf("    nElements:%d\n\r",taglist.tag[a].nElements);
      for(int c=0;c<taglist.tag[a].nElements;c++){
        printf("    Element%d:\n\r",c);
        printf("      value:%s\n\r",taglist.tag[a].keyList[c].value);
        printf("      field:");
        for(int b=0;b<taglist.tag[a].keyList[c].keyField->nElements;b++){
          printf("%s.",taglist.tag[a].keyList[c].keyField->names[b]
                                                          ->get_name().c_str());
        }
        printf("\n\r");
      }
    }
    printf("%sIntX encoding\n\r", intx ? "" : "not ");
    printf("String format: %s\n\r", stringformat == CharCoding::UTF_8 ? "UTF-8" :
      (stringformat == CharCoding::UTF16 ? "UTF-16" : "unknown"));
}

void copy_rawAST_to_struct(RawAST *from, raw_attrib_struct *to){
    to->fieldlength=from->fieldlength;
    to->comp=from->comp;
    to->byteorder=from->byteorder;
    to->align=from->align;
    to->bitorderinfield=from->bitorderinfield;
    to->bitorderinoctet=from->bitorderinoctet;
    to->extension_bit=from->extension_bit;
    to->ext_bit_goup_num=from->ext_bit_goup_num;
    if (from->ext_bit_goup_num > 0)
      to->ext_bit_groups = static_cast<rawAST_coding_ext_group*>(
        Malloc(from->ext_bit_goup_num * sizeof(*to->ext_bit_groups)) );
    else to->ext_bit_groups = NULL;
    to->hexorder=from->hexorder;
    to->padding=from->padding;
    to->lengthto_num=from->lengthto_num;
    if (from->lengthto_num > 0)
      to->lengthto = static_cast<int*>( Malloc(from->lengthto_num * sizeof(int)) );
    else to->lengthto = NULL;
    to->pointerto=-1;
    to->ptroffset=from->ptroffset;
    to->unit=from->unit;
    if (from->lengthindex != NULL)
      to->lengthindex = static_cast<rawAST_coding_fields*>(
        Malloc(sizeof(rawAST_coding_fields)) );
    else to->lengthindex = NULL;
    to->crosstaglist.nElements = from->crosstaglist.nElements;
    if (to->crosstaglist.nElements > 0) {
      to->crosstaglist.list = static_cast<rawAST_coding_taglist*>(
         Malloc(to->crosstaglist.nElements * sizeof(rawAST_coding_taglist)) );
      for (int i = 0; i < to->crosstaglist.nElements; i++) {
	to->crosstaglist.list[i].nElements = 0;
	to->crosstaglist.list[i].fields = NULL;
      }
    } else to->crosstaglist.list = NULL;
    to->taglist.nElements = from->taglist.nElements;
    if (to->taglist.nElements > 0) {
      to->taglist.list = static_cast<rawAST_coding_taglist*>(
         Malloc(to->taglist.nElements * sizeof(rawAST_coding_taglist)) );
      for (int i = 0; i < to->taglist.nElements; i++) {
	to->taglist.list[i].nElements = 0;
	to->taglist.list[i].fields = NULL;
      }
    } else to->taglist.list = NULL;
    to->presence.nElements = from->presence.nElements;
    if (to->presence.nElements > 0)
      to->presence.fields = static_cast<rawAST_coding_field_list*>(
         Malloc(to->presence.nElements * sizeof(rawAST_coding_field_list)) );
    else to->presence.fields = NULL;
    to->topleveleind=from->topleveleind;
    to->toplevel.bitorder=from->toplevel.bitorder;
    to->union_member_num=0;
    to->member_name=NULL;
    to->repeatable=from->repeatable;
    to->length = -1;
}

void free_raw_attrib_struct(raw_attrib_struct *raw)
{
  // extension bit groups
  Free(raw->ext_bit_groups);
  // lengthto
  Free(raw->lengthto);
  // lengthindex
  Free(raw->lengthindex);
  // tag
  for (int i = 0; i < raw->taglist.nElements; i++) {
    for (int j = 0; j < raw->taglist.list[i].nElements; j++)
      Free(raw->taglist.list[i].fields[j].fields);
    Free(raw->taglist.list[i].fields);
  }
  Free(raw->taglist.list);
  // crosstag
  for (int i = 0; i < raw->crosstaglist.nElements; i++) {
    for (int j = 0; j < raw->crosstaglist.list[i].nElements; j++)
      Free(raw->crosstaglist.list[i].fields[j].fields);
    Free(raw->crosstaglist.list[i].fields);
  }
  Free(raw->crosstaglist.list);
  // presence
  for (int i = 0; i < raw->presence.nElements; i++)
    Free(raw->presence.fields[i].fields);
  Free(raw->presence.fields);
  // member name
  Free(raw->member_name);
}

int compare_raw_attrib(RawAST *a, RawAST *b){
    if(a==NULL) return 0;
    if(a==b) return 0;
    if(b==NULL) return 1;
    return a->fieldlength!=b->fieldlength ||
           a->comp!=b->comp ||
           a->byteorder!=b->byteorder ||
           a->align!=b->align ||
           a->bitorderinfield!=b->bitorderinfield ||
           a->bitorderinoctet!=b->bitorderinoctet ||
           a->extension_bit!=b->extension_bit ||
           a->hexorder!=b->hexorder ||
           a->fieldorder!=b->fieldorder ||
           a->topleveleind!=b->topleveleind ||
           (a->topleveleind && a->toplevel.bitorder!=b->toplevel.bitorder) ||
           a->padding!=b->padding ||
           a->ptroffset!=b->ptroffset ||
           a->repeatable!=b->repeatable ||
           a->unit!=b->unit ||
           a->intx != b->intx;
}
