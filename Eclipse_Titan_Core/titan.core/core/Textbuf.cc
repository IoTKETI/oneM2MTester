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
 *   Szalai, Gabor
 *
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h> // for atof
#include <string.h>
#include <errno.h>

#include "Textbuf.hh"
#include "../common/memory.h"
#include "Error.hh"

#include <openssl/bn.h>

// Note: Do not use dbgnew.hh; it doesn't play well with Qt in mctr_gui

#define BUF_SIZE 1000
#define BUF_HEAD 24


Text_Buf::Text_Buf()
: buf_size()
, buf_begin(BUF_HEAD)
, buf_pos(BUF_HEAD)
, buf_len(0)
{
  Allocate(BUF_SIZE);
}

Text_Buf::~Text_Buf()
{
  Free(data_ptr);
}

void Text_Buf::Allocate(int size)
{
  int new_buf_size = BUF_SIZE + BUF_HEAD;
  while(new_buf_size < size + buf_begin) new_buf_size *= 2;
  data_ptr = Malloc(new_buf_size);
  buf_size = new_buf_size; // always a power of 2, from 1024 up
}

void Text_Buf::Reallocate(int size)
{
  int new_buf_size = BUF_SIZE + BUF_HEAD;
  while (new_buf_size < size + buf_begin) new_buf_size *= 2;
  if (new_buf_size != buf_size) {
    data_ptr = Realloc(data_ptr, new_buf_size);
    buf_size = new_buf_size; // always a power of 2, from 1024 up
  }
}

void Text_Buf::reset()
{
  buf_begin = BUF_HEAD;
  Reallocate(BUF_SIZE);
  buf_pos = BUF_HEAD;
  buf_len = 0;
}

/** Encode a native integer in the buffer
 *
 * @param value native integer
 */
void Text_Buf::push_int(const RInt& value)
{
  int_val_t tmp(value);
  push_int(tmp);
}

/** Encode an integer (may be bigint) into the text buffer
 *
 * @param value may be big integer
 */
void Text_Buf::push_int(const int_val_t& value)
{
  if (value.is_native()) {
    boolean is_negative = value < 0;
    unsigned int unsigned_value = is_negative ? -value.get_val() :
      value.get_val();
    unsigned int bytes_needed = 1;
    for (unsigned int tmp = unsigned_value >> 6; tmp != 0; tmp >>= 7)
      bytes_needed++;
    Reallocate(buf_len + bytes_needed);
    unsigned char *buf = (unsigned char *)data_ptr + buf_begin + buf_len;
    for (unsigned int i = bytes_needed - 1; ; i--) {
      // The top bit is always 1 for a "middle" byte, 0 for the last byte.
      // That leaves 7 bits, except for the first byte where the 2nd highest
      // bit is the sign bit, so only 6 payload bits are available.
      if (i > 0) {
        buf[i] = unsigned_value & 0x7f;
        unsigned_value >>= 7;
      } else buf[i] = unsigned_value & 0x3f;
      // Set the top bit for all but the last byte
      if (i < bytes_needed - 1) buf[i] |= 0x80;
      if (i == 0) break;
    }
    if (is_negative) buf[0] |= 0x40; // Put in the sign bit
    buf_len += bytes_needed;
  } else {
    BIGNUM *D = BN_new();
    BN_copy(D, value.get_val_openssl());
    unsigned num_bits = BN_num_bits(D);
    // Calculation
    // first 6 bit +the sign bit are stored in the first octet
    // the remaining bits stored in 7 bit group + continuation bit
    // So how many octest needed to store the (num_bits + 1) many bits
    // in 7 bit ber octet form?
    // ((num_bits+1)+6)/7 =>
    // (num_bits+7)/7 =>
    // (num_bits / 7)+1
    
    unsigned num_bytes = (num_bits / 7)+1;
    Reallocate(buf_len + num_bytes);
    unsigned char *buf = (unsigned char *)data_ptr + buf_begin + buf_len;
    // Alloc once, free once
    unsigned char* buf2 = (unsigned char*)Malloc(BN_num_bytes(D) * sizeof(unsigned char));
    for (unsigned i = num_bytes - 1; ; i--) {
      BN_bn2bin(D, buf2); // TODO: query once and then get the 7 loads
      unsigned temp_num_bytes = BN_num_bytes(D);
      // Seven bits at a time, except the first byte has only 6 payload bits
      if (i > 0) {
        buf[i] = buf2[temp_num_bytes-1] & 0x7f;
        if (!BN_rshift(D, D, 7)) return;
      } else {
          buf[i] = (BN_is_zero(D) ? 0 : buf2[temp_num_bytes-1]) & 0x3f;
      }
      if (i < num_bytes - 1) buf[i] |= 0x80;
      if (i == 0) break;
    }
    if (BN_is_negative(D)) buf[0] |= 0x40; // Put in the sign bit
    BN_free(D);
    Free(buf2);
    buf_len += num_bytes;
  }
}

/** Extract an integer from the buffer.
 *
 * @return the extracted value
 * @pre An integer must be available, else dynamic testcase error
 */
const int_val_t Text_Buf::pull_int()
{
  int_val_t value;
  if (!safe_pull_int(value))
    TTCN_error("Text decoder: Decoding of integer failed.");
  return value;
}

/** Extract an integer if it's safe to do so.
 *
 * @param[out] value set to the extracted value if successful
 * @return TRUE if an integer could be extracted, FALSE otherwise
 */
boolean Text_Buf::safe_pull_int(int_val_t& value)
{
  int buf_end = buf_begin + buf_len;
  if (buf_pos >= buf_end) return FALSE;
  int pos = buf_pos;
  // Count continuation flags.
  while (pos < buf_end && ((unsigned char *)data_ptr)[pos] & 0x80) pos++;
  if (pos >= buf_end) return FALSE;
  unsigned int bytes_needed = pos - buf_pos + 1;
  const unsigned char *buf = (unsigned char *)data_ptr + buf_pos;
  if (bytes_needed > sizeof(RInt)) {
    BIGNUM *D = BN_new();
    int neg = 0;
    BN_clear(D);
    for (unsigned i = 0; i < bytes_needed; i++) {
      // TTCN-TCC-INTERNAL-0026 (HJ87126)
      if (i > 0) BN_add_word(D, buf[i] & 0x7f);
      else BN_add_word(D, buf[i] & 0x3f);
      if (i < bytes_needed - 1)
        BN_lshift(D, D, 7);
    }
    if (buf[0] & 0x40) { neg = 1; BN_set_negative(D, 1); }
    if (BN_num_bits(D) > (RInt)sizeof(RInt) * 8 - 1) {
      value = int_val_t(D);
    } else {
      BN_ULONG num = BN_get_word(D); // BN_ULONG is unsigned long
      value = int_val_t(neg ? -num : num);
      BN_free(D);
    }
  } else {
    unsigned long loc_value = 0;
    for (unsigned i = 0; i < bytes_needed; i++) {
      if (i > 0) loc_value |= buf[i] & 0x7f;
      else loc_value |= buf[i] & 0x3f;
      if (i < bytes_needed - 1) loc_value <<= 7;
    }
    if (buf[0] & 0x40) value = -loc_value;
    else value = loc_value;
  }
  buf_pos = pos + 1;
  return TRUE;
}

/** Encode a double precision floating point number in the buffer.
 *
 * @param value
 */
void Text_Buf::push_double(double value)
{
  Reallocate(buf_len + 8);
  union{
    double d;
    unsigned char c[8];
  } m;
  m.d=value;
  unsigned char *st=(unsigned char *)data_ptr + buf_begin + buf_len;
#if defined __sparc__ || defined __sparc
    st[0]=m.c[0];
    st[1]=m.c[1];
    st[2]=m.c[2];
    st[3]=m.c[3];
    st[4]=m.c[4];
    st[5]=m.c[5];
    st[6]=m.c[6];
    st[7]=m.c[7];
#else
    st[0]=m.c[7];
    st[1]=m.c[6];
    st[2]=m.c[5];
    st[3]=m.c[4];
    st[4]=m.c[3];
    st[5]=m.c[2];
    st[6]=m.c[1];
    st[7]=m.c[0];
#endif
  buf_len += 8;
}

/** Extract a double precision floating point number
 *
 * @return the extracted value
 * @pre A suitably formatted float value must be in the buffer, else
 * dynamic testcase error
 */
double Text_Buf::pull_double()
{
  if (buf_pos + 8 > buf_begin + buf_len) TTCN_error("Text decoder: Decoding of float failed. "
    "(End of buffer reached)");
  const unsigned char *st = (unsigned char *)data_ptr+buf_pos;

  union{
    double d;
    unsigned char c[8];
  } m;
#if defined __sparc__ || defined __sparc
    m.c[0]=st[0];
    m.c[1]=st[1];
    m.c[2]=st[2];
    m.c[3]=st[3];
    m.c[4]=st[4];
    m.c[5]=st[5];
    m.c[6]=st[6];
    m.c[7]=st[7];
#else
    m.c[0]=st[7];
    m.c[1]=st[6];
    m.c[2]=st[5];
    m.c[3]=st[4];
    m.c[4]=st[3];
    m.c[5]=st[2];
    m.c[6]=st[1];
    m.c[7]=st[0];
#endif
buf_pos += 8;
return m.d;
}

/** Write a fixed number of bytes in the buffer.
 *
 * @param len number of bytes to write
 * @param data pointer to the data
 */
void Text_Buf::push_raw(int len, const void *data)
{
  if (len < 0) TTCN_error("Text encoder: Encoding raw data with negative "
                          "length (%d).", len);
  Reallocate(buf_len + len);
  memcpy((char*)data_ptr + buf_begin + buf_len, data, len);
  buf_len += len;
}

void Text_Buf::push_raw_front(int len, const void* data)
{
  if (len < 0) TTCN_error("Text encoder: Encoding raw data with negative "
                          "length (%d).", len);
  Reallocate(buf_len + len);
  for (int i = buf_len - 1; i >= 0; --i) {
    ((char*)data_ptr)[buf_begin + len + i] = ((char*)data_ptr)[buf_begin + i];
  }
  memcpy((char*)data_ptr + buf_begin, data, len);
  buf_len += len;
}

/** Extract a fixed number of bytes from the buffer.
 *
 * @param len number of bytes to read
 * @param data pointer to the data
 * @pre at least \a len bytes are available, else dynamic testcase error
 */
void Text_Buf::pull_raw(int len, void *data)
{
  if (len < 0) TTCN_error("Text decoder: Decoding raw data with negative "
                          "length (%d).", len);
  if (buf_pos + len > buf_begin + buf_len)
    TTCN_error("Text decoder: End of buffer reached.");
  memcpy(data, (char*)data_ptr + buf_pos, len);
  buf_pos += len;
}

/** Write a 0-terminated string
 *
 * Writes the length followed by the raw bytes (no end marker)
 *
 * @param string_ptr pointer to the string
 */
void Text_Buf::push_string(const char *string_ptr)
{
  if (string_ptr != NULL) {
    int len = strlen(string_ptr);
    push_int(len);
    push_raw(len, string_ptr);
  } else push_int((RInt)0);
}

/** Extract a string
 *
 * @return the string allocated with new[], must be freed by the caller
 */
char *Text_Buf::pull_string()
{
  int len = pull_int().get_val();
  if (len < 0)
    TTCN_error("Text decoder: Negative string length (%d).", len);
  char *string_ptr = new char[len + 1];
  pull_raw(len, string_ptr);
  string_ptr[len] = '\0';
  return string_ptr;
}

/// Push two strings
void Text_Buf::push_qualified_name(const qualified_name& name)
{
  push_string(name.module_name);
  push_string(name.definition_name);
}

/// Extract two strings
void Text_Buf::pull_qualified_name(qualified_name& name)
{
  name.module_name = pull_string();
  if (name.module_name[0] == '\0') {
    delete [] name.module_name;
    name.module_name = NULL;
  }
  name.definition_name = pull_string();
  if (name.definition_name[0] == '\0') {
    delete [] name.definition_name;
    name.definition_name = NULL;
  }
}

/** Calculate the length of the buffer and write it at the beginning.
 *
 */
void Text_Buf::calculate_length()
{
  unsigned int value = buf_len;
  unsigned int bytes_needed = 1;
  for (unsigned int tmp = value >> 6; tmp != 0; tmp >>= 7) bytes_needed++;
  if ((unsigned int)buf_begin < bytes_needed)
    TTCN_error("Text encoder: There is not enough space to calculate message "
	"length.");
  unsigned char *buf = (unsigned char*)data_ptr + buf_begin - bytes_needed;
  for (unsigned int i = bytes_needed - 1; ; i--) {
    if (i > 0) {
      buf[i] = value & 0x7F;
      value >>= 7;
    } else buf[i] = value & 0x3F;
    if (i < bytes_needed - 1) buf[i] |= 0x80;
    if (i == 0) break;
  }
  buf_begin -= bytes_needed;
  buf_len += bytes_needed;
}


void Text_Buf::get_end(char*& end_ptr, int& end_len)
{
  int buf_end = buf_begin + buf_len;
  if (buf_size - buf_end < BUF_SIZE) Reallocate(buf_len + BUF_SIZE);
  end_ptr = (char*)data_ptr + buf_end;
  end_len = buf_size - buf_end;
}

void Text_Buf::increase_length(int add_len)
{
  if (add_len < 0) TTCN_error("Text decoder: Addition is negative (%d) when "
                              "increasing length.", add_len);
  if (buf_begin + buf_len + add_len > buf_size)
    TTCN_error("Text decoder: Addition is too big when increasing length.");
  buf_len += add_len;
}

/** Check if a known message is in the buffer
 *
 * @return TRUE if an int followed by the number of bytes specified
 * by that int is in the buffer; FALSE otherwise.
 * @post buf_pos == buf_begin
 */
boolean Text_Buf::is_message()
{
  rewind();
  int_val_t msg_len;
  boolean ret_val = FALSE;
  if (safe_pull_int(msg_len)) {
    if (msg_len < 0) {
      char *tmp_str = msg_len.as_string();
      TTCN_error("Text decoder: Negative message length (%s).", tmp_str);
      Free(tmp_str);  // ???
    }
    ret_val = buf_pos + msg_len.get_val() <= buf_begin + buf_len;
  }
  rewind();
  return ret_val;
}

/** Overwrite the extracted message with the rest of the buffer
 *  @post buf_pos == buf_begin
 */
void Text_Buf::cut_message()
{
  if (is_message()) {
    int msg_len = pull_int().get_val();
    int msg_end = buf_pos + msg_len;
    buf_len -= msg_end - buf_begin;
    memmove((char*)data_ptr + buf_begin, (char*)data_ptr + msg_end,
	    buf_len);
    Reallocate(buf_len);
    rewind();
  }
}
