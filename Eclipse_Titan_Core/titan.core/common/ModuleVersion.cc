/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   
 *   Balasko, Jeno
 *
 ******************************************************************************/
#undef new

#include <new>
#include <sstream> 
#include "ModuleVersion.hh"
#include "dbgnew.hh"

std::string ModuleVersion::toString() const {
  std::stringstream stream;
  const char separator = ' ';
  if (!productNumber.empty()) {
    stream << productNumber;
  }

  if (suffix != 0) {
    stream << "/" << suffix;
  }
  if (release != 0) {
    stream << separator << 'R' << release << separator
      << static_cast<char>('A' + patch);
  }
  if (build != 0) {
    stream << separator << build;
  }

  if (!extra.empty()) {
    stream << extra;
  }
  return stream.str();
}

bool ModuleVersion::operator<(const ModuleVersion& other) const{
  return productNumber < other.productNumber
    && suffix < other.suffix
    && build < other.build
    && extra < other.extra;
}
