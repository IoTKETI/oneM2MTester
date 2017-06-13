/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Baji, Laszlo
 *   Balasko, Jeno
 *   Baranyi, Botond
 *   Godar, Marton
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
#include "GeneralFunctions.hh"
#include "SimpleType.hh"
#include "TTCN3Module.hh"
#include "ImportStatement.hh"
#include "converter.hh"

#include <cctype> // for using "toupper" function
#include <cstring>
#include <cstdio>
#include <regex.h>

#include "../common/version_internal.h"

#include <ctime>

#if defined(WIN32) && !defined(MINGW)
#include <cygwin/version.h>
#include <sys/cygwin.h>
#include <limits.h>
#endif

extern bool w_flag_used;
extern bool t_flag_used;

// XSDName2TTCN3Name function:
// Parameters:
// 				in - input string - XSD name
// 				used_names - set of previously defined types, used field names etc.
// 				type_of_the_name - mode of the function behaviour
//				res - generated result
// 				variant - generated variant string for TTCN-3
//

void XSDName2TTCN3Name(const Mstring& in, QualifiedNames & used_names, modeType type_of_the_name,
  Mstring & res, Mstring & variant, bool no_replace) {
  static const char* TTCN3_reserved_words[] = {
    "action", "activate", "address", "alive", "all", "alt", "altstep", "and", "and4b", "any", "any2unistr", "anytype", "apply",
    "bitstring", "boolean", "break",
    "call", "case", "catch", "char", "charstring", "check", "clear", "complement", "component", "connect",
    "const", "continue", "control", "create",
    "decmatch", "deactivate", "decvalue_unichar", "default", "derefers", "disconnect", "display", "do", "done",
    "else", "encode", "encvalue_unichar", "enumerated", "error", "except", "exception", "execute", "extends", "extension",
    "external",
    "fail", "false", "float", "for", "friend", "from", "function",
    "getverdict", "getcall", "getreply", "goto", "group",
    "halt", "hexstring", "hostid",
    "if", "if present", "import", "in", "inconc", "infinity", "inout", "integer", "interleave", "istemplatekind",
    "kill", "killed",
    "label", "language", "length", "log",
    "map", "match", "message", "mixed", "mod", "modifies", "module", "modulepar", "mtc",
    "noblock", "none", "not", "not4b", "nowait", "null",
    "objid", "octetstring", "of", "omit", "on", "optional", "or", "or4b", "out", "override",
    "param", "pass", "pattern", "permutation", "port", "present", "private", "procedure", "public",
    "raise", "read", "receive", "record", "recursive", "refers", "rem", "repeat", "reply", "return", "running", "runs",
    "select", "self", "send", "sender", "set", "setverdict", "signature", "start", "stop", "subset",
    "superset", "system",
    "template", "testcase", "timeout", "timer", "to", "trigger", "true", "type",
    "union", "universal", "unmap",
    "value", "valueof", "var", "variant", "verdicttype",
    "while", "with",
    "xor", "xor4b",
    NULL
  };
  static const char* TTCN3_predefined_functions[] = {
    "bit2int", "bit2hex", "bit2oct", "bit2str",
    "char2int", "char2oct",
    "decomp", "decvalue",
    "encvalue", "enum2int",
    "float2int", "float2str",
    "hex2bit", "hex2int", "hex2oct", "hex2str",
    "int2bit", "int2char", "int2enum", "int2float", "int2hex", "int2oct", "int2str", "int2unichar",
    "isvalue", "ischosen", "ispresent",
    "lengthof", "log2str",
    "oct2bit", "oct2char", "oct2hex", "oct2int", "oct2str", "oct2unichar"
    "regexp", "replace", "rnd", "remove_bom", "get_stringencoding",
    "sizeof", "str2bit", "str2float", "str2hex", "str2int", "str2oct", "substr",
    "testcasename",
    "unichar2int", "unichar2char", "unichar2oct",
    NULL
  };
  static const char* ASN1_reserved_words[] = {
    "ABSENT", "ABSTRACT-SYNTAX", "ALL", "APPLICATION", "AUTOMATIC",
    "BEGIN", "BIT", "BMPString", "BOOLEAN", "BY",
    "CHARACTER", "CHOICE", "CLASS", "COMPONENT", "COMPONENTS", "CONSTRAINED", "CONTAINING",
    "DEFAULT", "DEFINITIONS",
    "EMBEDDED", "ENCODED", "END", "ENUMERATED", "EXCEPT", "EXPLICIT", "EXPORTS", "EXTENSIBILITY",
    "EXTERNAL",
    "FALSE", "FROM",
    "GeneralizedTime", "GeneralString", "GraphicString",
    "IA5String", "IDENTIFIER", "IMPLICIT", "IMPLIED", "IMPORTS", "INCLUDES", "INSTANCE", "INTEGER",
    "INTERSECTION", "ISO646String",
    "MAX", "MIN", "MINUS-INFINITY",
    "NULL", "NumericString",
    "OBJECT", "ObjectDescriptor", "OCTET", "OF", "OPTIONAL",
    "PATTERN", "PDV", "PLUS-INFINITY", "PRESENT", "PrintableString", "PRIVATE",
    "REAL", "RELATIVE-OID",
    "SEQUENCE", "SET", "SIZE", "STRING", "SYNTAX",
    "T61String", "TAGS", "TeletexString", "TRUE", "TYPE-IDENTIFIER",
    "UNION", "UNIQUE", "UNIVERSAL", "UniversalString", "UTCTime", "UTF8String",
    "VideotexString", "VisibleString",
    "WITH",
    NULL
  };

  Mstring ns_uri(variant);

  res.clear();
  variant.clear();
  res = in;

  if (res.size() > 0) {
    /********************************************************
     * STEP 0 - recognizing XSD built-in types
     *
     * ******************************************************/
    // If the type or field reference name is an XSD built-in type then it will be capitalized and get a prefix "XSD."
    // if (type_of_the_name == type_reference_name || type_of_the_name == field_reference_name) {

    if (type_of_the_name == type_reference_name) {
      if (isBuiltInType(res)) {
        res[0] = (char)toupper(res[0]);
        res = "XSD." + res;
        return;
      }
      if (res == "record" ||
        res == "union" ||
        res == "set") {
        return;
      }
    }

    if (type_of_the_name == enum_id_name) {
      bool found = false;
      for (QualifiedNames::iterator used = used_names.begin(); used; used = used->Next) {
        QualifiedName tmp(empty_string, res);
        if (tmp.nsuri == used->Data.nsuri && tmp.orig_name == used->Data.orig_name) {
          found = true;
          break;
        }
      }
      if (found) return;
    }
    /********************************************************
     * STEP 1 - character changes
     *
     * according to
     * clause 5.2.2.1 - Generic name transformation rules
     * ******************************************************/
    // the characters ' '(SPACE), '.'(FULL STOP) and '-'(HYPEN-MINUS)shall all be replaced by a "_" (LOW LINE)
    for (size_t i = 0; i != res.size(); ++i) {
      if ((res[i] == ' ') ||
        (res[i] == '.' && !no_replace) ||
        (res[i] == '-')) {
        res[i] = '_';
      }
    }
    // any character except "A" to "Z", "a" to "z" or "0" to "9" and "_" shall be removed
    for (size_t i = 0; i != res.size(); ++i) {
      if (!isalpha((const unsigned char) res[i]) && !isdigit((const unsigned char) res[i]) && (res[i] != '_')) {
        if (!no_replace && res[i] != '.') {
          res.eraseChar(i);
          i--;
        }
      }
    }
    // a sequence of two of more "_" (LOW LINE) shall be replaced with a single "_" (LOW LINE)
    for (size_t i = 1; i < res.size(); ++i) {
      if (res[i] == '_' && res[i - 1] == '_') {
        res.eraseChar(i);
        i--;
      }
    }
    // "_" (LOW LINE) characters occurring at the end of the name shall be removed
    if (!res.empty() && res[res.size() - 1] == '_') {
      res.eraseChar(res.size() - 1);
    }
    // "_" (LOW LINE) characters occurring at the beginning of the name shall be removed
    if (!res.empty() && res[0] == '_') {
      res.eraseChar(0);
    }
  }

  switch (type_of_the_name) {
    case type_reference_name:
    case type_name:
      if (res.empty()) {
        res = "X";
      } else {
        if (islower((const unsigned char) res[0])) {
          res.setCapitalized();
        } else if (isdigit((const unsigned char) res[0])) {
          res.insertChar(0, 'X');
        }
      }
      break;
    case field_name:
    case enum_id_name:
      if (res.empty()) {
        res = "x";
      } else {
        if (isupper((const unsigned char) res[0])) res.setUncapitalized();
        else if (isdigit((const unsigned char) res[0])) res.insertChar(0, 'x');
      }
      break;
  }
  /********************************************************
   * STEP 2 - process if the generated name is
   * 					- already used
   * 					- a TTCN3 keyword
   * 					- an ASN.1 keyword
   * and after any change is made
   * according to
   * clause 5.2.2.2 - Succeeding rules
   * ******************************************************/
  /********************************************************
   * according to paragraph a)
   * ******************************************************/
  bool postfixing = false;
  QualifiedName qual_name(ns_uri, res, in);


  switch (type_of_the_name) {
      // Do not use "res" in this switch; only qual_name
    case type_name:
    {
      for (int k = 0; ASN1_reserved_words[k]; k++) {
        if (qual_name.name == ASN1_reserved_words[k]) {
          postfixing = true;
          break;
        }
      }

      for (QualifiedNames::iterator used = used_names.begin(); used; used = used->Next) {
        if (qual_name == used->Data) {
          postfixing = true;
          break;
        }
      }

      if (postfixing) {
        bool found = false;
        int counter = 1;
        expstring_t tmpname = NULL;
        do {
          found = false;
          Free(tmpname);
          tmpname = mprintf("%s_%d", qual_name.name.c_str(), counter);
          for (QualifiedNames::iterator used = used_names.begin(); used; used = used->Next) {
            if (QualifiedName(/* empty_string ? */ ns_uri, Mstring(tmpname)) == used->Data) {
              found = true;
              break;
            }
          }
          counter++;
        } while (found);
        qual_name.name = tmpname; // NULL will result in an empty string
        Free(tmpname);
        postfixing = false;
      }
      break;
    }
    case field_name:
    case enum_id_name:
      for (int k = 0; TTCN3_reserved_words[k]; k++) {
        if (qual_name.name == TTCN3_reserved_words[k]) postfixing = true;
      }
      for (int k = 0; TTCN3_predefined_functions[k]; k++) {
        if (qual_name.name == TTCN3_predefined_functions[k]) postfixing = true;
      }
      if (postfixing) {
        qual_name.name += "_";
        postfixing = false;
      }

      for (QualifiedNames::iterator used = used_names.begin(); used; used = used->Next) {
        if (qual_name == used->Data) postfixing = true;
      }

      if (postfixing) {
        bool found = false;
        int counter = 1;
        if (qual_name.name[qual_name.name.size() - 1] == '_')
          qual_name.name.eraseChar(qual_name.name.size() - 1);
        expstring_t tmpname = mprintf("%s_%d", qual_name.name.c_str(), counter);
        do {
          found = false;
          if (counter > 0) {
            Free(tmpname);
            tmpname = mprintf("%s_%d", qual_name.name.c_str(), counter);
          }
          for (QualifiedNames::iterator used = used_names.begin(); used; used = used->Next) {
            if (QualifiedName(/* empty_string ? */ns_uri, Mstring(tmpname)) == used->Data) {
              found = true;
              break;
            }
          }
          counter++;
        } while (found);
        qual_name.name = tmpname;
        Free(tmpname);
        postfixing = false;
      }
      break;
    default:
      break;
  }

  res = qual_name.name;

  /********************************************************
   * STEP 3 - the defined name is put into the set of "not_av_names"
   * ******************************************************/
  // Finally recently defined name will be put into the set of "set<string> not_av_names"
  if (type_of_the_name != type_reference_name) {
    bool found = false;
    for (QualifiedNames::iterator used = used_names.begin(); used; used = used->Next) {
      if (qual_name == used->Data) {
        found = true;
        break;
      }
    }

    if (!found) {
      used_names.push_back(qual_name);
    }
  }
  /********************************************************
   * STEP 4
   *
   * ******************************************************/
  if (in == "sequence" ||
    in == "choice" ||
    in == "sequence_list" ||
    in == "choice_list") {
    return;
  }
  /********************************************************
   * STEP 5 - if "input name - in" and "generated name - res"
   * 			are different
   * 			then a "variant string" has to be generated
   * ******************************************************/
  // If the final generated type name, field name or enumeration identifier (res) is different from the original one (in) then ...
  if (in != res) {
    Mstring tmp1 = in;
    Mstring tmp2 = res;
    tmp1.setUncapitalized();
    tmp2.setUncapitalized();
    switch (type_of_the_name) {
      case type_name:
        if (tmp1 == tmp2) { // If the only difference is the case of the first letter
          if (isupper(in[0])) {
            variant += "\"name as capitalized\"";
          } else {
            variant += "\"name as uncapitalized\"";
          }
        } else { // Otherwise if other letters have changed too
          variant += "\"name as '" + in + "'\"";
        }
        break;
      case field_name:
        // Creating a variant string from a field of a complex type needs to write out the path of the fieldname
        if (tmp1 == tmp2) { // If the only difference is the case of the first letter
          if (isupper(in[0])) {
            variant += "\"name as capitalized\"";
          } else {
            variant += "\"name as uncapitalized\"";
          }
        } else { // Otherwise if other letters have changed too
          variant += "\"name as '" + in + "'\"";
        }
        break;
      case enum_id_name:
        if (tmp1 == tmp2) { // If the only difference is the case of the first letter
          if (isupper(in[0])) {
            variant += "\"text \'" + res + "\' as capitalized\"";
          } else {
            variant += "\"text \'" + res + "\' as uncapitalized\"";
          }
        } else { // Otherwise if other letters have changed too
          variant += "\"text \'" + res + "\' as '" + in + "'\"";
        }
        break;
      default:
        break;
    }
  }
}

bool isBuiltInType(const Mstring& in) {
  static const char* XSD_built_in_types[] = {
    "string", "normalizedString", "token", "Name", "NMTOKEN", "NCName", "ID", "IDREF", "ENTITY",
    "hexBinary", "base64Binary", "anyURI", "language", "integer", "positiveInteger", "nonPositiveInteger",
    "negativeInteger", "nonNegativeInteger", "long", "unsignedLong", "int", "unsignedInt", "short",
    "unsignedShort", "byte", "unsignedByte", "decimal", "float", "double", "duration", "dateTime", "time",
    "date", "gYearMonth", "gYear", "gMonthDay", "gDay", "gMonth", "NMTOKENS", "IDREFS", "ENTITIES",
    "QName", "boolean", "anyType", "anySimpleType", NULL
  };
  const Mstring& name = in.getValueWithoutPrefix(':');
  for (int i = 0; XSD_built_in_types[i]; ++i) {
    if (name == XSD_built_in_types[i]) {
      return true;
    }
  }
  return false;
}

bool isStringType(const Mstring& in) {
  static const char* string_types[] = {
    "string", "normalizedString", "token", "Name", "NMTOKEN", "NCName", "ID", "IDREF", "ENTITY",
    "hexBinary", "base64Binary", "anyURI", "language", NULL
  };
  const Mstring& name = in.getValueWithoutPrefix(':');
  for (int i = 0; string_types[i]; ++i) {
    if (name == string_types[i]) {
      return true;
    }
  }
  return false;
}

bool isIntegerType(const Mstring& in) {
  static const char* integer_types[] = {
    "integer", "positiveInteger", "nonPositiveInteger", "negativeInteger", "nonNegativeInteger", "long",
    "unsignedLong", "int", "unsignedInt", "short", "unsignedShort", "byte", "unsignedByte", NULL
  };
  const Mstring& name = in.getValueWithoutPrefix(':');
  for (int i = 0; integer_types[i]; ++i) {
    if (name == integer_types[i]) {
      return true;
    }
  }
  return false;
}

bool isFloatType(const Mstring& in) {
  static const char* float_types[] = {
    "decimal", "float", "double", NULL
  };
  const Mstring& name = in.getValueWithoutPrefix(':');
  for (int i = 0; float_types[i]; ++i) {
    if (name == float_types[i]) {
      return true;
    }
  }
  return false;
}

bool isTimeType(const Mstring& in) {
  static const char* time_types[] = {
    "duration", "dateTime", "time", "date", "gYearMonth", "gYear", "gMonthDay", "gDay", "gMonth", NULL
  };
  const Mstring& name = in.getValueWithoutPrefix(':');
  for (int i = 0; time_types[i]; ++i) {
    if (name == time_types[i]) {
      return true;
    }
  }
  return false;
}

bool isSequenceType(const Mstring& in) {
  static const char* sequence_types[] = {
    "NMTOKENS", "IDREFS", "ENTITIES", "QName", NULL
  };
  const Mstring& name = in.getValueWithoutPrefix(':');
  for (int i = 0; sequence_types[i]; ++i) {
    if (name == sequence_types[i]) {
      return true;
    }
  }
  return false;
}

bool isBooleanType(const Mstring& in) {
  static const Mstring booltype("boolean");
  return booltype == in.getValueWithoutPrefix(':');
}

bool isQNameType(const Mstring& in) {
  static const Mstring qntype("QName");
  return qntype == in.getValueWithoutPrefix(':');
}

bool isAnyType(const Mstring& in) {
  static const char* any_types[] = {
    "anyType", "anySimpleType", NULL
  };
  const Mstring& name = in.getValueWithoutPrefix(':');
  for (int i = 0; any_types[i]; ++i) {
    if (name == any_types[i]) return true;
  }
  return false;
}

bool matchDates(const char * string, const char * type) {
  const Mstring day("(0[1-9]|[12][0-9]|3[01])");
  const Mstring month("(0[1-9]|1[0-2])");
  const Mstring year("([0-9][0-9][0-9][0-9])");
  const Mstring hour("([01][0-9]|2[0-3])");
  const Mstring minute("([0-5][0-9])");
  const Mstring second("([0-5][0-9])");
  const Mstring endofdayext("24:00:00(.0?)?");
  const Mstring yearext("((-)([1-9][0-9]*)?)?");
  const Mstring time_zone("(Z|[+-]((0[0-9]|1[0-3]):[0-5][0-9]|14:00))?");
  const Mstring fraction("(.[0-9]+)?");
  const Mstring nums("[0-9]+");
  const Mstring durtime("(T[0-9]+"
    "(H([0-9]+(M([0-9]+(S|.[0-9]+S))?|.[0-9]+S|S))?|"
    "M([0-9]+(S|.[0-9]+S)|.[0-9]+M)?|S|.[0-9]+S))");

  Mstring pattern;
  if (strcmp(type, "gDay") == 0) {
    pattern = Mstring("(---)") + day + time_zone;
  } else if (strcmp(type, "gMonth") == 0) {
    pattern = Mstring("(--)") + month + time_zone;
  } else if (strcmp(type, "gYear") == 0) {
    pattern = yearext + year + time_zone;
  } else if (strcmp(type, "gYearMonth") == 0) {
    pattern = yearext + year + Mstring("(-)") + month + time_zone;
  } else if (strcmp(type, "gMonthDay") == 0) {
    pattern = Mstring("(--)") + month + Mstring("(-)") + day + time_zone;
  } else if (strcmp(type, "date") == 0) {
    pattern = yearext + year + Mstring("(-)") + month + Mstring("(-)") + day + time_zone;
  } else if (strcmp(type, "time") == 0) {
    pattern = Mstring("(") + hour + Mstring(":") + minute + Mstring(":") + second +
      fraction + Mstring("|") + endofdayext + Mstring(")") + time_zone;
  } else if (strcmp(type, "dateTime") == 0) {
    pattern = yearext + year + Mstring("(-)") + month + Mstring("(-)") + day +
      Mstring("T(") + hour + Mstring(":") + minute + Mstring(":") + second +
      fraction + Mstring("|") + endofdayext + Mstring(")") + time_zone;
  } else if (strcmp(type, "duration") == 0) {
    pattern = Mstring("(-)?P(") + nums + Mstring("(Y(") + nums + Mstring("(M(") +
      nums + Mstring("D") + durtime + Mstring("?|") + durtime + Mstring("?|D") +
      durtime + Mstring("?)|") + durtime + Mstring("?)|M") + nums + Mstring("D") +
      durtime + Mstring("?|") + durtime + Mstring("?)|D") + durtime +
      Mstring("?)|") + durtime + Mstring(")");
  } else {
    return false;
  }

  pattern = Mstring("^") + pattern + Mstring("$");
  return matchRegexp(string, pattern.c_str());
}

bool matchRegexp(const char * string, const char * pattern) {
  int status;
  regex_t re;
  if (regcomp(&re, pattern, REG_EXTENDED | REG_NOSUB) != 0) {
    return (false); /* report error */
  }
  status = regexec(&re, string, (size_t) 0, NULL, 0);
  regfree(&re);
  if (status != 0) {
    return (false); /* report error */
  }
  return (true);
}

void printError(const Mstring& filename, int lineNumber, const Mstring& text) {
  fprintf(stderr,
    "ERROR:\n"
    "%s (in line %d): "
    "%s\n",
    filename.c_str(),
    lineNumber,
    text.c_str());
}

void printError(const Mstring& filename, const Mstring& typeName, const Mstring& text) {
  fprintf(stderr,
    "ERROR:\n"
    "%s (in type %s): "
    "%s\n",
    filename.c_str(),
    typeName.c_str(),
    text.c_str());
}

void printWarning(const Mstring& filename, int lineNumber, const Mstring& text) {
  if (w_flag_used) return;
  fprintf(stderr,
    "WARNING:\n"
    "%s (in line %d): "
    "%s\n",
    filename.c_str(),
    lineNumber,
    text.c_str());
}

void printWarning(const Mstring& filename, const Mstring& typeName, const Mstring& text) {
  if (w_flag_used) return;
  fprintf(stderr,
    "WARNING:\n"
    "%s (in type %s): "
    "%s\n",
    filename.c_str(),
    typeName.c_str(),
    text.c_str());
}

void indent(FILE* file, const unsigned int x) {
  for (unsigned int l = 0; l < x; ++l) {
    fprintf(file, "\t");
  }
}

Mstring xmlFloat2TTCN3FloatStr(const Mstring& xmlFloat) {
  Mstring result;
  if (xmlFloat == "INF") {
    result = "infinity";
  } else if (xmlFloat == "-INF") {
    result = "-infinity";
  } else if (xmlFloat == "NaN") {
    result = "not_a_number";
  } else if (!xmlFloat.isFound('.') && !xmlFloat.isFound('e') && !xmlFloat.isFound('E')) {
    // Float types need the .0 if it is a single integer or 
    // translate special float values to TTCN3.
    result = xmlFloat + Mstring(".0");
  } else {
    result = xmlFloat;
  }
  return result;
}

long double stringToLongDouble(const char *input) {
  long double result = 0.0;
  // `strtold()' is not available on older platforms.
  sscanf(input, "%Lf", &result);
  return result;
}

const Mstring& getNameSpaceByPrefix(const RootType * root, const Mstring& prefix){
  for(List<NamespaceType>::iterator mod = root->getModule()->getDeclaredNamespaces().begin(); mod; mod = mod->Next){
    if(mod->Data.prefix == prefix){
      return mod->Data.uri;
    }
  }
  return empty_string;
}

const Mstring& getPrefixByNameSpace(const RootType * root, const Mstring& namespace_){
  for(List<NamespaceType>::iterator mod = root->getModule()->getDeclaredNamespaces().begin(); mod; mod = mod->Next){
    if(mod->Data.uri == namespace_){
      return mod->Data.prefix;
    }
  }
  return empty_string;
}

const Mstring findBuiltInType(const RootType* ref, Mstring type){
  RootType * root = TTCN3ModuleInventory::getInstance().lookup(ref, type, want_BOTH);
  if(root != NULL && isBuiltInType(root->getType().originalValueWoPrefix)){
    return root->getType().originalValueWoPrefix;
  }else if(root != NULL){
    return findBuiltInType(root, root->getType().originalValueWoPrefix);
  }else {
    return type;
  }
}

RootType * lookup(const List<TTCN3Module*> mods, const SimpleType * reference, wanted w) {
  const Mstring& uri = reference->getReference().get_uri();
  const Mstring& name = reference->getReference().get_val();

  return lookup(mods, name, uri, reference, w);
}

RootType * lookup(const List<TTCN3Module*> mods,
  const Mstring& name, const Mstring& nsuri, const RootType *reference, wanted w) {
  RootType *ret = NULL;
  for (List<TTCN3Module*>::iterator module = mods.begin(); module; module = module->Next) {
    ret = lookup1(module->Data, name, nsuri, reference, w);
    if (ret != NULL) break;
  } // next doc

  return ret;
}

RootType *lookup1(const TTCN3Module *module,
  const Mstring& name, const Mstring& nsuri, const RootType *reference, wanted w) {
  if (nsuri != module->getTargetNamespace()) return NULL;
      
  for (List<RootType*>::iterator type = module->getDefinedTypes().begin(); type; type = type->Next) {
    switch (type->Data->getConstruct()) {
      case c_simpleType:
      case c_element:
      case c_attribute:
        if (w == want_ST || w == want_BOTH) {
          if ((const RootType*) reference != type->Data
            && name == type->Data->getName().originalValueWoPrefix) {
            return type->Data;
          }
        }
        break;

      case c_complexType:
      case c_group:
      case c_attributeGroup:
        if (w == want_CT || w == want_BOTH) {
          if ((const RootType*) reference != type->Data
            && name == type->Data->getName().originalValueWoPrefix) {
            return type->Data;
          }
        }
        break;

      default:
        break;
    }
  }
  
  //Check for an include with NoTargetNamespace to look for the type
  //XSD to TTCN ETSI standard 5.1.2
  for(List<RootType*>::iterator it = module->getDefinedTypes().begin(); it; it = it->Next) {
    if (it->Data != NULL && it->Data->getConstruct() == c_include &&
        ((ImportStatement*)(it->Data))->getSourceModule() != NULL &&
        ((ImportStatement*)(it->Data))->getSourceModule()->getTargetNamespace() == Mstring("NoTargetNamespace")) {
      return lookup1(((ImportStatement*)(it->Data))->getSourceModule(), name, Mstring("NoTargetNamespace"), reference, w);
    }
  }
  return NULL;
}

int multi(const TTCN3Module *module, ReferenceData const& outside_reference,
  const RootType *obj) {
  int multiplicity = 0;

  RootType * st = ::lookup1(module, outside_reference.get_val(), outside_reference.get_uri(), obj, want_ST);
  RootType * ct = ::lookup1(module, outside_reference.get_val(), outside_reference.get_uri(), obj, want_CT);
  if (st || ct) {
    multiplicity = 1; // locally defined, no qualif needed
    // means that outside_reference.get_uri() == module->getTargetNamespace())
  } else {
    // Look for definitions in the imported modules
    for (List<const TTCN3Module*>::iterator it = module->getImportedModules().begin(); it; it = it->Next) {
      // Artificial lookup
      st = ::lookup1(it->Data, outside_reference.get_val(), it->Data->getTargetNamespace(), obj, want_ST);
      ct = ::lookup1(it->Data, outside_reference.get_val(), it->Data->getTargetNamespace(), obj, want_CT);
      if (st || ct) {
        ++multiplicity;
      }
    }
    // If multiplicity > 1 then the qualif needed
    // But if == 1 we need to check this module for a type definition with
    // the same name as outsize_reference.get_val()
    if (multiplicity == 1) {
      st = ::lookup1(module, outside_reference.get_val(), module->getTargetNamespace(), obj, want_ST);
      ct = ::lookup1(module, outside_reference.get_val(), module->getTargetNamespace(), obj, want_CT);
      if (st || ct) {
        ++multiplicity;
      }
    }
  }
  return multiplicity;
}

void generate_TTCN3_header(FILE * file, const char * modulename, const bool timestamp /* = true */) {
  time_t time_current = time(NULL);
  fprintf(file,
    "/*******************************************************************************\n"
    );
  if (t_flag_used) {
    fprintf(file,
      "* Copyright Ericsson Telecom AB\n"
      "*\n"
      "* XSD to TTCN-3 Translator\n"
      "*\n"
      );
  } else {
    fprintf(file,
      "* Copyright (c) 2000-%-4d Ericsson Telecom AB\n"
      "*\n"
      "* XSD to TTCN-3 Translator version: %-40s\n"
      "*\n",
      1900 + (localtime(&time_current))->tm_year,
      PRODUCT_NUMBER
      );
  }
  fprintf(file,
    "* All rights reserved. This program and the accompanying materials\n"
    "* are made available under the terms of the Eclipse Public License v1.0\n"
    "* which accompanies this distribution, and is available at\n"
    "* http://www.eclipse.org/legal/epl-v10.html\n"
    "*******************************************************************************/\n"
    "//\n"
    "//  File:          %s.ttcn\n"
    "//  Description:\n"
    "//  References:\n"
    "//  Rev:\n"
    "//  Prodnr:\n",
    modulename
    );
  if (t_flag_used || !timestamp) {
    fprintf(file,
      "//  Updated:\n"
      );
  } else {
    fprintf(file,
      "//  Updated:       %s",
      ctime(&time_current)
      );
  }
  fprintf(file,
    "//  Contact:       http://ttcn.ericsson.se\n"
    "//\n"
    "////////////////////////////////////////////////////////////////////////////////\n"
    );
}
