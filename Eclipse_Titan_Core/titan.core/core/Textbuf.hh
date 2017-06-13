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
#ifndef TEXTBUF_HH
#define TEXTBUF_HH

#include "Types.h"
#include "RInt.hh"

class Text_Buf {
private:
  int buf_size;  ///< amount of allocated memory
  int buf_begin; ///< number of reserved bytes
  int buf_pos;   ///< read position into the payload
  int buf_len;   ///< number of payload bytes
  void *data_ptr;

  void Allocate(int size);
  void Reallocate(int size);

  boolean safe_pull_int(int_val_t& value);
  /// Copy constructor disabled
  Text_Buf(const Text_Buf&);
  /// Assignment disabled
  Text_Buf& operator=(const Text_Buf&);
public:
  Text_Buf();
  ~Text_Buf();

  void reset();
  inline void rewind() { buf_pos = buf_begin; }

  inline int get_len() const { return buf_len; }
  inline int get_pos() const { return buf_pos - buf_begin; }
  inline void buf_seek(int new_pos) { buf_pos = buf_begin + new_pos; }
  inline const char *get_data() const
    { return reinterpret_cast<const char*>(data_ptr) + buf_begin; }

  void push_int(const int_val_t& value);
  void push_int(const RInt& value);
  const int_val_t pull_int();

  void push_double(double value);
  double pull_double();

  void push_raw(int len, const void *data);
  void push_raw_front(int len, const void *data);
  void pull_raw(int len, void *data);

  void push_string(const char *string_ptr);
  char *pull_string();

  void push_qualified_name(const qualified_name& name);
  void pull_qualified_name(qualified_name& name);

  void calculate_length();

  void get_end(char*& end_ptr, int& end_len);
  void increase_length(int add_len);
  boolean is_message();
  void cut_message();

};

#endif
