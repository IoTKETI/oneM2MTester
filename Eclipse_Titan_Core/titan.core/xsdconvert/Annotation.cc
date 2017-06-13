/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Godar, Marton
 *   Raduly, Csaba
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
#include "Annotation.hh"
#include "XMLParser.hh"

Annotation::Annotation(XMLParser * a_parser, TTCN3Module * a_module, ConstructType a_construct)
: RootType(a_parser, a_module, a_construct) {
}

void Annotation::loadWithValues() {
  switch (parser->getActualTagName()) {
    case n_label:
      addComment(Mstring("LABEL:"));
      break;
    case n_definition:
      addComment(Mstring("DEFINITION:"));
      break;
    default:
      break;
  }
}

void Annotation::printToFile(FILE * file) {
  printComment(file);
  fprintf(file, "\n\n");
}

void Annotation::dump(unsigned int depth) const {
  fprintf(stderr, "%*s Annotation at %p\n", depth * 2, "", (const void*) this);
}
