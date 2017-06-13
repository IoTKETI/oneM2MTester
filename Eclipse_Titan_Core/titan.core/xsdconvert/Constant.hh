/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
#include "SimpleType.hh"

#ifndef CONSTANT_HH_
#define CONSTANT_HH_

class Constant : public RootType {
  
  SimpleType* parent; // not owned
  Mstring value; // The value of the constant
  // Points to the constant which have the same type and value as this
  Constant* alterego; // not owned
  bool checked;
  
  public:  
    Constant(SimpleType* p_parent, Mstring p_type, Mstring p_value);
    Constant(Constant &); // Not implemented
    Constant & operator=(Constant &); // Not implemented
    bool operator==(const Constant &other) const {
      return (parent->getModule() == other.parent->getModule() &&
              type.convertedValue == other.type.convertedValue &&
              value == other.value);
    }
    
    void loadWithValues() {}
    void printToFile(FILE * file);
    void dump(const unsigned int) const;
    void nameConversion(const NameConversionMode, const List<NamespaceType> &);
    void nameConversion_types(const List<NamespaceType> & ns);
    // Here the value is converted to a TTCN value string which matches the constant type.
    void finalModification();
    
    // Returns the maybe qualified final name of the constant.
    Mstring getConstantName(const TTCN3Module* other_mod) const;
    
    Constant * getAlterego() {
      if (alterego != NULL) {
        return alterego;
      } else {
        return this;
      }
    }
    
    // Remove 'equal' constants and give them names.
    static void finalFinalModification(List<RootType*> constantDefs);
};

#endif /* CONSTANT_HH_ */

