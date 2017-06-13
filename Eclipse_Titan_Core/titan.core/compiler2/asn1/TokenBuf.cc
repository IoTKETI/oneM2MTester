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
 *   Delic, Adam
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#include "TokenBuf.hh"
#include "../AST.hh"
#include "Block.hh"

extern void asn1_yyerror(const char *s);
#define yyerror asn1_yyerror
extern int asn1_yylex();
#define yylex asn1_yylex
extern const char *asn1_infile;
extern int asn1_yylineno;
#define yylineno asn1_yylineno
extern int asn1_plineno;
#define plineno asn1_plineno
extern YYLTYPE asn1_yylloc;
#define yylloc asn1_yylloc

namespace Asn {

  using namespace Common;

  // =================================
  // ===== Token
  // =================================

  Token::Token(const Token& p)
    : Node(p), Location(p), token(p.token)
  {
    switch (p.token) {
    case TOK_UpperIdentifier:
    case TOK_LowerIdentifier:
    case TOK_ampUpperIdentifier:
    case TOK_ampLowerIdentifier:
      semval.id = p.semval.id->clone();
      break;
    case TOK_Number:
      semval.i = new int_val_t(*p.semval.i);
      break;
    case TOK_CString:
      semval.str = new string(*p.semval.str);
      break;
    case TOK_RealNumber:
    case TOK_BString:
    case TOK_HString:
      semval.value = p.semval.value->clone();
      break;
    case TOK_Block:
      semval.block = p.semval.block->clone();
    default:
      break;
    }
  }

  bool Token::has_semval(int p_token)
  {
    switch (p_token) {
    case TOK_UpperIdentifier:
    case TOK_LowerIdentifier:
    case TOK_ampUpperIdentifier:
    case TOK_ampLowerIdentifier:
    case TOK_Number:
    case TOK_CString:
    case TOK_RealNumber:
    case TOK_BString:
    case TOK_HString:
    case TOK_Block:
      return true;
    default:
      return false;
    }
  }

  Token::Token(int p_token, const Location& p_loc)
    : Node(), Location(p_loc), token(p_token)
  {
    if (has_semval(p_token)) FATAL_ERROR("Token::Token()");
  }

  Token::Token(int p_token, const YYSTYPE& p_semval, const Location& p_loc)
    : Node(), Location(p_loc), token(p_token)
  {
    switch (p_token) {
    case TOK_UpperIdentifier:
    case TOK_LowerIdentifier:
    case TOK_ampUpperIdentifier:
    case TOK_ampLowerIdentifier:
      semval.id = p_semval.id;
      break;
    case TOK_Number:
      semval.i = p_semval.i;
      break;
    case TOK_CString:
      semval.str = p_semval.str;
      break;
    case TOK_RealNumber:
    case TOK_BString:
    case TOK_HString:
      semval.value = p_semval.value;
      break;
    case TOK_Block:
      semval.block = p_semval.block;
    default:
      break;
    }
  }

  Token::~Token()
  {
    switch(token) {
    case TOK_UpperIdentifier:
    case TOK_LowerIdentifier:
    case TOK_ampUpperIdentifier:
    case TOK_ampLowerIdentifier:
      delete semval.id;
      break;
    case TOK_CString:
      delete semval.str;
      break;
    case TOK_RealNumber:
    case TOK_BString:
    case TOK_HString:
      delete semval.value;
      break;
    case TOK_Block:
      delete semval.block;
      break;
    case TOK_Number:
      delete semval.i;
      break;
    default:
      break;
    } // switch
  }

  Token *Token::clone() const
  {
    return new Token(*this);
  }

  void Token::set_token(int new_token)
  {
    if (new_token != token) {
      if (has_semval(token) || has_semval(new_token))
	FATAL_ERROR("Token::set_token()");
      token = new_token;
    }
  }

  void Token::steal_semval(YYSTYPE& p_semval)
  {
    switch (token) {
    case TOK_UpperIdentifier:
    case TOK_LowerIdentifier:
    case TOK_ampUpperIdentifier:
    case TOK_ampLowerIdentifier:
      p_semval.id = semval.id;
      break;
    case TOK_Number:
      p_semval.i = semval.i;
      break;
    case TOK_CString:
      p_semval.str = semval.str;
      break;
    case TOK_RealNumber:
    case TOK_BString:
    case TOK_HString:
      p_semval.value = semval.value;
      break;
    case TOK_Block:
      p_semval.block = semval.block;
    default:
      break;
    }
    token = '\0';
  }

  void Token::set_loc_info() const
  {
    asn1_infile = get_filename();
    if(get_lineno() > 0) plineno = get_lineno();
  }

  bool Token::is_literal_id() const
  {
    if(token==TOK_UpperIdentifier
       && semval.id->isvalid_asn_word())
      return true;
    else return false;
  }

  bool Token::is_literal_kw() const
  {
    switch(token) {
    case ',':
    case KW_ABSENT:
    case KW_ALL:
    case KW_ANY:
    case KW_APPLICATION:
    case KW_AUTOMATIC:
    case KW_BEGIN:
    case KW_BY:
    case KW_CLASS:
    case KW_COMPONENT:
    case KW_COMPONENTS:
    case KW_CONSTRAINED:
    case KW_CONTAINING:
    case KW_DEFAULT:
    case KW_DEFINED:
    case KW_DEFINITIONS:
    case KW_ENCODED:
    case KW_EXCEPT:
    case KW_EXPLICIT:
    case KW_EXPORTS:
    case KW_EXTENSIBILITY:
    case KW_FROM:
    case KW_IDENTIFIER:
    case KW_IMPLICIT:
    case KW_IMPLIED:
    case KW_IMPORTS:
    case KW_INCLUDES:
    case KW_MAX:
    case KW_MIN:
    case KW_OF:
    case KW_OPTIONAL:
    case KW_PATTERN:
    case KW_PDV:
    case KW_PRESENT:
    case KW_PRIVATE:
    case KW_SIZE:
    case KW_STRING:
    case KW_SYNTAX:
    case KW_TAGS:
    case KW_UNIQUE:
    case KW_UNIVERSAL:
    case KW_WITH:
      return true;
    default:
      return false;
    } // switch token
  }

  bool Token::is_ampId() const
  {
    if(token==TOK_ampUpperIdentifier
       || token==TOK_ampLowerIdentifier)
      return true;
    else return false;
  }

  bool Token::is_id() const
  {
    switch(token) {
    case TOK_UpperIdentifier:
    case TOK_LowerIdentifier:
    case TOK_ampUpperIdentifier:
    case TOK_ampLowerIdentifier:
      return true;
    default:
      return false;
    } // switch
  }

  const char* Token::get_token_name(int p_token)
  {
    switch(p_token) {
    case '\0': return "<end of file or statement>";
    case TOK_Assignment: return "::=";
    case TOK_RangeSeparator: return "..";
    case TOK_Ellipsis: return "...";
    case TOK_LeftVersionBrackets: return "[[";
    case TOK_RightVersionBrackets: return "]]";
    case '{': return "{";
    case '}': return "}";
    case '(': return "(";
    case ')': return ")";
    case '[': return "[";
    case ']': return "]";
    case ',': return ",";
    case '.': return ".";
    case '-': return "-";
    case ':': return ":";
    case ';': return ";";
    case '@': return "@";
    case '|': return "|";
    case '!': return "!";
    case '^': return "^";
    case '<': return "<";
    case '>': return ">";
    case '\'': return "'";
    case '"': return "\"";
    case KW_ABSENT: return "ABSENT";
    case KW_ALL: return "ALL";
    case KW_ANY: return "ANY";
    case KW_APPLICATION: return "APPLICATION";
    case KW_AUTOMATIC: return "AUTOMATIC";
    case KW_BEGIN: return "BEGIN";
    case KW_BIT: return "BIT";
    case KW_BMPString: return "BMPString";
    case KW_BOOLEAN: return "BOOLEAN";
    case KW_BY: return "BY";
    case KW_CHARACTER: return "CHARACTER";
    case KW_CHOICE: return "CHOICE";
    case KW_CLASS: return "CLASS";
    case KW_COMPONENT: return "COMPONENT";
    case KW_COMPONENTS: return "COMPONENTS";
    case KW_CONSTRAINED: return "CONSTRAINED";
    case KW_CONTAINING: return "CONTAINING";
    case KW_DEFAULT: return "DEFAULT";
    case KW_DEFINED: return "DEFINED";
    case KW_DEFINITIONS: return "DEFINITIONS";
    case KW_EMBEDDED: return "EMBEDDED";
    case KW_ENCODED: return "ENCODED";
    case KW_END: return "END";
    case KW_ENUMERATED: return "ENUMERATED";
    case KW_EXCEPT: return "EXCEPT";
    case KW_EXPLICIT: return "EXPLICIT";
    case KW_EXPORTS: return "EXPORTS";
    case KW_EXTENSIBILITY: return "EXTENSIBILITY";
    case KW_EXTERNAL: return "EXTERNAL";
    case KW_FALSE: return "FALSE";
    case KW_GeneralizedTime: return "GeneralizedTime";
    case KW_GeneralString: return "GeneralString";
    case KW_GraphicString: return "GraphicString";
    case KW_IA5String: return "IA5String";
    case KW_FROM: return "FROM";
    case KW_IDENTIFIER: return "IDENTIFIER";
    case KW_IMPLICIT: return "IMPLICIT";
    case KW_IMPLIED: return "IMPLIED";
    case KW_IMPORTS: return "IMPORTS";
    case KW_INCLUDES: return "INCLUDES";
    case KW_INSTANCE: return "INSTANCE";
    case KW_INTEGER: return "INTEGER";
    case KW_INTERSECTION: return "INTERSECTION";
    case KW_ISO646String: return "ISO646String";
    case KW_MAX: return "MAX";
    case KW_MIN: return "MIN";
    case KW_MINUS_INFINITY: return "MINUS-INFINITY";
    case KW_NULL: return "NULL";
    case KW_NumericString: return "NumericString";
    case KW_OBJECT: return "OBJECT";
    case KW_ObjectDescriptor: return "ObjectDescriptor";
    case KW_OCTET: return "OCTET";
    case KW_OF: return "OF";
    case KW_OPTIONAL: return "OPTIONAL";
    case KW_PATTERN: return "PATTERN";
    case KW_PDV: return "PDV";
    case KW_PLUS_INFINITY: return "PLUS-INFINITY";
    case KW_PRESENT: return "PRESENT";
    case KW_PrintableString: return "PrintableString";
    case KW_PRIVATE: return "PRIVATE";
    case KW_REAL: return "REAL";
    case KW_RELATIVE_OID: return "RELATIVE-OID";
    case KW_SEQUENCE: return "SEQUENCE";
    case KW_SET: return "SET";
    case KW_SIZE: return "SIZE";
    case KW_STRING: return "STRING";
    case KW_SYNTAX: return "SYNTAX";
    case KW_T61String: return "T61String";
    case KW_TAGS: return "TAGS";
    case KW_TeletexString: return "TeletexString";
    case KW_TRUE: return "TRUE";
    case KW_UNION: return "UNION";
    case KW_UNIQUE: return "UNIQUE";
    case KW_UNIVERSAL: return "UNIVERSAL";
    case KW_UniversalString: return "UniversalString";
    case KW_UTCTime: return "UTCTime";
    case KW_UTF8String: return "UTF8String";
    case KW_VideotexString: return "VideotexString";
    case KW_VisibleString: return "VisibleString";
    case KW_WITH: return "WITH";
    case TOK_UpperIdentifier: return "<upperidentifier>";
    case TOK_LowerIdentifier: return "<loweridentifier>";
    case TOK_ampUpperIdentifier: return "<&upperidentifier>";
    case TOK_ampLowerIdentifier: return "<&loweridentifier>";
    case TOK_Number: return "<number>";
    case TOK_RealNumber: return "<realnumber>";
    case TOK_BString: return  "<bstring>";
    case TOK_CString: return  "<cstring>";
    case TOK_HString: return  "<hstring>";
    case TOK_Block: return  "<{block}>";
    /* special stuff */
    case KW_Block_NamedNumberList: return "<NamedNumberList:>";
    case KW_Block_Enumerations: return "<Enumerations:>";
    case KW_Block_Assignment: return "<Assignment:>";
    case KW_Block_NamedBitList: return "<NamedBitList:>";
    case KW_Block_IdentifierList: return "<IdentifierList:>";
    case KW_Block_FieldSpecList: return "<FieldSpecList:>";
    case KW_Block_ComponentTypeLists: return "<ComponentTypeLists:>";
    case KW_Block_AlternativeTypeLists: return "<AlternativeTypeLists:>";
    case KW_Block_Type: return "<Type:>";
    case KW_Block_Value: return "<Value:>";
    case KW_Block_ValueSet: return "<ValueSet:>";
    case KW_Block_Object: return "<Object:>";
    case KW_Block_ObjectSet: return "<ObjectSet:>";
    case KW_Block_SeqOfValue: return "<SeqOfValue:>";
    case KW_Block_SetOfValue: return "<SetOfValue:>";
    case KW_Block_SequenceValue: return "<SequenceValue:>";
    case KW_Block_SetValue: return "<SetValue:>";
    case KW_Block_ObjectSetSpec: return "<ObjectSetSpec:>";
    case KW_Block_DefinedObjectSetBlock: return "<DefinedObjectSetBlock:>";
    case KW_Block_AtNotationList: return "<AtNotationList:>";
    case KW_Block_OIDValue: return "<OIDValue:>";
    case KW_Block_ROIDValue: return "<ROIDValue:>";
    case KW_Block_CharStringValue: return "<CharStringValue:>";
    case KW_Block_QuadrupleOrTuple: return "<QuadrupleOrTuple:>";
    default:
      return "<Error! Not a keyword.>";
    } // switch token
  }

  const Identifier& Token::get_semval_id() const
  {
    switch(token) {
    case TOK_UpperIdentifier:
    case TOK_LowerIdentifier:
    case TOK_ampUpperIdentifier:
    case TOK_ampLowerIdentifier:
      return *semval.id;
    default:
      FATAL_ERROR("Token::get_semval_id()");
    } // switch
  }

  void Token::create_block(TokenBuf *tb)
  {
    if (token != '{' || !tb) FATAL_ERROR("Token::create_block()");
    token = TOK_Block;
    /** \todo set the correct end line/column info */
    semval.block = new Block(tb);
    semval.block->set_location(*this);
  }

  void Token::dump(unsigned level) const
  {
    switch(token) {
    case TOK_UpperIdentifier:
    case TOK_LowerIdentifier:
    case TOK_ampUpperIdentifier:
    case TOK_ampLowerIdentifier:
      semval.id->dump(level);
      break;
    case TOK_Number:
      DEBUG(level, "token: number (%s)", semval.i->t_str().c_str());
      break;
    case TOK_CString:
      DEBUG(level, "token: cstring (\"%s\")", semval.str->c_str());
      break;
    case TOK_RealNumber:
    case TOK_BString:
    case TOK_HString:
      semval.value->dump(level);
      break;
    case TOK_Block:
      semval.block->dump(level);
      break;
    default:
      DEBUG(level, "%s", get_token_name());
    }
  }

  // =================================
  // ===== TokenBuf
  // =================================

  TokenBuf::TokenBuf()
    : Node(), filename("<undef>")
  {
    tokens=new tokens_t();
  }

  TokenBuf::TokenBuf(tokens_t *p_tokens)
    : Node(), tokens(p_tokens), filename("<undef>")
  {
    if(!p_tokens)
      FATAL_ERROR("NULL parameter: Asn::TokenBuf::TokenBuf(tokens_t*)");
  }

  TokenBuf::TokenBuf(const TokenBuf& p)
    : Node(), filename(p.filename)
  {
    tokens=new tokens_t();
    for(size_t i=0; i<p.tokens->size(); i++)
      tokens->add((*p.tokens)[i]->clone());
  }

  TokenBuf::~TokenBuf()
  {
    delete_tokens();
    delete tokens;
  }

  void TokenBuf::delete_tokens()
  {
    for(size_t i=0; i<tokens->size(); i++)
      delete (*tokens)[i];
    tokens->clear();
  }

  void TokenBuf::reset(const char *p_filename)
  {
    delete_tokens();
    filename=p_filename;
  }

  bool TokenBuf::read_next()
  {
    /* if there was an EOF, don't try to read another token */
    if(!tokens->empty() && (*tokens)[tokens->size()-1]->get_token()=='\0')
      return false;
    int token=yylex();
    tokens->add(new Token(token, yylval, Location(filename, yylineno)));
    return true;
  }

  Token*& TokenBuf::get_at(size_t pos)
  {
    while(tokens->size()<=pos && read_next()) ;
    if(pos<tokens->size()) {
      return (*tokens)[pos];
    }
    else {
      if(tokens->empty())
        FATAL_ERROR("Asn::TokenBuf::get_at()");
      return (*tokens)[tokens->size()-1];
    }
  }

  void TokenBuf::push_front_token(Token *p_token)
  {
    if(!p_token)
      FATAL_ERROR("Asn::TokenBuf::push_front_token()");
    tokens->add_front(p_token);
  }

  void TokenBuf::push_front_kw_token(int kw)
  {
    if(tokens->empty())
      FATAL_ERROR("Asn::TokenBuf::push_front_kw_token()");
    tokens->add_front(new Token(kw, *(*tokens)[0]));
  }

  void TokenBuf::push_back_token(Token *p_token)
  {
    if(!p_token)
      FATAL_ERROR("NULL parameter: Asn::TokenBuf::push_back_token()");
    tokens->add(p_token);
  }

  void TokenBuf::push_back_kw_token(int kw)
  {
    if(tokens->empty())
      FATAL_ERROR("Asn::TokenBuf::push_back_kw_token()");
    tokens->add(new Token(kw, *(*tokens)[tokens->size() - 1]));
  }

  void TokenBuf::move_tokens_from(TokenBuf *tb)
  {
    if(!tb)
      FATAL_ERROR("NULL parameter: Asn::TokenBuf::move_tokens_from()");
    size_t insert_pos=tokens->size();
    if(!tokens->empty() && (*tokens)[tokens->size()-1]->get_token()=='\0')
      insert_pos--;
    size_t num_of_toks=tb->tokens->size();
    if(!tb->tokens->empty() &&
       (*tb->tokens)[tb->tokens->size()-1]->get_token()=='\0')
      num_of_toks--;
    tokens_t *tmp_tokens=tb->tokens->subvector(0, num_of_toks);
    tb->tokens->replace(0, num_of_toks);
    tokens->replace(insert_pos, 0, tmp_tokens);
    tmp_tokens->clear(); delete tmp_tokens;
  }

  Token* TokenBuf::pop_front_token()
  {
    Token *token=get_at(0);
    if(token->get_token()=='{') {
      token->set_loc_info(); /* report this line if unmatch_error */
      /* search the matching '}' */
      int blocklevel=1;
      size_t i;
      bool unmatch_error=false;
      for(i=1; blocklevel>0 && !unmatch_error; i++) {
        switch(get_at(i)->get_token()) {
        case '{':
          blocklevel++;
          break;
        case '}':
          blocklevel--;
          break;
        case 0:
          yyerror("Unmatched '{'");
          unmatch_error=true;
          i--; /* EOF is not part of the block */
          break;
        }
      }
      /* copy the tokens of the block to a new vector */
      tokens_t *tokens_block=tokens->subvector(1, i-1);
      if(!unmatch_error) /* replace '}' to EOF */
        (*tokens_block)[tokens_block->size()-1]->set_token('\0');
      else /* clone EOF and add to block */
        tokens_block->add(get_at(i)->clone());
      /* remove the tokens from their original vector */
      tokens->replace(0, i);
      /* create the new tokenbuf from the tokens of the block */
      TokenBuf *tb=new TokenBuf(tokens_block);
      tb->filename=filename;
      /* transform the opening '{' to a block-token */
      token->create_block(tb);
    }
    else {
      /* remove the first token */
      tokens->replace(0, 1);
    }
    return token;
  }

  int TokenBuf::lex()
  {
    Token *token=pop_front_token();
    int tmp_tok = token->get_token();
    token->set_loc_info();
    token->steal_semval(yylval);
    yylloc.first_line=yylloc.last_line=token->get_lineno();
    yylloc.first_column=yylloc.last_column=0;
    delete token;
    return tmp_tok;
  }

  void TokenBuf::dump(unsigned level) const
  {
    DEBUG(level, "Tokens: (%lu pcs.)", static_cast<unsigned long>( tokens->size() ));
    level++;
    for(size_t i=0; i<tokens->size(); i++)
      (*tokens)[i]->dump(level);
  }

} // namespace Asn
