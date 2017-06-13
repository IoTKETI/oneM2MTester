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
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#include "Block.hh"
#include "TokenBuf.hh"
#include "../error.h"

extern void asn1la_newtokenbuf(Asn::TokenBuf *tb);
extern int asn1_yyparse();
#define yyparse asn1_yyparse

namespace Asn {

  using namespace Common;

  Node *parsed_node;

  Block::Block(TokenBuf *p_tokenbuf)
    : Node(), Location(), tokenbuf(p_tokenbuf)
  {
    if (!p_tokenbuf) FATAL_ERROR("NULL parameter: Asn::Block::Block()");
  }

  Block::Block(const Block& p)
    : Node(p), Location(p)
  {
    tokenbuf=p.tokenbuf->clone();
  }

  Block::~Block()
  {
    delete tokenbuf;
  }

  Node* Block::parse(int kw_token)
  {
    tokenbuf->push_front_kw_token(kw_token);
    asn1la_newtokenbuf(tokenbuf);
    if(yyparse()) parsed_node=0;
    return parsed_node;
  }

  void Block::dump(unsigned level) const
  {
    DEBUG(level, "Block");
    tokenbuf->dump(level + 1);
  }

} // namespace Asn
