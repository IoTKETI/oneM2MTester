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
#ifndef TCOV2LCOV_HH
#define TCOV2LCOV_HH

#include <string>
#include <map>
#include <vector>

// File specific data.
class TcovData
{
public:
  void inc_function(const std::string& function, int n);
  void inc_line(int line, int n);
  inline const std::map<std::string, int>& get_functions() const { return m_functions; }
  inline const std::map<int, int>& get_lines() const { return m_lines; }
private:
  std::map<std::string, int> m_functions;
  std::map<int, int> m_lines;
};

class Tcov2Lcov {
public:
  Tcov2Lcov(const char *code_base, const char *input_dir, const char *output_file, const char *xsd_file);
  ~Tcov2Lcov();
  int collect();
  int validate() const;
  int merge();
  int generate();
  void d_print_files() const;
private:
  int collect_dir(std::string dir);

  std::map<std::string, TcovData *> m_data;
  std::vector<std::string> m_files;  // Relative paths.
  std::string m_code_base;
  std::string m_input_dir;
  std::string m_output_file;
  std::string m_xsd_file;
  int m_ver_major;
  int m_ver_minor;
};

#endif  // TCOV2LCOV_HH

