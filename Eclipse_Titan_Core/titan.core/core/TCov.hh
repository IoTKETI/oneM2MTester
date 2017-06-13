/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *
 ******************************************************************************/
#ifndef TCOV_HH
#define TCOV_HH

#include <sys/types.h> // for pid_t
#include "memory.h"
#include "Types.h" // for boolean

#include "Vector.hh"

class FunctionData
{
public:
  explicit FunctionData(const char *name = NULL, int pos = 0, int count = 0)
  : m_pos(pos), m_count(count) { if (name != NULL) m_name = mcopystr(name); }
  ~FunctionData() { Free(m_name); }
  inline const char *get_name() const { return m_name; }
  inline int get_pos() const { return m_pos; }
  inline int get_count() const { return m_count; }
  inline void reset() { m_count = 0; }
  FunctionData& operator++() { ++m_count; return *this; }
private:
  /// Copy constructor disabled
  FunctionData(const FunctionData&);
  /// %Assignment disabled
  FunctionData& operator=(const FunctionData&);
  char *m_name;
  int m_pos;  // For later improvement.
  int m_count;
};

class LineData
{
public:
  explicit LineData(int no = 0, int count = 0): m_no(no), m_count(count) { }
  inline int get_no() const { return m_no; }
  inline int get_count() const { return m_count; }
  inline void reset() { m_count = 0; }
  LineData& operator++() { ++m_count; return *this; }
private:
  /// Copy constructor disabled
  LineData(const LineData&);
  /// %Assignment disabled
  LineData& operator=(const LineData&);
  int m_no;
  int m_count;
};

class FileData
{
public:
  explicit FileData(const char *file_name);
  ~FileData();
  inline const char *get_file_name() const { return m_file_name; }
  inline const Vector<FunctionData *>& get_function_data() const { return m_function_data; }
  inline const Vector<LineData *>& get_line_data() const { return m_line_data; }
  void inc_function(const char *function_name, int line_no);
  void inc_line(int line_no);
  void init_function(const char *function_name);
  void init_line(int line_no);
  size_t has_line_no(int line_no);
  size_t has_function_name(const char *function_name);
  void reset();
private:
  /// Copy constructor disabled
  FileData(const FileData&);
  /// %Assignment disabled
  FileData& operator=(const FileData&);
  char *m_file_name;
  Vector<FunctionData *> m_function_data;
  Vector<LineData *> m_line_data;
};

// We all belong to the _same_ component.
class TCov
{
public:
  static void hit(const char *file_name, int line_no, const char *function_name = NULL);
  static void init_file_lines(const char *file_name, const int line_nos[], size_t line_nos_len);
  static void init_file_functions(const char *file_name, const char *function_names[], size_t function_names_len);
  static expstring_t comp(boolean withname = FALSE);
  static void close_file();
private:
  /// Copy constructor disabled
  TCov(const TCov& tcov);
  /// %Assignment disabled
  TCov& operator=(const TCov& tcov);
  static size_t has_file_name(const char *file_name);
  static void pid_check();

  static Vector<FileData *> m_file_data;
  static pid_t mypid;
  static expstring_t mycomp;
  static expstring_t mycomp_name;
  static int ver_major;
  static int ver_minor;
};

#endif  // TCOV_HH
