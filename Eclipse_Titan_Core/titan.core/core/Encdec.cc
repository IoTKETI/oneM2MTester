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
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Ormandi, Matyas
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#include "Encdec.hh"

#include "../common/memory.h"
#include "Octetstring.hh"
#include "Charstring.hh"
#include "String_struct.hh"
#include "RAW.hh"
#include "Error.hh"
#include "Logger.hh"
#include "string.h"

const TTCN_EncDec::error_behavior_t
TTCN_EncDec::default_error_behavior[TTCN_EncDec::ET_ALL] = {
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_WARNING,
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_WARNING,
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_WARNING,
  TTCN_EncDec::EB_WARNING,
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_WARNING,
  TTCN_EncDec::EB_WARNING,
  TTCN_EncDec::EB_WARNING,
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_IGNORE,
  TTCN_EncDec::EB_WARNING,
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_ERROR
};

TTCN_EncDec::error_behavior_t
TTCN_EncDec::error_behavior[TTCN_EncDec::ET_ALL] = {
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_WARNING,
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_WARNING,
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_WARNING,
  TTCN_EncDec::EB_WARNING,
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_WARNING,
  TTCN_EncDec::EB_WARNING,
  TTCN_EncDec::EB_WARNING,
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_IGNORE,
  TTCN_EncDec::EB_WARNING,
  TTCN_EncDec::EB_ERROR,
  TTCN_EncDec::EB_ERROR
};

TTCN_EncDec::error_type_t TTCN_EncDec::last_error_type=ET_NONE;
char *TTCN_EncDec::error_str=NULL;

static TTCN_EncDec::error_type_t& operator++(TTCN_EncDec::error_type_t& eb)
{
  return eb
    =(eb==TTCN_EncDec::ET_NONE)
    ?TTCN_EncDec::ET_UNDEF
    :TTCN_EncDec::error_type_t(eb+1);
}

void TTCN_EncDec::set_error_behavior(error_type_t p_et, error_behavior_t p_eb)
{
  if(p_et<ET_UNDEF || p_et>ET_ALL || p_eb<EB_DEFAULT || p_eb>EB_IGNORE)
    TTCN_error("EncDec::set_error_behavior(): Invalid parameter.");
  if(p_eb==EB_DEFAULT) {
    if(p_et==ET_ALL)
      for(error_type_t i=ET_UNDEF; i<ET_ALL; ++i)
        error_behavior[i]=default_error_behavior[i];
    else
      error_behavior[p_et]=default_error_behavior[p_et];
  }
  else {
    if(p_et==ET_ALL)
      for(error_type_t i=ET_UNDEF; i<ET_ALL; ++i)
        error_behavior[i]=p_eb;
    else
      error_behavior[p_et]=p_eb;
  }
}

TTCN_EncDec::error_behavior_t
TTCN_EncDec::get_error_behavior(error_type_t p_et)
{
  if(p_et<ET_UNDEF || p_et>=ET_ALL)
    TTCN_error("EncDec::get_error_behavior(): Invalid parameter.");
  return error_behavior[p_et];
}

TTCN_EncDec::error_behavior_t
TTCN_EncDec::get_default_error_behavior(error_type_t p_et)
{
  if(p_et<ET_UNDEF || p_et>=ET_ALL)
    TTCN_error("EncDec::get_error_behavior(): Invalid parameter.");
  return default_error_behavior[p_et];
}

void TTCN_EncDec::clear_error()
{
  last_error_type=ET_NONE;
  Free(error_str); error_str=NULL;
}

void TTCN_EncDec::error(error_type_t p_et, char *msg)
{
  last_error_type=p_et;
  Free(error_str);
  error_str=msg;
  if (p_et >= ET_UNDEF && p_et < ET_ALL) {
    switch(error_behavior[p_et]) {
    case TTCN_EncDec::EB_ERROR:
      TTCN_error("%s", error_str);
    case TTCN_EncDec::EB_WARNING:
      TTCN_warning("%s", error_str);
      break;
    default:
      break;
    } // switch
  }
}

TTCN_EncDec_ErrorContext *TTCN_EncDec_ErrorContext::head=NULL;
TTCN_EncDec_ErrorContext *TTCN_EncDec_ErrorContext::tail=NULL;

TTCN_EncDec_ErrorContext::TTCN_EncDec_ErrorContext()
{
  msg=NULL;
  if(!head) head=this;
  if(tail) tail->next=this;
  prev=tail;
  next=0;
  tail=this;
}

TTCN_EncDec_ErrorContext::TTCN_EncDec_ErrorContext(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  msg=mprintf_va_list(fmt, args);
  va_end(args);
  if(!head) head=this;
  if(tail) tail->next=this;
  prev=tail;
  next=0;
  tail=this;
}

TTCN_EncDec_ErrorContext::~TTCN_EncDec_ErrorContext()
{
  Free(msg);
  if(tail!=this)
    TTCN_error("Internal error:"
               " TTCN_EncDec_ErrorContext::~TTCN_EncDec_ErrorContext()");
  if(prev) prev->next=NULL;
  else head=NULL;
  tail=prev;
}

void TTCN_EncDec_ErrorContext::set_msg(const char *fmt, ...)
{
  Free(msg);
  va_list args;
  va_start(args, fmt);
  msg=mprintf_va_list(fmt, args);
  va_end(args);
}

void TTCN_EncDec_ErrorContext::error(TTCN_EncDec::error_type_t p_et,
                                     const char *fmt, ...)
{
  char *err_msg=NULL;
  for(TTCN_EncDec_ErrorContext *p=head; p!=NULL; p=p->next)
    err_msg=mputstr(err_msg, p->msg);
  va_list parameters;
  va_start(parameters, fmt);
  err_msg=mputprintf_va_list(err_msg, fmt, parameters);
  va_end(parameters);
  TTCN_EncDec::error(p_et, err_msg);
}

void TTCN_EncDec_ErrorContext::error_internal(const char *fmt, ...)
{
  char *err_msg=mcopystr("Internal error: ");
  for(TTCN_EncDec_ErrorContext *p=head; p!=NULL; p=p->next)
    err_msg=mputstr(err_msg, p->msg);
  va_list parameters;
  va_start(parameters, fmt);
  err_msg=mputprintf_va_list(err_msg, fmt, parameters);
  va_end(parameters);
  TTCN_EncDec::error(TTCN_EncDec::ET_INTERNAL, err_msg);
  TTCN_error("%s", TTCN_EncDec::get_error_str());
}

void TTCN_EncDec_ErrorContext::warning(const char *fmt, ...)
{
  char *warn_msg=NULL;
  for(TTCN_EncDec_ErrorContext *p=head; p!=NULL; p=p->next)
    warn_msg=mputstr(warn_msg, p->msg);
  va_list parameters;
  va_start(parameters, fmt);
  warn_msg=mputprintf_va_list(warn_msg, fmt, parameters);
  va_end(parameters);
  TTCN_warning("%s", warn_msg);
  Free(warn_msg);
}

#define INITIAL_SIZE 1024
#define MEMORY_SIZE(n) (sizeof(buffer_struct) - sizeof(int) + (n))

void TTCN_Buffer::reset_buffer()
{
  buf_pos = 0;
  bit_pos = 0;
  last_bit_pos = 0;
  last_bit_bitpos = 0;
  start_of_ext_bit = 0;
  last_bit = FALSE;
  current_bitorder = FALSE;
  ext_bit_reverse = FALSE;
  ext_level = 0;
}

void TTCN_Buffer::release_memory()
{
  if (buf_ptr != NULL) {
    if (buf_ptr->ref_count > 1) buf_ptr->ref_count--;
    else if (buf_ptr->ref_count == 1) Free(buf_ptr);
    else {
      TTCN_EncDec_ErrorContext::error_internal("Invalid reference counter %u "
	"when freeing a TTCN_Buffer.", buf_ptr->ref_count);
    }
  }
}

size_t TTCN_Buffer::get_memory_size(size_t target_size)
{
  size_t new_size = INITIAL_SIZE;
  while (new_size < target_size) {
    size_t next_size = new_size + new_size;
    if (next_size > new_size) new_size = next_size;
    else {
      // integer overflow occurred
      return static_cast<size_t>(-1);
    }
  }
  return new_size;
}

void TTCN_Buffer::copy_memory()
{
  if (buf_ptr != NULL && buf_ptr->ref_count > 1) {
    buffer_struct *old_ptr = buf_ptr;
    old_ptr->ref_count--;
    buf_size = get_memory_size(buf_len);
    buf_ptr = (buffer_struct*)Malloc(MEMORY_SIZE(buf_size));
    buf_ptr->ref_count = 1;
    memcpy(buf_ptr->data_ptr, old_ptr->data_ptr, buf_len);
  }
}

void TTCN_Buffer::increase_size(size_t size_incr)
{
  if (buf_ptr != NULL) {
    size_t target_size = buf_len + size_incr;
    if (target_size < buf_len)
      TTCN_EncDec_ErrorContext::error_internal("TTCN_Buffer: Overflow error "
	"(cannot increase buffer size)."); // unsigned overflow
    if (buf_ptr->ref_count > 1) { // shared, need to split (copy-on-write)
      buffer_struct *old_ptr = buf_ptr;
      old_ptr->ref_count--;
      buf_size = get_memory_size(target_size);
      buf_ptr = (buffer_struct*)Malloc(MEMORY_SIZE(buf_size));
#ifndef NDEBUG
      memset(buf_ptr->data_ptr,0, buf_size);
#endif
      buf_ptr->ref_count = 1;
      memcpy(buf_ptr->data_ptr, old_ptr->data_ptr, buf_len);
    } else if (target_size > buf_size) { // not shared, just change the size
      buf_size = get_memory_size(target_size);
      buf_ptr = (buffer_struct*)Realloc(buf_ptr, MEMORY_SIZE(buf_size));
#ifndef NDEBUG
      memset(buf_ptr->data_ptr + buf_len,0, buf_size - buf_len);
#endif
    }
  } else { // a brand new buffer
    buf_size = get_memory_size(size_incr);
    buf_ptr = (buffer_struct*)Malloc(MEMORY_SIZE(buf_size));
#ifndef NDEBUG
      memset(buf_ptr->data_ptr,0, buf_size);
#endif
    buf_ptr->ref_count = 1;
  }
}

TTCN_Buffer::TTCN_Buffer()
{
  buf_ptr = NULL;
  buf_size = 0;
  buf_len = 0;
  reset_buffer();
}

TTCN_Buffer::TTCN_Buffer(const TTCN_Buffer& p_buf)
{
  buf_ptr = p_buf.buf_ptr;
  buf_ptr->ref_count++;
  buf_size = p_buf.buf_size;
  buf_len = p_buf.buf_len;
  reset_buffer();
}

TTCN_Buffer::TTCN_Buffer(const OCTETSTRING& p_os)
{
  p_os.must_bound("Initializing a TTCN_Buffer with an unbound octetstring "
    "value.");
  buf_ptr = (buffer_struct*)p_os.val_ptr;
  buf_ptr->ref_count++;
  buf_size = p_os.val_ptr->n_octets;
  buf_len = p_os.val_ptr->n_octets;
  reset_buffer();
}

TTCN_Buffer::TTCN_Buffer(const CHARSTRING& p_cs)
{
  p_cs.must_bound("Initializing a TTCN_Buffer with an unbound charstring "
    "value.");
  buf_ptr = (buffer_struct*)p_cs.val_ptr;
  buf_ptr->ref_count++;
  buf_size = p_cs.val_ptr->n_chars + 1;
  buf_len = p_cs.val_ptr->n_chars;
  reset_buffer();
}

TTCN_Buffer& TTCN_Buffer::operator=(const TTCN_Buffer& p_buf)
{
  if (&p_buf != this) {
    release_memory();
    buf_ptr = p_buf.buf_ptr;
    buf_ptr->ref_count++;
    buf_size = p_buf.buf_size;
    buf_len = p_buf.buf_len;
  }
  reset_buffer();
  return *this;
}

TTCN_Buffer& TTCN_Buffer::operator=(const OCTETSTRING& p_os)
{
  p_os.must_bound("Assignment of an unbound octetstring value to a "
    "TTCN_Buffer.");
  release_memory();
  buf_ptr = (buffer_struct*)p_os.val_ptr;
  buf_ptr->ref_count++;
  buf_size = p_os.val_ptr->n_octets;
  buf_len = p_os.val_ptr->n_octets;
  reset_buffer();
  return *this;
}

TTCN_Buffer& TTCN_Buffer::operator=(const CHARSTRING& p_cs)
{
  p_cs.must_bound("Assignment of an unbound charstring value to a "
    "TTCN_Buffer.");
  release_memory();
  buf_ptr = (buffer_struct*)p_cs.val_ptr;
  buf_ptr->ref_count++;
  buf_size = p_cs.val_ptr->n_chars + 1;
  buf_len = p_cs.val_ptr->n_chars;
  reset_buffer();
  return *this;
}

void TTCN_Buffer::clear()
{
  release_memory();
  buf_ptr = NULL;
  buf_size = 0;
  buf_len = 0;
  reset_buffer();
}

const unsigned char* TTCN_Buffer::get_data() const
{
  if (buf_ptr != NULL) return buf_ptr->data_ptr;
  else return NULL;
}

const unsigned char* TTCN_Buffer::get_read_data() const
{
  if (buf_ptr != NULL) return buf_ptr->data_ptr + buf_pos;
  else return NULL;
}

void TTCN_Buffer::set_pos(size_t new_pos)
{
  if (new_pos < buf_len) buf_pos = new_pos;
  else buf_pos = buf_len;
}

void TTCN_Buffer::increase_pos(size_t delta)
{
  size_t new_buf_pos=buf_pos+delta;
  if(new_buf_pos<buf_pos || new_buf_pos>buf_len)
    buf_pos=buf_len;
  else
    buf_pos=new_buf_pos;
}

void TTCN_Buffer::get_end(unsigned char*& end_ptr, size_t& end_len)
{
  increase_size(end_len);
  end_len = buf_size - buf_len;
  if (buf_ptr != NULL) end_ptr = buf_ptr->data_ptr + buf_len;
  else end_ptr = NULL;
}

void TTCN_Buffer::increase_length(size_t size_incr)
{
  if (buf_size < buf_len + size_incr) increase_size(size_incr);
  buf_len += size_incr;
}

void TTCN_Buffer::put_c(unsigned char c)
{
  increase_size(1);
  buf_ptr->data_ptr[buf_len] = c;
  buf_len++;
}

void TTCN_Buffer::put_s(size_t len, const unsigned char *s)
{
  if (len > 0) {
    increase_size(len);
    memcpy(buf_ptr->data_ptr + buf_len, s, len);
    buf_len += len;
  }
}

void TTCN_Buffer::put_string(const OCTETSTRING& p_os)
{
  p_os.must_bound("Appending an unbound octetstring value to a TTCN_Buffer.");
  if (p_os.val_ptr->n_octets > 0) {
    if (buf_len > 0) {
      increase_size(p_os.val_ptr->n_octets);
      memcpy(buf_ptr->data_ptr + buf_len, p_os.val_ptr->octets_ptr,
	p_os.val_ptr->n_octets);
      buf_len += p_os.val_ptr->n_octets;
    } else {
      release_memory();
      buf_ptr = (buffer_struct*)p_os.val_ptr;
      buf_ptr->ref_count++;
      buf_size = p_os.val_ptr->n_octets;
      buf_len = p_os.val_ptr->n_octets;
    }
  }
}

void TTCN_Buffer::put_string(const CHARSTRING& p_cs)
{
  p_cs.must_bound("Appending an unbound charstring value to a TTCN_Buffer.");
  if (p_cs.val_ptr->n_chars > 0) { // there is something in the CHARSTRING
    if (buf_len > 0) { // there is something in this buffer, append
      increase_size(p_cs.val_ptr->n_chars);
      memcpy(buf_ptr->data_ptr + buf_len, p_cs.val_ptr->chars_ptr,
	p_cs.val_ptr->n_chars);
      buf_len += p_cs.val_ptr->n_chars;
    } else { // share the data
      release_memory();
      buf_ptr = (buffer_struct*)p_cs.val_ptr;
      buf_ptr->ref_count++;
      buf_size = p_cs.val_ptr->n_chars + 1;
      buf_len = p_cs.val_ptr->n_chars;
    }
  }
}

void TTCN_Buffer::put_buf(const TTCN_Buffer& p_buf) {
  if (p_buf.buf_ptr == 0) return;
  if (p_buf.buf_len > 0) { // there is something in the other buffer
    if (buf_len > 0) { // there is something in this buffer, append
      increase_size(p_buf.buf_len);
      memcpy(buf_ptr->data_ptr + buf_len, p_buf.buf_ptr->data_ptr,
        p_buf.buf_len);
      buf_len += p_buf.buf_len;
    }
    else { // share the data
      *this = p_buf;
    }
  }
}

void TTCN_Buffer::get_string(OCTETSTRING& p_os)
{
  p_os.clean_up();
  if (buf_len > 0) {
    if (buf_ptr->ref_count > 1) {
      p_os.init_struct(buf_len);
      memcpy(p_os.val_ptr->octets_ptr, buf_ptr->data_ptr, buf_len);
    } else {
      if (buf_size != buf_len) {
	buf_ptr = (buffer_struct*)Realloc(buf_ptr, MEMORY_SIZE(buf_len));
	buf_size = buf_len;
      }
      p_os.val_ptr = (OCTETSTRING::octetstring_struct*)buf_ptr;
      p_os.val_ptr->ref_count++;
      p_os.val_ptr->n_octets = buf_len;
    }
  } else p_os.init_struct(0);
}

void TTCN_Buffer::get_string(CHARSTRING& p_cs)
{
  p_cs.clean_up();
  if (buf_len > 0) {
    if (buf_ptr->ref_count > 1) { // buffer is shared, copy is needed
      p_cs.init_struct(buf_len);
      memcpy(p_cs.val_ptr->chars_ptr, buf_ptr->data_ptr, buf_len);
    } else { // we are the sole owner
      // Share our buffer_struct with CHARSTRING's charstring_struct
      // (they have the same layout), after putting in a string terminator.
      if (buf_size != buf_len + 1) {
	buf_ptr = (buffer_struct*)Realloc(buf_ptr, MEMORY_SIZE(buf_len + 1));
	buf_size = buf_len + 1;
      }
      p_cs.val_ptr = (CHARSTRING::charstring_struct*)buf_ptr;
      p_cs.val_ptr->ref_count++;
      p_cs.val_ptr->n_chars = buf_len;
      p_cs.val_ptr->chars_ptr[buf_len] = '\0';
    }
  } else p_cs.init_struct(0);
}

void TTCN_Buffer::get_string(UNIVERSAL_CHARSTRING& p_cs)
{
  p_cs.clean_up();
  if (buf_len > 0) {
    // TODO what if not multiple of 4 ?
    p_cs.init_struct(buf_len / 4);
    memcpy(p_cs.val_ptr->uchars_ptr, buf_ptr->data_ptr, buf_len);
  } else p_cs.init_struct(0);
}

void TTCN_Buffer::cut()
{
  if (buf_pos > 0) {
    if (buf_pos > buf_len)
      TTCN_EncDec_ErrorContext::error_internal("Read pointer points beyond "
	"the buffer end when cutting from a TTCN_Buffer.");
    size_t new_len = buf_len - buf_pos;
    if (new_len > 0) {
      if (buf_ptr->ref_count > 1) {
	buffer_struct *old_ptr = buf_ptr;
	old_ptr->ref_count--;
	buf_size = get_memory_size(new_len);
	buf_ptr = (buffer_struct*)Malloc(MEMORY_SIZE(buf_size));
	buf_ptr->ref_count = 1;
	memcpy(buf_ptr->data_ptr, old_ptr->data_ptr + buf_pos, new_len);
      } else {
	memmove(buf_ptr->data_ptr, buf_ptr->data_ptr + buf_pos, new_len);
	size_t new_size = get_memory_size(new_len);
	if (new_size < buf_size) {
	  buf_ptr = (buffer_struct*)Realloc(buf_ptr, MEMORY_SIZE(new_size));
	  buf_size = new_size;
	}
      }
    } else {
      release_memory();
      buf_ptr = NULL;
      buf_size = 0;
    }
    buf_len = new_len;
  }
  reset_buffer();
}

void TTCN_Buffer::cut_end()
{
  if (buf_pos > buf_len)
    TTCN_EncDec_ErrorContext::error_internal("Read pointer points beyond "
      "the buffer end when cutting from a TTCN_Buffer.");
  if (buf_pos < buf_len) {
    if (buf_pos > 0) {
      if (buf_ptr == NULL)
	TTCN_EncDec_ErrorContext::error_internal("Data pointer is NULL when "
	  "cutting from a TTCN_Buffer.");
      if (buf_ptr->ref_count == 1) {
	size_t new_size = get_memory_size(buf_pos);
	if (new_size < buf_size) {
	  buf_ptr = (buffer_struct*)Realloc(buf_ptr, MEMORY_SIZE(new_size));
	  buf_size = new_size;
	}
      }
    } else {
      release_memory();
      buf_ptr = NULL;
      buf_size = 0;
    }
    buf_len = buf_pos;
  }
  last_bit_pos = 0;
  last_bit_bitpos = 0;
  start_of_ext_bit = 0;
  last_bit = FALSE;
  current_bitorder = FALSE;
  ext_bit_reverse = FALSE;
  ext_level = 0;
}

boolean TTCN_Buffer::contains_complete_TLV()
{
  if (buf_len > buf_pos) {
    ASN_BER_TLV_t tmp_tlv;
    return ASN_BER_str2TLV(buf_len - buf_pos, buf_ptr->data_ptr + buf_pos,
                           tmp_tlv, BER_ACCEPT_ALL);
  } else return FALSE;
}

void TTCN_Buffer::log() const
{
  TTCN_Logger::log_event("Buffer: size: %lu, pos: %lu, len: %lu data: (",
    (unsigned long)buf_size, (unsigned long)buf_pos, (unsigned long)buf_len);
  if (buf_len > 0) {
    const unsigned char *data_ptr = buf_ptr->data_ptr;
    for(size_t i=0; i<buf_pos; i++)
      TTCN_Logger::log_octet(data_ptr[i]);
    TTCN_Logger::log_event_str(" | ");
    for(size_t i=buf_pos; i<buf_len; i++)
      TTCN_Logger::log_octet(data_ptr[i]);
  }
  TTCN_Logger::log_char(')');
}

void TTCN_Buffer::put_b(size_t len, const unsigned char *s,
                        const RAW_coding_par& coding_par, int align)
{

  unsigned char* st=NULL;
  unsigned char* st2=NULL;
  int loc_align=align<0?-align:align;
  boolean must_align=FALSE;
  raw_order_t local_bitorder=coding_par.bitorder;
  raw_order_t local_fieldorder=coding_par.fieldorder;
  if(current_bitorder) {
    if(local_bitorder==ORDER_LSB) local_bitorder=ORDER_MSB;
    else local_bitorder=ORDER_LSB;
    if(local_fieldorder==ORDER_LSB) local_fieldorder=ORDER_MSB;
    else local_fieldorder=ORDER_LSB;
  }
/*printf("len:%d\r\n",len);
printf("align:%d\r\n",align);
printf("coding bito:%s,byte:%s,field:%s\r\n",coding_par.bitorder==ORDER_MSB?"M":"L",
coding_par.byteorder==ORDER_MSB?"M":"L",
coding_par.fieldorder==ORDER_MSB?"M":"L"
);
printf("local bito:%s,field:%s\r\n",local_bitorder==ORDER_MSB?"M":"L",
local_fieldorder==ORDER_MSB?"M":"L"
);*/
  if(align) {
    if((local_fieldorder==ORDER_LSB && (local_bitorder!=coding_par.byteorder))||
     (local_fieldorder==ORDER_MSB && (local_bitorder==coding_par.byteorder))) {
      st=(unsigned char*)Malloc((len+loc_align+7)/8*sizeof(unsigned char));
      memset(st,0,(len+loc_align+7)/8*sizeof(unsigned char));
      if(align>0){
        memcpy(st,s,(len+7)/8*sizeof(unsigned char));
        if(len%8) st[(len+7)/8-1]&=BitMaskTable[len%8];
      }
      else{
        if(loc_align%8){
          int bit_bound=loc_align%8;
          size_t max_index=(len+loc_align+7)/8-loc_align/8-1;
          unsigned char* ptr=st+loc_align/8;
          unsigned char mask=BitMaskTable[bit_bound];
          for(size_t a=0;a<(len+7)/8;a++){
            ptr[a]&=mask;
            ptr[a]|=s[a]<<(8-bit_bound);
            if(a<max_index) ptr[a+1]=s[a]>>bit_bound;
          }
        }
        else{
          memcpy(st+loc_align/8,s,(len+7)/8*sizeof(unsigned char));
        }
      }
      s=st;
      len+=loc_align;
    }
    else{
      if(coding_par.byteorder==ORDER_MSB) align=-align;
      if(align<0) put_zero(loc_align,local_fieldorder);
      else must_align=TRUE;
    }
  }
  if(len==0) {
    if(must_align) put_zero(loc_align,local_fieldorder);
    return;
  }
  size_t new_size=((bit_pos==0?buf_len*8:buf_len*8-(8-bit_pos))+len+7)/8;
  size_t new_bit_pos=(bit_pos+len)%8;
  if (new_size > buf_len) increase_size(new_size - buf_len);
  else copy_memory();
  unsigned char *data_ptr = buf_ptr != NULL ? buf_ptr->data_ptr : NULL;
//printf("buf_len:%d bit_pos:%d\r\n",buf_len,bit_pos);
//printf("new_size:%d new_bit_pos:%d\r\n",new_size,new_bit_pos);
  if(coding_par.hexorder==ORDER_MSB){
    st2=(unsigned char*)Malloc((len+7)/8*sizeof(unsigned char));
    if(bit_pos==4){
      st2[0]=s[0];
      for(size_t a=1;a<(len+7)/8;a++){
        unsigned char ch='\0';
        ch|=s[a-1]>>4;
        st2[a-1]=(st2[a-1]&0x0f)|(s[a]<<4);
        st2[a]=(s[a]&0xf0)|ch;
      }
    }
    else{
      for(size_t a=0;a<(len+7)/8;a++) st2[a]=(s[a]<<4)|(s[a]>>4);
      if(len%8) st2[(len+7)/8]>>=4;
    }
    s=st2;
  }
  if(bit_pos+len<=8){  // there is enough space within 1 octet to store the data
    if(local_bitorder==ORDER_LSB){
      if(local_fieldorder==ORDER_LSB){
        data_ptr[new_size-1]=
          (data_ptr[new_size-1]&BitMaskTable[bit_pos])|
                                                               (s[0]<<bit_pos);
      }else{
        data_ptr[new_size-1]=
          (data_ptr[new_size-1]&~BitMaskTable[8-bit_pos])|
                 ((s[0]&BitMaskTable[len])<<(8-bit_pos-len));
      }
    }
    else{
      if(local_fieldorder==ORDER_LSB){
        data_ptr[new_size-1]=
          (data_ptr[new_size-1]&BitMaskTable[bit_pos])|
                                       (REVERSE_BITS(s[0])>>(8-len-bit_pos));
      }else{
        data_ptr[new_size-1]=
          (data_ptr[new_size-1]&~BitMaskTable[8-bit_pos])|
                 (REVERSE_BITS(s[0]&BitMaskTable[len])>>bit_pos);
      }
    }
  }
  else if(bit_pos==0 && (len%8)==0){ // octet aligned data
    if(coding_par.byteorder==ORDER_LSB){
      if(local_bitorder==ORDER_LSB){
        memcpy(data_ptr+buf_len, s, len/8);
      }
      else{
        unsigned char *prt=data_ptr+buf_len;
        for(size_t a=0;a<len/8;a++) prt[a]=REVERSE_BITS(s[a]);
      }
    }
    else{
      if(local_bitorder==ORDER_LSB){
        unsigned char *prt=data_ptr+buf_len;
        for(size_t a=0,b=len/8-1;a<len/8;a++,b--) prt[a]=s[b];
      }
      else{
        unsigned char *prt=data_ptr+buf_len;
        for(size_t a=0,b=len/8-1;a<len/8;a++,b--) prt[a]=REVERSE_BITS(s[b]);
      }
    }
  }
  else{
    size_t maxindex=new_size-1;
    if(coding_par.byteorder==ORDER_LSB){
      if(local_bitorder==ORDER_LSB){
        if(bit_pos){
          unsigned char mask1=BitMaskTable[bit_pos];
          unsigned char *prt=data_ptr+(buf_len==0?0:buf_len-1);
          if(local_fieldorder==ORDER_MSB){
            unsigned int num_bytes = (len+7) / 8;
            unsigned int active_bits_in_last = len % 8;
            if(!active_bits_in_last) active_bits_in_last=8;
            for(unsigned int a=0; a < num_bytes; a++){
              prt[a]&=REVERSE_BITS(mask1);
              unsigned char sa = s[a];
              if (a == num_bytes - 1) { // last byte
                sa <<= (8 - active_bits_in_last);
                // push bits up so the first active bit is in MSB
              }
              prt[a]|=(sa>>bit_pos)&~REVERSE_BITS(mask1);
              if(a<maxindex)
                prt[a+1]=sa<<(8-bit_pos);
            }
          }
          else{
            for(unsigned int a=0;a<(len+7)/8;a++){
              prt[a]&=mask1;
              prt[a]|=s[a]<<bit_pos;
              if(a<maxindex)
                prt[a+1]=s[a]>>(8-bit_pos);
            }
          }
        }
        else{  // start from octet boundary
          memcpy(data_ptr+buf_len, s, (len+7)/8*sizeof(unsigned char));
          if(local_fieldorder==ORDER_MSB  && new_bit_pos){
              data_ptr[new_size-1]<<=(8-new_bit_pos);
            
          }
        }
      }
      else{ // bitorder==ORDER_MSB
        if(bit_pos){
          unsigned char mask1=REVERSE_BITS(BitMaskTable[bit_pos]);
          unsigned char *prt=data_ptr+(buf_len==0?0:buf_len-1);
          if(local_fieldorder==ORDER_MSB){
            prt[0]&=mask1;
            prt[0]|=REVERSE_BITS(s[0])>>bit_pos;
            prt[1]=REVERSE_BITS(s[0])<<(8-bit_pos);
          }
          else{
            prt[0]&=REVERSE_BITS(mask1);
            prt[0]|=REVERSE_BITS(s[0])&~REVERSE_BITS(mask1);
            prt[1]=REVERSE_BITS(s[0])<<(8-bit_pos);
          }
          for(unsigned int a=1;a<(len+7)/8;a++){
            prt[a]&=mask1;
            prt[a]|=REVERSE_BITS(s[a])>>bit_pos;
            if(a<maxindex)
              prt[a+1]=REVERSE_BITS(s[a])<<(8-bit_pos);
          }
        }
        else{  // start from octet boundary
          unsigned char *prt=data_ptr+buf_len;
          for(unsigned int a=0;a<(len+7)/8;a++) prt[a]=REVERSE_BITS(s[a]);
        }
        if(local_fieldorder==ORDER_LSB && new_bit_pos)
              data_ptr[new_size-1]>>=(8-new_bit_pos);
      }
    }
    else{ // byteorder==ORDER_MSB
      if(local_bitorder==ORDER_LSB){
        if(bit_pos){
          unsigned char mask1=BitMaskTable[bit_pos];
          unsigned char ch=get_byte_rev(s,len,0);
          unsigned char *prt=data_ptr+(buf_len==0?0:buf_len-1);
          if(local_fieldorder==ORDER_MSB){
            prt[0]&=REVERSE_BITS(mask1);
            prt[0]|=ch>>bit_pos;
            prt[1]=ch<<(8-bit_pos);
          }
          else{
            prt[0]&=mask1;
            prt[0]|=ch&~mask1;
            prt[1]=ch<<(8-bit_pos);
          }
          for(unsigned int a=1;a<(len+7)/8;a++){
            ch=get_byte_rev(s,len,a);
            prt[a]&=REVERSE_BITS(mask1);
            prt[a]|=ch>>bit_pos;
            if(a<maxindex)
              prt[a+1]=ch<<(8-bit_pos);
          }
        }
        else{
          unsigned char *prt=data_ptr+buf_len;
          for(unsigned int a=0;a<(len+7)/8;a++) prt[a]=get_byte_rev(s,len,a);
        }
        if(local_fieldorder==ORDER_LSB && new_bit_pos)
            data_ptr[new_size-1]>>=(8-new_bit_pos);
      }
      else{  // bitorder==ORDER_MSB
        if(bit_pos){
          unsigned char mask1=BitMaskTable[bit_pos];
          unsigned char ch=get_byte_rev(s,len,0);
          unsigned char *prt=data_ptr+(buf_len==0?0:buf_len-1);
          if(local_fieldorder==ORDER_MSB){
            prt[0]&=REVERSE_BITS(mask1);
            prt[0]|=REVERSE_BITS(ch)&~REVERSE_BITS(mask1);
            prt[1]=REVERSE_BITS(ch)>>(8-bit_pos);
          }
          else{
            prt[0]&=mask1;
            prt[0]|=REVERSE_BITS(ch)<<bit_pos;
            prt[1]=REVERSE_BITS(ch)>>(8-bit_pos);
          }
          for(unsigned int a=1;a<(len+7)/8;a++){
            ch=get_byte_rev(s,len,a);
            prt[a]&=mask1;
            prt[a]|=REVERSE_BITS(ch)<<bit_pos;
            if(a<maxindex)
              prt[a+1]=REVERSE_BITS(ch)>>(8-bit_pos);
          }
        }
        else{  // start from octet boundary
          unsigned char *prt=data_ptr+buf_len;
          for(unsigned int a=0;a<(len+7)/8;a++) prt[a]=
                                            REVERSE_BITS(get_byte_rev(s,len,a));
        }
        if(local_fieldorder==ORDER_MSB && new_bit_pos)
            data_ptr[new_size-1]<<=(8-new_bit_pos);
      }
    }
  }
  if(st) Free(st);
  if(st2) Free(st2);
/*  last_bit_pos=((bit_pos==0?buf_len*8:buf_len*8-(8-bit_pos))+len+6)/8;
  if(local_fieldorder==ORDER_LSB)
    last_bit_bitpos=(bit_pos+len-1)%8;
  else
    last_bit_bitpos=7-(bit_pos+len-1)%8;*/
  buf_len=new_size;
  bit_pos=new_bit_pos;
  if(bit_pos){
    last_bit_pos=buf_len-1;
    if(local_fieldorder==ORDER_LSB)
      last_bit_bitpos=bit_pos-1;
    else
      last_bit_bitpos=7-(bit_pos-1);
  }
  else{
    last_bit_pos=buf_len-1;
    if(local_fieldorder==ORDER_LSB)
      last_bit_bitpos=7;
    else
      last_bit_bitpos=0;
  }
  if(must_align) put_zero(loc_align,local_fieldorder);
}

void TTCN_Buffer::get_b(size_t len, unsigned char *s,
    const RAW_coding_par& coding_par, raw_order_t top_bit_order)
{
  if(len==0) return;
  size_t new_buf_pos=buf_pos+(bit_pos+len)/8;
  size_t new_bit_pos=(bit_pos+len)%8;
  raw_order_t local_bitorder=coding_par.bitorder;
  raw_order_t local_fieldorder=coding_par.fieldorder;
  if(top_bit_order==ORDER_LSB){
    if(local_bitorder==ORDER_LSB) local_bitorder=ORDER_MSB;
    else local_bitorder=ORDER_LSB;
    if(local_fieldorder==ORDER_LSB) local_fieldorder=ORDER_MSB;
    else local_fieldorder=ORDER_LSB;
  }
  const unsigned char *data_ptr = buf_ptr != NULL ? buf_ptr->data_ptr : NULL;
  if(bit_pos+len<=8){  // the data is within 1 octet
    if(local_bitorder==ORDER_LSB){
      if(local_fieldorder==ORDER_LSB){
        s[0]=data_ptr[buf_pos]>>bit_pos;
      }else{
        s[0]=data_ptr[buf_pos]>>(8-bit_pos-len);
      }
    }
    else{
      if(local_fieldorder==ORDER_LSB){
        s[0]=REVERSE_BITS(data_ptr[buf_pos])>>(8-bit_pos-len);
      }else{
        s[0]=REVERSE_BITS(data_ptr[buf_pos])>>bit_pos;
      }
    }
  }
  else if(bit_pos==0 && (len%8)==0){ // octet aligned data
    if(coding_par.byteorder==ORDER_LSB){
      if(local_bitorder==ORDER_LSB){
        memcpy(s, data_ptr+buf_pos, len/8*sizeof(unsigned char));
      }
      else{
        const unsigned char *prt=data_ptr+buf_pos;
        for(size_t a=0;a<len/8;a++) s[a]=REVERSE_BITS(prt[a]);
      }
    }
    else{
      if(local_bitorder==ORDER_LSB){
        const unsigned char *prt=data_ptr+buf_pos;
        for(size_t a=0,b=len/8-1;a<len/8;a++,b--) s[a]=prt[b];
      }
      else{
        const unsigned char *prt=data_ptr+buf_pos;
        for(size_t a=0,b=len/8-1;a<len/8;a++,b--) s[a]=REVERSE_BITS(prt[b]);
      }
    }
  }
  else{ // unaligned
    size_t num_bytes = (len + 7) / 8;
    if(coding_par.byteorder==ORDER_LSB){
      if(local_bitorder==ORDER_LSB){
        if(bit_pos){
          unsigned char mask1=BitMaskTable[8-bit_pos];
          if(local_fieldorder==ORDER_LSB){
            for(unsigned int a=0;a<num_bytes;a++){
              s[a]=(get_byte_align(len,local_fieldorder,ORDER_MSB,a+1)
                                                                 <<(8-bit_pos))|
                   ((get_byte_align(len,local_fieldorder,ORDER_MSB,a)
                                                              >>bit_pos)&mask1);
            }
          } else {
            mask1=BitMaskTable[bit_pos];
            for(unsigned int a=0;a<num_bytes;a++){
              s[a]=(get_byte_align(len,local_fieldorder,ORDER_LSB,a+1)
                                                                 >>(8-bit_pos)&mask1)|
                   ((get_byte_align(len,local_fieldorder,ORDER_LSB,a)
                                                              <<bit_pos));
            }
            int active_bits_in_last_byte = len % 8;
            if (active_bits_in_last_byte) {
              s[num_bytes - 1] >>= (8 - active_bits_in_last_byte);
            }
          }
        }
        else{  // start from octet boundary
          memcpy(s, data_ptr+buf_pos, num_bytes*sizeof(unsigned char));
          if(local_fieldorder==ORDER_MSB && new_bit_pos)
             s[num_bytes-1]>>=(8-new_bit_pos);
        }
      }
      else{ // bitorder==ORDER_MSB
        if(bit_pos){
          unsigned char mask1=BitMaskTable[bit_pos];
          for(unsigned int a=0;a<num_bytes;a++){
            s[a]=REVERSE_BITS(
                 ((get_byte_align(len,local_fieldorder,ORDER_LSB,a+1)
                                                        >>(8-bit_pos))&mask1)|
                 (get_byte_align(len,local_fieldorder,ORDER_LSB,a)
                                                           <<bit_pos));
          }
        }
        else{  // start from octet boundary
          const unsigned char *prt=data_ptr+buf_pos;
          for(unsigned int a=0;a<num_bytes;a++) s[a]=REVERSE_BITS(prt[a]);
          if(local_fieldorder==ORDER_LSB && new_bit_pos)
              s[num_bytes-1]>>=(8-new_bit_pos);
        }
      }
    }
    else{ // byteorder==ORDER_MSB
      if(local_bitorder==ORDER_LSB){
        if(new_bit_pos){
          unsigned char mask1=BitMaskTable[new_bit_pos];
          for(unsigned int a=0,b=(bit_pos+len)/8;a<num_bytes;a++,b--){
            s[a]=((get_byte_align(len,local_fieldorder,ORDER_LSB,b)
                                                     >>(8-new_bit_pos))&mask1)|
                 (get_byte_align(len,local_fieldorder,ORDER_LSB,b-1)
                                                           <<new_bit_pos);
          }
        }
        else{
//          unsigned char *prt=data_ptr+buf_pos;
          for(unsigned int a=0,b=new_buf_pos-1;a<num_bytes;a++,b--)
                        s[a]=data_ptr[b];
          if(local_fieldorder==ORDER_LSB && bit_pos)
            s[num_bytes-1]>>=bit_pos;
        }
      }
      else{  // bitorder==ORDER_MSB
        if(new_bit_pos){
//          unsigned char mask1=BitMaskTable[new_bit_pos];
          for(unsigned int a=0,b=(bit_pos+len)/8;a<num_bytes;a++,b--){
            s[a]=REVERSE_BITS(
                 (get_byte_align(len,local_fieldorder,ORDER_MSB,b)
                                                     <<(8-new_bit_pos))|
                 (get_byte_align(len,local_fieldorder,ORDER_MSB,b-1)
                                                     >>new_bit_pos));
          }
        }
        else{  // start from octet boundary
//          unsigned char *prt=data_ptr+buf_pos;
          for(unsigned int a=0,b=new_buf_pos-1;a<num_bytes;a++,b--)
                        s[a]=REVERSE_BITS(data_ptr[b]);
          if(local_fieldorder==ORDER_MSB && bit_pos)
            s[num_bytes-1]>>=bit_pos;
        }
      }
    }
  }
  if(coding_par.hexorder==ORDER_MSB){
    if(bit_pos==4){
      for(size_t a=1;a<(len+7)/8;a++){
        unsigned char ch='\0';
        ch|=s[a-1]>>4;
        s[a-1]=(s[a-1]&0x0f)|(s[a]<<4);
        s[a]=(s[a]&0xf0)|ch;
      }
    }
    else{
      for(size_t a=0;a<(len+7)/8;a++) s[a]=(s[a]<<4)|(s[a]>>4);
      if(len%8) s[(len+7)/8]>>=4;
    }
  }

  size_t last_bit_offset = bit_pos + len - 1;
  unsigned char last_bit_octet = data_ptr[buf_pos + last_bit_offset / 8];
  if (local_fieldorder == ORDER_LSB) last_bit_octet >>= last_bit_offset % 8;
  else last_bit_octet >>= 7 - last_bit_offset % 8;
  if (last_bit_octet & 0x01) last_bit = TRUE;
  else last_bit = FALSE;

  buf_pos=new_buf_pos;
  bit_pos=new_bit_pos;
}

void TTCN_Buffer::put_zero(size_t len, raw_order_t fieldorder)
{
  if(len==0) return;
  size_t new_size=((bit_pos==0?buf_len*8:buf_len*8-(8-bit_pos))+len+7)/8;
  if (new_size > buf_len) increase_size(new_size - buf_len);
  else copy_memory();
  unsigned char *data_ptr = buf_ptr != NULL ? buf_ptr->data_ptr : NULL;
  if(bit_pos){
    if(bit_pos+len>8){
      unsigned char mask1=BitMaskTable[bit_pos];
      unsigned char *prt=data_ptr+(buf_len==0?0:buf_len-1);
      if(fieldorder==ORDER_LSB) prt[0]&=mask1;
      else prt[0]&=~mask1;
      memset(prt+1, 0, (len-1+bit_pos)/8);
    }
    else {
      if(fieldorder==ORDER_LSB)
        data_ptr[new_size-1]=data_ptr[new_size-1]&BitMaskTable[bit_pos];
      else
        data_ptr[new_size-1]=data_ptr[new_size-1]&
                                            REVERSE_BITS(BitMaskTable[bit_pos]);
    }
  }
  else {
    memset(data_ptr+buf_len, 0, (len+7)/8);
  }
  buf_len=new_size;
  bit_pos=(bit_pos+len)%8;
  if(bit_pos){
    last_bit_pos=buf_len-1;
    if(fieldorder==ORDER_LSB)
      last_bit_bitpos=bit_pos-1;
    else
      last_bit_bitpos=7-(bit_pos-1);
  }
  else{
    last_bit_pos=buf_len-1;
    if(fieldorder==ORDER_LSB)
      last_bit_bitpos=7;
    else
      last_bit_bitpos=0;
  }
}

const unsigned char* TTCN_Buffer::get_read_data(size_t &bitpos) const
{
  bitpos=bit_pos;
  if (buf_ptr != NULL) return buf_ptr->data_ptr + buf_pos;
  else return NULL;
}

void TTCN_Buffer::set_pos(size_t pos, size_t bitpos)
{
  buf_pos=pos<buf_len?pos:buf_len;
  bit_pos=bitpos;
}


void TTCN_Buffer::set_pos_bit(size_t new_bit_pos)
{
  size_t new_pos = new_bit_pos / 8;
  if (new_pos < buf_len) {
    buf_pos = new_pos;
    bit_pos = new_bit_pos % 8;
  } else {
    buf_pos = buf_len;
    bit_pos = 0;
  }
}

void TTCN_Buffer::increase_pos_bit(size_t delta)
{
  size_t new_buf_pos=buf_pos+(bit_pos+delta)/8; // bytes
  if(new_buf_pos<buf_pos || new_buf_pos>buf_len) {
    buf_pos=buf_len;
    bit_pos=7;
  }
  else {
    buf_pos=new_buf_pos;
    bit_pos=(bit_pos+delta)%8;
  }
}

int TTCN_Buffer::increase_pos_padd(int padding)
{
  if(padding) { //         <---old bit pos--->
    size_t new_bit_pos = ((buf_pos*8 + bit_pos + padding-1)/padding) * padding;
    int padded = new_bit_pos - buf_pos * 8 - bit_pos;
    //  padded = bits skipped to reach the next multiple of padding (bits)
    buf_pos = new_bit_pos / 8;
    bit_pos = new_bit_pos % 8;
    return padded;
  }else
    return 0;
}

size_t TTCN_Buffer::unread_len_bit()
{
  return (buf_len-buf_pos)*8-bit_pos;
}

void TTCN_Buffer::start_ext_bit(boolean p_reverse)
{
  if (ext_level++ == 0) {
    start_of_ext_bit = buf_len;
    ext_bit_reverse = p_reverse;
  }
}

void TTCN_Buffer::stop_ext_bit()
{
  if (ext_level <= 0)
    TTCN_EncDec_ErrorContext::error_internal("TTCN_Buffer::stop_ext_bit() "
      "was called without start_ext_bit().");
  if (--ext_level == 0) {
    unsigned char one = current_bitorder ? 0x01 : 0x80;
    unsigned char zero= ~one;
    unsigned char *data_ptr = buf_ptr != NULL ? buf_ptr->data_ptr : NULL;
    if (ext_bit_reverse) {
      for(size_t a=start_of_ext_bit;a<buf_len-1;a++) data_ptr[a] |= one;
      data_ptr[buf_len-1] &= zero;
    } else {
      for(size_t a=start_of_ext_bit;a<buf_len-1;a++) data_ptr[a] &= zero;
      data_ptr[buf_len-1] |= one;
    }
  }
}

void TTCN_Buffer::put_pad(size_t len, const unsigned char *s,
                          int pat_len, raw_order_t fieldorder)
{
  if(len==0) return;
  if(pat_len==0){
    put_zero(len,fieldorder);
    return;
  }
  RAW_coding_par cp;
  cp.bitorder=ORDER_LSB;
  cp.byteorder=ORDER_LSB;
  cp.fieldorder=fieldorder;
  cp.hexorder=ORDER_LSB;
  int length=len;
  while(length>0){
    put_b(length>pat_len?pat_len:length,s,cp,0);
    length-=pat_len;
  }
}

void TTCN_Buffer::set_last_bit(boolean p_last_bit)
{
  unsigned char *last_bit_ptr = buf_ptr->data_ptr + last_bit_pos;
  unsigned char bitmask = 0x01 << last_bit_bitpos;
  if (p_last_bit) *last_bit_ptr |= bitmask;
  else *last_bit_ptr &= ~bitmask;
}

unsigned char TTCN_Buffer::get_byte_rev(const unsigned char* data,
                                        size_t len, size_t idx)
{
  unsigned char ch='\0';
  size_t hossz=(len+7)/8-1;
  int bit_limit=len%8;
  if(idx>hossz) return ch;
  if(bit_limit==0)return data[hossz-idx];
  ch=data[hossz-idx]<<(8-bit_limit);
  if((hossz-idx)>0) ch|=(data[hossz-idx-1]>>bit_limit)
                        &BitMaskTable[8-bit_limit];
  return ch;
}

unsigned char TTCN_Buffer::get_byte_align(size_t len,
                                          raw_order_t fieldorder,
                                          raw_order_t req_align,
                                          size_t idx)
{
  if(idx>(bit_pos+len)/8) return '\0';
  const unsigned char *data_ptr = buf_ptr != NULL ? buf_ptr->data_ptr : NULL;
  if(idx==0){ // first byte
    if(fieldorder==req_align){
      if(fieldorder==ORDER_LSB){
        return data_ptr[buf_pos]>>bit_pos;
      }
      else {return data_ptr[buf_pos]<<bit_pos;}
    }
    else {return data_ptr[buf_pos];}
  }
  if(idx==(bit_pos+len)/8){ // last byte
    if(fieldorder==req_align){
      if(fieldorder==ORDER_LSB){
        return data_ptr[buf_pos+idx]<<(8-(bit_pos+len)%8);
      }
      else {return data_ptr[buf_pos+idx]>>(8-(bit_pos+len)%8);}
    }
    else {return data_ptr[buf_pos+idx];}
  }
  return data_ptr[buf_pos+idx];
}
