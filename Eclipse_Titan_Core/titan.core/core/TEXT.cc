/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Delic, Adam
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *
 ******************************************************************************/
#include <string.h>
#include <sys/types.h>
#include <regex.h>
#include "../common/memory.h"
#include "../common/pattern.hh"
#include "TEXT.hh"
#include "Addfunc.hh"
#include "Logger.hh"
#include "Integer.hh"
#include "Charstring.hh"
#define ERRMSG_BUFSIZE2 500

Token_Match::Token_Match(const char *posix_str, boolean case_sensitive,
  boolean fixed)
: posix_regexp_begin()
, posix_regexp_first()
, token_str(posix_str)
, fixed_len(0)
, null_match(FALSE)
{
  if (posix_str == NULL || posix_str[0] == '\0') {
    token_str = "";
    null_match = TRUE;
    return;
  }

  if (fixed) {
    // no regexp
    fixed_len = strlen(posix_str);

    if (!case_sensitive) {
      // Can't happen. The compiler should always generate
      // case sensitive matching for fixed strings.
      TTCN_EncDec_ErrorContext::error_internal(
        "Case insensitive fixed string matching not implemented");
    }
  }
  else {
    int regcomp_flags = REG_EXTENDED;
    if (!case_sensitive) regcomp_flags |= REG_ICASE;
    int ret_val = regcomp(&posix_regexp_begin, posix_str, regcomp_flags);
    if (ret_val != 0) {
      char msg[ERRMSG_BUFSIZE2];
      regerror(ret_val, &posix_regexp_begin, msg, ERRMSG_BUFSIZE2);
      TTCN_error("Internal error: regcomp() failed on posix_regexp_begin when "
        "constructing Token_Match: %s", msg);
    }
    ret_val = regcomp(&posix_regexp_first, posix_str + 1, regcomp_flags);
    if (ret_val != 0) {
      regfree(&posix_regexp_begin);
      char msg[ERRMSG_BUFSIZE2];
      regerror(ret_val, &posix_regexp_first, msg, ERRMSG_BUFSIZE2);
      TTCN_error("Internal error: regcomp() failed on posix_regexp_first when "
        "constructing Token_Match: %s", msg);
    }
  }
}

Token_Match::~Token_Match()
{
  if (!null_match && fixed_len == 0) {
    regfree(&posix_regexp_begin);
    regfree(&posix_regexp_first);
  }
}

int Token_Match::match_begin(TTCN_Buffer& buff) const
{
  int retval=-1;
  int ret_val=-1;
  if(null_match){ 
    if (TTCN_EncDec::get_error_behavior(TTCN_EncDec::ET_LOG_MATCHING) !=
        TTCN_EncDec::EB_IGNORE) {
      char msg[ERRMSG_BUFSIZE2];
      regerror(ret_val, &posix_regexp_begin, msg, ERRMSG_BUFSIZE2);
      TTCN_Logger::log(TTCN_Logger::DEBUG_ENCDEC, "match_begin data: %s",
        (const char*)buff.get_read_data());
      TTCN_Logger::begin_event(TTCN_Logger::DEBUG_ENCDEC);
      TTCN_Logger::log_event_str("match_begin token: null_match");
      TTCN_Logger::end_event();
      TTCN_Logger::log(TTCN_Logger::DEBUG_ENCDEC, "match_begin result: 0");
    }
    
    return 0;
  }
  if (fixed_len != 0) { //
    retval=strncmp((const char*)buff.get_read_data(), token_str, fixed_len)
      ? -1 // not matched
        : fixed_len;
  } else {
    regmatch_t pmatch[2];
    ret_val=regexec(&posix_regexp_begin,(const char*)buff.get_read_data(),
                    2, pmatch, 0);
    if(ret_val==0) {
      retval=pmatch[1].rm_eo-pmatch[1].rm_so; // pmatch[1] is the capture group
    } else if (ret_val==REG_NOMATCH) {
      retval=-1;
    } else {
       /* regexp error */
      char msg[ERRMSG_BUFSIZE2];
      regerror(ret_val, &posix_regexp_begin, msg, ERRMSG_BUFSIZE2);
      TTCN_error("Internal error: regexec() failed in "
        "Token_Match::match_begin(): %s", msg);
    }
  }
  if (TTCN_EncDec::get_error_behavior(TTCN_EncDec::ET_LOG_MATCHING) !=
      TTCN_EncDec::EB_IGNORE) {
    TTCN_Logger::log(TTCN_Logger::DEBUG_ENCDEC, "match_begin data: %s",
      (const char*)buff.get_read_data());
    TTCN_Logger::begin_event(TTCN_Logger::DEBUG_ENCDEC);
    TTCN_Logger::log_event_str("match_begin token: \"");
    for (size_t i = 0; token_str[i] != '\0'; i++)
      TTCN_Logger::log_char_escaped(token_str[i]);
    TTCN_Logger::log_char('"');
    TTCN_Logger::end_event();
    if(fixed_len == 0){
      char msg[ERRMSG_BUFSIZE2];
      regerror(ret_val, &posix_regexp_begin, msg, ERRMSG_BUFSIZE2);
      TTCN_Logger::log(TTCN_Logger::DEBUG_ENCDEC, "match_begin regexec result: %d, %s",
        ret_val,msg);
    }
    TTCN_Logger::log(TTCN_Logger::DEBUG_ENCDEC, "match_begin result: %d",
      retval);
  }
  return retval;
}

int Token_Match::match_first(TTCN_Buffer& buff) const
{
  int retval=-1;
  int ret_val=-1;
  if(null_match){ 
    if (TTCN_EncDec::get_error_behavior(TTCN_EncDec::ET_LOG_MATCHING) !=
        TTCN_EncDec::EB_IGNORE) {
      char msg[ERRMSG_BUFSIZE2];
      regerror(ret_val, &posix_regexp_begin, msg, ERRMSG_BUFSIZE2);
      TTCN_Logger::log(TTCN_Logger::DEBUG_ENCDEC, "match_first data: %s",
        (const char*)buff.get_read_data());
      TTCN_Logger::begin_event(TTCN_Logger::DEBUG_ENCDEC);
      TTCN_Logger::log_event_str("match_first token: null_match");
      TTCN_Logger::end_event();
      TTCN_Logger::log(TTCN_Logger::DEBUG_ENCDEC, "match_first result: 0");
    }
    
    return 0;
  }
//TTCN_Logger::log(TTCN_Logger::DEBUG_ENCDEC,"match_first data: %s\n\r",(const char*)buff.get_read_data());
//TTCN_Logger::log(TTCN_Logger::DEBUG_ENCDEC,"match_first token: %s\n\r",token);
  if (fixed_len != 0) {
    const char *haystack = (const char*)buff.get_read_data();
    const char *pos = strstr(haystack, token_str); // TODO smarter search: Boyer-Moore / Shift-Or
    retval = (pos == NULL) ? -1 : (pos - haystack);
  } else {
    regmatch_t pmatch[2];
    ret_val=regexec(&posix_regexp_first,(const char*)buff.get_read_data(),
                    2, pmatch, REG_NOTBOL);
    if(ret_val==0) {
      retval=pmatch[1].rm_so;
    } else if (ret_val==REG_NOMATCH) {
      retval=-1;
    } else {
       /* regexp error */
      char msg[ERRMSG_BUFSIZE2];
      regerror(ret_val, &posix_regexp_begin, msg, ERRMSG_BUFSIZE2);
      TTCN_error("Internal error: regexec() failed in "
        "Token_Match::match_first(): %s", msg);
    }
  }
  if (TTCN_EncDec::get_error_behavior(TTCN_EncDec::ET_LOG_MATCHING) !=
      TTCN_EncDec::EB_IGNORE) {
    TTCN_Logger::log(TTCN_Logger::DEBUG_ENCDEC, "match_first data: %s",
      (const char*)buff.get_read_data());
    TTCN_Logger::begin_event(TTCN_Logger::DEBUG_ENCDEC);
    TTCN_Logger::log_event_str("match_first token: \"");
    for (size_t i = 0; token_str[i] != '\0'; i++)
      TTCN_Logger::log_char_escaped(token_str[i]);
    TTCN_Logger::log_char('"');
    TTCN_Logger::end_event();
    if (fixed_len == 0) {
      char msg[ERRMSG_BUFSIZE2];
      regerror(ret_val, &posix_regexp_begin, msg, ERRMSG_BUFSIZE2);

      TTCN_Logger::log(TTCN_Logger::DEBUG_ENCDEC, "match_begin regexec result: %d, %s",
        ret_val,msg);
    }
    TTCN_Logger::log(TTCN_Logger::DEBUG_ENCDEC, "match_first result: %d",
      retval);
  }
  return retval;
}

Limit_Token_List::Limit_Token_List(){
  num_of_tokens=0;
  size_of_list=16;
  list=(const Token_Match **)Malloc(size_of_list*sizeof(Token_Match *));
  last_match=(int *)Malloc(size_of_list*sizeof(int));
  last_ret_val=-1;
  last_pos=NULL;
}

Limit_Token_List::~Limit_Token_List(){
  Free(list);
  Free(last_match);
}

void Limit_Token_List::add_token(const Token_Match *token){
  if(num_of_tokens==size_of_list){
    size_of_list*=2;
    list=(const Token_Match **)Realloc(list,size_of_list*sizeof(Token_Match *));
    last_match=(int *)Realloc(last_match,size_of_list*sizeof(int));
  }
  list[num_of_tokens]=token;
  last_match[num_of_tokens]=-1;
  num_of_tokens++;
}

void Limit_Token_List::remove_tokens(size_t num){
  if(num_of_tokens<num) num_of_tokens=0;
  else num_of_tokens-=num;
}

int Limit_Token_List::match(TTCN_Buffer& buff, size_t lim){
  int ret_val=-1;
  const char* curr_pos=(const char*)buff.get_read_data();
//TTCN_Logger::log(TTCN_Logger::DEBUG_ENCDEC,"Limit Token begin: %s\n\r",(const char*)buff.get_read_data());
  if(last_pos!=NULL){
    int diff=curr_pos-last_pos;
    if(diff){
      for(size_t a=0;a<num_of_tokens;a++){
        last_match[a]-=diff;
      }
      last_ret_val-=diff;
    }
  }
  last_pos=curr_pos;
//  if(last_ret_val<0){
    for(size_t a=0;a<num_of_tokens-lim;a++){
      if(last_match[a]<0) last_match[a]=list[a]->match_first(buff);
      if(last_match[a]>=0){
        if(ret_val==-1) ret_val=last_match[a];
        else if(last_match[a]<ret_val) ret_val=last_match[a];
      }
    }
    last_ret_val=ret_val;
//  }
  if (TTCN_EncDec::get_error_behavior(TTCN_EncDec::ET_LOG_MATCHING)!=
      TTCN_EncDec::EB_IGNORE) {
    TTCN_Logger::log(TTCN_Logger::DEBUG_ENCDEC,
      "match_list data: %s",(const char*)buff.get_read_data());
    TTCN_Logger::log(TTCN_Logger::DEBUG_ENCDEC,"match_list result: %d",ret_val);
  }
  return ret_val;
}

const TTCN_TEXTdescriptor_t INTEGER_text_ = { NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, { NULL } };

const TTCN_TEXTdescriptor_t BOOLEAN_text_ = { NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, { NULL } };

const TTCN_TEXTdescriptor_t CHARSTRING_text_ = { NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, { NULL } };

const TTCN_TEXTdescriptor_t UNIVERSAL_CHARSTRING_text_ = { NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, { NULL } };

const TTCN_TEXTdescriptor_t BITSTRING_text_ = { NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, { NULL } };

const TTCN_TEXTdescriptor_t HEXSTRING_text_ = { NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, { NULL } };

const TTCN_TEXTdescriptor_t OCTETSTRING_text_ = { NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, { NULL } };
