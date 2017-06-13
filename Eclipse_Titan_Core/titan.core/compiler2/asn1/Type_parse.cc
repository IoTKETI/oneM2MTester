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
#include "../Type.hh"
#include "Block.hh"
#include "TokenBuf.hh"

#include "../EnumItem.hh"

namespace Common {

  void Type::parse_block_Enum()
  {
    Node *node=u.enums.block->parse(KW_Block_Enumerations);
    delete u.enums.block; u.enums.block=0;
    Type *tmp=dynamic_cast<Type*>(node);
    if(!tmp) {
      delete u.enums.eis;
      typetype=T_ERROR;
      return;
    }
    delete u.enums.eis1;
    u.enums.eis1=tmp->u.enums.eis1;
    tmp->u.enums.eis1=0;
    if(!u.enums.eis1) FATAL_ERROR("NULL parameter");
    u.enums.eis1->set_fullname(get_fullname());
    u.enums.eis1->set_my_scope(my_scope);
    u.enums.ellipsis=tmp->u.enums.ellipsis;
    delete u.enums.eis2;
    u.enums.eis2=tmp->u.enums.eis2;
    tmp->u.enums.eis2=0;
    if(u.enums.eis2) {
      u.enums.eis2->set_fullname(get_fullname());
      u.enums.eis2->set_my_scope(my_scope);
    }
    delete tmp;
  }

  void Type::parse_block_Int()
  {
    Node *node=u.namednums.block->parse(KW_Block_NamedNumberList);
    delete u.namednums.block; u.namednums.block=0;
    u.namednums.nvs=dynamic_cast<NamedValues*>(node);
    if(!u.namednums.nvs) {
      typetype=T_ERROR;
      return;
    }
    u.namednums.nvs->set_fullname(get_fullname());
    u.namednums.nvs->set_my_scope(my_scope);
  }

  void Type::parse_block_BStr()
  {
    Node *node=u.namednums.block->parse(KW_Block_NamedBitList);
    delete u.namednums.block; u.namednums.block=0;
    delete u.namednums.nvs;
    u.namednums.nvs=dynamic_cast<NamedValues*>(node);
    if(!u.namednums.nvs) {
      typetype=T_ERROR;
      return;
    }
    u.namednums.nvs->set_fullname(get_fullname());
    u.namednums.nvs->set_my_scope(my_scope);
  }

  void Type::parse_block_Choice()
  {
    Node *node=u.secho.block->parse(KW_Block_AlternativeTypeLists);
    delete u.secho.block; u.secho.block=0;
    u.secho.ctss=dynamic_cast<CTs_EE_CTs*>(node);
    if(!u.secho.ctss) {
      typetype=T_ERROR;
      return;
    }
    u.secho.ctss->set_fullname(get_fullname());
    u.secho.ctss->set_my_scope(my_scope);
    u.secho.ctss->set_my_type(this);
  }

  void Type::parse_block_Se()
  {
    Node *node=u.secho.block->parse(KW_Block_ComponentTypeLists);
    delete u.secho.block; u.secho.block=0;
    u.secho.ctss = dynamic_cast<CTs_EE_CTs*>(node);
    if(!u.secho.ctss) {
      typetype=T_ERROR;
      return;
    }
    u.secho.ctss->set_fullname(get_fullname());
    u.secho.ctss->set_my_scope(my_scope);
    u.secho.ctss->set_my_type(this);
  }


} // namespace Common
