/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Cserveni, Akos
 *   Forstner, Matyas
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#include "OCSV.hh"
#include "Block.hh"
#include "TokenBuf.hh"
#include "../Type.hh"
#include "../Value.hh"

namespace Asn {

  // =================================
  // ===== OCSV_Builder
  // =================================

  OCSV_Builder::OCSV_Builder(TokenBuf *p_tb, FieldSpecs *p_fss)
    : OCS_Visitor(), fss(p_fss)
  {
    if(!p_fss)
      FATAL_ERROR("NULL parameter: Asn::OCSV_Builder::OCSV_Builder()");
    if(!p_tb) { // default syntax
      tb=0;
    }
    else {
      tb=new TokenBuf();
      tb->reset(p_tb->get_filename());
      bool ok=false;
      while(!ok) {
        Token *token=p_tb->pop_front_token();
        switch(token->get_token()) {
        case TOK_LeftVersionBrackets:
          token->set_token('[');
          tb->push_back_token(token);
          tb->push_back_token(token->clone());
          break;
        case TOK_RightVersionBrackets:
          token->set_token(']');
          tb->push_back_token(token);
          tb->push_back_token(token->clone());
          break;
        case '\0':
          ok=true;
        default:
          tb->push_back_token(token);
        } // switch token
      } // while !ok
    }
  }

  OCSV_Builder::~OCSV_Builder()
  {
    delete tb;
  }

  void OCSV_Builder::visit0(OCS_Node& p)
  {
    if(p.get_is_builded())
      FATAL_ERROR("Asn::OCSV_Builder::visit0()");
  }

  void OCSV_Builder::visit_root(OCS_root& p)
  {
    visit0(p);
    if(tb) {
      p.get_seq().accept(*this);
    }
    else {
      OCS_seq& seq=p.get_seq();
      for(size_t i=0; i<fss->get_nof_fss(); i++) {
        FieldSpec *t_fs=fss->get_fs_byIndex(i)->get_last();
        OCS_setting::settingtype_t t_st=OCS_setting::S_UNDEF;
        switch(t_fs->get_fstype()) {
        case FieldSpec::FS_T:
          t_st=OCS_setting::S_T;
          break;
        case FieldSpec::FS_V_FT:
        case FieldSpec::FS_V_VT:
          t_st=OCS_setting::S_V;
          break;
        case FieldSpec::FS_VS_FT:
        case FieldSpec::FS_VS_VT:
          t_st=OCS_setting::S_VS;
          break;
        case FieldSpec::FS_O:
          t_st=OCS_setting::S_O;
          break;
        case FieldSpec::FS_OS:
          t_st=OCS_setting::S_OS;
          break;
        case FieldSpec::FS_ERROR:
          continue;
          break;
        default:
          FATAL_ERROR("Asn::OCSV_Builder::visit_root()");
        } // switch
        OCS_seq *t_seq=new OCS_seq
          (t_fs->get_is_optional() || t_fs->has_default(), true);
        t_seq->add_node(new OCS_literal(t_fs->get_id().clone()));
        t_seq->add_node(new OCS_setting(t_st, t_fs->get_id().clone()));
        for(size_t j=0; j<2; j++)
          t_seq->get_nth_node(j)->set_location(*t_fs);
        seq.add_node(t_seq);
      } // for i
    }
    p.set_is_builded();
  }

  void OCSV_Builder::visit_seq(OCS_seq& p)
  {
    visit0(p);

    bool ok=false;
    Token *token;
    while(!ok) {
      token=tb->pop_front_token();
      token->set_loc_info();
      if(token->get_token()=='\0') {
        ok=true;
        delete token;
      }
      else if(token->get_token()=='[') {
        delete token;
        int blocklevel=1;
        TokenBuf *new_tb=new TokenBuf();
        while(blocklevel>0) {
	  token=tb->pop_front_token();
          switch(token->get_token()) {
          case '[':
            blocklevel++;
            new_tb->push_back_token(token);
            break;
          case ']':
            blocklevel--;
            if(!blocklevel) token->set_token('\0');
            new_tb->push_back_token(token);
            break;
          case 0:
            token->set_loc_info();
            token->error("Unmatched `['");
            break;
          default:
            new_tb->push_back_token(token);
            break;
          } // switch
        } // while
        OCSV_Builder *new_OCSV_Builder=new OCSV_Builder(new_tb, fss);
        delete new_tb;
        OCS_seq *new_OCS_seq=new OCS_seq(true);
        p.add_node(new_OCS_seq);
        new_OCS_seq->accept(*new_OCSV_Builder);
        delete new_OCSV_Builder;
      }
      else if(token->is_literal_kw()) {
        p.add_node(new OCS_literal(token->get_token()));
        delete token;
      }
      else if(token->is_literal_id()) {
        p.add_node(new OCS_literal(token->get_semval_id().clone()));
        delete token;
      }
      else if(token->is_ampId()) {
        OCS_setting::settingtype_t st=OCS_setting::S_UNDEF;
        FieldSpec *fs=0;
	const Identifier& id = token->get_semval_id();
        if(!fss->has_fs_withId(id)) {
          token->error("No field with name `%s'", id.get_dispname().c_str());
          goto afteradding;
        }
        fs=fss->get_fs_byId(id)->get_last();
        switch(fs->get_fstype()) {
        case FieldSpec::FS_UNDEF:
          FATAL_ERROR("Asn::OCSV_Builder::visit_seq()");
          break;
        case FieldSpec::FS_T:
          st=OCS_setting::S_T;
          break;
        case FieldSpec::FS_V_FT:
        case FieldSpec::FS_V_VT:
          st=OCS_setting::S_V;
          break;
        case FieldSpec::FS_VS_FT:
        case FieldSpec::FS_VS_VT:
          st=OCS_setting::S_VS;
          break;
        case FieldSpec::FS_O:
          st=OCS_setting::S_O;
          break;
        case FieldSpec::FS_OS:
          st=OCS_setting::S_OS;
          break;
        case FieldSpec::FS_ERROR:
          goto afteradding;
        default:
          FATAL_ERROR("Asn::OCSV_Builder::visit_seq()");
        } // switch
        p.add_node(new OCS_setting(st, id.clone()));
      afteradding:
        delete token;
      }
      else {
        token->set_loc_info();
        token->error("Unexpected `%s'", token->get_token_name());
        delete token;
      }
    } // while !ok
    if(p.get_is_opt() && p.get_nof_nodes()==0)
      token->error("Empty optional group is not allowed");
    p.set_is_builded();
  }

  void OCSV_Builder::visit_literal(OCS_literal&)
  {
    FATAL_ERROR("Asn::OCSV_Builder::visit_literal()");
  }

  void OCSV_Builder::visit_setting(OCS_setting&)
  {
    FATAL_ERROR("Asn::OCSV_Builder::visit_setting()");
  }

  // =================================
  // ===== OCSV_Parser
  // =================================

  OCSV_Parser::OCSV_Parser(TokenBuf *p_tb, Obj_defn *p_my_obj)
    : OCS_Visitor(), my_obj(p_my_obj), success(true)
  {
    if(!p_tb || !my_obj)
      FATAL_ERROR("NULL parameter: Asn::OCSV_Parser::OCSV_Parser()");
    tb=new TokenBuf();
    tb->reset(p_tb->get_filename());
    bool ok=false;
    while(!ok) {
      Token *token=p_tb->pop_front_token();
      switch(token->get_token()) {
      case '\0':
        ok=true;
      default:
        tb->push_back_token(token);
      } // switch token
    } // while !ok
  }

  OCSV_Parser::~OCSV_Parser()
  {
    delete tb;
  }

  void OCSV_Parser::visit_root(OCS_root& p)
  {
    if(!p.get_is_builded() || !success)
      FATAL_ERROR("Asn::OCSV_Parser::visit_root()");
    prev_success=false;
    my_obj->set_location(*tb->get_at(0));
    Error_Context ec(my_obj, "While parsing object `%s'",
                  my_obj->get_fullname().c_str());
    p.get_seq().accept(*this);
    if(success && tb->get_at(0)->get_token()!='\0') {
      success=false;
      tb->get_at(0)->error("Unexpected `%s'",
                           tb->get_at(0)->get_token_name());
      tb->get_at(0)->error("Superfluous part detected");
    }
    if(!success) {
      DEBUG(1, "Erroneous object definition detected.");
      DEBUG(1, "The correct syntax is:");
      p.dump(1);
      my_obj->error("Check the syntax of objectclass"
                    " (consider using debug messages)");
      my_obj->set_is_erroneous();
    }
  }

  void OCSV_Parser::visit_seq(OCS_seq& p)
  {
    size_t i;
    if(p.get_opt_first_comma() && my_obj->get_nof_fss()>0) {
      if(tb->get_at(0)->get_token()==',') {
        delete tb->pop_front_token();
      }
      else {
        if(!p.get_is_opt()) {
          success=false;
          tb->get_at(0)->error("Unexpected `%s'",
                               tb->get_at(0)->get_token_name());
          tb->get_at(0)->error("Expecting `,'");
        }
        else prev_success=true;
        return;
      }
      i=0;
    }
    else {
      /* This can be only if each of the fieldspecs were erroneous */
      if(p.get_nof_nodes()==0) return;
      p.get_nth_node(0)->accept(*this);
      if(!success) return;
      if(!prev_success) {
        if(!p.get_is_opt()) {
          success=false;
          tb->get_at(0)->error("Unexpected `%s'",
                               tb->get_at(0)->get_token_name());
          tb->get_at(0)->error("Expecting %s",
                               p.get_dispname().c_str());
        }
        else prev_success=true;
        return;
      }
      i=1;
    }
    for(; i<p.get_nof_nodes(); i++) {
      p.get_nth_node(i)->accept(*this);
      if(!prev_success) {
        if(p.get_is_opt()){
          prev_success = true;
          tb->push_front_kw_token(',');
          return;
        }
        success=false;
        tb->get_at(0)->error("Unexpected `%s'",
                             tb->get_at(0)->get_token_name());
        tb->get_at(0)->error("Expecting %s",
                             p.get_dispname().c_str());
      }
      if(!success) return;
    }
  }

  void OCSV_Parser::visit_literal(OCS_literal& p)
  {
    prev_success=false;
    if(p.is_keyword()) {
      if(p.get_keyword()==tb->get_at(0)->get_token()) {
        delete tb->pop_front_token();
        prev_success=true;
      }
    }
    else {
      Token *token=tb->get_at(0);
      if(token->is_id()
         && (token->get_semval_id().get_dispname()
             == p.get_word()->get_dispname())) {
        delete tb->pop_front_token();
        prev_success=true;
      }
    }
  }

  void OCSV_Parser::visit_setting(OCS_setting& p)
  {
    Error_Context cntxt(&p, "While parsing setting for this field: `%s'",
                        p.get_id()->get_dispname().c_str());
    FieldSetting *fs=0;
    switch(p.get_st()) {
    case OCS_setting::S_T: {
      Type *setting=parse_type();
      if(setting) {
        fs=new FieldSetting_Type(p.get_id()->clone(), setting);
        fs->set_location(*setting);
      }
      break;}
    case OCS_setting::S_V: {
      Value *setting=parse_value();
      if(setting) {
        fs=new FieldSetting_Value(p.get_id()->clone(), setting);
        fs->set_location(*setting);
      }
      break;}
    case OCS_setting::S_VS:
      NOTSUPP("ValueSet settings");
      prev_success=false;
      return;
      break;
    case OCS_setting::S_O: {
      Object *setting=parse_object();
      if(setting) {
        fs=new FieldSetting_O(p.get_id()->clone(), setting);
        fs->set_location(*setting);
      }
      break;}
    case OCS_setting::S_OS: {
      ObjectSet *setting=parse_objectset();
      if(setting) {
        fs=new FieldSetting_OS(p.get_id()->clone(), setting);
        fs->set_location(*setting);
      }
      break;}
    case OCS_setting::S_UNDEF:
      FATAL_ERROR("Undefined setting");
      break;
    } // switch
    if(!fs) {
      prev_success=false;
      return;
    }
    prev_success=true;
    my_obj->add_fs(fs);
  }

  size_t OCSV_Parser::is_ref(size_t pos)
  {
    if(!(tb->get_at(pos)->get_token()==TOK_UpperIdentifier
         || tb->get_at(pos)->get_token()==TOK_LowerIdentifier))
      return 0;
    size_t pos2=pos+1;
    while(tb->get_at(pos2)->get_token()=='.'
          && tb->get_at(pos2+1)->is_id())
      pos2+=2;
    if(tb->get_at(pos2)->get_token()==TOK_Block)
      pos2++;
    return pos2-pos;
  }

  size_t OCSV_Parser::is_tag(size_t pos)
  {
    size_t pos2=pos;
    if(tb->get_at(pos2)->get_token()!='[')
      return 0;
    do pos2++;
    while(tb->get_at(pos2)->get_token()!=']'
          && tb->get_at(pos2)->get_token()!='\0');
    if(tb->get_at(pos2)->get_token()==']') pos2++;
    switch(tb->get_at(pos2)->get_token()) {
    case KW_IMPLICIT:
    case KW_EXPLICIT:
      pos2++;
    } //switch
    return pos2-pos;
  }

  size_t OCSV_Parser::is_constraint(size_t pos)
  {
    size_t pos2=pos;
    if(tb->get_at(pos2)->get_token()!='(')
      return 0;
    unsigned blocklevel=1;
    for(pos2=pos+1; blocklevel>0; pos2++) {
      switch(tb->get_at(pos2)->get_token()) {
      case '(':
        blocklevel++;
        break;
      case ')':
        blocklevel--;
        break;
      case '\0':
        blocklevel=0;
        break;
      } // switch
    } // for pos2
    if(tb->get_at(pos2)->get_token()==')') pos2++;
    return pos2-pos;
  }

  size_t OCSV_Parser::is_constraints(size_t pos)
  {
    size_t pos2=pos, pos3;
    while((pos3=is_constraint(pos2)))
      pos2+=pos3;
    return pos2-pos;
  }

  size_t OCSV_Parser::is_nakedtype(size_t pos)
  {
    size_t pos2=pos;
    switch(tb->get_at(pos2)->get_token()) {
    case KW_NULL:
    case KW_BOOLEAN:
    case KW_GeneralString:
    case KW_BMPString:
    case KW_GraphicString:
    case KW_IA5String:
    case KW_NumericString:
    case KW_PrintableString:
    case KW_TeletexString:
    case KW_T61String:
    case KW_UniversalString:
    case KW_UTF8String:
    case KW_VideotexString:
    case KW_VisibleString:
    case KW_ISO646String:
    case KW_RELATIVE_OID:
    case KW_REAL:
    case KW_EXTERNAL:
    case KW_GeneralizedTime:
    case KW_UTCTime:
    case KW_ObjectDescriptor:
      pos2++;
      break;
    case KW_BIT:
      pos2++;
      if(tb->get_at(pos2)->get_token()==KW_STRING) {
        pos2++;
        if(tb->get_at(pos2)->get_token()==TOK_Block)
          pos2++;
      }
      break;
    case KW_CHARACTER:
      pos2++;
      if(tb->get_at(pos2)->get_token()==KW_STRING)
        pos2++;
      break;
    case KW_CHOICE:
    case KW_ENUMERATED:
    case KW_INTEGER:
      pos2++;
      if(tb->get_at(pos2)->get_token()==TOK_Block)
        pos2++;
      break;
    case KW_SEQUENCE:
    case KW_SET:
      pos2++;
      if(tb->get_at(pos2)->get_token()==TOK_Block)
        pos2++;
      else { // SeOf
        if(tb->get_at(pos2)->get_token()==KW_SIZE)
          pos2++;
        pos2+=is_constraint(pos2);
        if(tb->get_at(pos2)->get_token()==KW_OF) {
          pos2+=tb->get_at(pos2+1)->get_token()==TOK_LowerIdentifier?static_cast<size_t>(2):static_cast<size_t>(1);
          pos2+=is_type(pos2);
        }
      }
      break;
    case KW_OBJECT:
      pos2++;
      if(tb->get_at(pos2)->get_token()==KW_IDENTIFIER)
        pos2++;
      break;
    case KW_OCTET:
      pos2++;
      if(tb->get_at(pos2)->get_token()==KW_STRING)
        pos2++;
      break;
    case KW_EMBEDDED:
      pos2++;
      if(tb->get_at(pos2)->get_token()==KW_PDV)
        pos2++;
      break;
    case KW_ANY:
      pos2++;
      if(tb->get_at(pos2)->get_token()==KW_DEFINED
         && tb->get_at(pos2+1)->get_token()==KW_BY
         && tb->get_at(pos2+2)->get_token()==TOK_LowerIdentifier
         )
        pos2+=3;
      break;
    } // switch
    if(pos2==pos)
      pos2=is_ref(pos)+pos2;
    return pos2-pos;
  }

  size_t OCSV_Parser::is_type(size_t pos)
  {
    size_t pos2=pos, pos3;
    while((pos3=is_tag(pos2)))
      pos2+=pos3;
    pos2+=is_nakedtype(pos2);
    pos2+=is_constraints(pos2);
    return pos2-pos;
  }

  size_t OCSV_Parser::is_value(size_t pos)
  {
    size_t pos2=pos;
    switch(tb->get_at(pos2)->get_token()) {
    case TOK_Block:
    case TOK_BString:
    case TOK_HString:
    case TOK_CString:
    case KW_NULL:
    case KW_TRUE:
    case KW_FALSE:
    case TOK_Number:
    case TOK_RealNumber:
    case KW_PLUS_INFINITY:
    case KW_MINUS_INFINITY:
      pos2++;
      break;
    case '-':
      pos2++;
      if(tb->get_at(pos2)->get_token()==TOK_Number
         || tb->get_at(pos2)->get_token()==TOK_RealNumber)
        pos2++;
      break;
    case TOK_LowerIdentifier:
      if(tb->get_at(pos2+1)->get_token()==':') {
        pos2=is_value(pos2+2);
        if(pos2) pos2+=pos+2;
        else pos2=pos;
      } else pos2=pos;
      break;
    } // switch
    if(pos2==pos)
      pos2=is_ref(pos)+pos;
    return pos2-pos;
  }

  size_t OCSV_Parser::is_object(size_t pos)
  {
    size_t pos2=pos;
    if(tb->get_at(pos2)->get_token()==TOK_Block)
      pos2++;
    if(pos2==pos)
      pos2=is_ref(pos)+pos;
    return pos2-pos;
  }

  size_t OCSV_Parser::is_objectset(size_t pos)
  {
    size_t pos2=pos;
    if(tb->get_at(pos2)->get_token()==TOK_Block)
      pos2++;
    return pos2-pos;
  }

  Block* OCSV_Parser::get_first_n(size_t n)
  {
    TokenBuf *new_tb=new TokenBuf();
    new_tb->reset(tb->get_filename());
    for(size_t i=0; i<n; i++)
      new_tb->push_back_token(tb->pop_front_token());
    new_tb->push_back_token(new Token(0, Location(tb->get_filename())));
    return new Block(new_tb);
  }

  Type* OCSV_Parser::parse_type()
  {
    size_t n;
    n=is_type(0);
    if(!n) return 0;
    Block *block=get_first_n(n);
    Node *node=block->parse(KW_Block_Type);
    delete block;
    Type *new_type=dynamic_cast<Type*>(node);
    if(!new_type)
      new_type=new Type(Type::T_ERROR);
    return new_type;
  }

  Value* OCSV_Parser::parse_value()
  {
    size_t n;
    n=is_value(0);
    if(!n) return 0;
    Block *block=get_first_n(n);
    Node *node=block->parse(KW_Block_Value);
    delete block;
    Value *new_value=dynamic_cast<Value*>(node);
    if(!new_value)
      new_value=new Value(Value::V_ERROR);
    return new_value;
  }

  Object* OCSV_Parser::parse_object()
  {
    size_t n;
    n=is_object(0);
    if(!n) return 0;
    Block *block=get_first_n(n);
    Node *node=block->parse(KW_Block_Object);
    delete block;
    Object *new_object=dynamic_cast<Object*>(node);
    if(!new_object)
      new_object=new Obj_defn();
    return new_object;
  }

  ObjectSet* OCSV_Parser::parse_objectset()
  {
    size_t n;
    n=is_objectset(0);
    if(!n) return 0;
    Block *block=get_first_n(n);
    Node *node=block->parse(KW_Block_ObjectSet);
    delete block;
    ObjectSet *new_objectset=dynamic_cast<ObjectSet*>(node);
    if(!new_objectset)
      new_objectset=new OS_defn();
    return new_objectset;
  }

} // namespace Asn
