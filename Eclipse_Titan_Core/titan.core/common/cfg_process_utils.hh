/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Beres, Szabolcs
 *
 ******************************************************************************/
#ifndef CFG_PROCESS_UTILS_HH_
#define CFG_PROCESS_UTILS_HH_


#include <deque>
#include <string>

#include "Path2.hh"

template <typename T_BUFFER_STATE>
class IncludeElem {
public:
  std::string dir;
  std::string fname;
  FILE* fp;
  T_BUFFER_STATE buffer_state;
  int line_number;


  IncludeElem(const std::string& fname_)
    : dir(Path::get_dir(fname_)), fname(Path::get_file(fname_)),
      fp(NULL), buffer_state(NULL), line_number(-1) { }

  IncludeElem(const std::string& fname_, FILE* fp_)
    : dir(Path::get_dir(fname_)), fname(Path::get_file(fname_)),
      fp(fp_), buffer_state(NULL), line_number(-1) { }

  bool equals(const std::string& path) const {
    return Path::compose(dir, fname) == path;
  }

  std::string get_full_path() const {
    return Path::compose(dir, fname);
  }

};

template <typename T_BUFFER_STATE>
std::string dump_include_chain(const std::deque<IncludeElem<T_BUFFER_STATE> >& chain) {

  std::string result;
  if (chain.empty()) {
    return result;
  }

  typename std::deque<IncludeElem<T_BUFFER_STATE> >::const_iterator it = chain.begin();
  result.append(it->dir).append(it->fname);
  for (++it; it != chain.end(); ++it) {
    result.append("->");
    result.append(it->dir).append(it->fname);
  }
  return result;
}

template <typename T_BUFFER_STATE>
std::string switch_lexer(std::deque<IncludeElem<T_BUFFER_STATE> >* p_include_chain, 
    const std::string& include_file, T_BUFFER_STATE p_current_buffer,
    T_BUFFER_STATE (*p_yy_create_buffer)(FILE*, int),
    void (*p_yy_switch_to_buffer)(T_BUFFER_STATE),
    int p_current_line, int p_buf_size) {

  if (include_file.empty()) {
    return std::string("Empty file name.");
  } 

  std::string abs_path; 
  if (Path::is_absolute(include_file)) {
    abs_path = include_file;
  } else {
    abs_path = Path::normalize(Path::compose(p_include_chain->back().dir, include_file));
  }

  for (typename std::deque<IncludeElem<T_BUFFER_STATE> >::iterator it = p_include_chain->begin();
      it != p_include_chain->end(); ++it) {
    if (it->equals(abs_path)) {
      p_include_chain->push_back(IncludeElem<T_BUFFER_STATE>(abs_path));
      std::string error_msg("Circular import chain detected:\n");
      error_msg.append(dump_include_chain(*p_include_chain));
      p_include_chain->pop_back();
      return error_msg;
    }
  }

  p_include_chain->back().buffer_state = p_current_buffer;
  p_include_chain->back().line_number = p_current_line;

  FILE* fp = fopen(abs_path.c_str(), "r");
  if (!fp) {
    std::string error_msg("File not found: ");
    error_msg.append(abs_path);
    return error_msg;
  }

  IncludeElem<T_BUFFER_STATE> new_elem(abs_path, fp);
  p_include_chain->push_back(new_elem);
  new_elem.buffer_state = p_yy_create_buffer(fp, p_buf_size);
  p_yy_switch_to_buffer(new_elem.buffer_state);
  return std::string("");
}

#endif
