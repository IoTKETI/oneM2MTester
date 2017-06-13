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
#ifndef PATH_HH_
#define PATH_HH_

#include <string>


class Path {
private:
  Path();
public:
  static const char SEPARATOR; 

  static std::string normalize(const std::string& original);

  /* Creates a normalized absolute path from the given filename.
   * The relative path will be resolved according to
   * the current working directory. */
  static std::string get_abs_path(const std::string& fname);

  /* Returns the filename from the path. (The suffix after the last '/')
   * e.g.: "abc/def" -> "def"
   *       "abc"     -> "abc"
   *       "abc/"   -> ""
   */
  static std::string get_file(const std::string& path);
  /* Returns the directory part of the given path.
   * e.g.:  "/a/b/cde -> "/a/b/"
   *        "abc"     -> ""
   *        "../abc"  -> "../"
   *        "/"       -> "/"    */
  static std::string get_dir(const std::string& path);

  static std::string compose(const std::string& path1, const std::string& path2);

  static bool is_absolute(const std::string& path);
};

#endif
