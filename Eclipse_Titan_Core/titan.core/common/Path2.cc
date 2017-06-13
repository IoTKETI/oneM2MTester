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
#include "Path2.hh"

#include "path.h"
//#include "dbgnew.hh"

using std::string;

const char Path::SEPARATOR = '/';

std::string Path::normalize(const std::string& original) {
  std::string result;
  bool last_slash = false;
  for (size_t i = 0; i < original.size(); ++i) {
    if (original[i] != SEPARATOR) {
      result += original[i];
      last_slash = false;
      continue;
    }

    if (!last_slash) {
      last_slash = true;
      result += original[i];
    } 
  }
  return result;
}

std::string Path::get_abs_path(const std::string& fname) {
  if (fname.empty()) {
    return std::string(1, SEPARATOR);
  }

  if (fname[0] == SEPARATOR) {
    return normalize(fname);
  }

  expstring_t working_dir = get_working_dir();
  std::string work_dir(working_dir);
  Free(working_dir);
  work_dir += SEPARATOR;
  work_dir.append(fname);
  return normalize(work_dir);
}

std::string Path::get_file(const std::string& path) {
  size_t idx = path.rfind(SEPARATOR);

  if (idx == string::npos) {
    return path;
  } 
  if (idx == path.size() - 1) {
    return string();
  }

  return path.substr(idx + 1);
}

string Path::get_dir(const string& path) {
  size_t idx = path.rfind(SEPARATOR);

  if (idx == string::npos) {
    return string();
  } 
  return path.substr(0, idx + 1);
}

string Path::compose(const string& path1, const string& path2) {
  if (path1.empty()) {
    return path2;
  }

  if (path2.empty()) {
    return path1;
  }

  string result = path1;
  if (result[result.size() - 1] != SEPARATOR
      && path2[0] != SEPARATOR) {
    result += SEPARATOR;
  }

  result.append(path2);

  return result;
}

bool Path::is_absolute(const string& path) {
  return (!path.empty() && path[0] == SEPARATOR);
}

