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
#ifndef GENERALFUNCTIONS_HH_
#define GENERALFUNCTIONS_HH_

#include "Mstring.hh"
#include "List.hh"
#include "GeneralTypes.hh"

typedef List<QualifiedName> QualifiedNames;

enum modeType {
    type_reference_name, type_name, field_name, enum_id_name
};

void XSDName2TTCN3Name(const Mstring& in, QualifiedNames & used_names, modeType type_of_the_name,
        Mstring & res, Mstring & variant, bool no_replace = false);

class ReferenceData;


bool isBuiltInType(const Mstring& in);
bool isStringType(const Mstring& in);
bool isIntegerType(const Mstring& in);
bool isFloatType(const Mstring& in);
bool isTimeType(const Mstring& in);
bool isSequenceType(const Mstring& in);
bool isBooleanType(const Mstring& in);
bool isQNameType(const Mstring& in);
bool isAnyType(const Mstring& in);

bool matchDates(const char * string, const char * type);
bool matchRegexp(const char * string, const char * pattern);

void printWarning(const Mstring& filename, int lineNumber, const Mstring& text);
void printWarning(const Mstring& filename, const Mstring& typeName, const Mstring& text);
void printError(const Mstring& filename, int lineNumber, const Mstring& text);
void printError(const Mstring& filename, const Mstring& typeName, const Mstring& text);
void indent(FILE * file, const unsigned int x);

// Converts an XML float value as string to a correct ttcn float value as string
Mstring xmlFloat2TTCN3FloatStr(const Mstring& xmlFloat);

long double stringToLongDouble(const char * input);

class RootType;
class SimpleType;
class TTCN3Module;

const Mstring& getNameSpaceByPrefix(const RootType * root, const Mstring& prefix);
const Mstring& getPrefixByNameSpace(const RootType * root, const Mstring& namespace_);

const Mstring findBuiltInType(const RootType * ref, Mstring type);

/// Lookup in a list of modules
RootType * lookup(const List<TTCN3Module*> mods,
        const SimpleType * reference, wanted w);
/// Lookup in a list of modules
RootType * lookup(const List<TTCN3Module*> mods,
        const Mstring& name, const Mstring& nsuri, const RootType *reference, wanted w);
/// Lookup inside one module
RootType *lookup1(const TTCN3Module *module,
        const Mstring& name, const Mstring& nsuri, const RootType *reference, wanted w);

int multi(const TTCN3Module *module, ReferenceData const& outside_reference,
        const RootType *obj);

void generate_TTCN3_header(FILE * file, const char* modulename, const bool timestamp = true);

inline unsigned long long llmin(unsigned long long l, unsigned long long r) {
    return l < r ? l : r;
}

inline unsigned long long llmax(unsigned long long l, unsigned long long r) {
    return l > r ? l : r;
}

#endif /* GENERALFUNCTIONS_HH_ */
