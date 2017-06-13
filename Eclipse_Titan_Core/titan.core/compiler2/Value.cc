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
 *   Bibo, Zoltan
 *   Cserveni, Akos
 *   Delic, Adam
 *   Dimitrov, Peter
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Gecse, Roland
 *   Kovacs, Ferenc
 *   Ormandi, Matyas
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *   Tatarka, Gabor
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#include "../common/dbgnew.hh"
#include "Value.hh"
#include "Identifier.hh"
#include "Valuestuff.hh"
#include "PredefFunc.hh"
#include "CompField.hh"
#include "CompType.hh"
#include "EnumItem.hh"
#include "TypeCompat.hh"
#include "asn1/Block.hh"
#include "asn1/TokenBuf.hh"
#include "Real.hh"
#include "Int.hh"
#include "main.hh"
#include "Setting.hh"
#include "Type.hh"
#include "ttcn3/TtcnTemplate.hh"
#include "ttcn3/ArrayDimensions.hh"
#include "ustring.hh"
#include "../common/pattern.hh"

#include "ttcn3/PatternString.hh"
#include "ttcn3/Statement.hh"

#include "ttcn3/Attributes.hh"
#include "../common/JSON_Tokenizer.hh"
#include "ttcn3/Ttcn2Json.hh"

#include <regex.h>
#include <limits.h>

namespace Common {

  static void clean_up_string_elements(map<size_t, Value>*& string_elements)
  {
    if (string_elements) {
      for (size_t i = 0; i < string_elements->size(); i++)
	delete string_elements->get_nth_elem(i);
      string_elements->clear();
      delete string_elements;
      string_elements = 0;
    }
  }

  // =================================
  // ===== Value
  // =================================

  Value::Value(const Value& p)
    : GovernedSimple(p), valuetype(p.valuetype), my_governor(0)
  {
    switch(valuetype) {
    case V_ERROR:
    case V_NULL:
    case V_OMIT:
    case V_TTCN3_NULL:
    case V_DEFAULT_NULL:
    case V_FAT_NULL:
    case V_NOTUSED:
      break;
    case V_BOOL:
      u.val_bool=p.u.val_bool;
      break;
    case V_INT:
      u.val_Int=new int_val_t(*(p.u.val_Int));
      break;
    case V_NAMEDINT:
    case V_ENUM:
    case V_UNDEF_LOWERID:
      u.val_id=p.u.val_id->clone();
      break;
    case V_REAL:
      u.val_Real=p.u.val_Real;
      break;
    case V_BSTR:
    case V_HSTR:
    case V_OSTR:
    case V_CSTR:
    case V_ISO2022STR:
      set_val_str(new string(*p.u.str.val_str));
      break;
    case V_USTR:
      set_val_ustr(new ustring(*p.u.ustr.val_ustr));
      u.ustr.convert_str = p.u.ustr.convert_str;
      break;
    case V_CHARSYMS:
      u.char_syms = p.u.char_syms->clone();
      break;
    case V_OID:
    case V_ROID:
      u.oid_comps=new vector<OID_comp>;
      for(size_t i=0; i<p.u.oid_comps->size(); i++)
        add_oid_comp((*p.u.oid_comps)[i]->clone());
      break;
    case V_CHOICE:
      u.choice.alt_name=p.u.choice.alt_name->clone();
      u.choice.alt_value=p.u.choice.alt_value->clone();
      break;
    case V_SEQOF:
    case V_SETOF:
    case V_ARRAY:
      u.val_vs=p.u.val_vs->clone();
      break;
    case V_SEQ:
    case V_SET:
      u.val_nvs=p.u.val_nvs->clone();
      break;
    case V_REFD:
      u.ref.ref=p.u.ref.ref->clone();
      u.ref.refd_last=0;
      break;
    case V_NAMEDBITS:
      for(size_t i=0; i<p.u.ids->size(); i++) {
        Identifier *id = p.u.ids->get_nth_elem(i);
        u.ids->add(id->get_name(), id->clone());
      }
      break;
    case V_UNDEF_BLOCK:
      u.block=p.u.block->clone();
      break;
    case V_VERDICT:
      u.verdict=p.u.verdict;
      break;
    case V_EXPR:
      u.expr.v_optype = p.u.expr.v_optype;
      u.expr.state = EXPR_NOT_CHECKED;
      switch(u.expr.v_optype) {
      case OPTYPE_RND: // -
      case OPTYPE_COMP_NULL:
      case OPTYPE_COMP_MTC:
      case OPTYPE_COMP_SYSTEM:
      case OPTYPE_COMP_SELF:
      case OPTYPE_COMP_RUNNING_ANY:
      case OPTYPE_COMP_RUNNING_ALL:
      case OPTYPE_COMP_ALIVE_ANY:
      case OPTYPE_COMP_ALIVE_ALL:
      case OPTYPE_TMR_RUNNING_ANY:
      case OPTYPE_GETVERDICT:
      case OPTYPE_TESTCASENAME:
      case OPTYPE_PROF_RUNNING:
        break;
      case OPTYPE_COMP_RUNNING: // v1 [r2] b4
      case OPTYPE_COMP_ALIVE:
        u.expr.r2 = p.u.expr.r2 != NULL ? p.u.expr.r2->clone() : NULL;
        u.expr.b4 = p.u.expr.b4;
        // no break
      case OPTYPE_UNARYPLUS: // v1
      case OPTYPE_UNARYMINUS:
      case OPTYPE_NOT:
      case OPTYPE_NOT4B:
      case OPTYPE_BIT2HEX:
      case OPTYPE_BIT2INT:
      case OPTYPE_BIT2OCT:
      case OPTYPE_BIT2STR:
      case OPTYPE_CHAR2INT:
      case OPTYPE_CHAR2OCT:
      case OPTYPE_FLOAT2INT:
      case OPTYPE_FLOAT2STR:
      case OPTYPE_HEX2BIT:
      case OPTYPE_HEX2INT:
      case OPTYPE_HEX2OCT:
      case OPTYPE_HEX2STR:
      case OPTYPE_INT2CHAR:
      case OPTYPE_INT2FLOAT:
      case OPTYPE_INT2STR:
      case OPTYPE_INT2UNICHAR:
      case OPTYPE_OCT2BIT:
      case OPTYPE_OCT2CHAR:
      case OPTYPE_OCT2HEX:
      case OPTYPE_OCT2INT:
      case OPTYPE_OCT2STR:
      case OPTYPE_STR2BIT:
      case OPTYPE_STR2FLOAT:
      case OPTYPE_STR2HEX:
      case OPTYPE_STR2INT:
      case OPTYPE_STR2OCT:
      case OPTYPE_UNICHAR2INT:
      case OPTYPE_UNICHAR2CHAR:
      case OPTYPE_ENUM2INT:
      case OPTYPE_RNDWITHVAL:
      case OPTYPE_GET_STRINGENCODING:
      case OPTYPE_DECODE_BASE64:
      case OPTYPE_REMOVE_BOM:
        u.expr.v1=p.u.expr.v1->clone();
        break;
      case OPTYPE_HOSTID: // [v1]
        u.expr.v1=p.u.expr.v1?p.u.expr.v1->clone():0;
        break;
      case OPTYPE_ADD: // v1 v2
      case OPTYPE_SUBTRACT:
      case OPTYPE_MULTIPLY:
      case OPTYPE_DIVIDE:
      case OPTYPE_MOD:
      case OPTYPE_REM:
      case OPTYPE_CONCAT:
      case OPTYPE_EQ:
      case OPTYPE_LT:
      case OPTYPE_GT:
      case OPTYPE_NE:
      case OPTYPE_GE:
      case OPTYPE_LE:
      case OPTYPE_AND:
      case OPTYPE_OR:
      case OPTYPE_XOR:
      case OPTYPE_AND4B:
      case OPTYPE_OR4B:
      case OPTYPE_XOR4B:
      case OPTYPE_SHL:
      case OPTYPE_SHR:
      case OPTYPE_ROTL:
      case OPTYPE_ROTR:
      case OPTYPE_INT2BIT:
      case OPTYPE_INT2HEX:
      case OPTYPE_INT2OCT:
        u.expr.v1=p.u.expr.v1->clone();
        u.expr.v2=p.u.expr.v2->clone();
        break;
      case OPTYPE_UNICHAR2OCT: // v1 [v2]
      case OPTYPE_OCT2UNICHAR:
      case OPTYPE_ENCODE_BASE64:
        u.expr.v1=p.u.expr.v1->clone();
        u.expr.v2=p.u.expr.v2?p.u.expr.v2->clone():0;
        break;
      case OPTYPE_DECODE: // r1 r2
      case OPTYPE_UNDEF_RUNNING: // r1 [r2] b4
      case OPTYPE_TMR_RUNNING: // r1 [r2] b4
        u.expr.r1=p.u.expr.r1->clone();
        u.expr.r2 = p.u.expr.r2 != NULL ? p.u.expr.r2->clone() : NULL;
        u.expr.b4 = p.u.expr.b4;
        break;
      case OPTYPE_SUBSTR:
        u.expr.ti1=p.u.expr.ti1->clone();
        u.expr.v2=p.u.expr.v2->clone();
        u.expr.v3=p.u.expr.v3->clone();
        break;
      case OPTYPE_REGEXP:
        u.expr.ti1=p.u.expr.ti1->clone();
        u.expr.t2=p.u.expr.t2->clone();
        u.expr.v3=p.u.expr.v3->clone();
        u.expr.b4=p.u.expr.b4;
        break;
      case OPTYPE_DECOMP: // v1 v2 v3
        u.expr.v1=p.u.expr.v1->clone();
        u.expr.v2=p.u.expr.v2->clone();
        u.expr.v3=p.u.expr.v3->clone();
        break;
      case OPTYPE_REPLACE:
        u.expr.ti1 = p.u.expr.ti1->clone();
        u.expr.v2 = p.u.expr.v2->clone();
        u.expr.v3 = p.u.expr.v3->clone();
        u.expr.ti4 = p.u.expr.ti4->clone();
        break;
      case OPTYPE_LENGTHOF: // ti1
      case OPTYPE_SIZEOF:  // ti1
      case OPTYPE_VALUEOF: // ti1
      case OPTYPE_ENCODE:
      case OPTYPE_ISPRESENT:
      case OPTYPE_TTCN2STRING:
        u.expr.ti1=p.u.expr.ti1->clone();
        break;
      case OPTYPE_ENCVALUE_UNICHAR: // ti1 [v2]
        u.expr.ti1=p.u.expr.ti1->clone();
        u.expr.v2=p.u.expr.v2?p.u.expr.v2->clone():0;
        break; 
      case OPTYPE_DECVALUE_UNICHAR: // r1 r2 [v3]
        u.expr.r1 = p.u.expr.r1->clone();
        u.expr.r2 = p.u.expr.r2->clone();
        u.expr.v3=p.u.expr.v3?p.u.expr.v3->clone():0;
        break;
      case OPTYPE_TMR_READ:
      case OPTYPE_ACTIVATE:
        u.expr.r1=p.u.expr.r1->clone();
        break;
      case OPTYPE_EXECUTE: // r1 [v2]
        u.expr.r1=p.u.expr.r1->clone();
        u.expr.v2=p.u.expr.v2?p.u.expr.v2->clone():0;
        break;
      case OPTYPE_CHECKSTATE_ANY: // [r1] v2
      case OPTYPE_CHECKSTATE_ALL:
        u.expr.r1=p.u.expr.r1?p.u.expr.r1->clone():0;
        u.expr.v2=p.u.expr.v2->clone();
        break;
      case OPTYPE_COMP_CREATE: // r1 [v2] [v3]
        u.expr.r1=p.u.expr.r1->clone();
        u.expr.v2=p.u.expr.v2?p.u.expr.v2->clone():0;
        u.expr.v3=p.u.expr.v3?p.u.expr.v3->clone():0;
        u.expr.b4 = p.u.expr.b4;
        break;
      case OPTYPE_MATCH: // v1 t2
        u.expr.v1=p.u.expr.v1->clone();
        u.expr.t2=p.u.expr.t2->clone();
        break;
      case OPTYPE_ISCHOSEN: // r1 i2
        u.expr.r1=p.u.expr.r1->clone();
        u.expr.i2=p.u.expr.i2->clone();
        break;
      case OPTYPE_ISCHOSEN_V: // v1 i2
        u.expr.v1=p.u.expr.v1->clone();
        u.expr.i2=p.u.expr.i2->clone();
        break;
      case OPTYPE_ISCHOSEN_T: // t1 i2
        u.expr.t1=p.u.expr.t1->clone();
        u.expr.i2=p.u.expr.i2->clone();
        break;
      case OPTYPE_ACTIVATE_REFD:
        u.expr.v1 = p.u.expr.v1->clone();
        if(p.u.expr.state!=EXPR_CHECKED)
          u.expr.t_list2 = p.u.expr.t_list2->clone();
        else {
          u.expr.ap_list2 = p.u.expr.ap_list2->clone();
          u.expr.state = EXPR_CHECKED;
          }
        break;
      case OPTYPE_EXECUTE_REFD:
        u.expr.v1 = p.u.expr.v1->clone();
        if(p.u.expr.state!=EXPR_CHECKED)
          u.expr.t_list2 = p.u.expr.t_list2->clone();
        else {
          u.expr.ap_list2 = p.u.expr.ap_list2->clone();
          u.expr.state = EXPR_CHECKED;
          }
        u.expr.v3 = p.u.expr.v3 ? p.u.expr.v3->clone() : 0;
        break;
      case OPTYPE_LOG2STR:
      case OPTYPE_ANY2UNISTR:
        u.expr.logargs = p.u.expr.logargs->clone();
        break;
      default:
        FATAL_ERROR("Value::Value()");
      } // switch
      break;
    case V_MACRO:
      u.macro = p.u.macro;
      break;
    case V_FUNCTION:
    case V_ALTSTEP:
    case V_TESTCASE:
      u.refd_fat = p.u.refd_fat;
      break;
    case V_INVOKE:
      u.invoke.v = p.u.invoke.v->clone();
      u.invoke.t_list = p.u.invoke.t_list?p.u.invoke.t_list->clone():0;
      u.invoke.ap_list = p.u.invoke.ap_list?p.u.invoke.ap_list->clone():0;
      break;
    case V_REFER:
      u.refered = p.u.refered->clone();
      break;
    case V_ANY_VALUE:
    case V_ANY_OR_OMIT:
      u.len_res = p.u.len_res != NULL ? p.u.len_res->clone() : NULL;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }

  void Value::clean_up()
  {
    switch (valuetype) {
    case V_ERROR:
    case V_NULL:
    case V_BOOL:
    case V_REAL:
    case V_OMIT:
    case V_VERDICT:
    case V_TTCN3_NULL:
    case V_DEFAULT_NULL:
    case V_FAT_NULL:
    case V_MACRO:
    case V_NOTUSED:
    case V_FUNCTION:
    case V_ALTSTEP:
    case V_TESTCASE:
      break;
    case V_INT:
      delete u.val_Int;
      break;
    case V_NAMEDINT:
    case V_ENUM:
    case V_UNDEF_LOWERID:
      delete u.val_id;
      break;
    case V_BSTR:
    case V_HSTR:
    case V_OSTR:
    case V_CSTR:
    case V_ISO2022STR:
      delete u.str.val_str;
      clean_up_string_elements(u.str.str_elements);
      break;
    case V_USTR:
      delete u.ustr.val_ustr;
      clean_up_string_elements(u.ustr.ustr_elements);
      break;
    case V_CHARSYMS:
      delete u.char_syms;
      break;
    case V_OID:
    case V_ROID:
      if (u.oid_comps) {
	for(size_t i=0; i<u.oid_comps->size(); i++)
          delete (*u.oid_comps)[i];
	u.oid_comps->clear();
	delete u.oid_comps;
      }
      break;
    case V_EXPR:
      clean_up_expr();
      break;
    case V_CHOICE:
      delete u.choice.alt_name;
      delete u.choice.alt_value;
      break;
    case V_SEQOF:
    case V_SETOF:
    case V_ARRAY:
      delete u.val_vs;
      break;
    case V_SEQ:
    case V_SET:
      delete u.val_nvs;
      break;
    case V_REFD:
      delete u.ref.ref;
      break;
    case V_REFER:
      delete u.refered;
      break;
    case V_INVOKE:
      delete u.invoke.v;
      delete u.invoke.t_list;
      delete u.invoke.ap_list;
      break;
    case V_NAMEDBITS:
      if(u.ids) {
        for(size_t i=0; i<u.ids->size(); i++) delete u.ids->get_nth_elem(i);
        u.ids->clear();
        delete u.ids;
      }
      break;
    case V_UNDEF_BLOCK:
      delete u.block;
      break;
    case V_ANY_VALUE:
    case V_ANY_OR_OMIT:
      delete u.len_res;
      break;
    default:
      FATAL_ERROR("Value::clean_up()");
    } // switch
  }

  void Value::clean_up_expr()
  {
    switch (u.expr.state) {
    case EXPR_CHECKING:
    case EXPR_CHECKING_ERR:
      FATAL_ERROR("Value::clean_up_expr()");
    default:
      break;
    }
    switch (u.expr.v_optype) {
    case OPTYPE_RND: // -
    case OPTYPE_COMP_NULL:
    case OPTYPE_COMP_MTC:
    case OPTYPE_COMP_SYSTEM:
    case OPTYPE_COMP_SELF:
    case OPTYPE_COMP_RUNNING_ANY:
    case OPTYPE_COMP_RUNNING_ALL:
    case OPTYPE_COMP_ALIVE_ANY:
    case OPTYPE_COMP_ALIVE_ALL:
    case OPTYPE_TMR_RUNNING_ANY:
    case OPTYPE_GETVERDICT:
    case OPTYPE_TESTCASENAME:
    case OPTYPE_PROF_RUNNING:
      break;
    case OPTYPE_COMP_RUNNING: // v1 [r2] b4
    case OPTYPE_COMP_ALIVE:
      delete u.expr.r2;
      // no break
    case OPTYPE_UNARYPLUS: // v1
    case OPTYPE_UNARYMINUS:
    case OPTYPE_NOT:
    case OPTYPE_NOT4B:
    case OPTYPE_BIT2HEX:
    case OPTYPE_BIT2INT:
    case OPTYPE_BIT2OCT:
    case OPTYPE_BIT2STR:
    case OPTYPE_CHAR2INT:
    case OPTYPE_CHAR2OCT:
    case OPTYPE_FLOAT2INT:
    case OPTYPE_FLOAT2STR:
    case OPTYPE_HEX2BIT:
    case OPTYPE_HEX2INT:
    case OPTYPE_HEX2OCT:
    case OPTYPE_HEX2STR:
    case OPTYPE_INT2CHAR:
    case OPTYPE_INT2FLOAT:
    case OPTYPE_INT2STR:
    case OPTYPE_INT2UNICHAR:
    case OPTYPE_OCT2BIT:
    case OPTYPE_OCT2CHAR:
    case OPTYPE_OCT2HEX:
    case OPTYPE_OCT2INT:
    case OPTYPE_OCT2STR:
    case OPTYPE_STR2BIT:
    case OPTYPE_STR2FLOAT:
    case OPTYPE_STR2HEX:
    case OPTYPE_STR2INT:
    case OPTYPE_STR2OCT:
    case OPTYPE_UNICHAR2INT:
    case OPTYPE_UNICHAR2CHAR:
    case OPTYPE_ENUM2INT:
    case OPTYPE_RNDWITHVAL:
    case OPTYPE_REMOVE_BOM:
    case OPTYPE_GET_STRINGENCODING:
    case OPTYPE_DECODE_BASE64:
    case OPTYPE_HOSTID:
      delete u.expr.v1;
      break;
    case OPTYPE_ADD: // v1 v2
    case OPTYPE_SUBTRACT:
    case OPTYPE_MULTIPLY:
    case OPTYPE_DIVIDE:
    case OPTYPE_MOD:
    case OPTYPE_REM:
    case OPTYPE_CONCAT:
    case OPTYPE_EQ:
    case OPTYPE_LT:
    case OPTYPE_GT:
    case OPTYPE_NE:
    case OPTYPE_GE:
    case OPTYPE_LE:
    case OPTYPE_AND:
    case OPTYPE_OR:
    case OPTYPE_XOR:
    case OPTYPE_AND4B:
    case OPTYPE_OR4B:
    case OPTYPE_XOR4B:
    case OPTYPE_SHL:
    case OPTYPE_SHR:
    case OPTYPE_ROTL:
    case OPTYPE_ROTR:
    case OPTYPE_INT2BIT:
    case OPTYPE_INT2HEX:
    case OPTYPE_INT2OCT:
    case OPTYPE_UNICHAR2OCT:
    case OPTYPE_OCT2UNICHAR:
    case OPTYPE_ENCODE_BASE64:
      delete u.expr.v1;
      delete u.expr.v2;
      break;
    case OPTYPE_DECODE: // r1 r2
    case OPTYPE_UNDEF_RUNNING: // r1 [r2] b4
    case OPTYPE_TMR_RUNNING: // r1 [r2] b4
      delete u.expr.r1;
      delete u.expr.r2;
      break;
    case OPTYPE_SUBSTR:
      delete u.expr.ti1;
      delete u.expr.v2;
      delete u.expr.v3;
      break;
    case OPTYPE_REGEXP:
      delete u.expr.ti1;
      delete u.expr.t2;
      delete u.expr.v3;
      break;
    case OPTYPE_DECOMP: // v1 v2 v3
      delete u.expr.v1;
      delete u.expr.v2;
      delete u.expr.v3;
      break;
    case OPTYPE_REPLACE:
      delete u.expr.ti1;
      delete u.expr.v2;
      delete u.expr.v3;
      delete u.expr.ti4;
      break;
    case OPTYPE_LENGTHOF: // ti1
    case OPTYPE_SIZEOF:  // ti1
    case OPTYPE_VALUEOF: // ti1
    case OPTYPE_ISVALUE:
    case OPTYPE_ISBOUND:
    case OPTYPE_ENCODE:
    case OPTYPE_ISPRESENT:
    case OPTYPE_TTCN2STRING:
      delete u.expr.ti1;
      break;
    case OPTYPE_ISTEMPLATEKIND: // ti1 v2
    case OPTYPE_ENCVALUE_UNICHAR: // ti1 [v2]
      delete u.expr.ti1;
      delete u.expr.v2;
      break;
    case OPTYPE_DECVALUE_UNICHAR: // r1 r2 [v3]
      delete u.expr.r1;
      delete u.expr.r2;
      delete u.expr.v3;
      break;
    case OPTYPE_TMR_READ:
    case OPTYPE_ACTIVATE:
      delete u.expr.r1;
      break;
    case OPTYPE_EXECUTE: // r1 [v2]
      delete u.expr.r1;
      delete u.expr.v2;
      break;
    case OPTYPE_CHECKSTATE_ANY: // [r1] v2
    case OPTYPE_CHECKSTATE_ALL:
      delete u.expr.r1;
      delete u.expr.v2;
      break;
    case OPTYPE_COMP_CREATE: // r1 [v2] [v3] b4
      delete u.expr.r1;
      delete u.expr.v2;
      delete u.expr.v3;
      break;
    case OPTYPE_MATCH: // v1 t2
      delete u.expr.v1;
      delete u.expr.t2;
      break;
    case OPTYPE_ISCHOSEN: // r1 i2
      delete u.expr.r1;
      delete u.expr.i2;
      break;
    case OPTYPE_ISCHOSEN_V: // v1 i2
      delete u.expr.v1;
      delete u.expr.i2;
      break;
    case OPTYPE_ISCHOSEN_T: // t1 i2
      delete u.expr.t1;
      delete u.expr.i2;
      break;
    case OPTYPE_ACTIVATE_REFD: //v1 t_list2
      delete u.expr.v1;
      if(u.expr.state!=EXPR_CHECKED)
        delete u.expr.t_list2;
      else
        delete u.expr.ap_list2;
      break;
    case OPTYPE_EXECUTE_REFD: //v1 t_list2 [v3]
      delete u.expr.v1;
      if(u.expr.state!=EXPR_CHECKED)
        delete u.expr.t_list2;
      else
        delete u.expr.ap_list2;
      delete u.expr.v3;
      break;
    case OPTYPE_LOG2STR:
    case OPTYPE_ANY2UNISTR:
      delete u.expr.logargs;
      break;
    default:
      FATAL_ERROR("Value::clean_up_expr()");
    } // switch
  }

  void Value::copy_and_destroy(Value *src)
  {
    clean_up();
    valuetype = src->valuetype;
    u = src->u;
    // update the pointer used for caching if it points to the value itself
    if (valuetype == V_REFD && u.ref.refd_last == src) u.ref.refd_last = this;
    src->valuetype = V_ERROR;
    delete src;
  }

  Value::Value(valuetype_t p_vt)
    : GovernedSimple(S_V), valuetype(p_vt), my_governor(0)
  {
    switch(valuetype) {
    case V_NULL:
    case V_TTCN3_NULL:
    case V_OMIT:
    case V_NOTUSED:
    case V_ERROR:
      break;
    case V_OID:
    case V_ROID:
      u.oid_comps=new vector<OID_comp>();
      break;
    case V_NAMEDBITS:
      u.ids=new map<string, Identifier>();
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }

  Value::Value(valuetype_t p_vt, bool p_val_bool)
    : GovernedSimple(S_V), valuetype(p_vt), my_governor(0)
  {
    switch(valuetype) {
    case V_BOOL:
      u.val_bool=p_val_bool;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }

  Value::Value(valuetype_t p_vt, const Int& p_val_Int)
    : GovernedSimple(S_V), valuetype(p_vt), my_governor(0)
  {
    switch(valuetype) {
    case V_INT:
      u.val_Int=new int_val_t(p_val_Int);
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }

  Value::Value(valuetype_t p_vt, int_val_t *p_val_Int)
    : GovernedSimple(S_V), valuetype(p_vt), my_governor(0)
  {
    switch(valuetype){
    case V_INT:
      u.val_Int=p_val_Int;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    }
  }

  Value::Value(valuetype_t p_vt, string *p_val_str)
    : GovernedSimple(S_V), valuetype(p_vt), my_governor(0)
  {
    if(!p_val_str) FATAL_ERROR("NULL parameter");
    switch(valuetype) {
    case V_BSTR:
    case V_HSTR:
    case V_OSTR:
    case V_CSTR:
    case V_ISO2022STR:
      set_val_str(p_val_str);
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }

  Value::Value(valuetype_t p_vt, ustring *p_val_ustr)
    : GovernedSimple(S_V), valuetype(p_vt), my_governor(0)
  {
    if (p_vt != V_USTR || !p_val_ustr) FATAL_ERROR("Value::Value()");
    set_val_ustr(p_val_ustr);
    u.ustr.convert_str = false;
  }

  Value::Value(valuetype_t p_vt, CharSyms *p_char_syms)
    : GovernedSimple(S_V), valuetype(p_vt), my_governor(0)
  {
    if (!p_char_syms) FATAL_ERROR("NULL parameter");
    switch (valuetype) {
    case V_CHARSYMS:
      u.char_syms = p_char_syms;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }

  Value::Value(valuetype_t p_vt, Identifier *p_val_id)
    : GovernedSimple(S_V), valuetype(p_vt), my_governor(0)
  {
    if(!p_val_id)
      FATAL_ERROR("NULL parameter");
    switch(valuetype) {
    case V_NAMEDINT:
    case V_ENUM:
    case V_UNDEF_LOWERID:
      u.val_id=p_val_id;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }

  Value::Value(valuetype_t p_vt, Identifier *p_id, Value *p_val)
    : GovernedSimple(S_V), valuetype(p_vt), my_governor(0)
  {
    if(!p_id || !p_val)
      FATAL_ERROR("NULL parameter");
    switch(valuetype) {
    case V_CHOICE:
      u.choice.alt_name=p_id;
      u.choice.alt_value=p_val;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }

  Value::Value(valuetype_t p_vt, const Real& p_val_Real)
    : GovernedSimple(S_V), valuetype(p_vt), my_governor(0)
  {
    switch(valuetype) {
    case V_REAL:
      u.val_Real=p_val_Real;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }

  Value::Value(valuetype_t p_vt, Values *p_vs)
    : GovernedSimple(S_V), valuetype(p_vt), my_governor(0)
  {
    if(!p_vs) FATAL_ERROR("NULL parameter");
    switch(valuetype) {
    case V_SEQOF:
    case V_SETOF:
    case V_ARRAY:
      u.val_vs=p_vs;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }

  Value::Value(valuetype_t p_vt, Value *p_v,
    Ttcn::ParsedActualParameters *p_t_list)
    : GovernedSimple(S_V), valuetype(p_vt), my_governor(0)
  {
    if(!p_v || !p_t_list) FATAL_ERROR("NULL parameter");
    switch(valuetype) {
    case V_INVOKE:
      u.invoke.v = p_v;
      u.invoke.t_list = p_t_list;
      u.invoke.ap_list = 0;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    }
  }

  // -
  Value::Value(operationtype_t p_optype)
    : GovernedSimple(S_V), valuetype(V_EXPR), my_governor(0)
  {
    u.expr.v_optype = p_optype;
    u.expr.state = EXPR_NOT_CHECKED;
    switch(p_optype) {
    case OPTYPE_RND:
    case OPTYPE_COMP_NULL:
    case OPTYPE_COMP_MTC:
    case OPTYPE_COMP_SYSTEM:
    case OPTYPE_COMP_SELF:
    case OPTYPE_COMP_RUNNING_ANY:
    case OPTYPE_COMP_RUNNING_ALL:
    case OPTYPE_COMP_ALIVE_ANY:
    case OPTYPE_COMP_ALIVE_ALL:
    case OPTYPE_TMR_RUNNING_ANY:
    case OPTYPE_GETVERDICT:
    case OPTYPE_TESTCASENAME:
    case OPTYPE_PROF_RUNNING:
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }

  // v1
  Value::Value(operationtype_t p_optype, Value *p_v1)
    : GovernedSimple(S_V), valuetype(V_EXPR), my_governor(0)
  {
    u.expr.v_optype = p_optype;
    u.expr.state = EXPR_NOT_CHECKED;
    switch(p_optype) {
    case OPTYPE_UNARYPLUS:
    case OPTYPE_UNARYMINUS:
    case OPTYPE_NOT:
    case OPTYPE_NOT4B:
    case OPTYPE_BIT2HEX:
    case OPTYPE_BIT2INT:
    case OPTYPE_BIT2OCT:
    case OPTYPE_BIT2STR:
    case OPTYPE_CHAR2INT:
    case OPTYPE_CHAR2OCT:
    case OPTYPE_FLOAT2INT:
    case OPTYPE_FLOAT2STR:
    case OPTYPE_HEX2BIT:
    case OPTYPE_HEX2INT:
    case OPTYPE_HEX2OCT:
    case OPTYPE_HEX2STR:
    case OPTYPE_INT2CHAR:
    case OPTYPE_INT2FLOAT:
    case OPTYPE_INT2STR:
    case OPTYPE_INT2UNICHAR:
    case OPTYPE_OCT2BIT:
    case OPTYPE_OCT2CHAR:
    case OPTYPE_OCT2HEX:
    case OPTYPE_OCT2INT:
    case OPTYPE_OCT2STR:
    case OPTYPE_STR2BIT:
    case OPTYPE_STR2FLOAT:
    case OPTYPE_STR2HEX:
    case OPTYPE_STR2INT:
    case OPTYPE_STR2OCT:
    case OPTYPE_UNICHAR2INT:
    case OPTYPE_UNICHAR2CHAR:
    case OPTYPE_ENUM2INT:
    case OPTYPE_RNDWITHVAL:
    case OPTYPE_REMOVE_BOM:
    case OPTYPE_GET_STRINGENCODING:
    case OPTYPE_DECODE_BASE64:
      if(!p_v1) FATAL_ERROR("Value::Value()");
      u.expr.v1=p_v1;
      break;
    case OPTYPE_HOSTID:
      u.expr.v1=p_v1;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }
  
  // v1 [r2] b4
  Value::Value(operationtype_t p_optype, Value* p_v1, Ttcn::Ref_base *p_r2,
               bool p_b4)
    : GovernedSimple(S_V), valuetype(V_EXPR), my_governor(0)
  {
    u.expr.v_optype = p_optype;
    u.expr.state = EXPR_NOT_CHECKED;
    switch(p_optype) {
    case OPTYPE_COMP_RUNNING:
    case OPTYPE_COMP_ALIVE:
      if (p_v1 == NULL) {
        FATAL_ERROR("Value::Value()");
      }
      u.expr.v1 = p_v1;
      u.expr.r2 = p_r2;
      u.expr.b4 = p_b4;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }

  // ti1
  Value::Value(operationtype_t p_optype, TemplateInstance *p_ti1)
    : GovernedSimple(S_V), valuetype(V_EXPR), my_governor(0)
  {
    u.expr.v_optype = p_optype;
    u.expr.state = EXPR_NOT_CHECKED;
    switch(p_optype) {
    case OPTYPE_LENGTHOF:
    case OPTYPE_SIZEOF:
    case OPTYPE_VALUEOF:
    case OPTYPE_ISVALUE:
    case OPTYPE_ISBOUND:
    case OPTYPE_ENCODE:
    case OPTYPE_ENCVALUE_UNICHAR:
    case OPTYPE_ISPRESENT:
    case OPTYPE_TTCN2STRING:
      if(!p_ti1) FATAL_ERROR("Value::Value()");
      u.expr.ti1=p_ti1;
      // Needed in the case of OPTYPE_ENCVALUE_UNICHAR
      u.expr.v2=NULL;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }

  // r1
  Value::Value(operationtype_t p_optype, Ttcn::Ref_base *p_r1)
    : GovernedSimple(S_V), valuetype(V_EXPR), my_governor(0)
  {
    u.expr.v_optype = p_optype;
    u.expr.state = EXPR_NOT_CHECKED;
    switch(p_optype) {
    case OPTYPE_TMR_READ:
    case OPTYPE_ACTIVATE:
      if(!p_r1) FATAL_ERROR("Value::Value()");
      u.expr.r1=p_r1;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }
  
  // r1 [r2] b4
  Value::Value(operationtype_t p_optype, Ttcn::Ref_base* p_r1, Ttcn::Ref_base* p_r2,
               bool p_b4)
    : GovernedSimple(S_V), valuetype(V_EXPR), my_governor(0)
  {
    u.expr.v_optype = p_optype;
    u.expr.state = EXPR_NOT_CHECKED;
    switch (p_optype) {
    case OPTYPE_UNDEF_RUNNING:
      if (p_r1 == NULL) {
        FATAL_ERROR("Value::Value()");
      }
      u.expr.r1 = p_r1;
      u.expr.r2 = p_r2;
      u.expr.b4 = p_b4;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }

  // v1 t_list2
  Value::Value(operationtype_t p_optype, Value *p_v1,
      Ttcn::ParsedActualParameters *p_ap_list)
    : GovernedSimple(S_V), valuetype(V_EXPR), my_governor(0)
  {
    u.expr.v_optype = p_optype;
    u.expr.state = EXPR_NOT_CHECKED;
    switch(p_optype) {
    case OPTYPE_ACTIVATE_REFD:
      if(!p_v1 || !p_ap_list) FATAL_ERROR("Value::Value()");
      u.expr.v1 = p_v1;
      u.expr.t_list2 = p_ap_list;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    }
  }

  //v1 t_list2 v3
  Value::Value(operationtype_t p_optype, Value *p_v1,
    Ttcn::ParsedActualParameters *p_t_list2, Value *p_v3)
    : GovernedSimple(S_V), valuetype(V_EXPR), my_governor(0)
  {
    u.expr.v_optype = p_optype;
    u.expr.state = EXPR_NOT_CHECKED;
    switch(p_optype) {
    case OPTYPE_EXECUTE_REFD:
      if(!p_v1 || !p_t_list2) FATAL_ERROR("Value::Value()");
      u.expr.v1 = p_v1;
      u.expr.t_list2 = p_t_list2;
      u.expr.v3 = p_v3;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    }
  }

  // r1 [v2] or [r1] v2
  Value::Value(operationtype_t p_optype, Ttcn::Ref_base *p_r1, Value *p_v2)
    : GovernedSimple(S_V), valuetype(V_EXPR), my_governor(0)
  {
    u.expr.v_optype = p_optype;
    u.expr.state = EXPR_NOT_CHECKED;
    switch(p_optype) {
    case OPTYPE_EXECUTE:
      if(!p_r1) FATAL_ERROR("Value::Value()");
      u.expr.r1=p_r1;
      u.expr.v2=p_v2;
      break;
    case OPTYPE_CHECKSTATE_ANY:
    case OPTYPE_CHECKSTATE_ALL:
      if(!p_v2) FATAL_ERROR("Value::Value()");
      u.expr.r1=p_r1; // may be null if any port or all port
      u.expr.v2=p_v2;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }

  // r1 [v2] [v3] b4
  Value::Value(operationtype_t p_optype, Ttcn::Ref_base *p_r1,
               Value *p_v2, Value *p_v3, bool p_b4)
    : GovernedSimple(S_V), valuetype(V_EXPR), my_governor(0)
  {
    u.expr.v_optype = p_optype;
    u.expr.state = EXPR_NOT_CHECKED;
    switch(p_optype) {
    case OPTYPE_COMP_CREATE:
      if(!p_r1) FATAL_ERROR("Value::Value()");
      u.expr.r1=p_r1;
      u.expr.v2=p_v2;
      u.expr.v3=p_v3;
      u.expr.b4=p_b4;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }

  // v1 v2
  Value::Value(operationtype_t p_optype, Value *p_v1, Value *p_v2)
    : GovernedSimple(S_V), valuetype(V_EXPR), my_governor(0)
  {
    u.expr.v_optype = p_optype;
    u.expr.state = EXPR_NOT_CHECKED;
    switch(p_optype) {
    case OPTYPE_ADD:
    case OPTYPE_SUBTRACT:
    case OPTYPE_MULTIPLY:
    case OPTYPE_DIVIDE:
    case OPTYPE_MOD:
    case OPTYPE_REM:
    case OPTYPE_CONCAT:
    case OPTYPE_EQ:
    case OPTYPE_LT:
    case OPTYPE_GT:
    case OPTYPE_NE:
    case OPTYPE_GE:
    case OPTYPE_LE:
    case OPTYPE_AND:
    case OPTYPE_OR:
    case OPTYPE_XOR:
    case OPTYPE_AND4B:
    case OPTYPE_OR4B:
    case OPTYPE_XOR4B:
    case OPTYPE_SHL:
    case OPTYPE_SHR:
    case OPTYPE_ROTL:
    case OPTYPE_ROTR:
    case OPTYPE_INT2BIT:
    case OPTYPE_INT2HEX:
    case OPTYPE_INT2OCT:
      if(!p_v1 || !p_v2) FATAL_ERROR("Value::Value()");
      u.expr.v1=p_v1;
      u.expr.v2=p_v2;
      break;
    case OPTYPE_UNICHAR2OCT:
    case OPTYPE_OCT2UNICHAR:
    case OPTYPE_ENCODE_BASE64:
      if(!p_v1) FATAL_ERROR("Value::Value()");
      u.expr.v1=p_v1;
      // p_v2 may be NULL if there is no second param
      u.expr.v2=p_v2;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }

  // ti1 v2 v3 ti4
  Value::Value(operationtype_t p_optype, TemplateInstance *p_ti1, Value *p_v2,
    Value *p_v3, TemplateInstance *p_ti4) :
      GovernedSimple(S_V), valuetype(V_EXPR), my_governor(0)
  {
    u.expr.v_optype = p_optype;
    u.expr.state = EXPR_NOT_CHECKED;
    switch (p_optype) {
    case OPTYPE_REPLACE:
      if (!p_ti1 || !p_v2 || !p_v3 || !p_ti4) FATAL_ERROR("Value::Value()");
      u.expr.ti1 = p_ti1;
      u.expr.v2 = p_v2;
      u.expr.v3 = p_v3;
      u.expr.ti4 = p_ti4;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }

  // v1 v2 v3
  Value::Value(operationtype_t p_optype, Value *p_v1, Value *p_v2, Value *p_v3)
    : GovernedSimple(S_V), valuetype(V_EXPR), my_governor(0)
  {
    u.expr.v_optype = p_optype;
    u.expr.state = EXPR_NOT_CHECKED;
    switch(p_optype) {
    case OPTYPE_DECOMP:
      if(!p_v1 || !p_v2 || !p_v3) FATAL_ERROR("Value::Value()");
      u.expr.v1=p_v1;
      u.expr.v2=p_v2;
      u.expr.v3=p_v3;
      break;   
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }

  // ti1 [v2]
  Value::Value(operationtype_t p_optype, TemplateInstance *p_ti1,  Value *p_v2)
    : GovernedSimple(S_V), valuetype(V_EXPR), my_governor(0)
  {
    u.expr.v_optype = p_optype;
    u.expr.state = EXPR_NOT_CHECKED;
    switch(p_optype) {
    case OPTYPE_ISTEMPLATEKIND:
    case OPTYPE_ENCVALUE_UNICHAR:
      if(!p_ti1 || !p_v2) FATAL_ERROR("Value::Value()");
      u.expr.ti1=p_ti1;
      u.expr.v2=p_v2;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }
  
  // ti1 v2 v3
  Value::Value(operationtype_t p_optype, TemplateInstance *p_ti1, Value *p_v2, Value *p_v3)
    : GovernedSimple(S_V), valuetype(V_EXPR), my_governor(0)
  {
    u.expr.v_optype=p_optype;
    u.expr.state=EXPR_NOT_CHECKED;
    switch(p_optype) {
    case OPTYPE_SUBSTR:
      if(!p_ti1 || !p_v2 || !p_v3) FATAL_ERROR("Value::Value()");
      u.expr.ti1 = p_ti1;
      u.expr.v2=p_v2;
      u.expr.v3=p_v3;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }

  // ti1 t2 v3 b4
  Value::Value(operationtype_t p_optype, TemplateInstance *p_ti1,
               TemplateInstance *p_t2, Value *p_v3, bool p_b4)
    : GovernedSimple(S_V), valuetype(V_EXPR), my_governor(0)
  {
    u.expr.v_optype=p_optype;
    u.expr.state=EXPR_NOT_CHECKED;
    switch(p_optype) {
    case OPTYPE_REGEXP:
      if(!p_ti1 || !p_t2 || !p_v3) FATAL_ERROR("Value::Value()");
      u.expr.ti1 = p_ti1;
      u.expr.t2 = p_t2;
      u.expr.v3 = p_v3;
      u.expr.b4 = p_b4;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }

  // v1 t2
  Value::Value(operationtype_t p_optype, Value *p_v1, TemplateInstance *p_t2)
    : GovernedSimple(S_V), valuetype(V_EXPR), my_governor(0)
  {
    u.expr.v_optype = p_optype;
    u.expr.state = EXPR_NOT_CHECKED;
    switch(p_optype) {
    case OPTYPE_MATCH:
      if(!p_v1 || !p_t2) FATAL_ERROR("Value::Value()");
      u.expr.v1=p_v1;
      u.expr.t2=p_t2;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }

  // r1 i2
  Value::Value(operationtype_t p_optype, Ttcn::Reference *p_r1,
               Identifier *p_i2)
    : GovernedSimple(S_V), valuetype(V_EXPR), my_governor(0)
  {
    u.expr.v_optype = p_optype;
    u.expr.state = EXPR_NOT_CHECKED;
    switch(p_optype) {
    case OPTYPE_ISCHOSEN:
      if(!p_r1 || !p_i2) FATAL_ERROR("Value::Value()");
      u.expr.r1=p_r1;
      u.expr.i2=p_i2;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }

  Value::Value(operationtype_t p_optype, LogArguments *p_logargs)
    : GovernedSimple(S_V), valuetype(V_EXPR), my_governor(0)
  {
    u.expr.v_optype = p_optype;
    u.expr.state = EXPR_NOT_CHECKED;
    switch(p_optype) {
    case OPTYPE_LOG2STR:
    case OPTYPE_ANY2UNISTR:
      if (!p_logargs) FATAL_ERROR("Value::Value()");
      u.expr.logargs = p_logargs;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }

  Value::Value(valuetype_t p_vt, macrotype_t p_macrotype)
    : GovernedSimple(S_V), valuetype(p_vt), my_governor(0)
  {
    if (p_vt != V_MACRO) FATAL_ERROR("Value::Value()");
    switch (p_macrotype) {
    case MACRO_MODULEID:
    case MACRO_FILENAME:
    case MACRO_BFILENAME:
    case MACRO_FILEPATH:
    case MACRO_LINENUMBER:
    case MACRO_LINENUMBER_C:
    case MACRO_DEFINITIONID:
    case MACRO_SCOPE:
    case MACRO_TESTCASEID:
      break;
    default:
      FATAL_ERROR("Value::Value()");
    }
    u.macro = p_macrotype;
  }

  Value::Value(valuetype_t p_vt, NamedValues *p_nvs)
    : GovernedSimple(S_V), valuetype(p_vt), my_governor(0)
  {
    if(!p_nvs) FATAL_ERROR("NULL parameter");
    switch(valuetype) {
    case V_SEQ:
    case V_SET:
      u.val_nvs=p_nvs;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }

  Value::Value(valuetype_t p_vt, Reference *p_ref)
    : GovernedSimple(S_V), valuetype(p_vt), my_governor(0)
  {
    if (!p_ref) FATAL_ERROR("NULL parameter: Value::Value()");
    switch(p_vt) {
    case V_REFD:
      u.ref.ref = p_ref;
      u.ref.refd_last = 0;
      break;
    case V_REFER:
      u.refered = p_ref;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    }
  }

  Value::Value(valuetype_t p_vt, Block *p_block)
    : GovernedSimple(S_V), valuetype(p_vt), my_governor(0)
  {
    if(!p_block) FATAL_ERROR("NULL parameter");
    switch(valuetype) {
    case V_UNDEF_BLOCK:
      u.block=p_block;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }

  Value::Value(valuetype_t p_vt, verdict_t p_verdict)
    : GovernedSimple(S_V), valuetype(p_vt), my_governor(0)
  {
    if (valuetype != V_VERDICT) FATAL_ERROR("Value::Value()");
    switch (p_verdict) {
    case Verdict_NONE:
    case Verdict_PASS:
    case Verdict_INCONC:
    case Verdict_FAIL:
    case Verdict_ERROR:
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
    u.verdict = p_verdict;
  }

  Value::Value(operationtype_t p_optype, Ttcn::Ref_base *p_r1, Ttcn::Ref_base *p_r2)
    : GovernedSimple(S_V), valuetype(V_EXPR), my_governor(0)
  {
    u.expr.v_optype = p_optype;
    u.expr.state = EXPR_NOT_CHECKED;
    switch(p_optype) {
    case OPTYPE_DECODE:
    case OPTYPE_DECVALUE_UNICHAR:
      if(!p_r1 || !p_r2) FATAL_ERROR("Value::Value()");
      u.expr.r1=p_r1;
      u.expr.r2=p_r2;
      // Needed in the case of OPTYPE_DECVALUE_UNICHAR
      u.expr.v3=NULL;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }
  
  // r1 r2 [v3]
  Value::Value(operationtype_t p_optype, Ttcn::Ref_base *p_r1, Ttcn::Ref_base *p_r2,
          Value *p_v3)
    : GovernedSimple(S_V), valuetype(V_EXPR), my_governor(0)
  {
    u.expr.v_optype = p_optype;
    u.expr.state = EXPR_NOT_CHECKED;
    switch(p_optype) {
    case OPTYPE_DECVALUE_UNICHAR:
      if(!p_r1 || !p_r2 || !p_v3) FATAL_ERROR("Value::Value()");
      u.expr.r1=p_r1;
      u.expr.r2=p_r2;
      u.expr.v3=p_v3;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }
  
  // V_ANY_VALUE, V_ANY_OR_OMIT
  Value::Value(valuetype_t p_vt, Ttcn::LengthRestriction* p_len_res)
    : GovernedSimple(S_V), valuetype(p_vt), my_governor(0)
  {
    switch(p_vt) {
    case V_ANY_VALUE:
    case V_ANY_OR_OMIT:
      u.len_res = p_len_res;
      break;
    default:
      FATAL_ERROR("Value::Value()");
    } // switch
  }

  Value::~Value()
  {
    clean_up();
  }

  Value *Value::clone() const
  {
    return new Value(*this);
  }

  Value::operationtype_t Value::get_optype() const
  {
    if(valuetype!=V_EXPR)
      FATAL_ERROR("Value::get_optype()");
    return u.expr.v_optype;
  }
  
  Value* Value::get_concat_operand(bool first_operand) const
  {
    if (valuetype != V_EXPR || u.expr.v_optype != OPTYPE_CONCAT) {
      FATAL_ERROR("Value::get_concat_operand()");
    }
    return first_operand ? u.expr.v1 : u.expr.v2;
  }
  
  Ttcn::LengthRestriction* Value::take_length_restriction()
  {
    if (valuetype != V_ANY_VALUE && valuetype != V_ANY_OR_OMIT) {
      FATAL_ERROR("Value::take_length_restriction()");
    }
    Ttcn::LengthRestriction* ptr = u.len_res;
    u.len_res = NULL;
    return ptr;
  }

  void Value::set_my_governor(Type *p_gov)
  {
    if(!p_gov)
      FATAL_ERROR("Value::set_my_governor(): NULL parameter");
    my_governor=p_gov;
  }

  Type *Value::get_my_governor() const
  {
    return my_governor;
  }

  void Value::set_fullname(const string& p_fullname)
  {
    GovernedSimple::set_fullname(p_fullname);
    switch(valuetype) {
    case V_CHARSYMS:
      u.char_syms->set_fullname(p_fullname);
      break;
    case V_OID:
    case V_ROID:
      for(size_t i=0; i<u.oid_comps->size(); i++)
        (*u.oid_comps)[i]->set_fullname(p_fullname+"."+Int2string(i+1));
      break;
    case V_CHOICE:
      u.choice.alt_value->set_fullname(p_fullname + "." +
        u.choice.alt_name->get_dispname());
      break;
    case V_SEQOF:
    case V_SETOF:
    case V_ARRAY:
      u.val_vs->set_fullname(p_fullname);
      break;
    case V_SEQ:
    case V_SET:
      u.val_nvs->set_fullname(p_fullname);
      break;
    case V_REFD:
      u.ref.ref->set_fullname(p_fullname);
      break;
    case V_REFER:
      u.refered->set_fullname(p_fullname);
      break;
    case V_INVOKE:
      u.invoke.v->set_fullname(p_fullname);
      if(u.invoke.t_list) u.invoke.t_list->set_fullname(p_fullname);
      if(u.invoke.ap_list) u.invoke.ap_list->set_fullname(p_fullname);
      break;
    case V_EXPR:
      set_fullname_expr(p_fullname);
      break;
    default:
      break;
    } // switch
  }

  void Value::set_my_scope(Scope *p_scope)
  {
    GovernedSimple::set_my_scope(p_scope);
    switch(valuetype) {
    case V_CHARSYMS:
      u.char_syms->set_my_scope(p_scope);
      break;
    case V_OID:
    case V_ROID:
      for(size_t i=0; i<u.oid_comps->size(); i++)
        (*u.oid_comps)[i]->set_my_scope(p_scope);
      break;
    case V_CHOICE:
      u.choice.alt_value->set_my_scope(p_scope);
      break;
    case V_SEQOF:
    case V_SETOF:
    case V_ARRAY:
      u.val_vs->set_my_scope(p_scope);
      break;
    case V_SEQ:
    case V_SET:
      u.val_nvs->set_my_scope(p_scope);
      break;
    case V_REFD:
      u.ref.ref->set_my_scope(p_scope);
      break;
    case V_REFER:
      u.refered->set_my_scope(p_scope);
      break;
    case V_INVOKE:
      u.invoke.v->set_my_scope(p_scope);
      if(u.invoke.t_list) u.invoke.t_list->set_my_scope(p_scope);
      if(u.invoke.ap_list) u.invoke.ap_list->set_my_scope(p_scope);
      break;
    case V_EXPR:
      set_my_scope_expr(p_scope);
      break;
    default:
      break;
    } // switch
  }

  void Value::set_fullname_expr(const string& p_fullname)
  {
    switch (u.expr.v_optype) {
    case OPTYPE_RND: // -
    case OPTYPE_COMP_NULL:
    case OPTYPE_COMP_MTC:
    case OPTYPE_COMP_SYSTEM:
    case OPTYPE_COMP_SELF:
    case OPTYPE_COMP_RUNNING_ANY:
    case OPTYPE_COMP_RUNNING_ALL:
    case OPTYPE_COMP_ALIVE_ANY:
    case OPTYPE_COMP_ALIVE_ALL:
    case OPTYPE_TMR_RUNNING_ANY:
    case OPTYPE_GETVERDICT:
    case OPTYPE_TESTCASENAME:
    case OPTYPE_PROF_RUNNING:
      break;
    case OPTYPE_UNARYPLUS: // v1
    case OPTYPE_UNARYMINUS:
    case OPTYPE_NOT:
    case OPTYPE_NOT4B:
    case OPTYPE_BIT2HEX:
    case OPTYPE_BIT2INT:
    case OPTYPE_BIT2OCT:
    case OPTYPE_BIT2STR:
    case OPTYPE_CHAR2INT:
    case OPTYPE_CHAR2OCT:
    case OPTYPE_FLOAT2INT:
    case OPTYPE_FLOAT2STR:
    case OPTYPE_HEX2BIT:
    case OPTYPE_HEX2INT:
    case OPTYPE_HEX2OCT:
    case OPTYPE_HEX2STR:
    case OPTYPE_INT2CHAR:
    case OPTYPE_INT2FLOAT:
    case OPTYPE_INT2STR:
    case OPTYPE_INT2UNICHAR:
    case OPTYPE_OCT2BIT:
    case OPTYPE_OCT2CHAR:
    case OPTYPE_OCT2HEX:
    case OPTYPE_OCT2INT:
    case OPTYPE_OCT2STR:
    case OPTYPE_STR2BIT:
    case OPTYPE_STR2FLOAT:
    case OPTYPE_STR2HEX:
    case OPTYPE_STR2INT:
    case OPTYPE_STR2OCT:
    case OPTYPE_UNICHAR2INT:
    case OPTYPE_UNICHAR2CHAR:
    case OPTYPE_ENUM2INT:
    case OPTYPE_RNDWITHVAL:
    case OPTYPE_REMOVE_BOM:
    case OPTYPE_GET_STRINGENCODING:
    case OPTYPE_DECODE_BASE64:
      u.expr.v1->set_fullname(p_fullname+".<operand>");
      break;
    case OPTYPE_HOSTID: // [v1]
      if(u.expr.v1) u.expr.v1->set_fullname(p_fullname+".<operand>");
      break;
    case OPTYPE_ADD: // v1 v2
    case OPTYPE_SUBTRACT:
    case OPTYPE_MULTIPLY:
    case OPTYPE_DIVIDE:
    case OPTYPE_MOD:
    case OPTYPE_REM:
    case OPTYPE_CONCAT:
    case OPTYPE_EQ:
    case OPTYPE_LT:
    case OPTYPE_GT:
    case OPTYPE_NE:
    case OPTYPE_GE:
    case OPTYPE_LE:
    case OPTYPE_AND:
    case OPTYPE_OR:
    case OPTYPE_XOR:
    case OPTYPE_AND4B:
    case OPTYPE_OR4B:
    case OPTYPE_XOR4B:
    case OPTYPE_SHL:
    case OPTYPE_SHR:
    case OPTYPE_ROTL:
    case OPTYPE_ROTR:
    case OPTYPE_INT2BIT:
    case OPTYPE_INT2HEX:
    case OPTYPE_INT2OCT:
      u.expr.v1->set_fullname(p_fullname+".<operand1>");
      u.expr.v2->set_fullname(p_fullname+".<operand2>");
      break;
    case OPTYPE_UNICHAR2OCT:
    case OPTYPE_OCT2UNICHAR:
    case OPTYPE_ENCODE_BASE64:
      u.expr.v1->set_fullname(p_fullname+".<operand1>");
      if(u.expr.v2) u.expr.v2->set_fullname(p_fullname+".<operand2>");
      break;
    case OPTYPE_DECODE:
      u.expr.r1->set_fullname(p_fullname+".<operand1>");
      u.expr.r2->set_fullname(p_fullname+".<operand2>");
      break;
    case OPTYPE_SUBSTR:
      u.expr.ti1->set_fullname(p_fullname+".<operand1>");
      u.expr.v2->set_fullname(p_fullname+".<operand2>");
      u.expr.v3->set_fullname(p_fullname+".<operand3>");
      break;
    case OPTYPE_REGEXP:
      u.expr.ti1->set_fullname(p_fullname+".<operand1>");
      u.expr.t2->set_fullname(p_fullname+".<operand2>");
      u.expr.v3->set_fullname(p_fullname+".<operand3>");
      break;
    case OPTYPE_DECOMP: // v1 v2 v3
      u.expr.v1->set_fullname(p_fullname+".<operand1>");
      u.expr.v2->set_fullname(p_fullname+".<operand2>");
      u.expr.v3->set_fullname(p_fullname+".<operand3>");
      break;
    case OPTYPE_REPLACE:
      u.expr.ti1->set_fullname(p_fullname+".<operand1>");
      u.expr.v2->set_fullname(p_fullname+".<operand2>");
      u.expr.v3->set_fullname(p_fullname+".<operand3>");
      u.expr.ti4->set_fullname(p_fullname+".<operand4>");
      break;
    case OPTYPE_LENGTHOF: // ti1
    case OPTYPE_SIZEOF:   // ti1
    case OPTYPE_VALUEOF: // ti1
    case OPTYPE_ISVALUE:
    case OPTYPE_ISBOUND:
    case OPTYPE_ENCODE:
    case OPTYPE_ISPRESENT:
    case OPTYPE_TTCN2STRING:
      u.expr.ti1->set_fullname(p_fullname+".<operand>");
      break;
    case OPTYPE_ISTEMPLATEKIND: // ti1 v2
      u.expr.ti1->set_fullname(p_fullname+".<operand1>");
      u.expr.v2->set_fullname(p_fullname+".<operand2>");
      break;
    case OPTYPE_ENCVALUE_UNICHAR: // ti1 [v2]
      u.expr.ti1->set_fullname(p_fullname+".<operand1>");
      if (u.expr.v2) u.expr.v2->set_fullname(p_fullname+".<operand2>");
      break;
    case OPTYPE_DECVALUE_UNICHAR: // r1 r2 [v3]
      u.expr.r1->set_fullname(p_fullname+".<operand1>");
      u.expr.r2->set_fullname(p_fullname+".<operand2>");
      if (u.expr.v3) u.expr.v3->set_fullname(p_fullname+".<operand3>");
    case OPTYPE_TMR_READ: // r1
    case OPTYPE_ACTIVATE:
      u.expr.r1->set_fullname(p_fullname+".<operand>");
      break;
    case OPTYPE_EXECUTE: // r1 [v2]
      u.expr.r1->set_fullname(p_fullname+".<operand1>");
      if(u.expr.v2) u.expr.v2->set_fullname(p_fullname+".<operand2>");
      break;
    case OPTYPE_COMP_CREATE: // r1 [v2] [v3] b4
      u.expr.r1->set_fullname(p_fullname+".<operand1>");
      if(u.expr.v2) u.expr.v2->set_fullname(p_fullname+".<operand2>");
      if(u.expr.v3) u.expr.v3->set_fullname(p_fullname+".<operand3>");
      break;
    case OPTYPE_MATCH: // v1 t2
      u.expr.v1->set_fullname(p_fullname+".<operand1>");
      u.expr.t2->set_fullname(p_fullname+".<operand2>");
      break;
    case OPTYPE_ISCHOSEN: // r1 i2
      u.expr.r1->set_fullname(p_fullname+".<operand>");
      break;
    case OPTYPE_ISCHOSEN_V: // v1 i2
      u.expr.v1->set_fullname(p_fullname+".<operand>");
      break;
    case OPTYPE_ISCHOSEN_T: // t1 i2
      u.expr.t1->set_fullname(p_fullname+".<operand>");
      break;
    case OPTYPE_ACTIVATE_REFD:
      u.expr.v1->set_fullname(p_fullname+".<reference>");
      if(u.expr.state!=EXPR_CHECKED)
        u.expr.t_list2->set_fullname(p_fullname+".<parameterlist>");
      else
        u.expr.ap_list2->set_fullname(p_fullname+".<parameterlist>");
      break;
    case OPTYPE_EXECUTE_REFD:
      u.expr.v1->set_fullname(p_fullname+".<reference>");
      if(u.expr.state!=EXPR_CHECKED)
        u.expr.t_list2->set_fullname(p_fullname+".<parameterlist>");
      else
        u.expr.ap_list2->set_fullname(p_fullname+".<parameterlist>");
      if(u.expr.v3)
        u.expr.v3->set_fullname(p_fullname+".<operand3>");
      break;
    case OPTYPE_LOG2STR:
      u.expr.logargs->set_fullname(p_fullname+".<logargs>");
      break;
    case OPTYPE_ANY2UNISTR:
      u.expr.logargs->set_fullname(p_fullname+".<logarg>");
      break;
    case OPTYPE_CHECKSTATE_ANY: // [r1] v2
    case OPTYPE_CHECKSTATE_ALL:
      u.expr.v2->set_fullname(p_fullname+".<operand1>");
      break;
    case OPTYPE_UNDEF_RUNNING: // r1 [r2] b4
    case OPTYPE_TMR_RUNNING:
      u.expr.r1->set_fullname(p_fullname+".<operand>");
      if (u.expr.r2 != NULL) {
        u.expr.r2->set_fullname(p_fullname+".redirindex");
      }
      break;
    case OPTYPE_COMP_RUNNING: // v1 [r2] b4
    case OPTYPE_COMP_ALIVE:
      u.expr.v1->set_fullname(p_fullname+".<operand>");
      if (u.expr.r2 != NULL) {
        u.expr.r2->set_fullname(p_fullname+".redirindex");
      }
      break;
    default:
      FATAL_ERROR("Value::set_fullname_expr()");
    } // switch
  }

  void Value::set_my_scope_expr(Scope *p_scope)
  {
    switch (u.expr.v_optype) {
    case OPTYPE_RND: // -
    case OPTYPE_COMP_NULL:
    case OPTYPE_COMP_MTC:
    case OPTYPE_COMP_SYSTEM:
    case OPTYPE_COMP_SELF:
    case OPTYPE_COMP_RUNNING_ANY:
    case OPTYPE_COMP_RUNNING_ALL:
    case OPTYPE_COMP_ALIVE_ANY:
    case OPTYPE_COMP_ALIVE_ALL:
    case OPTYPE_TMR_RUNNING_ANY:
    case OPTYPE_GETVERDICT:
    case OPTYPE_TESTCASENAME:
    case OPTYPE_PROF_RUNNING:
      break;
    case OPTYPE_COMP_RUNNING: // v1 [r2] b4
    case OPTYPE_COMP_ALIVE:
      if (u.expr.r2 != NULL) {
        u.expr.r2->set_my_scope(p_scope);
      }
      // no break
    case OPTYPE_UNARYPLUS: // v1
    case OPTYPE_UNARYMINUS:
    case OPTYPE_NOT:
    case OPTYPE_NOT4B:
    case OPTYPE_BIT2HEX:
    case OPTYPE_BIT2INT:
    case OPTYPE_BIT2OCT:
    case OPTYPE_BIT2STR:
    case OPTYPE_CHAR2INT:
    case OPTYPE_CHAR2OCT:
    case OPTYPE_FLOAT2INT:
    case OPTYPE_FLOAT2STR:
    case OPTYPE_HEX2BIT:
    case OPTYPE_HEX2INT:
    case OPTYPE_HEX2OCT:
    case OPTYPE_HEX2STR:
    case OPTYPE_INT2CHAR:
    case OPTYPE_INT2FLOAT:
    case OPTYPE_INT2STR:
    case OPTYPE_INT2UNICHAR:
    case OPTYPE_OCT2BIT:
    case OPTYPE_OCT2CHAR:
    case OPTYPE_OCT2HEX:
    case OPTYPE_OCT2INT:
    case OPTYPE_OCT2STR:
    case OPTYPE_STR2BIT:
    case OPTYPE_STR2FLOAT:
    case OPTYPE_STR2HEX:
    case OPTYPE_STR2INT:
    case OPTYPE_STR2OCT:
    case OPTYPE_UNICHAR2INT:
    case OPTYPE_UNICHAR2CHAR:
    case OPTYPE_ENUM2INT:
    case OPTYPE_RNDWITHVAL:
    case OPTYPE_REMOVE_BOM:
    case OPTYPE_GET_STRINGENCODING:
    case OPTYPE_DECODE_BASE64:
      u.expr.v1->set_my_scope(p_scope);
      break;
    case OPTYPE_HOSTID: // [v1]
      if(u.expr.v1) u.expr.v1->set_my_scope(p_scope);
      break;
    case OPTYPE_ADD: // v1 v2
    case OPTYPE_SUBTRACT:
    case OPTYPE_MULTIPLY:
    case OPTYPE_DIVIDE:
    case OPTYPE_MOD:
    case OPTYPE_REM:
    case OPTYPE_CONCAT:
    case OPTYPE_EQ:
    case OPTYPE_LT:
    case OPTYPE_GT:
    case OPTYPE_NE:
    case OPTYPE_GE:
    case OPTYPE_LE:
    case OPTYPE_AND:
    case OPTYPE_OR:
    case OPTYPE_XOR:
    case OPTYPE_AND4B:
    case OPTYPE_OR4B:
    case OPTYPE_XOR4B:
    case OPTYPE_SHL:
    case OPTYPE_SHR:
    case OPTYPE_ROTL:
    case OPTYPE_ROTR:
    case OPTYPE_INT2BIT:
    case OPTYPE_INT2HEX:
    case OPTYPE_INT2OCT:
      u.expr.v1->set_my_scope(p_scope);
      u.expr.v2->set_my_scope(p_scope);
      break;
    case OPTYPE_UNICHAR2OCT:
    case OPTYPE_OCT2UNICHAR:
    case OPTYPE_ENCODE_BASE64:
      u.expr.v1->set_my_scope(p_scope);
      if(u.expr.v2) u.expr.v2->set_my_scope(p_scope);
      break;
    case OPTYPE_DECODE: // r1 r2
    case OPTYPE_UNDEF_RUNNING: // r1 [r2] b4
    case OPTYPE_TMR_RUNNING: // r1 [r2] b4
      u.expr.r1->set_my_scope(p_scope);
      if (u.expr.r2 != NULL) {
        u.expr.r2->set_my_scope(p_scope);
      }
      break;
    case OPTYPE_SUBSTR:
      u.expr.ti1->set_my_scope(p_scope);
      u.expr.v2->set_my_scope(p_scope);
      u.expr.v3->set_my_scope(p_scope);
      break;
    case OPTYPE_REGEXP:
      u.expr.ti1->set_my_scope(p_scope);
      u.expr.t2->set_my_scope(p_scope);
      u.expr.v3->set_my_scope(p_scope);
      break;
    case OPTYPE_DECOMP: // v1 v2 v3
      u.expr.v1->set_my_scope(p_scope);
      u.expr.v2->set_my_scope(p_scope);
      u.expr.v3->set_my_scope(p_scope);
      break;
    case OPTYPE_REPLACE:
      u.expr.ti1->set_my_scope(p_scope);
      u.expr.v2->set_my_scope(p_scope);
      u.expr.v3->set_my_scope(p_scope);
      u.expr.ti4->set_my_scope(p_scope);
      break;
    case OPTYPE_LENGTHOF: // ti1
    case OPTYPE_SIZEOF: // ti1
    case OPTYPE_VALUEOF: // ti1
    case OPTYPE_ISVALUE:
    case OPTYPE_ISBOUND:
    case OPTYPE_ENCODE:
    case OPTYPE_ISPRESENT:
    case OPTYPE_TTCN2STRING:
      u.expr.ti1->set_my_scope(p_scope);
      break;
    case OPTYPE_ISTEMPLATEKIND: // ti1 v2
      u.expr.ti1->set_my_scope(p_scope);
      u.expr.v2->set_my_scope(p_scope);
      break;
    case OPTYPE_ENCVALUE_UNICHAR: //ti1 [v2]
      u.expr.ti1->set_my_scope(p_scope);
      if(u.expr.v2) u.expr.v2->set_my_scope(p_scope);
      break;
    case OPTYPE_DECVALUE_UNICHAR: // r1 r2 [v3]
      u.expr.r1->set_my_scope(p_scope);
      u.expr.r2->set_my_scope(p_scope);
      if(u.expr.v3) u.expr.v3->set_my_scope(p_scope);
      break;
    case OPTYPE_TMR_READ: // r1
    case OPTYPE_ACTIVATE:
      u.expr.r1->set_my_scope(p_scope);
      break;
    case OPTYPE_EXECUTE: // r1 [v2]
      u.expr.r1->set_my_scope(p_scope);
      if(u.expr.v2) u.expr.v2->set_my_scope(p_scope);
      break;
    case OPTYPE_CHECKSTATE_ANY: // [r1] v2
    case OPTYPE_CHECKSTATE_ALL:
      if(u.expr.r1) u.expr.r1->set_my_scope(p_scope);
      u.expr.v2->set_my_scope(p_scope);
      break;
    case OPTYPE_COMP_CREATE: // r1 [v2] [v3]
      u.expr.r1->set_my_scope(p_scope);
      if(u.expr.v2) u.expr.v2->set_my_scope(p_scope);
      if(u.expr.v3) u.expr.v3->set_my_scope(p_scope);
      break;
    case OPTYPE_MATCH: // v1 t2
      u.expr.v1->set_my_scope(p_scope);
      u.expr.t2->set_my_scope(p_scope);
      break;
    case OPTYPE_ISCHOSEN: // r1 i2
      u.expr.r1->set_my_scope(p_scope);
      break;
    case OPTYPE_ISCHOSEN_V: // v1 i2
      u.expr.v1->set_my_scope(p_scope);
      break;
    case OPTYPE_ISCHOSEN_T: // t1 i2
      u.expr.t1->set_my_scope(p_scope);
      break;
    case OPTYPE_ACTIVATE_REFD:
      u.expr.v1->set_my_scope(p_scope);
      if(u.expr.state!=EXPR_CHECKED) {
	if(u.expr.t_list2) u.expr.t_list2->set_my_scope(p_scope);
	else
	  if(u.expr.ap_list2) u.expr.ap_list2->set_my_scope(p_scope);
      } break;
    case OPTYPE_EXECUTE_REFD:
      u.expr.v1->set_my_scope(p_scope);
      if(u.expr.state!=EXPR_CHECKED) {
	if(u.expr.t_list2) u.expr.t_list2->set_my_scope(p_scope);
	else
	  if(u.expr.ap_list2) u.expr.ap_list2->set_my_scope(p_scope);
      }
      if(u.expr.v3)
	u.expr.v3->set_my_scope(p_scope);
      break;
    case OPTYPE_LOG2STR:
    case OPTYPE_ANY2UNISTR:
      u.expr.logargs->set_my_scope(p_scope);
      break;
    default:
      FATAL_ERROR("Value::set_my_scope_expr()");
    } // switch
  }

  void Value::set_genname_recursive(const string& p_genname)
  {
    size_t genname_len = p_genname.size();
    if (genname_len >= 4 &&
	p_genname.find("()()", genname_len - 4) == genname_len - 4) {
      // if the genname ends with ()() (i.e. the value stands for an optional
      // field) then drop the last () from the own genname, but leave it for
      // the embedded values
      set_genname(p_genname.substr(0, genname_len - 2));
    } else set_genname(p_genname);
    switch(valuetype) {
    case V_CHOICE: {
      string embedded_genname(p_genname);
      embedded_genname += '.';
      // If this is a choice value for an anytype, prepend the AT_ prefix
      // to the name of the alternative. The genname is used later in
      // Common::Value::generate_code_init_se()
      if (my_governor->get_type_refd_last()->get_typetype()==Type::T_ANYTYPE)
        embedded_genname += "AT_";
      embedded_genname += u.choice.alt_name->get_name();
      embedded_genname += "()";
      u.choice.alt_value->set_genname_recursive(embedded_genname);
      break; }
    case V_SEQOF:
    case V_SETOF: {
      if (!is_indexed()) {
        size_t nof_vs = u.val_vs->get_nof_vs();
        for (size_t i = 0; i < nof_vs; i++) {
          string embedded_genname(p_genname);
          embedded_genname += '[';
          embedded_genname += Int2string(i);
          embedded_genname += ']';
          u.val_vs->get_v_byIndex(i)->set_genname_recursive(embedded_genname);
        }
      } else {
        size_t nof_ivs = u.val_vs->get_nof_ivs();
        for (size_t i = 0; i < nof_ivs; i++) {
          string embedded_genname(p_genname);
          embedded_genname += '[';
          embedded_genname += Int2string(i);
          embedded_genname += ']';
          u.val_vs->get_iv_byIndex(i)->get_value()
            ->set_genname_recursive(embedded_genname);
        }
      }
      break; }
    case V_ARRAY: {
      if (!my_governor) return; // error recovery
      Type *type = my_governor->get_type_refd_last();
      if (type->get_typetype() != Type::T_ARRAY) return; // error recovery
      Int offset = type->get_dimension()->get_offset();
      if (!is_indexed()) {
        size_t nof_vs = u.val_vs->get_nof_vs();
        for (size_t i = 0; i < nof_vs; i++) {
          string embedded_genname(p_genname);
          embedded_genname += '[';
          embedded_genname += Int2string(offset + i);
          embedded_genname += ']';
          u.val_vs->get_v_byIndex(i)->set_genname_recursive(embedded_genname);
        }
      } else {
        size_t nof_ivs = u.val_vs->get_nof_ivs();
        for (size_t i = 0; i < nof_ivs; i++) {
          string embedded_genname(p_genname);
          embedded_genname += '[';
          embedded_genname += Int2string(offset + i);
          embedded_genname += ']';
          u.val_vs->get_iv_byIndex(i)->get_value()
            ->set_genname_recursive(embedded_genname);
        }
      }
      break; }
    case V_SEQ:
    case V_SET: {
      if (!my_governor) return; // error recovery
      Type *t = my_governor->get_type_refd_last();
      if (!t->is_secho()) return; // error recovery
      size_t nof_nvs = u.val_nvs->get_nof_nvs();
      for (size_t i = 0; i < nof_nvs; i++) {
	NamedValue *nv = u.val_nvs->get_nv_byIndex(i);
	const Identifier& id = nv->get_name();
	if (!t->has_comp_withName(id)) return; // error recovery
	string embedded_genname(p_genname);
	embedded_genname += '.';
	embedded_genname += id.get_name();
	embedded_genname += "()";
	if (t->get_comp_byName(id)->get_is_optional())
	  embedded_genname += "()";
	nv->get_value()->set_genname_recursive(embedded_genname);
      }
      break; }
    default:
      break;
    } // switch
  }

  void Value::set_genname_prefix(const char *p_genname_prefix)
  {
    GovernedSimple::set_genname_prefix(p_genname_prefix);
    switch(valuetype) {
    case V_CHOICE:
      u.choice.alt_value->set_genname_prefix(p_genname_prefix);
      break;
    case V_SEQOF:
    case V_SETOF:
    case V_ARRAY:
      if (!is_indexed()) {
        for (size_t i = 0; i < u.val_vs->get_nof_vs(); i++)
          u.val_vs->get_v_byIndex(i)->set_genname_prefix(p_genname_prefix);
      } else {
        for (size_t i = 0; i < u.val_vs->get_nof_ivs(); i++)
          u.val_vs->get_iv_byIndex(i)->get_value()
            ->set_genname_prefix(p_genname_prefix);
      }
      break;
    case V_SEQ:
    case V_SET:
      for (size_t i = 0; i < u.val_nvs->get_nof_nvs(); i++)
        u.val_nvs->get_nv_byIndex(i)->get_value()
	  ->set_genname_prefix(p_genname_prefix);
      break;
    default:
      break;
    } // switch
  }

  void Value::set_code_section(code_section_t p_code_section)
  {
    GovernedSimple::set_code_section(p_code_section);
    switch(valuetype) {
    case V_EXPR:
      switch (u.expr.v_optype) {
      case OPTYPE_RND: // -
      case OPTYPE_COMP_NULL:
      case OPTYPE_COMP_MTC:
      case OPTYPE_COMP_SYSTEM:
      case OPTYPE_COMP_SELF:
      case OPTYPE_COMP_RUNNING_ANY:
      case OPTYPE_COMP_RUNNING_ALL:
      case OPTYPE_COMP_ALIVE_ANY:
      case OPTYPE_COMP_ALIVE_ALL:
      case OPTYPE_TMR_RUNNING_ANY:
      case OPTYPE_GETVERDICT:
      case OPTYPE_TESTCASENAME:
      case OPTYPE_PROF_RUNNING:
        break;
      case OPTYPE_COMP_RUNNING: // v1 [r2] b4
      case OPTYPE_COMP_ALIVE:
        if (u.expr.r2 != NULL) {
          u.expr.r2->set_code_section(p_code_section);
        }
        // no break
      case OPTYPE_UNARYPLUS: // v1
      case OPTYPE_UNARYMINUS:
      case OPTYPE_NOT:
      case OPTYPE_NOT4B:
      case OPTYPE_BIT2HEX:
      case OPTYPE_BIT2INT:
      case OPTYPE_BIT2OCT:
      case OPTYPE_BIT2STR:
      case OPTYPE_CHAR2INT:
      case OPTYPE_CHAR2OCT:
      case OPTYPE_FLOAT2INT:
      case OPTYPE_FLOAT2STR:
      case OPTYPE_HEX2BIT:
      case OPTYPE_HEX2INT:
      case OPTYPE_HEX2OCT:
      case OPTYPE_HEX2STR:
      case OPTYPE_INT2CHAR:
      case OPTYPE_INT2FLOAT:
      case OPTYPE_INT2STR:
      case OPTYPE_INT2UNICHAR:
      case OPTYPE_OCT2BIT:
      case OPTYPE_OCT2CHAR:
      case OPTYPE_OCT2HEX:
      case OPTYPE_OCT2INT:
      case OPTYPE_OCT2STR:
      case OPTYPE_STR2BIT:
      case OPTYPE_STR2FLOAT:
      case OPTYPE_STR2HEX:
      case OPTYPE_STR2INT:
      case OPTYPE_STR2OCT:
      case OPTYPE_UNICHAR2INT:
      case OPTYPE_UNICHAR2CHAR:
      case OPTYPE_ENUM2INT:
      case OPTYPE_RNDWITHVAL:
      case OPTYPE_GET_STRINGENCODING:
      case OPTYPE_DECODE_BASE64:
      case OPTYPE_REMOVE_BOM:
        u.expr.v1->set_code_section(p_code_section);
        break;
      case OPTYPE_HOSTID: // [v1]
        if(u.expr.v1) u.expr.v1->set_code_section(p_code_section);
        break;
      case OPTYPE_ADD: // v1 v2
      case OPTYPE_SUBTRACT:
      case OPTYPE_MULTIPLY:
      case OPTYPE_DIVIDE:
      case OPTYPE_MOD:
      case OPTYPE_REM:
      case OPTYPE_CONCAT:
      case OPTYPE_EQ:
      case OPTYPE_LT:
      case OPTYPE_GT:
      case OPTYPE_NE:
      case OPTYPE_GE:
      case OPTYPE_LE:
      case OPTYPE_AND:
      case OPTYPE_OR:
      case OPTYPE_XOR:
      case OPTYPE_AND4B:
      case OPTYPE_OR4B:
      case OPTYPE_XOR4B:
      case OPTYPE_SHL:
      case OPTYPE_SHR:
      case OPTYPE_ROTL:
      case OPTYPE_ROTR:
      case OPTYPE_INT2BIT:
      case OPTYPE_INT2HEX:
      case OPTYPE_INT2OCT:
        u.expr.v1->set_code_section(p_code_section);
        u.expr.v2->set_code_section(p_code_section);
        break;
      case OPTYPE_UNICHAR2OCT:
      case OPTYPE_OCT2UNICHAR:
      case OPTYPE_ENCODE_BASE64:
        u.expr.v1->set_code_section(p_code_section);
        if(u.expr.v2) u.expr.v2->set_code_section(p_code_section);
        break;
      case OPTYPE_DECODE: // r1 r2
      case OPTYPE_UNDEF_RUNNING: // r1 [r2] b4
      case OPTYPE_TMR_RUNNING: // r1 [r2] b4
        u.expr.r1->set_code_section(p_code_section);
        if (u.expr.r2 != NULL) {
          u.expr.r2->set_code_section(p_code_section);
        }
        break;
      case OPTYPE_SUBSTR:
        u.expr.ti1->set_code_section(p_code_section);
        u.expr.v2->set_code_section(p_code_section);
        u.expr.v3->set_code_section(p_code_section);
        break;
      case OPTYPE_REGEXP:
        u.expr.ti1->set_code_section(p_code_section);
        u.expr.t2->set_code_section(p_code_section);
        u.expr.v3->set_code_section(p_code_section);
        break;
      case OPTYPE_DECOMP: // v1 v2 v3
        u.expr.v1->set_code_section(p_code_section);
        u.expr.v2->set_code_section(p_code_section);
        u.expr.v3->set_code_section(p_code_section);
        break;
      case OPTYPE_REPLACE:
        u.expr.ti1->set_code_section(p_code_section);
        u.expr.v2->set_code_section(p_code_section);
        u.expr.v3->set_code_section(p_code_section);
        u.expr.ti4->set_code_section(p_code_section);
        break;
      case OPTYPE_LENGTHOF: // ti1
      case OPTYPE_SIZEOF: // ti1
      case OPTYPE_VALUEOF: // ti1
      case OPTYPE_ISVALUE:
      case OPTYPE_ISBOUND:
      case OPTYPE_ENCODE:
      case OPTYPE_ISPRESENT:
      case OPTYPE_TTCN2STRING:
        u.expr.ti1->set_code_section(p_code_section);
        break;
      case OPTYPE_ISTEMPLATEKIND: // ti1 v2
        u.expr.ti1->set_code_section(p_code_section);
        u.expr.v2->set_code_section(p_code_section);
        break;
      case OPTYPE_ENCVALUE_UNICHAR: // ti1 [v2]
        u.expr.ti1->set_code_section(p_code_section);
        if (u.expr.v2) u.expr.v2->set_code_section(p_code_section);
        break;
      case OPTYPE_DECVALUE_UNICHAR: // r1 r2 [v3]
        u.expr.r1->set_code_section(p_code_section);
        u.expr.r2->set_code_section(p_code_section);
        if (u.expr.v3) u.expr.v3->set_code_section(p_code_section);
        break;
      case OPTYPE_TMR_READ: // r1
      case OPTYPE_ACTIVATE:
        u.expr.r1->set_code_section(p_code_section);
        break;
      case OPTYPE_EXECUTE: // r1 [v2]
        u.expr.r1->set_code_section(p_code_section);
        if(u.expr.v2) u.expr.v2->set_code_section(p_code_section);
        break;
      case OPTYPE_CHECKSTATE_ANY: // [r1] v2
      case OPTYPE_CHECKSTATE_ALL:
        if(u.expr.r1) u.expr.r1->set_code_section(p_code_section);
        u.expr.v2->set_code_section(p_code_section);
        break;
      case OPTYPE_COMP_CREATE: // r1 [v2] [v3] b4
        u.expr.r1->set_code_section(p_code_section);
        if(u.expr.v2) u.expr.v2->set_code_section(p_code_section);
        if(u.expr.v3) u.expr.v3->set_code_section(p_code_section);
        break;
      case OPTYPE_MATCH: // v1 t2
        u.expr.v1->set_code_section(p_code_section);
        u.expr.t2->set_code_section(p_code_section);
        break;
      case OPTYPE_ISCHOSEN: // r1 i2
        u.expr.r1->set_code_section(p_code_section);
        break;
      case OPTYPE_ISCHOSEN_V: // v1 i2
        u.expr.v1->set_code_section(p_code_section);
        break;
      case OPTYPE_ISCHOSEN_T: // t1 i2
        u.expr.t1->set_code_section(p_code_section);
        break;
      case OPTYPE_ACTIVATE_REFD:
        u.expr.v1->set_code_section(p_code_section);
        if(u.expr.state!=EXPR_CHECKED)
          u.expr.t_list2->set_code_section(p_code_section);
        else {
          for(size_t i = 0; i < u.expr.ap_list2->get_nof_pars(); i++)
            u.expr.ap_list2->get_par(i)->set_code_section(p_code_section);
          u.expr.state = EXPR_CHECKED;
          }
        break;
      case OPTYPE_EXECUTE_REFD:
        u.expr.v1->set_code_section(p_code_section);
        if(u.expr.state!=EXPR_CHECKED)
          u.expr.t_list2->set_code_section(p_code_section);
        else {
          for(size_t i = 0; i < u.expr.ap_list2->get_nof_pars(); i++)
            u.expr.ap_list2->get_par(i)->set_code_section(p_code_section);
          u.expr.state = EXPR_CHECKED;
          }
        if(u.expr.v3)
          u.expr.v3->set_code_section(p_code_section);
      break;
      case OPTYPE_LOG2STR:
      case OPTYPE_ANY2UNISTR:
        u.expr.logargs->set_code_section(p_code_section);
        break;
      default:
        FATAL_ERROR("Value::set_code_section()");
      } // switch
      break;
    case V_CHOICE:
      u.choice.alt_value->set_code_section(p_code_section);
      break;
    case V_SEQOF:
    case V_SETOF:
    case V_ARRAY:
      if (!is_indexed()) {
        for (size_t i = 0; i < u.val_vs->get_nof_vs(); i++)
          u.val_vs->get_v_byIndex(i)->set_code_section(p_code_section);
      } else {
        for (size_t i = 0; i < u.val_vs->get_nof_ivs(); i++)
          u.val_vs->get_iv_byIndex(i)->set_code_section(p_code_section);
      }
      break;
    case V_SEQ:
    case V_SET:
      for (size_t i = 0; i < u.val_nvs->get_nof_nvs(); i++)
        u.val_nvs->get_nv_byIndex(i)->get_value()
	  ->set_code_section(p_code_section);
      break;
    case V_REFD:
      u.ref.ref->set_code_section(p_code_section);
      break;
    case V_REFER:
      u.refered->set_code_section(p_code_section);
      break;
    case V_INVOKE:
      u.invoke.v->set_code_section(p_code_section);
      if(u.invoke.t_list) u.invoke.t_list->set_code_section(p_code_section);
      if(u.invoke.ap_list)
        for(size_t i = 0; i < u.invoke.ap_list->get_nof_pars(); i++)
            u.invoke.ap_list->get_par(i)->set_code_section(p_code_section);
      break;
    default:
      break;
    } // switch
  }

  void Value::change_sign()
  {
    switch(valuetype) {
    case V_INT:
      *u.val_Int=-*u.val_Int;
      break;
    case V_REAL:
      u.val_Real*=-1.0;
      break;
    case V_ERROR:
      break;
    default:
      FATAL_ERROR("Value::change_sign()");
    } // switch
  }

  void Value::add_oid_comp(OID_comp* p_comp)
  {
    if(!p_comp)
      FATAL_ERROR("NULL parameter");
    u.oid_comps->add(p_comp);
    p_comp->set_fullname(get_fullname()+"."
                         +Int2string(u.oid_comps->size()));
    p_comp->set_my_scope(my_scope);
  }

  void Value::set_valuetype(valuetype_t p_valuetype)
  {
    if (valuetype == V_ERROR) return;
    else if (p_valuetype == V_ERROR) {
      if(valuetype==V_EXPR) {
        switch(u.expr.state) {
        case EXPR_CHECKING:
          u.expr.state=EXPR_CHECKING_ERR;
          // no break
        case EXPR_CHECKING_ERR:
          return;
        default:
          break;
        }
      }
      clean_up();
      valuetype = V_ERROR;
      return;
    }
    switch(valuetype) {
    case V_UNDEF_LOWERID:
      switch(p_valuetype) {
      case V_ENUM:
      case V_NAMEDINT:
        break;
      case V_REFD:
        if (is_asn1()) u.ref.ref = new Asn::Ref_defd_simple(0, u.val_id);
	else u.ref.ref = new Ttcn::Reference(0, u.val_id);
        u.ref.ref->set_my_scope(get_my_scope());
	u.ref.ref->set_fullname(get_fullname());
        u.ref.ref->set_location(*this);
        u.ref.refd_last = 0;
        break;
      default:
        FATAL_ERROR("Value::set_valuetype()");
      } // switch
      break;
    case V_UNDEF_BLOCK: {
      Block *t_block=u.block;
      Value *v=0;
      switch(p_valuetype) {
      case V_NAMEDBITS: {
        Node *node=t_block->parse(KW_Block_IdentifierList);
        v=dynamic_cast<Value*>(node);
        if(!v) {
          /* syntax error */
          u.ids=new map<string, Identifier>();
        }
        else {
          u.ids=v->u.ids; v->u.ids=0;
        }
        break;}
      case V_SEQOF: {
        Node *node=t_block->parse(KW_Block_SeqOfValue);
        v=dynamic_cast<Value*>(node);
        if(!v) {
          /* syntax error */
          u.val_vs=new Values();
        }
        else {
          u.val_vs=v->u.val_vs; v->u.val_vs=0;
        }
        u.val_vs->set_my_scope(get_my_scope());
	u.val_vs->set_fullname(get_fullname());
        break;}
      case V_SETOF: {
        Node *node=t_block->parse(KW_Block_SetOfValue);
        v=dynamic_cast<Value*>(node);
        if(!v) {
          /* syntax error */
          u.val_vs=new Values();
        }
        else {
          u.val_vs=v->u.val_vs; v->u.val_vs=0;
        }
        u.val_vs->set_my_scope(get_my_scope());
	u.val_vs->set_fullname(get_fullname());
        break;}
      case V_SEQ: {
        Node *node=t_block->parse(KW_Block_SequenceValue);
        v=dynamic_cast<Value*>(node);
        if(!v) {
          /* syntax error */
          u.val_nvs=new NamedValues();
        }
        else {
          u.val_nvs=v->u.val_nvs; v->u.val_nvs=0;
        }
        u.val_nvs->set_my_scope(get_my_scope());
	u.val_nvs->set_fullname(get_fullname());
        break;}
      case V_SET: {
        Node *node=t_block->parse(KW_Block_SetValue);
        v=dynamic_cast<Value*>(node);
        if(!v) {
          /* syntax error */
          u.val_nvs=new NamedValues();
        }
        else {
          u.val_nvs=v->u.val_nvs; v->u.val_nvs=0;
        }
        u.val_nvs->set_my_scope(get_my_scope());
	u.val_nvs->set_fullname(get_fullname());
        break;}
      case V_OID: {
        Node *node=t_block->parse(KW_Block_OIDValue);
        v=dynamic_cast<Value*>(node);
        if(!v) {
          /* syntax error */
          u.oid_comps=new vector<OID_comp>();
        }
        else {
          u.oid_comps=v->u.oid_comps; v->u.oid_comps=0;
        }
        for (size_t i = 0; i < u.oid_comps->size(); i++)
          (*u.oid_comps)[i]->set_my_scope(get_my_scope());
        break;}
      case V_ROID: {
        Node *node=t_block->parse(KW_Block_ROIDValue);
        v=dynamic_cast<Value*>(node);
        if(!v) {
          /* syntax error */
          u.oid_comps=new vector<OID_comp>();
        }
        else {
          u.oid_comps=v->u.oid_comps; v->u.oid_comps=0;
        }
        for (size_t i = 0; i < u.oid_comps->size(); i++)
          (*u.oid_comps)[i]->set_my_scope(get_my_scope());
        break;}
      case V_CHARSYMS: {
        Node *node=t_block->parse(KW_Block_CharStringValue);
        u.char_syms=dynamic_cast<CharSyms*>(node);
        if(!u.char_syms) {
          /* syntax error */
          u.char_syms=new CharSyms();
        }
        u.char_syms->set_my_scope(get_my_scope());
        u.char_syms->set_fullname(get_fullname());
        break;}
      default:
        FATAL_ERROR("Value::set_valuetype()");
      } // switch
      delete v;
      delete t_block;
      break;}
    case V_REFD:
      if (p_valuetype == V_USTR) {
	Value *v_last = get_value_refd_last();
	if (v_last->valuetype != V_CSTR) FATAL_ERROR("Value::set_valuetype()");
	ustring *ustr = new ustring(*v_last->u.str.val_str);
	delete u.ref.ref;
	set_val_ustr(ustr);
	u.ustr.convert_str = true; // will be converted back to string
      } else FATAL_ERROR("Value::set_valuetype()");
      break;
    case V_CHARSYMS:
      switch(p_valuetype) {
      case V_CSTR: {
        const string& str = u.char_syms->get_string();
        delete u.char_syms;
        set_val_str(new string(str));
        break;}
      case V_USTR: {
        const ustring& ustr = u.char_syms->get_ustring();
        delete u.char_syms;
        set_val_ustr(new ustring(ustr));
        u.ustr.convert_str = false;
        break;}
      case V_ISO2022STR: {
        const string& str = u.char_syms->get_iso2022string();
        delete u.char_syms;
        set_val_str(new string(str));
        break;}
      default:
        FATAL_ERROR("Value::set_valuetype()");
      } // switch
      break;
    case V_INT: {
      Real val_Real;
      if (p_valuetype == V_REAL)
        val_Real = u.val_Int->to_real();
      else FATAL_ERROR("Value::set_valuetype()");
      clean_up();
      u.val_Real = val_Real;
      break; }
    case V_HSTR: {
      clean_up_string_elements(u.str.str_elements);
      string *old_str = u.str.val_str;
      switch(p_valuetype) {
      case V_BSTR:
        set_val_str(hex2bit(*old_str));
        break;
      case V_OSTR:
        set_val_str(asn_hex2oct(*old_str));
        break;
      default:
        FATAL_ERROR("Value::set_valuetype()");
      } // switch
      delete old_str;
      break;}
    case V_BSTR:
      clean_up_string_elements(u.str.str_elements);
      if (p_valuetype == V_OSTR) {
        string *old_str = u.str.val_str;
        set_val_str(asn_bit2oct(*old_str));
	delete old_str;
      } else FATAL_ERROR("Value::set_valuetype()");
      break;
    case V_CSTR:
      clean_up_string_elements(u.str.str_elements);
      switch(p_valuetype) {
      case V_USTR: {
        string *old_str = u.str.val_str;
        set_val_ustr(new ustring(*old_str));
        u.ustr.convert_str = true; // will be converted back to string
        delete old_str;
        break;}
      case V_ISO2022STR:
        // do nothing
        break;
      default:
        FATAL_ERROR("Value::set_valuetype()");
      } // switch p_valuetype
      break;
    case V_USTR:
      clean_up_string_elements(u.ustr.ustr_elements);
      switch(p_valuetype) {
      case V_CSTR: {
	ustring *old_str = u.ustr.val_ustr;
	size_t nof_chars = old_str->size();
	bool warning_flag = false;
	for (size_t i = 0; i < nof_chars; i++) {
	  const ustring::universal_char& uchar = (*old_str)[i];
	  if (uchar.group != 0 || uchar.plane != 0 || uchar.row != 0) {
	    error("This string value cannot contain multiple-byte characters, "
	      "but it has quadruple char(%u, %u, %u, %u) at index %lu",
	      uchar.group, uchar.plane, uchar.row, uchar.cell,
              (unsigned long) i);
	    p_valuetype = V_ERROR;
	    break;
	  } else if (uchar.cell > 127 && !warning_flag) {
	    warning("This string value may not contain characters with code "
	      "higher than 127, but it has character with code %u (0x%02X) "
	      "at index %lu", uchar.cell, uchar.cell, (unsigned long) i);
	      warning_flag = true;
	  }
	}
	if (p_valuetype != V_ERROR) set_val_str(new string(*old_str));
        delete old_str;
        break; }
      case V_ISO2022STR:
        error("ISO-10646 string value cannot be converted to "
              "ISO-2022 string");
        delete u.ustr.val_ustr;
        p_valuetype = V_ERROR;
        break;
      default:
        FATAL_ERROR("Value::set_valuetype()");
      } // switch p_valuetype
      break;
    case V_SEQ:
      switch (p_valuetype) {
      case V_CHOICE: {
        NamedValues *nvs = u.val_nvs;
        if (nvs->get_nof_nvs() < 1) {
          error("Union value must have one active field");
          delete nvs;
          valuetype = V_ERROR;
          return;
        } else if (nvs->get_nof_nvs() > 1) {
          error("Only one field was expected in union value instead of %lu",
                (unsigned long) nvs->get_nof_nvs());
        }
        NamedValue *nv = nvs->get_nv_byIndex(0);
        u.choice.alt_name = nv->get_name().clone();
        u.choice.alt_value = nv->steal_value();
        delete nvs;
        break;}
      case V_SET:
        // do nothing
        break;
      case V_REAL: {
        NamedValues *nvs = u.val_nvs;
        bool err = false;
        /* mantissa */
        Int i_mant = 0;
        Identifier id_mant(Identifier::ID_ASN, string("mantissa"));
        if (nvs->has_nv_withName(id_mant)) {
          Value *v_tmp = nvs->get_nv_byName(id_mant)->get_value()
            ->get_value_refd_last();
          if (v_tmp->get_valuetype() == V_INT) {
            const int_val_t *i_mant_int = v_tmp->get_val_Int();
            if (*i_mant_int > INT_MAX) {
              error("Mantissa `%s' should be less than `%d'",
                (i_mant_int->t_str()).c_str(), INT_MAX);
              err = true;
            } else {
              i_mant = i_mant_int->get_val();
            }
          }
          else err = true;
        }
        else err = true;
        /* base */
        Int i_base = 0;
        Identifier id_base(Identifier::ID_ASN, string("base"));
        if (!err && nvs->has_nv_withName(id_base)) {
          Value *v = nvs->get_nv_byName(id_base)->get_value();
          Value *v_tmp = v->get_value_refd_last();
          if (v_tmp->get_valuetype() == V_INT) {
            const int_val_t *i_base_int = v_tmp->get_val_Int();
            if (!err && *i_base_int != 10 && *i_base_int != 2) {
              v->error("Base of the REAL must be 2 or 10");
              err = true;
            } else {
              i_base = i_base_int->get_val();
            }
          }
          else err = true;
        }
        else err = true;
        /* exponent */
        Int i_exp = 0;
        Identifier id_exp(Identifier::ID_ASN, string("exponent"));
        if (!err && nvs->has_nv_withName(id_exp)) {
          Value *v_tmp = nvs->get_nv_byName(id_exp)->get_value()
            ->get_value_refd_last();
          if (v_tmp->get_valuetype() == V_INT) {
            const int_val_t *i_exp_int = v_tmp->get_val_Int();
            if (*i_exp_int > INT_MAX) {
              error("Exponent `%s' should be less than `%d'",
                (i_exp_int->t_str()).c_str(), INT_MAX);
              err = true;
            } else {
              i_exp = i_exp_int->get_val();
            }
          }
          else err = true;
        }
        else err = true;
        /* clean up */
        delete nvs;
        if (err) {
          valuetype = V_ERROR;
          return;
        }
        u.val_Real = i_mant * pow(static_cast<double>(i_base),
	  static_cast<double>(i_exp));
        break; }
      case V_NOTUSED:
        clean_up();
        break;
      default:
        FATAL_ERROR("Value::set_valuetype()");
      } // switch
      break;
    case V_SEQOF:
      switch (p_valuetype) {
      case V_SEQ: {
        // SEQOF -> SEQ: value list notation (TTCN-3 only)
        if (!my_governor) FATAL_ERROR("Value::set_valuetype()");
        Type *t = my_governor->get_type_refd_last();
        switch (t->get_typetype()) {
        case Type::T_SEQ_T:
        case Type::T_SEQ_A:
          break;
        default:
          FATAL_ERROR("Value::set_valuetype()");
        }
        Values *vals = u.val_vs;
        size_t nof_vals = vals->get_nof_vs();
        size_t nof_comps = t->get_nof_comps();
        if (nof_vals > nof_comps) {
          error("Too many elements in value list notation for type `%s': "
                "%lu was expected instead of %lu",
                t->get_typename().c_str(),
                (unsigned long)nof_comps, (unsigned long)nof_vals);
        }
        size_t upper_limit;
        bool allnotused;
        if (nof_vals <= nof_comps) {
          upper_limit = nof_vals;
          allnotused = true;
        } else {
          upper_limit = nof_comps;
          allnotused = false;
        }
        u.val_nvs = new NamedValues;
        for (size_t i = 0; i < upper_limit; i++) {
          Value *v = vals->steal_v_byIndex(i);
          if (v->valuetype != V_NOTUSED) {
            allnotused = false;
          }
            NamedValue *nv =
              new NamedValue(t->get_comp_id_byIndex(i).clone(), v);
            nv->set_location(*v);
            u.val_nvs->add_nv(nv);
        }
        u.val_nvs->set_my_scope(get_my_scope());
        u.val_nvs->set_fullname(get_fullname());
        delete vals;
        if (allnotused && nof_vals > 0)
          warning("All elements of value list notation for type `%s' are not "
                  "used symbols (`-')", t->get_typename().c_str());
        break; }
      case V_SET:
        // { } -> empty set value
        if (u.val_vs->get_nof_vs() != 0)
          FATAL_ERROR("Value::set_valuetype()");
        delete u.val_vs;
        u.val_nvs = new NamedValues;
        break;
      case V_SETOF:
      case V_ARRAY:
        // SEQOF -> SETOF or ARRAY: trivial
        break;
      default:
        FATAL_ERROR("Value::set_valuetype()");
      }
      break;
    case V_SET:
    case V_CHOICE:
      if (p_valuetype == V_NOTUSED) {
        clean_up();
      }
      else {
        FATAL_ERROR("Value::set_valuetype()");
      }
      break;
    case V_TTCN3_NULL:
      switch (p_valuetype) {
      case V_DEFAULT_NULL:
        break;
      case V_FAT_NULL:
        break;
      default:
        FATAL_ERROR("Value::set_valuetype()");
      }
      break;
    case V_NOTUSED:
      if (V_OMIT != p_valuetype) { // in case of implicit omit
        FATAL_ERROR("Value::set_valuetype()");
      }
      break;
    default:
      FATAL_ERROR("Value::set_valuetype()");
    } // switch
    valuetype=p_valuetype;
  }

  void Value::set_valuetype_COMP_NULL()
  {
    if(valuetype == V_ERROR) return;
    if(valuetype==V_TTCN3_NULL) {
      valuetype=V_EXPR;
      u.expr.v_optype=OPTYPE_COMP_NULL;
      // Nothing to check.
      u.expr.state=EXPR_CHECKED;
    }
    else FATAL_ERROR("Value::set_valuetype_COMP_NULL()");
  }

  void Value::set_valuetype(valuetype_t p_valuetype, const Int& p_val_int)
  {
    if (valuetype == V_NAMEDINT && p_valuetype == V_INT) {
      delete u.val_id;
      u.val_Int = new int_val_t(p_val_int);
      valuetype = V_INT;
    } else FATAL_ERROR("Value::set_valuetype()");
  }

  void Value::set_valuetype(valuetype_t p_valuetype, string *p_str)
  {
    if (p_str && valuetype == V_NAMEDBITS && p_valuetype == V_BSTR) {
      clean_up();
      valuetype = V_BSTR;
      set_val_str(p_str);
    } else FATAL_ERROR("Value::set_valuetype()");
  }

  void Value::set_valuetype(valuetype_t p_valuetype, Identifier *p_id)
  {
    if (p_id && valuetype == V_UNDEF_LOWERID && p_valuetype == V_ENUM) {
      delete u.val_id;
      u.val_id = p_id;
      valuetype = V_ENUM;
    } else FATAL_ERROR("Value::set_valuetype()");
  }

  void Value::set_valuetype(valuetype_t p_valuetype, Assignment *p_ass)
  {
    switch (p_valuetype) {
    case V_FUNCTION:
    case V_ALTSTEP:
    case V_TESTCASE:
      if (valuetype == V_REFER && p_ass) break;
      // no break
    default:
      FATAL_ERROR("Value::set_valuetype()");
    }
    delete u.refered;
    u.refd_fat = p_ass;
    valuetype = p_valuetype;
  }

  bool Value::is_undef_lowerid()
  {
    switch (valuetype) {
    case V_UNDEF_LOWERID:
      return true;
    case V_EXPR:
      if (u.expr.v_optype == OPTYPE_VALUEOF && !u.expr.ti1->get_Type() &&
         !u.expr.ti1->get_DerivedRef()) {
	return u.expr.ti1->get_Template()->is_undef_lowerid();
      }
      // no break
    default:
      return false;
    }
  }

  const Identifier& Value::get_undef_lowerid()
  {
    switch (valuetype) {
    case V_UNDEF_LOWERID:
      return *u.val_id;
    case V_EXPR:
      if (u.expr.v_optype != OPTYPE_VALUEOF)
        FATAL_ERROR("Value::get_undef_lowerid()");
      return u.expr.ti1->get_Template()->get_specific_value()
        ->get_undef_lowerid();
    default:
      FATAL_ERROR("Value::get_undef_lowerid()");
    }
    const Identifier *dummy = 0;
    return *dummy;
  }

  void Value::set_lowerid_to_ref()
  {
    switch (valuetype) {
    case V_UNDEF_LOWERID:
      set_valuetype(V_REFD);
      break;
    case V_EXPR:
      // if the governor of the expression is not known (in log(), etc...)
      // then the governor is taken from the reference (using
      // v1/ti1->get_expr_governor()), but that runs before the
      // params were checked, this smells like a workaround :)
      switch (u.expr.v_optype) {
      case OPTYPE_ROTL:
      case OPTYPE_ROTR:
        u.expr.v1->set_lowerid_to_ref();
        break;
      case OPTYPE_CONCAT:
        u.expr.v1->set_lowerid_to_ref();
        u.expr.v2->set_lowerid_to_ref();
        break;
      case OPTYPE_VALUEOF:
      case OPTYPE_ISVALUE:
      case OPTYPE_ISBOUND:
      case OPTYPE_ISPRESENT:
      case OPTYPE_SUBSTR:
      case OPTYPE_REGEXP:
      case OPTYPE_REPLACE:
      case OPTYPE_TTCN2STRING:
      case OPTYPE_ISTEMPLATEKIND:
        if (!u.expr.ti1->get_Type() && !u.expr.ti1->get_DerivedRef()) {
          Error_Context cntxt(u.expr.ti1->get_Template(),
                              "In the operand of operation `%s'",
                              get_opname());
          u.expr.ti1->get_Template()->set_lowerid_to_ref();
        }
        if (u.expr.v_optype==OPTYPE_REGEXP) {
          if (!u.expr.t2->get_Type() && !u.expr.t2->get_DerivedRef()) {
            Error_Context cntxt(u.expr.t2->get_Template(),
                               "In the operand of operation `%s'",
                               get_opname());
            u.expr.t2->get_Template()->set_lowerid_to_ref();
          }
        }
        if (u.expr.v_optype==OPTYPE_REPLACE) {
          if (!u.expr.ti4->get_Type() && !u.expr.ti4->get_DerivedRef()) {
            Error_Context cntxt(u.expr.ti4->get_Template(),
                                "In the operand of operation `%s'",
                                get_opname());
            u.expr.ti4->get_Template()->set_lowerid_to_ref();
          }
        }
        break;
      default:
        break;
      }
      break;
    default:
      break;
    }
  }

  Type::typetype_t Value::get_expr_returntype(Type::expected_value_t exp_val)
  {
    switch (valuetype) {
    case V_CHARSYMS:
    case V_CHOICE:
    case V_SEQOF:
    case V_SETOF:
    case V_ARRAY:
    case V_SEQ:
    case V_SET:
    case V_UNDEF_LOWERID:
    case V_UNDEF_BLOCK:
    case V_OMIT:
    case V_TTCN3_NULL:
    case V_NOTUSED:
    case V_REFER:
    case V_FAT_NULL:
    case V_ANY_VALUE:
    case V_ANY_OR_OMIT:
      return Type::T_UNDEF;
    case V_NAMEDINT:
    case V_NAMEDBITS:
    case V_OPENTYPE:
      FATAL_ERROR("Value::get_expr_returntype()");
    case V_ERROR:
      return Type::T_ERROR;
    case V_REFD:
    case V_INVOKE: {
      Type *t = get_expr_governor(exp_val);
      if (t) return t->get_type_refd_last()->get_typetype_ttcn3();
      else return Type::T_ERROR; }
    case V_FUNCTION:
      return Type::T_FUNCTION;
    case V_ALTSTEP:
      return Type::T_ALTSTEP;
    case V_TESTCASE:
      return Type::T_TESTCASE;
    case V_EXPR:
      switch(u.expr.v_optype) {
      case OPTYPE_COMP_NULL:
      case OPTYPE_COMP_MTC:
      case OPTYPE_COMP_SYSTEM:
      case OPTYPE_COMP_SELF:
      case OPTYPE_COMP_CREATE:
        return Type::T_COMPONENT;
      case OPTYPE_UNDEF_RUNNING:
      case OPTYPE_COMP_RUNNING:
      case OPTYPE_COMP_RUNNING_ANY:
      case OPTYPE_COMP_RUNNING_ALL:
      case OPTYPE_COMP_ALIVE:
      case OPTYPE_COMP_ALIVE_ANY:
      case OPTYPE_COMP_ALIVE_ALL:
      case OPTYPE_TMR_RUNNING:
      case OPTYPE_TMR_RUNNING_ANY:
      case OPTYPE_MATCH:
      case OPTYPE_EQ:
      case OPTYPE_LT:
      case OPTYPE_GT:
      case OPTYPE_NE:
      case OPTYPE_GE:
      case OPTYPE_LE:
      case OPTYPE_NOT:
      case OPTYPE_AND:
      case OPTYPE_OR:
      case OPTYPE_XOR:
      case OPTYPE_ISPRESENT:
      case OPTYPE_ISCHOSEN:
      case OPTYPE_ISCHOSEN_V:
      case OPTYPE_ISCHOSEN_T:
      case OPTYPE_ISVALUE:
      case OPTYPE_ISBOUND:
      case OPTYPE_PROF_RUNNING:
      case OPTYPE_CHECKSTATE_ANY:
      case OPTYPE_CHECKSTATE_ALL:
      case OPTYPE_ISTEMPLATEKIND:
        return Type::T_BOOL;
      case OPTYPE_GETVERDICT:
        return Type::T_VERDICT;
      case OPTYPE_VALUEOF: {
        Error_Context cntxt(this, "In the operand of operation `%s'",
                            get_opname());
        return u.expr.ti1->get_expr_returntype(Type::EXPECTED_TEMPLATE);}
      case OPTYPE_TMR_READ:
      case OPTYPE_INT2FLOAT:
      case OPTYPE_STR2FLOAT:
      case OPTYPE_RND:
      case OPTYPE_RNDWITHVAL:
        return Type::T_REAL;
      case OPTYPE_ACTIVATE:
        return Type::T_DEFAULT;
      case OPTYPE_ACTIVATE_REFD:
        return Type::T_DEFAULT;
      case OPTYPE_EXECUTE:
      case OPTYPE_EXECUTE_REFD:
        return Type::T_VERDICT;
      case OPTYPE_UNARYPLUS: // v1
      case OPTYPE_UNARYMINUS: {
        Type::typetype_t tmp_tt;
        {
          Error_Context cntxt(this, "In the operand of operation `%s'",
                              get_opname());
          u.expr.v1->set_lowerid_to_ref();
          tmp_tt=u.expr.v1->get_expr_returntype(exp_val);
        }
        switch(tmp_tt) {
        case Type::T_INT:
        case Type::T_REAL:
          return tmp_tt;
        default:
          get_value_refd_last(); // to report the error
          return Type::T_ERROR;
        } // switch tmp_tt
      }
      case OPTYPE_ADD: // v1 v2
      case OPTYPE_SUBTRACT:
      case OPTYPE_MULTIPLY:
      case OPTYPE_DIVIDE: {
        Type::typetype_t tmp_tt;
        {
          Error_Context cntxt(this, "In the left operand of operation `%s'",
                              get_opname());
          u.expr.v1->set_lowerid_to_ref();
          tmp_tt=u.expr.v1->get_expr_returntype(exp_val);
        }
        switch(tmp_tt) {
        case Type::T_INT:
        case Type::T_REAL:
          return tmp_tt;
        default:
          if(u.expr.v_optype==OPTYPE_ADD) {
            Type::typetype_t tmp_tt2;
            {
              Error_Context cntxt(this, "In the right operand of operation `%s'",
                                  get_opname());
              u.expr.v2->set_lowerid_to_ref();
              tmp_tt2=u.expr.v2->get_expr_returntype(exp_val);
            }
            Type::typetype_t ret_val=Type::T_ERROR;
            bool maybeconcat=false;
            switch(tmp_tt) {
            case Type::T_BSTR:
            case Type::T_HSTR:
            case Type::T_OSTR:
              if(tmp_tt2==tmp_tt) {
                maybeconcat=true;
                ret_val=tmp_tt;
              }
              break;
            case Type::T_CSTR:
            case Type::T_USTR:
              if(tmp_tt2==Type::T_CSTR || tmp_tt2==Type::T_USTR) {
                maybeconcat=true;
                if(tmp_tt==Type::T_USTR || tmp_tt2==Type::T_USTR)
                  ret_val=Type::T_USTR;
                else ret_val=Type::T_CSTR;
              }
              break;
            default:
              break;
            }
            if(maybeconcat) {
              error("Did you mean the concat operation (`&') instead of"
                    " addition operator (`+')?");
              u.expr.v_optype=OPTYPE_CONCAT;
              return ret_val;
            }
          }
          get_value_refd_last(); // to report the error
          return Type::T_ERROR;
        } // switch tmp_tt
      }
      case OPTYPE_NOT4B: // v1
      case OPTYPE_AND4B: // v1 v2
      case OPTYPE_OR4B:
      case OPTYPE_XOR4B:
      case OPTYPE_SHL:
      case OPTYPE_SHR: {
        Type::typetype_t tmp_tt;
        {
          Error_Context cntxt(this, "In the %soperand of operation `%s'",
                              u.expr.v_optype==OPTYPE_NOT4B?"":"left ",
                              get_opname());
          u.expr.v1->set_lowerid_to_ref();
          tmp_tt=u.expr.v1->get_expr_returntype(exp_val);
        }
        switch(tmp_tt) {
        case Type::T_BSTR:
        case Type::T_HSTR:
        case Type::T_OSTR:
          return tmp_tt;
        default:
          get_value_refd_last(); // to report the error
          return Type::T_ERROR;
        } // switch tmp_tt
      }
      case OPTYPE_ROTL: // v1 v2
      case OPTYPE_ROTR: {
        Type::typetype_t tmp_tt;
        {
          Error_Context cntxt(this, "In the %s operand of operation `%s'",
                              u.expr.v_optype==OPTYPE_ROTL
                              || u.expr.v_optype==OPTYPE_ROTR?"left":"first",
                              get_opname());
          u.expr.v1->set_lowerid_to_ref();
          tmp_tt=u.expr.v1->get_expr_returntype(exp_val);
        }
        switch(tmp_tt) {
        case Type::T_BSTR:
        case Type::T_HSTR:
        case Type::T_OSTR:
        case Type::T_CSTR:
        case Type::T_USTR:
        case Type::T_SETOF:
        case Type::T_SEQOF:
        case Type::T_ARRAY:
          return tmp_tt;
        default:
          get_value_refd_last(); // to report the error
          return Type::T_ERROR;
        } // switch tmp_tt
      }
      case OPTYPE_SUBSTR:
      case OPTYPE_REPLACE: {
        Type::typetype_t tmp_tt;
        {
          Error_Context cntxt(this, "In the operand of operation `%s'",
                              get_opname());
          u.expr.ti1->get_Template()->set_lowerid_to_ref();
          tmp_tt = u.expr.ti1->get_expr_returntype(Type::EXPECTED_TEMPLATE);
        }
        switch (tmp_tt) {
        case Type::T_BSTR:
        case Type::T_HSTR:
        case Type::T_OSTR:
        case Type::T_CSTR:
        case Type::T_USTR:
        case Type::T_SETOF:
        case Type::T_SEQOF:
          return tmp_tt;
        default:
          get_value_refd_last(); // to report the error
          return Type::T_ERROR;
        }
      }
      case OPTYPE_REGEXP: {
        Type::typetype_t tmp_tt;
        {
          Error_Context cntxt(this, "In the first operand of operation `%s'",
                              get_opname());
          u.expr.ti1->get_Template()->set_lowerid_to_ref();
          tmp_tt = u.expr.ti1->get_expr_returntype(Type::EXPECTED_TEMPLATE);
        }
        switch(tmp_tt) {
        case Type::T_CSTR:
        case Type::T_USTR:
          return tmp_tt;
        default:
          get_value_refd_last(); // to report the error
          return Type::T_ERROR;
        } // switch tmp_tt
      }
      case OPTYPE_CONCAT: { // v1 v2
        Type::typetype_t tmp_tt;
        {
          Error_Context cntxt(this, "In the first operand of operation `%s'",
                              get_opname());
          u.expr.v1->set_lowerid_to_ref();
          tmp_tt=u.expr.v1->get_expr_returntype(exp_val);
        }
        switch(tmp_tt) {
        case Type::T_CSTR:
        case Type::T_USTR:
        case Type::T_BSTR:
        case Type::T_HSTR:
        case Type::T_OSTR:
        case Type::T_SETOF:
        case Type::T_SEQOF:
          return tmp_tt;
        default:
          get_value_refd_last(); // to report the error
          return Type::T_ERROR;
        } // switch tmp_tt
      }
      case OPTYPE_MOD:
      case OPTYPE_REM:
      case OPTYPE_CHAR2INT:
      case OPTYPE_UNICHAR2INT:
      case OPTYPE_BIT2INT:
      case OPTYPE_HEX2INT:
      case OPTYPE_OCT2INT:
      case OPTYPE_STR2INT:
      case OPTYPE_FLOAT2INT:
      case OPTYPE_LENGTHOF:
      case OPTYPE_SIZEOF:
      case OPTYPE_DECODE:
      case OPTYPE_ENUM2INT:
      case OPTYPE_DECVALUE_UNICHAR:
        return Type::T_INT;
      case OPTYPE_BIT2STR:
      case OPTYPE_FLOAT2STR:
      case OPTYPE_HEX2STR:
      case OPTYPE_INT2CHAR:
      case OPTYPE_INT2STR:
      case OPTYPE_OCT2CHAR:
      case OPTYPE_OCT2STR:
      case OPTYPE_UNICHAR2CHAR:
      case OPTYPE_LOG2STR:
      case OPTYPE_TESTCASENAME:
      case OPTYPE_TTCN2STRING:
      case OPTYPE_GET_STRINGENCODING:
      case OPTYPE_ENCODE_BASE64:
      case OPTYPE_HOSTID:
        return Type::T_CSTR;
      case OPTYPE_INT2UNICHAR:
      case OPTYPE_OCT2UNICHAR:
      case OPTYPE_ENCVALUE_UNICHAR:
      case OPTYPE_ANY2UNISTR:
        return Type::T_USTR;
      case OPTYPE_INT2BIT:
      case OPTYPE_HEX2BIT:
      case OPTYPE_OCT2BIT:
      case OPTYPE_STR2BIT:
      case OPTYPE_ENCODE:
        return Type::T_BSTR;
      case OPTYPE_INT2HEX:
      case OPTYPE_BIT2HEX:
      case OPTYPE_OCT2HEX:
      case OPTYPE_STR2HEX:
        return Type::T_HSTR;
      case OPTYPE_INT2OCT:
      case OPTYPE_CHAR2OCT:
      case OPTYPE_HEX2OCT:
      case OPTYPE_BIT2OCT:
      case OPTYPE_STR2OCT:
      case OPTYPE_UNICHAR2OCT:
      case OPTYPE_REMOVE_BOM:
      case OPTYPE_DECODE_BASE64:
        return Type::T_OSTR;
      case OPTYPE_DECOMP:
        return Type::T_OID;
      default:
        FATAL_ERROR("Value::get_expr_returntype(): invalid optype");
        // to avoid warning
        return Type::T_ERROR;
      } // switch optype
    case V_MACRO:
      switch (u.macro) {
      case MACRO_MODULEID:
      case MACRO_FILENAME:
      case MACRO_BFILENAME:
      case MACRO_FILEPATH:
      case MACRO_LINENUMBER:
      case MACRO_DEFINITIONID:
      case MACRO_SCOPE:
      case MACRO_TESTCASEID:
        return Type::T_CSTR;
      case MACRO_LINENUMBER_C:
        return Type::T_INT;
      default:
        return Type::T_ERROR;
      }
    case V_NULL:
      return Type::T_NULL;
    case V_BOOL:
      return Type::T_BOOL;
    case V_INT:
      return Type::T_INT;
    case V_REAL:
      return Type::T_REAL;
    case V_ENUM:
      return Type::T_ENUM_T;
    case V_BSTR:
      return Type::T_BSTR;
    case V_HSTR:
      return Type::T_HSTR;
    case V_OSTR:
      return Type::T_OSTR;
    case V_CSTR:
      return Type::T_CSTR;
    case V_USTR:
      return Type::T_USTR;
    case V_ISO2022STR:
      return Type::T_GENERALSTRING;
    case V_OID:
      return Type::T_OID;
    case V_ROID:
      return Type::T_ROID;
    case V_VERDICT:
      return Type::T_VERDICT;
    case V_DEFAULT_NULL:
      return Type::T_DEFAULT;
    default:
      FATAL_ERROR("Value::get_expr_returntype(): invalid valuetype");
      // to avoid warning
      return Type::T_ERROR;
    } // switch
  }

  Type* Value::get_expr_governor(Type::expected_value_t exp_val)
  {
    if(my_governor) return my_governor;
    switch (valuetype) {
    case V_INVOKE: {
      Type *t = u.invoke.v->get_expr_governor(exp_val);
      if(!t) {
        if(u.invoke.v->get_valuetype() != V_ERROR)
          u.invoke.v->error("A value of type function expected");
        goto error;
      }
      t = t->get_type_refd_last();
      switch(t->get_typetype()) {
      case Type::T_FUNCTION: {
	Type *t_return_type = t->get_function_return_type();
	if (!t_return_type) {
	  error("Reference to a %s was expected instead of invocation "
	    "of behavior type `%s' with no return type",
	    exp_val == Type::EXPECTED_TEMPLATE ? "value or template" : "value",
	    t->get_fullname().c_str());
	  goto error;
	}
        if (exp_val != Type::EXPECTED_TEMPLATE && t->get_returns_template()) {
          error("Reference to a value was expected, but functions of type "
	    "`%s' return a template of type `%s'", t->get_typename().c_str(),
	    t_return_type->get_typename().c_str());
          goto error;
        }
        return t_return_type; }
      case Type::T_ALTSTEP:
        goto error;
      default:
        u.invoke.v->error("A value of type function expected instead of `%s'",
          t->get_typename().c_str());
        goto error;
      }
      break; }
    case V_REFD: {
      Assignment *ass=u.ref.ref->get_refd_assignment();
      Type *tmp_type=0;
      if (!ass) goto error;
      switch (ass->get_asstype()) {
      case Assignment::A_CONST:
      case Assignment::A_EXT_CONST:
      case Assignment::A_MODULEPAR:
      case Assignment::A_MODULEPAR_TEMP:
      case Assignment::A_TEMPLATE:
      case Assignment::A_VAR:
      case Assignment::A_VAR_TEMPLATE:
      case Assignment::A_FUNCTION_RVAL:
      case Assignment::A_FUNCTION_RTEMP:
      case Assignment::A_EXT_FUNCTION_RVAL:
      case Assignment::A_EXT_FUNCTION_RTEMP:
      case Assignment::A_PAR_VAL_IN:
      case Assignment::A_PAR_VAL_OUT:
      case Assignment::A_PAR_VAL_INOUT:
      case Assignment::A_PAR_TEMPL_IN:
      case Assignment::A_PAR_TEMPL_OUT:
      case Assignment::A_PAR_TEMPL_INOUT:
        tmp_type=ass->get_Type();
        break;
      case Assignment::A_FUNCTION:
      case Assignment::A_EXT_FUNCTION:
        error("Reference to a %s was expected instead of a call of %s, which "
	  "does not have return type",
	  exp_val == Type::EXPECTED_TEMPLATE ? "value or template" : "value",
	  ass->get_description().c_str());
        goto error;
      default:
        error("Reference to a %s was expected instead of %s",
	  exp_val == Type::EXPECTED_TEMPLATE ? "value or template" : "value",
	  ass->get_description().c_str());
        goto error;
      } // end switch
      tmp_type=tmp_type->get_field_type(u.ref.ref->get_subrefs(), exp_val);
      if(!tmp_type) goto error;
      return tmp_type; }
    case V_EXPR:
      switch (u.expr.v_optype) {
      case OPTYPE_VALUEOF:
      case OPTYPE_SUBSTR:
      case OPTYPE_REGEXP:
      case OPTYPE_REPLACE:{
      	Type *tmp_type = u.expr.ti1->get_expr_governor(exp_val ==
          Type::EXPECTED_DYNAMIC_VALUE ? Type::EXPECTED_TEMPLATE : exp_val);
	if(tmp_type) tmp_type = tmp_type->get_type_refd_last();
	return tmp_type;
	  }
      case OPTYPE_ROTL:
      case OPTYPE_ROTR:
        return u.expr.v1->get_expr_governor(exp_val);
      case OPTYPE_CONCAT:
        return get_expr_governor_v1v2(exp_val);
      case OPTYPE_COMP_MTC:
	if (my_scope) return my_scope->get_mtc_system_comptype(false);
	else return 0;
      case OPTYPE_COMP_SYSTEM:
	if (my_scope) return my_scope->get_mtc_system_comptype(true);
	else return 0;
      case OPTYPE_COMP_SELF:
	if (my_scope) {
	  Ttcn::RunsOnScope *t_ros = my_scope->get_scope_runs_on();
	  if (t_ros) return t_ros->get_component_type();
	  else return 0;
	} else return 0;
      case OPTYPE_COMP_CREATE:
	return chk_expr_operand_comptyperef_create();
      default:
        break;
      }
      // no break
    default:
      return Type::get_pooltype(get_expr_returntype(exp_val));
    }
  error:
    set_valuetype(V_ERROR);
    return 0;
  }

  Type* Value::get_expr_governor_v1v2(Type::expected_value_t exp_val)
  {
    Type* v1_gov = u.expr.v1->get_expr_governor(exp_val);
    Type* v2_gov = u.expr.v2->get_expr_governor(exp_val);
    if (v1_gov) {
      if (v2_gov) { // both have governors
        // return the type that is compatible with both (if there is no type mismatch)
        if (v1_gov->is_compatible(v2_gov, NULL, NULL))
          return v1_gov;
        else return v2_gov;
      } else return v1_gov;
    } else { // v1 has no governor
      if (v2_gov) return v2_gov;
      else return NULL; // neither has governor
    }
  }

  Type *Value::get_expr_governor_last()
  {
    Value *v_last = get_value_refd_last();
    if (v_last->valuetype == V_ERROR) return 0;
    Type *t = v_last->get_expr_governor(Type::EXPECTED_TEMPLATE);
    if(!t) return 0;
    return t->get_type_refd_last();
  }

  Type *Value::get_invoked_type(Type::expected_value_t exp_val)
  {
    if(valuetype != V_INVOKE) FATAL_ERROR("Value::get_invoked_type()");
    return u.invoke.v->get_expr_governor(exp_val);
  }

  const char* Value::get_opname() const
  {
    if(valuetype!=V_EXPR) FATAL_ERROR("Value::get_opname()");
    switch(u.expr.v_optype) {
    case OPTYPE_RND: // -
      return "rnd()";
    case OPTYPE_COMP_NULL:
      return "(component) null";
    case OPTYPE_COMP_MTC:
      return "mtc";
    case OPTYPE_COMP_SYSTEM:
      return "system";
    case OPTYPE_COMP_SELF:
      return "self";
    case OPTYPE_COMP_RUNNING_ANY:
      return "any component.running";
    case OPTYPE_COMP_RUNNING_ALL:
      return "all component.running";
    case OPTYPE_COMP_ALIVE_ANY:
      return "any component.alive";
    case OPTYPE_COMP_ALIVE_ALL:
      return "all component.alive";
    case OPTYPE_TMR_RUNNING_ANY:
      return "any timer.running";
    case OPTYPE_GETVERDICT:
      return "getverdict()";
    case OPTYPE_TESTCASENAME:
      return "testcasename()";
    case OPTYPE_CHECKSTATE_ANY:
      if (u.expr.r1) {
        return "port.checkstate()";
      } else {
        return "any port.checkstate()";
      }
    case OPTYPE_CHECKSTATE_ALL:
      if (u.expr.r1) {
        return "port.checkstate()";
      } else {
        return "all port.checkstate()";
      }
    case OPTYPE_UNARYPLUS: // v1
      return "unary +";
    case OPTYPE_UNARYMINUS:
      return "unary -";
    case OPTYPE_NOT:
      return "not";
    case OPTYPE_NOT4B:
      return "not4b";
    case OPTYPE_BIT2HEX:
      return "bit2hex()";
    case OPTYPE_BIT2INT:
      return "bit2int()";
    case OPTYPE_BIT2OCT:
      return "bit2oct()";
    case OPTYPE_BIT2STR:
      return "bit2str()";
    case OPTYPE_CHAR2INT:
      return "char2int()";
    case OPTYPE_CHAR2OCT:
      return "char2oct()";
    case OPTYPE_FLOAT2INT:
      return "float2int()";
    case OPTYPE_FLOAT2STR:
      return "float2str()";
    case OPTYPE_HEX2BIT:
      return "hex2bit()";
    case OPTYPE_HEX2INT:
      return "hex2int()";
    case OPTYPE_HEX2OCT:
      return "hex2oct()";
    case OPTYPE_HEX2STR:
      return "hex2str()";
    case OPTYPE_INT2CHAR:
      return "int2char()";
    case OPTYPE_INT2FLOAT:
      return "int2float()";
    case OPTYPE_INT2STR:
      return "int2str()";
    case OPTYPE_INT2UNICHAR:
      return "int2unichar()";
    case OPTYPE_OCT2BIT:
      return "oct2bit()";
    case OPTYPE_OCT2CHAR:
      return "oct2char()";
    case OPTYPE_OCT2HEX:
      return "oct2hex()";
    case OPTYPE_OCT2INT:
      return "oct2int()";
    case OPTYPE_OCT2STR:
      return "oct2str()";
    case OPTYPE_STR2BIT:
      return "str2bit()";
    case OPTYPE_STR2FLOAT:
      return "str2float()";
    case OPTYPE_STR2HEX:
      return "str2hex()";
    case OPTYPE_STR2INT:
      return "str2int()";
    case OPTYPE_STR2OCT:
      return "str2oct()";
    case OPTYPE_UNICHAR2INT:
      return "unichar2int()";
    case OPTYPE_UNICHAR2CHAR:
      return "unichar2char()";
    case OPTYPE_UNICHAR2OCT:
      return "unichar2oct()";
    case OPTYPE_ENUM2INT:
      return "enum2int()";
    case OPTYPE_LENGTHOF:
      return "lengthof()";
    case OPTYPE_SIZEOF:
      return "sizeof()";
    case OPTYPE_RNDWITHVAL:
      return "rnd (seed)";
    case OPTYPE_ENCODE:
      return "encvalue()";
    case OPTYPE_DECODE:
      return "decvalue()";
    case OPTYPE_GET_STRINGENCODING:
      return "get_stringencoding()";
    case OPTYPE_REMOVE_BOM:
      return "remove_bom()";
    case OPTYPE_ENCODE_BASE64:
      return "encode_base64()";
    case OPTYPE_DECODE_BASE64:
      return "decode_base64()";
    case OPTYPE_HOSTID: // [v1]
      return "hostid()";
    case OPTYPE_ADD: // v1 v2
      return "+";
    case OPTYPE_SUBTRACT:
      return "-";
    case OPTYPE_MULTIPLY:
      return "*";
    case OPTYPE_DIVIDE:
      return "/";
    case OPTYPE_MOD:
      return "mod";
    case OPTYPE_REM:
      return "rem";
    case OPTYPE_CONCAT:
      return "&";
    case OPTYPE_EQ:
      return "==";
    case OPTYPE_LT:
      return "<";
    case OPTYPE_GT:
      return ">";
    case OPTYPE_NE:
      return "!=";
    case OPTYPE_GE:
      return ">=";
    case OPTYPE_LE:
      return "<=";
    case OPTYPE_AND:
      return "and";
    case OPTYPE_OR:
      return "or";
    case OPTYPE_XOR:
      return "xor";
    case OPTYPE_AND4B:
      return "and4b";
    case OPTYPE_OR4B:
      return "or4b";
    case OPTYPE_XOR4B:
      return "xor4b";
    case OPTYPE_SHL:
      return "<<";
    case OPTYPE_SHR:
      return ">>";
    case OPTYPE_ROTL:
      return "<@";
    case OPTYPE_ROTR:
      return "@>";
    case OPTYPE_INT2BIT:
      return "int2bit()";
    case OPTYPE_INT2HEX:
      return "int2hex()";
    case OPTYPE_INT2OCT:
      return "int2oct()";
    case OPTYPE_OCT2UNICHAR:
      return "oct2unichar()";
    case OPTYPE_ENCVALUE_UNICHAR:
      return "encvalue_unichar()";
    case OPTYPE_DECVALUE_UNICHAR:
      return "decvalue_unichar()";
    case OPTYPE_SUBSTR:
      return "substr()";
    case OPTYPE_REGEXP:
      return "regexp()";
    case OPTYPE_DECOMP:
      return "decomp()";
    case OPTYPE_REPLACE:
      return "replace()";
    case OPTYPE_VALUEOF: // t1
      return "valueof()";
    case OPTYPE_UNDEF_RUNNING:
      return "<timer or component> running";
    case OPTYPE_COMP_CREATE: // r1 [v2] [v3] b4
      return "create()";
    case OPTYPE_COMP_RUNNING: // v1
      return "component running";
    case OPTYPE_COMP_ALIVE: // v1
      return "alive";
    case OPTYPE_TMR_READ:
      return "timer read";
    case OPTYPE_TMR_RUNNING:
      return "timer running";
    case OPTYPE_ACTIVATE:
      return "activate()";
    case OPTYPE_ACTIVATE_REFD:
      return "activate()";
    case OPTYPE_EXECUTE: // r1 [v2]
    case OPTYPE_EXECUTE_REFD:
      return "execute()";
    case OPTYPE_MATCH: // v1 t2
      return "match()";
    case OPTYPE_ISPRESENT:
      return "ispresent()";
    case OPTYPE_ISCHOSEN:
    case OPTYPE_ISCHOSEN_V:
    case OPTYPE_ISCHOSEN_T:
      return "ischosen()";
    case OPTYPE_ISVALUE:
      return "isvalue()";
    case OPTYPE_ISBOUND:
      return "isbound()";
    case OPTYPE_ISTEMPLATEKIND:
      return "istemplatekind()";
    case OPTYPE_LOG2STR:
      return "log2str()";
    case OPTYPE_ANY2UNISTR:
      return "any2unistr()";
    case OPTYPE_TTCN2STRING:
      return "ttcn2string()";
    case OPTYPE_PROF_RUNNING:
      return "@profiler.running";
    default:
      FATAL_ERROR("Value::get_opname()");
    } // switch
  }

  void Value::chk_expr_ref_ischosen()
  {
    Error_Context cntxt(this, "In the operand of operation `%s'", get_opname());
    Ttcn::Ref_base *tmpref=u.expr.r1;
    Assignment *ass=tmpref->get_refd_assignment();
    if (!ass) {
      set_valuetype(V_ERROR);
      return;
    }
    // Now we know whether the argument of ischosen() is a value or template.
    // Wrap u.expr.r1 of OPTYPE_ISCHOSEN in a value (OPTYPE_ISCHOSEN_V)
    // or template (OPTYPE_ISCHOSEN_T).
    switch (ass->get_asstype()) {
    case Assignment::A_CONST:
    case Assignment::A_EXT_CONST:
    case Assignment::A_MODULEPAR:
    case Assignment::A_VAR:
    case Assignment::A_PAR_VAL_IN:
    case Assignment::A_PAR_VAL_OUT:
    case Assignment::A_PAR_VAL_INOUT:
      u.expr.v1=new Value(V_REFD, tmpref);
      u.expr.v1->set_location(*tmpref);
      u.expr.v1->set_my_scope(get_my_scope());
      u.expr.v1->set_fullname(get_fullname()+".<operand>");
      u.expr.v_optype=OPTYPE_ISCHOSEN_V;
      break;
    case Assignment::A_MODULEPAR_TEMP:
    case Assignment::A_TEMPLATE:
    case Assignment::A_VAR_TEMPLATE:
    case Assignment::A_PAR_TEMPL_IN:
    case Assignment::A_PAR_TEMPL_OUT:
    case Assignment::A_PAR_TEMPL_INOUT:
      u.expr.t1=new Template(tmpref); // TEMPLATE_REFD constructor
      u.expr.t1->set_location(*tmpref);
      u.expr.t1->set_my_scope(get_my_scope());
      u.expr.t1->set_fullname(get_fullname()+".<operand>");
      u.expr.v_optype=OPTYPE_ISCHOSEN_T;
      break;
    default:
      tmpref->error("Reference to a value or template was expected instead of "
	"%s", ass->get_description().c_str());
      set_valuetype(V_ERROR);
      break;
    } // switch
  }

  void Value::chk_expr_operandtype_enum(const char *opname, Value *v,
                                        Type::expected_value_t exp_val)
  {
    v->set_lowerid_to_ref(); // can only be reference to enum
    Type *t = v->get_expr_governor(exp_val);
    if (v->valuetype==V_ERROR) return;
    if (!t) {
      v->error("Please use reference to an enumerated value as the operand of "
            "operation `%s'", get_opname());
      set_valuetype(V_ERROR);
      return;
    }
    t = t->get_type_refd_last();
    if (t->get_typetype()!=Type::T_ENUM_A && t->get_typetype()!=Type::T_ENUM_T) {
      v->error("The operand of operation `%s' should be enumerated value", opname);
      set_valuetype(V_ERROR);
    }
    if (v->get_value_refd_last()->valuetype==V_OMIT) {
      v->error("The operand of operation `%s' cannot be omit", opname);
      set_valuetype(V_ERROR);
    }
  }

  void Value::chk_expr_operandtype_bool(Type::typetype_t tt,
                                        const char *opnum,
                                        const char *opname,
                                        const Location *loc)
  {
    if(tt==Type::T_BOOL) return;
    if(tt!=Type::T_ERROR)
      loc->error("%s operand of operation `%s' should be boolean value",
                 opnum, opname);
    set_valuetype(V_ERROR);
  }

  void Value::chk_expr_operandtype_int(Type::typetype_t tt,
                                       const char *opnum,
                                       const char *opname,
                                       const Location *loc)
  {
    if(tt==Type::T_INT) return;
    if(tt!=Type::T_ERROR)
      loc->error("%s operand of operation `%s' should be integer value",
                 opnum, opname);
    set_valuetype(V_ERROR);
  }

  void Value::chk_expr_operandtype_float(Type::typetype_t tt,
                                         const char *opnum,
                                         const char *opname,
                                         const Location *loc)
  {
    if(tt==Type::T_REAL) return;
    else if(tt==Type::T_INT)
      loc->error("%s operand of operation `%s' should be float value."
                 " Perhaps you missed an int2float() conversion function"
                 " or `.0' at the end of the number",
                 opnum, opname);
    else if(tt!=Type::T_ERROR)
      loc->error("%s operand of operation `%s' should be float value",
                 opnum, opname);
    set_valuetype(V_ERROR);
  }

  void Value::chk_expr_operandtype_int_float(Type::typetype_t tt,
                                             const char *opnum,
                                             const char *opname,
                                             const Location *loc)
  {
    switch(tt) {
    case Type::T_INT:
    case Type::T_REAL:
      return;
    default:
      break;
    }
    if(tt!=Type::T_ERROR)
      loc->error("%s operand of operation `%s' should be integer"
                 " or float value",
                 opnum, opname);
    set_valuetype(V_ERROR);
  }

  void Value::chk_expr_operandtype_int_float_enum(Type::typetype_t tt,
                                                  const char *opnum,
                                                  const char *opname,
                                                  const Location *loc)
  {
    switch(tt) {
    case Type::T_INT:
    case Type::T_REAL:
    case Type::T_ENUM_T:
      return;
    default:
      break;
    }
    if(tt!=Type::T_ERROR)
      loc->error("%s operand of operation `%s' should be integer, float"
                 " or enumerated value", opnum, opname);
    set_valuetype(V_ERROR);
  }

  void Value::chk_expr_operandtype_list(Type* t,
                                        const char *opnum,
                                        const char *opname,
                                        const Location *loc,
                                        bool allow_array)
  {
    if (valuetype == V_ERROR) return;
    if (t->get_typetype() == Type::T_ERROR) {
      set_valuetype(V_ERROR);
      return;
    }
    if (!t->is_list_type(allow_array)) {
      loc->error("%s operand of operation `%s' should be a string, "
                 "`record of'%s `set of'%s value", opnum, opname,
                 allow_array ? "," : " or", allow_array ? " or array" : "");
      set_valuetype(V_ERROR);
      return;
    }
    TypeCompatInfo info(my_scope->get_scope_mod(), my_governor, t, true,
                        u.expr.v_optype == OPTYPE_LENGTHOF);  // The only outsider.
    TypeChain l_chain;
    TypeChain r_chain;
    if (my_governor && my_governor->is_list_type(allow_array)
        && !my_governor->is_compatible(t, &info, this, &l_chain, &r_chain)) {
      if (info.is_subtype_error()) {
        // this is ok.
        if (info.needs_conversion()) set_needs_conversion();
      } else
      if (!info.is_erroneous()) {
        error("%s operand of operation `%s' is of type `%s', but a value of "
              "type `%s' was expected here", opnum, opname,
              t->get_typename().c_str(), my_governor->get_typename().c_str());
      } else {
        error("%s", info.get_error_str_str().c_str());
      }
    } else {
      if (info.needs_conversion())
        set_needs_conversion();
    }
  }

  void Value::chk_expr_operandtype_str(Type::typetype_t tt,
                                       const char *opnum,
                                       const char *opname,
                                       const Location *loc)
  {
    switch(tt) {
    case Type::T_CSTR:
    case Type::T_USTR:
    case Type::T_BSTR:
    case Type::T_HSTR:
    case Type::T_OSTR:
      return;
    default:
      break;
    }
    if(tt!=Type::T_ERROR)
      loc->error("%s operand of operation `%s' should be string value",
                 opnum, opname);
    set_valuetype(V_ERROR);
  }

  void Value::chk_expr_operandtype_charstr(Type::typetype_t tt,
                                           const char *opnum,
                                           const char *opname,
                                           const Location *loc)
  {
    switch(tt) {
    case Type::T_CSTR:
    case Type::T_USTR:
      return;
    default:
      break;
    }
    if(tt!=Type::T_ERROR)
      loc->error("%s operand of operation `%s' should be (universal)"
                 " charstring value",
                 opnum, opname);
    set_valuetype(V_ERROR);
  }

  void Value::chk_expr_operandtype_cstr(Type::typetype_t tt,
                                        const char *opnum,
                                        const char *opname,
                                        const Location *loc)
  {
    if(tt==Type::T_CSTR) return;
    if(tt!=Type::T_ERROR)
      loc->error("%s operand of operation `%s' should be charstring value",
                 opnum, opname);
    set_valuetype(V_ERROR);
  }

  void Value::chk_expr_operandtype_binstr(Type::typetype_t tt,
                                          const char *opnum,
                                          const char *opname,
                                          const Location *loc)
  {
    switch(tt) {
    case Type::T_BSTR:
    case Type::T_HSTR:
    case Type::T_OSTR:
      return;
    default:
      break;
    }
    if(tt!=Type::T_ERROR)
      loc->error("%s operand of operation `%s' should be binary string value",
                 opnum, opname);
    set_valuetype(V_ERROR);
  }

  void Value::chk_expr_operandtype_bstr(Type::typetype_t tt,
                                        const char *opnum,
                                        const char *opname,
                                        const Location *loc)
  {
    if(tt==Type::T_BSTR) return;
    if(tt!=Type::T_ERROR)
      loc->error("%s operand of operation `%s' should be bitstring value",
                 opnum, opname);
    set_valuetype(V_ERROR);
  }

  void Value::chk_expr_operandtype_hstr(Type::typetype_t tt,
                                        const char *opnum,
                                        const char *opname,
                                        const Location *loc)
  {
    if(tt==Type::T_HSTR) return;
    if(tt!=Type::T_ERROR)
      loc->error("%s operand of operation `%s' should be hexstring value",
                 opnum, opname);
    set_valuetype(V_ERROR);
  }

  void Value::chk_expr_operandtype_ostr(Type::typetype_t tt,
                                        const char *opnum,
                                        const char *opname,
                                        const Location *loc)
  {
    if(tt==Type::T_OSTR) return;
    if(tt!=Type::T_ERROR)
      loc->error("%s operand of operation `%s' should be octetstring value",
                 opnum, opname);
    set_valuetype(V_ERROR);
  }

  void Value::chk_expr_operandtypes_same(Type::typetype_t tt1,
                                         Type::typetype_t tt2,
                                         const char *opname)
  {
    if(valuetype==V_ERROR) return;
    // if(u.expr.state==EXPR_CHECKING_ERR) return;
    if(tt1==Type::T_ERROR || tt2==Type::T_ERROR) {
      set_valuetype(V_ERROR);
      return;
    }
    if(tt1==tt2) return;
    error("The operands of operation `%s' should be of same type", opname);
    set_valuetype(V_ERROR);
  }

  /* For predefined functions.  */
  void Value::chk_expr_operandtypes_same_with_opnum(Type::typetype_t tt1,
		  			            Type::typetype_t tt2,
		  				    const char *opnum1,
		  				    const char *opnum2,
		  				    const char *opname)
  {
	if(valuetype==V_ERROR) return;
	if(tt1==Type::T_ERROR || tt2==Type::T_ERROR) {
	  set_valuetype(V_ERROR);
	  return;
	}
	if(tt1==tt2) return;
	error("The %s and %s operands of operation `%s' should be of same type",
	  opnum1, opnum2, opname);
	set_valuetype(V_ERROR);
  }

  void Value::chk_expr_operandtypes_compat(Type::expected_value_t exp_val,
                                           Value *v1, Value *v2,
                                           const char *opnum1,
                                           const char *opnum2)
  {
  start:
    if (valuetype == V_ERROR) return;
    // if (u.expr.state == EXPR_CHECKING_ERR) return;
    Type::typetype_t tt1 = v1->get_expr_returntype(exp_val);
    Type::typetype_t tt2 = v2->get_expr_returntype(exp_val);

    if (tt1 == Type::T_ERROR || tt2 == Type::T_ERROR) {
      set_valuetype(V_ERROR);
      return;
    }
    if (tt1 == Type::T_UNDEF) {
      if (tt2 == Type::T_UNDEF) {
        if (v1->is_undef_lowerid()) {
          if (v2->is_undef_lowerid()) {
            Scope *scope = get_my_scope();
            Module *my_mod = scope->get_scope_mod();
            const Identifier& id1 = v1->get_undef_lowerid();
            if (scope->has_ass_withId(id1)
                || my_mod->has_imported_ass_withId(id1)) {
              /* It can be ref-ref, ref-enum or enum-ref. Perhaps we
               * should examine this situation better, but now I suppose
               * the first is ref, not enum. */
              v1->set_lowerid_to_ref();
              goto start;
            } else {
              const Identifier& id2 = v2->get_undef_lowerid();
              if (scope->has_ass_withId(id2)
                  || my_mod->has_imported_ass_withId(id2)) {
                v2->set_lowerid_to_ref();
                goto start;
              }
            }
            /* This is perhaps enum-enum, but it has no real
             * significance, so this should be an error. */
          } else {
            v1->set_lowerid_to_ref();
            goto start;
          }
        } else if (v2->is_undef_lowerid()) {
          v2->set_lowerid_to_ref();
          goto start;
        }
        error("Cannot determine the type of the operands in operation `%s'",
              get_opname());
        set_valuetype(V_ERROR);
        return;
      } else if (v1->is_undef_lowerid() && tt2 != Type::T_ENUM_T) {
        v1->set_lowerid_to_ref();
        goto start;
      } else {
        /* v1 is something undefined, but not lowerid; v2 has
         * returntype (perhaps also governor) */
      }
    } else if (tt2 == Type::T_UNDEF) {
      /* but tt1 is not undef */
      if (v2->is_undef_lowerid() && tt1 != Type::T_ENUM_T) {
        v2->set_lowerid_to_ref();
        goto start;
      } else {
        /* v2 is something undefined, but not lowerid; v1 has
         * returntype (perhaps also governor) */
      }
    }

    /* Now undef_lower_id's are converted to references, or the other
     * value has governor; let's see the governors, if they exist.  */
    Type *t1 = v1->get_expr_governor(exp_val);
    Type *t2 = v2->get_expr_governor(exp_val);
    if (t1) {
      if (t2) {
        // Both value has governor.  Are they compatible?  According to 7.1.2
        // and C.34 it's required to have the same root types for
        // OPTYPE_{CONCAT,REPLACE}.
        TypeCompatInfo info1(my_scope->get_scope_mod(), t1, t2, true,
                             u.expr.v_optype == OPTYPE_REPLACE);
        TypeCompatInfo info2(my_scope->get_scope_mod(), t2, t1, true,
                             u.expr.v_optype == OPTYPE_REPLACE);
        TypeChain l_chain1, l_chain2;
        TypeChain r_chain1, r_chain2;
        bool compat_t1 = t1->is_compatible(t2, &info1, this, &l_chain1, &r_chain1);
        bool compat_t2 = t2->is_compatible(t1, &info2, NULL, &l_chain2, &r_chain2);
        if (!compat_t1 && !compat_t2) {
          if (!info1.is_erroneous() && !info2.is_erroneous()) {
            // the subtypes don't need to be compatible here
            if (!info1.is_subtype_error() && !info2.is_subtype_error()) {
              error("The operands of operation `%s' should be of compatible "
                    "types", get_opname());
              set_valuetype(V_ERROR);
            } else {
              if (info1.needs_conversion() || info2.needs_conversion()) {
                set_needs_conversion();  // Avoid folding.
                return;
              }
            }
          } else {
            if (info1.is_erroneous())
              v1->error("%s", info1.get_error_str_str().c_str());
            else if (info2.is_erroneous())
              v2->error("%s", info2.get_error_str_str().c_str());
            set_valuetype(V_ERROR);
          }
          return;
        } else if (info1.needs_conversion() || info2.needs_conversion()) {
          set_needs_conversion();  // Avoid folding.
          return;
        }
      } else {
        // t1, no t2.
        v2->set_my_governor(t1);
        t1->chk_this_value_ref(v2);
        if (v2->valuetype == V_OMIT) {
          Error_Context cntxt(this, "In %s operand of operation `%s'", opnum1,
                              get_opname());
          v1->chk_expr_omit_comparison(exp_val);
        } else {
          Error_Context cntxt(this, "In %s operand of operation `%s'", opnum2,
                              get_opname());
          (void)t1->chk_this_value(v2, 0, exp_val, INCOMPLETE_NOT_ALLOWED,
            OMIT_NOT_ALLOWED, NO_SUB_CHK);
          goto start;
        }
      }
    } else if (t2) {
      v1->set_my_governor(t2);
      t2->chk_this_value_ref(v1);
      if (v1->valuetype == V_OMIT) {
        Error_Context cntxt(this, "In %s operand of operation `%s'", opnum2,
                            get_opname());
        v2->chk_expr_omit_comparison(exp_val);
      } else {
        Error_Context cntxt(this, "In %s operand of operation `%s'", opnum1,
                            get_opname());
        (void)t2->chk_this_value(v1, 0, exp_val, INCOMPLETE_NOT_ALLOWED,
          OMIT_NOT_ALLOWED, NO_SUB_CHK);
        goto start;
      }
    } else {
      // Neither v1 nor v2 has a governor.  Let's see the returntypes.
      if (tt1 == Type::T_UNDEF || tt2 == Type::T_UNDEF) {
        // Here, it cannot be that both are T_UNDEF.
        // TODO: What if "a" == char(0, 0, 0, 65) or self == null etc.
        error("Please use reference as %s operand of operator `%s'",
              tt1 == Type::T_UNDEF ? opnum1 : opnum2, get_opname());
        set_valuetype(V_ERROR);
        return;
      }
      // Deny type compatibility if no governors found.  The typetype_t must
      // be the same.  TODO: How can this happen?
      // Default false the 5th param
      if (!Type::is_compatible_tt_tt(tt1, tt2, false, false, false)
          && !Type::is_compatible_tt_tt(tt2, tt1, false, false, false)) {
        error("The operands of operation `%s' should be of compatible types",
              get_opname());
        set_valuetype(V_ERROR);
      }
    }
  }

  void Value::chk_expr_operand_undef_running(Type::expected_value_t exp_val,
    Ttcn::Ref_base *ref, bool any_from, Ttcn::Ref_base* index_ref,
    const char *opnum, const char *opname)
  {
    if(valuetype==V_ERROR) return;
    // if(u.expr.state==EXPR_CHECKING_ERR) return;
    Assignment *t_ass = ref->get_refd_assignment();
    if(!t_ass) goto error;
    switch(t_ass->get_asstype()) {
    case Assignment::A_TIMER:
    case Assignment::A_PAR_TIMER:
      u.expr.v_optype=OPTYPE_TMR_RUNNING;
      chk_expr_operand_tmrref(ref, opnum, get_opname(), any_from);
      chk_expr_dynamic_part(exp_val, true);
      if (index_ref != NULL) {
        Ttcn::Reference* index_ref2 = dynamic_cast<Ttcn::Reference*>(index_ref);
        if (index_ref2 == NULL) {
          FATAL_ERROR("Value::chk_expr_operand_undef_running");
        }
        Ttcn::Statement::chk_index_redirect(index_ref2, t_ass->get_Dimensions(),
          any_from, "timer");
      }
      break;
    case Assignment::A_CONST:
    case Assignment::A_EXT_CONST:
    case Assignment::A_MODULEPAR:
    case Assignment::A_VAR:
    case Assignment::A_FUNCTION_RVAL:
    case Assignment::A_EXT_FUNCTION_RVAL:
    case Assignment::A_PAR_VAL_IN:
    case Assignment::A_PAR_VAL_OUT:
    case Assignment::A_PAR_VAL_INOUT: {
      u.expr.v_optype = OPTYPE_COMP_RUNNING;
      Value* val = new Value(V_REFD, ref);
      val->set_my_scope(my_scope);
      val->set_fullname(ref->get_fullname());
      val->set_location(*ref);
      u.expr.v1 = val;
      Type* t = chk_expr_operand_compref(val, opnum, get_opname(), any_from);
      chk_expr_dynamic_part(exp_val, false);
      if (index_ref != NULL && t != NULL) {
        t = t->get_type_refd_last();
        Ttcn::ArrayDimensions dummy;
        while (t->get_typetype() == Type::T_ARRAY) {
          dummy.add(t->get_dimension()->clone());
          t = t->get_ofType()->get_type_refd_last();
        }
        Ttcn::Reference* index_ref2 = dynamic_cast<Ttcn::Reference*>(index_ref);
        if (index_ref2 == NULL) {
          FATAL_ERROR("Value::chk_expr_operand_undef_running");
        }
        Ttcn::Statement::chk_index_redirect(index_ref2, &dummy, any_from,
          "component");
      }
      break; }
    default:
      ref->error("%s operand of operation `%s' should be timer or"
        " component reference instead of %s",
        opnum, opname, t_ass->get_description().c_str());
      goto error;
    } // switch
    return;
  error:
    set_valuetype(V_ERROR);
  }

  Type *Value::chk_expr_operand_comptyperef_create()
  {
    if (valuetype != V_EXPR || u.expr.v_optype != OPTYPE_COMP_CREATE)
      FATAL_ERROR("Value::chk_expr_operand_comptyperef_create()");
    Assignment *t_ass = u.expr.r1->get_refd_assignment();
    if (!t_ass) goto error;
    if (t_ass->get_asstype() == Assignment::A_TYPE) {
      Type *t_type = t_ass->get_Type()->get_field_type(u.expr.r1->get_subrefs(),
        Type::EXPECTED_DYNAMIC_VALUE);
      if (!t_type) goto error;
      t_type = t_type->get_type_refd_last();
      if (t_type->get_typetype() == Type::T_COMPONENT) {
        if (my_governor) {
          Type *my_governor_last = my_governor->get_type_refd_last();
          if (my_governor_last->get_typetype() == Type::T_COMPONENT &&
              !my_governor_last->is_compatible(t_type, NULL, NULL)) {
                u.expr.r1->error("Incompatible component types: operation "
                                 "`create' should refer to `%s' instead of "
                                 "`%s'",
                                 my_governor_last->get_typename().c_str(),
                                 t_type->get_typename().c_str());
                goto error;
          }
        }
        return t_type;
      } else {
        u.expr.r1->error("Type mismatch: reference to a component type was "
                         "expected in operation `create' instead of `%s'",
                         t_type->get_typename().c_str());
      }
    } else {
      u.expr.r1->error("Operation `create' should refer to a component type "
                       "instead of %s", t_ass->get_description().c_str());
    }
  error:
    set_valuetype(V_ERROR);
    return NULL;
  }

  void Value::chk_expr_comptype_compat()
  {
    if (valuetype != V_EXPR)
      FATAL_ERROR("Value::chk_expr_comptype_compat()");
    if (!my_governor || !my_scope) return;
    Type *my_governor_last = my_governor->get_type_refd_last();
    if (my_governor_last->get_typetype() != Type::T_COMPONENT) return;
    Type *t_comptype;
    switch (u.expr.v_optype) {
    case OPTYPE_COMP_MTC:
      t_comptype = my_scope->get_mtc_system_comptype(false);
      break;
    case OPTYPE_COMP_SYSTEM:
      t_comptype = my_scope->get_mtc_system_comptype(true);
      break;
    case OPTYPE_COMP_SELF: {
      Ttcn::RunsOnScope *t_ros = my_scope->get_scope_runs_on();
      t_comptype = t_ros ? t_ros->get_component_type() : 0;
      break; }
    default:
      FATAL_ERROR("Value::chk_expr_comptype_compat()");
      t_comptype = 0;
      break;
    }
    if (t_comptype
        && !my_governor_last->is_compatible(t_comptype, NULL, NULL)) {
      error("Incompatible component types: a component reference of "
            "type `%s' was expected, but `%s' has type `%s'",
            my_governor_last->get_typename().c_str(), get_opname(),
            t_comptype->get_typename().c_str());
      set_valuetype(V_ERROR);
    }
  }

  Type* Value::chk_expr_operand_compref(Value *val, const char *opnum,
                                       const char *opname, bool any_from)
  {
    Type* ret_val;
    if(valuetype == V_ERROR) return NULL;
    switch(val->get_valuetype()) {
    case V_INVOKE: {
      Error_Context cntxt(this, "In `%s' operation", opname);
      Value *v_last = val->get_value_refd_last();
      if(!v_last) goto error;
      ret_val = v_last->get_expr_governor(Type::EXPECTED_DYNAMIC_VALUE);
      if(!ret_val) goto error;
      Type* t = ret_val->get_type_refd_last();
      if(t->get_typetype() != (any_from ? Type::T_ARRAY : Type::T_COMPONENT)) {
        v_last->error("%s operand of operation `%s': Type mismatch:"
                      " component%s reference was expected instead of `%s'",
                      opnum, opname, any_from ? " array" : "",
                      t->get_typename().c_str());
        goto error;
      }
      if (any_from) {
        while (t->get_typetype() == Type::T_ARRAY) {
          t = t->get_ofType()->get_type_refd_last();
        }
        if (t->get_typetype() != Type::T_COMPONENT) {
          v_last->error("%s operand of operation `%s': Type mismatch:"
                        " component array reference was expected instead of array"
                        " of type `%s'", opnum, opname, t->get_typename().c_str());
          goto error;
        }
      }
      return ret_val; }
    case V_REFD: {
      Reference *ref = val->get_reference();
      Assignment *t_ass = ref->get_refd_assignment();
      Value *t_val = 0;
      if (!t_ass) goto error;
      switch(t_ass->get_asstype()) {
      case Assignment::A_CONST:
        t_val = t_ass->get_Value();
        // no break
      case Assignment::A_EXT_CONST:
      case Assignment::A_MODULEPAR:
      case Assignment::A_VAR:
      case Assignment::A_FUNCTION_RVAL:
      case Assignment::A_EXT_FUNCTION_RVAL:
      case Assignment::A_PAR_VAL_IN:
      case Assignment::A_PAR_VAL_OUT:
      case Assignment::A_PAR_VAL_INOUT: {
        ret_val=t_ass->get_Type()
          ->get_field_type(ref->get_subrefs(), Type::EXPECTED_DYNAMIC_VALUE);
        if(!ret_val) goto error;
        Type* t_type=ret_val->get_type_refd_last();
        if(t_type->get_typetype() != (any_from ? Type::T_ARRAY : Type::T_COMPONENT)) {
          ref->error("%s operand of operation `%s': Type mismatch:"
                     " component%s reference was expected instead of `%s'",
                     opnum, opname, any_from ? " array" : "",
                     t_type->get_typename().c_str());
          goto error;
        }
        if (any_from) {
          while (t_type->get_typetype() == Type::T_ARRAY) {
            t_type = t_type->get_ofType()->get_type_refd_last();
          }
          if (t_type->get_typetype() != Type::T_COMPONENT) {
            ref->error("%s operand of operation `%s': Type mismatch:"
                     " component array reference was expected instead of array"
                     " of type `%s'", opnum, opname, t_type->get_typename().c_str());
            goto error;
          }
        }
        break;}
      default:
        ref->error("%s operand of operation `%s' should be"
                   " component reference instead of %s",
                   opnum, opname, t_ass->get_description().c_str());
        goto error;
      }
      if (t_val) {
        ReferenceChain refch(this, "While searching referenced value");
        t_val = t_val->get_refd_sub_value(ref->get_subrefs(), 0, false, &refch);
        if (!t_val) return NULL;
        t_val = t_val->get_value_refd_last();
        if (t_val->valuetype != V_EXPR) return NULL;
        switch (t_val->u.expr.v_optype) {
        case OPTYPE_COMP_NULL:
          ref->error("%s operand of operation `%s' refers to `null' component "
	    "reference", opnum, opname);
	  goto error;
        case OPTYPE_COMP_MTC:
          ref->error("%s operand of operation `%s' refers to the component "
	    "reference of the `mtc'", opnum, opname);
	  goto error;
        case OPTYPE_COMP_SYSTEM:
          ref->error("%s operand of operation `%s' refers to the component "
	    "reference of the `system'", opnum, opname);
	  goto error;
        default:
          break;
        }
      }
      return ret_val;}
    default:
      FATAL_ERROR("Value::chk_expr_operand_compref()");
    }
  error:
    set_valuetype(V_ERROR);
    return NULL;
  }

  void Value::chk_expr_operand_tmrref(Ttcn::Ref_base *ref,
                                      const char *opnum,
                                      const char *opname,
                                      bool any_from)
  {
    if(valuetype==V_ERROR) return;
    // if(u.expr.state==EXPR_CHECKING_ERR) return;
    Assignment *t_ass = ref->get_refd_assignment();
    if(!t_ass) goto error;
    switch(t_ass->get_asstype()) {
    case Assignment::A_TIMER: {
      Ttcn::ArrayDimensions *t_dims = t_ass->get_Dimensions();
      if (t_dims) t_dims->chk_indices(ref, "timer", false,
	Type::EXPECTED_DYNAMIC_VALUE, any_from);
      else {
        if (any_from) {
          ref->error("%s operand of operation `%s': Reference to a timer "
            "array was expected instead of a timer", opnum, opname);
        }
        else if (ref->get_subrefs()) {
          ref->error("%s operand of operation `%s': "
            "Reference to single timer `%s' cannot have field or array "
            "sub-references", opnum, opname,
            t_ass->get_id().get_dispname().c_str());
          goto error;
        }
      }
      break; }
    case Assignment::A_PAR_TIMER:
      if (any_from) {
        ref->error("%s operand of operation `%s': Reference to a timer array "
          "was expected instead of a timer parameter",  opnum, opname);
      }
      if (ref->get_subrefs()) {
        ref->error("%s operand of operation `%s': "
		   "Reference to %s cannot have field or array sub-references",
		   opnum, opname, t_ass->get_description().c_str());
        goto error;
      }
      break;
    default:
      ref->error("%s operand of operation `%s' should be timer"
                 "%s instead of %s",
                 opnum, opname, any_from ? " array" : "",
                 t_ass->get_description().c_str());
      goto error;
    } // switch
    return;
  error:
    set_valuetype(V_ERROR);
  }

  void Value::chk_expr_operand_activate(Ttcn::Ref_base *ref,
                                        const char *,
                                        const char *opname)
  {
    if(valuetype==V_ERROR) return;
    // if(u.expr.state==EXPR_CHECKING_ERR) return;
    Ttcn::Ref_pard *t_ref_pard = dynamic_cast<Ttcn::Ref_pard*>(ref);
    if (!t_ref_pard) FATAL_ERROR("Value::chk_expr_operand_activate()");
    Error_Context cntxt(this, "In `%s' operation", opname);
    if (!t_ref_pard->chk_activate_argument()) set_valuetype(V_ERROR);
  }

  void Value::chk_expr_operand_activate_refd(Value *val,
                                            Ttcn::TemplateInstances* t_list2,
                                            Ttcn::ActualParList *&parlist,
                                            const char *,
                                            const char *opname)
  {
    if(valuetype==V_ERROR) return;
    Error_Context cntxt(this, "In `%s' operation", opname);
    Type *t = val->get_expr_governor_last();
    if (t) {
      switch (t->get_typetype()) {
      case Type::T_ERROR:
	set_valuetype(V_ERROR);
	break;
      case Type::T_ALTSTEP: {
	Ttcn::FormalParList *fp_list = t->get_fat_parameters();
	bool is_erroneous = fp_list->chk_actual_parlist(t_list2, parlist);
	if(is_erroneous) {
          delete parlist;
          parlist = 0;
          set_valuetype(V_ERROR);
	} else {
          parlist->set_fullname(get_fullname());
	  parlist->set_my_scope(get_my_scope());
          if (!fp_list->chk_activate_argument(parlist,
	      get_stringRepr().c_str())) set_valuetype(V_ERROR);
	}
	break; }
      default:
	error("Reference to an altstep was expected in the argument of "
	  "`derefers()' instead of `%s'", t->get_typename().c_str());
	set_valuetype(V_ERROR);
	break;
      }
    } else set_valuetype(V_ERROR);
  }

  void Value::chk_expr_operand_execute(Ttcn::Ref_base *ref, Value *val,
                                       const char *,
                                       const char *opname)
  {
    if(valuetype==V_ERROR) return;
    // if(u.expr.state==EXPR_CHECKING_ERR) return;
    Error_Context cntxt(this, "In `%s' operation", opname);
    Assignment *t_ass = ref->get_refd_assignment();
    bool error_flag = false;
    if (t_ass) {
      if (t_ass->get_asstype() != Common::Assignment::A_TESTCASE) {
	ref->error("Reference to a testcase was expected in the argument "
	  "instead of %s", t_ass->get_description().c_str());
	error_flag = true;
      }
    } else error_flag = true;
    if (val) {
      val->chk_expr_float(Type::EXPECTED_DYNAMIC_VALUE);
      Value *v_last = val->get_value_refd_last();
      switch (v_last->valuetype) {
      case V_REAL: {
        ttcn3float v_real = v_last->get_val_Real();
	if (v_real < 0.0) {
	  val->error("The testcase guard timer has negative value: `%s'",
	    Real2string(v_real).c_str());
	  error_flag = true;
	}
	break; }
      case V_ERROR:
        error_flag = true;
        break;
      default:
	break;
      }
    }
    if (error_flag) set_valuetype(V_ERROR);
  }

void Value::chk_expr_operand_execute_refd(Value *v1,
                                            Ttcn::TemplateInstances* t_list2,
                                            Ttcn::ActualParList *&parlist,
                                            Value *v3,
                                            const char *,
                                            const char *opname)
  {
    if(valuetype==V_ERROR) return;
    Error_Context cntxt(this, "In `%s' operation", opname);
    Type *t = v1->get_expr_governor_last();
    if (t) {
      switch (t->get_typetype()) {
      case Type::T_ERROR:
	set_valuetype(V_ERROR);
	break;
      case Type::T_TESTCASE: {
	Ttcn::FormalParList *fp_list = t->get_fat_parameters();
	bool is_erroneous = fp_list->chk_actual_parlist(t_list2, parlist);
	if(is_erroneous) {
          delete parlist;
          parlist = 0;
          set_valuetype(V_ERROR);
	} else {
          parlist->set_fullname(get_fullname());
	  parlist->set_my_scope(get_my_scope());
	}
	break; }
      default:
	v1->error("Reference to a value of type testcase was expected in the "
	  "argument of `derefers()' instead of `%s'",
	  t->get_typename().c_str());
	set_valuetype(V_ERROR);
	break;
      }
    } else set_valuetype(V_ERROR);
    if (v3) {
      v3->chk_expr_float(Type::EXPECTED_DYNAMIC_VALUE);
      Value *v_last = v3->get_value_refd_last();
      switch (v_last->valuetype) {
      case V_REAL: {
        ttcn3float v_real = v_last->get_val_Real();
        if(v_real < 0.0) {
          v3->error("The testcase guard timer has negative value: `%s'",
	    Real2string(v_real).c_str());
          set_valuetype(V_ERROR);
        }
        break; }
      case V_ERROR:
        set_valuetype(V_ERROR);
        break;
      default:
        break;
      }
    }
  }

  void Value::chk_invoke(Type::expected_value_t exp_val)
  {
    if(valuetype == V_ERROR) return;
    if(valuetype != V_INVOKE) FATAL_ERROR("Value::chk_invoke()");
    if(!u.invoke.t_list) return; //already checked
    Error_Context cntxt(this, "In `apply()' operation");
    Type *t = u.invoke.v->get_expr_governor_last();
    if (!t) {
      set_valuetype(V_ERROR);
      return;
    }
    switch (t->get_typetype()) {
    case Type::T_ERROR:
      set_valuetype(V_ERROR);
      return;
    case Type::T_FUNCTION:
      break;
    default:
      u.invoke.v->error("A value of type function was expected in the "
	"argument instead of `%s'", t->get_typename().c_str());
      set_valuetype(V_ERROR);
      return;
    }
    my_scope->chk_runs_on_clause(t, *this, "call");
    Ttcn::FormalParList *fp_list = t->get_fat_parameters();
    Ttcn::ActualParList *parlist = new Ttcn::ActualParList;
    bool is_erroneous = fp_list->fold_named_and_chk(u.invoke.t_list, parlist);
    delete u.invoke.t_list;
    u.invoke.t_list = 0;
    if(is_erroneous) {
      delete parlist;
      u.invoke.ap_list = 0;
    } else {
      parlist->set_fullname(get_fullname());
      parlist->set_my_scope(get_my_scope());
      u.invoke.ap_list = parlist;
    }
    switch (exp_val) {
    case Type::EXPECTED_CONSTANT:
      error("An evaluable constant value was expected instead of operation "
        "`apply()'");
      set_valuetype(V_ERROR);
      break;
    case Type::EXPECTED_STATIC_VALUE:
      error("A static value was expected instead of operation `apply()'");
      set_valuetype(V_ERROR);
      break;
    default:
      break;
    } // switch
  }

  void Value::chk_expr_eval_value(Value *val, Type &t,
                                  ReferenceChain *refch,
                                  Type::expected_value_t exp_val)
  {
    bool self_ref = false;
    if(valuetype==V_ERROR) return;
    // Commented out to report more errors :)
    // e.g.: while ( 2 + Nonexi03 > 2 + Nonexi04 ) {}
    // if(u.expr.state==EXPR_CHECKING_ERR) return;
    switch(val->get_valuetype()) {
    case V_REFD:
      self_ref = t.chk_this_refd_value(val, 0, exp_val, refch);
      break;
    case V_EXPR:
    case V_MACRO:
    case V_INVOKE:
      val->get_value_refd_last(refch, exp_val);
      break;
    default:
      break;
    } // switch
    if(val->get_valuetype()==V_ERROR) set_valuetype(V_ERROR);

    (void)self_ref;
  }

  void Value::chk_expr_eval_ti(TemplateInstance *ti, Type *type,
    ReferenceChain *refch, Type::expected_value_t exp_val)
  {
    bool self_ref = false;
    ti->chk(type);
    if (exp_val != Type::EXPECTED_TEMPLATE && ti->get_DerivedRef()) {
      ti->error("Reference to a %s value was expected instead of an in-line "
	"modified template",
        exp_val == Type::EXPECTED_CONSTANT ? "constant" : "static");
      set_valuetype(V_ERROR);
      return;
    }
    Template *templ = ti->get_Template();
    switch (templ->get_templatetype()) {
    case Template::TEMPLATE_REFD:
      // not foldable
      if (exp_val == Type::EXPECTED_TEMPLATE) {
	templ = templ->get_template_refd_last(refch);
	if (templ->get_templatetype() == Template::TEMPLATE_ERROR)
	  set_valuetype(V_ERROR);
      } else {
        ti->error("Reference to a %s value was expected instead of %s",
	  exp_val == Type::EXPECTED_CONSTANT ? "constant" : "static",
	  templ->get_reference()->get_refd_assignment()
	    ->get_description().c_str());
        set_valuetype(V_ERROR);
      }
      break;
    case Template::SPECIFIC_VALUE: {
      Value *val = templ->get_specific_value();
      switch (val->get_valuetype()) {
      case V_REFD:
        self_ref = type->chk_this_refd_value(val, 0, exp_val, refch);
        break;
      case V_EXPR:
        val->get_value_refd_last(refch, exp_val);
      default:
        break;
      } // switch
      if (val->get_valuetype() == V_ERROR) set_valuetype(V_ERROR);
      break; }
    case Template::TEMPLATE_ERROR:
      set_valuetype(V_ERROR);
      break;
    default:
      break;
    } // switch

    (void)self_ref;
  }

  void Value::chk_expr_val_int_pos0(Value *val, const char *opnum,
                                    const char *opname)
  {
    if(valuetype==V_ERROR) return;
    if(u.expr.state==EXPR_CHECKING_ERR) return;
    if(val->is_unfoldable()) return;
    if(*val->get_val_Int()<0) {
      val->error("%s operand of operation `%s' should not be negative",
                 opnum, opname);
      set_valuetype(V_ERROR);
    }
  }

  void Value::chk_expr_val_int_pos7bit(Value *val, const char *opnum,
                                       const char *opname)
  {
    if(valuetype==V_ERROR) return;
    if(u.expr.state==EXPR_CHECKING_ERR) return;
    if(val->is_unfoldable()) return;
    if(*val->get_val_Int()<0 || *val->get_val_Int()>127) {
      val->error("%s operand of operation `%s' should be in range 0..127",
                 opnum, opname);
      set_valuetype(V_ERROR);
    }
  }

  void Value::chk_expr_val_int_pos31bit(Value *val, const char *opnum,
                                        const char *opname)
  {
    if(valuetype==V_ERROR) return;
    if(u.expr.state==EXPR_CHECKING_ERR) return;
    if(val->is_unfoldable()) return;
    if(*val->get_val_Int()<0 || *val->get_val_Int()>2147483647) {
      val->error("%s operand of operation `%s' should be in range"
                 " 0..2147483647", opnum, opname);
      set_valuetype(V_ERROR);
    }
  }

  void Value::chk_expr_val_int_float_not0(Value *val, const char *opnum,
                                          const char *opname)
  {
    if(valuetype==V_ERROR) return;
    if(u.expr.state==EXPR_CHECKING_ERR) return;
    if(val->is_unfoldable()) return;
    if((val->get_expr_returntype()==Type::T_INT && *val->get_val_Int()==0)
      ||
      (val->get_expr_returntype()==Type::T_REAL && val->get_val_Real()==0.0))
    {
      val->error("%s operand of operation `%s' should not be zero",
        opnum, opname);
      set_valuetype(V_ERROR);
    }
  }

  void Value::chk_expr_val_large_int(Value *val, const char *opnum,
    const char *opname)
  {
    if (valuetype == V_ERROR) return;
    if (u.expr.state == EXPR_CHECKING_ERR) return;
    if (val->get_expr_returntype() != Type::T_INT) return;
    if (val->is_unfoldable()) return;
    const int_val_t *val_int = val->get_val_Int();
    if (*val_int > static_cast<Int>(INT_MAX)) {
      val->error("%s operand of operation `%s' should be less than `%d' "
        "instead of `%s'", opnum, opname, INT_MAX,
        (val_int->t_str()).c_str());
      set_valuetype(V_ERROR);
    }
  }

  void Value::chk_expr_val_len1(Value *val, const char *opnum,
                                const char *opname)
  {
    if(valuetype==V_ERROR) return;
    if(u.expr.state==EXPR_CHECKING_ERR) return;
    if(val->is_unfoldable()) return;
    if(val->get_val_strlen()!=1) {
      val->error("%s operand of operation `%s' should be of length 1",
                 opnum, opname);
      set_valuetype(V_ERROR);
    }
  }

  void Value::chk_expr_val_str_len_even(Value *val, const char *opnum,
                                        const char *opname)
  {
    if (valuetype == V_ERROR || u.expr.state == EXPR_CHECKING_ERR) return;
    Value *v_last = val->get_value_refd_last();
    if (v_last->valuetype == V_CSTR) {
      size_t len = v_last->get_val_strlen();
      if (len % 2) {
	val->error("%s operand of operation `%s' should contain even number "
	  "of characters instead of %lu", opnum, opname, (unsigned long) len);
	set_valuetype(V_ERROR);
      }
    } else if (v_last->valuetype == V_REFD) {
      Ttcn::FieldOrArrayRefs *t_subrefs = v_last->u.ref.ref->get_subrefs();
      if (t_subrefs && t_subrefs->refers_to_string_element()) {
	val->error("%s operand of operation `%s' should contain even number "
	  "of characters, but a string element contains 1", opnum, opname);
	set_valuetype(V_ERROR);
      }
    }
  }

  void Value::chk_expr_val_str_bindigits(Value *val, const char *opnum,
                                         const char *opname)
  {
    if(valuetype==V_ERROR) return;
    if(u.expr.state==EXPR_CHECKING_ERR) return;
    if(val->is_unfoldable()) return;
    const string& s=val->get_val_str();
    for(size_t i=0; i<s.size(); i++) {
      char c=s[i];
      if(!(c=='0' || c=='1')) {
        val->error("%s operand of operation `%s' can contain only"
                   " binary digits (position %lu is `%c')",
                   opnum, opname, (unsigned long) i, c);
        set_valuetype(V_ERROR);
        return;
      }
    }
  }

  void Value::chk_expr_val_str_hexdigits(Value *val, const char *opnum,
                                         const char *opname)
  {
    if(valuetype==V_ERROR) return;
    if(u.expr.state==EXPR_CHECKING_ERR) return;
    if(val->is_unfoldable()) return;
    const string& s=val->get_val_str();
    for(size_t i=0; i<s.size(); i++) {
      char c=s[i];
      if(!((c>='0' && c<='9') || (c>='A' && c<='F') || (c>='a' && c<='f'))) {
        val->error("%s operand of operation `%s' can contain only valid "
                   "hexadecimal digits (position %lu is `%c')",
                   opnum, opname, (unsigned long) i, c);
        set_valuetype(V_ERROR);
        return;
      }
    }
  }

  void Value::chk_expr_val_str_7bitoctets(Value *val, const char *opnum,
                                         const char *opname)
  {
    if (valuetype == V_ERROR || u.expr.state == EXPR_CHECKING_ERR) return;
    Value *v = val->get_value_refd_last();
    if (v->valuetype != V_OSTR) return;
    const string& s = val->get_val_str();
    size_t n_octets = s.size() / 2;
    for (size_t i = 0; i < n_octets; i++) {
      char c = s[2 * i];
      if (!(c >= '0' && c <= '7')) {
        val->error("%s operand of operation `%s' shall consist of octets "
	  "within the range 00 .. 7F, but the string `%s'O contains octet "
	  "%c%c at index %lu", opnum, opname, s.c_str(), c, s[2 * i + 1],
          (unsigned long) i);
        set_valuetype(V_ERROR);
        return;
      }
    }
  }

  void Value::chk_expr_val_str_int(Value *val, const char *opnum,
                                   const char *opname)
  {
    if (valuetype == V_ERROR || u.expr.state == EXPR_CHECKING_ERR) return;
    Value *v_last = val->get_value_refd_last();
    if (v_last->valuetype != V_CSTR) return;
    const string& s = v_last->get_val_str();
    enum { S_INITIAL, S_INITIAL_WS, S_FIRST, S_ZERO, S_MORE, S_END, S_ERR }
      state = S_INITIAL;
    // state: expected characters
    // S_INITIAL, S_INITIAL_WS: +, -, first digit, leading whitespace
    // S_FIRST: first digit
    // S_ZERO, S_MORE: more digit(s), trailing whitespace
    // S_END: trailing whitespace
    // S_ERR: error was found, stop
    for (size_t i = 0; i < s.size(); i++) {
      char c = s[i];
      switch (state) {
      case S_INITIAL:
      case S_INITIAL_WS:
	if (c == '+' || c == '-') state = S_FIRST;
	else if (c == '0') state = S_ZERO;
	else if (c >= '1' && c <= '9') state = S_MORE;
	else if (string::is_whitespace(c)) {
	  if (state == S_INITIAL) {
	    val->warning("Leading whitespace was detected and ignored in the "
	      "operand of operation `%s'", opname);
	    state = S_INITIAL_WS;
	  }
	} else state = S_ERR;
	break;
      case S_FIRST:
	if (c == '0') state = S_ZERO;
	else if (c >= '1' && c <= '9') state = S_MORE;
	else state = S_ERR;
	break;
      case S_ZERO:
	if (c >= '0' && c <= '9') {
	  val->warning("Leading zero digit was detected and ignored in the "
	    "operand of operation `%s'", opname);
	  state = S_MORE;
	} else if (string::is_whitespace(c)) state = S_END;
	else state = S_ERR;
	break;
      case S_MORE:
	if (c >= '0' && c <= '9') {}
	else if (string::is_whitespace(c)) state = S_END;
	else state = S_ERR;
	break;
      case S_END:
	if (!string::is_whitespace(c)) state = S_ERR;
	break;
      default:
	break;
      }
      if (state == S_ERR) {
	if (string::is_printable(c)) {
	  val->error("%s operand of operation `%s' should be a string "
	    "containing a valid integer value, but invalid character `%c' "
	    "was detected at index %lu", opnum, opname, c, (unsigned long) i);
	} else {
	  val->error("%s operand of operation `%s' should be a string "
	    "containing a valid integer value, but invalid character with "
	    "character code %u was detected at index %lu", opnum, opname, c,
            (unsigned long) i);
	}
	set_valuetype(V_ERROR);
	break;
      }
    }
    switch (state) {
    case S_INITIAL:
    case S_INITIAL_WS:
      val->error("%s operand of operation `%s' should be a string containing a "
	"valid integer value instead of an empty string", opnum, opname);
      set_valuetype(V_ERROR);
      break;
    case S_FIRST:
      val->error("%s operand of operation `%s' should be a string containing a "
	"valid integer value, but only a sign character was detected", opnum,
	opname);
      set_valuetype(V_ERROR);
      break;
    case S_END:
      val->warning("Trailing whitespace was detected and ignored in the "
	"operand of operation `%s'", opname);
      break;
    default:
      break;
    }
  }

  void Value::chk_expr_val_str_float(Value *val, const char *opnum,
                                     const char *opname)
  {
    if (valuetype == V_ERROR || u.expr.state == EXPR_CHECKING_ERR) return;
    Value *v_last = val->get_value_refd_last();
    if (v_last->valuetype == V_REFD) {
      Ttcn::FieldOrArrayRefs *t_subrefs = v_last->u.ref.ref->get_subrefs();
      if (t_subrefs && t_subrefs->refers_to_string_element()) {
	val->error("%s operand of operation `%s' should be a string containing "
	  "a valid float value instead of a string element, which cannot "
	  "represent a floating point number", opnum, opname);
	set_valuetype(V_ERROR);
      }
      return;
    } else if (v_last->valuetype != V_CSTR) return;
    const string& s = v_last->get_val_str();
    enum { S_INITIAL, S_INITIAL_WS, S_FIRST_M, S_ZERO_M, S_MORE_M, S_FIRST_F,
      S_MORE_F, S_INITIAL_E, S_FIRST_E, S_ZERO_E, S_MORE_E, S_END, S_ERR }
      state = S_INITIAL;
    // state: expected characters
    // S_INITIAL, S_INITIAL_WS: +, -, first digit of integer part in mantissa,
    //                          leading whitespace
    // S_FIRST_M: first digit of integer part in mantissa
    // S_ZERO_M, S_MORE_M: more digits of mantissa, decimal dot, E
    // S_FIRST_F: first digit of fraction
    // S_MORE_F: more digits of fraction, E, trailing whitespace
    // S_INITIAL_E: +, -, first digit of exponent
    // S_FIRST_E: first digit of exponent
    // S_ZERO_E, S_MORE_E: more digits of exponent, trailing whitespace
    // S_END: trailing whitespace
    // S_ERR: error was found, stop
    for (size_t i = 0; i < s.size(); i++) {
      char c = s[i];
      switch (state) {
      case S_INITIAL:
      case S_INITIAL_WS:
	if (c == '+' || c == '-') state = S_FIRST_M;
	else if (c == '0') state = S_ZERO_M;
	else if (c >= '1' && c <= '9') state = S_MORE_M;
	else if (string::is_whitespace(c)) {
	  if (state == S_INITIAL) {
	    val->warning("Leading whitespace was detected and ignored in the "
	      "operand of operation `%s'", opname);
	    state = S_INITIAL_WS;
	  }
	} else state = S_ERR;
	break;
      case S_FIRST_M:
	if (c == '0') state = S_ZERO_M;
	else if (c >= '1' && c <= '9') state = S_MORE_M;
	else state = S_ERR;
	break;
      case S_ZERO_M:
	if (c == '.') state = S_FIRST_F;
	else if (c == 'E' || c == 'e') state = S_INITIAL_E;
	else if (c >= '0' && c <= '9') {
	  val->warning("Leading zero digit was detected and ignored in the "
	    "mantissa of the operand of operation `%s'", opname);
	  state = S_MORE_M;
	} else state = S_ERR;
	break;
      case S_MORE_M:
	if (c == '.') state = S_FIRST_F;
	else if (c == 'E' || c == 'e') state = S_INITIAL_E;
	else if (c >= '0' && c <= '9') {}
	else state = S_ERR;
	break;
      case S_FIRST_F:
	if (c >= '0' && c <= '9') state = S_MORE_F;
	else state = S_ERR;
	break;
      case S_MORE_F:
	if (c == 'E' || c == 'e') state = S_INITIAL_E;
	else if (c >= '0' && c <= '9') {}
	else if (string::is_whitespace(c)) state = S_END;
	else state = S_ERR;
	break;
      case S_INITIAL_E:
	if (c == '+' || c == '-') state = S_FIRST_E;
	else if (c == '0') state = S_ZERO_E;
	else if (c >= '1' && c <= '9') state = S_MORE_E;
	else state = S_ERR;
	break;
      case S_FIRST_E:
	if (c == '0') state = S_ZERO_E;
	else if (c >= '1' && c <= '9') state = S_MORE_E;
	else state = S_ERR;
	break;
      case S_ZERO_E:
	if (c >= '0' && c <= '9') {
	  val->warning("Leading zero digit was detected and ignored in the "
	    "exponent of the operand of operation `%s'", opname);
	  state = S_MORE_E;
	} else if (string::is_whitespace(c)) state = S_END;
	else state = S_ERR;
	break;
      case S_MORE_E:
	if (c >= '0' && c <= '9') {}
	else if (string::is_whitespace(c)) state = S_END;
	else state = S_ERR;
	break;
      case S_END:
	if (!string::is_whitespace(c)) state = S_ERR;
	break;
      default:
	break;
      }
      if (state == S_ERR) {
	if (string::is_printable(c)) {
	  val->error("%s operand of operation `%s' should be a string "
	    "containing a valid float value, but invalid character `%c' "
	    "was detected at index %lu", opnum, opname, c, (unsigned long) i);
	} else {
	  val->error("%s operand of operation `%s' should be a string "
	    "containing a valid float value, but invalid character with "
	    "character code %u was detected at index %lu", opnum, opname, c,
            (unsigned long) i);
	}
	set_valuetype(V_ERROR);
	break;
      }
    }
    switch (state) {
    case S_INITIAL:
    case S_INITIAL_WS:
      val->error("%s operand of operation `%s' should be a string containing a "
	"valid float value instead of an empty string", opnum, opname);
      set_valuetype(V_ERROR);
      break;
    case S_FIRST_M:
      val->error("%s operand of operation `%s' should be a string containing a "
	"valid float value, but only a sign character was detected", opnum,
	opname);
      set_valuetype(V_ERROR);
      break;
    case S_ZERO_M:
    case S_MORE_M:
      // HL67862: Missing decimal dot allowed for str2float
      break;
    case S_FIRST_F:
      // HL67862: Missing fraction part is allowed for str2float
      break;
    case S_INITIAL_E:
    case S_FIRST_E:
      val->error("%s operand of operation `%s' should be a string containing a "
	"valid float value, but the exponent is missing after the `E' sign",
	opnum, opname);
      set_valuetype(V_ERROR);
      break;
    case S_END:
      val->warning("Trailing whitespace was detected and ignored in the "
	"operand of operation `%s'", opname);
      break;
    default:
      break;
    }
  }

  void Value::chk_expr_val_ustr_7bitchars(Value *val, const char *opnum,
                                          const char *opname)
  {
    if (valuetype == V_ERROR || u.expr.state == EXPR_CHECKING_ERR) return;
    Value *v = val->get_value_refd_last();
    if (v->valuetype != V_USTR) return;
    const ustring& us = v->get_val_ustr();
    for (size_t i = 0; i < us.size(); i++) {
      const ustring::universal_char& uchar = us[i];
      if (uchar.group != 0 || uchar.plane != 0 || uchar.row != 0 ||
	  uchar.cell > 127) {
        val->error("%s operand of operation `%s' shall consist of characters "
	  "within the range char(0, 0, 0, 0) .. char(0, 0, 0, 127), but the "
	  "string %s contains character char(%u, %u, %u, %u) at index %lu",
	  opnum, opname, us.get_stringRepr().c_str(), uchar.group, uchar.plane,
	  uchar.row, uchar.cell, (unsigned long) i);
        set_valuetype(V_ERROR);
        return;
      }
    }
  }

  void Value::chk_expr_val_bitstr_intsize(Value *val, const char *opnum,
                                          const char *opname)
  {
    if(valuetype==V_ERROR) return;
    if(u.expr.state==EXPR_CHECKING_ERR) return;
    if(val->is_unfoldable()) return;
    const string& bstr=val->get_val_str();
    // see also PredefFunc.cc::bit2int()
    size_t nof_bits = bstr.size();
    // skip the leading zeros
    size_t start_index = 0;
    while (start_index < nof_bits && bstr[start_index] == '0') start_index++;
    // check whether the remaining bits fit in Int
    if (nof_bits - start_index > 8 * sizeof(Int) - 1) {
      val->error("%s operand of operation `%s' is too large (maximum number"
                 " of bits in integer is %lu)",
                 opnum, opname, (unsigned long) (8 * sizeof(Int) - 1));
      set_valuetype(V_ERROR);
    }
  }

  void Value::chk_expr_val_hexstr_intsize(Value *val, const char *opnum,
                                          const char *opname)
  {
    if(valuetype==V_ERROR) return;
    if(u.expr.state==EXPR_CHECKING_ERR) return;
    if(val->is_unfoldable()) return;
    const string& hstr=val->get_val_str();
    // see also PredefFunc.cc::hex2int()
    size_t nof_digits = hstr.size();
    // skip the leading zeros
    size_t start_index = 0;
    while (start_index < nof_digits && hstr[start_index] == '0') start_index++;
    // check whether the remaining hex digits fit in Int
    if (nof_digits - start_index > 2 * sizeof(Int) ||
	(nof_digits - start_index == 2 * sizeof(Int) &&
	 char_to_hexdigit(hstr[start_index]) > 7)) {
      val->error("%s operand of operation `%s' is too large (maximum number"
                 " of bits in integer is %lu)",
                 opnum, opname, (unsigned long) (8 * sizeof(Int) - 1));
      set_valuetype(V_ERROR);
    }
  }

  void Value::chk_expr_operands_int2binstr()
  {
    if (valuetype == V_ERROR || u.expr.state == EXPR_CHECKING_ERR) return;
    if (u.expr.v1->is_unfoldable()) return;
    if (u.expr.v2->is_unfoldable()) return;
    // It is already checked that i1 and i2 are non-negative.
    Error_Context cntxt(this, "In operation `%s'", get_opname());
    const int_val_t *i1 = u.expr.v1->get_val_Int();
    const int_val_t *i2 = u.expr.v2->get_val_Int();
    if (!i2->is_native()) {
      u.expr.v2->error("The length of the resulting string is too large for "
        "being represented in memory");
      set_valuetype(V_ERROR);
      return;
    }
    Int nof_bits = i2->get_val();
    if (u.expr.v1->is_unfoldable()) return;
    switch (u.expr.v_optype) {
    case OPTYPE_INT2BIT:
      break;
    case OPTYPE_INT2HEX:
      nof_bits *= 4;
      break;
    case OPTYPE_INT2OCT:
      nof_bits *= 8;
      break;
    default:
      FATAL_ERROR("Value::chk_expr_operands_int2binstr()");
    }
    if (*i1 >> nof_bits > 0) {  // Expensive?
      u.expr.v1->error("Value %s does not fit in length %s",
        i1->t_str().c_str(), i2->t_str().c_str());
      set_valuetype(V_ERROR);
    }
  }

  void Value::chk_expr_operands_str_samelen()
  {
    if(valuetype==V_ERROR) return;
    if(u.expr.state==EXPR_CHECKING_ERR) return;
    Value *v1=u.expr.v1;
    if(v1->is_unfoldable()) return;
    Value *v2=u.expr.v2;
    if(v2->is_unfoldable()) return;
    Error_Context cntxt(this, "In operation `%s'", get_opname());
    size_t i1=v1->get_val_strlen();
    size_t i2=v2->get_val_strlen();
    if(i1!=i2) {
      error("The operands should have the same length");
      set_valuetype(V_ERROR);
    }
  }

  void Value::chk_expr_operands_replace()
  {
    // The fourth operand doesn't need to be checked at all here.
    if(valuetype==V_ERROR) return;
    if(u.expr.state==EXPR_CHECKING_ERR) return;
    Value* v1 = u.expr.ti1->get_specific_value();
    if (!v1) return;

    Error_Context cntxt(this, "In operation `%s'", get_opname());
    size_t list_len = 0;
    bool list_len_known = false;
    if (v1->valuetype == V_REFD) {
      Ttcn::FieldOrArrayRefs *subrefs = v1->u.ref.ref->get_subrefs();
      if (subrefs && subrefs->refers_to_string_element()) {
        warning("Replacing a string element does not make any sense");
        list_len = 1;
        list_len_known = true;
      }
    }
    if (!v1->is_unfoldable()) {
      list_len = v1->is_string_type(Type::EXPECTED_TEMPLATE) ?
        v1->get_val_strlen() : v1->get_value_refd_last()->get_nof_comps();
      list_len_known = true;
    }
    if (!list_len_known) return;
    if (u.expr.v2->is_unfoldable()) {
      if (!u.expr.v3->is_unfoldable()) {
        const int_val_t *len_int_3 = u.expr.v3->get_val_Int();
        if (*len_int_3 > static_cast<Int>(list_len)) {
          error("Third operand `len' (%s) is greater than the length of "
            "the first operand (%lu)", (len_int_3->t_str()).c_str(),
            (unsigned long)list_len);
          set_valuetype(V_ERROR);
        }
      }
    } else {
      const int_val_t *index_int_2 = u.expr.v2->get_val_Int();
      if (u.expr.v3->is_unfoldable()) {
        if (*index_int_2 > static_cast<Int>(list_len)) {
          error("Second operand `index' (%s) is greater than the length of "
            "the first operand (%lu)", (index_int_2->t_str()).c_str(),
            (unsigned long)list_len);
          set_valuetype(V_ERROR);
        }
      } else {
        const int_val_t *len_int_3 = u.expr.v3->get_val_Int();
        if (*index_int_2 + *len_int_3 > static_cast<Int>(list_len)) {
          error("The sum of second operand `index' (%s) and third operand "
            "`len' (%s) is greater than the length of the first operand (%lu)",
            (index_int_2->t_str()).c_str(), (len_int_3->t_str()).c_str(),
            (unsigned long)list_len);
          set_valuetype(V_ERROR);
        }
      }
    }
  }

  void Value::chk_expr_operands_substr()
  {
    if(valuetype==V_ERROR) return;
    if(u.expr.state==EXPR_CHECKING_ERR) return;
    Value* v1 = u.expr.ti1->get_specific_value();
    if (!v1) return;

    Error_Context cntxt(this, "In operation `%s'", get_opname());
    size_t list_len = 0;
    bool list_len_known = false;
    if (v1->valuetype == V_REFD) {
      Ttcn::FieldOrArrayRefs *subrefs = v1->u.ref.ref->get_subrefs();
      if (subrefs && subrefs->refers_to_string_element()) {
        warning("Taking the substring of a string element does not make any "
                "sense");
        list_len = 1;
        list_len_known = true;
      }
    }
    if (!list_len_known && !v1->is_unfoldable()) {
      list_len = v1->is_string_type(Type::EXPECTED_TEMPLATE) ?
        v1->get_val_strlen() : v1->get_value_refd_last()->get_nof_comps();
      list_len_known = true;
    }
    // Do nothing if the length of the first operand is unknown.
    if (!list_len_known) return;
    if (u.expr.v2->is_unfoldable()) {
      if (!u.expr.v3->is_unfoldable()) {
        const int_val_t *returncount_int_3 = u.expr.v3->get_val_Int();
        // Only the third operand is known.
        if (*returncount_int_3 > static_cast<Int>(list_len)) {
          error("Third operand `returncount' (%s) is greater than the "
            "length of the first operand (%lu)",
            (returncount_int_3->t_str()).c_str(), (unsigned long)list_len);
          set_valuetype(V_ERROR);
        }
      }
    } else {
      const int_val_t *index_int_2 = u.expr.v2->get_val_Int();
      if (u.expr.v3->is_unfoldable()) {
        // Only the second operand is known.
	if (*index_int_2 > static_cast<Int>(list_len)) {
	  error("Second operand `index' (%s) is greater than the length "
	    "of the first operand (%lu)", (index_int_2->t_str()).c_str(),
            (unsigned long)list_len);
	  set_valuetype(V_ERROR);
	}
      } else {
        // Both second and third operands are known.
        const int_val_t *returncount_int_3 = u.expr.v3->get_val_Int();
        if (*index_int_2 + *returncount_int_3 > static_cast<Int>(list_len)) {
          error("The sum of second operand `index' (%s) and third operand "
            "`returncount' (%s) is greater than the length of the first operand "
            "(%lu)", (index_int_2->t_str()).c_str(),
            (returncount_int_3->t_str()).c_str(), (unsigned long)list_len);
          set_valuetype(V_ERROR);
        }
      }
    }
  }

  void Value::chk_expr_operands_regexp()
  {
    if (valuetype == V_ERROR || u.expr.state == EXPR_CHECKING_ERR) return;
    Value* v1 = u.expr.ti1->get_specific_value();
    Value* v2 = u.expr.t2->get_specific_value();
    if (!v1 || !v2) return;

    Error_Context cntxt(this, "In operation `regexp()'");
    Value* v1_last = v1->get_value_refd_last();
    if (v1_last->valuetype == V_CSTR) {
      // the input string is available at compile time
      const string& instr = v1_last->get_val_str();
      const char *input_str = instr.c_str();
      size_t instr_len = strlen(input_str);
      if (instr_len < instr.size()) {
	v1->warning("The first operand of `regexp()' contains a "
	  "character with character code zero at index %s. The rest of the "
	  "string will be ignored during matching",
	  Int2string(instr_len).c_str());
      }
    }

    size_t nof_groups = 0;
    Value *v2_last = v2->get_value_refd_last();

    if (v2_last->valuetype == V_CSTR) {
      // the pattern is available at compile time
      const string& expression = v2_last->get_val_str();
      const char *pattern_str = expression.c_str();
      size_t pattern_len = strlen(pattern_str);
      if (pattern_len < expression.size()) {
	v2->warning("The second operand of `regexp()' contains a "
	  "character with character code zero at index %s. The rest of the "
	  "string will be ignored during matching",
	  Int2string(pattern_len).c_str());
      }
      char *posix_str;
      {
	Error_Context cntxt2(v2, "In character string pattern");
	posix_str = TTCN_pattern_to_regexp(pattern_str);
      }
      if (posix_str != NULL) {
	regex_t posix_regexp;
	int ret_val = regcomp(&posix_regexp, posix_str, REG_EXTENDED);
	if (ret_val != 0) {
	  char msg[512];
	  regerror(ret_val, &posix_regexp, msg, sizeof(msg));
	  FATAL_ERROR("Value::chk_expr_operands_regexp(): " \
	    "regcomp() failed: %s", msg);
	}
	if (posix_regexp.re_nsub > 0) nof_groups = posix_regexp.re_nsub;
	else {
	  v2->error("The character pattern in the second operand of "
	    "`regexp()' does not contain any groups");
	  set_valuetype(V_ERROR);
	}
	regfree(&posix_regexp);
	Free(posix_str);
      } else {
        // the pattern is faulty
	// the error has been reported by TTCN_pattern_to_regexp
	set_valuetype(V_ERROR);
      }
    }
    if (nof_groups > 0) {
      Value *v3 = u.expr.v3->get_value_refd_last();
      if (v3->valuetype == V_INT) {
	// the group number is available at compile time
        const int_val_t *groupno_int = v3->get_val_Int();
        if (*groupno_int >= static_cast<Int>(nof_groups)) {
          u.expr.v3->error("The the third operand of `regexp()' is too "
            "large: The requested group index is %s, but the pattern "
            "contains only %s group%s", (groupno_int->t_str()).c_str(),
            Int2string(nof_groups).c_str(), nof_groups > 1 ? "s" : "");
          set_valuetype(V_ERROR);
        }
      }
    }
  }

  void Value::chk_expr_operands_ischosen(ReferenceChain *refch,
    Type::expected_value_t exp_val)
  {
    const char *opname = get_opname();
    Error_Context cntxt(this, "In the operand of operation `%s'", opname);
    Type *t_governor;
    const Location *loc;
    bool error_flag = false;
    switch (u.expr.v_optype) {
    case OPTYPE_ISCHOSEN_V:
      // u.expr.v1 is always a referenced value
      t_governor = u.expr.v1->get_expr_governor(exp_val);
      if (t_governor) {
	u.expr.v1->set_my_governor(t_governor);
	t_governor->chk_this_refd_value(u.expr.v1, 0, exp_val, refch);
	if (u.expr.v1->valuetype == V_ERROR) error_flag = true;
      } else error_flag = true;
      loc = u.expr.v1;
      break;
    case OPTYPE_ISCHOSEN_T:
      // u.expr.t1 is always a referenced template
      if (exp_val == Type::EXPECTED_DYNAMIC_VALUE)
	exp_val = Type::EXPECTED_TEMPLATE;
      t_governor = u.expr.t1->get_expr_governor(exp_val);
      if (t_governor) {
	u.expr.t1->set_my_governor(t_governor);
	//
	// FIXME: commenting out the 2 lines below "fixes" the ischosen for HQ46602
	//
	u.expr.t1->get_template_refd_last(refch);
	if (u.expr.t1->get_templatetype() == Template::TEMPLATE_ERROR)
	  error_flag = true;
      } else error_flag = true;
      if (exp_val != Type::EXPECTED_TEMPLATE) {
	u.expr.t1->error("Reference to a %s value was expected instead of %s",
	  exp_val == Type::EXPECTED_CONSTANT ? "constant" : "static",
	  u.expr.t1->get_reference()->get_refd_assignment()
	    ->get_description().c_str());
	error_flag = true;
      }
      loc = u.expr.t1;
      break;
    default:
      FATAL_ERROR("Value::chk_expr_operands_ischosen()");
      t_governor = 0;
      loc = 0;
    }
    if (t_governor) {
      t_governor = t_governor->get_type_refd_last();
      switch (t_governor->get_typetype()) {
      case Type::T_ERROR:
        error_flag = true;
        break;
      case Type::T_CHOICE_A:
      case Type::T_CHOICE_T:
      case Type::T_ANYTYPE:
      case Type::T_OPENTYPE:
	if (!t_governor->has_comp_withName(*u.expr.i2)) {
	  error(t_governor->get_typetype()==Type::T_ANYTYPE ?
	    "%s does not have a field named `%s'"   :
	    "Union type `%s' does not have a field named `%s'",
	    t_governor->get_typename().c_str(),
	    u.expr.i2->get_dispname().c_str());
	  error_flag = true;
	}
        break;
      default:
	loc->error("The operand of operation `%s' should be a union value "
	  "or template instead of `%s'", opname,
	  t_governor->get_typename().c_str());
        error_flag = true;
        break;
      }
    }
    if (error_flag) set_valuetype(V_ERROR);
  }

  void Value::chk_expr_operand_encode(ReferenceChain *refch,
    Type::expected_value_t exp_val) {

    Error_Context cntxt(this, "In the parameter of %s",
      u.expr.v_optype == OPTYPE_ENCVALUE_UNICHAR ? "encvalue_unichar()" : "encvalue()");
    Type t_chk(Type::T_ERROR);
    Type* t_type;

    Type::expected_value_t ti_exp_val = exp_val;
    if (ti_exp_val == Type::EXPECTED_DYNAMIC_VALUE)
      ti_exp_val = Type::EXPECTED_TEMPLATE;

    t_type = chk_expr_operands_ti(u.expr.ti1, ti_exp_val);
    if (t_type) {
      chk_expr_eval_ti(u.expr.ti1, t_type, refch, ti_exp_val);
      if (valuetype!=V_ERROR)
        u.expr.ti1->get_Template()->chk_specific_value(false);
      t_type = t_type->get_type_refd_last();
    } else {
      error("Cannot determine type of value");
      goto error;
    }
    
    // todo: fix this
    /*if (u.expr.par1_is_value && u.expr.v1->get_valuetype() != V_REFD) {
      error("Expecting a value of a type with coding attributes in first"
        "parameter of encvalue() which belongs to a generic type '%s'",
        t_type->get_typename().c_str());
      goto error;
    }*/

    if(!disable_attribute_validation()) {
      t_type->chk_coding(true, my_scope->get_scope_mod());
    }
    
    switch (t_type->get_typetype()) {
    case Type::T_UNDEF:
    case Type::T_ERROR:
    case Type::T_NULL:
    case Type::T_REFD:
    case Type::T_REFDSPEC:
    case Type::T_SELTYPE:
    case Type::T_VERDICT:
    case Type::T_PORT:
    case Type::T_COMPONENT:
    case Type::T_DEFAULT:
    case Type::T_SIGNATURE:
    case Type::T_FUNCTION:
    case Type::T_ALTSTEP:
    case Type::T_TESTCASE:
      error("Type of parameter of encvalue() cannot be '%s'",
        t_type->get_typename().c_str());
      goto error;
    default:
      break;
    }
    return;
error:
    set_valuetype(V_ERROR);
  }

  void Value::chk_expr_operands_decode(operationtype_t p_optype)
  {
    Error_Context cntxt(this, "In the parameters of %s",
      p_optype == OPTYPE_DECVALUE_UNICHAR ? "decvalue_unichar()" : "decvalue()");
    Ttcn::Ref_base* ref = u.expr.r1;
    Ttcn::FieldOrArrayRefs* t_subrefs = ref->get_subrefs();
    Type* t_type = 0;
    Assignment* t_ass = ref->get_refd_assignment();

    if (!t_ass) {
      error("Could not determine the assignment for first parameter");
      goto error;
    }
    switch (t_ass->get_asstype()) {
    case Assignment::A_PAR_VAL_IN:
      t_ass->use_as_lvalue(*this);
      break;
    case Assignment::A_CONST:
    case Assignment::A_EXT_CONST:
    case Assignment::A_MODULEPAR:
    case Assignment::A_MODULEPAR_TEMP:
    case Assignment::A_TEMPLATE:
      ref->error("Reference to '%s' cannot be used as the first operand of "
                 "the 'decvalue' operation", t_ass->get_assname());
      goto error;
      break;
    case Assignment::A_VAR:
    case Assignment::A_PAR_VAL_OUT:
    case Assignment::A_PAR_VAL_INOUT:
      break;
    case Assignment::A_VAR_TEMPLATE:
    case Assignment::A_PAR_TEMPL_IN:
    case Assignment::A_PAR_TEMPL_OUT:
    case Assignment::A_PAR_TEMPL_INOUT: {
      Template* t = new Template(ref->clone());
      t->set_location(*ref);
      t->set_my_scope(get_my_scope());
      t->set_fullname(get_fullname()+".<operand>");
      Template* t_last = t->get_template_refd_last();
      if (t_last->get_templatetype() != Template::SPECIFIC_VALUE
          && t_last != t) {
        ref->error("Specific value template was expected instead of '%s'.",
          t->get_template_refd_last()->get_templatetype_str());
        delete t;
        goto error;
      }
      delete t;
      break; }
    default:
      ref->error("Reference to '%s' cannot be used.", t_ass->get_assname());
      goto error;
    }
    t_type = t_ass->get_Type()->get_field_type(t_subrefs,
                                                 Type::EXPECTED_DYNAMIC_VALUE);
    if (!t_type) {
      goto error;
    }
    switch(p_optype) {
      case OPTYPE_DECODE:
        if (t_type->get_type_refd_last()->get_typetype() != Type::T_BSTR){
          error("First parameter has to be a bitstring");
          goto error;
        }
        break;
      case OPTYPE_DECVALUE_UNICHAR:
        if (t_type->get_type_refd_last()->get_typetype() != Type::T_USTR){
          error("First parameter has to be a universal charstring");
          goto error;
        }
        break; 
      default:
        FATAL_ERROR("Value::chk_expr_decode_operands()");
        break;
    }

    ref = u.expr.r2;
    t_subrefs = ref->get_subrefs();
    t_ass = ref->get_refd_assignment();

    if (!t_ass) {
      error("Could not determine the assignment for second parameter");
      goto error;
    }
    // Extra check for HM59355.
    switch (t_ass->get_asstype()) {
    case Assignment::A_VAR:
    case Assignment::A_PAR_VAL_IN:
    case Assignment::A_PAR_VAL_OUT:
    case Assignment::A_PAR_VAL_INOUT:
      break;
    default:
      ref->error("Reference to '%s' cannot be used.", t_ass->get_assname());
      goto error;
    }
    t_type = t_ass->get_Type()->get_field_type(t_subrefs,
                                               Type::EXPECTED_DYNAMIC_VALUE);
    if (!t_type) {
      goto error;
    }
    t_type = t_type->get_type_refd_last();
    switch (t_type->get_typetype()) {
    case Type::T_UNDEF:
    case Type::T_ERROR:
    case Type::T_NULL:
    case Type::T_REFD:
    case Type::T_REFDSPEC:
    case Type::T_SELTYPE:
    case Type::T_VERDICT:
    case Type::T_PORT:
    case Type::T_COMPONENT:
    case Type::T_DEFAULT:
    case Type::T_SIGNATURE:
    case Type::T_FUNCTION:
    case Type::T_ALTSTEP:
    case Type::T_TESTCASE:
      error("Type of second parameter cannot be %s",
            t_type->get_typename().c_str());
      goto error;
    default:
      break;
    }

    if(!disable_attribute_validation()) {
      t_type->chk_coding(false, my_scope->get_scope_mod());
    }

    return;
  error:
    set_valuetype(V_ERROR);
  }

  void Value::chk_expr_omit_comparison(Type::expected_value_t exp_val)
  {
    Ttcn::FieldOrArrayRefs *subrefs;
    Identifier *field_id = 0;
    Assignment *t_ass;
    Type *t_type;
    if (valuetype == V_ERROR) return;
    else if (valuetype != V_REFD) {
      error("Only a referenced value can be compared with `omit'");
      goto error;
    }
    subrefs = u.ref.ref->get_subrefs();
    if (subrefs) field_id = subrefs->remove_last_field();
    if (!field_id) {
      error("Only a reference pointing to an optional record or set field "
	"can be compared with `omit'");
      goto error;
    }
    t_ass = u.ref.ref->get_refd_assignment();
    if (!t_ass) goto error;
    t_type = t_ass->get_Type()->get_field_type(subrefs, exp_val);
    if (!t_type) goto error;
    t_type = t_type->get_type_refd_last();
    switch (t_type->get_typetype()) {
    case Type::T_ERROR:
      goto error;
    case Type::T_SEQ_A:
    case Type::T_SEQ_T:
    case Type::T_SET_A:
    case Type::T_SET_T:
      break;
    default:
      error("Only a reference pointing to an optional field of a record"
            " or set type can be compared with `omit'");
      goto error;
    }
    if (!t_type->has_comp_withName(*field_id)) {
      error("Type `%s' does not have field named `%s'",
        t_type->get_typename().c_str(), field_id->get_dispname().c_str());
      goto error;
    } else if (!t_type->get_comp_byName(*field_id)->get_is_optional()) {
      error("Field `%s' is mandatory in type `%s'. It cannot be compared with "
        "`omit'", field_id->get_dispname().c_str(),
	t_type->get_typename().c_str());
      goto error;
    }
    // putting the last field_id back to subrefs
    subrefs->add(new Ttcn::FieldOrArrayRef(field_id));
    return;
  error:
    set_valuetype(V_ERROR);
    delete field_id;
  }

  Int Value::chk_eval_expr_sizeof(ReferenceChain *refch,
                                  Type::expected_value_t exp_val)
  {
    if(valuetype==V_ERROR) return -1;
    if(u.expr.state==EXPR_CHECKING_ERR) return -1;
    if(exp_val==Type::EXPECTED_DYNAMIC_VALUE)
      exp_val=Type::EXPECTED_TEMPLATE;

    Error_Context cntxt(this, "In the operand of"
                        " operation `%s'", get_opname());

    Int result = -1;
    Template* t_templ = u.expr.ti1->get_Template();

    if (!t_templ) {
      FATAL_ERROR("chk_eval_expr_sizeof()\n");
    }

    t_templ = t_templ->get_template_refd_last(refch);

    // Timer and port arrays are handled separately
    if (t_templ->get_templatetype() == Template::SPECIFIC_VALUE) {
      Value* val = t_templ->get_specific_value();
      if (val->get_valuetype() == V_UNDEF_LOWERID) {
        val->set_lowerid_to_ref();
      }
      if (val && val->get_valuetype() == V_REFD) {
        Reference* ref = val->get_reference();
        Assignment* t_ass = ref->get_refd_assignment();
        Common::Assignment::asstype_t asstype =
          t_ass ? t_ass->get_asstype() : Assignment::A_ERROR;
        if (asstype == Assignment::A_PORT || asstype == Assignment::A_TIMER) {
          if (t_ass->get_Dimensions()) {
            // here we have a timer or port array
            Ttcn::FieldOrArrayRefs* t_subrefs = ref->get_subrefs();
            Ttcn::ArrayDimensions *t_dims = t_ass->get_Dimensions();
            t_dims->chk_indices(ref, t_ass->get_assname(), true,
                Type::EXPECTED_DYNAMIC_VALUE);
            size_t refd_dim;
            if (t_subrefs) {
              refd_dim = t_subrefs->get_nof_refs();
              size_t nof_dims = t_dims->get_nof_dims();
              if (refd_dim >= nof_dims) {
                u.expr.ti1->error("Operation is not applicable to a %s",
                    t_ass->get_assname());
                set_valuetype(V_ERROR);
                return -1;
              }
            } else refd_dim = 0;
            return t_dims->get_dim_byIndex(refd_dim)->get_size();
          } else {
            u.expr.ti1->error("Operation is not applicable to single `%s'",
                t_ass->get_description().c_str());
            set_valuetype(V_ERROR);
            return -1;
          }
        }
      }
    }

    Value* t_val = 0;
    Type* t_type = 0;
    Assignment* t_ass = 0;
    Reference* ref = 0;
    Ttcn::FieldOrArrayRefs* t_subrefs = 0;
    t_type = chk_expr_operands_ti(u.expr.ti1, exp_val);
    if (t_type) {
      chk_expr_eval_ti(u.expr.ti1, t_type, refch, exp_val);
      t_type = t_type->get_type_refd_last();
    } else {
      error("Cannot determine type of value");
      goto error;
    }

    if(valuetype==V_ERROR) return -1;

    t_templ = t_templ->get_template_refd_last(refch);
    switch(t_templ->get_templatetype()) {
    case Template::TEMPLATE_ERROR:
      goto error;
    case Template::INDEXED_TEMPLATE_LIST:
      return -1;
    case Template::TEMPLATE_REFD:
    case Template::TEMPLATE_LIST:
    case Template::NAMED_TEMPLATE_LIST:
      // computed later
      break;
    case Template::SPECIFIC_VALUE:
    {
      t_val=t_templ->get_specific_value()->get_value_refd_last(refch);
      if(t_val) {
        switch(t_val->get_valuetype()) {
        case V_SEQOF:
        case V_SETOF:
        case V_ARRAY:
        case V_ROID:
        case V_OID:
        case V_SEQ:
        case V_SET:
          break;
        case V_REFD: {
          ref = t_val->get_reference();
          t_ass = ref->get_refd_assignment();
          t_subrefs = ref->get_subrefs();
          break;
        }
        default:
          u.expr.ti1->error("Operation is not applicable to `%s'",
              t_val->create_stringRepr().c_str());
          goto error;
        }
      }
      break;
    }
    default:
      u.expr.ti1->error("Operation is not applicable to %s `%s'",
          t_templ->get_templatetype_str(), t_templ->get_fullname().c_str());
      goto error;
    } // switch

    if (t_ass) {
    switch(t_ass->get_asstype()) {
    case Assignment::A_ERROR:
      goto error;
    case Assignment::A_CONST:
      t_val = t_ass->get_Value();
      break;
    case Assignment::A_EXT_CONST:
    case Assignment::A_MODULEPAR:
    case Assignment::A_MODULEPAR_TEMP:
      if(exp_val==Type::EXPECTED_CONSTANT) {
        u.expr.ti1->error("Reference to an (evaluable) constant value was "
                   "expected instead of %s", t_ass->get_description().c_str());
        goto error;
      }
      break;
    case Assignment::A_VAR:
    case Assignment::A_PAR_VAL_IN:
    case Assignment::A_PAR_VAL_OUT:
    case Assignment::A_PAR_VAL_INOUT:
      switch(exp_val) {
      case Type::EXPECTED_CONSTANT:
        u.expr.ti1->error("Reference to a constant value was expected instead of %s",
                   t_ass->get_description().c_str());
        goto error;
	break;
      case Type::EXPECTED_STATIC_VALUE:
        u.expr.ti1->error("Reference to a static value was expected instead of %s",
                   t_ass->get_description().c_str());
        goto error;
        break;
      default:
        break;
      }
      break;
    case Assignment::A_TEMPLATE:
      t_templ = t_ass->get_Template();
      // no break
    case Assignment::A_VAR_TEMPLATE:
    case Assignment::A_PAR_TEMPL_IN:
    case Assignment::A_PAR_TEMPL_OUT:
    case Assignment::A_PAR_TEMPL_INOUT:
      if (exp_val!=Type::EXPECTED_TEMPLATE)
        u.expr.ti1->error("Reference to a value was expected instead of %s",
                   t_ass->get_description().c_str());
      goto error;
      break;
    case Assignment::A_FUNCTION_RVAL:
    case Assignment::A_EXT_FUNCTION_RVAL:
      switch(exp_val) {
      case Type::EXPECTED_CONSTANT:
        u.expr.ti1->error("Reference to a constant value was expected instead of "
                   "the return value of %s", t_ass->get_description().c_str());
        goto error;
        break;
      case Type::EXPECTED_STATIC_VALUE:
        u.expr.ti1->error("Reference to a static value was expected instead of "
                   "the return value of %s", t_ass->get_description().c_str());
        goto error;
        break;
      default:
        break;
      }
      break;
    case Assignment::A_FUNCTION_RTEMP:
    case Assignment::A_EXT_FUNCTION_RTEMP:
      if(exp_val!=Type::EXPECTED_TEMPLATE)
        u.expr.ti1->error("Reference to a value was expected instead of a call"
                   " of %s, which returns a template",
        t_ass->get_description().c_str());
      goto error;
      break;
    case Assignment::A_TIMER:
    case Assignment::A_PORT:
      if (u.expr.v_optype == OPTYPE_SIZEOF) {
        // sizeof is applicable to timer and port arrays
        Ttcn::ArrayDimensions *t_dims = t_ass->get_Dimensions();
        if (!t_dims) {
          u.expr.ti1->error("Operation is not applicable to single %s",
            t_ass->get_description().c_str());
          goto error;
        }
        t_dims->chk_indices(ref, t_ass->get_assname(), true,
            Type::EXPECTED_DYNAMIC_VALUE);
        size_t refd_dim;
        if (t_subrefs) {
          refd_dim = t_subrefs->get_nof_refs();
          size_t nof_dims = t_dims->get_nof_dims();
          if (refd_dim > nof_dims) goto error;
          else if (refd_dim == nof_dims) {
            u.expr.ti1->error("Operation is not applicable to a %s",
                t_ass->get_assname());
            goto error;
          }
        } else refd_dim = 0;
        return t_dims->get_dim_byIndex(refd_dim)->get_size();
      }
      // no break
    default:
      u.expr.ti1->error("Reference to a %s was expected instead of %s",
          exp_val == Type::EXPECTED_TEMPLATE ? "value or template" : "value",
          t_ass->get_description().c_str());
      goto error;
    } // end switch

    t_type = t_ass->get_Type()->get_field_type(t_subrefs, exp_val);
    if (!t_type) goto error;
    t_type = t_type->get_type_refd_last();

    switch(t_type->get_typetype()) {
    case Type::T_ERROR:
      goto error;
    case Type::T_SEQOF:
    case Type::T_SETOF:
    // no break
    case Type::T_SEQ_T:
    case Type::T_SET_T:
    case Type::T_SEQ_A:
    case Type::T_SET_A:
    case Type::T_ARRAY:
      // ok
      break;
    case Type::T_OID:
    case Type::T_ROID:
      break;
    default:
      u.expr.ti1->error("Reference to value or template of type record, record of,"
                 " set, set of, objid or array was expected");
      goto error;
    } // switch
    }

    // check for index overflows in subrefs if possible
    if (t_val) {
      switch (t_val->get_valuetype()) {
      case V_SEQOF:
      case V_SETOF:
      case V_ARRAY:
        if (t_val->is_indexed()) {
          return -1;
        }
        break;
      default:
        break;
      }
      /* The reference points to a constant.  */
      if (!t_subrefs || !t_subrefs->has_unfoldable_index()) {
        t_val = t_val->get_refd_sub_value(t_subrefs, 0, false, refch);
        if (!t_val) goto error;
        t_val=t_val->get_value_refd_last(refch);
      } else { t_val = 0; }
    } else if (t_templ) {
      /* The size of INDEXED_TEMPLATE_LIST nodes is unknown at compile
         time.  Don't try to evaluate it at compile time.  */
      if (t_templ->get_templatetype() == Template::INDEXED_TEMPLATE_LIST) {
        return -1;
      /* The reference points to a static template.  */
      } else if (!t_subrefs || !t_subrefs->has_unfoldable_index()) {
        t_templ = t_templ->get_refd_sub_template(t_subrefs, ref && ref->getUsedInIsbound(), refch);
        if (!t_templ) goto error;
        t_templ = t_templ->get_template_refd_last(refch);
      } else { t_templ = 0; }
    }

    if(u.expr.v_optype==OPTYPE_SIZEOF) {
      if(t_templ) {
        switch(t_templ->get_templatetype()) {
        case Template::TEMPLATE_ERROR:
          goto error;
        case Template::TEMPLATE_REFD:
          // not foldable
          t_templ=0;
          break;
        case Template::SPECIFIC_VALUE:
          t_val=t_templ->get_specific_value()->get_value_refd_last(refch);
          t_templ=0;
          break;
        case Template::TEMPLATE_LIST:
        case Template::NAMED_TEMPLATE_LIST:
          break;
        default:
          u.expr.ti1->error("Operation is not applicable to %s `%s'",
                     t_templ->get_templatetype_str(),
                     t_templ->get_fullname().c_str());
          goto error;
        } // switch
      }
      if(t_val) {
        switch(t_val->get_valuetype()) {
        case V_SEQOF:
        case V_SETOF:
        case V_ARRAY:
        case V_SEQ:
        case V_SET:
        case V_OID:
        case V_ROID:
          // ok
          break;
        default:
          // error is already reported
          t_val=0;
          break;
        } // switch
      }
    }

    /* evaluation */

    if(t_type->get_typetype()==Type::T_ARRAY) {
      result = t_type->get_dimension()->get_size();
    }
    else if(t_templ) { // sizeof()
      switch(t_templ->get_templatetype()) {
        case Template::TEMPLATE_LIST:
          if(t_templ->temps_contains_anyornone_symbol()) {
            if(t_templ->is_length_restricted()) {
	      Ttcn::LengthRestriction *lr = t_templ->get_length_restriction();
	      if (lr->get_is_range()) {
		Value *v_upper = lr->get_upper_value();
		if (v_upper) {
		  if (v_upper->valuetype == V_INT) {
		    Int nof_comps =
		      static_cast<Int>(t_templ->get_nof_comps_not_anyornone());
		    if (v_upper->u.val_Int->get_val() == nof_comps)
		      result = nof_comps;
		    else {
		      u.expr.ti1->error("`sizeof' operation is not applicable for "
				 "templates without exact size");
		      goto error;
		    }
		  }
		} else {
		  u.expr.ti1->error("`sizeof' operation is not applicable for "
		    "templates containing `*' without upper boundary in the "
		    "length restriction");
		  goto error;
		}
	      } else {
		Value *v_single = lr->get_single_value();
		if (v_single->valuetype == V_INT)
		  result = v_single->u.val_Int->get_val();
	      }
            }
            else { // not length restricted
              u.expr.ti1->error("`sizeof' operation is not applicable for templates"
                         " containing `*' without length restriction");
              goto error;
            }
          }
          else result=t_templ->get_nof_listitems();
          break;
        case Template::NAMED_TEMPLATE_LIST:
          result=0;
          for(size_t i=0; i<t_templ->get_nof_comps(); i++)
            if(t_templ->get_namedtemp_byIndex(i)->get_template()
               ->get_templatetype()!=Template::OMIT_VALUE) result++;
        return result;
      default:
          FATAL_ERROR("Value::chk_eval_expr_sizeof()");
      } // switch
    }
    else if(t_val) {
      switch(t_val->get_valuetype()) {
      case V_SEQOF:
      case V_SETOF:
      case V_ARRAY:

      case V_OID:
      case V_ROID:
        result=t_val->get_nof_comps();
        break;
      case V_SEQ:
      case V_SET:
        result=0;
        for(size_t i=0; i<t_val->get_nof_comps(); i++)
          if(t_val->get_se_comp_byIndex(i)->get_value()
             ->get_valuetype()!=V_OMIT) result++;
        break;

      default:
        FATAL_ERROR("Value::chk_eval_expr_sizeof()");
      } // switch
    }

    return result;
  error:
    set_valuetype(V_ERROR);
    return -1;
  }

  Type *Value::chk_expr_operands_ti(TemplateInstance* ti, Type::expected_value_t exp_val)
  {
    Type *governor = ti->get_expr_governor(exp_val);
    if (!governor) {
      ti->get_Template()->set_lowerid_to_ref();
      governor = ti->get_expr_governor(exp_val);
    }
    if (!governor) {
      string str;
      ti->append_stringRepr( str);
      ti->error("Cannot determine the argument type of %s in the `%s' operation.\n"
                "If type is known, use valueof(<type>: %s) as argument.",
                str.c_str(), get_opname(), str.c_str()); 
      set_valuetype(V_ERROR);
    }
    return governor;
  }

  void Value::chk_expr_operands_match(Type::expected_value_t exp_val)
  {
  start:
    Type *governor = u.expr.v1->get_expr_governor(exp_val);
    if (!governor) governor = u.expr.t2->get_expr_governor(
      exp_val == Type::EXPECTED_DYNAMIC_VALUE ?
      Type::EXPECTED_TEMPLATE : exp_val);
    if (!governor) {
      Template *t_temp = u.expr.t2->get_Template();
      if (t_temp->is_undef_lowerid()) {
	// We convert the template to reference first even if the value is also
	// an undef lowerid. The user can prevent this by explicit type
	// specification.
        t_temp->set_lowerid_to_ref();
	goto start;
      } else if (u.expr.v1->is_undef_lowerid()) {
        u.expr.v1->set_lowerid_to_ref();
	goto start;
      }
    }
    if (!governor) {
      error("Cannot determine the type of arguments in `match()' operation");
      set_valuetype(V_ERROR);
      return;
    }
    u.expr.v1->set_my_governor(governor);
    {
      Error_Context cntxt(this, "In the first argument of `match()'"
                          " operation");
      governor->chk_this_value_ref(u.expr.v1);
      (void)governor->chk_this_value(u.expr.v1, 0, exp_val,
        INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED, SUB_CHK);
    }
    {
      Error_Context cntxt(this, "In the second argument of `match()' "
        "operation");
      u.expr.t2->chk(governor);
    }
  }

  void Value::chk_expr_dynamic_part(Type::expected_value_t exp_val,
    bool allow_controlpart, bool allow_runs_on, bool require_runs_on)
  {
    Ttcn::StatementBlock *my_sb;
    switch (exp_val) {
    case Type::EXPECTED_CONSTANT:
      error("An evaluable constant value was expected instead of operation "
	"`%s'", get_opname());
      goto error;
    case Type::EXPECTED_STATIC_VALUE:
      error("A static value was expected instead of operation `%s'",
	get_opname());
      goto error;
    default:
      break;
    } // switch
    if (!my_scope) FATAL_ERROR("Value::chk_expr_dynamic_part()");
    my_sb = dynamic_cast<Ttcn::StatementBlock*>(my_scope);
    if (!my_sb) {
      error("Operation `%s' is allowed only within statements",
	get_opname());
      goto error;
    }
    if (!allow_controlpart && !my_sb->get_my_def()) {
      error("Operation `%s' is not allowed in the control part",
	get_opname());
      goto error;
    }
    if (!allow_runs_on && my_scope->get_scope_runs_on()) {
      error("Operation `%s' cannot be used in a definition that has "
	"`runs on' clause", get_opname());
      goto error;
    }
    if (require_runs_on && !my_scope->get_scope_runs_on()) {
      error("Operation `%s' can be used only in a definition that has "
	"`runs on' clause", get_opname());
      goto error;
    }
    return;
  error:
    set_valuetype(V_ERROR);
  }

  void Value::chk_expr_operand_valid_float(Value* v, const char *opnum, const char *opname)
  {
    if(valuetype==V_ERROR) return;
    if(u.expr.state==EXPR_CHECKING_ERR) return;
    if(v->is_unfoldable()) return;
    if(v->get_expr_returntype()!=Type::T_REAL) return;
    ttcn3float r = v->get_val_Real();
    if (isSpecialFloatValue(r)) {
      v->error("%s operand of operation `%s' cannot be %s, it must be a numeric value",
               opnum, opname, Real2string(r).c_str());
      set_valuetype(V_ERROR);
    }
  }

  void Value::chk_expr_operands(ReferenceChain *refch,
                                Type::expected_value_t exp_val)
  {
    const char *first="First", *second="Second", *third="Third",
               *fourth="Fourth", *the="The", *left="Left", *right="Right";
    Value *v1, *v2, *v3;
    Type::typetype_t tt1, tt2, tt3;
    Type t_chk(Type::T_ERROR);

    const char *opname=get_opname();

    // first classify the unchecked ischosen() operation
    if (u.expr.v_optype==OPTYPE_ISCHOSEN) chk_expr_ref_ischosen();

    switch (u.expr.v_optype) {
    case OPTYPE_COMP_NULL:
    case OPTYPE_TESTCASENAME:
    case OPTYPE_PROF_RUNNING:
      break;
    case OPTYPE_COMP_MTC:
    case OPTYPE_COMP_SYSTEM:
      chk_expr_comptype_compat();
      break;
    case OPTYPE_RND: // -
    case OPTYPE_TMR_RUNNING_ANY:
      chk_expr_dynamic_part(exp_val, true);
      break;
    case OPTYPE_COMP_RUNNING_ANY:
    case OPTYPE_COMP_RUNNING_ALL:
    case OPTYPE_COMP_ALIVE_ANY:
    case OPTYPE_COMP_ALIVE_ALL:
    case OPTYPE_GETVERDICT:
      chk_expr_dynamic_part(exp_val, false);
      break;
    case OPTYPE_COMP_SELF:
      chk_expr_comptype_compat();
      chk_expr_dynamic_part(exp_val, false, true, false);
      break;
    case OPTYPE_UNARYPLUS: // v1
    case OPTYPE_UNARYMINUS:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_int_float(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
      }
      break;
    case OPTYPE_NOT:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_bool(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
      }
      break;
    case OPTYPE_NOT4B:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_binstr(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
      }
      break;
    case OPTYPE_BIT2HEX:
    case OPTYPE_BIT2OCT:
    case OPTYPE_BIT2STR:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_bstr(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
      }
      break;
    case OPTYPE_BIT2INT:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_bstr(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
        // Skip `chk_expr_val_bitstr_intsize(v1, the, opname);'.
      }
      break;
    case OPTYPE_CHAR2INT:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_cstr(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
        chk_expr_val_len1(v1, the, opname);
      }
      break;
    case OPTYPE_CHAR2OCT:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_cstr(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
      }
      break;
    case OPTYPE_STR2INT:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_cstr(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
        chk_expr_val_str_int(v1, the, opname);
      }
      break;
    case OPTYPE_STR2FLOAT:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_cstr(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
        chk_expr_val_str_float(v1, the, opname);
      }
      break;
    case OPTYPE_STR2BIT:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_cstr(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
        chk_expr_val_str_bindigits(v1, the, opname);
      }
      break;
    case OPTYPE_STR2HEX:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_cstr(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
        chk_expr_val_str_hexdigits(v1, the, opname);
      }
      break;
    case OPTYPE_STR2OCT:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_cstr(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
        chk_expr_val_str_len_even(v1, the, opname);
        chk_expr_val_str_hexdigits(v1, the, opname);
      }
      break;
    case OPTYPE_ENUM2INT:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        chk_expr_operandtype_enum(opname, v1, exp_val);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
      }
      break;
    case OPTYPE_ENCODE:
      chk_expr_operand_encode(refch, exp_val);
      break;
    case OPTYPE_FLOAT2INT:
    case OPTYPE_FLOAT2STR:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_float(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
        if (u.expr.v_optype==OPTYPE_FLOAT2INT)
          chk_expr_operand_valid_float(v1, the, opname);
      }
      break;
    case OPTYPE_RNDWITHVAL:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_float(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
        chk_expr_operand_valid_float(v1, the, opname);
      }
      chk_expr_dynamic_part(exp_val, true);
      break;
    case OPTYPE_HEX2BIT:
    case OPTYPE_HEX2OCT:
    case OPTYPE_HEX2STR:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_hstr(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
      }
      break;
    case OPTYPE_HEX2INT:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_hstr(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
        // Skip `chk_expr_val_hexstr_intsize(v1, the, opname);'.
      }
      break;
    case OPTYPE_INT2CHAR:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_int(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
        chk_expr_val_int_pos7bit(v1, the, opname);
      }
      break;
    case OPTYPE_INT2UNICHAR:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_int(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
        chk_expr_val_int_pos31bit(v1, first, opname);
      }
      break;
    case OPTYPE_INT2FLOAT:
    case OPTYPE_INT2STR:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_int(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
      }
      break;
    case OPTYPE_OCT2BIT:
    case OPTYPE_OCT2HEX:
    case OPTYPE_OCT2STR:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_ostr(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
      }
      break;
    case OPTYPE_OCT2INT:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_ostr(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
        // Simply skip `chk_expr_val_hexstr_intsize(v1, the, opname);' for
        // now.
      }
      break;
    case OPTYPE_OCT2CHAR:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_ostr(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
        chk_expr_val_str_7bitoctets(v1, the, opname);
      }
      break;
    case OPTYPE_REMOVE_BOM:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_ostr(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
      }
      break;
    case OPTYPE_GET_STRINGENCODING:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_ostr(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
      }
      break;
    case OPTYPE_ENCODE_BASE64:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_ostr(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
      }
      v2=u.expr.v2 ? u.expr.v2 : 0;
      if (v2)
      {
        Error_Context cntxt(this, "In the second operand of operation `%s'", opname);
        v2->set_lowerid_to_ref();
        tt2=v2->get_expr_returntype(exp_val);
        chk_expr_operandtype_bool(tt2, second, opname, v2);
        chk_expr_eval_value(v2, t_chk, refch, exp_val);
      }
      break;
      case OPTYPE_DECODE_BASE64:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_cstr(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
      }
      break;
    case OPTYPE_UNICHAR2INT:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_charstr(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
        chk_expr_val_len1(v1, the, opname);
      }
      break;
    case OPTYPE_UNICHAR2CHAR:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_charstr(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
        chk_expr_val_ustr_7bitchars(v1, the, opname);
      }
      break;
    case OPTYPE_HOSTID:
      v1=u.expr.v1 ? u.expr.v1 : 0;
      if (v1)
      {
        Error_Context cntxt(this, "In the first operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_cstr(tt1, second, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
      }
      break;
    case OPTYPE_UNICHAR2OCT: // v1 [v2]
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_charstr(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
      }
      v2=u.expr.v2 ? u.expr.v2 : 0;
      if (v2)
      {
        Error_Context cntxt(this, "In the second operand of operation `%s'", opname);
        v2->set_lowerid_to_ref();
        tt2=v2->get_expr_returntype(exp_val);
        chk_expr_operandtype_cstr(tt2, second, opname, v2);
        chk_expr_eval_value(v2, t_chk, refch, exp_val);
      }
      break;
    case OPTYPE_OCT2UNICHAR: // v1 [v2]
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_ostr(tt1, the, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
      }
      v2=u.expr.v2 ? u.expr.v2 : 0;
      if (v2)
      {
        Error_Context cntxt(this, "In the second operand of operation `%s'", opname);
        v2->set_lowerid_to_ref();
        tt2=v2->get_expr_returntype(exp_val);
        chk_expr_operandtype_cstr(tt2, second, opname, v2);
        chk_expr_eval_value(v2, t_chk, refch, exp_val);
      }
      break;
    case OPTYPE_ISTEMPLATEKIND: { // ti1 v2
      if (exp_val == Type::EXPECTED_DYNAMIC_VALUE)
      	exp_val = Type::EXPECTED_TEMPLATE;
      {
        Error_Context cntxt(u.expr.ti1, "In the first operand of operation `%s'", opname);
        Type *governor = chk_expr_operands_ti(u.expr.ti1, exp_val);
        if (!governor) return;
        if (valuetype == V_ERROR) return;
        chk_expr_eval_ti(u.expr.ti1, governor, refch, exp_val);
      }
      Error_Context cntxt(u.expr.v2, "In the second operand of operation `%s'", opname);
      u.expr.v2->set_lowerid_to_ref();
      tt2=u.expr.v2->get_expr_returntype(exp_val);
      chk_expr_operandtype_charstr(tt2, second, opname, u.expr.v2);
      chk_expr_eval_value(u.expr.v2, t_chk, refch, exp_val);
      if (!u.expr.v2->is_unfoldable()) {
        const string& type_param = u.expr.v2->get_val_str();
        if (type_param != "value" && type_param != "list" && type_param != "complement" &&
            type_param != "AnyValue" && type_param != "?" && type_param != "AnyValueOrNone" &&
            type_param != "*" && type_param != "range" && type_param != "superset" &&
            type_param != "subset" && type_param != "omit" && type_param != "decmatch" &&
            type_param != "AnyElement" && type_param != "AnyElementsOrNone" &&
            type_param != "permutation" && type_param != "length" && 
            type_param != "ifpresent" && type_param != "pattern") {
          error("Incorrect second parameter (%s) was passed to istemplatekind.",
            type_param.c_str());
          set_valuetype(V_ERROR);
        }
      }
      break; }
    case OPTYPE_ENCVALUE_UNICHAR: // ti1 [v2]
      chk_expr_operand_encode(refch, exp_val);
      v2=u.expr.v2 ? u.expr.v2 : 0;
      if (v2)
      {
        Error_Context cntxt(this, "In the second operand of operation `%s'", opname);
        v2->set_lowerid_to_ref();
        tt2=v2->get_expr_returntype(exp_val);
        chk_expr_operandtype_charstr(tt2, second, opname, v2);
        chk_expr_eval_value(v2, t_chk, refch, exp_val);
        if (!u.expr.v2->is_unfoldable()) {
          const string& type_param = u.expr.v2->get_val_str();
          if (type_param != "UTF-8" && type_param != "UTF-16" && type_param != "UTF-16LE" &&
              type_param != "UTF-16BE" && type_param != "UTF-32" &&
              type_param != "UTF-32LE" && type_param != "UTF-32BE") {
            error("Incorrect second parameter (%s) was passed to encvalue_unichar.",
              type_param.c_str());
            set_valuetype(V_ERROR);
          }
        }
      }
      break;
    case OPTYPE_DECVALUE_UNICHAR:
      chk_expr_operands_decode(OPTYPE_DECVALUE_UNICHAR);
      v3=u.expr.v3 ? u.expr.v3 : 0;
      if (v3)
      {
        Error_Context cntxt(this, "In the thrid operand of operation `%s'", opname);
        v3->set_lowerid_to_ref();
        tt3=v3->get_expr_returntype(exp_val);
        chk_expr_operandtype_charstr(tt3, third, opname, v3);
        chk_expr_eval_value(v3, t_chk, refch, exp_val);
        if (!u.expr.v3->is_unfoldable()) {
          const string& type_param = u.expr.v3->get_val_str();
          if (type_param != "UTF-8" && type_param != "UTF-16" && type_param != "UTF-16LE" &&
              type_param != "UTF-16BE" && type_param != "UTF-32" &&
              type_param != "UTF-32LE" && type_param != "UTF-32BE") {
            error("Incorrect third parameter (%s) was passed to decvalue_unichar.",
              type_param.c_str());
            set_valuetype(V_ERROR);
          }
        }
      }
      break;
    case OPTYPE_ADD: // v1 v2
    case OPTYPE_SUBTRACT:
    case OPTYPE_MULTIPLY:
    case OPTYPE_DIVIDE: {
      v1=u.expr.v1;
      v2=u.expr.v2;
      Error_Context cntxt(this, "In the operands of operation `%s'", opname);
      v1->set_lowerid_to_ref();
      v2->set_lowerid_to_ref();
      tt1=v1->get_expr_returntype(exp_val);
      tt2=v2->get_expr_returntype(exp_val);
      chk_expr_operandtype_int_float(tt1, first, opname, v1);
      chk_expr_operandtype_int_float(tt2, second, opname, v2);
      chk_expr_eval_value(v1, t_chk, refch, exp_val);
      chk_expr_eval_value(v2, t_chk, refch, exp_val);
      // No float checks needed, everything is allowed on -+infinity and not_a_number
      if(u.expr.v_optype==OPTYPE_DIVIDE)
        chk_expr_val_int_float_not0(v2, second, opname);
      chk_expr_operandtypes_same(tt1, tt2, opname);
      break; }
    case OPTYPE_MOD:
    case OPTYPE_REM:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the left operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_int(tt1, left, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
      }
      v2=u.expr.v2;
      {
        Error_Context cntxt(this, "In the right operand of operation `%s'", opname);
        v2->set_lowerid_to_ref();
        tt2=v2->get_expr_returntype(exp_val);
        chk_expr_operandtype_int(tt2, right, opname, v2);
        chk_expr_eval_value(v2, t_chk, refch, exp_val);
        chk_expr_val_int_float_not0(v2, right, opname);
      }
      break;
    case OPTYPE_CONCAT: {
      v1=u.expr.v1;
      v2=u.expr.v2;
      v1->set_lowerid_to_ref();
      v2->set_lowerid_to_ref();
      if (v1->is_string_type(exp_val) || v2->is_string_type(exp_val)) {
        {
          Error_Context cntxt(this, "In the left operand of operation `%s'", opname);
          tt1=v1->get_expr_returntype(exp_val);
          chk_expr_operandtype_str(tt1, left, opname, v1);
          chk_expr_eval_value(v1, t_chk, refch, exp_val);
        }
        {
          Error_Context cntxt(this, "In the right operand of operation `%s'", opname);
          tt2=v2->get_expr_returntype(exp_val);
          chk_expr_operandtype_str(tt2, right, opname, v2);
          chk_expr_eval_value(v2, t_chk, refch, exp_val);
        }
        if (!((tt1==Type::T_CSTR && tt2==Type::T_USTR)
            || (tt2==Type::T_CSTR && tt1==Type::T_USTR)))
          chk_expr_operandtypes_same(tt1, tt2, opname);
      } else { // other list types
        Type* v1_gov = v1->get_expr_governor(exp_val);
        Type* v2_gov = v2->get_expr_governor(exp_val);
        if (!v1_gov) {
          error("Cannot determine the type of the left operand of `%s' operation", opname);
          set_valuetype(V_ERROR);
          return;
        } else {
          Error_Context cntxt(this, "In the left operand of operation `%s'", opname);
          v1_gov->chk_this_value_ref(v1);
          (void)v1_gov->chk_this_value(v1, 0, exp_val,
            INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED, SUB_CHK);
          chk_expr_operandtype_list(v1_gov, left, opname, v1, false);
        }
        if (!v2_gov) {
          if (!v1_gov) {
            error("Cannot determine the type of the right operand of `%s' operation", opname);
            set_valuetype(V_ERROR);
            return;
          }
          // for recof/setof literals set the type from v1
          v2_gov = v1_gov;
          v2->set_my_governor(v1_gov);
        }
        {
          Error_Context cntxt(this, "In the right operand of operation `%s'",
                              opname);
          v2_gov->chk_this_value_ref(v2);
          (void)v2_gov->chk_this_value(v2, 0, exp_val,
            INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED, SUB_CHK);
          chk_expr_operandtype_list(v2_gov, right, opname, v2, false);
          if (valuetype == V_ERROR) return;
          // 7.1.2 says that we shouldn't allow type compatibility.
          if (!v1_gov->is_compatible(v2_gov, NULL, this)
              && !v2_gov->is_compatible(v1_gov, NULL, NULL)) {
            error("The operands of operation `%s' should be of compatible "
                  "types", get_opname());
          }
        }
      }
      break; }
    case OPTYPE_EQ:
    case OPTYPE_NE:
      v1 = u.expr.v1;
      v2 = u.expr.v2;
      chk_expr_operandtypes_compat(exp_val, v1, v2);
      {
        Error_Context cntxt(this, "In the left operand of operation `%s'",
                            opname);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
      }
      {
        Error_Context cntxt(this, "In the right operand of operation `%s'",
                            opname);
        chk_expr_eval_value(v2, t_chk, refch, exp_val);
        /* According to the BNF v4.1.1, the "arguments" around ==/!= in an
         * EqualExpression are RelExpression-s, not NotExpression-s. This means:
         * "not a == b" is supposed to be equivalent to "not (a == b)", and
         * "a == not b" is not allowed. (HL69107)
         * The various *Expressions implement operator precedence in the std.
         * Titan's parser has only one Expression and relies on Bison
         * for operator precedence. The check below brings Titan in line
         * with the standard by explicitly making "a == not b" an error */
        if (v2->get_valuetype() == V_EXPR
          && v2->u.expr.v_optype == OPTYPE_NOT) {
          error("The operation `%s' is not allowed to be "
              "the second operand of operation `%s'", v2->get_opname(), opname);
          set_valuetype(V_ERROR);
        }
      }
      break;
    case OPTYPE_LT:
    case OPTYPE_GT:
    case OPTYPE_GE:
    case OPTYPE_LE:
      v1=u.expr.v1;
      v2=u.expr.v2;
      chk_expr_operandtypes_compat(exp_val, v1, v2);
      {
        Error_Context cntxt(this, "In the left operand of operation `%s'",
                            opname);
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_int_float_enum(tt1, left, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
      }
      {
        Error_Context cntxt(this, "In the right operand of operation `%s'",
                            opname);
        tt2=v2->get_expr_returntype(exp_val);
        chk_expr_operandtype_int_float_enum(tt2, right, opname, v2);
        chk_expr_eval_value(v2, t_chk, refch, exp_val);
      }
      break;
    case OPTYPE_AND:
    case OPTYPE_OR:
    case OPTYPE_XOR:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the left operand of operation `%s'",
                            opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_bool(tt1, left, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
      }
      v2=u.expr.v2;
      {
        Error_Context cntxt(this, "In the right operand of operation `%s'",
                            opname);
        v2->set_lowerid_to_ref();
        tt2=v2->get_expr_returntype(exp_val);
        chk_expr_operandtype_bool(tt2, right, opname, v2);
        chk_expr_eval_value(v2, t_chk, refch, exp_val);
      }
      break;
    case OPTYPE_AND4B:
    case OPTYPE_OR4B:
    case OPTYPE_XOR4B:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the left operand of operation `%s'",
                            opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_binstr(tt1, left, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
      }
      v2=u.expr.v2;
      {
        Error_Context cntxt(this, "In the right operand of operation `%s'",
                            opname);
        v2->set_lowerid_to_ref();
        tt2=v2->get_expr_returntype(exp_val);
        chk_expr_operandtype_binstr(tt2, right, opname, v2);
        chk_expr_eval_value(v2, t_chk, refch, exp_val);
      }
      chk_expr_operandtypes_same(tt1, tt2, opname);
      chk_expr_operands_str_samelen();
      break;
    case OPTYPE_SHL:
    case OPTYPE_SHR:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the left operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_binstr(tt1, left, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
      }
      v2=u.expr.v2;
      {
        Error_Context cntxt(this, "In the right operand of operation `%s'", opname);
        v2->set_lowerid_to_ref();
        tt2=v2->get_expr_returntype(exp_val);
        chk_expr_operandtype_int(tt2, right, opname, v2);
        chk_expr_eval_value(v2, t_chk, refch, exp_val);
        chk_expr_val_large_int(v2, right, opname);
      }
      break;
    case OPTYPE_ROTL:
    case OPTYPE_ROTR:
      v1=u.expr.v1;
      v1->set_lowerid_to_ref();
      if (v1->is_string_type(exp_val)) {
        Error_Context cntxt(this, "In the left operand of operation `%s'", opname);
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_str(tt1, left, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
      } else { // other list types
        Type* v1_gov = v1->get_expr_governor(exp_val);
        if (!v1_gov) { // a recof/setof literal would be a syntax error here
          error("Cannot determine the type of the left operand of `%s' operation", opname);
        } else {
          Error_Context cntxt(this, "In the left operand of operation `%s'", opname);
          v1_gov->chk_this_value_ref(v1);
          (void)v1_gov->chk_this_value(v1, 0, exp_val,
            INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED, SUB_CHK);
          chk_expr_operandtype_list(v1_gov, left, opname, v1, true);
        }
      }
      v2=u.expr.v2;
      {
        Error_Context cntxt(this, "In the right operand of operation `%s'", opname);
        v2->set_lowerid_to_ref();
        tt2=v2->get_expr_returntype(exp_val);
        chk_expr_operandtype_int(tt2, right, opname, v2);
        chk_expr_eval_value(v2, t_chk, refch, exp_val);
        chk_expr_val_large_int(v2, right, opname);
      }
      break;
    case OPTYPE_INT2BIT:
    case OPTYPE_INT2HEX:
    case OPTYPE_INT2OCT:
      v1=u.expr.v1;
      {
        Error_Context cntxt(this, "In the first operand of operation `%s'", opname);
        v1->set_lowerid_to_ref();
        tt1=v1->get_expr_returntype(exp_val);
        chk_expr_operandtype_int(tt1, first, opname, v1);
        chk_expr_eval_value(v1, t_chk, refch, exp_val);
        chk_expr_val_int_pos0(v1, first, opname);
      }
      v2=u.expr.v2;
      {
        Error_Context cntxt(this, "In the second operand of operation `%s'", opname);
        v2->set_lowerid_to_ref();
        tt2=v2->get_expr_returntype(exp_val);
        chk_expr_operandtype_int(tt2, second, opname, v2);
        chk_expr_eval_value(v2, t_chk, refch, exp_val);
        chk_expr_val_int_pos0(v2, second, opname);
      }
      chk_expr_operands_int2binstr();
      break;
    case OPTYPE_DECODE:
      chk_expr_operands_decode(OPTYPE_DECODE);
      break;
    case OPTYPE_SUBSTR:
      {
        Error_Context cntxt(this, "In the first operand of operation `%s'", opname);
        Type::expected_value_t ti_exp_val = exp_val;
        if (ti_exp_val == Type::EXPECTED_DYNAMIC_VALUE) ti_exp_val = Type::EXPECTED_TEMPLATE;
        Type* governor = chk_expr_operands_ti(u.expr.ti1, ti_exp_val);
        if (!governor) return;
        chk_expr_eval_ti(u.expr.ti1, governor, refch, ti_exp_val);
        if (valuetype!=V_ERROR)
          u.expr.ti1->get_Template()->chk_specific_value(false);
        chk_expr_operandtype_list(governor, first, opname, u.expr.ti1, false);
      }
      v2=u.expr.v2;
      {
        Error_Context cntxt(this, "In the second operand of operation `%s'", opname);
        v2->set_lowerid_to_ref();
        tt2=v2->get_expr_returntype(exp_val);
        chk_expr_operandtype_int(tt2, second, opname, v2);
        chk_expr_eval_value(v2, t_chk, refch, exp_val);
        chk_expr_val_int_pos0(v2, second, opname);
      }
      v3=u.expr.v3;
      {
        Error_Context cntxt(this, "In the third operand of operation `%s'", opname);
        v3->set_lowerid_to_ref();
        tt3=v3->get_expr_returntype(exp_val);
        chk_expr_operandtype_int(tt3, third, opname, v3);
        chk_expr_eval_value(v3, t_chk, refch, exp_val);
        chk_expr_val_int_pos0(v3, third, opname);
      }
      chk_expr_operands_substr();
      break;
    case OPTYPE_REGEXP: {
      Type::expected_value_t ti_exp_val = exp_val;
      if (ti_exp_val == Type::EXPECTED_DYNAMIC_VALUE) ti_exp_val = Type::EXPECTED_TEMPLATE;
      {
        Error_Context cntxt(this, "In the first operand of operation `%s'", opname);
        Type* governor = chk_expr_operands_ti(u.expr.ti1, ti_exp_val);
        if (!governor) return;
        chk_expr_eval_ti(u.expr.ti1, governor, refch, ti_exp_val);
        if (valuetype!=V_ERROR) {
          u.expr.ti1->get_Template()->chk_specific_value(false);
          chk_expr_operandtype_charstr(governor->get_type_refd_last()->
            get_typetype_ttcn3(), first, opname, u.expr.ti1);
        }
      }
      {
        Error_Context cntxt(this, "In the second operand of operation `%s'", opname);
        Type* governor = chk_expr_operands_ti(u.expr.t2, ti_exp_val);
        if (!governor) return;
        chk_expr_eval_ti(u.expr.t2, governor, refch, ti_exp_val);
        chk_expr_operandtype_charstr(governor->get_type_refd_last()->
          get_typetype_ttcn3(), second, opname, u.expr.t2);
      }
      v3=u.expr.v3;
      {
        Error_Context cntxt(this, "In the third operand of operation `%s'", opname);
        v3->set_lowerid_to_ref();
        tt3=v3->get_expr_returntype(exp_val);
        chk_expr_operandtype_int(tt3, third, opname, v3);
        chk_expr_eval_value(v3, t_chk, refch, exp_val);
        chk_expr_val_int_pos0(v3, third, opname);
      }
      chk_expr_operands_regexp();
    } break;
    case OPTYPE_ISCHOSEN:
      // do nothing: the operand is erroneous
      // the error was already reported in chk_expr_ref_ischosen()
      break;
    case OPTYPE_ISCHOSEN_V: // v1 i2
    case OPTYPE_ISCHOSEN_T: // t1 i2
      chk_expr_operands_ischosen(refch, exp_val);
      break;
    case OPTYPE_VALUEOF: { // ti1
      if (exp_val == Type::EXPECTED_DYNAMIC_VALUE)
	exp_val = Type::EXPECTED_TEMPLATE;
      Error_Context cntxt(this, "In the operand of operation `%s'", opname);
      Type *governor = my_governor;
      if (!governor) governor = chk_expr_operands_ti(u.expr.ti1, exp_val);
      if (!governor) return;
      chk_expr_eval_ti(u.expr.ti1, governor, refch, exp_val);
      if (valuetype == V_ERROR) return;
      u.expr.ti1->get_Template()->chk_specific_value(false);
      break; }
    case OPTYPE_ISPRESENT: // TODO: rename UsedInIsbound to better name
    case OPTYPE_ISBOUND: {
      Template *templ = u.expr.ti1->get_Template();
      switch (templ->get_templatetype()) {
      case Template::TEMPLATE_REFD:
        templ->get_reference()->setUsedInIsbound();
        break;
      case Template::SPECIFIC_VALUE: {
        Value *value = templ->get_specific_value();
        if (Value::V_REFD == value->get_valuetype()) {
          value->get_reference()->setUsedInIsbound();
        }
        break; }
      default:
        break;
      }
    }
    // no break
    case OPTYPE_ISVALUE: {// ti1
      // This code is almost, but not quite, the same as for OPTYPE_VALUEOF
      if (exp_val == Type::EXPECTED_DYNAMIC_VALUE)
	exp_val = Type::EXPECTED_TEMPLATE;
      Error_Context cntxt(this, "In the operand of operation `%s'", opname);
      Type *governor = chk_expr_operands_ti(u.expr.ti1, exp_val);
      if (!governor) return;
      tt1 = u.expr.ti1->get_expr_returntype(exp_val);
      chk_expr_eval_ti(u.expr.ti1, governor, refch, exp_val);
      break; }
    case OPTYPE_SIZEOF: // ti1
      /* this checking is too complex, do the checking during eval... */
      break;
    case OPTYPE_LENGTHOF: { // ti1
      if (exp_val == Type::EXPECTED_DYNAMIC_VALUE)
	exp_val = Type::EXPECTED_TEMPLATE;
      Error_Context cntxt(this, "In the operand of operation `%s'", opname);
      Type *governor = chk_expr_operands_ti(u.expr.ti1, exp_val);
      if (!governor) return;
      chk_expr_operandtype_list(governor, the, opname, u.expr.ti1, true);
      if (valuetype == V_ERROR) return;
      chk_expr_eval_ti(u.expr.ti1, governor, refch, exp_val);
      break; }
    case OPTYPE_MATCH: // v1 t2
      chk_expr_operands_match(exp_val);
      break;
    case OPTYPE_UNDEF_RUNNING: // r1 [r2] b4
      chk_expr_operand_undef_running(exp_val, u.expr.r1, u.expr.b4, u.expr.r2,
        the, opname);
      break;
    case OPTYPE_COMP_ALIVE:
    case OPTYPE_COMP_RUNNING: { //v1
      Type* ref_type = chk_expr_operand_compref(u.expr.v1, the, opname, u.expr.b4);
      chk_expr_dynamic_part(exp_val, false);
      if (u.expr.r2 != NULL && ref_type != NULL) {
        Ttcn::Reference* index_ref = dynamic_cast<Ttcn::Reference*>(u.expr.r2);
        if (index_ref == NULL) {
          FATAL_ERROR("Value::chk_expr_operand_undef_running");
        }
        Ttcn::ArrayDimensions dummy;
        ref_type = ref_type->get_type_refd_last();
        while (ref_type->get_typetype() == Type::T_ARRAY) {
          dummy.add(ref_type->get_dimension()->clone());
          ref_type = ref_type->get_ofType()->get_type_refd_last();
        }
        Ttcn::Statement::chk_index_redirect(index_ref, &dummy, u.expr.b4, "component");
      }
      break; }
    case OPTYPE_TMR_READ: // r1
    case OPTYPE_TMR_RUNNING: // r1
      chk_expr_operand_tmrref(u.expr.r1, the, opname);
      chk_expr_dynamic_part(exp_val, true);
      break;
    case OPTYPE_EXECUTE: // r1 [v2] // testcase
      chk_expr_operand_execute(u.expr.r1, u.expr.v2, the, opname);
      chk_expr_dynamic_part(exp_val, true, false, false);
      break;
    case OPTYPE_COMP_CREATE: // r1 [v2] [v3] b4
      chk_expr_operand_comptyperef_create();
      v2=u.expr.v2;
      if(v2) {
        Error_Context cntxt(this, "In the first operand of operation `%s'", opname);
        v2->set_lowerid_to_ref();
        tt2=v2->get_expr_returntype(exp_val);
	chk_expr_operandtype_cstr(tt2, first, opname, v2);
	chk_expr_eval_value(v2, t_chk, refch, exp_val);
      }
      v3=u.expr.v3;
      if(v3) {
        Error_Context cntxt(this, "In the second operand of operation `%s'", opname);
        v3->set_lowerid_to_ref();
        tt3=v3->get_expr_returntype(exp_val);
	chk_expr_operandtype_cstr(tt3, second, opname, v3);
	chk_expr_eval_value(v3, t_chk, refch, exp_val);
      }
      chk_expr_dynamic_part(exp_val, false);
      break;
    case OPTYPE_ACTIVATE: // r1 // altstep
      chk_expr_operand_activate(u.expr.r1, the, opname);
      chk_expr_dynamic_part(exp_val, true);
      break;
    case OPTYPE_CHECKSTATE_ANY: // [r1] v2
    case OPTYPE_CHECKSTATE_ALL:
      chk_expr_dynamic_part(exp_val, false);
      v2=u.expr.v2;
      if(v2) {
        Error_Context cntxt(this, "In the first operand of operation `%s'", opname);
        v2->set_lowerid_to_ref();
        tt2=v2->get_expr_returntype(exp_val);
	chk_expr_operandtype_cstr(tt2, first, opname, v2);
	chk_expr_eval_value(v2, t_chk, refch, exp_val);
      }
      break;
    case OPTYPE_ACTIVATE_REFD:{ //v1 t_list2
      Ttcn::ActualParList *parlist = new Ttcn::ActualParList;
      chk_expr_operand_activate_refd(u.expr.v1,u.expr.t_list2->get_tis(), parlist, the,
                                    opname);
      delete u.expr.t_list2;
      u.expr.ap_list2 = parlist;
      chk_expr_dynamic_part(exp_val, true);
      break; }
    case OPTYPE_EXECUTE_REFD: {// v1 t_list2 [v3]
      Ttcn::ActualParList *parlist = new Ttcn::ActualParList;
      chk_expr_operand_execute_refd(u.expr.v1, u.expr.t_list2->get_tis(), parlist,
                                    u.expr.v3, the, opname);
      delete u.expr.t_list2;
      u.expr.ap_list2 = parlist;
      chk_expr_dynamic_part(exp_val, true);
      break; }
    case OPTYPE_DECOMP:
      error("Built-in function `%s' is not yet supported", opname);
      set_valuetype(V_ERROR);
      break;
    case OPTYPE_REPLACE: {
      Type::expected_value_t ti_exp_val = exp_val;
      if (ti_exp_val == Type::EXPECTED_DYNAMIC_VALUE)
        ti_exp_val = Type::EXPECTED_TEMPLATE;
      {
        Error_Context cntxt(this, "In the first operand of operation `%s'",
                            opname);
        Type* governor = chk_expr_operands_ti(u.expr.ti1, ti_exp_val);
        if (!governor) return;
        chk_expr_eval_ti(u.expr.ti1, governor, refch, ti_exp_val);
        if (valuetype != V_ERROR)
          u.expr.ti1->get_Template()->chk_specific_value(false);
        chk_expr_operandtype_list(governor, first, opname, u.expr.ti1, false);
      }
      v2 = u.expr.v2;
      {
        Error_Context cntxt(this, "In the second operand of operation `%s'",
                            opname);
        v2->set_lowerid_to_ref();
        tt2 = v2->get_expr_returntype(exp_val);
        chk_expr_operandtype_int(tt2, second, opname, v2);
        chk_expr_eval_value(v2, t_chk, refch, exp_val);
        chk_expr_val_int_pos0(v2, second, opname);
      }
      v3 = u.expr.v3;
      {
        Error_Context cntxt(this, "In the third operand of operation `%s'",
                            opname);
        v3->set_lowerid_to_ref();
        tt3 = v3->get_expr_returntype(exp_val);
        chk_expr_operandtype_int(tt3, third, opname, v3);
        chk_expr_eval_value(v3, t_chk, refch, exp_val);
        chk_expr_val_int_pos0(v3, third, opname);
      }
      {
        Error_Context cntxt(this, "In the fourth operand of operation `%s'",
                            opname);
        Type* governor = chk_expr_operands_ti(u.expr.ti4, ti_exp_val);
        if (!governor) return;
        chk_expr_eval_ti(u.expr.ti4, governor, refch, ti_exp_val);
        if (valuetype != V_ERROR)
          u.expr.ti4->get_Template()->chk_specific_value(false);
        chk_expr_operandtype_list(governor, fourth, opname, u.expr.ti4, false);
      }
      chk_expr_operands_replace();
      break; }
    case OPTYPE_LOG2STR:
    case OPTYPE_ANY2UNISTR: {
      Error_Context cntxt(this, "In the operand of operation `%s'", opname);
      u.expr.logargs->chk();
      if (!semantic_check_only) u.expr.logargs->join_strings();
      break; }
    case OPTYPE_TTCN2STRING: {
      Error_Context cntxt(this, "In the parameter of ttcn2string()");
      Type::expected_value_t ti_exp_val = exp_val;
      if (ti_exp_val == Type::EXPECTED_DYNAMIC_VALUE) ti_exp_val = Type::EXPECTED_TEMPLATE;
      Type *governor = chk_expr_operands_ti(u.expr.ti1, ti_exp_val);
      if (!governor) return;
      chk_expr_eval_ti(u.expr.ti1, governor, refch, ti_exp_val);
    } break;
    default:
      FATAL_ERROR("chk_expr_operands()");
    } // switch optype
  }

  // Compile-time evaluation. It may change the valuetype from V_EXPR to
  // the result of evaluating the expression.  E.g. V_BOOL for
  // OPTYPE_ISCHOSEN.
  void Value::evaluate_value(ReferenceChain *refch,
                             Type::expected_value_t exp_val)
  {
    if(valuetype!=V_EXPR) FATAL_ERROR("Value::evaluate_value()");
    if(u.expr.state!=EXPR_NOT_CHECKED) return;

    u.expr.state=EXPR_CHECKING;

    get_expr_returntype(exp_val); // to report 'didyamean'-errors etc
    chk_expr_operands(refch, exp_val == Type::EXPECTED_TEMPLATE ?
      Type::EXPECTED_DYNAMIC_VALUE : exp_val);

    if(valuetype==V_ERROR) return;
    if(u.expr.state==EXPR_CHECKING_ERR) {
      u.expr.state=EXPR_CHECKED;
      set_valuetype(V_ERROR);
      return;
    }

    u.expr.state=EXPR_CHECKED;

    Value *v1, *v2, *v3, *v4;
    switch(u.expr.v_optype) {
    case OPTYPE_RND: // -
    case OPTYPE_COMP_NULL: // the only foldable in this group
    case OPTYPE_COMP_MTC:
    case OPTYPE_COMP_SYSTEM:
    case OPTYPE_COMP_SELF:
    case OPTYPE_COMP_RUNNING_ANY:
    case OPTYPE_COMP_RUNNING_ALL:
    case OPTYPE_COMP_ALIVE_ANY:
    case OPTYPE_COMP_ALIVE_ALL:
    case OPTYPE_TMR_RUNNING_ANY:
    case OPTYPE_GETVERDICT:
    case OPTYPE_PROF_RUNNING:
    case OPTYPE_RNDWITHVAL: // v1
    case OPTYPE_COMP_RUNNING: // v1
    case OPTYPE_COMP_ALIVE:
    case OPTYPE_TMR_READ:
    case OPTYPE_TMR_RUNNING:
    case OPTYPE_ACTIVATE:
    case OPTYPE_ACTIVATE_REFD:
    case OPTYPE_EXECUTE: // r1 [v2]
    case OPTYPE_EXECUTE_REFD: // v1 t_list2 [v3]
    case OPTYPE_COMP_CREATE: // r1 [v2] [v3] b4
    case OPTYPE_MATCH: // v1 t2
    case OPTYPE_ISCHOSEN_T:
    case OPTYPE_LOG2STR:
    case OPTYPE_ANY2UNISTR:
    case OPTYPE_ENCODE:
    case OPTYPE_DECODE:
    case OPTYPE_ISBOUND:
    case OPTYPE_ISPRESENT:
    case OPTYPE_TTCN2STRING:
    case OPTYPE_UNICHAR2OCT:
    case OPTYPE_OCT2UNICHAR:
    case OPTYPE_ENCODE_BASE64:
    case OPTYPE_DECODE_BASE64:
    case OPTYPE_ENCVALUE_UNICHAR:
    case OPTYPE_DECVALUE_UNICHAR:
    case OPTYPE_CHECKSTATE_ANY:
    case OPTYPE_CHECKSTATE_ALL:
    case OPTYPE_HOSTID:
    case OPTYPE_ISTEMPLATEKIND: // ti1 v2
      break;
    case OPTYPE_TESTCASENAME: { // -
      if (!my_scope) FATAL_ERROR("Value::evaluate_value()");
      Ttcn::StatementBlock *my_sb =
        dynamic_cast<Ttcn::StatementBlock *>(my_scope);
      if (!my_sb) break;
      Ttcn::Definition *my_def = my_sb->get_my_def();
      if (!my_def) { // In control part.
        set_val_str(new string(""));
        valuetype = V_CSTR;
      } else if (my_def->get_asstype() == Assignment::A_TESTCASE) {
        set_val_str(new string(my_def->get_id().get_dispname()));
        valuetype = V_CSTR;
      }
      break; }
    case OPTYPE_UNARYPLUS: // v1
      v1=u.expr.v1;
      u.expr.v1=0;
      copy_and_destroy(v1);
      break;
    case OPTYPE_UNARYMINUS:
      if (is_unfoldable()) break;
      v1 = u.expr.v1->get_value_refd_last();
      switch (v1->valuetype) {
      case V_INT: {
        int_val_t *i = new int_val_t(-*(v1->get_val_Int()));
        if (!i) FATAL_ERROR("Value::evaluate_value()");
        clean_up();
        valuetype = V_INT;
        u.val_Int = i;
        break; }
      case V_REAL: {
        ttcn3float r = v1->get_val_Real();
        clean_up();
        valuetype = V_REAL;
        u.val_Real = -r;
        break; }
      default:
        FATAL_ERROR("Value::evaluate_value()");
      }
      break;
    case OPTYPE_NOT: {
      if(is_unfoldable()) break;
      bool b=u.expr.v1->get_value_refd_last()->get_val_bool();
      clean_up();
      valuetype=V_BOOL;
      u.val_bool=!b;
      break;}
    case OPTYPE_NOT4B: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      const string& s = v1->get_val_str();
      valuetype_t vt=v1->valuetype;
      clean_up();
      valuetype=vt;
      set_val_str(vt==V_BSTR?not4b_bit(s):not4b_hex(s));
      break;}
    case OPTYPE_BIT2HEX: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      const string& s = v1->get_val_str();
      clean_up();
      valuetype=V_HSTR;
      set_val_str(bit2hex(s));
      break;}
    case OPTYPE_BIT2OCT: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      const string& s = v1->get_val_str();
      clean_up();
      valuetype=V_OSTR;
      set_val_str(bit2oct(s));
      break;}
    case OPTYPE_BIT2STR:
    case OPTYPE_HEX2STR:
    case OPTYPE_OCT2STR: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      const string& s = v1->get_val_str();
      clean_up();
      valuetype=V_CSTR;
      set_val_str(new string(s));
      break;}
    case OPTYPE_BIT2INT: {
      if (is_unfoldable()) break;
      v1 = u.expr.v1->get_value_refd_last();
      const string& s = v1->get_val_str();
      clean_up();
      valuetype = V_INT;
      u.val_Int = bit2int(s);
      break; }
    case OPTYPE_CHAR2INT: {
      if (is_unfoldable()) break;
      v1 = u.expr.v1->get_value_refd_last();
      char c = v1->get_val_str()[0];
      clean_up();
      valuetype = V_INT;
      u.val_Int = new int_val_t((Int)c);
      break; }
    case OPTYPE_CHAR2OCT: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      const string& s = v1->get_val_str();
      clean_up();
      valuetype=V_OSTR;
      set_val_str(char2oct(s));
      break;}
    case OPTYPE_STR2INT: {
      if (is_unfoldable()) break;
      v1 = u.expr.v1->get_value_refd_last();
      int_val_t *i = new int_val_t((v1->get_val_str()).c_str(), *u.expr.v1);
      clean_up();
      valuetype = V_INT;
      u.val_Int = i;
      /** \todo hiba eseten lenyeli... */
      break; }
    case OPTYPE_STR2FLOAT: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      Real r=string2Real(v1->get_val_str(), *u.expr.v1);
      clean_up();
      valuetype=V_REAL;
      u.val_Real=r;
      /** \todo hiba eseten lenyeli... */
      break;}
    case OPTYPE_STR2BIT: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      const string& s = v1->get_val_str();
      clean_up();
      valuetype=V_BSTR;
      set_val_str(new string(s));
      break;}
    case OPTYPE_STR2HEX:
    case OPTYPE_OCT2HEX: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      const string& s = v1->get_val_str();
      clean_up();
      valuetype=V_HSTR;
      set_val_str(to_uppercase(s));
      break;}
    case OPTYPE_STR2OCT: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      const string& s = v1->get_val_str();
      clean_up();
      valuetype=V_OSTR;
      set_val_str(to_uppercase(s));
      break;}
    case OPTYPE_FLOAT2INT: {
      if (is_unfoldable()) break;
      v1 = u.expr.v1->get_value_refd_last();
      ttcn3float r = v1->get_val_Real();
      clean_up();
      valuetype = V_INT;
      u.val_Int = float2int(r, *u.expr.v1);
      break;}
    case OPTYPE_FLOAT2STR: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      ttcn3float r=v1->get_val_Real();
      clean_up();
      valuetype=V_CSTR;
      set_val_str(float2str(r));
      break;}
    case OPTYPE_HEX2BIT:
    case OPTYPE_OCT2BIT: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      const string& s = v1->get_val_str();
      clean_up();
      valuetype=V_BSTR;
      set_val_str(hex2bit(s));
      break;}
    case OPTYPE_HEX2INT:
    case OPTYPE_OCT2INT: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      const string& s = v1->get_val_str();
      clean_up();
      valuetype=V_INT;
      u.val_Int=hex2int(s);
      break;}
    case OPTYPE_HEX2OCT: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      const string& s = v1->get_val_str();
      clean_up();
      valuetype=V_OSTR;
      set_val_str(hex2oct(s));
      break;}
    case OPTYPE_INT2CHAR: {
      if (is_unfoldable()) break;
      v1 = u.expr.v1->get_value_refd_last();
      const int_val_t *c_int = v1->get_val_Int();
      char c = static_cast<char>(c_int->get_val());
      clean_up();
      valuetype = V_CSTR;
      set_val_str(new string(1, &c));
      break; }
    case OPTYPE_INT2UNICHAR: {
      if (is_unfoldable()) break;
      v1 = u.expr.v1->get_value_refd_last();
      const int_val_t *i_int = v1->get_val_Int();
      Int i = i_int->get_val();
      clean_up();
      valuetype = V_USTR;
      set_val_ustr(int2unichar(i));
      u.ustr.convert_str = false;
      break; }
    case OPTYPE_INT2FLOAT: {
      if (is_unfoldable()) break;
      v1 = u.expr.v1->get_value_refd_last();
      const int_val_t *i_int = v1->get_val_Int();
      Real i_int_real = i_int->to_real();
      clean_up();
      valuetype = V_REAL;
      u.val_Real = i_int_real;
      break; }
    case OPTYPE_INT2STR: {
      if (is_unfoldable()) break;
      v1 = u.expr.v1->get_value_refd_last();
      const int_val_t *i_int = v1->get_val_Int();
      string *i_int_str = new string(i_int->t_str());
      clean_up();
      valuetype = V_CSTR;
      set_val_str(i_int_str);
      break; }
    case OPTYPE_OCT2CHAR: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      const string& s = v1->get_val_str();
      clean_up();
      valuetype=V_CSTR;
      set_val_str(oct2char(s));
      break;}
    case OPTYPE_GET_STRINGENCODING: {
      if(is_unfoldable()) break;
      v1 = u.expr.v1->get_value_refd_last();
      const string& s1 = v1->get_val_str();
      clean_up();
      valuetype = V_CSTR;
      set_val_str(get_stringencoding(s1));
      break;}
    case OPTYPE_REMOVE_BOM: {
      if(is_unfoldable()) break;
      v1 = u.expr.v1->get_value_refd_last();
      const string& s1 = v1->get_val_str();
      clean_up();
      valuetype = V_OSTR;
      set_val_str(remove_bom(s1));
      break;}
    case OPTYPE_ENUM2INT: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      Type* enum_type = v1->get_my_governor();
      const Int& enum_val = enum_type->get_enum_val_byId(*(v1->u.val_id));
      clean_up();
      valuetype = V_INT;
      u.val_Int = new int_val_t(enum_val);
      break;}
    case OPTYPE_UNICHAR2INT:
      if (is_unfoldable()) {
	// replace the operation with char2int() if the operand is a charstring
	// value to avoid its unnecessary conversion to universal charstring
	if (u.expr.v1->get_expr_returntype(exp_val) == Type::T_CSTR)
	  u.expr.v_optype = OPTYPE_CHAR2INT;
      } else {
	v1=u.expr.v1->get_value_refd_last();
	const ustring& s = v1->get_val_ustr();
	clean_up();
	valuetype=V_INT;
	u.val_Int=new int_val_t(unichar2int(s));
      }
      break;
    case OPTYPE_UNICHAR2CHAR:
      v1 = u.expr.v1;
      if (is_unfoldable()) {
	// replace the operation with its operand if it is a charstring
	// value to avoid its unnecessary conversion to universal charstring
	if (v1->get_expr_returntype(exp_val) == Type::T_CSTR) {
	  u.expr.v1 = 0;
	  copy_and_destroy(v1);
	}
      } else {
	v1 = v1->get_value_refd_last();
	const ustring& s = v1->get_val_ustr();
	clean_up();
	valuetype = V_CSTR;
	set_val_str(new string(s));
      }
      break;
    case OPTYPE_MULTIPLY: { // v1 v2
      if (!is_unfoldable()) goto eval_arithmetic;
      v1 = u.expr.v1->get_value_refd_last();
      v2 = u.expr.v2->get_value_refd_last();
      if (v1->is_unfoldable()) v1 = v2;
      if (v1->is_unfoldable()) break;
      switch(v1->valuetype) {
      case V_INT: {
        if (*v1->get_val_Int() != 0) break;
        clean_up();
        valuetype = V_INT;
        u.val_Int = new int_val_t((Int)0);
        break; }
      case V_REAL: {
        if (v1->get_val_Real() != 0.0) break;
        clean_up();
        valuetype = V_REAL;
        u.val_Real = 0.0;
        break; }
      default:
        FATAL_ERROR("Value::evaluate_value()");
      }
      break; }
    case OPTYPE_ADD: // v1 v2
    case OPTYPE_SUBTRACT:
    case OPTYPE_DIVIDE:
    case OPTYPE_MOD:
    case OPTYPE_REM: {
      eval_arithmetic:
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      v2=u.expr.v2->get_value_refd_last();
      operationtype_t ot=u.expr.v_optype;
      switch (v1->valuetype) {
      case V_INT: {
        const int_val_t *i1 = new int_val_t(*(v1->get_val_Int()));
        const int_val_t *i2 = new int_val_t(*(v2->get_val_Int()));
        clean_up();
        valuetype = V_INT;
        switch (ot) {
        case OPTYPE_ADD:
          u.val_Int = new int_val_t(*i1 + *i2);
          break;
        case OPTYPE_SUBTRACT:
          u.val_Int = new int_val_t(*i1 - *i2);
          break;
        case OPTYPE_MULTIPLY:
          u.val_Int = new int_val_t(*i1 * *i2);
          break;
        case OPTYPE_DIVIDE:
          u.val_Int = new int_val_t(*i1 / *i2);
          break;
        case OPTYPE_MOD:
          u.val_Int = new int_val_t(mod(*i1, *i2));
          break;
        case OPTYPE_REM:
          u.val_Int = new int_val_t(rem(*i1, *i2));
          break;
        default:
          FATAL_ERROR("Value::evaluate_value()");
        }
        delete i1;
        delete i2;
        break; }
      case V_REAL: {
        ttcn3float r1=v1->get_val_Real();
        ttcn3float r2=v2->get_val_Real();
        clean_up();
        valuetype=V_REAL;
        switch(ot) {
        case OPTYPE_ADD:
          u.val_Real=r1+r2;
          break;
        case OPTYPE_SUBTRACT:
          u.val_Real=r1-r2;
          break;
        case OPTYPE_MULTIPLY:
          u.val_Real=r1*r2;
          break;
        case OPTYPE_DIVIDE:
          u.val_Real=r1/r2;
          break;
        default:
          FATAL_ERROR("Value::evaluate_value()");
        }
        break;}
      default:
        FATAL_ERROR("Value::evaluate_value()");
      }
      break;}
    case OPTYPE_CONCAT: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      v2=u.expr.v2->get_value_refd_last();
      valuetype_t vt = v1->valuetype;
      if (vt == V_USTR || v2->valuetype == V_USTR) { // V_USTR wins
        const ustring& s1 = v1->get_val_ustr();
        const ustring& s2 = v2->get_val_ustr();
        clean_up();
        valuetype = V_USTR;
        set_val_ustr(new ustring(s1 + s2));
        u.ustr.convert_str = false;
      } else {
        const string& s1 = v1->get_val_str();
        const string& s2 = v2->get_val_str();
        clean_up();
        valuetype = vt;
        set_val_str(new string(s1 + s2));
      }
      break;}
    case OPTYPE_EQ: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      v2=u.expr.v2->get_value_refd_last();
      bool b=*v1==*v2;
      clean_up();
      valuetype=V_BOOL;
      u.val_bool=b;
      break;}
    case OPTYPE_NE: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      v2=u.expr.v2->get_value_refd_last();
      bool b=*v1==*v2;
      clean_up();
      valuetype=V_BOOL;
      u.val_bool=!b;
      break;}
    case OPTYPE_LT: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      v2=u.expr.v2->get_value_refd_last();
      bool b=*v1<*v2;
      clean_up();
      valuetype=V_BOOL;
      u.val_bool=b;
      break;}
    case OPTYPE_GT: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      v2=u.expr.v2->get_value_refd_last();
      bool b=*v2<*v1;
      clean_up();
      valuetype=V_BOOL;
      u.val_bool=b;
      break;}
    case OPTYPE_GE: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      v2=u.expr.v2->get_value_refd_last();
      bool b=*v1<*v2;
      clean_up();
      valuetype=V_BOOL;
      u.val_bool=!b;
      break;}
    case OPTYPE_LE: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      v2=u.expr.v2->get_value_refd_last();
      bool b=*v2<*v1;
      clean_up();
      valuetype=V_BOOL;
      u.val_bool=!b;
      break;}
    case OPTYPE_AND:
      v1 = u.expr.v1->get_value_refd_last();
      if (v1->valuetype == V_BOOL) {
	if (v1->get_val_bool()) {
	  // the left operand is a literal "true"
	  // substitute the expression with the right operand
	  v2 = u.expr.v2;
	  u.expr.v2 = 0;
	  copy_and_destroy(v2);
	} else {
	  // the left operand is a literal "false"
	  // the result must be false regardless the right operand
	  // because of the short circuit evaluation rule
	  clean_up();
	  valuetype = V_BOOL;
	  u.val_bool = false;
	}
      } else {
	// we must keep the left operand because of the potential side effects
	// the right operand can only be eliminated if it is a literal "true"
	v2 = u.expr.v2->get_value_refd_last();
	if (v2->valuetype == V_BOOL && v2->get_val_bool()) {
	  v1 = u.expr.v1;
	  u.expr.v1 = 0;
	  copy_and_destroy(v1);
	}
      }
      break;
    case OPTYPE_OR:
      v1 = u.expr.v1->get_value_refd_last();
      if (v1->valuetype == V_BOOL) {
	if (v1->get_val_bool()) {
	  // the left operand is a literal "true"
	  // the result must be true regardless the right operand
	  // because of the short circuit evaluation rule
	  clean_up();
	  valuetype = V_BOOL;
	  u.val_bool = true;
	} else {
	  // the left operand is a literal "false"
	  // substitute the expression with the right operand
	  v2 = u.expr.v2;
	  u.expr.v2 = 0;
	  copy_and_destroy(v2);
	}
      } else {
	// we must keep the left operand because of the potential side effects
	// the right operand can only be eliminated if it is a literal "false"
	v2 = u.expr.v2->get_value_refd_last();
	if (v2->valuetype == V_BOOL && !v2->get_val_bool()) {
	  v1 = u.expr.v1;
	  u.expr.v1 = 0;
	  copy_and_destroy(v1);
	}
      }
      break;
    case OPTYPE_XOR: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      v2=u.expr.v2->get_value_refd_last();
      bool b=v1->get_val_bool() ^ v2->get_val_bool();
      clean_up();
      valuetype=V_BOOL;
      u.val_bool=b;
      break;}
    case OPTYPE_AND4B: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      v2=u.expr.v2->get_value_refd_last();
      valuetype_t vt=v1->valuetype;
      const string& s1 = v1->get_val_str();
      const string& s2 = v2->get_val_str();
      clean_up();
      valuetype=vt;
      set_val_str(and4b(s1, s2));
      break;}
    case OPTYPE_OR4B: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      v2=u.expr.v2->get_value_refd_last();
      valuetype_t vt=v1->valuetype;
      const string& s1 = v1->get_val_str();
      const string& s2 = v2->get_val_str();
      clean_up();
      valuetype=vt;
      set_val_str(or4b(s1, s2));
      break;}
    case OPTYPE_XOR4B: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      v2=u.expr.v2->get_value_refd_last();
      valuetype_t vt=v1->valuetype;
      const string& s1 = v1->get_val_str();
      const string& s2 = v2->get_val_str();
      clean_up();
      valuetype=vt;
      set_val_str(xor4b(s1, s2));
      break;}
    case OPTYPE_SHL: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      v2=u.expr.v2->get_value_refd_last();
      valuetype_t vt=v1->valuetype;
      const string& s = v1->get_val_str();
      const int_val_t *i_int = v2->get_val_Int();
      Int i=i_int->get_val();
      if(vt==V_OSTR) i*=2;
      clean_up();
      valuetype=vt;
      set_val_str(shift_left(s, i));
      break;}
    case OPTYPE_SHR: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      v2=u.expr.v2->get_value_refd_last();
      valuetype_t vt=v1->valuetype;
      const string& s = v1->get_val_str();
      const int_val_t *i_int = v2->get_val_Int();
      Int i=i_int->get_val();
      if(vt==V_OSTR) i*=2;
      clean_up();
      valuetype=vt;
      set_val_str(shift_right(s, i));
      break;}
    case OPTYPE_ROTL: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      v2=u.expr.v2->get_value_refd_last();
      valuetype_t vt=v1->valuetype;
      const int_val_t *i_int=v2->get_val_Int();
      Int i=i_int->get_val();
      if(vt==V_USTR) {
        const ustring& s = v1->get_val_ustr();
        clean_up();
        valuetype=vt;
        set_val_ustr(rotate_left(s, i));
        u.ustr.convert_str = false;
      }
      else {
        if(vt==V_OSTR) i*=2;
        const string& s = v1->get_val_str();
        clean_up();
        valuetype=vt;
        set_val_str(rotate_left(s, i));
      }
      break;}
    case OPTYPE_ROTR: {
      if(is_unfoldable()) break;
      v1=u.expr.v1->get_value_refd_last();
      v2=u.expr.v2->get_value_refd_last();
      valuetype_t vt=v1->valuetype;
      const int_val_t *i_int=v2->get_val_Int();
      Int i=i_int->get_val();
      if(vt==V_USTR) {
        const ustring& s = v1->get_val_ustr();
        clean_up();
        valuetype=vt;
        set_val_ustr(rotate_right(s, i));
        u.ustr.convert_str = false;
      }
      else {
        if(vt==V_OSTR) i*=2;
        const string& s = v1->get_val_str();
        clean_up();
        valuetype=vt;
        set_val_str(rotate_right(s, i));
      }
      break;}
    case OPTYPE_INT2BIT: {
      if (is_unfoldable()) break;
      v1 = u.expr.v1->get_value_refd_last();
      v2 = u.expr.v2->get_value_refd_last();
      const int_val_t *i1_int = v1->get_val_Int();
      const int_val_t *i2_int = v2->get_val_Int();
      string *val = int2bit(*i1_int, i2_int->get_val());
      clean_up();
      valuetype = V_BSTR;
      set_val_str(val);
      break; }
    case OPTYPE_INT2HEX: {
      if (is_unfoldable()) break;
      v1 = u.expr.v1->get_value_refd_last();
      v2 = u.expr.v2->get_value_refd_last();
      const int_val_t *i1_int = v1->get_val_Int();
      const int_val_t *i2_int = v2->get_val_Int();
      // Do it before the `clean_up'.  i2_int is already checked.
      string *val = int2hex(*i1_int, i2_int->get_val());
      clean_up();
      valuetype = V_HSTR;
      set_val_str(val);
      break; }
    case OPTYPE_INT2OCT: {
      if (is_unfoldable()) break;
      v1 = u.expr.v1->get_value_refd_last();
      v2 = u.expr.v2->get_value_refd_last();
      const int_val_t i1_int(*v1->get_val_Int());
      // `v2' is a native integer.
      Int i2_int = v2->get_val_Int()->get_val() * 2;
      clean_up();
      valuetype = V_OSTR;
      set_val_str(int2hex(i1_int, i2_int));
      break; }
    case OPTYPE_SUBSTR: {
      if(is_unfoldable()) break;
      v1=u.expr.ti1->get_specific_value()->get_value_refd_last();
      v2=u.expr.v2->get_value_refd_last();
      v3=u.expr.v3->get_value_refd_last();
      valuetype_t vt=v1->valuetype;
      const int_val_t *i2_int=v2->get_val_Int();
      const int_val_t *i3_int=v3->get_val_Int();
      Int i2=i2_int->get_val();
      Int i3=i3_int->get_val();
      if(vt==V_USTR) {
        const ustring& s = v1->get_val_ustr();
        clean_up();
        valuetype=vt;
        set_val_ustr(new ustring(s.substr(i2, i3)));
        u.ustr.convert_str = false;
      }
      else {
        if(vt==V_OSTR) {
          i2*=2;
          i3*=2;
        }
        const string& s = v1->get_val_str();
        clean_up();
        valuetype=vt;
        set_val_str(new string(s.substr(i2, i3)));
      }
      break;}
    case OPTYPE_REPLACE: {
    	if(is_unfoldable()) break;
    	v1=u.expr.ti1->get_specific_value()->get_value_refd_last();
    	v2=u.expr.v2->get_value_refd_last();
    	v3=u.expr.v3->get_value_refd_last();
    	v4=u.expr.ti4->get_specific_value()->get_value_refd_last();
    	valuetype_t vt=v1->valuetype;
    	const int_val_t *i2_int=v2->get_val_Int();
    	const int_val_t *i3_int=v3->get_val_Int();
    	Int i2=i2_int->get_val();
    	Int i3=i3_int->get_val();
    	switch(vt) {
    	case V_BSTR: {
       	  string *s1 = new string(v1->get_val_str());
       	  const string& s2 = v4->get_val_str();
       	  clean_up();
    	  valuetype=vt;
    	  s1->replace(i2, i3, s2);
    	  set_val_str(s1);
    	  break;}
    	case V_HSTR: {
      	  string *s1 = new string(v1->get_val_str());
      	  const string& s2 = v4->get_val_str();
      	  clean_up();
          valuetype=vt;
    	  s1->replace(i2, i3, s2);
    	  set_val_str(s1);
    	  break;}
    	case V_OSTR: {
    	  i2*=2;
    	  i3*=2;
    	  string *s1 = new string(v1->get_val_str());
    	  const string& s2 = v4->get_val_str();
    	  clean_up();
    	  valuetype=vt;
    	  s1->replace(i2, i3, s2);
    	  set_val_str(s1);
    	  break;}
    	case V_CSTR: {
    	  string *s1 = new string(v1->get_val_str());
    	  const string& s2 = v4->get_val_str();
    	  clean_up();
          valuetype=vt;
          s1->replace(i2, i3, s2);
          set_val_str(s1);
    	  break;}
    	case V_USTR: {
      	  ustring *s1 = new ustring(v1->get_val_ustr());
      	  const ustring& s2 = v4->get_val_ustr();
      	  clean_up();
          valuetype=vt;
          s1->replace(i2, i3, s2);
          set_val_ustr(s1);
          u.ustr.convert_str = false;
      	  break;}
    	default:
          FATAL_ERROR("Value::evaluate_value()");
    	}
    	break; }
    case OPTYPE_REGEXP: {
      if (is_unfoldable()) break;
      v1=u.expr.ti1->get_specific_value()->get_value_refd_last();
      v2=u.expr.t2->get_specific_value()->get_value_refd_last();
      v3=u.expr.v3->get_value_refd_last();
      const int_val_t *i3_int = v3->get_val_Int();
      Int i3 = i3_int->get_val();
      if (v1->valuetype == V_CSTR) {
        const string& s1 = v1->get_val_str();
        const string& s2 = v2->get_val_str();
	string *result = regexp(s1, s2, i3, u.expr.b4);
	clean_up();
	valuetype = V_CSTR;
	set_val_str(result);
      } if (v1->valuetype == V_USTR) {
        const ustring& s1 = v1->get_val_ustr();
        const ustring& s2 = v2->get_val_ustr();
        ustring *result = regexp(s1, s2, i3, u.expr.b4);
        clean_up();
        valuetype = V_USTR;
        set_val_ustr(result);
        u.ustr.convert_str = false;
      }
      break; }
    case OPTYPE_LENGTHOF:{
      if(is_unfoldable()) break;
      v1=u.expr.ti1->get_Template()->get_specific_value()
        ->get_value_refd_last();
      size_t i;
      if(v1->is_string_type(exp_val)) {
        i=v1->get_val_strlen();
      } else { // v1 is be seq/set of or array
        switch (v1->valuetype) {
        case V_SEQOF:
        case V_SETOF:
        case V_ARRAY: {
          if(v1->u.val_vs->is_indexed())
        	{ i = v1->u.val_vs->get_nof_ivs();}
          else { i = v1->u.val_vs->get_nof_vs();}
          break; }
        default:
          FATAL_ERROR("Value::evaluate_value()");
        }
      }
      clean_up();
      valuetype=V_INT;
      u.val_Int=new int_val_t(i);
      break;}
    case OPTYPE_SIZEOF: {
      Int i=chk_eval_expr_sizeof(refch, exp_val);
      if(i!=-1) {
        clean_up();
        valuetype=V_INT;
        u.val_Int=new int_val_t(i);
      }
      break;}
    case OPTYPE_ISVALUE: {
      if(is_unfoldable()) break;
      bool is_singleval = !u.expr.ti1->get_DerivedRef()
      	&& u.expr.ti1->get_Template()->is_Value();
      if (is_singleval) {
        Value * other_val = u.expr.ti1->get_Template()->get_Value();
        is_singleval = other_val->evaluate_isvalue(false);
        // is_singleval now contains the compile-time result of isvalue
        delete other_val;
      }
      clean_up();
      valuetype = V_BOOL;
      u.val_bool = is_singleval;
      break;}
    case OPTYPE_ISCHOSEN_V: {
      if (is_unfoldable()) break;
      v1 = u.expr.v1->get_value_refd_last();
      bool b = v1->field_is_chosen(*u.expr.i2);
      clean_up();
      valuetype = V_BOOL;
      u.val_bool = b;
      break; }
    case OPTYPE_VALUEOF: // ti1
      if (!u.expr.ti1->get_DerivedRef() &&
          u.expr.ti1->get_Template()->is_Value() &&
          !u.expr.ti1->get_Type()) {
        // FIXME actually if the template instance has a type
        // it might still be foldable.
        // the argument is a single specific value
        v1 = u.expr.ti1->get_Template()->get_Value();
        Type *governor = my_governor;
        if (governor == NULL) {
          governor = u.expr.ti1->get_expr_governor(exp_val);
          if (governor != NULL) governor = governor->get_type_refd_last();
        }
        if (governor == NULL) governor = v1->get_my_governor()->get_type_refd_last();
        if (governor == NULL)
          FATAL_ERROR("Value::evaluate_value()");
        clean_up();
        valuetype = v1->valuetype;
        u = v1->u;
        set_my_governor(governor);
        if (valuetype == V_REFD && u.ref.refd_last == v1)
          u.ref.refd_last = this;
        v1->valuetype = V_ERROR;
        delete v1;
      }
      break;
    case OPTYPE_UNDEF_RUNNING:
    default:
      FATAL_ERROR("Value::evaluate_value()");
    } // switch optype
  }

  bool Value::evaluate_isvalue(bool from_sequence)
  {
    switch (valuetype) {
    case V_OMIT:
      // Omit is not a value unless a member of a sequence or set
      return from_sequence;
    case V_NOTUSED:
      return false;
    case V_NULL: /**< NULL (for ASN.1 NULL type, also in TTCN-3) */
    case V_BOOL: /**< boolean */
    case V_NAMEDINT: /**< integer / named number */
    case V_NAMEDBITS: /**< named bits (identifiers) */
    case V_INT: /**< integer */
    case V_REAL: /**< real/float */
    case V_ENUM: /**< enumerated */
    case V_BSTR: /**< bitstring */
    case V_HSTR: /**< hexstring */
    case V_OSTR: /**< octetstring */
    case V_CSTR: /**< charstring */
    case V_USTR: /**< universal charstring */
    case V_ISO2022STR: /**< ISO-2022 string (treat as octetstring) */
    case V_CHARSYMS: /**< parsed ASN.1 universal string notation */
    case V_OID: /**< object identifier */
    case V_ROID: /**< relative object identifier */
    case V_VERDICT: /**< all verdicts */
      return true; // values of built-in types return true

      // Code below was adapted from is_unfoldable(), false returned early.
    case V_CHOICE:
      return u.choice.alt_value->evaluate_isvalue(false);

    case V_SEQOF:
    case V_SETOF:
    case V_ARRAY:
      for (size_t i = 0; i < u.val_vs->get_nof_vs(); i++) {
        if (!u.val_vs->get_v_byIndex(i)->evaluate_isvalue(false)) {
          return false;
        }
      }
      return true;

    case V_SEQ:
    case V_SET:
      for (size_t i = 0; i < u.val_nvs->get_nof_nvs(); i++) {
        if (!u.val_nvs->get_nv_byIndex(i)->get_value()
          ->evaluate_isvalue(true)) return false;
      }
      return true;

    case V_REFD:
      // alas, get_value_refd_last prevents this function from const
      return get_value_refd_last()->evaluate_isvalue(false);

    case V_EXPR:
      switch (u.expr.v_optype) {
      // A constant null component reference is a corner case: it is foldable
      // but escapes unmodified from evaluate_value.
      // A V_EXPR with any other OPTYPE_ is either unfoldable,
      // or is transformed into some other valuetype in evaluate_value.
      case OPTYPE_COMP_NULL:
        return false;
      default:
        break; // and fall through to the FATAL_ERROR
      }
      // no break
    default:
      FATAL_ERROR("Value::evaluate_isvalue()");
      break;
    }
    return true;
  }

  void Value::evaluate_macro(Type::expected_value_t exp_val)
  {
    switch (u.macro) {
    case MACRO_MODULEID:
      if (!my_scope)
	FATAL_ERROR("Value::evaluate_macro(): my_scope is not set");
      set_val_str(new string(my_scope->get_scope_mod()
	->get_modid().get_dispname()));
      valuetype = V_CSTR;
      break;
    case MACRO_FILENAME:
    case MACRO_BFILENAME: {
      const char *t_filename = get_filename();
      if (!t_filename)
	FATAL_ERROR("Value::evaluate_macro(): file name is not set");
      set_val_str(new string(t_filename));
      valuetype = V_CSTR;
      break; }
    case MACRO_FILEPATH: {
      const char *t_filename = get_filename();
      if (!t_filename)
        FATAL_ERROR("Value::evaluate_macro(): file name is not set");
      char *t_filepath = canonize_input_file(t_filename);
      if (!t_filepath)
        FATAL_ERROR("Value::evaluate_macro(): file path cannot be determined");
      set_val_str(new string(t_filepath));
      valuetype = V_CSTR;
      Free(t_filepath);
      break; }
    case MACRO_LINENUMBER: {
      int t_lineno = get_first_line();
      if (t_lineno <= 0)
	FATAL_ERROR("Value::evaluate_macro(): line number is not set");
      set_val_str(new string(Int2string(t_lineno)));
      valuetype = V_CSTR;
      break; }
    case MACRO_LINENUMBER_C: {
      int t_lineno = get_first_line();
      if (t_lineno <= 0)
	FATAL_ERROR("Value::evaluate_macro(): line number is not set");
      u.val_Int = new int_val_t(t_lineno);
      valuetype = V_INT;
      break; }
    case MACRO_DEFINITIONID: {
      // cut the second part from the fullname separated by dots
      const string& t_fullname = get_fullname();
      size_t first_char = t_fullname.find('.') + 1;
      if (first_char >= t_fullname.size())
	FATAL_ERROR("Value::evaluate_macro(): malformed fullname: `%s'", \
	  t_fullname.c_str());
      set_val_str(new string(t_fullname.substr(first_char,
	t_fullname.find('.', first_char) - first_char)));
      valuetype = V_CSTR;
      break; }
    case MACRO_SCOPE: {
      if (!my_scope) FATAL_ERROR("Value::evaluate_macro(): scope is not set");
      set_val_str(new string(my_scope->get_scopeMacro_name()));
      valuetype = V_CSTR;
      break;
    }
    case MACRO_TESTCASEID: {
      if (exp_val == Type::EXPECTED_CONSTANT ||
	  exp_val == Type::EXPECTED_STATIC_VALUE) {
	error("A %s value was expected instead of macro `%%testcaseId', "
	  "which is evaluated at runtime",
	  exp_val == Type::EXPECTED_CONSTANT ? "constant" : "static");
	goto error;
      }
      if (!my_scope)
	FATAL_ERROR("Value::evaluate_macro(): my_scope is not set");
      Ttcn::StatementBlock *my_sb =
	dynamic_cast<Ttcn::StatementBlock*>(my_scope);
      if (!my_sb) {
	error("Usage of macro %%testcaseId is allowed only within the "
	  "statement blocks of functions, altsteps and testcases");
	goto error;
      }
      Ttcn::Definition *my_def = my_sb->get_my_def();
      if (!my_def) {
	error("Macro %%testcaseId cannot be used in the control part. "
	  "It is allowed only within the statement blocks of functions, "
	  "altsteps and testcases");
	goto error;
      }
      if (my_def->get_asstype() == Assignment::A_TESTCASE) {
	// folding is possible only within testcases
	set_val_str(new string(my_def->get_id().get_dispname()));
	valuetype = V_CSTR;
      }
      break; }
    default:
      FATAL_ERROR("Value::evaluate_macro()");
    }
    return;
  error:
    set_valuetype(V_ERROR);
  }

  void Value::add_id(Identifier *p_id)
  {
    switch(valuetype) {
    case V_NAMEDBITS:
      if(u.ids->has_key(p_id->get_name())) {
        error("Duplicate named bit `%s'", p_id->get_dispname().c_str());
        // The Value does not take ownership for the identifier,
        // so it must be deleted (add_is acts as a sink).
        delete p_id;
      }
      else u.ids->add(p_id->get_name(), p_id);
      break;
    default:
      FATAL_ERROR("Value::add_id()");
    } // switch
  }

  Value* Value::get_value_refd_last(ReferenceChain *refch,
                                    Type::expected_value_t exp_val)
  {
    set_lowerid_to_ref();
    switch (valuetype) {
    case V_INVOKE:
      // there might be a better place for this
      chk_invoke(exp_val);
      return this;
    case V_REFD:
      // use the cache if available
      if (u.ref.refd_last) return u.ref.refd_last;
      else {
        Assignment *ass = u.ref.ref->get_refd_assignment();
        if (!ass) {
          // the referred definition is not found
          set_valuetype(V_ERROR);
        } else {
          switch (ass->get_asstype()) {
          case Assignment::A_OBJECT:
          case Assignment::A_OS: {
            // the referred definition is an ASN.1 object or object set
            Setting *setting = u.ref.ref->get_refd_setting();
            if (!setting || setting->get_st() == S_ERROR) {
              // remain silent, the error has been already reported
              set_valuetype(V_ERROR);
              break;
            } else if (setting->get_st() != S_V) {
              u.ref.ref->error("InformationFromObjects construct `%s' does not"
                " refer to a value", u.ref.ref->get_dispname().c_str());
              set_valuetype(V_ERROR);
              break;
            }
            bool destroy_refch;
            if (refch) {
              refch->mark_state();
              destroy_refch = false;
            } else {
              refch = new ReferenceChain(this,
                "While searching referenced value");
              destroy_refch = true;
            }
            if (refch->add(get_fullname())) {
              Value *v_refd = dynamic_cast<Value*>(setting);
              Value *v_last = v_refd->get_value_refd_last(refch);
              // in case of circular recursion the valuetype is already set
              // to V_ERROR, so don't set the cache
              if (valuetype == V_REFD) u.ref.refd_last = v_last;
            } else {
              // a circular recursion was detected
              set_valuetype(V_ERROR);
            }
            if (destroy_refch) delete refch;
            else refch->prev_state();
            break; }
          case Assignment::A_CONST: {
            // the referred definition is a constant
            bool destroy_refch;
            if (refch) {
              refch->mark_state();
              destroy_refch = false;
            } else {
              refch = new ReferenceChain(this,
                "While searching referenced value");
              destroy_refch = true;
            }
            if (refch->add(get_fullname())) {
              Ttcn::FieldOrArrayRefs *subrefs = u.ref.ref->get_subrefs();
              Value *v_refd = ass->get_Value()
              ->get_refd_sub_value(subrefs, 0,
                  u.ref.ref->getUsedInIsbound(), refch);
              if (v_refd) {
                Value *v_last = v_refd->get_value_refd_last(refch);
                // in case of circular recursion the valuetype is already set
                // to V_ERROR, so don't set the cache
                if (valuetype == V_REFD) u.ref.refd_last = v_last;
              } else if (subrefs && subrefs->has_unfoldable_index()) {
                u.ref.refd_last = this;
              } else if (u.ref.ref->getUsedInIsbound()) {
                u.ref.refd_last = this;
              } else {
                // the sub-reference points to a non-existent field
                set_valuetype(V_ERROR);
              }
            } else {
              // a circular recursion was detected
              set_valuetype(V_ERROR);
            }
            if (destroy_refch) delete refch;
            else refch->prev_state();
            break; }
          case Assignment::A_EXT_CONST:
          case Assignment::A_MODULEPAR:
          case Assignment::A_VAR:
          case Assignment::A_FUNCTION_RVAL:
          case Assignment::A_EXT_FUNCTION_RVAL:
          case Assignment::A_PAR_VAL_IN:
          case Assignment::A_PAR_VAL_OUT:
          case Assignment::A_PAR_VAL_INOUT:
            // the referred definition is not a constant
            u.ref.refd_last = this;
            break;
          case Assignment::A_FUNCTION:
          case Assignment::A_EXT_FUNCTION:
            u.ref.ref->error("Reference to a value was expected instead of a "
              "call of %s, which does not have return type",
              ass->get_description().c_str());
            set_valuetype(V_ERROR);
            break;
          case Assignment::A_FUNCTION_RTEMP:
          case Assignment::A_EXT_FUNCTION_RTEMP:
            u.ref.ref->error("Reference to a value was expected instead of a "
              "call of %s, which returns a template",
              ass->get_description().c_str());
            set_valuetype(V_ERROR);
            break;
          default:
            u.ref.ref->error("Reference to a value was expected instead of %s",
              ass->get_description().c_str());
            set_valuetype(V_ERROR);
          } // switch asstype
        }
        if (valuetype == V_REFD) return u.ref.refd_last;
        else return this;
      }
    case V_EXPR: {
      // try to evaluate the expression
      bool destroy_refch;
      if(refch) {
        refch->mark_state();
        destroy_refch=false;
      }
      else {
        refch=new ReferenceChain(this, "While evaluating expression");
        destroy_refch=true;
      }
      if(refch->add(get_fullname())) evaluate_value(refch, exp_val);
      else set_valuetype(V_ERROR);
      if(destroy_refch) delete refch;
      else refch->prev_state();
      return this; }
    case V_MACRO:
      evaluate_macro(exp_val);
      // no break
    default:
      // return this for all other value types
      return this;
    } // switch
  }
  
  map<Value*, void> Value::UnfoldabilityCheck::running;

  /* Note that the logic here needs to be in sync with evaluate_value,
   * and possibly others, i.e. if evaluate_value is called for a Value
   * for which is_unfoldable returns false, FATAL_ERROR might happen. */
  bool Value::is_unfoldable(ReferenceChain *refch,
                            Type::expected_value_t exp_val)
  {
    if (UnfoldabilityCheck::is_running(this)) {
      // This function is already running on this value => infinite recursion
      return true;
    }
    
    UnfoldabilityCheck checker(this);
    
    if (get_needs_conversion()) return true;
    switch (valuetype) {
    case V_NAMEDINT:
    case V_NAMEDBITS:
    case V_OPENTYPE:
    case V_UNDEF_LOWERID:
    case V_UNDEF_BLOCK:
    case V_TTCN3_NULL:
    case V_REFER:
      // these value types are eliminated during semantic analysis
      FATAL_ERROR("Value::is_unfoldable()");
    case V_ERROR:
    case V_INVOKE:
      return true;
    case V_CHOICE:
      return u.choice.alt_value->is_unfoldable(refch, exp_val);
    case V_SEQOF:
    case V_SETOF:
    case V_ARRAY:
      if (!is_indexed()) {
        for (size_t i = 0; i < u.val_vs->get_nof_vs(); i++) {
          if (u.val_vs->get_v_byIndex(i)->is_unfoldable(refch, exp_val))
            return true;
        }
      } else {
    	  for(size_t i = 0; i < u.val_vs->get_nof_ivs(); ++i) {
    		  if (u.val_vs->get_iv_byIndex(i)->is_unfoldable(refch, exp_val))
    			  return true;
    	  }
      }
      return false;
    case V_SEQ:
    case V_SET:
    	for (size_t i = 0; i < u.val_nvs->get_nof_nvs(); i++) {
    		if (u.val_nvs->get_nv_byIndex(i)->get_value()
    				->is_unfoldable(refch, exp_val)) return true;
    	}
      return false;
    case V_OID:
    case V_ROID:
      chk();
      for (size_t i = 0; i < u.oid_comps->size(); ++i) {
        if ((*u.oid_comps)[i]->is_variable()) return true;
      }
      return false;
    case V_REFD: {
      Value *v_last=get_value_refd_last(refch, exp_val);
      if(v_last==this) return true; // there weren't any references to chase
      else return v_last->is_unfoldable(refch, exp_val);
    }
    case V_EXPR:
      // classify the unchecked ischosen() operation, if it was not done so far
      if (u.expr.v_optype==OPTYPE_ISCHOSEN) chk_expr_ref_ischosen();
      if(u.expr.state==EXPR_CHECKING_ERR) return true;
      switch (u.expr.v_optype) {
      case OPTYPE_RND: // -
      case OPTYPE_COMP_MTC:
      case OPTYPE_COMP_SYSTEM:
      case OPTYPE_COMP_SELF:
      case OPTYPE_COMP_RUNNING_ANY:
      case OPTYPE_COMP_RUNNING_ALL:
      case OPTYPE_COMP_ALIVE_ANY:
      case OPTYPE_COMP_ALIVE_ALL:
      case OPTYPE_TMR_RUNNING_ANY:
      case OPTYPE_GETVERDICT:
      case OPTYPE_TESTCASENAME:
      case OPTYPE_PROF_RUNNING:
      case OPTYPE_RNDWITHVAL: // v1
      case OPTYPE_MATCH: // v1 t2
      case OPTYPE_UNDEF_RUNNING: // v1
      case OPTYPE_COMP_RUNNING:
      case OPTYPE_COMP_ALIVE:
      case OPTYPE_TMR_READ:
      case OPTYPE_TMR_RUNNING:
      case OPTYPE_ACTIVATE:
      case OPTYPE_ACTIVATE_REFD:
      case OPTYPE_EXECUTE: // r1 [v2]
      case OPTYPE_EXECUTE_REFD:
      case OPTYPE_COMP_CREATE: // r1 [v2] [v3] b4
      case OPTYPE_ISCHOSEN:
      case OPTYPE_ISCHOSEN_T:
      case OPTYPE_SIZEOF: // ti1
      case OPTYPE_DECODE:
      case OPTYPE_ENCODE:
      case OPTYPE_OCT2UNICHAR:
      case OPTYPE_UNICHAR2OCT:
      case OPTYPE_ENCODE_BASE64:
      case OPTYPE_DECODE_BASE64:
      case OPTYPE_ENCVALUE_UNICHAR:
      case OPTYPE_DECVALUE_UNICHAR:
      case OPTYPE_CHECKSTATE_ANY:
      case OPTYPE_CHECKSTATE_ALL:
      case OPTYPE_HOSTID:
      case OPTYPE_ISTEMPLATEKIND: // ti1 v2
        return true;
      case OPTYPE_COMP_NULL: // -
        return false;
      case OPTYPE_UNARYPLUS: // v1
      case OPTYPE_UNARYMINUS:
      case OPTYPE_NOT:
      case OPTYPE_NOT4B:
      case OPTYPE_BIT2HEX:
      case OPTYPE_BIT2INT:
      case OPTYPE_BIT2OCT:
      case OPTYPE_BIT2STR:
      case OPTYPE_CHAR2INT:
      case OPTYPE_CHAR2OCT:
      case OPTYPE_FLOAT2INT:
      case OPTYPE_FLOAT2STR:
      case OPTYPE_HEX2BIT:
      case OPTYPE_HEX2INT:
      case OPTYPE_HEX2OCT:
      case OPTYPE_HEX2STR:
      case OPTYPE_INT2CHAR:
      case OPTYPE_INT2FLOAT:
      case OPTYPE_INT2STR:
      case OPTYPE_INT2UNICHAR:
      case OPTYPE_OCT2BIT:
      case OPTYPE_OCT2CHAR:
      case OPTYPE_OCT2HEX:
      case OPTYPE_OCT2INT:
      case OPTYPE_OCT2STR:
      case OPTYPE_STR2BIT:
      case OPTYPE_STR2FLOAT:
      case OPTYPE_STR2HEX:
      case OPTYPE_STR2INT:
      case OPTYPE_STR2OCT:
      case OPTYPE_UNICHAR2INT:
      case OPTYPE_UNICHAR2CHAR:
      case OPTYPE_ENUM2INT:
      case OPTYPE_GET_STRINGENCODING:
      case OPTYPE_REMOVE_BOM:
        return u.expr.v1->is_unfoldable(refch, exp_val);
      case OPTYPE_ISBOUND: /*{
        //TODO once we have the time for it make isbound foldable.
        if (u.expr.ti1->get_DerivedRef() != 0) return true;
        Template* temp = u.expr.ti1->get_Template();
        if (temp->get_templatetype() == Template::SPECIFIC_VALUE) {
          Value* specificValue = temp->get_specific_value();
          if (specificValue->get_valuetype() == Value::V_REFD) {
            //FIXME implement
          }

          return specificValue->is_unfoldable(refch, exp_val);
        } else if (temp->get_templatetype() == Template::TEMPLATE_REFD) {
          //FIXME implement
        }
        }*/
        return true;
      case OPTYPE_ISPRESENT:
        // TODO: "if you have motivation"
        return true;
      case OPTYPE_ISVALUE:  // ti1
	// fallthrough
      case OPTYPE_LENGTHOF: // ti1
        return u.expr.ti1->get_DerivedRef() != 0
	  || u.expr.ti1->get_Template()->get_templatetype()
	     != Template::SPECIFIC_VALUE
	  || u.expr.ti1->get_Template()->get_specific_value()
	     ->is_unfoldable(refch, exp_val);
      case OPTYPE_ROTL:
      case OPTYPE_ROTR:
      case OPTYPE_CONCAT:
        if (!u.expr.v1->is_string_type(exp_val)) return true;
        // no break
      case OPTYPE_ADD: // v1 v2
      case OPTYPE_SUBTRACT:
      case OPTYPE_MULTIPLY:
      case OPTYPE_DIVIDE:
      case OPTYPE_MOD:
      case OPTYPE_REM:
      case OPTYPE_EQ:
      case OPTYPE_LT:
      case OPTYPE_GT:
      case OPTYPE_NE:
      case OPTYPE_GE:
      case OPTYPE_LE:
      case OPTYPE_XOR:
      case OPTYPE_AND4B:
      case OPTYPE_OR4B:
      case OPTYPE_XOR4B:
      case OPTYPE_SHL:
      case OPTYPE_SHR:
      case OPTYPE_INT2BIT:
      case OPTYPE_INT2HEX:
      case OPTYPE_INT2OCT:
        return u.expr.v1->is_unfoldable(refch, exp_val)
          || u.expr.v2->is_unfoldable(refch, exp_val);
      case OPTYPE_AND: // short-circuit evaluation
        return u.expr.v1->is_unfoldable(refch, exp_val)
          || (u.expr.v1->get_val_bool() &&
              u.expr.v2->is_unfoldable(refch, exp_val));
      case OPTYPE_OR: // short-circuit evaluation
        return u.expr.v1->is_unfoldable(refch, exp_val)
          || (!u.expr.v1->get_val_bool() &&
               u.expr.v2->is_unfoldable(refch, exp_val));
      case OPTYPE_SUBSTR:
        if (!u.expr.ti1->get_specific_value()) return true;
        if (!u.expr.ti1->is_string_type(exp_val)) return true;
        return u.expr.ti1->get_specific_value()->is_unfoldable(refch, exp_val)
          || u.expr.v2->is_unfoldable(refch, exp_val)
          || u.expr.v3->is_unfoldable(refch, exp_val);
      case OPTYPE_REGEXP:
        if (!u.expr.ti1->get_specific_value() ||
            !u.expr.t2->get_specific_value()) return true;
        return u.expr.ti1->get_specific_value()->is_unfoldable(refch, exp_val)
          || u.expr.t2->get_specific_value()->is_unfoldable(refch, exp_val)
          || u.expr.v3->is_unfoldable(refch, exp_val);
      case OPTYPE_DECOMP:
        return u.expr.v1->is_unfoldable(refch, exp_val)
          || u.expr.v2->is_unfoldable(refch, exp_val)
          || u.expr.v3->is_unfoldable(refch, exp_val);
      case OPTYPE_REPLACE: {
        if (!u.expr.ti1->get_specific_value() ||
            !u.expr.ti4->get_specific_value()) return true;
        if (!u.expr.ti1->is_string_type(exp_val)) return true;
        return u.expr.ti1->get_specific_value()->is_unfoldable(refch, exp_val)
          || u.expr.v2->is_unfoldable(refch, exp_val)
          || u.expr.v3->is_unfoldable(refch, exp_val)
          || u.expr.ti4->get_specific_value()->is_unfoldable(refch, exp_val);
      }
      case OPTYPE_VALUEOF: // ti1
        /* \todo if you have motivation to implement the eval function
           for valueof()... */
        return true;
      case OPTYPE_ISCHOSEN_V:
        return u.expr.v1->is_unfoldable(refch, exp_val);
      case OPTYPE_LOG2STR:
      case OPTYPE_ANY2UNISTR:
      case OPTYPE_TTCN2STRING:
        return true;
      default:
        FATAL_ERROR("Value::is_unfoldable()");
      } // switch
      FATAL_ERROR("Value::is_unfoldable()");
      break; // should never get here
    case V_MACRO:
      switch (u.macro) {
      case MACRO_TESTCASEID:
	// this is known only at runtime
	return true;
      default:
	return false;
      }
    default:
      // all literal values are foldable
      return false;
    }
  }

  Value* Value::get_refd_sub_value(Ttcn::FieldOrArrayRefs *subrefs,
                                   size_t start_i, bool usedInIsbound,
                                   ReferenceChain *refch,
                                   bool silent)
  {
    if (!subrefs) return this;
    Value *v = this;
    for (size_t i = start_i; i < subrefs->get_nof_refs(); i++) {
      if (!v) break;
      v = v->get_value_refd_last(refch);
      switch(v->valuetype) {
      case V_ERROR:
        return v;
      case V_REFD:
        // unfoldable stuff
        return this;
      default:
        break;
      } // switch
      Ttcn::FieldOrArrayRef *ref = subrefs->get_ref(i);
      if (ref->get_type() == Ttcn::FieldOrArrayRef::FIELD_REF) {
        if (v->get_my_governor()->get_type_refd_last()->get_typetype() == 
            Type::T_OPENTYPE) {
          // allow the alternatives of open types as both lower and upper identifiers
          ref->set_field_name_to_lowercase();
        }
        v = v->get_refd_field_value(*ref->get_id(), usedInIsbound, *ref, silent);
      }
      else v = v->get_refd_array_value(ref->get_val(), usedInIsbound, refch, silent);
    }
    return v;
  }

  Value *Value::get_refd_field_value(const Identifier& field_id,
                                     bool usedInIsbound, const Location& loc,
                                     bool silent)
  {
    if (valuetype == V_OMIT) {
      if (!silent) {
        loc.error("Reference to field `%s' of omit value `%s'",
          field_id.get_dispname().c_str(), get_fullname().c_str());
      }
      return 0;
    }
    if (!my_governor) FATAL_ERROR("Value::get_refd_field_value()");
    Type *t = my_governor->get_type_refd_last();
    switch (t->get_typetype()) {
    case Type::T_ERROR:
      // remain silent
      return 0;
    case Type::T_CHOICE_A:
    case Type::T_CHOICE_T:
    case Type::T_OPENTYPE:
    case Type::T_ANYTYPE:
      if (!t->has_comp_withName(field_id)) {
        if (!silent) {
          loc.error("Reference to non-existent union field `%s' in type `%s'",
            field_id.get_dispname().c_str(), t->get_typename().c_str());
        }
        return 0;
      } else if (valuetype != V_CHOICE) {
        // remain silent, the error is already reported
        return 0;
      } else if (*u.choice.alt_name == field_id) {
        // everything is OK
        return u.choice.alt_value;
      }else {
        if (!usedInIsbound && !silent) {
          loc.error("Reference to inactive field `%s' in a value of union type "
	  "`%s'. The active field is `%s'",
	  field_id.get_dispname().c_str(), t->get_typename().c_str(),
	  u.choice.alt_name->get_dispname().c_str());
        }
	return 0;
      }
    case Type::T_SEQ_A:
    case Type::T_SEQ_T:
      if (!t->has_comp_withName(field_id)) {
        if (!silent) {
          loc.error("Reference to non-existent record field `%s' in type `%s'",
            field_id.get_dispname().c_str(), t->get_typename().c_str());
        }
        return 0;
      } else if (valuetype != V_SEQ) {
        // remain silent, the error has been already reported
        return 0;
      } else break;
    case Type::T_SET_A:
    case Type::T_SET_T:
      if (!t->has_comp_withName(field_id)) {
        if (!silent) {
          loc.error("Reference to non-existent set field `%s' in type `%s'",
            field_id.get_dispname().c_str(), t->get_typename().c_str());
        }
        return 0;
      } else if (valuetype != V_SET) {
        // remain silent, the error has been already reported
        return 0;
      } else break;
    default:
      if (!silent) {
        loc.error("Invalid field reference `%s': type `%s' "
          "does not have fields", field_id.get_dispname().c_str(),
          t->get_typename().c_str());
      }
      return 0;
    }
    // the common end for record & set types
    if (u.val_nvs->has_nv_withName(field_id)) {
      // everything is OK
      return u.val_nvs->get_nv_byName(field_id)->get_value();
    } else if (!is_asn1()) {
      if (!usedInIsbound && !silent) {
        loc.error("Reference to unbound field `%s'",
	  field_id.get_dispname().c_str());
        // this is an error in TTCN-3, which has been already reported
      }
      return 0;
    } else {
      CompField *cf = t->get_comp_byName(field_id);
      if (cf->get_is_optional()) {
	// creating an explicit omit value
	Value *v = new Value(V_OMIT);
	v->set_fullname(get_fullname() + "." + field_id.get_dispname());
	v->set_my_scope(get_my_scope());
	u.val_nvs->add_nv(new NamedValue(field_id.clone(), v));
	return v;
      } else if (cf->has_default()) {
	// returning the component's default value
	return cf->get_defval();
      } else {
	// this is an error in ASN.1, which has been already reported
	return 0;
      }
    }
  }

  Value *Value::get_refd_array_value(Value *array_index, bool usedInIsbound,
                                     ReferenceChain *refch, bool silent)
  {
    Value *v_index = array_index->get_value_refd_last(refch);
    if (!my_governor) FATAL_ERROR("Value::get_refd_field_value()");
    Type *t = my_governor->get_type_refd_last();
    Int index = 0;
    bool index_available = false;
    if (!v_index->is_unfoldable()) {
      if (v_index->valuetype == V_INT) {
        index = v_index->get_val_Int()->get_val();
        index_available = true;
      } else if (v_index->valuetype == V_ARRAY || v_index->valuetype == V_SEQOF) {
        Value *v = this;
        size_t comps = v_index->get_nof_comps();
        for (size_t i = 0; i < comps; i++) {
          Value *comp = v_index->get_comp_byIndex(i);
          v = v->get_refd_array_value(comp, usedInIsbound, refch, silent);
          if (v == NULL) {
            // error reported already
            return 0;
          }
        }
        return v;
      } else if (!silent) {
        array_index->error("An integer value or a fixed length array or record of integer value was expected as index");
      }
    }
    if (valuetype == V_OMIT) {
      if (!silent) {
        array_index->error("Accessing an element with index of omit value `%s'",
          get_fullname().c_str());
      }
      return 0;
    }

    switch (t->get_typetype()) {
    case Type::T_ERROR:
      // remain silent
      return 0;
    case Type::T_SEQOF:
      if (index_available) {
        if (index < 0) {
          if (!silent) {
            array_index->error("A non-negative integer value was expected "
                               "instead of %s for indexing a value of `record "
                               "of' type `%s'", Int2string(index).c_str(),
                               t->get_typename().c_str());
          }
          return 0;
        }
        switch (valuetype) {
        case V_SEQOF:
          if (!is_indexed()) {
            if (index >= static_cast<Int>(u.val_vs->get_nof_vs())) {
              if (!usedInIsbound && !silent) {
                array_index->error("Index overflow in a value of `record of' "
                                 "type `%s': the index is %s, but the value "
                                 "has only %lu elements",
                                 t->get_typename().c_str(),
                                 Int2string(index).c_str(),
                                 (unsigned long)u.val_vs->get_nof_vs());
              }
              return 0;
            } else {
              Value* temp = u.val_vs->get_v_byIndex(index);
              if(!silent && temp->get_value_refd_last()->get_valuetype() == V_NOTUSED)
                temp->error("Not used symbol is not allowed in this context");
              return u.val_vs->get_v_byIndex(index);
            }
          } else {
            // Search the appropriate constant index.
            for (size_t i = 0; i < u.val_vs->get_nof_ivs(); i++) {
              Value *iv_index = u.val_vs->get_iv_byIndex(i)->get_index()
                ->get_value_refd_last();
              if (iv_index->get_valuetype() != V_INT) continue;
              if (iv_index->get_val_Int()->get_val() == index)
                return u.val_vs->get_iv_byIndex(i)->get_value();
            }
            return 0;
          }
          break;
        default:
          // remain silent, the error has been already reported
          return 0;
        }
      } else {
        // the error has been reported above
        return 0;
      }
    case Type::T_SETOF:
      if (index_available) {
        if (index < 0) {
          if (!silent) {
            array_index->error("A non-negative integer value was expected "
              "instead of %s for indexing a value of `set of' type `%s'",
              Int2string(index).c_str(), t->get_typename().c_str());
          }
          return 0;
        }
        switch (valuetype) {
        case V_SETOF:
          if (!is_indexed()) {
            if (index >= static_cast<Int>(u.val_vs->get_nof_vs())) {
              if (!usedInIsbound && !silent) {
                array_index->error("Index overflow in a value of `set of' type "
                                 "`%s': the index is %s, but the value has "
                                 "only %lu elements",
                                 t->get_typename().c_str(),
                                 Int2string(index).c_str(),
                                 (unsigned long)u.val_vs->get_nof_vs());
              }
              return 0;
            } else {
              Value* temp = u.val_vs->get_v_byIndex(index);
              if(!silent && temp->get_value_refd_last()->get_valuetype() == V_NOTUSED)
                temp->error("Not used symbol is not allowed in this context");
              return temp;
            }
          } else {
            for (size_t i = 0; i < u.val_vs->get_nof_ivs(); i++) {
              Value *iv_index = u.val_vs->get_iv_byIndex(i)->get_index()
                ->get_value_refd_last();
              if (iv_index->get_valuetype() != V_INT) continue;
              if (iv_index->get_val_Int()->get_val() == index)
                return u.val_vs->get_iv_byIndex(i)->get_value();
            }
            return 0;
          }
          break;
        default:
          // remain silent, the error has been already reported
          return 0;
        }
      } else {
        // the error has been reported above
        return 0;
      }
    case Type::T_ARRAY:
      if (index_available) {
        Ttcn::ArrayDimension *dim = t->get_dimension();
        dim->chk_index(v_index, Type::EXPECTED_CONSTANT);
        if (valuetype == V_ARRAY && !dim->get_has_error()) {
          // perform the index transformation
          index -= dim->get_offset();
          if (!is_indexed()) {
            // check for index underflow/overflow or too few elements in the
            // value
            if (index < 0 ||
                index >= static_cast<Int>(u.val_vs->get_nof_vs()))
              return 0;
            else return u.val_vs->get_v_byIndex(index);
          } else {
            if (index < 0) return 0;
            for (size_t i = 0; i < u.val_vs->get_nof_ivs(); i++) {
              Value *iv_index = u.val_vs->get_iv_byIndex(i)->get_index()
                ->get_value_refd_last();
              if (iv_index->get_valuetype() != V_INT) continue;
              if (iv_index->get_val_Int()->get_val() == index)
                return u.val_vs->get_iv_byIndex(index)->get_value();
            }
            return 0;
          }
        } else {
          // remain silent, the error has been already reported
          return 0;
        }
      } else {
        // the error has been reported above
        return 0;
      }
    case Type::T_BSTR:
    case Type::T_BSTR_A:
    case Type::T_HSTR:
    case Type::T_OSTR:
    case Type::T_CSTR:
    case Type::T_USTR:
    case Type::T_UTF8STRING:
    case Type::T_NUMERICSTRING:
    case Type::T_PRINTABLESTRING:
    case Type::T_TELETEXSTRING:
    case Type::T_VIDEOTEXSTRING:
    case Type::T_IA5STRING:
    case Type::T_GRAPHICSTRING:
    case Type::T_VISIBLESTRING:
    case Type::T_GENERALSTRING:
    case Type::T_UNIVERSALSTRING:
    case Type::T_BMPSTRING:
    case Type::T_UTCTIME:
    case Type::T_GENERALIZEDTIME:
    case Type::T_OBJECTDESCRIPTOR:
      if (index_available) return get_string_element(index, *array_index);
      else return 0;
    default:
      if (!silent) {
        array_index->error("Invalid array element reference: type `%s' cannot "
          "be indexed", t->get_typename().c_str());
      }
      return 0;
    }
  }

  Value *Value::get_string_element(const Int& index, const Location& loc)
  {
    if (index < 0) {
      loc.error("A non-negative integer value was expected instead of %s "
	"for indexing a string element", Int2string(index).c_str());
      return 0;
    }
    size_t string_length;
    switch (valuetype) {
    case V_BSTR:
    case V_HSTR:
    case V_CSTR:
    case V_ISO2022STR:
      string_length = u.str.val_str->size();
      break;
    case V_OSTR:
      string_length = u.str.val_str->size() / 2;
      break;
    case V_USTR:
      string_length = u.ustr.val_ustr->size();
      break;
    default:
      // remain silent, the error has been already reported
      return 0;
    }
    if (index >= static_cast<Int>(string_length)) {
      loc.error("Index overflow when accessing a string element: "
	"the index is %s, but the string has only %lu elements",
	Int2string(index).c_str(), (unsigned long) string_length);
      return 0;
    }
    switch (valuetype) {
    case V_BSTR:
    case V_HSTR:
    case V_CSTR:
    case V_ISO2022STR:
      if (u.str.str_elements && u.str.str_elements->has_key(index))
        return (*u.str.str_elements)[index];
      else {
	Value *t_val = new Value(valuetype,
	  new string(u.str.val_str->substr(index, 1)));
	add_string_element(index, t_val, u.str.str_elements);
        return t_val;
      }
    case V_OSTR:
      if (u.str.str_elements && u.str.str_elements->has_key(index))
        return (*u.str.str_elements)[index];
      else {
	Value *t_val = new Value(V_OSTR,
          new string(u.str.val_str->substr(2 * index, 2)));
	add_string_element(index, t_val, u.str.str_elements);
        return t_val;
      }
    case V_USTR:
      if (u.ustr.ustr_elements && u.ustr.ustr_elements->has_key(index))
        return (*u.ustr.ustr_elements)[index];
      else {
	Value *t_val = new Value(V_USTR,
          new ustring(u.ustr.val_ustr->substr(index, 1)));
	add_string_element(index, t_val, u.ustr.ustr_elements);
        return t_val;
      }
    default:
      FATAL_ERROR("Value::get_string_element()");
      return 0;
    }
  }

  void Value::chk_expr_type(Type::typetype_t p_tt, const char *type_name,
    Type::expected_value_t exp_val)
  {
    set_lowerid_to_ref();
    Type::typetype_t r_tt = get_expr_returntype(exp_val);
    bool error_flag = r_tt != Type::T_ERROR && r_tt != p_tt;
    if (error_flag)
      error("A value or expression of type %s was expected", type_name);
    if (valuetype == V_REFD) {
      Type *t_chk = Type::get_pooltype(Type::T_ERROR);
      t_chk->chk_this_refd_value(this, 0, exp_val);
    }
    get_value_refd_last(0, exp_val);
    if (error_flag) set_valuetype(V_ERROR);
    else if (!my_governor) set_my_governor(Type::get_pooltype(p_tt));
  }
  
  bool Value::chk_string_encoding(Common::Assignment* lhs)
  {
    Error_Context cntxt(this, "In encoding format");
    set_lowerid_to_ref();
    bool self_ref = Type::get_pooltype(Type::T_CSTR)->chk_this_value(this, lhs,
      Type::EXPECTED_DYNAMIC_VALUE, INCOMPLETE_NOT_ALLOWED, OMIT_NOT_ALLOWED,
      NO_SUB_CHK);
    if (!is_unfoldable()) {
      string enc_name = get_val_str();
      if (enc_name != "UTF-8" && enc_name != "UTF-16" && enc_name != "UTF-32"
          && enc_name != "UTF-16LE" && enc_name != "UTF-16BE"
          && enc_name != "UTF-32LE" && enc_name != "UTF-32BE") {
        error("'%s' is not a valid encoding format", enc_name.c_str());
      }
    }
    return self_ref;
  }

  int Value::is_parsed_infinity()
  {
    if ( (get_valuetype()==V_REAL) && (get_val_Real()==REAL_INFINITY) )
      return 1;
    if ( (get_valuetype()==V_EXPR) && (get_optype()==OPTYPE_UNARYMINUS) &&
         (u.expr.v1->get_valuetype()==V_REAL) &&
         (u.expr.v1->get_val_Real()==REAL_INFINITY) )
      return -1;
    return 0;
  }

  bool Value::get_val_bool()
  {
    Value *v;
    if (valuetype == V_REFD) v = get_value_refd_last();
    else v = this;
    if (v->valuetype != V_BOOL) FATAL_ERROR("Value::get_val_bool()");
    return v->u.val_bool;
  }

  int_val_t* Value::get_val_Int()
  {
    Value *v;
    if (valuetype == V_REFD) v = get_value_refd_last();
    else v = this;
    switch (v->valuetype) {
    case V_INT:
      break;
    case V_UNDEF_LOWERID:
      FATAL_ERROR("Cannot use this value (here) as an integer: " \
                  "`%s'", (*u.val_id).get_dispname().c_str());
    default:
      FATAL_ERROR("Value::get_val_Int()");
    } // switch
    return v->u.val_Int;
  }

  const Identifier* Value::get_val_id()
  {
    switch(valuetype) {
    case V_NAMEDINT:
    case V_ENUM:
    case V_UNDEF_LOWERID:
      return u.val_id;
    default:
      FATAL_ERROR("Value::get_val_id()");
      return 0;
    } // switch
  }

  const ttcn3float& Value::get_val_Real()
  {
    Value *v;
    if (valuetype == V_REFD) v = get_value_refd_last();
    else v = this;
    if (v->valuetype != V_REAL) FATAL_ERROR("Value::get_val_Real()");
    return v->u.val_Real;
  }

  string Value::get_val_str()
  {
    Value *v = get_value_refd_last();
    switch (v->valuetype) {
    case V_BSTR:
    case V_HSTR:
    case V_OSTR:
    case V_CSTR:
      return *v->u.str.val_str;
    case V_CHARSYMS:
      return v->u.char_syms->get_string();
    case V_USTR:
      error("Cannot use ISO-10646 string value in string context");
      return string();
    case V_ISO2022STR:
      error("Cannot use ISO-2022 string value in string context");
      // no break
    case V_ERROR:
      return string();
    default:
      error("Cannot use this value in charstring value context");
      return string();
    } // switch
  }

  ustring Value::get_val_ustr()
  {
    Value *v = get_value_refd_last();
    switch (v->valuetype) {
    case V_CSTR:
      return ustring(*v->u.str.val_str);
    case V_USTR:
      return *v->u.ustr.val_ustr;
    case V_CHARSYMS:
      return v->u.char_syms->get_ustring();
    case V_ISO2022STR:
      error("Cannot use ISO-2022 string value in ISO-10646 string context");
      // no break
    case V_ERROR:
      return ustring();
    default:
      error("Cannot use this value in ISO-10646 string context");
      return ustring();
    } // switch
  }

  string Value::get_val_iso2022str()
  {
    Value *v = get_value_refd_last();
    switch (v->valuetype) {
    case V_CSTR:
    case V_ISO2022STR:
      return *v->u.str.val_str;
    case V_CHARSYMS:
      return v->u.char_syms->get_iso2022string();
    case V_USTR:
      error("Cannot use ISO-10646 string value in ISO-2022 string context");
      // no break
    case V_ERROR:
      return string();
    default:
      error("Cannot use this value in ISO-2022 string context");
      return string();
    } // switch
  }

  size_t Value::get_val_strlen()
  {
    Value *v = get_value_refd_last();
    switch (v->valuetype) {
    case V_BSTR:
    case V_HSTR:
    case V_CSTR:
    case V_ISO2022STR:
      return v->u.str.val_str->size();
    case V_OSTR:
      return v->u.str.val_str->size()/2;
    case V_CHARSYMS:
      return v->u.char_syms->get_len();
    case V_USTR:
      return v->u.ustr.val_ustr->size();
    case V_ERROR:
      return 0;
    default:
      error("Cannot use this value in string value context");
      return 0;
    } // switch
  }

  Value::verdict_t Value::get_val_verdict()
  {
    switch(valuetype) {
    case V_VERDICT:
      return u.verdict;
    default:
      FATAL_ERROR("Value::get_val_verdict()");
      return u.verdict;
    } // switch
  }

  size_t Value::get_nof_comps()
  {
    switch (valuetype) {
    case V_OID:
    case V_ROID:
      chk();
      return u.oid_comps->size();
    case V_SEQOF:
    case V_SETOF:
    case V_ARRAY:
      if (u.val_vs->is_indexed()) return u.val_vs->get_nof_ivs();
      else return u.val_vs->get_nof_vs();
    case V_SEQ:
    case V_SET:
      return u.val_nvs->get_nof_nvs();
    case V_BSTR:
    case V_HSTR:
    case V_CSTR:
    case V_ISO2022STR:
      return u.str.val_str->size();
    case V_OSTR:
      return u.str.val_str->size()/2;
    case V_USTR:
      return u.ustr.val_ustr->size();
    default:
      FATAL_ERROR("Value::get_nof_comps()");
      return 0;
    } // switch
  }

  bool Value::is_indexed() const
  {
    switch (valuetype) {
    case V_SEQOF:
    case V_SETOF:
    case V_ARRAY:
      // Applicable only for list-types.  Assigning a record/SEQUENCE or
      // set/SET with indexed notation is not supported.
      return u.val_vs->is_indexed();
    default:
      FATAL_ERROR("Value::is_indexed()");
      break;
    }
    return false;
  }

  const Identifier& Value::get_alt_name()
  {
    if (valuetype != V_CHOICE) FATAL_ERROR("Value::get_alt_name()");
    return *u.choice.alt_name;
  }

  Value *Value::get_alt_value()
  {
    if (valuetype != V_CHOICE) FATAL_ERROR("Value::get_alt_value()");
    return u.choice.alt_value;
  }
  
  void Value::set_alt_name_to_lowercase()
  {
    if (valuetype != V_CHOICE) FATAL_ERROR("Value::set_alt_name_to_lowercase()");
    string new_name = u.choice.alt_name->get_name();
    if (isupper(new_name[0])) {
      new_name[0] = (char)tolower(new_name[0]);
      if (new_name[new_name.size() - 1] == '_') {
        // an underscore is inserted at the end of the alternative name if it's
        // a basic type's name (since it would conflict with the class generated
        // for that type)
        // remove the underscore, it won't conflict with anything if its name
        // starts with a lowercase letter
        new_name.replace(new_name.size() - 1, 1, "");
      }
      delete u.choice.alt_name;
      u.choice.alt_name = new Identifier(Identifier::ID_NAME, new_name);
    }
  }

  bool Value::has_oid_error()
  {
    Value *v;
    if (valuetype == V_REFD) v = get_value_refd_last();
    else v = this;
    switch (valuetype) {
    case V_OID:
    case V_ROID:
      for (size_t i = 0; i < v->u.oid_comps->size(); i++)
	if ((*v->u.oid_comps)[i]->has_error()) return true;
      return false;
    default:
      return true;
    }
  }

  bool Value::get_oid_comps(vector<string>& comps)
  {
    bool ret_val = true;
    Value *v = this;
    switch (valuetype) {
    case V_REFD:
      v = get_value_refd_last();
      // no break
    case V_OID:
    case V_ROID:
      for (size_t i = 0; i < v->u.oid_comps->size(); i++) {
	(*v->u.oid_comps)[i]->get_comps(comps);
	if ((*v->u.oid_comps)[i]->is_variable()) {
	  // not all components can be calculated in compile-time
	  ret_val = false;
	}
      }
      break;
    default:
      FATAL_ERROR("Value::get_oid_comps()");
    }
    return ret_val;
  }

  void Value::add_se_comp(NamedValue* nv) {
    switch (valuetype) {
    case V_SEQ:
    case V_SET:
      if (!u.val_nvs)
        u.val_nvs = new NamedValues();
      u.val_nvs->add_nv(nv);
      break;
    default:
      FATAL_ERROR("Value::add_se_comp()");
    }
  }

  NamedValue* Value::get_se_comp_byIndex(size_t n)
  {
    switch(valuetype) {
    case V_SEQ:
    case V_SET:
      return u.val_nvs->get_nv_byIndex(n);
    default:
      FATAL_ERROR("Value::get_se_comp_byIndex()");
      return 0;
    } // switch
  }

  Value *Value::get_comp_byIndex(size_t n)
  {
    switch (valuetype) {
    case V_SEQOF:
    case V_SETOF:
    case V_ARRAY:
      if (!is_indexed()) return u.val_vs->get_v_byIndex(n);
      return u.val_vs->get_iv_byIndex(n)->get_value();
    default:
      FATAL_ERROR("Value::get_comp_byIndex()");
      return 0;
    } // switch
  }

  Value *Value::get_index_byIndex(size_t n)
  {
    switch (valuetype) {
    case V_SEQOF:
    case V_SETOF:
    case V_ARRAY:
      if (!is_indexed()) FATAL_ERROR("Value::get_index_byIndex()");
      return u.val_vs->get_iv_byIndex(n)->get_index();
    default:
      FATAL_ERROR("Value::get_index_byIndex()");
      return 0;
    } // switch
  }

  bool Value::has_comp_withName(const Identifier& p_name)
  {
    switch(valuetype) {
    case V_SEQ:
    case V_SET:
      return u.val_nvs->has_nv_withName(p_name);
    case V_CHOICE:
      return u.choice.alt_name->get_dispname() == p_name.get_dispname();
    default:
      FATAL_ERROR("Value::get_has_comp_withName()");
      return false;
    } // switch
  }

  bool Value::field_is_chosen(const Identifier& p_name)
  {
    Value *v=get_value_refd_last();
    if(v->valuetype!=V_CHOICE) FATAL_ERROR("Value::field_is_chosen()");
    return *v->u.choice.alt_name==p_name;
  }

  bool Value::field_is_present(const Identifier& p_name)
  {
    Value *v=get_value_refd_last();
    if(!(v->valuetype==V_SEQ || v->valuetype==V_SET))
      FATAL_ERROR("Value::field_is_present()");
    return v->u.val_nvs->has_nv_withName(p_name)
      && v->u.val_nvs->get_nv_byName(p_name)->get_value()
      ->get_value_refd_last()->valuetype != V_OMIT;
  }

  NamedValue* Value::get_se_comp_byName(const Identifier& p_name)
  {
    switch(valuetype) {
    case V_SEQ:
    case V_SET:
      return u.val_nvs->get_nv_byName(p_name);
    default:
      FATAL_ERROR("Value::get_se_comp_byName()");
      return 0;
    } // switch
  }

  Value* Value::get_comp_value_byName(const Identifier& p_name)
  {
    switch(valuetype) {
    case V_SEQ:
    case V_SET:
      return u.val_nvs->get_nv_byName(p_name)->get_value();
    case V_CHOICE:
      if(u.choice.alt_name->get_dispname() == p_name.get_dispname())
         return u.choice.alt_value;
      else
         return NULL;
    default:
      FATAL_ERROR("Value::get_se_comp_byName()");
      return 0;
    } // switch
  }

  void Value::chk_dupl_id()
  {
    switch(valuetype) {
    case V_SEQ:
    case V_SET:
      u.val_nvs->chk_dupl_id();
      break;
    default:
      FATAL_ERROR("Value::chk_dupl_id()");
    } // switch
  }

  size_t Value::get_nof_ids() const
  {
    switch(valuetype) {
    case V_NAMEDBITS:
      return u.ids->size();
      break;
    default:
      FATAL_ERROR("Value::get_nof_ids()");
      return 0;
    } // switch
  }

  Identifier* Value::get_id_byIndex(size_t p_i)
  {
    switch(valuetype) {
    case V_NAMEDBITS:
      return u.ids->get_nth_elem(p_i);
      break;
    default:
      FATAL_ERROR("Value::get_id_byIndex()");
      return 0;
    } // switch
  }

  bool Value::has_id(const Identifier& p_id)
  {
    switch(valuetype) {
    case V_NAMEDBITS:
      return u.ids->has_key(p_id.get_name());
      break;
    default:
      FATAL_ERROR("Value::has_id()");
      return false;
    } // switch
  }

  Reference *Value::get_reference() const
  {
    if (valuetype != V_REFD) FATAL_ERROR("Value::get_reference()");
    return u.ref.ref;
  }

  Reference *Value::get_refered() const
  {
    if (valuetype != V_REFER) FATAL_ERROR("Value::get_referred()");
    return u.refered;
  }

  Common::Assignment *Value::get_refd_fat() const
  {
    switch(valuetype){
    case V_FUNCTION:
    case V_ALTSTEP:
    case V_TESTCASE:
      return u.refd_fat;
    default:
      FATAL_ERROR("Value::get_refd_fat()");
    }
  }

  Ttcn::Reference* Value::steal_ttcn_ref()
  {
    Ttcn::Reference *ret_val =
      dynamic_cast<Ttcn::Reference*>(steal_ttcn_ref_base());
    if(!ret_val) FATAL_ERROR("Value::steal_ttcn_ref()");
    return ret_val;
  }

  Ttcn::Ref_base* Value::steal_ttcn_ref_base()
  {
    Ttcn::Ref_base *t_ref;
    if(valuetype==V_REFD) {
      t_ref=dynamic_cast<Ttcn::Ref_base*>(u.ref.ref);
      if(!t_ref) FATAL_ERROR("Value::steal_ttcn_ref_base()");
      u.ref.ref=0;
    }
    else if(valuetype==V_UNDEF_LOWERID) {
      t_ref=new Ttcn::Reference(u.val_id);
      t_ref->set_location(*this);
      t_ref->set_fullname(get_fullname());
      t_ref->set_my_scope(get_my_scope());
      u.val_id=0;
    }
    else {
      FATAL_ERROR("Value::steal_ttcn_ref_base()");
      t_ref = 0;
    }
    set_valuetype(V_ERROR);
    return t_ref;
  }

  void Value::steal_invoke_data(Value*& p_v, Ttcn::ParsedActualParameters*& p_ti,
                                Ttcn::ActualParList*& p_ap)
  {
    if(valuetype != V_INVOKE) FATAL_ERROR("Value::steal_invoke_data()");
    p_v = u.invoke.v;
    u.invoke.v = 0;
    p_ti = u.invoke.t_list;
    u.invoke.t_list = 0;
    p_ap = u.invoke.ap_list;
    u.invoke.ap_list = 0;
    set_valuetype(V_ERROR);
  }

  Common::Assignment* Value::get_refd_assignment()
  {
    switch(valuetype) {
    case V_FUNCTION:
    case V_ALTSTEP:
    case V_TESTCASE:
      return u.refd_fat;
      break;
    default:
      FATAL_ERROR("Value::get_refd_assignment()");
      return 0;
    }
  }

  void Value::chk()
  {
    if(checked) return;
    switch(valuetype) {
    case V_OID: {
      ReferenceChain refch(this, "While checking OBJECT IDENTIFIER"
                           " components");
      chk_OID(refch);
      break; }
    case V_ROID: {
      ReferenceChain refch(this, "While checking RELATIVE-OID components");
      chk_ROID(refch);
      break; }
    default:
      break;
    } // switch
    checked=true;
  }

  void Value::chk_OID(ReferenceChain& refch)
  {
    if (checked) return;
    if (valuetype != V_OID || u.oid_comps->size() < 1)
      FATAL_ERROR("Value::chk_OID()");
    if (!refch.add(get_fullname())) {
      checked = true;
      return;
    }
    OID_comp::oidstate_t state = OID_comp::START;
    for (size_t i = 0; i < u.oid_comps->size(); i++) {
      refch.mark_state();
      (*u.oid_comps)[i]->chk_OID(refch, this, i, state);
      refch.prev_state();
    }
    if (state != OID_comp::LATER && state != OID_comp::ITU_REC)
      error("An OBJECT IDENTIFIER value must have at least "
            "two components"); // X.680 (07/2002) 31.10
  }

  void Value::chk_ROID(ReferenceChain& refch)
  {
    if (checked) return;
    if (valuetype != V_ROID || u.oid_comps->size() < 1)
      FATAL_ERROR("Value::chk_ROID()");
    if (!refch.add(get_fullname())) {
      checked = true;
      return;
    }
    for (size_t i = 0; i < u.oid_comps->size(); i++) {
      refch.mark_state();
      (*u.oid_comps)[i]->chk_ROID(refch, i);
      refch.prev_state();
    }
  }

  void Value::chk_recursions(ReferenceChain& refch)
  {
    if (recurs_checked) return;
    Value *v = get_value_refd_last();
    if (refch.add(v->get_fullname())) {
      switch (v->valuetype) {
      case V_CHOICE:
	v->u.choice.alt_value->chk_recursions(refch);
	break;
      case V_SEQOF:
      case V_SETOF:
      case V_ARRAY:
        if (!v->is_indexed()) {
          for (size_t i = 0; i < v->u.val_vs->get_nof_vs(); i++) {
            refch.mark_state();
            v->u.val_vs->get_v_byIndex(i)->chk_recursions(refch);
            refch.prev_state();
          }
        } else {
          for (size_t i = 0; i < v->u.val_vs->get_nof_ivs(); i++) {
            refch.mark_state();
            v->u.val_vs->get_iv_byIndex(i)->get_value()
              ->chk_recursions(refch);
            refch.prev_state();
          }
        }
        break;
      case V_SEQ:
      case V_SET:
	for (size_t i = 0; i < v->u.val_nvs->get_nof_nvs(); i++) {
          refch.mark_state();
	  v->u.val_nvs->get_nv_byIndex(i)->get_value()->chk_recursions(refch);
	  refch.prev_state();
	}
        break;
      case V_EXPR:
        chk_recursions_expr(refch);
        break;
      default:
	break;
      }
      if (v->err_descrs) { // FIXME: make this work
        v->err_descrs->chk_recursions(refch);
      }
    }
    recurs_checked = true;
  }

  void Value::chk_recursions_expr(ReferenceChain& refch)
  {
    // first classify the unchecked ischosen() operation
    if (u.expr.v_optype==OPTYPE_ISCHOSEN) chk_expr_ref_ischosen();
    switch (u.expr.v_optype) {
    case OPTYPE_UNARYPLUS: // v1
    case OPTYPE_UNARYMINUS:
    case OPTYPE_NOT:
    case OPTYPE_NOT4B:
    case OPTYPE_BIT2HEX:
    case OPTYPE_BIT2INT:
    case OPTYPE_BIT2OCT:
    case OPTYPE_BIT2STR:
    case OPTYPE_CHAR2INT:
    case OPTYPE_CHAR2OCT:
    case OPTYPE_FLOAT2INT:
    case OPTYPE_FLOAT2STR:
    case OPTYPE_HEX2BIT:
    case OPTYPE_HEX2INT:
    case OPTYPE_HEX2OCT:
    case OPTYPE_HEX2STR:
    case OPTYPE_INT2CHAR:
    case OPTYPE_INT2FLOAT:
    case OPTYPE_INT2STR:
    case OPTYPE_INT2UNICHAR:
    case OPTYPE_OCT2BIT:
    case OPTYPE_OCT2CHAR:
    case OPTYPE_OCT2HEX:
    case OPTYPE_OCT2INT:
    case OPTYPE_OCT2STR:
    case OPTYPE_STR2BIT:
    case OPTYPE_STR2FLOAT:
    case OPTYPE_STR2HEX:
    case OPTYPE_STR2INT:
    case OPTYPE_STR2OCT:
    case OPTYPE_UNICHAR2INT:
    case OPTYPE_ENUM2INT:
    case OPTYPE_UNICHAR2CHAR:
    case OPTYPE_RNDWITHVAL:
    case OPTYPE_ISCHOSEN_V:
    case OPTYPE_GET_STRINGENCODING:
    case OPTYPE_REMOVE_BOM:
    case OPTYPE_DECODE_BASE64:
      refch.mark_state();
      u.expr.v1->chk_recursions(refch);
      refch.prev_state();
      break;
    case OPTYPE_ISCHOSEN_T:
      refch.mark_state();
      u.expr.t1->chk_recursions(refch);
      refch.prev_state();
      break;
    case OPTYPE_HOSTID: // [v1]
      if (u.expr.v1) {
        refch.mark_state();
        u.expr.v1->chk_recursions(refch);
        refch.prev_state();
      }
      break;
    case OPTYPE_ADD: // v1 v2
    case OPTYPE_SUBTRACT:
    case OPTYPE_MULTIPLY:
    case OPTYPE_DIVIDE:
    case OPTYPE_MOD:
    case OPTYPE_REM:
    case OPTYPE_CONCAT:
    case OPTYPE_EQ:
    case OPTYPE_LT:
    case OPTYPE_GT:
    case OPTYPE_NE:
    case OPTYPE_GE:
    case OPTYPE_LE:
    case OPTYPE_AND:
    case OPTYPE_OR:
    case OPTYPE_XOR:
    case OPTYPE_AND4B:
    case OPTYPE_OR4B:
    case OPTYPE_XOR4B:
    case OPTYPE_SHL:
    case OPTYPE_SHR:
    case OPTYPE_ROTL:
    case OPTYPE_ROTR:
    case OPTYPE_INT2BIT:
    case OPTYPE_INT2HEX:
    case OPTYPE_INT2OCT:
      refch.mark_state();
      u.expr.v1->chk_recursions(refch);
      refch.prev_state();
      refch.mark_state();
      u.expr.v2->chk_recursions(refch);
      refch.prev_state();
      break;
    case OPTYPE_UNICHAR2OCT: // v1 [v2]
    case OPTYPE_OCT2UNICHAR:
    case OPTYPE_ENCODE_BASE64:
      refch.mark_state();
      u.expr.v1->chk_recursions(refch);
      refch.prev_state();
      if (u.expr.v2) {
        refch.mark_state();
        u.expr.v2->chk_recursions(refch);
        refch.prev_state();
      }
      break;
    case OPTYPE_DECODE:
      chk_recursions_expr_decode(u.expr.r1, refch);
      chk_recursions_expr_decode(u.expr.r2, refch);
      break;
    case OPTYPE_SUBSTR:
      refch.mark_state();
      u.expr.ti1->chk_recursions(refch);
      refch.prev_state();
      refch.mark_state();
      u.expr.v2->chk_recursions(refch);
      refch.prev_state();
      refch.mark_state();
      u.expr.v3->chk_recursions(refch);
      refch.prev_state();
      break;
    case OPTYPE_REGEXP:
      refch.mark_state();
      u.expr.ti1->chk_recursions(refch);
      refch.prev_state();
      refch.mark_state();
      u.expr.t2->chk_recursions(refch);
      refch.prev_state();
      refch.mark_state();
      u.expr.v3->chk_recursions(refch);
      refch.prev_state();
      break;
    case OPTYPE_DECOMP: // v1 v2 v3
      refch.mark_state();
      u.expr.v1->chk_recursions(refch);
      refch.prev_state();
      refch.mark_state();
      u.expr.v2->chk_recursions(refch);
      refch.prev_state();
      refch.mark_state();
      u.expr.v3->chk_recursions(refch);
      refch.prev_state();
      break;
    case OPTYPE_REPLACE:
      refch.mark_state();
      u.expr.ti1->chk_recursions(refch);
      refch.prev_state();
      refch.mark_state();
      u.expr.v2->chk_recursions(refch);
      refch.prev_state();
      refch.mark_state();
      u.expr.v3->chk_recursions(refch);
      refch.prev_state();
      refch.mark_state();
      u.expr.ti4->chk_recursions(refch);
      refch.prev_state();
      break;
    case OPTYPE_LENGTHOF: // ti1
    case OPTYPE_SIZEOF: // ti1
    case OPTYPE_VALUEOF: // ti1
    case OPTYPE_ENCODE:
    case OPTYPE_ISPRESENT:
    case OPTYPE_TTCN2STRING:
      refch.mark_state();
      u.expr.ti1->chk_recursions(refch);
      refch.prev_state();
      break;
    case OPTYPE_ENCVALUE_UNICHAR: // ti1 [v2]
      refch.mark_state();
      u.expr.ti1->chk_recursions(refch);
      refch.prev_state();
      if (u.expr.v2){
        refch.mark_state();
        u.expr.v2->chk_recursions(refch);
        refch.prev_state();
      }
      break;
    case OPTYPE_DECVALUE_UNICHAR: // r1 r2 [v3]
      chk_recursions_expr_decode(u.expr.r1, refch);
      chk_recursions_expr_decode(u.expr.r2, refch);
      if (u.expr.v3){
        refch.mark_state();
        u.expr.v3->chk_recursions(refch);
        refch.prev_state();
      }
      break;
    case OPTYPE_MATCH: // v1 t2
      refch.mark_state();
      u.expr.v1->chk_recursions(refch);
      refch.prev_state();
      refch.mark_state();
      u.expr.t2->chk_recursions(refch);
      refch.prev_state();
      break;
    case OPTYPE_LOG2STR:
    case OPTYPE_ANY2UNISTR:
      u.expr.logargs->chk_recursions(refch);
      break;
    default:
      break;
    } // switch
  }

  void Value::chk_recursions_expr_decode(Ttcn::Ref_base* ref,
      ReferenceChain& refch) {
    Error_Context cntxt(this, "In the operand of operation `%s'", get_opname());
    Assignment *ass = ref->get_refd_assignment();
    if (!ass) {
      set_valuetype(V_ERROR);
      return;
    }
    switch (ass->get_asstype()) {
    case Assignment::A_CONST:
    case Assignment::A_EXT_CONST:
    case Assignment::A_MODULEPAR:
    case Assignment::A_VAR:
    case Assignment::A_PAR_VAL_IN:
    case Assignment::A_PAR_VAL_OUT:
    case Assignment::A_PAR_VAL_INOUT: {
      Value* v = new Value(V_REFD, ref);
      v->set_location(*ref);
      v->set_my_scope(get_my_scope());
      v->set_fullname(get_fullname()+".<operand>");
      refch.mark_state();
      v->chk_recursions(refch);
      refch.prev_state();
      delete v;
      break; }
    case Assignment::A_MODULEPAR_TEMP:
    case Assignment::A_TEMPLATE:
    case Assignment::A_VAR_TEMPLATE:
    case Assignment::A_PAR_TEMPL_IN:
    case Assignment::A_PAR_TEMPL_OUT:
    case Assignment::A_PAR_TEMPL_INOUT: {
      Template* t = new Template(ref->clone());
      t->set_location(*ref);
      t->set_my_scope(get_my_scope());
      t->set_fullname(get_fullname()+".<operand>");
      refch.mark_state();
      t->chk_recursions(refch);
      refch.prev_state();
      delete t;
      break; }
    default:
      // remain silent, the error has been already reported
      set_valuetype(V_ERROR);
      break;
    } // switch
  }

  bool Value::chk_expr_self_ref_templ(Ttcn::Template *t, Common::Assignment *lhs)
  {
    bool self_ref = false;
    switch (t->get_templatetype()) {
    case Ttcn::Template::SPECIFIC_VALUE: {
      Value *v = t->get_specific_value();
      self_ref |= v->get_expr_governor(Type::EXPECTED_DYNAMIC_VALUE)
        ->chk_this_value(v, lhs, Type::EXPECTED_DYNAMIC_VALUE,
          INCOMPLETE_NOT_ALLOWED, OMIT_ALLOWED, NO_SUB_CHK, NOT_IMPLICIT_OMIT, NOT_STR_ELEM);
      break; }
    case Ttcn::Template::TEMPLATE_REFD: {
      Ttcn::Ref_base *refb = t->get_reference();
      Common::Assignment *ass = refb->get_refd_assignment();
      self_ref |= (ass == lhs);
      break; }
    case Ttcn::Template::ALL_FROM:
      self_ref |= chk_expr_self_ref_templ(t->get_all_from(), lhs);
      break;
    case Ttcn::Template::TEMPLATE_LIST:
    case Ttcn::Template::SUPERSET_MATCH:
    case Ttcn::Template::SUBSET_MATCH:
    case Ttcn::Template::PERMUTATION_MATCH:
    case Ttcn::Template::COMPLEMENTED_LIST:
    case Ttcn::Template::VALUE_LIST: {
      size_t num = t->get_nof_comps();
      for (size_t i = 0; i < num; ++i) {
        self_ref |= chk_expr_self_ref_templ(t->get_temp_byIndex(i), lhs);
      }
      break; }
// not yet clear whether we should use this or the above for TEMPLATE_LIST
//    case Ttcn::Template::TEMPLATE_LIST: {
//      size_t num = t->get_nof_listitems();
//      for (size_t i=0; i < num; ++i) {
//        self_ref |= chk_expr_self_ref_templ(t->get_listitem_byIndex(i), lhs);
//      }
//      break; }
    case Ttcn::Template::NAMED_TEMPLATE_LIST: {
      size_t nnt = t->get_nof_comps();
      for (size_t i=0; i < nnt; ++i) {
        Ttcn::NamedTemplate *nt = t->get_namedtemp_byIndex(i);
        self_ref |= chk_expr_self_ref_templ(nt->get_template(), lhs);
      }
      break; }
    case Ttcn::Template::INDEXED_TEMPLATE_LIST: {
      size_t nnt = t->get_nof_comps();
      for (size_t i=0; i < nnt; ++i) {
        Ttcn::IndexedTemplate *it = t->get_indexedtemp_byIndex(i);
        self_ref |= chk_expr_self_ref_templ(it->get_template(), lhs);
      }
      break; }
    case Ttcn::Template::VALUE_RANGE: {
      Ttcn::ValueRange *vr = t->get_value_range();
      Common::Value *v = vr->get_min_v();
      if (v) self_ref |= chk_expr_self_ref_val(v, lhs);
      v = vr->get_max_v();
      if (v) self_ref |= chk_expr_self_ref_val(v, lhs);
      break; }
    case Ttcn::Template::CSTR_PATTERN:
    case Ttcn::Template::USTR_PATTERN: {
      Ttcn::PatternString *ps = t->get_cstr_pattern();
      self_ref |= ps->chk_self_ref(lhs);
      break; }
    case Ttcn::Template::BSTR_PATTERN:
    case Ttcn::Template::HSTR_PATTERN:
    case Ttcn::Template::OSTR_PATTERN: {
      // FIXME: cannot access u.pattern
      break; }
    case Ttcn::Template::ANY_VALUE:
    case Ttcn::Template::ANY_OR_OMIT:
    case Ttcn::Template::OMIT_VALUE:
    case Ttcn::Template::TEMPLATE_NOTUSED:
      break; // self-ref can't happen
    case Ttcn::Template::TEMPLATE_INVOKE:
      break; // assume self-ref can't happen
    case Ttcn::Template::DECODE_MATCH:
      self_ref |= chk_expr_self_ref_templ(t->get_decode_target()->get_Template(), lhs);
      break;
    case Ttcn::Template::TEMPLATE_ERROR:
      //FATAL_ERROR("Value::chk_expr_self_ref_templ()");
      break;
    case Ttcn::Template::TEMPLATE_CONCAT:
      self_ref |= chk_expr_self_ref_templ(t->get_concat_operand(true), lhs);
      self_ref |= chk_expr_self_ref_templ(t->get_concat_operand(false), lhs);
      break;
//    default:
//      FATAL_ERROR("todo ttype %d", t->get_templatetype());
//      break; // and hope for the best
    }
    return self_ref;
  }

  bool Value::chk_expr_self_ref_val(Common::Value *v, Common::Assignment *lhs)
  {
    Common::Type *gov = v->get_expr_governor(Type::EXPECTED_DYNAMIC_VALUE);
    namedbool is_str_elem = NOT_STR_ELEM;
    if (v->valuetype == V_REFD) {
      Reference *ref = v->get_reference();
      Ttcn::FieldOrArrayRefs *subrefs = ref->get_subrefs();
      if (subrefs && subrefs->refers_to_string_element()) {
        is_str_elem = IS_STR_ELEM;
      }
    }
    return gov->chk_this_value(v, lhs, Type::EXPECTED_DYNAMIC_VALUE,
      INCOMPLETE_NOT_ALLOWED, OMIT_ALLOWED, NO_SUB_CHK, NOT_IMPLICIT_OMIT,
      is_str_elem);
  }

  bool Value::chk_expr_self_ref(Common::Assignment *lhs)
  {
    if (valuetype != V_EXPR) FATAL_ERROR("Value::chk_expr_self_ref");
    if (!lhs) FATAL_ERROR("no lhs!");
    bool self_ref = false;
    switch (u.expr.v_optype) {
    case OPTYPE_RND: // -
    case OPTYPE_TESTCASENAME: // -
    case OPTYPE_COMP_NULL: // - (from V_TTCN3_NULL)
    case OPTYPE_COMP_MTC: // -
    case OPTYPE_COMP_SYSTEM: // -
    case OPTYPE_COMP_SELF: // -
    case OPTYPE_COMP_RUNNING_ANY: // -
    case OPTYPE_COMP_RUNNING_ALL: // -
    case OPTYPE_COMP_ALIVE_ANY: // -
    case OPTYPE_COMP_ALIVE_ALL: // -
    case OPTYPE_TMR_RUNNING_ANY: // -
    case OPTYPE_GETVERDICT: // -
    case OPTYPE_PROF_RUNNING: // -
    case OPTYPE_CHECKSTATE_ANY:
    case OPTYPE_CHECKSTATE_ALL:
      break; // nothing to do

    case OPTYPE_MATCH: // v1 t2
      self_ref |= chk_expr_self_ref_templ(u.expr.t2->get_Template(), lhs);
      // no break
    case OPTYPE_UNARYPLUS: // v1
    case OPTYPE_UNARYMINUS: // v1
    case OPTYPE_NOT: // v1
    case OPTYPE_NOT4B: // v1
    case OPTYPE_BIT2HEX: // v1
    case OPTYPE_BIT2INT: // v1
    case OPTYPE_BIT2OCT: // v1
    case OPTYPE_BIT2STR: // v1
    case OPTYPE_CHAR2INT: // v1
    case OPTYPE_CHAR2OCT: // v1
    case OPTYPE_FLOAT2INT: // v1
    case OPTYPE_FLOAT2STR: // v1
    case OPTYPE_HEX2BIT: // v1
    case OPTYPE_HEX2INT: // v1
    case OPTYPE_HEX2OCT: // v1
    case OPTYPE_HEX2STR: // v1
    case OPTYPE_INT2CHAR: // v1
    case OPTYPE_INT2FLOAT: // v1
    case OPTYPE_INT2STR: // v1
    case OPTYPE_INT2UNICHAR: // v1
    case OPTYPE_OCT2BIT: // v1
    case OPTYPE_OCT2CHAR: // v1
    case OPTYPE_OCT2HEX: // v1
    case OPTYPE_OCT2INT: // v1
    case OPTYPE_OCT2STR: // v1
    case OPTYPE_STR2BIT: // v1
    case OPTYPE_STR2FLOAT: // v1
    case OPTYPE_STR2HEX: // v1
    case OPTYPE_STR2INT: // v1
    case OPTYPE_STR2OCT: // v1
    case OPTYPE_UNICHAR2INT: // v1
    case OPTYPE_UNICHAR2CHAR: // v1
    case OPTYPE_ENUM2INT: // v1
    case OPTYPE_RNDWITHVAL: // v1
    case OPTYPE_ISCHOSEN_V: // v1 i2; ignore the identifier
    case OPTYPE_GET_STRINGENCODING:
    case OPTYPE_DECODE_BASE64:
    case OPTYPE_REMOVE_BOM:
      self_ref |= chk_expr_self_ref_val(u.expr.v1, lhs);
      break;
    case OPTYPE_COMP_RUNNING: // v1 [r2] b4
    case OPTYPE_COMP_ALIVE: // v1 [r2] b4
      self_ref |= chk_expr_self_ref_val(u.expr.v1, lhs);
      if (u.expr.r2 != NULL) {
        Common::Assignment *ass = u.expr.r2->get_refd_assignment();
        self_ref |= (ass == lhs);
      }
      break;
    case OPTYPE_HOSTID: // [v1]
      if (u.expr.v1) self_ref |= chk_expr_self_ref_val(u.expr.v1, lhs);
      break;
    case OPTYPE_ADD: // v1 v2
    case OPTYPE_SUBTRACT: // v1 v2
    case OPTYPE_MULTIPLY: // v1 v2
    case OPTYPE_DIVIDE: // v1 v2
    case OPTYPE_MOD: // v1 v2
    case OPTYPE_REM: // v1 v2
    case OPTYPE_CONCAT: // v1 v2
    case OPTYPE_EQ: // v1 v2
    case OPTYPE_LT: // v1 v2
    case OPTYPE_GT: // v1 v2
    case OPTYPE_NE: // v1 v2
    case OPTYPE_GE: // v1 v2
    case OPTYPE_LE: // v1 v2
    case OPTYPE_AND: // v1 v2
    case OPTYPE_OR: // v1 v2
    case OPTYPE_XOR: // v1 v2
    case OPTYPE_AND4B: // v1 v2
    case OPTYPE_OR4B: // v1 v2
    case OPTYPE_XOR4B: // v1 v2
    case OPTYPE_SHL: // v1 v2
    case OPTYPE_SHR: // v1 v2
    case OPTYPE_ROTL: // v1 v2
    case OPTYPE_ROTR: // v1 v2
    case OPTYPE_INT2BIT: // v1 v2
    case OPTYPE_INT2HEX: // v1 v2
    case OPTYPE_INT2OCT: // v1 v2
      self_ref |= chk_expr_self_ref_val(u.expr.v1, lhs);
      self_ref |= chk_expr_self_ref_val(u.expr.v2, lhs);
      break;
    case OPTYPE_UNICHAR2OCT: // v1 [v2]
    case OPTYPE_OCT2UNICHAR:
    case OPTYPE_ENCODE_BASE64:
      self_ref |= chk_expr_self_ref_val(u.expr.v1, lhs);
      if (u.expr.v2) self_ref |= chk_expr_self_ref_val(u.expr.v2, lhs);
      break;
    case OPTYPE_DECOMP: // v1 v2 v3
      self_ref |= chk_expr_self_ref_val(u.expr.v1, lhs);
      self_ref |= chk_expr_self_ref_val(u.expr.v2, lhs);
      self_ref |= chk_expr_self_ref_val(u.expr.v3, lhs);
      break;

    case OPTYPE_REPLACE: // ti1 v2 v3 ti4
      self_ref |= chk_expr_self_ref_templ(u.expr.ti4->get_Template(), lhs);
      // no break
    case OPTYPE_SUBSTR: // ti1 v2 v3
      self_ref |= chk_expr_self_ref_templ(u.expr.ti1->get_Template(), lhs);
      self_ref |= chk_expr_self_ref_val  (u.expr.v2, lhs);
      self_ref |= chk_expr_self_ref_val  (u.expr.v3, lhs);
      break;

    case OPTYPE_REGEXP: // ti1 t2 v3
      self_ref |= chk_expr_self_ref_templ(u.expr.ti1->get_Template(), lhs);
      self_ref |= chk_expr_self_ref_templ(u.expr.t2 ->get_Template(), lhs);
      // no break
    case OPTYPE_LENGTHOF: // ti1
    case OPTYPE_SIZEOF: // ti1
    case OPTYPE_VALUEOF: // ti1
    case OPTYPE_ENCODE: // ti1
    case OPTYPE_TTCN2STRING:
      self_ref |= chk_expr_self_ref_templ(u.expr.ti1->get_Template(), lhs);
      break;
    case OPTYPE_ENCVALUE_UNICHAR: // ti1 [v2]
      self_ref |= chk_expr_self_ref_templ(u.expr.ti1->get_Template(), lhs);
      if (u.expr.v2) self_ref |= chk_expr_self_ref_val(u.expr.v2, lhs);
      break;
    case OPTYPE_DECVALUE_UNICHAR: { // r1 r2 [v3]
      Common::Assignment *ass = u.expr.r2->get_refd_assignment();
      self_ref |= (ass == lhs);
      if (u.expr.v3) self_ref |= chk_expr_self_ref_val(u.expr.v3, lhs);
      goto label_r1;
      break; }
    case OPTYPE_COMP_CREATE: // r1 [v2] [v3] b4
      // component.create -- assume no self-ref
    case OPTYPE_ACTIVATE: // r1
      // defaultref := activate(altstep) -- assume no self-ref
    case OPTYPE_TMR_RUNNING: // r1
      // boolvar := a_timer.running -- assume no self-ref
      break;
      break;
      
    case OPTYPE_ANY2UNISTR:
    case OPTYPE_LOG2STR: {// logargs
      for (size_t i = 0, e = u.expr.logargs->get_nof_logargs(); i < e; ++i) {
        const Ttcn::LogArgument *la = u.expr.logargs->get_logarg_byIndex(i);
        switch (la->get_type()) {
        case Ttcn::LogArgument::L_UNDEF:
        case Ttcn::LogArgument::L_ERROR:
          FATAL_ERROR("%s argument type",
            u.expr.v_optype == OPTYPE_ANY2UNISTR ? "any2unistr" : "log2str");
          break; // not reached

        case Ttcn::LogArgument::L_MACRO:
        case Ttcn::LogArgument::L_STR:
          break; // self reference not possible

        case Ttcn::LogArgument::L_VAL:
        case Ttcn::LogArgument::L_MATCH:
          self_ref |= chk_expr_self_ref_val(la->get_val(), lhs);
          break;

        case Ttcn::LogArgument::L_REF: {
          Ttcn::Ref_base *ref = la->get_ref();
          Common::Assignment *ass = ref->get_refd_assignment();
          self_ref |= (ass == lhs);
          break; }

        case Ttcn::LogArgument::L_TI: {
          Ttcn::TemplateInstance *ti = la->get_ti();
          Ttcn::Template *t = ti->get_Template();
          self_ref |= chk_expr_self_ref_templ(t, lhs);
          break; }

          // no default please
        } // switch la->logargtype
      }
      break; }

    case OPTYPE_DECODE: { // r1 r2
      Common::Assignment *ass = u.expr.r2->get_refd_assignment();
      self_ref |= (ass == lhs);
      goto label_r1; }
    case OPTYPE_EXECUTE:       // r1 [v2]
      if (u.expr.v2) {
        self_ref |= chk_expr_self_ref_val(u.expr.v2, lhs);
      }
      label_r1:
      // no break
    case OPTYPE_TMR_READ: {     // r1
      Common::Assignment *ass = u.expr.r1->get_refd_assignment();
      self_ref |= (ass == lhs);
      break; }
    case OPTYPE_UNDEF_RUNNING: { // r1 [r2] b4
      if (u.expr.r2 != NULL) {
        Common::Assignment *ass2 = u.expr.r2->get_refd_assignment();
        self_ref |= (ass2 == lhs);
      }
      Common::Assignment *ass = u.expr.r1->get_refd_assignment();
      self_ref |= (ass == lhs);
      break; }
    
    case OPTYPE_ISCHOSEN_T: // t1 i2
    case OPTYPE_ISBOUND: // ti1
    case OPTYPE_ISVALUE: // ti1
    case OPTYPE_ISPRESENT: { // ti1
      Ttcn::Template *t;
      if (u.expr.v_optype == OPTYPE_ISCHOSEN_T) t = u.expr.t1;
      else t = u.expr.ti1->get_Template();
      self_ref |= chk_expr_self_ref_templ(t, lhs);
      break; }
    case OPTYPE_ISTEMPLATEKIND: // ti1 v2
      self_ref |= chk_expr_self_ref_templ(u.expr.ti1->get_Template(), lhs);
      self_ref |= chk_expr_self_ref_val(u.expr.v2, lhs);
      break;
    case OPTYPE_EXECUTE_REFD: // v1 t_list2 [v3]
      if (u.expr.v3) {
        self_ref |= chk_expr_self_ref_val(u.expr.v3, lhs);
      }
      // no break
    case OPTYPE_ACTIVATE_REFD: // v1 t_list2
      self_ref |= chk_expr_self_ref_val(u.expr.v1, lhs);
      // TODO t_list2
      break;

    case NUMBER_OF_OPTYPES: // can never happen
    case OPTYPE_ISCHOSEN: // r1 i2, should have been classified as _T or _V
      FATAL_ERROR("Value::chk_expr_self_ref(%d)", u.expr.v_optype);
      break;
    } // switch u.expr.v_optype
    return self_ref;
  }


  string Value::create_stringRepr()
  {
    // note: cannot call is_asn1() when only parsing (scopes are not properly set) 
    switch (valuetype) {
    case V_ERROR:
      return string("<erroneous>");
    case V_NULL:
      return string("NULL");
    case V_BOOL:
      if (!parse_only && is_asn1()) {
        if (u.val_bool) return string("TRUE");
        else return string("FALSE");
      }
      else {
        if (u.val_bool) return string("true");
        else return string("false");
      }
    case V_INT:
      return u.val_Int->t_str();
    case V_REAL:
      return Real2string(u.val_Real);
    case V_ENUM:
    case V_NAMEDINT:
    case V_UNDEF_LOWERID:
      return u.val_id->get_name();
    case V_NAMEDBITS: {
      string ret_val("{ ");
      for (size_t i = 0; i < u.ids->size(); i++) {
        if (i>0) ret_val += ' ';
        ret_val += u.ids->get_nth_elem(i)->get_dispname();
      }
      ret_val += '}';
      return ret_val; }
    case V_BSTR: {
      string ret_val('\'');
      ret_val += *u.str.val_str;
      ret_val += "'B";
      return ret_val; }
    case V_HSTR: {
      string ret_val('\'');
      ret_val += *u.str.val_str;
      ret_val += "'H";
      return ret_val; }
    case V_OSTR: {
      string ret_val('\'');
      ret_val += *u.str.val_str;
      ret_val += "'O";
      return ret_val; }
    case V_CSTR:
    case V_ISO2022STR:
      return u.str.val_str->get_stringRepr();
    case V_USTR:
      return u.ustr.val_ustr->get_stringRepr();
    case V_CHARSYMS:
      /** \todo stringrepr of V_CHARSYMS */
      return string("<sorry, string representation of charsyms "
        "not implemented>");
    case V_OID:
    case V_ROID: {
      string ret_val;
      if (parse_only || !is_asn1()) ret_val += "objid ";
      ret_val += "{ ";
      for (size_t i = 0; i < u.oid_comps->size(); i++) {
        if (i>0) ret_val += ' ';
        (*u.oid_comps)[i]->append_stringRepr(ret_val);
      }
      ret_val += " }";
      return ret_val; }
    case V_CHOICE:
      if (!parse_only && is_asn1()) {
        string ret_val(u.choice.alt_name->get_dispname());
        ret_val += " : ";
        ret_val += u.choice.alt_value->get_stringRepr();
        return ret_val;
      } 
      else {
        string ret_val("{ ");
        ret_val += u.choice.alt_name->get_dispname();
        ret_val += " := ";
        ret_val += u.choice.alt_value->get_stringRepr();
        ret_val += " }";
        return ret_val;
      }
    case V_SEQOF:
    case V_SETOF:
    case V_ARRAY: {
      string ret_val("{ ");
      if (!is_indexed()) {
        for (size_t i = 0; i < u.val_vs->get_nof_vs(); i++) {
          if (i > 0) ret_val += ", ";
          ret_val += u.val_vs->get_v_byIndex(i)->get_stringRepr();
        }
      } else {
        for (size_t i = 0; i < u.val_vs->get_nof_ivs(); i++) {
          if (i > 0) ret_val += ", ";
          ret_val += u.val_vs->get_iv_byIndex(i)->get_value()->get_stringRepr();
        }
      }
      ret_val += " }";
      return ret_val; }
    case V_SEQ:
    case V_SET: {
      string ret_val("{ ");
      bool asn1_flag = !parse_only && is_asn1();
      for (size_t i = 0; i < u.val_nvs->get_nof_nvs(); i++) {
        if (i > 0) ret_val += ", ";
          NamedValue *nv = u.val_nvs->get_nv_byIndex(i);
        ret_val += nv->get_name().get_dispname();
        if (asn1_flag) ret_val += ' ';
        else ret_val += " := ";
        ret_val += nv->get_value()->get_stringRepr();
      }
      ret_val += " }";
      return ret_val; }
    case V_REFD: {
      // do not evaluate the reference if it is not done so far
      // (e.g. in parse-only mode)
      Value *t_val = u.ref.refd_last ? u.ref.refd_last : this;
      if (t_val->valuetype == V_REFD) return t_val->u.ref.ref->get_dispname();
      else return t_val->get_stringRepr(); }
    case V_OMIT:
      return string("omit");
    case V_VERDICT:
      switch (u.verdict) {
      case Verdict_NONE:
        return string("none");
      case Verdict_PASS:
        return string("pass");
      case Verdict_INCONC:
        return string("inconc");
      case Verdict_FAIL:
        return string("fail");
      case Verdict_ERROR:
        return string("error");
      default:
        return string("<unknown verdict value>");
      }
    case V_DEFAULT_NULL:
    case V_FAT_NULL:
      return string("null");
    case V_EXPR:
      switch (u.expr.v_optype) {
      case OPTYPE_RND:
        return string("rnd()");
      case OPTYPE_TESTCASENAME:
        return string("testcasename()");
      case OPTYPE_UNARYPLUS:
        return create_stringRepr_unary("+");
      case OPTYPE_UNARYMINUS:
        return create_stringRepr_unary("-");
      case OPTYPE_NOT:
        return create_stringRepr_unary("not");
      case OPTYPE_NOT4B:
        return create_stringRepr_unary("not4b");
      case OPTYPE_BIT2HEX:
        return create_stringRepr_predef1("bit2hex");
      case OPTYPE_BIT2INT:
        return create_stringRepr_predef1("bit2int");
      case OPTYPE_BIT2OCT:
        return create_stringRepr_predef1("bit2oct");
      case OPTYPE_BIT2STR:
        return create_stringRepr_predef1("bit2str");
      case OPTYPE_CHAR2INT:
        return create_stringRepr_predef1("char2int");
      case OPTYPE_CHAR2OCT:
        return create_stringRepr_predef1("char2oct");
      case OPTYPE_FLOAT2INT:
        return create_stringRepr_predef1("float2int");
      case OPTYPE_FLOAT2STR:
        return create_stringRepr_predef1("float2str");
      case OPTYPE_HEX2BIT:
        return create_stringRepr_predef1("hex2bit");
      case OPTYPE_HEX2INT:
        return create_stringRepr_predef1("hex2int");
      case OPTYPE_HEX2OCT:
        return create_stringRepr_predef1("hex2oct");
      case OPTYPE_HEX2STR:
        return create_stringRepr_predef1("hex2str");
      case OPTYPE_INT2CHAR:
        return create_stringRepr_predef1("int2char");
      case OPTYPE_INT2FLOAT:
        return create_stringRepr_predef1("int2float");
      case OPTYPE_INT2STR:
        return create_stringRepr_predef1("int2str");
      case OPTYPE_INT2UNICHAR:
        return create_stringRepr_predef1("int2unichar");
      case OPTYPE_OCT2BIT:
        return create_stringRepr_predef1("oct2bit");
      case OPTYPE_OCT2CHAR:
        return create_stringRepr_predef1("oct2char");
      case OPTYPE_OCT2HEX:
        return create_stringRepr_predef1("oct2hex");
      case OPTYPE_OCT2INT:
        return create_stringRepr_predef1("oct2int");
      case OPTYPE_OCT2STR:
        return create_stringRepr_predef1("oct2str");
      case OPTYPE_GET_STRINGENCODING:
        return create_stringRepr_predef1("get_stringencoding");
      case OPTYPE_REMOVE_BOM:
        return create_stringRepr_predef1("remove_bom");
      case OPTYPE_ENCODE_BASE64: {
        if (u.expr.v2) return create_stringRepr_predef2("encode_base64");
        else return create_stringRepr_predef1("encode_base64");
      }
      case OPTYPE_DECODE_BASE64:
        return create_stringRepr_predef1("decode_base64");
      case OPTYPE_OCT2UNICHAR:{
        if (u.expr.v2) return create_stringRepr_predef2("oct2unichar");
        else return create_stringRepr_predef1("oct2unichar");
      }
      case OPTYPE_UNICHAR2OCT: {
         if (u.expr.v2) return create_stringRepr_predef2("unichar2oct");
         else return create_stringRepr_predef1("unichar2oct");
      }
      case OPTYPE_ENCVALUE_UNICHAR: {
         if (u.expr.v2) return create_stringRepr_predef2("encvalue_unichar");
         else return create_stringRepr_predef1("encvalue_unichar");
      }
      case OPTYPE_HOSTID: {
        if (u.expr.v1) return create_stringRepr_predef1("hostid");   
        else return string("hostid()");
      }
      case OPTYPE_DECVALUE_UNICHAR: {
         if (u.expr.v3) {
           string ret_val("decvalue_unichar");
           ret_val += '(';
           ret_val += u.expr.v1->get_stringRepr();
           ret_val += ", ";
           ret_val += u.expr.v2->get_stringRepr();
           ret_val += ", ";
           ret_val += u.expr.v3->get_stringRepr();
           ret_val += ')';
           return ret_val;
         }
         else return create_stringRepr_predef2("decvalue_unichar");
      }
      case OPTYPE_STR2BIT:
        return create_stringRepr_predef1("str2bit");
      case OPTYPE_STR2FLOAT:
        return create_stringRepr_predef1("str2float");
      case OPTYPE_STR2HEX:
        return create_stringRepr_predef1("str2hex");
      case OPTYPE_STR2INT:
        return create_stringRepr_predef1("str2int");
      case OPTYPE_STR2OCT:
        return create_stringRepr_predef1("str2oct");
      case OPTYPE_UNICHAR2INT:
        return create_stringRepr_predef1("unichar2int");
      case OPTYPE_UNICHAR2CHAR:
        return create_stringRepr_predef1("unichar2char");
      case OPTYPE_ENUM2INT:
        return create_stringRepr_predef1("enum2int");
      case OPTYPE_ENCODE:
        return create_stringRepr_predef1("encvalue");
      case OPTYPE_DECODE:
        return create_stringRepr_predef2("decvalue");
      case OPTYPE_RNDWITHVAL:
        return create_stringRepr_predef1("rnd");
      case OPTYPE_ADD:
        return create_stringRepr_infix("+");
      case OPTYPE_SUBTRACT:
        return create_stringRepr_infix("-");
      case OPTYPE_MULTIPLY:
        return create_stringRepr_infix("*");
      case OPTYPE_DIVIDE:
        return create_stringRepr_infix("/");
      case OPTYPE_MOD:
        return create_stringRepr_infix("mod");
      case OPTYPE_REM:
        return create_stringRepr_infix("rem");
      case OPTYPE_CONCAT:
        return create_stringRepr_infix("&");
      case OPTYPE_EQ:
        return create_stringRepr_infix("==");
      case OPTYPE_LT:
        return create_stringRepr_infix("<");
      case OPTYPE_GT:
        return create_stringRepr_infix(">");
      case OPTYPE_NE:
        return create_stringRepr_infix("!=");
      case OPTYPE_GE:
        return create_stringRepr_infix(">=");
      case OPTYPE_LE:
        return create_stringRepr_infix("<=");
      case OPTYPE_AND:
        return create_stringRepr_infix("and");
      case OPTYPE_OR:
        return create_stringRepr_infix("or");
      case OPTYPE_XOR:
        return create_stringRepr_infix("xor");
      case OPTYPE_AND4B:
        return create_stringRepr_infix("and4b");
      case OPTYPE_OR4B:
        return create_stringRepr_infix("or4b");
      case OPTYPE_XOR4B:
        return create_stringRepr_infix("xor4b");
      case OPTYPE_SHL:
        return create_stringRepr_infix("<<");
      case OPTYPE_SHR:
        return create_stringRepr_infix(">>");
      case OPTYPE_ROTL:
        return create_stringRepr_infix("<@");
      case OPTYPE_ROTR:
        return create_stringRepr_infix("@>");
      case OPTYPE_INT2BIT:
        return create_stringRepr_predef2("int2bit");
      case OPTYPE_INT2HEX:
        return create_stringRepr_predef2("int2hex");
      case OPTYPE_INT2OCT:
        return create_stringRepr_predef2("int2oct");
      case OPTYPE_SUBSTR: {
        string ret_val("substr(");
        u.expr.ti1->append_stringRepr(ret_val);
        ret_val += ", ";
        ret_val += u.expr.v2->get_stringRepr();
        ret_val += ", ";
        ret_val += u.expr.v3->get_stringRepr();
        ret_val += ')';
        return ret_val;
      }
      case OPTYPE_REGEXP: {
        string ret_val("regexp");
        if (u.expr.b4) {
          ret_val += " @nocase ";
        }
        ret_val += "(";
        u.expr.ti1->append_stringRepr(ret_val);
        ret_val += ", ";
        u.expr.t2->append_stringRepr(ret_val);
        ret_val += ", ";
        ret_val += u.expr.v3->get_stringRepr();
        ret_val += ')';
        return ret_val;
      }
      case OPTYPE_DECOMP: {
        string ret_val("decomp(");
        ret_val += u.expr.v1->get_stringRepr();
        ret_val += ", ";
        ret_val += u.expr.v2->get_stringRepr();
        ret_val += ", ";
        ret_val += u.expr.v3->get_stringRepr();
        ret_val += ')';
        return ret_val;
      }
      case OPTYPE_REPLACE: {
        string ret_val("replace(");
        u.expr.ti1->append_stringRepr(ret_val);
        ret_val += ", ";
        ret_val += u.expr.v2->get_stringRepr();
        ret_val += ", ";
        ret_val += u.expr.v3->get_stringRepr();
        ret_val += ", ";
        u.expr.ti4->append_stringRepr(ret_val);
        ret_val += ')';
        return ret_val;
      }
      case OPTYPE_ISPRESENT: {
        string ret_val("ispresent(");
        u.expr.ti1->append_stringRepr(ret_val);
        ret_val += ')';
        return ret_val; }
      case OPTYPE_ISCHOSEN: {
        string ret_val("ischosen(");
	ret_val += u.expr.r1->get_dispname();
	ret_val += '.';
	ret_val += u.expr.i2->get_dispname();
	ret_val += ')';
	return ret_val; }
      case OPTYPE_ISCHOSEN_V: {
        string ret_val("ischosen(");
	ret_val += u.expr.v1->get_stringRepr();
	ret_val += '.';
	ret_val += u.expr.i2->get_dispname();
	ret_val += ')';
	return ret_val; }
      case OPTYPE_ISCHOSEN_T: {
        string ret_val("ischosen(");
	ret_val += u.expr.t1->get_stringRepr();
	ret_val += '.';
	ret_val += u.expr.i2->get_dispname();
	ret_val += ')';
	return ret_val; }
      case OPTYPE_LENGTHOF: {
        string ret_val("lengthof(");
        u.expr.ti1->append_stringRepr(ret_val);
        ret_val += ')';
	return ret_val; }
      case OPTYPE_SIZEOF: {
        string ret_val("sizeof(");
        u.expr.ti1->append_stringRepr(ret_val);
        ret_val += ')';
        return ret_val; }
      case OPTYPE_ISVALUE: {
	string ret_val("isvalue(");
        u.expr.ti1->append_stringRepr(ret_val);
        ret_val += ')';
	return ret_val; }
      case OPTYPE_VALUEOF: {
        string ret_val("valueof(");
        u.expr.ti1->append_stringRepr(ret_val);
        ret_val += ')';
	return ret_val; }
      case OPTYPE_LOG2STR:
        return string("log2str(...)");
      case OPTYPE_ANY2UNISTR:
        return string("any2unistr(...)");     
      case OPTYPE_MATCH: {
        string ret_val("match(");
        ret_val += u.expr.v1->get_stringRepr();
        ret_val += ", ";
        u.expr.t2->append_stringRepr(ret_val);
        ret_val += ')';
	return ret_val; }
      case OPTYPE_TTCN2STRING: {
        string ret_val("ttcn2string(");
        u.expr.ti1->append_stringRepr(ret_val);
        ret_val += ')';
        return ret_val;
      }
      case OPTYPE_COMP_RUNNING: // v1 [r2] b4
      case OPTYPE_UNDEF_RUNNING: // r1 [r2] b4
      case OPTYPE_TMR_RUNNING: // r1 [r2] b4
      case OPTYPE_COMP_ALIVE: { // v1 [r2] b4
        string ret_val;
        if (u.expr.b4) {
          ret_val = "any from ";
        }
        ret_val += (u.expr.v_optype == OPTYPE_COMP_RUNNING ||
                    u.expr.v_optype == OPTYPE_COMP_ALIVE) ?
          u.expr.v1->get_stringRepr() : u.expr.r1->get_dispname();
        ret_val += u.expr.v_optype == OPTYPE_COMP_ALIVE ? ".alive" : ".running";
        if (u.expr.r2 != NULL) {
          ret_val += " -> @index value " + u.expr.r2->get_dispname();
        }
        return ret_val; }
      case OPTYPE_COMP_NULL:
        return string("null");
      case OPTYPE_COMP_MTC:
        return string("mtc");
      case OPTYPE_COMP_SYSTEM:
        return string("system");
      case OPTYPE_COMP_SELF:
        return string("self");
      case OPTYPE_COMP_CREATE: {
        string ret_val(u.expr.r1->get_dispname());
	ret_val += ".create";
	if (u.expr.v2 || u.expr.v3) {
	  ret_val += '(';
	  if (u.expr.v2) ret_val += u.expr.v2->get_stringRepr();
	  else ret_val += '-';
	  if (u.expr.v3) {
	    ret_val += ", ";
	    ret_val += u.expr.v3->get_stringRepr();
	  }
	  ret_val += ')';
	}
	if (u.expr.b4) ret_val += " alive";
	return ret_val; }
      case OPTYPE_COMP_RUNNING_ANY:
        return string("any component.running");
      case OPTYPE_COMP_RUNNING_ALL:
        return string("all component.running");
      case OPTYPE_COMP_ALIVE_ANY:
        return string("any component.alive");
      case OPTYPE_COMP_ALIVE_ALL:
        return string("all component.alive");
      case OPTYPE_TMR_READ:
        return u.expr.r1->get_dispname() + ".read";
      case OPTYPE_TMR_RUNNING_ANY:
        return string("any timer.running");
      case OPTYPE_CHECKSTATE_ANY:
      case OPTYPE_CHECKSTATE_ALL: {
        string ret_val("");
        if (u.expr.r1) {
          ret_val += u.expr.r1->get_dispname();
        } else {
          if (u.expr.v_optype == OPTYPE_CHECKSTATE_ANY) {
            ret_val += "any port";
          } else if (u.expr.v_optype == OPTYPE_CHECKSTATE_ALL) {
            ret_val += "all port";
          }
        }
        ret_val += "checkstate(";
        ret_val += u.expr.v2->get_stringRepr();
        ret_val += ")";
        return ret_val; }
      case OPTYPE_GETVERDICT:
        return string("getverdict");
      case OPTYPE_ACTIVATE: {
        string ret_val("activate(");
	ret_val += u.expr.r1->get_dispname();
	ret_val += ')';
	return ret_val; }
      case OPTYPE_ACTIVATE_REFD: {
        string ret_val("activate(derefer(");
	ret_val += u.expr.v1->get_stringRepr();
	ret_val += ")(";
	if (u.expr.state == EXPR_CHECKED) {
          if (u.expr.ap_list2) {
	    size_t nof_pars = u.expr.ap_list2->get_nof_pars();
            for (size_t i = 0; i < nof_pars; i++) {
              if (i > 0) ret_val += ", ";
              u.expr.ap_list2->get_par(i)->append_stringRepr(ret_val);
            }
	  }
	} else {
          if (u.expr.t_list2) {
	    size_t nof_pars = u.expr.t_list2->get_nof_tis();
            for (size_t i = 0; i < nof_pars; i++) {
              if (i > 0) ret_val += ", ";
              u.expr.t_list2->get_ti_byIndex(i)->append_stringRepr(ret_val);
            }
	  }
	}
        ret_val += "))";
        return ret_val; }
      case OPTYPE_EXECUTE: {
        string ret_val("execute(");
	ret_val += u.expr.r1->get_dispname();
	if (u.expr.v2) {
	  ret_val += ", ";
	  ret_val += u.expr.v2->get_stringRepr();
	}
	ret_val += ')';
	return ret_val; }
      case OPTYPE_EXECUTE_REFD: {
        string ret_val("execute(derefers(");
	ret_val += u.expr.v1->get_stringRepr();
	ret_val += ")(";
	if (u.expr.state == EXPR_CHECKED) {
          if (u.expr.ap_list2) {
	    size_t nof_pars = u.expr.ap_list2->get_nof_pars();
            for (size_t i = 0; i < nof_pars; i++) {
              if (i > 0) ret_val += ", ";
              u.expr.ap_list2->get_par(i)->append_stringRepr(ret_val);
            }
	  }
	} else {
          if (u.expr.t_list2) {
	    size_t nof_pars = u.expr.t_list2->get_nof_tis();
            for (size_t i = 0; i < nof_pars; i++) {
              if (i > 0) ret_val += ", ";
              u.expr.t_list2->get_ti_byIndex(i)->append_stringRepr(ret_val);
            }
	  }
	}
        ret_val += ')';
        if(u.expr.v3) {
          ret_val += ", ";
          ret_val += u.expr.v3->get_stringRepr();
        }
        ret_val += ')';
        return ret_val; }
      case OPTYPE_PROF_RUNNING:
        return string("@profiler.running");
      default:
        return string("<unsupported optype>");
      } // switch u.expr.v_optype
    case V_MACRO:
      switch (u.macro) {
      case MACRO_MODULEID:
	return string("%moduleId");
      case MACRO_FILENAME:
	return string("%fileName");
      case MACRO_BFILENAME:
        return string("__BFILE__");
      case MACRO_FILEPATH:
        return string("__FILE__");
      case MACRO_LINENUMBER:
	return string("%lineNumber");
      case MACRO_LINENUMBER_C:
	return string("__LINE__");
      case MACRO_DEFINITIONID:
	return string("%definitionId");
      case MACRO_SCOPE:
	return string("__SCOPE__");
      case MACRO_TESTCASEID:
	return string("%testcaseId");
      default:
	return string("<unknown macro>");
      } // switch u.macro
    case V_NOTUSED:
      return string('-');
    case V_FUNCTION:
    case V_ALTSTEP:
    case V_TESTCASE: {
      string ret_val("refers(");
      ret_val += u.refd_fat->get_assname();
      ret_val += ')';
      return ret_val; }
    case V_INVOKE: {
      string ret_val;
      ret_val += u.invoke.v->get_stringRepr();
      ret_val += ".apply(";
      if (u.invoke.ap_list) {
        size_t nof_pars = u.invoke.ap_list->get_nof_pars();
        for (size_t i = 0; i < nof_pars; i++) {
          if (i > 0) ret_val += ", ";
          u.invoke.ap_list->get_par(i)->append_stringRepr(ret_val);
        }
      } else if (u.invoke.t_list) {
        size_t nof_pars = u.invoke.t_list->get_nof_tis();
        for (size_t i = 0; i < nof_pars; i++) {
          if (i > 0) ret_val += ", ";
          u.invoke.t_list->get_ti_byIndex(i)->append_stringRepr(ret_val);
        }
      }
      ret_val += ')';
      return ret_val; }
    case V_REFER: {
      string ret_val("refers(");
      ret_val += u.refered->get_dispname();
      ret_val += ')';
      return ret_val; }
    default:
      return string("<unsupported valuetype>");
    } // switch valuetype
  }

  string Value::create_stringRepr_unary(const char *operator_str)
  {
    string ret_val(operator_str);
    ret_val += '(';
    ret_val += u.expr.v1->get_stringRepr();
    ret_val += ')';
    return ret_val;
  }

  string Value::create_stringRepr_infix(const char *operator_str)
  {
    string ret_val('(');
    ret_val += u.expr.v1->get_stringRepr();
    ret_val += ' ';
    ret_val += operator_str;
    ret_val += ' ';
    ret_val += u.expr.v2->get_stringRepr();
    ret_val += ')';
    return ret_val;
  }

  string Value::create_stringRepr_predef1(const char *function_name)
  {
    string ret_val(function_name);
    ret_val += '(';
    if (u.expr.v_optype == OPTYPE_ENCODE || u.expr.v_optype == OPTYPE_ENCVALUE_UNICHAR) { // ti1, not v1
      ret_val += u.expr.ti1->get_specific_value()->get_stringRepr();
    }
    else ret_val += u.expr.v1->get_stringRepr();
    ret_val += ')';
    return ret_val;
  }

  string Value::create_stringRepr_predef2(const char *function_name)
  {
    string ret_val(function_name);
    ret_val += '(';
    ret_val += u.expr.v1->get_stringRepr();
    ret_val += ", ";
    ret_val += u.expr.v2->get_stringRepr();
    ret_val += ')';
    return ret_val;
  }

  bool Value::operator==(Value& val)
  {
    Value *left = get_value_refd_last();
    Type *left_governor = left->get_my_governor();
    if (left_governor) left_governor = left_governor->get_type_refd_last();
    Value *right = val.get_value_refd_last();
    Type *right_governor = right->get_my_governor();
    if (right_governor) right_governor = right_governor->get_type_refd_last();
    if (left_governor && right_governor
        && !left_governor->is_compatible(right_governor, NULL, NULL)
        && !right_governor->is_compatible(left_governor, NULL, NULL))
      FATAL_ERROR("Value::operator==");

    // Not-A-Value is not equal to anything (NaN analogy:)
    if ( (left->valuetype==V_ERROR) || (right->valuetype==V_ERROR) )
      return false;

    switch (left->valuetype) {
    case V_NULL:
    case V_OMIT:
    case V_DEFAULT_NULL:
    case V_FAT_NULL:
    case V_NOTUSED:
      return left->valuetype == right->valuetype;
    case V_BOOL:
      return right->valuetype == V_BOOL &&
        left->get_val_bool() == right->get_val_bool();
    case V_INT:
      return right->valuetype == V_INT && *left->get_val_Int()
        == *right->get_val_Int();
    case V_REAL:
      return right->valuetype == V_REAL &&
        left->get_val_Real() == right->get_val_Real();
    case V_CSTR:
      switch (right->valuetype) {
      case V_CSTR:
	return left->get_val_str() == right->get_val_str();
      case V_USTR:
	return right->get_val_ustr() == left->get_val_str();
      case V_ISO2022STR:
	return right->get_val_iso2022str() == left->get_val_str();
      default:
	return false;
      }
    case V_BSTR:
    case V_HSTR:
    case V_OSTR:
      return left->valuetype == right->valuetype &&
        left->get_val_str() == right->get_val_str();
    case V_USTR:
      switch (right->valuetype) {
      case V_CSTR:
        return left->get_val_ustr() == right->get_val_str();
      case V_USTR:
        return left->get_val_ustr() == right->get_val_ustr();
      case V_ISO2022STR:
        return left->get_val_ustr() == right->get_val_iso2022str();
      default:
        return false;
      }
    case V_ISO2022STR:
      switch (right->valuetype) {
      case V_CSTR:
        return left->get_val_iso2022str() == right->get_val_str();
      case V_USTR:
        // The appropriate operator==() is missing.  The operands are swapped,
        // but it shouldn't be a problem.
        return right->get_val_ustr() == left->get_val_iso2022str();
      case V_ISO2022STR:
        return left->get_val_iso2022str() == right->get_val_iso2022str();
      default:
        return false;
      }
    case V_ENUM:
      return right->valuetype == V_ENUM &&
        left->get_val_id()->get_name() == right->get_val_id()->get_name();
    case V_OID:
    case V_ROID:
      if (right->valuetype == V_OID || right->valuetype == V_ROID) {
	vector<string> act, other;
	get_oid_comps(act);
	val.get_oid_comps(other);
	size_t act_size = act.size(), other_size = other.size();
	bool ret_val;
	if (act_size == other_size) {
	  ret_val = true;
	  for (size_t i = 0; i < act_size; i++)
	    if (*act[i] != *other[i]) {
	      ret_val = false;
	      break;
	    }
	} else ret_val = false;
	for (size_t i = 0; i < act_size; i++) delete act[i];
	act.clear();
	for (size_t i = 0; i < other_size; i++) delete other[i];
	other.clear();
	return ret_val;
      } else return false;
    case V_CHOICE:
      return right->valuetype == V_CHOICE &&
         left->get_alt_name().get_name() == right->get_alt_name().get_name() &&
         *(left->get_alt_value()) == *(right->get_alt_value());
    case V_SEQ:
    case V_SET: {
      if (!left_governor) FATAL_ERROR("Value::operator==");
      if (left->valuetype != right->valuetype) return false;
      size_t nof_comps = left_governor->get_nof_comps();
      for (size_t i = 0; i < nof_comps; i++) {
        Value *lval = NULL, *rval = NULL;
        CompField* cfl = left_governor->get_comp_byIndex(i);
        const Identifier& field_name = cfl->get_name();
        if (left->has_comp_withName(field_name)) {
          lval = left->get_comp_value_byName(field_name);
          if (right->has_comp_withName(field_name)) {
            rval = right->get_comp_value_byName(field_name);
            if ((lval->valuetype == V_OMIT && rval->valuetype != V_OMIT)
                || (rval->valuetype == V_OMIT && lval->valuetype!=V_OMIT))
              return false;
            else if (!(*lval == *rval))
              return false;
          } else {
            if (cfl->has_default()) {
              if (!(*lval == *cfl->get_defval()))
                return false;
            } else {
              if (lval->valuetype != V_OMIT)
                return false;
            }
          }
        } else {
          if(right->has_comp_withName(field_name)) {
            rval = right->get_comp_value_byName(field_name);
            if(cfl->has_default()) {
              if(rval->valuetype==V_OMIT) return false;
              else {
                lval = cfl->get_defval();
                if (!(*lval==*rval)) return false;
              }
            }
          }
        }
      }
      return true; }
    case V_SEQOF:
    case V_ARRAY: {
      if (left->valuetype != right->valuetype) return false;
      size_t ncomps = get_nof_comps();
      if (ncomps != right->get_nof_comps()) return false;

      if (left->is_indexed() && right->is_indexed()) { //both of them are indexed
        bool found = false;
        map<IndexedValue*, void> uncovered;
        for (size_t i = 0; i < left->get_nof_comps(); ++i)
          uncovered.add(left->u.val_vs->get_iv_byIndex(i),0);

        for (size_t i = 0; i < right->get_nof_comps(); ++i) {
          found = false;
          for (size_t j = 0; j < uncovered.size(); ++j) {
            if (*(uncovered.get_nth_key(j)->get_value()) ==
              *(right->get_comp_byIndex(i)) &&
              *(uncovered.get_nth_key(j)->get_index()) ==
              *(right->get_index_byIndex(i))) {
                found = true;
                uncovered.erase(uncovered.get_nth_key(j));
                break;
            }
          }
          if (!found) break;
        }
        uncovered.clear();
        return found;
      } else if (left->is_indexed() || right->is_indexed()) {
        Value* indexed_one = 0;
        Value* not_indexed_one = 0;

        if(left->is_indexed()) { // left is indexed, right is not
          indexed_one = left;
          not_indexed_one = right;
        } else { // right indexed, left is not
          indexed_one = right;
          not_indexed_one = left;
        }

        for(size_t i = 0; i < ncomps; ++i) {
          Value* ind = indexed_one->get_index_byIndex(i)->get_value_refd_last();
          if(!(ind->valuetype == V_INT &&
            *(not_indexed_one->get_comp_byIndex(ind->u.val_Int->get_val())) ==
            *(indexed_one->get_comp_byIndex(i))))
            { return false; }
        }
        return true;
      } else { // none of them is indexed
        for (size_t i = 0; i < ncomps; i++) {
          if (!(*(left->get_comp_byIndex(i)) == *(right->get_comp_byIndex(i))))
            return false;
        }
        return true;
      }
    }
    case V_SETOF: {
      if (right->valuetype != V_SETOF) return false;
      size_t ncomps = get_nof_comps();
      if (ncomps != right->get_nof_comps()) return false;
      if (ncomps == 0) return true;
      map<size_t, void> uncovered;
      for (size_t i = 0; i < ncomps; i++) uncovered.add(i, 0);
      for (size_t i = 0; i < ncomps; i++) {
        Value *left_item = left->get_comp_byIndex(i);
        bool pair_found = false;
        for (size_t j = 0; j < ncomps - i; j++) {
          size_t right_index = uncovered.get_nth_key(j);
          if (*left_item == *right->get_comp_byIndex(right_index)) {
            uncovered.erase(right_index);
            pair_found = true;
            break;
          }
        }
        if (!pair_found) {
          uncovered.clear();
          return false;
        }
      }
      return true; }
    case V_VERDICT:
      return right->valuetype == V_VERDICT &&
        left->get_val_verdict() == right->get_val_verdict();
    case V_FUNCTION:
    case V_ALTSTEP:
    case V_TESTCASE:
      return left->valuetype == right->valuetype &&
        left->get_refd_assignment() == right->get_refd_assignment();
    default:
      FATAL_ERROR("Value::operator==");
    }
    return true;
  }

  bool Value::operator<(Value& val)
  {
    Value *left = get_value_refd_last();
    Type *left_governor = left->get_my_governor();
    if(left_governor) left_governor=left_governor->get_type_refd_last();
    Value *right = val.get_value_refd_last();
    Type *right_governor = right->get_my_governor();
    if(right_governor) right_governor=right_governor->get_type_refd_last();
    if (left->get_valuetype() != right->get_valuetype())
      FATAL_ERROR("Value::operator<");
    switch(valuetype){
    case V_INT:
      return *left->get_val_Int() < *right->get_val_Int();
    case V_REAL:
      return (left->get_val_Real() < right->get_val_Real());
    case V_ENUM:
      if(!left_governor || !right_governor)
        FATAL_ERROR("Value::operator<");
      if(left_governor!=right_governor)
        FATAL_ERROR("Value::operator<");
      return (left_governor->get_enum_val_byId(*left->get_val_id()) <
              right_governor->get_enum_val_byId(*right->get_val_id()));
    default:
      FATAL_ERROR("Value::operator<");
    }
    return true;
  }

  bool Value::is_string_type(Type::expected_value_t exp_val)
  {
    switch (get_expr_returntype(exp_val)) {
    case Type::T_CSTR:
    case Type::T_USTR:
    case Type::T_BSTR:
    case Type::T_HSTR:
    case Type::T_OSTR:
      return true;
    default:
      return false;
    }
  }

  void Value::generate_code_expr(expression_struct *expr)
  {
    if (has_single_expr()) {
      expr->expr = mputstr(expr->expr, get_single_expr().c_str());
    } else {
      switch (valuetype) {
      case V_EXPR:
        generate_code_expr_expr(expr);
        break;
      case V_CHOICE:
      case V_SEQOF:
      case V_SETOF:
      case V_ARRAY:
      case V_SEQ:
      case V_SET: {
        const string& tmp_id = get_temporary_id();
        const char *tmp_id_str = tmp_id.c_str();
        expr->preamble = mputprintf(expr->preamble, "%s %s;\n",
          my_governor->get_genname_value(my_scope).c_str(), tmp_id_str);
        set_genname_recursive(tmp_id);
        expr->preamble = generate_code_init(expr->preamble, tmp_id_str);
        expr->expr = mputstr(expr->expr, tmp_id_str);
        break; }
      case V_INT: {
        const string& tmp_id = get_temporary_id();
        const char *tmp_id_str = tmp_id.c_str();
        expr->preamble = mputprintf(expr->preamble, "INTEGER %s;\n",
                                    tmp_id_str);
        set_genname_recursive(tmp_id);
        expr->preamble = generate_code_init(expr->preamble, tmp_id_str);
        expr->expr = mputstr(expr->expr, tmp_id_str);
        break; }
      case V_REFD: {
        if (!get_needs_conversion()) {
          u.ref.ref->generate_code_const_ref(expr);
        } else {
          Type *my_gov = get_expr_governor_last();
          Type *refd_gov = u.ref.ref->get_refd_assignment()->get_Type()
            ->get_field_type(u.ref.ref->get_subrefs(),
            Type::EXPECTED_DYNAMIC_VALUE)->get_type_refd_last();
          // Make sure that nothing goes wrong.
          if (!my_gov || !refd_gov || my_gov == refd_gov)
            FATAL_ERROR("Value::generate_code_expr()");
          expression_struct expr_tmp;
          Code::init_expr(&expr_tmp);
          const string& tmp_id1 = get_temporary_id();
          const char *tmp_id_str1 = tmp_id1.c_str();
          const string& tmp_id2 = get_temporary_id();
          const char *tmp_id_str2 = tmp_id2.c_str();
          expr->preamble = mputprintf(expr->preamble,
            "%s %s;\n", refd_gov->get_genname_value(my_scope).c_str(),
            tmp_id_str1);
          expr_tmp.expr = mputprintf(expr_tmp.expr, "%s = ", tmp_id_str1);
          u.ref.ref->generate_code_const_ref(&expr_tmp);
          expr->preamble = Code::merge_free_expr(expr->preamble, &expr_tmp);
          expr->preamble = mputprintf(expr->preamble,
            "%s %s;\n"
            "if (!%s(%s, %s)) TTCN_error(\"Values or templates of types `%s' "
            "and `%s' are not compatible at run-time\");\n",
            my_gov->get_genname_value(my_scope).c_str(), tmp_id_str2,
            TypeConv::get_conv_func(refd_gov, my_gov, get_my_scope()
            ->get_scope_mod()).c_str(), tmp_id_str2, tmp_id_str1, my_gov
            ->get_typename().c_str(), refd_gov->get_typename().c_str());
          expr->expr = mputprintf(expr->expr, "%s", tmp_id_str2);
        }
        break; }
      case V_INVOKE:
        generate_code_expr_invoke(expr);
        break;
      default:
        FATAL_ERROR("Value::generate_code_expr(%d)", valuetype);
      }
    }
  }

  void Value::generate_code_expr_mandatory(expression_struct *expr)
  {
    generate_code_expr(expr);
    if (valuetype == V_REFD && get_value_refd_last()->valuetype == V_REFD)
      generate_code_expr_optional_field_ref(expr, u.ref.ref);
  }

  bool Value::can_use_increment(Reference *ref) const
  {
    if (valuetype != V_EXPR) {
      return false;
    }
    switch (u.expr.v_optype) {
    case OPTYPE_ADD:
    case OPTYPE_SUBTRACT:
      break;
    default:
      return false;
    }
    bool v1_one = u.expr.v1->get_valuetype() == V_INT && *u.expr.v1->get_val_Int() == 1;
    bool v2_one = u.expr.v2->get_valuetype() == V_INT && *u.expr.v2->get_val_Int() == 1;
    if ((v1_one && u.expr.v2->get_valuetype() == V_REFD &&
         u.expr.v2->get_reference()->get_refd_assignment()->get_id() == ref->get_refd_assignment()->get_id()) ||
        (v2_one && u.expr.v1->get_valuetype() == V_REFD &&
         u.expr.v1->get_reference()->get_refd_assignment()->get_id() == ref->get_refd_assignment()->get_id())) {
      return true;
    }
    return false;
  }

  char *Value::generate_code_init(char *str, const char *name)
  {
    if (get_code_generated()) return str;
    if (err_descrs != NULL && err_descrs->has_descr(NULL)) {
      str = err_descrs->generate_code_init_str(NULL, str, string(name));
    }
    switch (valuetype) {
    case V_NULL:
    case V_BOOL:
    case V_REAL:
    case V_ENUM:
    case V_BSTR:
    case V_HSTR:
    case V_OSTR:
    case V_CSTR:
    case V_USTR:
    case V_ISO2022STR:
    case V_OID:
    case V_ROID:
    case V_VERDICT:
    case V_DEFAULT_NULL:
    case V_FAT_NULL:
    case V_FUNCTION:
    case V_ALTSTEP:
    case V_TESTCASE:
      // These values have a single string equivalent.
      str = mputprintf(str, "%s = %s;\n", name, get_single_expr().c_str());
      break;
    case V_INT:
      if (u.val_Int->is_native_fit())
        str = mputprintf(str, "%s = %s;\n", name, get_single_expr().c_str());
      else
        // It's always an INTEGER.
        str = mputprintf(str, "{ INTEGER INTEGER_tmp(%s);\n%s = INTEGER_tmp; "
          "}\n", get_single_expr().c_str(), name);
      break;
    case V_EXPR:
    case V_INVOKE: {
      expression_struct expr;
      Code::init_expr(&expr);
      expr.expr = mputprintf(expr.expr, "%s = ", name);
      generate_code_expr(&expr);
      str = Code::merge_free_expr(str, &expr);
      break; }
    case V_CHOICE:
      str = generate_code_init_choice(str, name);
      break;
    case V_SEQOF:
    case V_SETOF:
      if (!is_indexed()) str = generate_code_init_seof(str, name);
      else str = generate_code_init_indexed(str, name);
      break;
    case V_ARRAY:
      if (!is_indexed()) str = generate_code_init_array(str, name);
      else str = generate_code_init_indexed(str, name);
      break;
    case V_SEQ:
    case V_SET:
      str = generate_code_init_se(str, name);
      break;
    case V_REFD:
      str = generate_code_init_refd(str, name);
      break;
    case V_MACRO:
      switch (u.macro) {
      case MACRO_TESTCASEID:
        str = mputprintf(str, "%s = TTCN_Runtime::get_testcase_id_macro();\n", name);
        break;
      default:
        // all others must already be evaluated away
        FATAL_ERROR("Value::generate_code_init()");
      }
      break;
    case V_NOTUSED:
      // unbound value, don't generate anything
      break;
    default:
      FATAL_ERROR("Value::generate_code_init()");
    }
    if (err_descrs != NULL && err_descrs->has_descr(NULL)) {
      str = mputprintf(str, "%s.set_err_descr(&%s_%lu_err_descr);\n", name,
        name, (unsigned long) err_descrs->get_descr_index(NULL));
    }
    set_code_generated();
    return str;
  }

  char *Value::rearrange_init_code(char *str, Common::Module* usage_mod)
  {
    switch (valuetype) {
    case V_REFD: {
      Ttcn::ActualParList *parlist = u.ref.ref->get_parlist();
      if (parlist) {
	str = parlist->rearrange_init_code(str, usage_mod);
      }
      break; }
    case V_INVOKE: {
      str = u.invoke.v->rearrange_init_code(str, usage_mod);
      str = u.invoke.ap_list->rearrange_init_code(str, usage_mod);
      break; }
    case V_EXPR:
      switch (u.expr.v_optype) {
      case OPTYPE_UNARYPLUS:
      case OPTYPE_UNARYMINUS:
      case OPTYPE_NOT:
      case OPTYPE_NOT4B:
      case OPTYPE_BIT2HEX:
      case OPTYPE_BIT2INT:
      case OPTYPE_BIT2OCT:
      case OPTYPE_BIT2STR:
      case OPTYPE_CHAR2INT:
      case OPTYPE_CHAR2OCT:
      case OPTYPE_FLOAT2INT:
      case OPTYPE_FLOAT2STR:
      case OPTYPE_HEX2BIT:
      case OPTYPE_HEX2INT:
      case OPTYPE_HEX2OCT:
      case OPTYPE_HEX2STR:
      case OPTYPE_INT2CHAR:
      case OPTYPE_INT2FLOAT:
      case OPTYPE_INT2STR:
      case OPTYPE_INT2UNICHAR:
      case OPTYPE_OCT2BIT:
      case OPTYPE_OCT2CHAR:
      case OPTYPE_OCT2HEX:
      case OPTYPE_OCT2INT:
      case OPTYPE_OCT2STR:
      case OPTYPE_STR2BIT:
      case OPTYPE_STR2FLOAT:
      case OPTYPE_STR2HEX:
      case OPTYPE_STR2INT:
      case OPTYPE_STR2OCT:
      case OPTYPE_UNICHAR2INT:
      case OPTYPE_UNICHAR2CHAR:
      case OPTYPE_ENUM2INT:
      case OPTYPE_ISCHOSEN_V:
      case OPTYPE_GET_STRINGENCODING:
      case OPTYPE_REMOVE_BOM:
      case OPTYPE_DECODE_BASE64:
        str = u.expr.v1->rearrange_init_code(str, usage_mod);
        break;
      case OPTYPE_DECODE: {
        Ttcn::ActualParList *parlist = u.expr.r1->get_parlist();
        if (parlist) str = parlist->rearrange_init_code(str, usage_mod);

        parlist = u.expr.r2->get_parlist();
        if (parlist) str = parlist->rearrange_init_code(str, usage_mod);
        break; }
      case OPTYPE_HOSTID:    
        if (u.expr.v1) str = u.expr.v1->rearrange_init_code(str, usage_mod);
        break;
      case OPTYPE_ADD:
      case OPTYPE_SUBTRACT:
      case OPTYPE_MULTIPLY:
      case OPTYPE_DIVIDE:
      case OPTYPE_MOD:
      case OPTYPE_REM:
      case OPTYPE_CONCAT:
      case OPTYPE_EQ:
      case OPTYPE_LT:
      case OPTYPE_GT:
      case OPTYPE_NE:
      case OPTYPE_GE:
      case OPTYPE_LE:
      case OPTYPE_AND:
      case OPTYPE_OR:
      case OPTYPE_XOR:
      case OPTYPE_AND4B:
      case OPTYPE_OR4B:
      case OPTYPE_XOR4B:
      case OPTYPE_SHL:
      case OPTYPE_SHR:
      case OPTYPE_ROTL:
      case OPTYPE_ROTR:
      case OPTYPE_INT2BIT:
      case OPTYPE_INT2HEX:
      case OPTYPE_INT2OCT:
      //case OPTYPE_DECODE:
        str = u.expr.v1->rearrange_init_code(str, usage_mod);
        str = u.expr.v2->rearrange_init_code(str, usage_mod);
        break;
      case OPTYPE_UNICHAR2OCT: // v1 [v2]
      case OPTYPE_OCT2UNICHAR:
      case OPTYPE_ENCODE_BASE64:
        str = u.expr.v1->rearrange_init_code(str, usage_mod);
        if (u.expr.v2) str = u.expr.v2->rearrange_init_code(str, usage_mod);
        break;
      case OPTYPE_SUBSTR:
        str = u.expr.ti1->rearrange_init_code(str, usage_mod);
        str = u.expr.v2->rearrange_init_code(str, usage_mod);
        str = u.expr.v3->rearrange_init_code(str, usage_mod);
        break;
      case OPTYPE_REGEXP:
        str = u.expr.ti1->rearrange_init_code(str, usage_mod);
        str = u.expr.t2->rearrange_init_code(str, usage_mod);
        str = u.expr.v3->rearrange_init_code(str, usage_mod);
        break;
      case OPTYPE_DECOMP:
        str = u.expr.v1->rearrange_init_code(str, usage_mod);
        str = u.expr.v2->rearrange_init_code(str, usage_mod);
        str = u.expr.v3->rearrange_init_code(str, usage_mod);
        break;
      case OPTYPE_REPLACE:
        str = u.expr.ti1->rearrange_init_code(str, usage_mod);
        str = u.expr.v2->rearrange_init_code(str, usage_mod);
        str = u.expr.v3->rearrange_init_code(str, usage_mod);
        str = u.expr.ti4->rearrange_init_code(str, usage_mod);
        break;
      case OPTYPE_ISTEMPLATEKIND:
        str = u.expr.ti1->rearrange_init_code(str, usage_mod);
        str = u.expr.v2->rearrange_init_code(str, usage_mod);
        break;
      case OPTYPE_LENGTHOF:
      case OPTYPE_SIZEOF:
      case OPTYPE_VALUEOF:
      case OPTYPE_ENCODE:
      case OPTYPE_ISPRESENT:
      case OPTYPE_TTCN2STRING:
        str = u.expr.ti1->rearrange_init_code(str, usage_mod);
        break;
      case OPTYPE_ENCVALUE_UNICHAR:
        str = u.expr.ti1->rearrange_init_code(str, usage_mod);
        if (u.expr.v2) str = u.expr.v2->rearrange_init_code(str, usage_mod);
        break;
      case OPTYPE_DECVALUE_UNICHAR: {
        Ttcn::ActualParList *parlist = u.expr.r1->get_parlist();
        Common::Assignment *ass = u.expr.r1->get_refd_assignment();
        if (parlist) str = parlist->rearrange_init_code(str, usage_mod);
        (void)ass; // eliminate assigned but not used warning
        parlist = u.expr.r2->get_parlist();
        ass = u.expr.r2->get_refd_assignment();
        if (parlist) str = parlist->rearrange_init_code(str, usage_mod);
        if (u.expr.v3) str = u.expr.v3->rearrange_init_code(str, usage_mod);
        break; }
      case OPTYPE_ISCHOSEN_T:
        str = u.expr.t1->rearrange_init_code(str, usage_mod);
        break;
      case OPTYPE_MATCH:
        str = u.expr.v1->rearrange_init_code(str, usage_mod);
        str = u.expr.t2->rearrange_init_code(str, usage_mod);
        break;
      default:
        // other kinds of expressions cannot appear within templates
        break;
      }
      break;
    default:
      break;
    }
    return str;
  }

  char* Value::generate_code_tmp(char *str, const char *prefix,
                                 size_t& blockcount)
  {
    char *s2 = memptystr();
    char *s1 = generate_code_tmp(NULL, s2);
    if (s2[0]) {
      if (blockcount == 0) {
	str = mputstr(str, "{\n");
	blockcount++;
      }
      str = mputstr(str, s2);
    }
    Free(s2);
    str=mputstr(str, prefix);
    str=mputstr(str, s1);
    Free(s1);
    return str;
  }

  char *Value::generate_code_tmp(char *str, char*& init)
  {
    expression_struct expr;
    Code::init_expr(&expr);
    generate_code_expr_mandatory(&expr);
    if (expr.preamble || expr.postamble) {
      if (valuetype == V_EXPR &&
	  (u.expr.v_optype == OPTYPE_AND || u.expr.v_optype == OPTYPE_OR)) {
	// a temporary variable is already introduced
	if (expr.preamble) init = mputstr(init, expr.preamble);
	if (expr.postamble) init = mputstr(init, expr.postamble);
	str = mputstr(str, expr.expr);
      } else {
	const string& tmp_id = get_temporary_id();
	const char *tmp_id_str = tmp_id.c_str();
	init = mputprintf(init, "%s %s;\n"
	  "{\n",
	  my_governor->get_type_refd_last()->get_typetype() == Type::T_BOOL ?
	    "boolean" : my_governor->get_genname_value(my_scope).c_str(),
	  tmp_id_str);
	if (expr.preamble) init = mputstr(init, expr.preamble);
	init = mputprintf(init, "%s = %s;\n", tmp_id_str, expr.expr);
	if (expr.postamble) init = mputstr(init, expr.postamble);
	init = mputstr(init, "}\n");
	str = mputstr(str, tmp_id_str);
      }
    } else str = mputstr(str, expr.expr);
    Code::free_expr(&expr);
    return str;
  }

  void Value::generate_code_log(expression_struct *expr)
  {
    if (explicit_cast_needed()) {
      char *expr_backup = expr->expr;
      expr->expr = NULL;
      generate_code_expr(expr);
      const string& tmp_id = get_temporary_id();
      const char *tmp_id_str = tmp_id.c_str();
      // We have to create a temporary object, because the parser of GCC
      // earlier than 3.4.x (e.g. 3.0.4) in some cases cannot recognize the
      // constructor call that is, this does not work: type(...).log(); but
      // this works: type tmp(...); tmp.log();.
      expr->preamble = mputprintf(expr->preamble, "%s %s(%s);\n",
        my_governor->get_genname_value(my_scope).c_str(), tmp_id_str,
        expr->expr);
      Free(expr->expr);
      expr->expr = mputstr(expr_backup, tmp_id_str);
    } else {
      generate_code_expr(expr);
    }
    expr->expr = mputstr(expr->expr, ".log()");
  }

  void Value::generate_code_log_match(expression_struct *expr)
  {
    if (valuetype != V_EXPR || u.expr.v_optype != OPTYPE_MATCH)
      FATAL_ERROR("Value::generate_code_log_match()");
    // Maybe, it's a more general problem, but for complete GCC 3.0.4
    // compliance the whole code-generation should be checked.  Standalone
    // constructs like: "A(a[0].f());" should be avoided.  The current
    // solution for HK38721 uses an additional assignment to overcome the
    // issue.  The generated code will be slower, but it's needed for old GCC
    // versions in specific circumstances.
    if (u.expr.t2->needs_temp_ref()) {
      char *expr_backup = expr->expr;
      expr->expr = NULL;
      u.expr.t2->generate_code(expr);
      const string& tmp_id = get_temporary_id();
      const char *tmp_id_str = tmp_id.c_str();
      expr->preamble = mputprintf(expr->preamble,
        "%s %s = %s;\n", u.expr.t2->get_expr_governor(Type::EXPECTED_TEMPLATE)
        ->get_genname_template(my_scope).c_str(), tmp_id_str, expr->expr);
      Free(expr->expr);
      expr->expr = mputstr(expr_backup, tmp_id_str);
    } else {
      // Workaround for "A(NS::B).a(C);" like constructs for GCC 3.0.4.  For
      // some reason "(A(NS::B)).a(C);" compiles fine.
      expr->expr = mputc(expr->expr, '(');
      u.expr.t2->generate_code(expr);
      expr->expr = mputc(expr->expr, ')');
    }
    expr->expr = mputstr(expr->expr, ".log_match(");
    u.expr.v1->generate_code_expr(expr);
    expr->expr = mputprintf(expr->expr, "%s)", omit_in_value_list ? ", TRUE" : "");
  }

  void Value::generate_code_expr_expr(expression_struct *expr)
  {
    switch (u.expr.v_optype) {
    case OPTYPE_RND:
      generate_code_expr_rnd(expr, 0);
      break;
    case OPTYPE_UNARYPLUS:
      // same as without the '+' operator
      u.expr.v1->generate_code_expr(expr);
      break;
    case OPTYPE_UNARYMINUS:
      generate_code_expr_unary(expr, "-", u.expr.v1);
      break;
    case OPTYPE_NOT:
      generate_code_expr_unary(expr, "!", u.expr.v1);
      break;
    case OPTYPE_NOT4B:
      generate_code_expr_unary(expr, "~", u.expr.v1);
      break;
    case OPTYPE_BIT2HEX:
      generate_code_expr_predef1(expr, "bit2hex", u.expr.v1);
      break;
    case OPTYPE_BIT2INT:
      generate_code_expr_predef1(expr, "bit2int", u.expr.v1);
      break;
    case OPTYPE_BIT2OCT:
      generate_code_expr_predef1(expr, "bit2oct", u.expr.v1);
      break;
    case OPTYPE_BIT2STR:
      generate_code_expr_predef1(expr, "bit2str", u.expr.v1);
      break;
    case OPTYPE_CHAR2INT:
      generate_code_expr_predef1(expr, "char2int", u.expr.v1);
      break;
    case OPTYPE_CHAR2OCT:
      generate_code_expr_predef1(expr, "char2oct", u.expr.v1);
      break;
    case OPTYPE_FLOAT2INT:
      generate_code_expr_predef1(expr, "float2int", u.expr.v1);
      break;
    case OPTYPE_FLOAT2STR:
      generate_code_expr_predef1(expr, "float2str", u.expr.v1);
      break;
    case OPTYPE_HEX2BIT:
      generate_code_expr_predef1(expr, "hex2bit", u.expr.v1);
      break;
    case OPTYPE_HEX2INT:
      generate_code_expr_predef1(expr, "hex2int", u.expr.v1);
      break;
    case OPTYPE_HEX2OCT:
      generate_code_expr_predef1(expr, "hex2oct", u.expr.v1);
      break;
    case OPTYPE_HEX2STR:
      generate_code_expr_predef1(expr, "hex2str", u.expr.v1);
      break;
    case OPTYPE_INT2CHAR:
      generate_code_expr_predef1(expr, "int2char", u.expr.v1);
      break;
    case OPTYPE_INT2FLOAT:
      generate_code_expr_predef1(expr, "int2float", u.expr.v1);
      break;
    case OPTYPE_INT2STR:
      generate_code_expr_predef1(expr, "int2str", u.expr.v1);
      break;
    case OPTYPE_INT2UNICHAR:
      generate_code_expr_predef1(expr, "int2unichar", u.expr.v1);
      break;
    case OPTYPE_OCT2BIT:
      generate_code_expr_predef1(expr, "oct2bit", u.expr.v1);
      break;
    case OPTYPE_OCT2CHAR:
      generate_code_expr_predef1(expr, "oct2char", u.expr.v1);
      break;
    case OPTYPE_GET_STRINGENCODING:
      generate_code_expr_predef1(expr, "get_stringencoding", u.expr.v1);
      break;
    case OPTYPE_REMOVE_BOM:
      generate_code_expr_predef1(expr, "remove_bom", u.expr.v1);
      break;
    case OPTYPE_ENCODE_BASE64:
      if (u.expr.v2)
        generate_code_expr_predef2(expr, "encode_base64", u.expr.v1, u.expr.v2);
      else
        generate_code_expr_predef1(expr, "encode_base64", u.expr.v1);
      break;
    case OPTYPE_DECODE_BASE64:
      generate_code_expr_predef1(expr, "decode_base64", u.expr.v1);
      break;
    case OPTYPE_OCT2UNICHAR:
      if (u.expr.v2)
        generate_code_expr_predef2(expr, "oct2unichar", u.expr.v1, u.expr.v2);
      else
        generate_code_expr_predef1(expr, "oct2unichar", u.expr.v1);
      break;
    case OPTYPE_UNICHAR2OCT:
      if (u.expr.v2)
        generate_code_expr_predef2(expr, "unichar2oct", u.expr.v1, u.expr.v2);
      else
        generate_code_expr_predef1(expr, "unichar2oct", u.expr.v1);
      break;
    case OPTYPE_ENCVALUE_UNICHAR:
        generate_code_expr_encvalue_unichar(expr);
      break;
    case OPTYPE_DECVALUE_UNICHAR:
        generate_code_expr_decvalue_unichar(expr);
      break;
    case OPTYPE_HOSTID:
      generate_code_expr_hostid(expr);
      break;
    case OPTYPE_OCT2HEX:
      generate_code_expr_predef1(expr, "oct2hex", u.expr.v1);
      break;
    case OPTYPE_OCT2INT:
      generate_code_expr_predef1(expr, "oct2int", u.expr.v1);
      break;
    case OPTYPE_OCT2STR:
      generate_code_expr_predef1(expr, "oct2str", u.expr.v1);
      break;
    case OPTYPE_STR2BIT:
      generate_code_expr_predef1(expr, "str2bit", u.expr.v1);
      break;
    case OPTYPE_STR2FLOAT:
      generate_code_expr_predef1(expr, "str2float", u.expr.v1);
      break;
    case OPTYPE_STR2HEX:
      generate_code_expr_predef1(expr, "str2hex", u.expr.v1);
      break;
    case OPTYPE_STR2INT:
      generate_code_expr_predef1(expr, "str2int", u.expr.v1);
      break;
    case OPTYPE_STR2OCT:
      generate_code_expr_predef1(expr, "str2oct", u.expr.v1);
      break;
    case OPTYPE_UNICHAR2INT:
      generate_code_expr_predef1(expr, "unichar2int", u.expr.v1);
      break;
    case OPTYPE_UNICHAR2CHAR:
      generate_code_expr_predef1(expr, "unichar2char", u.expr.v1);
      break;
    case OPTYPE_ENUM2INT: {
      Type* enum_type = u.expr.v1->get_expr_governor_last();
      if (!enum_type) FATAL_ERROR("Value::generate_code_expr_expr(): enum2int");
      expr->expr = mputprintf(expr->expr, "%s::enum2int(",
        enum_type->get_genname_value(my_scope).c_str());
      u.expr.v1->generate_code_expr_mandatory(expr);
      expr->expr = mputc(expr->expr, ')');
      break;}
    case OPTYPE_ENCODE:
      generate_code_expr_encode(expr);
      break;
    case OPTYPE_DECODE:
      generate_code_expr_decode(expr);
      break;
    case OPTYPE_RNDWITHVAL:
      generate_code_expr_rnd(expr, u.expr.v1);
      break;
    case OPTYPE_ADD:
      generate_code_expr_infix(expr, "+", u.expr.v1, u.expr.v2, false);
      break;
    case OPTYPE_SUBTRACT:
      generate_code_expr_infix(expr, "-", u.expr.v1, u.expr.v2, false);
      break;
    case OPTYPE_MULTIPLY:
      generate_code_expr_infix(expr, "*", u.expr.v1, u.expr.v2, false);
      break;
    case OPTYPE_DIVIDE:
      generate_code_expr_infix(expr, "/", u.expr.v1, u.expr.v2, false);
      break;
    case OPTYPE_MOD:
      generate_code_expr_predef2(expr, "mod", u.expr.v1, u.expr.v2);
      break;
    case OPTYPE_REM:
      generate_code_expr_predef2(expr, "rem", u.expr.v1, u.expr.v2);
      break;
    case OPTYPE_CONCAT:
      generate_code_expr_infix(expr, "+", u.expr.v1, u.expr.v2, false);
      break;
    case OPTYPE_EQ:
      generate_code_expr_infix(expr, "==", u.expr.v1, u.expr.v2, true);
      break;
    case OPTYPE_LT:
      generate_code_expr_infix(expr, "<", u.expr.v1, u.expr.v2, false);
      break;
    case OPTYPE_GT:
      generate_code_expr_infix(expr, ">", u.expr.v1, u.expr.v2, false);
      break;
    case OPTYPE_NE:
      generate_code_expr_infix(expr, "!=", u.expr.v1, u.expr.v2, true);
      break;
    case OPTYPE_GE:
      generate_code_expr_infix(expr, ">=", u.expr.v1, u.expr.v2, false);
      break;
    case OPTYPE_LE:
      generate_code_expr_infix(expr, "<=", u.expr.v1, u.expr.v2, false);
      break;
    case OPTYPE_AND:
    case OPTYPE_OR:
      generate_code_expr_and_or(expr);
      break;
    case OPTYPE_XOR:
      generate_code_expr_infix(expr, "^", u.expr.v1, u.expr.v2, false);
      break;
    case OPTYPE_AND4B:
      generate_code_expr_infix(expr, "&", u.expr.v1, u.expr.v2, false);
      break;
    case OPTYPE_OR4B:
      generate_code_expr_infix(expr, "|", u.expr.v1, u.expr.v2, false);
      break;
    case OPTYPE_XOR4B:
      generate_code_expr_infix(expr, "^", u.expr.v1, u.expr.v2, false);
      break;
    case OPTYPE_SHL:
      generate_code_expr_infix(expr, "<<", u.expr.v1, u.expr.v2, false);
      break;
    case OPTYPE_SHR:
      generate_code_expr_infix(expr, ">>", u.expr.v1, u.expr.v2, false);
      break;
    case OPTYPE_ROTL:
      generate_code_expr_infix(expr, "<<=", u.expr.v1, u.expr.v2, false);
      break;
    case OPTYPE_ROTR:
      generate_code_expr_infix(expr, ">>=", u.expr.v1, u.expr.v2, false);
      break;
    case OPTYPE_INT2BIT:
      generate_code_expr_predef2(expr, "int2bit", u.expr.v1, u.expr.v2);
      break;
    case OPTYPE_INT2HEX:
      generate_code_expr_predef2(expr, "int2hex", u.expr.v1, u.expr.v2);
      break;
    case OPTYPE_INT2OCT:
      generate_code_expr_predef2(expr, "int2oct", u.expr.v1, u.expr.v2);
      break;
    case OPTYPE_SUBSTR:
      if (!get_needs_conversion()) generate_code_expr_substr(expr);
      else generate_code_expr_substr_replace_compat(expr);
      break;
    case OPTYPE_REGEXP:
      generate_code_expr_regexp(expr);
      break;
    case OPTYPE_DECOMP:
      generate_code_expr_predef3(expr, "decomp", u.expr.v1, u.expr.v2, u.expr.v3);
      break;
    case OPTYPE_REPLACE:
      if (!get_needs_conversion()) generate_code_expr_replace(expr);
      else generate_code_expr_substr_replace_compat(expr);
      break;
    case OPTYPE_ISCHOSEN: // r1 i2
      FATAL_ERROR("Value::generate_code_expr_expr()");
      break;
    case OPTYPE_ISCHOSEN_V: // v1 i2
      u.expr.v1->generate_code_expr_mandatory(expr);
	expr->expr = mputprintf(expr->expr, ".ischosen(%s::ALT_%s)",
	  u.expr.v1->get_my_governor()->get_genname_value(my_scope).c_str(),
	  u.expr.i2->get_name().c_str());
      break;
    case OPTYPE_ISCHOSEN_T: // t1 i2
      u.expr.t1->generate_code_expr(expr);
	expr->expr = mputprintf(expr->expr, ".ischosen(%s::ALT_%s)",
	  u.expr.t1->get_my_governor()->get_genname_value(my_scope).c_str(),
	  u.expr.i2->get_name().c_str());
      break;
    case OPTYPE_ISPRESENT:
    case OPTYPE_ISBOUND: {
      Template::templatetype_t temp = u.expr.ti1->get_Template()
          ->get_templatetype();
      if (temp == Template::SPECIFIC_VALUE) {
        Value* specific_value = u.expr.ti1->get_Template()
            ->get_specific_value();
        if (specific_value->get_valuetype() == Value::V_REFD) {
          Ttcn::Reference* reference =
              dynamic_cast<Ttcn::Reference*>(specific_value->get_reference());
          if (reference) {
            reference->generate_code_ispresentbound(expr, false,
              u.expr.v_optype==OPTYPE_ISBOUND);
            break;
          }
        }
      } else if (temp == Template::TEMPLATE_REFD){
          Ttcn::Reference* reference =
            dynamic_cast<Ttcn::Reference*>(u.expr.ti1->get_Template()
              ->get_reference());
          if (reference) {
            reference->generate_code_ispresentbound(expr, true,
              u.expr.v_optype==OPTYPE_ISBOUND);
            break;
         }
      }
    }
    // no break
    case OPTYPE_LENGTHOF: // ti1
      // fall through, separated later
    case OPTYPE_SIZEOF:  // ti1
      // fall through, separated later
    case OPTYPE_ISVALUE: { // ti1
      if (u.expr.ti1->is_only_specific_value()) {
        Value *t_val=u.expr.ti1->get_Template()->get_specific_value();
        bool cast_needed = t_val->explicit_cast_needed(
          u.expr.v_optype != OPTYPE_LENGTHOF);
        if(cast_needed) {
	// the ambiguous C++ expression is converted to the value class
          expr->expr = mputprintf(expr->expr, "%s(",
	    t_val->get_my_governor()->get_genname_value(my_scope).c_str());
        }

        if (u.expr.v_optype != OPTYPE_LENGTHOF
             && u.expr.v_optype != OPTYPE_SIZEOF) {
          t_val->generate_code_expr(expr);
         } else {
           t_val->generate_code_expr_mandatory(expr);
         }

        if(cast_needed) expr->expr=mputc(expr->expr, ')');
      }
      else u.expr.ti1->generate_code(expr);

      switch (u.expr.v_optype) {
      case OPTYPE_ISBOUND:
        expr->expr=mputstr(expr->expr, ".is_bound()");
        break;
      case OPTYPE_ISPRESENT:
        expr->expr=mputprintf(expr->expr, ".is_present()");
        break;
      case OPTYPE_SIZEOF:
        expr->expr=mputstr(expr->expr, ".size_of()");
        break;
      case OPTYPE_LENGTHOF:
        expr->expr=mputstr(expr->expr, ".lengthof()");
        break;
      case OPTYPE_ISVALUE:
        expr->expr=mputstr(expr->expr, ".is_value()");
        break;
      default:
        FATAL_ERROR("Value::generate_code_expr_expr()");
      }
      break; }
    case OPTYPE_VALUEOF: // ti1
      u.expr.ti1->generate_code(expr);
      expr->expr = mputstr(expr->expr, ".valueof()");
      break;
    case OPTYPE_ISTEMPLATEKIND: // ti1 v2
      u.expr.ti1->generate_code(expr);
      expr->expr = mputstr(expr->expr, ".get_istemplate_kind((const char*)");
      u.expr.v2->generate_code_expr(expr);
      expr->expr = mputstr(expr->expr, ")");
      break;
    case OPTYPE_MATCH: // v1 t2
      u.expr.t2->generate_code(expr);
      expr->expr = mputstr(expr->expr, ".match(");
      u.expr.v1->generate_code_expr(expr);
      expr->expr = mputprintf(expr->expr, "%s)", omit_in_value_list ? ", TRUE" : "");
      break;
    case OPTYPE_UNDEF_RUNNING:
      // it is resolved during semantic check
      FATAL_ERROR("Value::generate_code_expr_expr(): undef running");
      break;
    case OPTYPE_COMP_NULL: // -
      expr->expr=mputstr(expr->expr, "NULL_COMPREF");
      break;
    case OPTYPE_COMP_MTC: // -
      expr->expr=mputstr(expr->expr, "MTC_COMPREF");
      break;
    case OPTYPE_COMP_SYSTEM: // -
      expr->expr=mputstr(expr->expr, "SYSTEM_COMPREF");
      break;
    case OPTYPE_COMP_SELF: // -
      expr->expr=mputstr(expr->expr, "self");
      break;
    case OPTYPE_COMP_CREATE: // r1 [v2] [v3] b4
      generate_code_expr_create(expr, u.expr.r1, u.expr.v2, u.expr.v3,
	u.expr.b4);
      break;
    case OPTYPE_COMP_RUNNING: // v1 [r2] b4
    case OPTYPE_COMP_ALIVE:
      u.expr.v1->generate_code_expr(expr);
      if(u.expr.v1->get_valuetype() == V_REFD)
        generate_code_expr_optional_field_ref(expr, u.expr.v1->get_reference());
      expr->expr = mputstr(expr->expr, u.expr.v_optype == OPTYPE_COMP_ALIVE ?
        ".alive(" : ".running(");
      if (u.expr.r2 != NULL) {
        Ttcn::Reference* index_ref = dynamic_cast<Ttcn::Reference*>(u.expr.r2);
        if (index_ref == NULL) {
          FATAL_ERROR("Value::generate_code_expr_expr");
        }
        Ttcn::Statement::generate_code_index_redirect(expr, index_ref, my_scope);
      }
      else {
        expr->expr=mputstr(expr->expr, "NULL");
      }
      expr->expr = mputc(expr->expr, ')');
      break;
    case OPTYPE_COMP_RUNNING_ANY: // -
      expr->expr=mputstr(expr->expr,
                         "TTCN_Runtime::component_running(ANY_COMPREF)");
      break;
    case OPTYPE_COMP_RUNNING_ALL: // -
      expr->expr=mputstr(expr->expr,
                         "TTCN_Runtime::component_running(ALL_COMPREF)");
      break;
     // v1
      u.expr.v1->generate_code_expr(expr);
      if(u.expr.v1->get_valuetype() == V_REFD)
        generate_code_expr_optional_field_ref(expr, u.expr.v1->get_reference());
      expr->expr = mputstr(expr->expr, ".alive()");
      break;
    case OPTYPE_COMP_ALIVE_ANY: // -
      expr->expr = mputstr(expr->expr,
	"TTCN_Runtime::component_alive(ANY_COMPREF)");
      break;
    case OPTYPE_COMP_ALIVE_ALL: // -
      expr->expr = mputstr(expr->expr,
	"TTCN_Runtime::component_alive(ALL_COMPREF)");
      break;
    case OPTYPE_TMR_READ: // r1
      u.expr.r1->generate_code(expr);
      expr->expr = mputstr(expr->expr, ".read()");
      break;
    case OPTYPE_TMR_RUNNING: // r1
      u.expr.r1->generate_code(expr);
      expr->expr = mputstr(expr->expr, ".running(");
      if (u.expr.r2 != NULL) {
        Ttcn::Reference* index_ref = dynamic_cast<Ttcn::Reference*>(u.expr.r2);
        if (index_ref == NULL) {
          FATAL_ERROR("Value::generate_code_expr_expr");
        }
        Ttcn::Statement::generate_code_index_redirect(expr, index_ref, my_scope);
      }
      else {
        expr->expr=mputstr(expr->expr, "NULL");
      }
      expr->expr = mputc(expr->expr, ')');
      break;
    case OPTYPE_TMR_RUNNING_ANY: // -
      expr->expr=mputstr(expr->expr, "TIMER::any_running()");
      break;
    case OPTYPE_GETVERDICT: // -
      expr->expr=mputstr(expr->expr, "TTCN_Runtime::getverdict()");
      break;
    case OPTYPE_TESTCASENAME: // -
      expr->expr = mputstr(expr->expr, "TTCN_Runtime::get_testcasename()");
      break;
    case OPTYPE_ACTIVATE: // r1
      generate_code_expr_activate(expr);
      break;
    case OPTYPE_CHECKSTATE_ANY: // [r1] v2
    case OPTYPE_CHECKSTATE_ALL:
      generate_code_expr_checkstate(expr);
      break;
    case OPTYPE_ACTIVATE_REFD: // v1 ap_list2
      generate_code_expr_activate_refd(expr);
      break;
    case OPTYPE_EXECUTE: // r1 [v2]
      generate_code_expr_execute(expr);
      break;
    case OPTYPE_EXECUTE_REFD: //v1 ap_list2 [v3]
      generate_code_expr_execute_refd(expr);
      break;
    case OPTYPE_LOG2STR:
    case OPTYPE_ANY2UNISTR:
      u.expr.logargs->generate_code_expr(expr);
      break;
    case OPTYPE_TTCN2STRING: {
      Type* param_governor = u.expr.ti1->get_Template()->get_template_refd_last()->get_my_governor();
      if (param_governor==NULL) FATAL_ERROR("Value::generate_code_expr_expr()");
      param_governor = param_governor->get_type_refd_last();
      expr->expr = mputstr(expr->expr, "ttcn_to_string(");
      if (!u.expr.ti1->get_DerivedRef() && !u.expr.ti1->get_Type() &&
          u.expr.ti1->get_Template()->is_Value()) {
        Value* v = u.expr.ti1->get_Template()->get_Value();
        delete u.expr.ti1;
        u.expr.ti1 = NULL;
        bool cast_needed = v->explicit_cast_needed();
        if (cast_needed) {
          expr->expr = mputprintf(expr->expr, "%s(", param_governor->get_genname_value(my_scope).c_str());
        }
        v->generate_code_expr(expr);
        if (cast_needed) {
          expr->expr = mputstr(expr->expr, ")");
        }
        delete v;
      } else {
        u.expr.ti1->generate_code(expr);
      }
      expr->expr = mputstr(expr->expr, ")"); 
    } break;
    case OPTYPE_PROF_RUNNING:
      expr->expr = mputstr(expr->expr, "ttcn3_prof.is_running()");
      break;
    default:
      FATAL_ERROR("Value::generate_code_expr_expr()");
    }
  }

  void Value::generate_code_expr_unary(expression_struct *expr,
                                       const char *operator_str, Value *v1)
  {
    expr->expr = mputprintf(expr->expr, "(%s(", operator_str);
    v1->generate_code_expr_mandatory(expr);
    expr->expr = mputstrn(expr->expr, "))", 2);
  }

  void Value::generate_code_expr_infix(expression_struct *expr,
                                       const char *operator_str, Value *v1,
                                       Value *v2, bool optional_allowed)
  {
    if (!get_needs_conversion()) {
      expr->expr = mputc(expr->expr, '(');
      if (optional_allowed) v1->generate_code_expr(expr);
      else v1->generate_code_expr_mandatory(expr);
      expr->expr = mputprintf(expr->expr, " %s ", operator_str);
      if (optional_allowed) v2->generate_code_expr(expr);
      else v2->generate_code_expr_mandatory(expr);
      expr->expr = mputc(expr->expr, ')');
    } else {  // Temporary variable for the converted value.
      const string& tmp_id1 = get_temporary_id();
      const char *tmp_id_str1 = tmp_id1.c_str();
      expression_struct expr_tmp;
      Code::init_expr(&expr_tmp);
      switch (u.expr.v_optype) {
      case OPTYPE_EQ:
      case OPTYPE_NE: {
        // Always "v1 -> v2".
        Type *t1 = v1->get_expr_governor_last();
        Type *t2 = v2->get_expr_governor_last();
        if (t1 == t2) FATAL_ERROR("Value::generate_code_expr_infix()");
        if (optional_allowed) v2->generate_code_expr(&expr_tmp);
        else v2->generate_code_expr_mandatory(&expr_tmp);
        if (expr_tmp.preamble)
          expr->preamble = mputstr(expr->preamble, expr_tmp.preamble);
        expr->preamble = mputprintf(expr->preamble,
          "%s %s;\n"
          "if (!%s(%s, %s)) TTCN_error(\"Values or templates of types `%s' "
          "and `%s' are not compatible at run-time\");\n",
          t1->get_genname_value(v1->get_my_scope()).c_str(), tmp_id_str1,
          TypeConv::get_conv_func(t2, t1, get_my_scope()
          ->get_scope_mod()).c_str(), tmp_id_str1, expr_tmp.expr,
          t2->get_typename().c_str(), t1->get_typename().c_str());
        Code::free_expr(&expr_tmp);
        if (optional_allowed) v1->generate_code_expr(expr);
        else v1->generate_code_expr_mandatory(expr);
        expr->expr = mputprintf(expr->expr, " %s %s", operator_str,
                                tmp_id_str1);
      break; }
      // OPTYPE_{REPLACE,SUBSTR} are handled in their own code generation
      // functions.  The governors of all operands must exist at this point.
      case OPTYPE_ROTL:
      case OPTYPE_ROTR:
      case OPTYPE_CONCAT: {
        const string& tmp_id2 = get_temporary_id();
        const char *tmp_id_str2 = tmp_id2.c_str();
        if (!my_governor) FATAL_ERROR("Value::generate_code_expr_infix()");
        Type *my_gov = my_governor->get_type_refd_last();
        Type *t1_gov = v1->get_expr_governor(Type::EXPECTED_DYNAMIC_VALUE)
          ->get_type_refd_last();
        if (!t1_gov || my_gov == t1_gov)
          FATAL_ERROR("Value::generate_code_expr_infix()");
        expr->preamble = mputprintf(expr->preamble, "%s %s;\n",
          t1_gov->get_genname_value(my_scope).c_str(), tmp_id_str1);
        expr_tmp.expr = mputprintf(expr_tmp.expr, "%s = ", tmp_id_str1);
        if (optional_allowed) v1->generate_code_expr(&expr_tmp);
        else v1->generate_code_expr_mandatory(&expr_tmp);
        expr_tmp.expr = mputprintf(expr_tmp.expr, " %s ", operator_str);
        if (optional_allowed) v2->generate_code_expr(&expr_tmp);
        else v2->generate_code_expr_mandatory(&expr_tmp);
        expr->preamble = Code::merge_free_expr(expr->preamble, &expr_tmp);
        expr->preamble = mputprintf(expr->preamble,
          "%s %s;\n"
          "if (!%s(%s, %s)) TTCN_error(\"Values or templates of types `%s' "
          "and `%s' are not compatible at run-time\");\n",
          my_gov->get_genname_value(my_scope).c_str(), tmp_id_str2,
          TypeConv::get_conv_func(t1_gov, my_gov, get_my_scope()
          ->get_scope_mod()).c_str(), tmp_id_str2, tmp_id_str1,
          my_gov->get_typename().c_str(), t1_gov->get_typename().c_str());
        expr->expr = mputprintf(expr->expr, "%s", tmp_id_str2);
        break; }
      default:
        FATAL_ERROR("Value::generate_code_expr_infix()");
        break;
      }
    }
  }

  void Value::generate_code_expr_and_or(expression_struct *expr)
  {
    if (u.expr.v2->needs_short_circuit()) {
      // introduce a temporary variable to store the result of the operation
      const string& tmp_id = get_temporary_id();
      const char *tmp_id_str = tmp_id.c_str();
      expr->preamble = mputprintf(expr->preamble, "boolean %s;\n", tmp_id_str);
      expression_struct expr2;
      // the left operand must be evaluated anyway
      Code::init_expr(&expr2);
      expr2.expr = mputprintf(expr2.expr, "%s = ", tmp_id_str);
      u.expr.v1->generate_code_expr_mandatory(&expr2);
      expr->preamble = Code::merge_free_expr(expr->preamble, &expr2);
      expr->preamble = mputprintf(expr->preamble, "if (%s%s) ",
	u.expr.v_optype == OPTYPE_AND ? "" : "!", tmp_id_str);
      // evaluate the right operand only when necessary
      // in this case the final result will be the right operand
      Code::init_expr(&expr2);
      expr2.expr = mputprintf(expr2.expr, "%s = ", tmp_id_str);
      u.expr.v2->generate_code_expr_mandatory(&expr2);
      expr->preamble = Code::merge_free_expr(expr->preamble, &expr2);
      // the result is now in the temporary variable
      expr->expr = mputstr(expr->expr, tmp_id_str);
    } else {
      // use the overloaded operator to get better error messages
      generate_code_expr_infix(expr, u.expr.v_optype == OPTYPE_AND ?
	"&&" : "||", u.expr.v1, u.expr.v2, false);
    }
  }

  void Value::generate_code_expr_predef1(expression_struct *expr,
                                         const char *function_name,
                                         Value *v1)
  {
    expr->expr = mputprintf(expr->expr, "%s(", function_name);
    v1->generate_code_expr_mandatory(expr);
    expr->expr = mputc(expr->expr, ')');
  }

  void Value::generate_code_expr_predef2(expression_struct *expr,
                                         const char *function_name,
                                         Value *v1, Value *v2)
  {
    expr->expr = mputprintf(expr->expr, "%s(", function_name);
    v1->generate_code_expr_mandatory(expr);
    expr->expr = mputstr(expr->expr, ", ");
    v2->generate_code_expr_mandatory(expr);
    expr->expr = mputc(expr->expr, ')');
  }

  void Value::generate_code_expr_predef3(expression_struct *expr,
                                         const char *function_name,
                                         Value *v1, Value *v2, Value *v3)
  {
    expr->expr = mputprintf(expr->expr, "%s(", function_name);
    v1->generate_code_expr_mandatory(expr);
    expr->expr = mputstr(expr->expr, ", ");
    v2->generate_code_expr_mandatory(expr);
    expr->expr = mputstr(expr->expr, ", ");
    v3->generate_code_expr_mandatory(expr);
    expr->expr = mputc(expr->expr, ')');
  }

  void Value::generate_code_expr_substr(expression_struct *expr)
  {
    bool par1_is_str;
    Value* v1 = u.expr.ti1->get_specific_value();
    if (v1) par1_is_str = v1->is_string_type(Type::EXPECTED_TEMPLATE);
    else par1_is_str = u.expr.ti1->is_string_type(Type::EXPECTED_TEMPLATE);
    if (par1_is_str) expr->expr = mputstr(expr->expr, "substr(");
    if (v1) v1->generate_code_expr_mandatory(expr);
    else u.expr.ti1->generate_code(expr);
    if (par1_is_str) expr->expr = mputstr(expr->expr, ", ");
    else expr->expr = mputstr(expr->expr, ".substr(");
    if (!par1_is_str && u.expr.v2->is_unfoldable())
      expr->expr = mputstr(expr->expr, "(int)");
    u.expr.v2->generate_code_expr_mandatory(expr);
    expr->expr = mputstr(expr->expr, ", ");
    if (!par1_is_str && u.expr.v3->is_unfoldable())
      expr->expr = mputstr(expr->expr, "(int)");
    u.expr.v3->generate_code_expr_mandatory(expr);
    expr->expr = mputc(expr->expr, ')');
  }

  void Value::generate_code_expr_substr_replace_compat(expression_struct *expr)
  {
    expression_struct expr_tmp;
    Code::init_expr(&expr_tmp);
    Type *t1 = u.expr.ti1->get_expr_governor(Type::EXPECTED_TEMPLATE)
      ->get_type_refd_last();
    if (!t1 || t1 == my_governor->get_type_refd_last())
      FATAL_ERROR("Value::generate_code_expr_substr_replace_compat()");
    if (u.expr.v_optype == OPTYPE_SUBSTR) {
      generate_code_expr_substr(&expr_tmp);
    } else if (u.expr.v_optype == OPTYPE_REPLACE) {
      generate_code_expr_replace(&expr_tmp);
    } else {
      FATAL_ERROR("Value::generate_code_expr_substr_replace_compat()");
    }
    // Two temporaries to store the result of substr() or replace() and to
    // store the converted value.
    const string& tmp_id1 = get_temporary_id();
    const char *tmp_id_str1 = tmp_id1.c_str();
    const string& tmp_id2 = get_temporary_id();
    const char *tmp_id_str2 = tmp_id2.c_str();
    if (expr_tmp.preamble)
      expr->preamble = mputstr(expr->preamble, expr_tmp.preamble);
    expr->preamble = mputprintf(expr->preamble, "%s %s;\n%s %s = %s;\n",
      my_governor->get_genname_value(my_scope).c_str(), tmp_id_str1,
      t1->get_genname_value(my_scope).c_str(), tmp_id_str2, expr_tmp.expr);
    if (expr_tmp.postamble)
      expr->preamble = mputstr(expr->preamble, expr_tmp.postamble);
    Code::free_expr(&expr_tmp);
    expr->preamble = mputprintf(expr->preamble,
      "if (!%s(%s, %s)) TTCN_error(\"Values or templates of types `%s' and "
      "`%s' are not compatible at run-time\");\n",
      TypeConv::get_conv_func(t1, my_governor->get_type_refd_last(),
      my_scope->get_scope_mod()).c_str(), tmp_id_str1, tmp_id_str2,
      my_governor->get_typename().c_str(), t1->get_typename().c_str());
    expr->expr = mputprintf(expr->expr, "%s", tmp_id_str1);
  }

  void Value::generate_code_expr_regexp(expression_struct *expr)
  {
    Value* v1 = u.expr.ti1->get_specific_value();
    Value* v2 = u.expr.t2->get_specific_value();
    expr->expr = mputstr(expr->expr, "regexp(");
    if (v1) v1->generate_code_expr_mandatory(expr);
    else u.expr.ti1->generate_code(expr);
    expr->expr = mputstr(expr->expr, ", ");
    if (v2) v2->generate_code_expr_mandatory(expr);
    else u.expr.t2->generate_code(expr);
    expr->expr = mputstr(expr->expr, ", ");
    u.expr.v3->generate_code_expr_mandatory(expr);
    expr->expr = mputprintf(expr->expr, ", %s)", u.expr.b4 ? "TRUE" : "FALSE");
  }

  void Value::generate_code_expr_replace(expression_struct *expr)
  {
    Value* v1 = u.expr.ti1->get_specific_value();
    Value* v4 = u.expr.ti4->get_specific_value();
    bool par1_is_str;
    if (v1) par1_is_str = v1->is_string_type(Type::EXPECTED_TEMPLATE);
    else par1_is_str = u.expr.ti1->is_string_type(Type::EXPECTED_TEMPLATE);
    if (par1_is_str) expr->expr = mputstr(expr->expr, "replace(");
    if (v1) v1->generate_code_expr_mandatory(expr);
    else u.expr.ti1->generate_code(expr);
    if (par1_is_str) expr->expr = mputstr(expr->expr, ", ");
    else expr->expr = mputstr(expr->expr, ".replace(");
    if (!par1_is_str && u.expr.v2->is_unfoldable())
      expr->expr = mputstr(expr->expr, "(int)");
    u.expr.v2->generate_code_expr_mandatory(expr);
    expr->expr = mputstr(expr->expr, ", ");
    if (!par1_is_str && u.expr.v3->is_unfoldable())
      expr->expr = mputstr(expr->expr, "(int)");
    u.expr.v3->generate_code_expr_mandatory(expr);
    expr->expr = mputstr(expr->expr, ", ");
    if (v4) {
      // if v4 is an empty record of constant (NULL_VALUE), the C++ compiler won't know
      // which replace function to call (replace(int,int,X) or replace(int,int,X_template))
      Value* v4_last = v4->get_value_refd_last();
      if ((v4_last->valuetype == V_SEQOF || v4_last->valuetype == V_SETOF)
          && !v4_last->u.val_vs->is_indexed() && v4_last->u.val_vs->get_nof_vs() == 0) {
        expr->expr = mputprintf(expr->expr, "(%s)", v4->my_governor->get_genname_value(my_scope).c_str());
      }
      v4->generate_code_expr_mandatory(expr);
    }
    else u.expr.ti4->generate_code(expr);
    expr->expr = mputc(expr->expr, ')');
  }

  void Value::generate_code_expr_rnd(expression_struct *expr,
                                     Value *v1)
  {
    if(!v1) // simple random generation
      expr->expr = mputstr(expr->expr, "rnd()");
    else { // random generation with seeding
      expr->expr = mputstr(expr->expr, "rnd(");
      v1->generate_code_expr_mandatory(expr);
      expr->expr = mputc(expr->expr, ')');
    }
  }

  void Value::generate_code_expr_create(expression_struct *expr,
    Ttcn::Ref_base *type, Value *name, Value *location, bool alive)
  {
    expr->expr = mputstr(expr->expr, "TTCN_Runtime::create_component(");
    // first two arguments: component type
    Assignment *t_ass = type->get_refd_assignment();
    if (!t_ass || t_ass->get_asstype() != Assignment::A_TYPE)
      FATAL_ERROR("Value::generate_code_expr_create()");
    Type *comptype = t_ass->get_Type()->get_field_type(type->get_subrefs(),
      Type::EXPECTED_DYNAMIC_VALUE);
    if (!comptype) FATAL_ERROR("Value::generate_code_expr_create()");
    comptype = comptype->get_type_refd_last();
    expr->expr = comptype->get_CompBody()
      ->generate_code_comptype_name(expr->expr);
    expr->expr = mputstr(expr->expr, ", ");
    // third argument: component name
    if (name) {
      Value *t_val = name->get_value_refd_last();
      if (t_val->valuetype == V_CSTR) {
	// the argument is foldable to a string literal
	size_t str_len = t_val->u.str.val_str->size();
	const char *str_ptr = t_val->u.str.val_str->c_str();
	expr->expr = mputc(expr->expr, '"');
	for (size_t i = 0; i < str_len; i++)
	  expr->expr = Code::translate_character(expr->expr, str_ptr[i], true);
	expr->expr = mputc(expr->expr, '"');
      } else name->generate_code_expr_mandatory(expr);
    } else expr->expr = mputstr(expr->expr, "NULL");
    expr->expr = mputstr(expr->expr, ", ");
    // fourth argument: location
    if (location) {
      Value *t_val = location->get_value_refd_last();
      if (t_val->valuetype == V_CSTR) {
	// the argument is foldable to a string literal
	size_t str_len = t_val->u.str.val_str->size();
	const char *str_ptr = t_val->u.str.val_str->c_str();
	expr->expr = mputc(expr->expr, '"');
	for (size_t i = 0; i < str_len; i++)
	  expr->expr = Code::translate_character(expr->expr, str_ptr[i], true);
	expr->expr = mputc(expr->expr, '"');
      } else location->generate_code_expr_mandatory(expr);
    } else expr->expr = mputstr(expr->expr, "NULL");
    // fifth argument: alive flag
    expr->expr = mputprintf(expr->expr, ", %s)", alive ? "TRUE" : "FALSE");
  }

  void Value::generate_code_expr_activate(expression_struct *expr)
  {
    Assignment *t_ass = u.expr.r1->get_refd_assignment();
    if (!t_ass || t_ass->get_asstype() != Assignment::A_ALTSTEP)
      FATAL_ERROR("Value::generate_code_expr_activate()");
    expr->expr = mputprintf(expr->expr, "%s(",
      t_ass->get_genname_from_scope(my_scope, "activate_").c_str());
    u.expr.r1->get_parlist()->generate_code_noalias(expr, t_ass->get_FormalParList());
    expr->expr = mputc(expr->expr, ')');
  }

  void Value::generate_code_expr_activate_refd(expression_struct *expr)
  {
    Value *v_last = u.expr.v1->get_value_refd_last();
    if (v_last->valuetype == V_ALTSTEP) {
      // the referred altstep is known
      expr->expr = mputprintf(expr->expr, "%s(", v_last->get_refd_fat()
	  ->get_genname_from_scope(my_scope, "activate_").c_str());
    } else {
      // the referred altstep is unknown
      u.expr.v1->generate_code_expr_mandatory(expr);
      expr->expr = mputstr(expr->expr,".activate(");
    }
    u.expr.ap_list2->generate_code_noalias(expr, NULL);
    expr->expr = mputc(expr->expr, ')');
  }

  void Value::generate_code_expr_execute(expression_struct *expr)
  {
    Assignment *testcase = u.expr.r1->get_refd_assignment();
    expr->expr = mputprintf(expr->expr, "%s(",
      testcase->get_genname_from_scope(my_scope, "testcase_").c_str());
    Ttcn::ActualParList *parlist = u.expr.r1->get_parlist();
    if (parlist->get_nof_pars() > 0) {
      parlist->generate_code_alias(expr, testcase->get_FormalParList(),
        0, false);
      expr->expr = mputstr(expr->expr, ", ");
    }
    if (u.expr.v2) {
      expr->expr = mputstr(expr->expr, "TRUE, ");
      u.expr.v2->generate_code_expr_mandatory(expr);
      expr->expr = mputc(expr->expr, ')');
    } else expr->expr = mputstr(expr->expr, "FALSE, 0.0)");
  }

  void Value::generate_code_expr_execute_refd(expression_struct *expr)
  {
    Value *v_last = u.expr.v1->get_value_refd_last();
    if (v_last->valuetype == V_TESTCASE) {
      // the referred testcase is known
      Assignment *testcase = v_last->get_refd_fat();
      expr->expr = mputprintf(expr->expr, "%s(",
        testcase->get_genname_from_scope(my_scope, "testcase_").c_str());
      u.expr.ap_list2->generate_code_alias(expr,
	testcase->get_FormalParList(), 0, false);
    } else {
      // the referred testcase is unknown
      u.expr.v1->generate_code_expr_mandatory(expr);
      expr->expr = mputstr(expr->expr,".execute(");
      u.expr.ap_list2->generate_code_alias(expr, 0, 0, false);
    }
    if (u.expr.ap_list2->get_nof_pars() > 0)
      expr->expr = mputstr(expr->expr, ", ");
    if (u.expr.v3) {
      expr->expr = mputstr(expr->expr, "TRUE, ");
      u.expr.v3->generate_code_expr_mandatory(expr);
      expr->expr = mputc(expr->expr, ')');
    } else expr->expr = mputstr(expr->expr, "FALSE, 0.0)");
  }

  void Value::generate_code_expr_invoke(expression_struct *expr)
  {
    Value *last_v = u.invoke.v->get_value_refd_last();
    if (last_v->get_valuetype() == V_FUNCTION) {
      // the referred function is known
      Assignment *function = last_v->get_refd_fat();
      expr->expr = mputprintf(expr->expr, "%s(",
	function->get_genname_from_scope(my_scope).c_str());
      u.invoke.ap_list->generate_code_alias(expr,
	function->get_FormalParList(), function->get_RunsOnType(), false);
    } else {
      // the referred function is unknown
      u.invoke.v->generate_code_expr_mandatory(expr);
      expr->expr = mputstr(expr->expr, ".invoke(");
      Type* gov_last = last_v->get_expr_governor_last();
      u.invoke.ap_list->generate_code_alias(expr, 0,
	gov_last->get_fat_runs_on_type(), gov_last->get_fat_runs_on_self());
    }
    expr->expr = mputc(expr->expr, ')');
  }

  void Value::generate_code_expr_optional_field_ref(expression_struct *expr,
                                                    Reference *ref)
  {
    // if the referenced value points to an optional value field the
    // generated code has to be corrected at the end:
    // `fieldid()' => `fieldid()()'
    Assignment *ass = ref->get_refd_assignment();
    if (!ass) FATAL_ERROR("Value::generate_code_expr_optional_field_ref()");
    switch (ass->get_asstype()) {
    case Assignment::A_CONST:
    case Assignment::A_EXT_CONST:
    case Assignment::A_MODULEPAR:
    case Assignment::A_VAR:
    case Assignment::A_FUNCTION_RVAL:
    case Assignment::A_EXT_FUNCTION_RVAL:
    case Assignment::A_PAR_VAL_IN:
    case Assignment::A_PAR_VAL_OUT:
    case Assignment::A_PAR_VAL_INOUT:
      // only these are mapped to value objects
      if (ass->get_Type()->field_is_optional(ref->get_subrefs()))
	expr->expr = mputstr(expr->expr, "()");
      break;
    default:
      break;
    }
  }

  void Value::generate_code_expr_encode(expression_struct *expr)
  {
    Value* v1 = 0;

    Template* templ = u.expr.ti1->get_Template()->get_template_refd_last();
    if (templ->get_templatetype() == Template::SPECIFIC_VALUE)
      v1 = templ->get_specific_value();
    Type* gov_last = templ->get_my_governor()->get_type_refd_last();

    expression_struct expr2;
    Code::init_expr(&expr2);

    bool is_templ = false;
    switch (templ->get_templatetype()) {
    case Template::SPECIFIC_VALUE:
      v1->generate_code_expr_mandatory(&expr2);
      break;
    default:
      u.expr.ti1->generate_code(&expr2);
      is_templ = true;
      break;
    }

    Scope* scope = u.expr.ti1->get_Template()->get_my_scope();
    if (!gov_last->is_coding_by_function(true)) {
      const string& tmp_id = get_temporary_id();
      const string& tmp_buf_id = get_temporary_id();
      const string& tmp_ref_id = get_temporary_id();
      expr->preamble = mputprintf(expr->preamble, "OCTETSTRING %s;\n",
        tmp_id.c_str());
      expr->preamble = mputprintf(expr->preamble, "TTCN_Buffer %s;\n",
        tmp_buf_id.c_str());
      if (expr2.preamble) { // copy preamble setting up the argument, if any
        expr->preamble = mputstr(expr->preamble, expr2.preamble);
        expr->preamble = mputc  (expr->preamble, '\n');
      }
      expr->preamble = mputprintf(expr->preamble, "%s const& %s = %s",
        gov_last->get_genname_typedescriptor(scope).c_str(),
        tmp_ref_id.c_str(),
        expr2.expr);
      if (is_templ) // make a value out of the template, if needed
        expr->preamble = mputprintf(expr->preamble, ".valueof()");
      expr->preamble = mputprintf(expr->preamble,
        ";\n%s.encode(%s_descr_, %s, TTCN_EncDec::CT_%s",
        tmp_ref_id.c_str(),
        gov_last->get_genname_typedescriptor(scope).c_str(),
        tmp_buf_id.c_str(),
        gov_last->get_coding(true).c_str()
      );
      expr->preamble = mputstr(expr->preamble, ");\n");
      expr->preamble = mputprintf(expr->preamble, "%s.get_string(%s);\n",
        tmp_buf_id.c_str(),
        tmp_id.c_str()
      );
      expr->expr = mputprintf(expr->expr, "oct2bit(%s)", tmp_id.c_str());
      if (expr2.postamble)
        expr->postamble = mputstr(expr->postamble, expr2.postamble);
    } else
      expr->expr = mputprintf(expr->expr, "%s(%s%s)",
        gov_last->get_coding_function(true)->get_genname_from_scope(scope).c_str(),
        expr2.expr, is_templ ? ".valueof()" : "");
    Code::free_expr(&expr2);
  }
  
  void Value::generate_code_expr_decode(expression_struct *expr)
  {
    expression_struct expr1, expr2;
    Code::init_expr(&expr1);
    Code::init_expr(&expr2);
    u.expr.r1->generate_code(&expr1);
    u.expr.r2->generate_code(&expr2);

    Type* _type = u.expr.r2->get_refd_assignment()->get_Type()->
      get_field_type(u.expr.r2->get_subrefs(), Type::EXPECTED_DYNAMIC_VALUE)->
      get_type_refd_last();

    if (expr1.preamble)
      expr->preamble = mputprintf(expr->preamble, "%s", expr1.preamble);
    if (expr2.preamble)
      expr->preamble = mputprintf(expr->preamble, "%s", expr2.preamble);

    Scope* scope = u.expr.r2->get_my_scope();
    if (!_type->is_coding_by_function(false)) {
      const string& tmp_id = get_temporary_id();
      const string& buffer_id = get_temporary_id();
      const string& retval_id = get_temporary_id();
      const bool optional = u.expr.r2->get_refd_assignment()->get_Type()->
        field_is_optional(u.expr.r2->get_subrefs());

      expr->preamble = mputprintf(expr->preamble,
        "TTCN_Buffer %s(bit2oct(%s));\n"
        "INTEGER %s;\n"
        "TTCN_EncDec::set_error_behavior("
        "TTCN_EncDec::ET_ALL, TTCN_EncDec::EB_WARNING);\n"
        "TTCN_EncDec::clear_error();\n",
        buffer_id.c_str(),
        expr1.expr,
        retval_id.c_str()
      );
      expr->preamble = mputprintf(expr->preamble,
        "%s%s.decode(%s_descr_, %s, TTCN_EncDec::CT_%s);\n",
        expr2.expr,
        optional ? "()" : "",
          _type->get_genname_typedescriptor(scope).c_str(),
          buffer_id.c_str(),
          _type->get_coding(false).c_str()
      );
      expr->preamble = mputprintf(expr->preamble,
        "switch (TTCN_EncDec::get_last_error_type()) {\n"
        "case TTCN_EncDec::ET_NONE: {\n"
        "%s.cut();\n"
        "OCTETSTRING %s;\n"
        "%s.get_string(%s);\n"
        "%s = oct2bit(%s);\n"
        "%s = 0;\n"
        "}break;\n"
        "case TTCN_EncDec::ET_INCOMPL_MSG:\n"
        "case TTCN_EncDec::ET_LEN_ERR:\n"
        "%s = 2;\n"
        "break;\n"
        "default:\n"
        "%s = 1;\n"
        "}\n"
        "TTCN_EncDec::set_error_behavior(TTCN_EncDec::ET_ALL,"
        "TTCN_EncDec::EB_DEFAULT);\n"
        "TTCN_EncDec::clear_error();\n",
        buffer_id.c_str(),
        tmp_id.c_str(),
        buffer_id.c_str(),
        tmp_id.c_str(),
        expr1.expr,
        tmp_id.c_str(),
        retval_id.c_str(),
        retval_id.c_str(),
        retval_id.c_str()
      );
      expr->expr = mputprintf(expr->expr, "%s", retval_id.c_str());
    } else
      expr->expr = mputprintf(expr->expr, "%s(%s, %s)",
        _type->get_coding_function(false)->get_genname_from_scope(scope).c_str(), expr1.expr, expr2.expr);
    if (expr1.postamble)
      expr->postamble = mputprintf(expr->postamble, "%s", expr1.postamble);
    if (expr2.postamble)
      expr->postamble = mputprintf(expr->postamble, "%s", expr2.postamble);
    Code::free_expr(&expr1);
    Code::free_expr(&expr2);
  }
  
void Value::generate_code_expr_encvalue_unichar(expression_struct *expr)
  {
    Value* v1 = 0;

    Template* templ = u.expr.ti1->get_Template()->get_template_refd_last();
    if (templ->get_templatetype() == Template::SPECIFIC_VALUE)
      v1 = templ->get_specific_value();
    Type* gov_last = templ->get_my_governor()->get_type_refd_last();

    expression_struct expr2;
    Code::init_expr(&expr2);

    bool is_templ = false;
    switch (templ->get_templatetype()) {
    case Template::SPECIFIC_VALUE:
      v1->generate_code_expr_mandatory(&expr2);
      break;
    default:
      u.expr.ti1->generate_code(&expr2);
      is_templ = true;
      break;
    }
    
    char * v2_code = NULL;
    if(u.expr.v2) {
      v2_code = generate_code_char_coding_check(expr, u.expr.v2, "encvalue_unichar");
    }

    Scope* scope = u.expr.ti1->get_Template()->get_my_scope();
    if (!gov_last->is_coding_by_function(true)) {
      const string& tmp_id = get_temporary_id();
      const string& tmp_buf_id = get_temporary_id();
      const string& tmp_ref_id = get_temporary_id();
      expr->preamble = mputprintf(expr->preamble, "OCTETSTRING %s;\n",
        tmp_id.c_str());
      expr->preamble = mputprintf(expr->preamble, "TTCN_Buffer %s;\n",
        tmp_buf_id.c_str());
      if (expr2.preamble) { // copy preamble setting up the argument, if any
        expr->preamble = mputstr(expr->preamble, expr2.preamble);
        expr->preamble = mputc  (expr->preamble, '\n');
      }
      expr->preamble = mputprintf(expr->preamble, "%s const& %s = %s",
        gov_last->get_genname_typedescriptor(scope).c_str(),
        tmp_ref_id.c_str(),
        expr2.expr);
      if (is_templ) // make a value out of the template, if needed
        expr->preamble = mputprintf(expr->preamble, ".valueof()");
      expr->preamble = mputprintf(expr->preamble,
        ";\n%s.encode(%s_descr_, %s, TTCN_EncDec::CT_%s",
        tmp_ref_id.c_str(),
        gov_last->get_genname_typedescriptor(scope).c_str(),
        tmp_buf_id.c_str(),
        gov_last->get_coding(true).c_str()
      );
      expr->preamble = mputstr(expr->preamble, ");\n");
      expr->preamble = mputprintf(expr->preamble, "%s.get_string(%s);\n",
        tmp_buf_id.c_str(),
        tmp_id.c_str()
      );
      expr->expr = mputprintf(expr->expr, "oct2unichar(%s", tmp_id.c_str());
      if(u.expr.v2) {
        expr->expr = mputprintf(expr->expr, ", %s", v2_code);
      } else {
        expr->expr = mputprintf(expr->expr, ", \"UTF-8\"");  //default
      }
      expr->expr = mputprintf(expr->expr, ")");
      if (expr2.postamble)
        expr->postamble = mputstr(expr->postamble, expr2.postamble);
    } else {
      expr->expr = mputprintf(expr->expr, "oct2unichar(bit2oct(%s(%s%s))",
        gov_last->get_coding_function(true)->get_genname_from_scope(scope).c_str(),
        expr2.expr, is_templ ? ".valueof()" : "");
      if(u.expr.v2) {
        expr->expr = mputprintf(expr->expr, ", %s", v2_code);
      } else {
        expr->expr = mputprintf(expr->expr, ", \"UTF-8\"");  //default
      }
      expr->expr = mputprintf(expr->expr, ")");
    }
    Code::free_expr(&expr2);
    Free(v2_code);
  }

  void Value::generate_code_expr_decvalue_unichar(expression_struct *expr)
  {
    expression_struct expr1, expr2;
    Code::init_expr(&expr1);
    Code::init_expr(&expr2);
    u.expr.r1->generate_code(&expr1);
    u.expr.r2->generate_code(&expr2);

    Type* _type = u.expr.r2->get_refd_assignment()->get_Type()->
      get_field_type(u.expr.r2->get_subrefs(), Type::EXPECTED_DYNAMIC_VALUE)->
      get_type_refd_last();

    if (expr1.preamble)
      expr->preamble = mputprintf(expr->preamble, "%s", expr1.preamble);
    if (expr2.preamble)
      expr->preamble = mputprintf(expr->preamble, "%s", expr2.preamble);
    char* v3_code = NULL;
    if(u.expr.v3) {
      v3_code = generate_code_char_coding_check(expr, u.expr.v3, "decvalue_unichar");
    }

    Scope* scope = u.expr.r2->get_my_scope();
    if (!_type->is_coding_by_function(false)) {
      const string& tmp_id = get_temporary_id();
      const string& buffer_id = get_temporary_id();
      const string& retval_id = get_temporary_id();
      const bool optional = u.expr.r2->get_refd_assignment()->get_Type()->
        field_is_optional(u.expr.r2->get_subrefs());

      expr->preamble = mputprintf(expr->preamble,
        "TTCN_Buffer %s(unichar2oct(%s, %s));\n"
        "INTEGER %s;\n"
        "TTCN_EncDec::set_error_behavior("
        "TTCN_EncDec::ET_ALL, TTCN_EncDec::EB_WARNING);\n"
        "TTCN_EncDec::clear_error();\n",
        buffer_id.c_str(),
        expr1.expr,
        u.expr.v3 ? v3_code : "\"UTF-8\"",
        retval_id.c_str()
      );
      expr->preamble = mputprintf(expr->preamble,
        "%s%s.decode(%s_descr_, %s, TTCN_EncDec::CT_%s);\n",
        expr2.expr,
        optional ? "()" : "",
          _type->get_genname_typedescriptor(scope).c_str(),
          buffer_id.c_str(),
          _type->get_coding(false).c_str()
      );
      expr->preamble = mputprintf(expr->preamble,
        "switch (TTCN_EncDec::get_last_error_type()) {\n"
        "case TTCN_EncDec::ET_NONE: {\n"
        "%s.cut();\n"
        "OCTETSTRING %s;\n"
        "%s.get_string(%s);\n"
        "%s = oct2unichar(%s, %s);\n"
        "%s = 0;\n"
        "}break;\n"
        "case TTCN_EncDec::ET_INCOMPL_MSG:\n"
        "case TTCN_EncDec::ET_LEN_ERR:\n"
        "%s = 2;\n"
        "break;\n"
        "default:\n"
        "%s = 1;\n"
        "}\n"
        "TTCN_EncDec::set_error_behavior(TTCN_EncDec::ET_ALL,"
        "TTCN_EncDec::EB_DEFAULT);\n"
        "TTCN_EncDec::clear_error();\n",
        buffer_id.c_str(),
        tmp_id.c_str(),
        buffer_id.c_str(),
        tmp_id.c_str(),
        expr1.expr,
        tmp_id.c_str(),
        u.expr.v3 ? v3_code : "\"UTF-8\"",
        retval_id.c_str(),
        retval_id.c_str(),
        retval_id.c_str()
      );
      expr->expr = mputprintf(expr->expr, "%s", retval_id.c_str());
    } else {
      const string& ustr_ref_id = get_temporary_id();
      const string& bstr_id = get_temporary_id();
      const string& ret_val_id = get_temporary_id();
      expr->preamble = mputprintf(expr->preamble,
        "UNIVERSAL_CHARSTRING& %s = %s;\n"
        "BITSTRING %s(oct2bit(unichar2oct(%s, %s)));\n"
        "INTEGER %s(%s(%s, %s));\n"
        "%s = oct2unichar(bit2oct(%s), %s);\n",
        ustr_ref_id.c_str(), expr1.expr,
        bstr_id.c_str(), ustr_ref_id.c_str(), u.expr.v3 ? v3_code : "\"UTF-8\"",
        ret_val_id.c_str(),
        _type->get_coding_function(false)->get_genname_from_scope(scope).c_str(),
        bstr_id.c_str(), expr2.expr,
        ustr_ref_id.c_str(), bstr_id.c_str(), u.expr.v3 ? v3_code : "\"UTF-8\"");
      expr->expr = mputprintf(expr->expr, "%s", ret_val_id.c_str());
    }
    if (expr1.postamble)
      expr->postamble = mputprintf(expr->postamble, "%s", expr1.postamble);
    if (expr2.postamble)
      expr->postamble = mputprintf(expr->postamble, "%s", expr2.postamble);
    Code::free_expr(&expr1);
    Code::free_expr(&expr2);
    Free(v3_code);
  }
  
  void Value::generate_code_expr_checkstate(expression_struct *expr)
  {
    if (u.expr.r1) { 
      // It is a port if r1 is not null
      u.expr.r1->generate_code_const_ref(expr);
      expr->expr = mputstr(expr->expr, ".");
    } else {
      // it is an any or all port if r1 is null
      if (u.expr.v_optype == OPTYPE_CHECKSTATE_ANY) {
       expr->expr = mputstr(expr->expr, "PORT::any_");
      } else if (u.expr.v_optype == OPTYPE_CHECKSTATE_ALL) {
        expr->expr = mputstr(expr->expr, "PORT::all_");
      } else {
        FATAL_ERROR("Value::generate_code_expr_checkstate()");
      }
    }
    expr->expr = mputstr(expr->expr, "check_port_state(");
    u.expr.v2->generate_code_expr_mandatory(expr);
    expr->expr = mputstr(expr->expr, ")");
  }
  
  void Value::generate_code_expr_hostid(expression_struct *expr)
  {
    expr->expr = mputstr(expr->expr, "TTCN_Runtime::get_host_address(");
    if (u.expr.v1) u.expr.v1->generate_code_expr_mandatory(expr);
    else expr->expr = mputstr(expr->expr, "CHARSTRING(\"Ipv4orIpv6\")");
    expr->expr = mputstr(expr->expr, ")");
  }
  
  char* Value::generate_code_char_coding_check(expression_struct *expr, Value *v, const char *name)
  {
    expression_struct expr2;
    Code::init_expr(&expr2);
    v->generate_code_expr_mandatory(&expr2);
    expr->preamble = mputprintf(expr->preamble,
      "if (\"UTF-8\" != %s && \"UTF-16\" != %s && \"UTF-16LE\" != %s && \n"
      "  \"UTF-16BE\" != %s && \"UTF-32\" != %s && \"UTF-32LE\" != %s && \n"
      "  \"UTF-32BE\" != %s) {\n"
      "   TTCN_error(\"%s: Invalid encoding parameter: %%s\", (const char*)%s);\n"
      "}\n", //todo errorbehaviour?
      expr2.expr,
      expr2.expr,
      expr2.expr,
      expr2.expr,
      expr2.expr,
      expr2.expr,
      expr2.expr,
      name,
      expr2.expr);
    return expr2.expr;
  }

  char *Value::generate_code_init_choice(char *str, const char *name)
  {
    const char *alt_name = u.choice.alt_name->get_name().c_str();
    // Safe as long as get_name() returns a const string&, not a temporary.
    const char *alt_prefix =
      (my_governor->get_type_refd_last()->get_typetype()==Type::T_ANYTYPE)
      ? "AT_" : "";
    if (u.choice.alt_value->needs_temp_ref()) {
      const string& tmp_id = get_temporary_id();
      const char *tmp_id_str = tmp_id.c_str();
      str = mputprintf(str, "{\n"
	"%s& %s = %s.%s%s();\n", my_governor->get_comp_byName(*u.choice.alt_name)
	  ->get_type()->get_genname_value(my_scope).c_str(), tmp_id_str, name,
	alt_prefix, alt_name);
      str = u.choice.alt_value->generate_code_init(str, tmp_id_str);
      str = mputstr(str, "}\n");
    } else {
      char *embedded_name = mprintf("%s.%s%s()", name, alt_prefix, alt_name);
      str = u.choice.alt_value->generate_code_init(str, embedded_name);
      Free(embedded_name);
    }
    return str;
  }

  char *Value::generate_code_init_seof(char *str, const char *name)
  {
    size_t nof_vs = u.val_vs->get_nof_vs();
    if (nof_vs > 0) {
      str = mputprintf(str, "%s.set_size(%lu);\n", name, (unsigned long)nof_vs);
      const string& embedded_type =
	my_governor->get_ofType()->get_genname_value(my_scope);
      const char *embedded_type_str = embedded_type.c_str();
      for (size_t i = 0; i < nof_vs; i++) {
	Value *comp_v = u.val_vs->get_v_byIndex(i);

	if (comp_v->valuetype == V_NOTUSED) continue;
	else if (comp_v->needs_temp_ref()) {
          const string& tmp_id = get_temporary_id();
	  const char *tmp_id_str = tmp_id.c_str();
	  str = mputprintf(str, "{\n"
	    "%s& %s = %s[%lu];\n", embedded_type_str, tmp_id_str, name,
            (unsigned long) i);
	  str = comp_v->generate_code_init(str, tmp_id_str);
	  str = mputstr(str, "}\n");
	} else {
	  char *embedded_name = mprintf("%s[%lu]", name, (unsigned long) i);
	  str = comp_v->generate_code_init(str, embedded_name);
	  Free(embedded_name);
	}
      }
    } else {
      str = mputprintf(str, "%s = NULL_VALUE;\n", name);
    }
    return str;
  }

  char *Value::generate_code_init_indexed(char *str, const char *name)
  {
    size_t nof_ivs = u.val_vs->get_nof_ivs();
    if (nof_ivs > 0) {
      // Previous values can be truncated.  The concept is similar to
      // templates.
      Type *t_last = my_governor->get_type_refd_last();
      const string& oftype_name =
        t_last->get_ofType()->get_genname_value(my_scope);
      const char *oftype_name_str = oftype_name.c_str();
      for (size_t i = 0; i < nof_ivs; i++)  {
        IndexedValue *iv = u.val_vs->get_iv_byIndex(i);
        const string& tmp_id_1 = get_temporary_id();
        str = mputstr(str, "{\n");
        Value *index = iv->get_index();
        if (index->get_valuetype() != V_INT) {
          const string& tmp_id_2 = get_temporary_id();
          str = mputprintf(str, "int %s;\n", tmp_id_2.c_str());
          str = index->generate_code_init(str, tmp_id_2.c_str());
          str = mputprintf(str, "%s& %s = %s[%s];\n", oftype_name_str,
                           tmp_id_1.c_str(), name, tmp_id_2.c_str());
        } else {
          str = mputprintf(str, "%s& %s = %s[%s];\n", oftype_name_str,
                           tmp_id_1.c_str(), name,
                           (index->get_val_Int()->t_str()).c_str());
        }
        str = iv->get_value()->generate_code_init(str, tmp_id_1.c_str());
        str = mputstr(str, "}\n");
      }
    } else { str = mputprintf(str, "%s = NULL_VALUE;\n", name); }
    return str;
  }

  char *Value::generate_code_init_array(char *str, const char *name)
  {
    size_t nof_vs = u.val_vs->get_nof_vs();
    Type *t_last = my_governor->get_type_refd_last();
    Int index_offset = t_last->get_dimension()->get_offset();
    const string& embedded_type =
      t_last->get_ofType()->get_genname_value(my_scope);
    const char *embedded_type_str = embedded_type.c_str();
    for (size_t i = 0; i < nof_vs; i++) {
      Value *comp_v = u.val_vs->get_v_byIndex(i);
      if (comp_v->valuetype == V_NOTUSED) continue;
      else if (comp_v->needs_temp_ref()) {
        const string& tmp_id = get_temporary_id();
	const char *tmp_id_str = tmp_id.c_str();
	str = mputprintf(str, "{\n"
	  "%s& %s = %s[%s];\n", embedded_type_str, tmp_id_str, name,
	  Int2string(index_offset + i).c_str());
	str = comp_v->generate_code_init(str, tmp_id_str);
	str = mputstr(str, "}\n");
      } else {
	char *embedded_name = mprintf("%s[%s]", name,
	  Int2string(index_offset + i).c_str());
	str = comp_v->generate_code_init(str, embedded_name);
	Free(embedded_name);
      }
    }
    return str;
  }

  char *Value::generate_code_init_se(char *str, const char *name)
  {
    Type *type = my_governor->get_type_refd_last();
    size_t nof_comps = type->get_nof_comps();
    if (nof_comps > 0) {
      for (size_t i = 0; i < nof_comps; i++) {
	CompField *cf = type->get_comp_byIndex(i);
	const Identifier& field_id = cf->get_name();
	const char *field_name = field_id.get_name().c_str();
	Value *field_v;
	if (u.val_nvs->has_nv_withName(field_id)) {
	  field_v = u.val_nvs->get_nv_byName(field_id)->get_value();
          if (field_v->valuetype == V_NOTUSED) continue;
	  if (field_v->valuetype == V_OMIT) field_v = 0;
	} else if (is_asn1()) {
	  if (cf->has_default()) {
	    // handle like a referenced value
	    Value *defval = cf->get_defval();
	    if (needs_init_precede(defval)) {
	      str = defval->generate_code_init(str,
	        defval->get_lhs_name().c_str());
	    }
	    str = mputprintf(str, "%s.%s() = %s;\n", name, field_name,
	      defval->get_genname_own(my_scope).c_str());
	    continue;
	  } else {
            if (!cf->get_is_optional())
	      FATAL_ERROR("Value::generate_code_init()");
            field_v = 0;
          }
        } else {
          continue;
        }
	if (field_v) {
	  // the value is not omit
	  if (field_v->needs_temp_ref()) {
	    const string& tmp_id = get_temporary_id();
	    const char *tmp_id_str = tmp_id.c_str();
	    str = mputprintf(str, "{\n"
	      "%s& %s = %s.%s();\n", type->get_comp_byName(field_id)->get_type()
		->get_genname_value(my_scope).c_str(), tmp_id_str, name,
	      field_name);
	    str = field_v->generate_code_init(str, tmp_id_str);
	    str = mputstr(str, "}\n");
	  } else {
	    char *embedded_name = mprintf("%s.%s()", name,
	      field_name);
	    if (cf->get_is_optional() && field_v->is_compound())
	      embedded_name = mputstr(embedded_name, "()");
	    str = field_v->generate_code_init(str, embedded_name);
	    Free(embedded_name);
	  }
	} else {
	  // the value is omit
	  str = mputprintf(str, "%s.%s() = OMIT_VALUE;\n",
	    name, field_name);
	}
      }
    } else {
      str = mputprintf(str, "%s = NULL_VALUE;\n", name);
    }
    return str;
  }

  char *Value::generate_code_init_refd(char *str, const char *name)
  {
    Value *v = get_value_refd_last();
    if (v == this) {
      // the referred value is not available at compile time
      // the code generation is based on the reference
      if (use_runtime_2 && TypeConv::needs_conv_refd(v)) {
        str = TypeConv::gen_conv_code_refd(str, name, v);
      } else {
        expression_struct expr;
        Code::init_expr(&expr);
        expr.expr = mputprintf(expr.expr, "%s = ", name);
        u.ref.ref->generate_code_const_ref(&expr);
        str = Code::merge_free_expr(str, &expr);
      }
    } else {
      // the referred value is available at compile time
      // the code generation is based on the referred value
      if (v->has_single_expr() &&
        my_scope->get_scope_mod_gen() == v->my_scope->get_scope_mod_gen()) {
	// simple substitution for in-line values within the same module
	str = mputprintf(str, "%s = %s;\n", name,
	  v->get_single_expr().c_str());
      } else {
        // use a simple reference to reduce code size
	if (needs_init_precede(v)) {
	  // the referred value must be initialized first
	  if (!v->is_toplevel() && v->needs_temp_ref()) {
	    // temporary id should be introduced for the lhs
	    const string& tmp_id = get_temporary_id();
	    const char *tmp_id_str = tmp_id.c_str();
	    str = mputprintf(str, "{\n"
	      "%s& %s = %s;\n",
	      v->get_my_governor()->get_genname_value(my_scope).c_str(),
	      tmp_id_str, v->get_lhs_name().c_str());
	    str = v->generate_code_init(str, tmp_id_str);
	    str = mputstr(str, "}\n");
	  } else {
	    str = v->generate_code_init(str, v->get_lhs_name().c_str());
	  }
	}
	str = mputprintf(str, "%s = %s;\n", name,
	  v->get_genname_own(my_scope).c_str());
      }
    }
    return str;
  }
  
  void Value::generate_json_value(JSON_Tokenizer& json,
                                  bool allow_special_float, /* = true */
                                  bool union_value_list, /* = false */
                                  Ttcn::JsonOmitCombination* omit_combo /* = NULL */)
  {
    switch (valuetype) {
    case V_INT:
      json.put_next_token(JSON_TOKEN_NUMBER, get_val_Int()->t_str().c_str());
      break;
    case V_REAL: {
      Real r = get_val_Real();
      if (r == REAL_INFINITY) {
        if (allow_special_float) {
          json.put_next_token(JSON_TOKEN_STRING, "\"infinity\"");
        }
      }
      else if (r == -REAL_INFINITY) {
        if (allow_special_float) {
          json.put_next_token(JSON_TOKEN_STRING, "\"-infinity\"");
        }
      }
      else if (r != r) {
        if (allow_special_float) {
          json.put_next_token(JSON_TOKEN_STRING, "\"not_a_number\"");
        }
      }
      else {
        // true if decimal representation possible (use %f format)
        bool decimal_repr = (r == 0.0)
          || (r > -MAX_DECIMAL_FLOAT && r <= -MIN_DECIMAL_FLOAT)
          || (r >= MIN_DECIMAL_FLOAT && r < MAX_DECIMAL_FLOAT);
        char* number_str = mprintf(decimal_repr ? "%f" : "%e", r);
        json.put_next_token(JSON_TOKEN_NUMBER, number_str);
        Free(number_str);
      }
      break; }
    case V_BOOL:
      json.put_next_token(get_val_bool() ? JSON_TOKEN_LITERAL_TRUE : JSON_TOKEN_LITERAL_FALSE);
      break;
    case V_BSTR:
    case V_HSTR:
    case V_OSTR:
    case V_CSTR: {
      char* str = convert_to_json_string(get_val_str().c_str());
      json.put_next_token(JSON_TOKEN_STRING, str);
      Free(str);
      break; }
    case V_USTR: {
      char* str = convert_to_json_string(ustring_to_uft8(get_val_ustr()).c_str());
      json.put_next_token(JSON_TOKEN_STRING, str);
      Free(str);
      break; }
    case V_VERDICT:
    case V_ENUM:
      json.put_next_token(JSON_TOKEN_STRING,
        (string('\"') + create_stringRepr() + string('\"')).c_str());
      break;
    case V_SEQOF:
    case V_SETOF:
      json.put_next_token(JSON_TOKEN_ARRAY_START);
      if (!u.val_vs->is_indexed()) {
        for (size_t i = 0; i < u.val_vs->get_nof_vs(); ++i) {
          u.val_vs->get_v_byIndex(i)->generate_json_value(json, allow_special_float,
            union_value_list, omit_combo);
        }
      }
      else {
        for (size_t i = 0; i < u.val_vs->get_nof_ivs(); ++i) {
          // look for the entry with index equal to i
          for (size_t j = 0; j < u.val_vs->get_nof_ivs(); ++j) {
            if (u.val_vs->get_iv_byIndex(j)->get_index()->get_val_Int()->get_val() == (Int)i) {
              u.val_vs->get_iv_byIndex(j)->get_value()->generate_json_value(json,
                allow_special_float, union_value_list, omit_combo);
              break;
            }
          }
        }
      }
      json.put_next_token(JSON_TOKEN_ARRAY_END);
      break;
    case V_SEQ:
    case V_SET: {
      // omitted fields have 2 possible JSON values (the field is absent, or it's
      // present with value 'null'), each combination of omitted values must be
      // generated
      if (omit_combo == NULL) {
        FATAL_ERROR("Value::generate_json_value - no combo");
      }
      size_t len = get_nof_comps();
      // generate the JSON object from the present combination
      json.put_next_token(JSON_TOKEN_OBJECT_START);
      for (size_t i = 0; i < len; ++i) {
        Ttcn::JsonOmitCombination::omit_state_t state = omit_combo->get_state(this, i);
        if (state == Ttcn::JsonOmitCombination::OMITTED_ABSENT) {
          // the field is absent, don't insert anything
          continue;
        }
        // use the field's alias, if it has one
        const char* alias = NULL;
        if (my_governor != NULL) {
          JsonAST* field_attrib = my_governor->get_comp_byName(
            get_se_comp_byIndex(i)->get_name())->get_type()->get_json_attributes();
          if (field_attrib != NULL) {
            alias = field_attrib->alias;
          }
        }
        json.put_next_token(JSON_TOKEN_NAME, (alias != NULL) ? alias :
          get_se_comp_byIndex(i)->get_name().get_ttcnname().c_str());
        if (state == Ttcn::JsonOmitCombination::OMITTED_NULL) {
          json.put_next_token(JSON_TOKEN_LITERAL_NULL);
        }
        else {
          get_se_comp_byIndex(i)->get_value()->generate_json_value(json,
            allow_special_float, union_value_list, omit_combo);
        }
      }
      json.put_next_token(JSON_TOKEN_OBJECT_END);
      break; }
    case V_CHOICE: {
      bool as_value = !union_value_list && my_governor != NULL && 
        my_governor->get_type_refd_last()->get_json_attributes() != NULL &&
        my_governor->get_type_refd_last()->get_json_attributes()->as_value;
      if (!as_value) {
        // no 'as value' coding instruction, insert an object with one field
        json.put_next_token(JSON_TOKEN_OBJECT_START);
        // use the field's alias, if it has one
        const char* alias = NULL;
        if (my_governor != NULL) {
          JsonAST* field_attrib = my_governor->get_comp_byName(
            get_alt_name())->get_type()->get_json_attributes();
          if (field_attrib != NULL) {
            alias = field_attrib->alias;
          }
        }
        json.put_next_token(JSON_TOKEN_NAME, (alias != NULL) ? alias :
          get_alt_name().get_ttcnname().c_str());
      }
      get_alt_value()->generate_json_value(json, allow_special_float,
        union_value_list, omit_combo);
      if (!as_value) {
        json.put_next_token(JSON_TOKEN_OBJECT_END);
      }
      break; }
    case V_REFD: {
      Value* v = get_value_refd_last();
      if (this != v) {
        v->generate_json_value(json, allow_special_float, union_value_list, omit_combo);
        return;
      }
    } // no break
    default:
      FATAL_ERROR("Value::generate_json_value - %d", valuetype);
    }
  }

  bool Value::explicit_cast_needed(bool forIsValue)
  {
    Value *v_last = get_value_refd_last();
    if (v_last != this) {
      // this is a foldable referenced value
      // if the reference points to an imported or compound value the code
      // generation will be based on the reference so cast is not needed
      if (v_last->my_scope->get_scope_mod_gen() != my_scope->get_scope_mod_gen()
          || !v_last->has_single_expr()) return false;
    } else if (v_last->valuetype == V_REFD) {
      // this is an unfoldable reference (v_last==this)
      // explicit cast is needed only for string element references
      if (forIsValue) return false;
      Ttcn::FieldOrArrayRefs *t_subrefs = v_last->u.ref.ref->get_subrefs();
      return t_subrefs && t_subrefs->refers_to_string_element();
    }
    if (!v_last->my_governor) FATAL_ERROR("Value::explicit_cast_needed()");
    Type *t_governor = v_last->my_governor->get_type_refd_last();
    switch (t_governor->get_typetype()) {
    case Type::T_NULL:
    case Type::T_BOOL:
    case Type::T_INT:
    case Type::T_INT_A:
    case Type::T_REAL:
    case Type::T_ENUM_A:
    case Type::T_ENUM_T:
    case Type::T_VERDICT:
    case Type::T_COMPONENT:
      // these are mapped to built-in C/C++ types
      return true;
    case Type::T_SEQ_A:
    case Type::T_SEQ_T:
    case Type::T_SET_A:
    case Type::T_SET_T:
      // the C++ equivalent of empty record/set value (i.e. {}) is ambiguous
      return t_governor->get_nof_comps() == 0;
    case Type::T_SEQOF:
    case Type::T_SETOF:
      // the C++ equivalent of value {} is ambiguous
      // tr926
      return true;
    case Type::T_FUNCTION:
    case Type::T_ALTSTEP:
    case Type::T_TESTCASE:
      return true;
    default:
      return false;
    }
  }

  bool Value::has_single_expr()
  {
    if (get_needs_conversion()) return false;
    switch (valuetype) {
    case V_EXPR:
      return has_single_expr_expr();
    case V_CHOICE:
    case V_ARRAY:
      // a union or array value cannot be represented as an in-line expression
      return false;
    case V_SEQOF:
    case V_SETOF:
      // only an empty record/set of value can be represented as an in-line
      // expression
      if (!is_indexed()) return u.val_vs->get_nof_vs() == 0;
      else return u.val_vs->get_nof_ivs() == 0;
    case V_SEQ:
    case V_SET: {
      // only a value for an empty record/set type can be represented as an
      // in-line expression
      if (!my_governor) FATAL_ERROR("Value::has_single_expr()");
      Type *type = my_governor->get_type_refd_last();
      return type->get_nof_comps() == 0; }
    case V_REFD: {
      Value *v_last = get_value_refd_last();
      // If the above call hit an error and set_valuetype(V_ERROR),
      // then u.ref.ref has been freed. Avoid the segfault.
      if (valuetype == V_ERROR)
	return false;
      if (v_last != this && v_last->has_single_expr() &&
	  v_last->my_scope->get_scope_mod_gen() ==
	  my_scope->get_scope_mod_gen()) return true;
      else return u.ref.ref->has_single_expr(); }
    case V_INVOKE:
      return has_single_expr_invoke(u.invoke.v, u.invoke.ap_list);
    case V_ERROR:
    case V_NAMEDINT:
    case V_NAMEDBITS:
    case V_UNDEF_LOWERID:
    case V_UNDEF_BLOCK:
    case V_REFER:
      // these values cannot occur during code generation
      FATAL_ERROR("Value::has_single_expr()");
    case V_INT:
      return u.val_Int->is_native_fit();
    case V_NOTUSED:
      // should only happen when generating code for an unbound record/set value
      return false;
    default:
      // other value types (literal values) do not need temporary reference
      return true;
    }
  }

  string Value::get_single_expr()
  {
    switch (valuetype) {
    case V_NULL:
      return string("ASN_NULL_VALUE");
    case V_BOOL:
      return string(u.val_bool ? "TRUE" : "FALSE");
    case V_INT:
      if (u.val_Int->is_native_fit()) {  // Be sure.
        return u.val_Int->t_str();
      } else {
        // get_single_expr may be called only if has_single_expr() is true.
        // The only exception is V_INT, where get_single_expr may be called
        // even if is_native_fit (which is used to implement has_single_expr)
        // returns false.
        string ret_val('"');
        ret_val += u.val_Int->t_str();
        ret_val += '"';
        return ret_val;
      }
    case V_REAL:
      return Real2code(u.val_Real);
    case V_ENUM:
      return get_single_expr_enum();
    case V_BSTR:
      return get_my_scope()->get_scope_mod_gen()
        ->add_bitstring_literal(*u.str.val_str);
    case V_HSTR:
      return get_my_scope()->get_scope_mod_gen()
	->add_hexstring_literal(*u.str.val_str);
    case V_OSTR:
      return get_my_scope()->get_scope_mod_gen()
	->add_octetstring_literal(*u.str.val_str);
    case V_CSTR:
      return get_my_scope()->get_scope_mod_gen()
	->add_charstring_literal(*u.str.val_str);
    case V_USTR:
      if (u.ustr.convert_str) {
        set_valuetype(V_CSTR);
        return get_my_scope()->get_scope_mod_gen()
          ->add_charstring_literal(*u.str.val_str);
      } else
        return get_my_scope()->get_scope_mod_gen()
	  ->add_ustring_literal(*u.ustr.val_ustr);
    case V_ISO2022STR:
      return get_single_expr_iso2022str();
    case V_OID:
    case V_ROID: {
      vector<string> comps;
      bool is_constant = get_oid_comps(comps);
      size_t nof_comps = comps.size();
      string oi_str;
      for (size_t i = 0; i < nof_comps; i++) {
        if (i > 0) oi_str += ", ";
        oi_str += *(comps[i]);
      }
      for (size_t i = 0; i < nof_comps; i++) delete comps[i];
      comps.clear();
      if (is_constant) {
        // the objid only contains constants 
        // => create a literal and return its name
        return get_my_scope()->get_scope_mod_gen()->add_objid_literal(oi_str, nof_comps);
      } 
      // the objid contains at least one variable
      // => append the number of components before the component values in the string and return it
      return "OBJID(" + Int2string(nof_comps) + ", " + oi_str + ")"; }
    case V_SEQOF:
    case V_SETOF:
      if (u.val_vs->get_nof_vs() > 0)
        FATAL_ERROR("Value::get_single_expr()");
      return string("NULL_VALUE");
    case V_SEQ:
    case V_SET:
      if (u.val_nvs->get_nof_nvs() > 0)
        FATAL_ERROR("Value::get_single_expr()");
      return string("NULL_VALUE");
    case V_REFD: {
      Value *v_last = get_value_refd_last();
      if (v_last != this && v_last->has_single_expr() &&
          v_last->my_scope->get_scope_mod_gen() ==
	  my_scope->get_scope_mod_gen()) {
	// the reference points to another single value in the same module
	return v_last->get_single_expr();
      } else {
	// convert the reference to a single expression
	expression_struct expr;
	Code::init_expr(&expr);
	u.ref.ref->generate_code_const_ref(&expr);
	if (expr.preamble || expr.postamble)
	  FATAL_ERROR("Value::get_single_expr()");
	string ret_val(expr.expr);
	Code::free_expr(&expr);
	return ret_val;
      } }
    case V_OMIT:
      return string("OMIT_VALUE");
    case V_VERDICT:
      switch (u.verdict) {
      case Verdict_NONE:
	return string("NONE");
      case Verdict_PASS:
	return string("PASS");
      case Verdict_INCONC:
	return string("INCONC");
      case Verdict_FAIL:
	return string("FAIL");
      case Verdict_ERROR:
	return string("ERROR");
      default:
	FATAL_ERROR("Value::get_single_expr()");
	return string();
      }
    case V_DEFAULT_NULL:
      return string("NULL_COMPREF");
    case V_FAT_NULL: {
      string ret_val('(');
      ret_val += my_governor->get_genname_value(my_scope);
      ret_val += "::function_pointer)Module_List::get_fat_null()";
      return ret_val; }
    case V_EXPR:
    case V_INVOKE: {
      expression_struct expr;
      Code::init_expr(&expr);
      if (valuetype == V_EXPR) generate_code_expr_expr(&expr);
      else generate_code_expr_invoke(&expr);
      if (expr.preamble || expr.postamble)
	FATAL_ERROR("Value::get_single_expr()");
      string ret_val(expr.expr);
      Code::free_expr(&expr);
      return ret_val; }
    case V_MACRO:
      switch (u.macro) {
      case MACRO_TESTCASEID:
	return string("TTCN_Runtime::get_testcase_id_macro()");
      default:
	FATAL_ERROR("Value::get_single_expr(): invalid macrotype");
	return string();
      }
    case V_FUNCTION:
    case V_ALTSTEP:
    case V_TESTCASE:
      return get_single_expr_fat();
    default:
      FATAL_ERROR("Value::get_single_expr()");
      return string();
    }
  }

  bool Value::has_single_expr_expr()
  {
    switch (u.expr.v_optype) {
    case OPTYPE_RND: // -
    case OPTYPE_COMP_NULL:
    case OPTYPE_COMP_MTC:
    case OPTYPE_COMP_SYSTEM:
    case OPTYPE_COMP_SELF:
    case OPTYPE_COMP_RUNNING_ANY:
    case OPTYPE_COMP_RUNNING_ALL:
    case OPTYPE_COMP_ALIVE_ANY:
    case OPTYPE_COMP_ALIVE_ALL:
    case OPTYPE_TMR_RUNNING_ANY:
    case OPTYPE_GETVERDICT:
    case OPTYPE_TESTCASENAME:
    case OPTYPE_PROF_RUNNING:
    case OPTYPE_CHECKSTATE_ANY:
    case OPTYPE_CHECKSTATE_ALL:
    case OPTYPE_HOSTID:
      return true;
    case OPTYPE_ENCODE:
    case OPTYPE_DECODE:
    case OPTYPE_ISBOUND:
    case OPTYPE_ISPRESENT:
    case OPTYPE_TTCN2STRING:
    case OPTYPE_ENCVALUE_UNICHAR:
    case OPTYPE_DECVALUE_UNICHAR:
      return false;
    case OPTYPE_COMP_RUNNING: // v1 [r2] b4
    case OPTYPE_COMP_ALIVE:
      if (u.expr.r2 != NULL) {
        return false;
      }
      // no break
    case OPTYPE_UNARYPLUS: // v1
    case OPTYPE_UNARYMINUS:
    case OPTYPE_NOT:
    case OPTYPE_NOT4B:
    case OPTYPE_BIT2HEX:
    case OPTYPE_BIT2INT:
    case OPTYPE_BIT2OCT:
    case OPTYPE_BIT2STR:
    case OPTYPE_CHAR2INT:
    case OPTYPE_CHAR2OCT:
    case OPTYPE_FLOAT2INT:
    case OPTYPE_FLOAT2STR:
    case OPTYPE_HEX2BIT:
    case OPTYPE_HEX2INT:
    case OPTYPE_HEX2OCT:
    case OPTYPE_HEX2STR:
    case OPTYPE_INT2CHAR:
    case OPTYPE_INT2FLOAT:
    case OPTYPE_INT2STR:
    case OPTYPE_INT2UNICHAR:
    case OPTYPE_OCT2BIT:
    case OPTYPE_OCT2CHAR:
    case OPTYPE_OCT2HEX:
    case OPTYPE_OCT2INT:
    case OPTYPE_OCT2STR:
    case OPTYPE_STR2BIT:
    case OPTYPE_STR2FLOAT:
    case OPTYPE_STR2HEX:
    case OPTYPE_STR2INT:
    case OPTYPE_STR2OCT:
    case OPTYPE_UNICHAR2INT:
    case OPTYPE_UNICHAR2CHAR:
    case OPTYPE_ENUM2INT:
    case OPTYPE_RNDWITHVAL:
    case OPTYPE_ISCHOSEN_V: // v1 i2
    case OPTYPE_GET_STRINGENCODING:
    case OPTYPE_REMOVE_BOM:
    case OPTYPE_DECODE_BASE64:
      return u.expr.v1->has_single_expr();
    case OPTYPE_ISCHOSEN_T: // t1 i2
      return u.expr.t1->has_single_expr();
    case OPTYPE_ADD: // v1 v2
    case OPTYPE_SUBTRACT:
    case OPTYPE_MULTIPLY:
    case OPTYPE_DIVIDE:
    case OPTYPE_MOD:
    case OPTYPE_REM:
    case OPTYPE_CONCAT:
    case OPTYPE_EQ:
    case OPTYPE_LT:
    case OPTYPE_GT:
    case OPTYPE_NE:
    case OPTYPE_GE:
    case OPTYPE_LE:
    case OPTYPE_XOR:
    case OPTYPE_AND4B:
    case OPTYPE_OR4B:
    case OPTYPE_XOR4B:
    case OPTYPE_SHL:
    case OPTYPE_SHR:
    case OPTYPE_ROTL:
    case OPTYPE_ROTR:
    case OPTYPE_INT2BIT:
    case OPTYPE_INT2HEX:
    case OPTYPE_INT2OCT:
      return u.expr.v1->has_single_expr() &&
             u.expr.v2->has_single_expr();
    case OPTYPE_UNICHAR2OCT:
    case OPTYPE_OCT2UNICHAR:
    case OPTYPE_ENCODE_BASE64:
      return u.expr.v1->has_single_expr() &&
           (!u.expr.v2 || u.expr.v2->has_single_expr());
    case OPTYPE_AND:
    case OPTYPE_OR:
      return u.expr.v1->has_single_expr() &&
             u.expr.v2->has_single_expr() &&
            !u.expr.v2->needs_short_circuit();
    case OPTYPE_SUBSTR:
      return u.expr.ti1->has_single_expr() &&
             u.expr.v2->has_single_expr() && u.expr.v3->has_single_expr();
    case OPTYPE_REGEXP:
      return u.expr.ti1->has_single_expr() && u.expr.t2->has_single_expr() &&
             u.expr.v3->has_single_expr();
    case OPTYPE_DECOMP: // v1 v2 v3
      return u.expr.v1->has_single_expr() &&
             u.expr.v2->has_single_expr() &&
             u.expr.v3->has_single_expr();
    case OPTYPE_REPLACE:
      return u.expr.ti1->has_single_expr() &&
             u.expr.v2->has_single_expr() && u.expr.v3->has_single_expr() &&
             u.expr.ti4->has_single_expr();
    case OPTYPE_ISVALUE: // ti1
    case OPTYPE_LENGTHOF: // ti1
    case OPTYPE_SIZEOF: // ti1
    case OPTYPE_VALUEOF: // ti1
      return u.expr.ti1->has_single_expr();
    case OPTYPE_LOG2STR:
    case OPTYPE_ANY2UNISTR:
      return u.expr.logargs->has_single_expr();
    case OPTYPE_ISTEMPLATEKIND:
      return u.expr.ti1->has_single_expr() &&
             u.expr.v2->has_single_expr();
    case OPTYPE_MATCH: // v1 t2
      return u.expr.v1->has_single_expr() &&
             u.expr.t2->has_single_expr();
    case OPTYPE_COMP_CREATE: // r1 [v2] [v3] b4
      return (!u.expr.v2 || u.expr.v2->has_single_expr()) &&
             (!u.expr.v3 || u.expr.v3->has_single_expr());
    case OPTYPE_TMR_RUNNING: // r1 [r2] b4
      if (u.expr.r2 != NULL) {
        return false;
      }
      // no break
    case OPTYPE_TMR_READ: // r1
    case OPTYPE_ACTIVATE:
      return u.expr.r1->has_single_expr();
    case OPTYPE_EXECUTE: // r1 [v2]
      return u.expr.r1->has_single_expr() &&
           (!u.expr.v2 || u.expr.v2->has_single_expr());
    case OPTYPE_ACTIVATE_REFD: // v1 ap_list2
      return has_single_expr_invoke(u.expr.v1, u.expr.ap_list2);
    case OPTYPE_EXECUTE_REFD: // v1 ap_list2 [v3]
      return has_single_expr_invoke(u.expr.v1, u.expr.ap_list2) &&
            (!u.expr.v3 || u.expr.v3->has_single_expr());
    default:
      FATAL_ERROR("Value::has_single_expr_expr()");
    } // switch
  }

  bool Value::has_single_expr_invoke(Value *v, Ttcn::ActualParList *ap_list)
  {
    if (!v->has_single_expr()) return false;
    for (size_t i = 0; i < ap_list->get_nof_pars(); i++)
      if (!ap_list->get_par(i)->has_single_expr()) return false;
    return true;
  }

  string Value::get_single_expr_enum()
  {
    string ret_val(my_governor->get_genname_value(my_scope));
    ret_val += "::";
    ret_val += u.val_id->get_name();
    return ret_val;
  }

  string Value::get_single_expr_iso2022str()
  {
    string ret_val;
    Type *type = get_my_governor()->get_type_refd_last();
    switch (type->get_typetype()) {
    case Type::T_TELETEXSTRING:
      ret_val += "TTCN_ISO2022_2_TeletexString";
      break;
    case Type::T_VIDEOTEXSTRING:
      ret_val += "TTCN_ISO2022_2_VideotexString";
      break;
    case Type::T_GRAPHICSTRING:
    case Type::T_OBJECTDESCRIPTOR:
      ret_val += "TTCN_ISO2022_2_GraphicString";
      break;
    case Type::T_GENERALSTRING:
      ret_val += "TTCN_ISO2022_2_GeneralString";
      break;
    default:
      FATAL_ERROR("Value::get_single_expr_iso2022str()");
    } // switch
    ret_val += '(';
    string *ostr = char2oct(*u.str.val_str);
    ret_val += get_my_scope()->get_scope_mod_gen()
      ->add_octetstring_literal(*ostr);
    delete ostr;
    ret_val += ')';
    return ret_val;
  }

  string Value::get_single_expr_fat()
  {
    if (!my_governor) FATAL_ERROR("Value::get_single_expr_fat()");
    // the ampersand operator is not really necessary to obtain the function
    // pointer, but some older versions of GCC cannot instantiate the
    // appropriate operator=() member of class OPTIONAL when necessary
    // if only the function name is given
    string ret_val('&');
    switch (valuetype) {
    case V_FUNCTION:
      ret_val += u.refd_fat->get_genname_from_scope(my_scope);
      break;
    case V_ALTSTEP:
      ret_val += u.refd_fat->get_genname_from_scope(my_scope);
      ret_val += "_instance";
      break;
    case V_TESTCASE:
      ret_val += u.refd_fat->get_genname_from_scope(my_scope, "testcase_");
      break;
    default:
      FATAL_ERROR("Value::get_single_expr_fat()");
    }
    return ret_val;
  }

  bool Value::is_compound()
  {
    switch (valuetype) {
    case V_CHOICE:
    case V_SEQOF:
    case V_SETOF:
    case V_ARRAY:
    case V_SEQ:
    case V_SET:
      return true;
    default:
      return false;
    }
  }

  bool Value::needs_temp_ref()
  {
    switch (valuetype) {
    case V_SEQOF:
    case V_SETOF:
      if (!is_indexed()) {
        // Temporary reference is needed if the value has at least one real
        // element (i.e. it is not empty or contains only not used symbols).
        for (size_t i = 0; i < u.val_vs->get_nof_vs(); i++) {
          if (u.val_vs->get_v_byIndex(i)->valuetype != V_NOTUSED) return true;
        }
      } else {
        for (size_t i = 0; i < u.val_vs->get_nof_ivs(); i++) {
          if (u.val_vs->get_iv_byIndex(i)->get_value()
              ->valuetype != V_NOTUSED)
            return true;
        }
      }
      return false;
    case V_ARRAY: {
      size_t nof_real_vs = 0;
      if (!is_indexed()) {
        // Temporary reference is needed if the array value has at least two
        // real elements (excluding not used symbols).
        for (size_t i = 0; i < u.val_vs->get_nof_vs(); i++) {
          if (u.val_vs->get_v_byIndex(i)->valuetype != V_NOTUSED) {
            nof_real_vs++;
            if (nof_real_vs > 1) return true;
          }
        }
      } else {
        for (size_t i = 0; i < u.val_vs->get_nof_ivs(); i++) {
          if (u.val_vs->get_iv_byIndex(i)->get_value()
              ->valuetype != V_NOTUSED) {
            nof_real_vs++;
            if (nof_real_vs > 1) return true;
          }
        }
      }
      return false; }
    case V_SEQ:
    case V_SET:
      if (is_asn1()) {
        // it depends on the type since fields with omit or default value
	// may not be present
	return my_governor->get_type_refd_last()->get_nof_comps() > 1;
      } else {
	// incomplete values are allowed in TTCN-3
	// we should check the number of value components
	return u.val_nvs->get_nof_nvs() > 1;
      }
    case V_ERROR:
    case V_NAMEDINT:
    case V_NAMEDBITS:
    case V_UNDEF_LOWERID:
    case V_UNDEF_BLOCK:
    case V_TTCN3_NULL:
      // these values cannot occur during code generation
      FATAL_ERROR("Value::needs_temp_ref()");
    case V_INT:
      return !u.val_Int->is_native();
    default:
      // other value types (literal values) do not need temporary reference
      return false;
    }
  }

  bool Value::needs_short_circuit()
  {
    switch (valuetype) {
    case V_BOOL:
      return false;
    case V_REFD:
      // examined below
      break;
    case V_EXPR:
    case V_INVOKE:
      // sub-expressions should be evaluated only if necessary
      return true;
    default:
      FATAL_ERROR("Value::needs_short_circuit()");
    }
    Assignment *t_ass = u.ref.ref->get_refd_assignment();
    if (!t_ass) FATAL_ERROR("Value::needs_short_circuit()");
    switch (t_ass->get_asstype()) {
    case Assignment::A_FUNCTION_RVAL:
    case Assignment::A_EXT_FUNCTION_RVAL:
      // avoid unnecessary call of a function
      return true;
    case Assignment::A_CONST:
    case Assignment::A_EXT_CONST:
    case Assignment::A_MODULEPAR:
    case Assignment::A_VAR:
    case Assignment::A_PAR_VAL_IN:
    case Assignment::A_PAR_VAL_OUT:
    case Assignment::A_PAR_VAL_INOUT:
      // depends on field/array sub-references, which is examined below
      break;
    default:
      FATAL_ERROR("Value::needs_short_circuit()");
    }
    Ttcn::FieldOrArrayRefs *t_subrefs = u.ref.ref->get_subrefs();
    if (t_subrefs) {
      // the evaluation of the reference does not have side effects
      // (i.e. false shall be returned) only if all sub-references point to
      // mandatory fields of record/set types, and neither sub-reference points
      // to a field of a union type
      Type *t_type = t_ass->get_Type();
      for (size_t i = 0; i < t_subrefs->get_nof_refs(); i++) {
	Ttcn::FieldOrArrayRef *t_fieldref = t_subrefs->get_ref(i);
	if (t_fieldref->get_type() == Ttcn::FieldOrArrayRef::FIELD_REF) {
	  CompField *t_cf = t_type->get_comp_byName(*t_fieldref->get_id());
	  if (Type::T_CHOICE_T == t_type->get_type_refd_last()->get_typetype() ||
        Type::T_CHOICE_A == t_type->get_type_refd_last()->get_typetype() ||
        t_cf->get_is_optional()) return true;
	  t_type = t_cf->get_type();
	} else return true;
      }
    }
    return false;
  }

  void Value::dump(unsigned level) const
  {
    switch (valuetype) {
    case V_ERROR:
    case V_NULL:
    case V_BOOL:
    case V_INT:
    case V_NAMEDINT:
    case V_NAMEDBITS:
    case V_REAL:
    case V_ENUM:
    case V_BSTR:
    case V_HSTR:
    case V_OSTR:
    case V_CSTR:
    case V_ISO2022STR:
    case V_OID:
    case V_ROID:
    case V_CHOICE:
    case V_SEQOF:
    case V_SETOF:
    case V_ARRAY:
    case V_SEQ:
    case V_SET:
    case V_OMIT:
    case V_VERDICT:
    case V_DEFAULT_NULL:
    case V_FAT_NULL:
    case V_EXPR:
    case V_MACRO:
    case V_NOTUSED:
    case V_FUNCTION:
    case V_ALTSTEP:
    case V_TESTCASE:
      DEBUG(level, "Value: %s", const_cast<Value*>(this)->get_stringRepr().c_str());
      break;
    case V_REFD:
    case V_REFER:
      DEBUG(level, "Value: reference");
      u.ref.ref->dump(level + 1);
      break;
    case V_UNDEF_LOWERID:
      DEBUG(level, "Value: identifier: %s", u.val_id->get_dispname().c_str());
      break;
    case V_UNDEF_BLOCK:
      DEBUG(level, "Value: {block}");
      break;
    case V_TTCN3_NULL:
      DEBUG(level, "Value: null");
      break;
    case V_INVOKE:
      DEBUG(level, "Value: invoke");
      u.invoke.v->dump(level + 1);
      if (u.invoke.ap_list) u.invoke.ap_list->dump(level + 1);
      else if (u.invoke.t_list) u.invoke.t_list->dump(level + 1);
      break;
    default:
      DEBUG(level, "Value: unknown type: %d", valuetype);
    } // switch
  }

  void Value::add_string_element(size_t index, Value *v_element,
    map<size_t, Value>*& string_elements)
  {
    v_element->set_my_scope(get_my_scope());
    v_element->set_my_governor(get_my_governor());
    v_element->set_fullname(get_fullname() + "[" + Int2string(index) + "]");
    v_element->set_location(*this);
    if (!string_elements) string_elements = new map<size_t, Value>;
    string_elements->add(index, v_element);
  }

///////////////////////////////////////////////////////////////////////////////
// class LazyFuzzyParamData

    int LazyFuzzyParamData::depth = 0;
    bool LazyFuzzyParamData::used_as_lvalue = false;
    vector<string>* LazyFuzzyParamData::type_vec = NULL;
    vector<string>* LazyFuzzyParamData::refd_vec = NULL;

    void LazyFuzzyParamData::init(bool p_used_as_lvalue) {
      if (depth<0) FATAL_ERROR("LazyFuzzyParamData::init()");
      if (depth==0) {
        if (type_vec || refd_vec) FATAL_ERROR("LazyFuzzyParamData::init()");
        used_as_lvalue = p_used_as_lvalue;
        type_vec = new vector<string>;
        refd_vec = new vector<string>;
      }
      depth++;
    }

    void LazyFuzzyParamData::clean() {
      if (depth<=0) FATAL_ERROR("LazyFuzzyParamData::clean()");
      if (!type_vec || !refd_vec) FATAL_ERROR("LazyFuzzyParamData::clean()");
      if (depth==1) {
        // type_vec
        for (size_t i=0; i<type_vec->size(); i++) delete (*type_vec)[i];
        type_vec->clear();
        delete type_vec;
        type_vec = NULL;
        // refd_vec
        for (size_t i=0; i<refd_vec->size(); i++) delete (*refd_vec)[i];
        refd_vec->clear();
        delete refd_vec;
        refd_vec = NULL;
      }
      depth--;
    }

    bool LazyFuzzyParamData::in_lazy_or_fuzzy() {
      if (depth<0) FATAL_ERROR("LazyFuzzyParamData::in_lazy_or_fuzzy()");
      return depth>0;
    }

    // returns a temporary id instead of the C++ reference to a definition
    // stores in vectors the C++ type of the definiton, the C++ reference to the
    // definition and whether it refers to a lazy/fuzzy formal parameter
    string LazyFuzzyParamData::add_ref_genname(Assignment* ass, Scope* scope) {
      if (!ass || !scope) FATAL_ERROR("LazyFuzzyParamData::add_ref_genname()");
      if (!type_vec || !refd_vec) FATAL_ERROR("LazyFuzzyParamData::add_ref_genname()");
      if (type_vec->size()!=refd_vec->size()) FATAL_ERROR("LazyFuzzyParamData::add_ref_genname()");
      // store the type of the assignment
      string* type_str = new string;
      switch (ass->get_asstype()) {
      case Assignment::A_MODULEPAR_TEMP:
      case Assignment::A_TEMPLATE:
      case Assignment::A_VAR_TEMPLATE:
      case Assignment::A_PAR_TEMPL_IN:
      case Assignment::A_PAR_TEMPL_OUT:
      case Assignment::A_PAR_TEMPL_INOUT:
        *type_str = ass->get_Type()->get_genname_template(scope);
        break;
      default:
        *type_str = ass->get_Type()->get_genname_value(scope);
      }
      // add the Lazy_Fuzzy_Expr<> part if the referenced assignment is a FormalPar lazy or fuzzy
      bool refd_ass_is_lazy_or_fuzzy_fpar = false;
      switch (ass->get_asstype()) {
      case Assignment::A_PAR_VAL:
      case Assignment::A_PAR_VAL_IN:
      case Assignment::A_PAR_TEMPL_IN:
        refd_ass_is_lazy_or_fuzzy_fpar = ass->get_eval_type() != NORMAL_EVAL;
        if (ass->get_eval_type() != NORMAL_EVAL) {
          *type_str = string("Lazy_Fuzzy_Expr<") + *type_str + string(">");
        }
        break;
      default:
        break;
      }
      // add the "const" part if the referenced assignment is a constant thing
      if (!refd_ass_is_lazy_or_fuzzy_fpar) {
        switch (ass->get_asstype()) {
        case Assignment::A_CONST:
        case Assignment::A_OC:
        case Assignment::A_OBJECT:
        case Assignment::A_OS:
        case Assignment::A_VS:
        case Assignment::A_EXT_CONST:
        case Assignment::A_MODULEPAR:
        case Assignment::A_MODULEPAR_TEMP:
        case Assignment::A_TEMPLATE:
        case Assignment::A_PAR_VAL:
        case Assignment::A_PAR_VAL_IN:
        case Assignment::A_PAR_TEMPL_IN:
          *type_str = string("const ") + *type_str;
          break;
        default:
          // nothing to do
          break;
        }
      }
      //
      type_vec->add(type_str);
      // store the C++ reference string
      refd_vec->add(new string(ass->get_genname_from_scope(scope,""))); // the "" parameter makes sure that no casting to type is generated into the string
      if (refd_ass_is_lazy_or_fuzzy_fpar) {
        Type* refd_ass_type = ass->get_Type();
        string refd_ass_type_genname = (ass->get_asstype()==Assignment::A_PAR_TEMPL_IN) ? refd_ass_type->get_genname_template(scope) : refd_ass_type->get_genname_value(scope);
        return string("((") + refd_ass_type_genname + string("&)") + get_member_name(refd_vec->size()-1) + string(")");
      } else {
        return get_member_name(refd_vec->size()-1);
      }
    }

    string LazyFuzzyParamData::get_member_name(size_t idx) {
      return string("lpm_") + Int2string(idx);
    }

    string LazyFuzzyParamData::get_constr_param_name(size_t idx) {
      return string("lpp_") + Int2string(idx);
    }

    void LazyFuzzyParamData::generate_code_for_value(expression_struct* expr, Value* val, Scope* my_scope) {
      // copied from ActualPar::generate_code(), TODO: remove duplication by refactoring
      if (use_runtime_2 && TypeConv::needs_conv_refd(val)) {
        const string& tmp_id = val->get_temporary_id();
        const char *tmp_id_str = tmp_id.c_str();
        expr->preamble = mputprintf(expr->preamble, "%s %s;\n",
          val->get_my_governor()->get_genname_value(my_scope).c_str(),
          tmp_id_str);
        expr->preamble = TypeConv::gen_conv_code_refd(expr->preamble,
          tmp_id_str, val);
        expr->expr = mputstr(expr->expr, tmp_id_str);
      } else {
        val->generate_code_expr(expr);
      }
    }

    void LazyFuzzyParamData::generate_code_for_template(expression_struct* expr, TemplateInstance* temp, template_restriction_t gen_restriction_check, Scope* my_scope) {
      // copied from ActualPar::generate_code(), TODO: remove duplication by refactoring
      if (use_runtime_2 && TypeConv::needs_conv_refd(temp->get_Template())) {
        const string& tmp_id = temp->get_Template()->get_temporary_id();
        const char *tmp_id_str = tmp_id.c_str();
        expr->preamble = mputprintf(expr->preamble, "%s %s;\n",
          temp->get_Template()->get_my_governor()
            ->get_genname_template(my_scope).c_str(), tmp_id_str);
        expr->preamble = TypeConv::gen_conv_code_refd(expr->preamble,
          tmp_id_str, temp->get_Template());
        // Not incorporated into gen_conv_code() yet.
        if (gen_restriction_check != TR_NONE)
          expr->preamble = Template::generate_restriction_check_code(
            expr->preamble, tmp_id_str, gen_restriction_check);
        expr->expr = mputstr(expr->expr, tmp_id_str);
      } else temp->generate_code(expr, gen_restriction_check);
    }

    void LazyFuzzyParamData::generate_code(expression_struct *expr, Value* value, Scope* scope, boolean lazy) {
      if (depth<=0) FATAL_ERROR("LazyFuzzyParamData::generate_code()");
      if (depth>1) {
        // if a function with lazy parameter(s) was called inside a lazy parameter then don't generate code for
        // lazy parameter inside a lazy parameter, call the function as a normal call
        // wrap the calculated parameter value inside a special constructor which calculates the value of its cache immediately
        expression_struct value_expr;
        Code::init_expr(&value_expr);
        generate_code_for_value(&value_expr, value, scope);
        // the id of the instance of Lazy_Fuzzy_Expr, which will be used as the actual parameter
        const string& param_id = value->get_temporary_id();
        if (value_expr.preamble) {
          expr->preamble = mputstr(expr->preamble, value_expr.preamble);
        }
        expr->preamble = mputprintf(expr->preamble, "Lazy_Fuzzy_Expr<%s> %s(%s, Lazy_Fuzzy_Expr<%s>::EXPR_EVALED, %s);\n",
          value->get_my_governor()->get_genname_value(scope).c_str(), param_id.c_str(),
          lazy ? "FALSE" : "TRUE", value->get_my_governor()->get_genname_value(scope).c_str(), value_expr.expr);
        Code::free_expr(&value_expr);
        expr->expr = mputstr(expr->expr, param_id.c_str());
        return;
      }
      // only if the formal parameter is *not* used as lvalue
      if (!used_as_lvalue && value->get_valuetype()==Value::V_REFD && value->get_reference()->get_subrefs()==NULL) {
        Assignment* refd_ass = value->get_reference()->get_refd_assignment();
        if (refd_ass) {
          bool refd_ass_is_lazy_or_fuzzy_fpar = false;
          switch (refd_ass->get_asstype()) {
          case Assignment::A_PAR_VAL:
          case Assignment::A_PAR_VAL_IN:
          case Assignment::A_PAR_TEMPL_IN:
            refd_ass_is_lazy_or_fuzzy_fpar = refd_ass->get_eval_type() != NORMAL_EVAL;
            break;
          default:
            break;
          }
          if (refd_ass_is_lazy_or_fuzzy_fpar) {
            string refd_str = refd_ass->get_genname_from_scope(scope, "");
            if ((refd_ass->get_eval_type() == LAZY_EVAL && !lazy) ||
                (refd_ass->get_eval_type() == FUZZY_EVAL && lazy)) {
              expr->preamble = mputprintf(expr->preamble, "%s.change();\n", refd_str.c_str());
              expr->postamble = mputprintf(expr->postamble, "%s.revert();\n", refd_str.c_str());
            }
            expr->expr = mputprintf(expr->expr, "%s", refd_str.c_str());
            return;
          }
        }
      }
      // generate the code for value in a temporary expr structure, this code is put inside the ::eval() member function
      expression_struct value_expr;
      Code::init_expr(&value_expr);
      generate_code_for_value(&value_expr, value, scope);
      // the id of the instance of Lazy_Fuzzy_Expr, which will be used as the actual parameter
      string param_id = value->get_temporary_id();
      string type_name = value->get_my_governor()->get_genname_value(scope);
      generate_code_param_class(expr, value_expr, param_id, type_name, lazy);
    }

    void LazyFuzzyParamData::generate_code(expression_struct *expr, TemplateInstance* temp, template_restriction_t gen_restriction_check, Scope* scope, boolean lazy) {
      if (depth<=0) FATAL_ERROR("LazyFuzzyParamData::generate_code()");
      if (depth>1) {
        // if a function with lazy parameter(s) was called inside a lazy parameter then don't generate code for
        // lazy parameter inside a lazy parameter, call the function as a normal call
        // wrap the calculated parameter value inside a special constructor which calculates the value of its cache immediately
        expression_struct tmpl_expr;
        Code::init_expr(&tmpl_expr);
        generate_code_for_template(&tmpl_expr, temp, gen_restriction_check, scope);
        // the id of the instance of Lazy_Fuzzy_Expr which will be used as the actual parameter
        const string& param_id = temp->get_Template()->get_temporary_id();
        if (tmpl_expr.preamble) {
          expr->preamble = mputstr(expr->preamble, tmpl_expr.preamble);
        }
        expr->preamble = mputprintf(expr->preamble, "Lazy_Fuzzy_Expr<%s> %s(%s, Lazy_Fuzzy_Expr<%s>::EXPR_EVALED, %s);\n",
          temp->get_Template()->get_my_governor()->get_genname_template(scope).c_str(), param_id.c_str(),
          lazy ? "FALSE" : "TRUE", temp->get_Template()->get_my_governor()->get_genname_template(scope).c_str(), tmpl_expr.expr);
        Code::free_expr(&tmpl_expr);
        expr->expr = mputstr(expr->expr, param_id.c_str());
        return;
      }
      // only if the formal parameter is *not* used as lvalue
      if (!used_as_lvalue && temp->get_Template()->get_templatetype()==Template::TEMPLATE_REFD && temp->get_Template()->get_reference()->get_subrefs()==NULL) {
        Assignment* refd_ass = temp->get_Template()->get_reference()->get_refd_assignment();
        if (refd_ass) {
          bool refd_ass_is_lazy_or_fuzzy_fpar = false;
          switch (refd_ass->get_asstype()) {
          case Assignment::A_PAR_VAL:
          case Assignment::A_PAR_VAL_IN:
          case Assignment::A_PAR_TEMPL_IN:
            refd_ass_is_lazy_or_fuzzy_fpar = refd_ass->get_eval_type() != NORMAL_EVAL;
            break;
          default:
            break;
          }
          if (refd_ass_is_lazy_or_fuzzy_fpar) {
            string refd_str = refd_ass->get_genname_from_scope(scope, "");
            if ((refd_ass->get_eval_type() == LAZY_EVAL && !lazy) ||
                (refd_ass->get_eval_type() == FUZZY_EVAL && lazy)) {
              expr->preamble = mputprintf(expr->preamble, "%s.change();\n", refd_str.c_str());
              expr->postamble = mputprintf(expr->postamble, "%s.revert();\n", refd_str.c_str());
            }
            expr->expr = mputprintf(expr->expr, "%s", refd_str.c_str());
            return;
          }
        }
      }
      // generate the code for template in a temporary expr structure, this code is put inside the ::eval_expr() member function
      expression_struct tmpl_expr;
      Code::init_expr(&tmpl_expr);
      generate_code_for_template(&tmpl_expr, temp, gen_restriction_check, scope);
      // the id of the instance of Lazy_Fuzzy_Expr which will be used as the actual parameter
      string param_id = temp->get_Template()->get_temporary_id();
      string type_name = temp->get_Template()->get_my_governor()->get_genname_template(scope);
      generate_code_param_class(expr, tmpl_expr, param_id, type_name, lazy);
    }

    void LazyFuzzyParamData::generate_code_param_class(expression_struct *expr, expression_struct& param_expr, const string& param_id, const string& type_name, boolean lazy) {
      expr->preamble = mputprintf(expr->preamble,
        "class Lazy_Fuzzy_Expr_%s : public Lazy_Fuzzy_Expr<%s> {\n",
          param_id.c_str(), type_name.c_str());
      // private members of the local class will be const references to the objects referenced by the expression
      for (size_t i=0; i<type_vec->size(); i++) {
        expr->preamble = mputprintf(expr->preamble, "%s& %s;\n", (*type_vec)[i]->c_str(), get_member_name(i).c_str());
      }
      expr->preamble = mputstr(expr->preamble, "public:\n");
      expr->preamble = mputprintf(expr->preamble, "Lazy_Fuzzy_Expr_%s(", param_id.c_str());
      for (size_t i=0; i<type_vec->size(); i++) {
          if (i>0) expr->preamble = mputstr(expr->preamble, ", ");
          expr->preamble = mputprintf(expr->preamble, "%s& %s", (*type_vec)[i]->c_str(), get_constr_param_name(i).c_str());
      }
      expr->preamble = mputprintf(expr->preamble, "): Lazy_Fuzzy_Expr<%s>(%s)", type_name.c_str(), lazy ? "FALSE" : "TRUE");
      for (size_t i=0; i<type_vec->size(); i++) {
        expr->preamble = mputprintf(expr->preamble, ", %s(%s)", get_member_name(i).c_str(), get_constr_param_name(i).c_str());
      }
      expr->preamble = mputstr(expr->preamble, " {}\n");
      expr->preamble = mputstr(expr->preamble, "private:\n");
      expr->preamble = mputstr(expr->preamble, "virtual void eval_expr() {\n");
      // use the temporary expr structure to fill the body of the eval_expr() function
      if (param_expr.preamble) {
        expr->preamble = mputstr(expr->preamble, param_expr.preamble);
      }
      expr->preamble = mputprintf(expr->preamble, "expr_cache = %s;\n", param_expr.expr);
      if (param_expr.postamble) {
        expr->preamble = mputstr(expr->preamble, param_expr.postamble);
      }
      Code::free_expr(&param_expr);
      expr->preamble = mputstr(expr->preamble, "}\n"
        "};\n" // end of local class definition
      );
      expr->preamble = mputprintf(expr->preamble, "Lazy_Fuzzy_Expr_%s %s", param_id.c_str(), param_id.c_str());
      if (type_vec->size()>0) {
        expr->preamble = mputc(expr->preamble, '(');
        // paramteres of the constructor are references to the objects used in the expression
        for (size_t i=0; i<refd_vec->size(); i++) {
          if (i>0) expr->preamble = mputstr(expr->preamble, ", ");
          expr->preamble = mputprintf(expr->preamble, "%s", (*refd_vec)[i]->c_str());
        }
        expr->preamble = mputc(expr->preamble, ')');
      }
      expr->preamble = mputstr(expr->preamble, ";\n");
      // the instance of the local class Lazy_Fuzzy_Expr_tmp_xxx is used as the actual parameter
      expr->expr = mputprintf(expr->expr, "%s", param_id.c_str());
    }

    void LazyFuzzyParamData::generate_code_ap_default_ref(expression_struct *expr, Ttcn::Ref_base* ref, Scope* scope, boolean lazy) {
      expression_struct ref_expr;
      Code::init_expr(&ref_expr);
      ref->generate_code(&ref_expr);
      const string& param_id = scope->get_scope_mod_gen()->get_temporary_id();
      if (ref_expr.preamble) {
         expr->preamble = mputstr(expr->preamble, ref_expr.preamble);
      }
      Assignment* ass = ref->get_refd_assignment();
      // determine C++ type of the assignment
      string type_str;
      switch (ass->get_asstype()) {
      case Assignment::A_MODULEPAR_TEMP:
      case Assignment::A_TEMPLATE:
      case Assignment::A_VAR_TEMPLATE:
      case Assignment::A_PAR_TEMPL_IN:
      case Assignment::A_PAR_TEMPL_OUT:
      case Assignment::A_PAR_TEMPL_INOUT:
        type_str = ass->get_Type()->get_genname_template(scope);
        break;
      default:
        type_str = ass->get_Type()->get_genname_value(scope);
      }
      expr->preamble = mputprintf(expr->preamble, "Lazy_Fuzzy_Expr<%s> %s(%s, Lazy_Fuzzy_Expr<%s>::EXPR_EVALED, %s);\n",
        type_str.c_str(), param_id.c_str(), lazy ? "FALSE" : "TRUE", type_str.c_str(), ref_expr.expr);
      if (ref_expr.postamble) {
        expr->postamble = mputstr(expr->postamble, ref_expr.postamble);
      }
      Code::free_expr(&ref_expr);
      expr->expr = mputstr(expr->expr, param_id.c_str());
    }

    void LazyFuzzyParamData::generate_code_ap_default_value(expression_struct *expr, Value* value, Scope* scope, boolean lazy) {
      const string& param_id = value->get_temporary_id();
      expr->preamble = mputprintf(expr->preamble, "Lazy_Fuzzy_Expr<%s> %s(%s, Lazy_Fuzzy_Expr<%s>::EXPR_EVALED, %s);\n",
        value->get_my_governor()->get_genname_value(scope).c_str(), param_id.c_str(),
        lazy ? "FALSE" : "TRUE", value->get_my_governor()->get_genname_value(scope).c_str(),
        value->get_genname_own(scope).c_str());
      expr->expr = mputstr(expr->expr, param_id.c_str());
    }

    void LazyFuzzyParamData::generate_code_ap_default_ti(expression_struct *expr, TemplateInstance* ti, Scope* scope, boolean lazy) {
      const string& param_id = ti->get_Template()->get_temporary_id();
      expr->preamble = mputprintf(expr->preamble, "Lazy_Fuzzy_Expr<%s> %s(%s, Lazy_Fuzzy_Expr<%s>::EXPR_EVALED, %s);\n",
        ti->get_Template()->get_my_governor()->get_genname_template(scope).c_str(), param_id.c_str(),
        lazy ? "FALSE" : "TRUE", ti->get_Template()->get_my_governor()->get_genname_template(scope).c_str(), 
        ti->get_Template()->get_genname_own(scope).c_str());
      expr->expr = mputstr(expr->expr, param_id.c_str());
    }

} // namespace Common
