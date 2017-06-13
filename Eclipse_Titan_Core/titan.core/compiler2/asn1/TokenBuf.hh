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
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef _Asn_TokenBuf_HH
#define _Asn_TokenBuf_HH

#include "../vector.hh"

#include "AST_asn1.hh"
#include "Ref.hh"
#include "../Type.hh"
#include "../Typestuff.hh" // FIXME CTs
#include "../Value.hh"
#include "../Valuestuff.hh"
#include "Tag.hh"
#include "TableConstraint.hh"
#include "Block.hh"
#include "Object.hh"

#include "asn1p.tab.hh"

#define yylval asn1_yylval

namespace Asn {

  class TokenBuf;

  /**
   * Representation of a token.
   */
  class Token : public Node, public Location {
  private:
    int token;
    union {
      Identifier *id;
      int_val_t *i;
      string *str;
      Value *value;
      Block *block;
    } semval;

    Token(const Token& p);
    static bool has_semval(int p_token);
  public:
    Token(int p_token, const Location& p_loc);
    Token(int p_token, const YYSTYPE& p_semval, const Location& p_loc);
    virtual ~Token();
    virtual Token* clone() const;
    int get_token() const { return token; }
    void set_token(int new_token);
    void steal_semval(YYSTYPE& p_semval);
    void set_loc_info() const;
    bool is_literal_id() const;
    bool is_literal_kw() const;
    bool is_ampId() const;
    bool is_id() const;
    static const char* get_token_name(int p_token);
    const char* get_token_name() const { return get_token_name(token); }
    const Identifier& get_semval_id() const;
    /** Transforms \a this (which must be a '{' token) to a block
     * containing \a tb. */
    void create_block(TokenBuf *tb);
    virtual void dump(unsigned level) const;
  };

  /**
   * Class to represent group of tokens.  An instance of TokenBuf
   * takes place between yyparse() and yylex().  yyparse() calls
   * (indirectly) TokenBuf::lex(), which calls (indirectly)
   * yylex(). TokenBuf::lex() never returns '{'; it buffers the entire
   * {}-block in a new TokenBuf, and returns a single token which
   * contains all the tokens (and line number information) in the
   * block. Then this new TokenBuf can be used to parse the block.
   */
  class TokenBuf : public Node {
  private:
    /** Vector type to store tokens. */
    typedef vector<Token> tokens_t;

    /** The Tokens. Not the band. :) */
    tokens_t *tokens;
    /** Name of the file from where the tokens are. */
    const char *filename;

    TokenBuf(const TokenBuf& p);
    /** Constructs a new TokenBuf with the given tokens. */
    TokenBuf(tokens_t *p_tokens);
    /** Deletes the stored tokens. */
    void delete_tokens();
    /** Reads (yylex) the next token. Returns true on success, false
     * on EOF. */
    bool read_next();
  public:
    /** Default constructor. */
    TokenBuf();
    /** Destructor. */
    virtual ~TokenBuf();
    virtual TokenBuf* clone() const
    {return new TokenBuf(*this);}
    const char *get_filename() {return filename;}
    /** Makes the buffer empty. */
    void reset(const char *p_filename);
    /** Inserts the given token as the first token. */
    void push_front_token(Token *p_token);
    /** Inserts the given keyword token as the first token. */
    void push_front_kw_token(int p_kw);
    /** Inserts the given token as the last token. */
    void push_back_token(Token *p_token);
    /** Inserts the given keyword token as the last token. */
    void push_back_kw_token(int p_kw);
    /** Moves the tokens from \a to to this. */
    void move_tokens_from(TokenBuf *tb);
    /** Removes and returns the first token. */
    Token* pop_front_token();
    /** Returns a reference to the token in buffer at position \a pos.
     * Use it carefully. :) */
    Token*& get_at(size_t pos);
    /** Returns a token, sets the corresponding yylineno and
     * yylval. As needed by yacc/bison. */
    int lex();
    virtual void dump(unsigned level) const;
  };

} // namespace Asn

#endif /* _Asn_TokenBuf_HH */
