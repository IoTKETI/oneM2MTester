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
 *
 ******************************************************************************/
#ifndef _Asn_Block_HH
#define _Asn_Block_HH

#include "../AST.hh"

namespace Asn {

  using namespace Common;

  extern Node *parsed_node;

  class TokenBuf;

  /**
   * A node which represents an unparsed block. When parsing a file,
   * it is not always known what to expect when a block begins
   * ('{'). So, the tokens of the block are pushed into a TokenBuf.
   * Block is a wrapping-class for TokenBuf. As Block is a
   * Node-descendant, it can be used in the AST.
   *
   * After the examination of the block's context, it is known what to
   * expect in the block, so the block can be parsed.
   */
  class Block : public Node, public Location {
  private:
    /** The wrapped token buffer. */
    TokenBuf *tokenbuf;

    Block(const Block& p);
  public:
    /** Constructs a Block from a TokenBuf. */
    Block(TokenBuf *p_tokenbuf);
    /** The destructor. */
    virtual ~Block();
    virtual Block* clone() const
    {return new Block(*this);}
    /** Returns the TokenBuf. You can modify (put tokens into/remove
     * tokens from) it, but do not delete the buffer! */
    TokenBuf* get_TokenBuf() {return tokenbuf;}
    /** Parses the block. The expected content is expressed by \a
     * kw_token, which is a constant defined in the grammar
     * (asn1p.y). Returns an AST-subtree. To use this function, you
     * have to include the TokenBuf header, too. */
    Node* parse(int kw_token);

    virtual void dump(unsigned) const;
  };

} // namespace Asn

#endif /* _Asn_Block_HH */
