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
 *   Delic, Adam
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#include "../common/dbgnew.hh"
#include "Identifier.hh"
#include <ctype.h>
#include "Setting.hh"
#include "CompilerError.hh"

namespace Common {

  // =================================
  // ===== Identifier::id_data_t
  // =================================

  struct Identifier::id_data_t {
    size_t ref_count;
    string name, asnname, ttcnname;
    /** ASN kind of the identifier. */
    enum asn_kind_t {
      ASN_UNDEF,    /**< undefined */
      ASN_LOWER,    /**< LOWERIDENTIFIER [a-z](-?[A-Za-z0-9]+)* */
      ASN_UPPER,    /**< UPPERIDENTIFIER [A-Z](-?[A-Za-z0-9]+)* */
      ASN_ALLUPPER, /**< ALLUPPERIDENTIFIER [A-Z](-?[A-Z0-9]+)* */
      ASN_WORD,     /**< WORD [A-Z](-?[A-Z]+)* */
      ASN_ampUPPER, /**< ampUPPERIDENTIFIER \\\&{UPPERIDENTIFIER} */
      ASN_ampLOWER  /**< ampLOWERIDENTIFIER \\\&{LOWERIDENTIFIER} */
    } asn_kind;
    static void asn_2_name(string& p_to, const string& p_from);
    static void name_2_asn(string& p_to, const string& p_from);
    static void ttcn_2_name(string& p_to, const string& p_from);
    static void name_2_ttcn(string& p_to, const string& p_from);
    id_data_t(const string& p_name)
    : ref_count(1), name(p_name), asnname(), ttcnname(), asn_kind(ASN_UNDEF){}
    static void delete_this(id_data_t *p_id_data_t);
    /** Gets the internal (and C++) name. */
    const string& get_name() const {return name;}
    /** Gets the ASN display name. */
    const string& get_asnname();
    /** Gets the TTCN display name. */
    const string& get_ttcnname();
    bool get_has_valid(id_t p_id_t);
    string get_names() const;
    /** True if it is a valid ASN modulereference. */
    bool isvalid_asn_modref();
    /** True if it is a valid ASN typereference. */
    bool isvalid_asn_typeref();
    /** True if it is a valid ASN valuereference. */
    bool isvalid_asn_valref();
    /** True if it is a valid ASN valuesetreference. */
    bool isvalid_asn_valsetref();
    /** True if it is a valid ASN objectclassreference. */
    bool isvalid_asn_objclassref();
    /** True if it is a valid ASN objectreference. */
    bool isvalid_asn_objref();
    /** True if it is a valid ASN objectsetreference. */
    bool isvalid_asn_objsetref();
    /** True if it is a valid ASN typefieldreference. */
    bool isvalid_asn_typefieldref();
    /** True if it is a valid ASN valuefieldreference. */
    bool isvalid_asn_valfieldref();
    /** True if it is a valid ASN valuesetfieldreference. */
    bool isvalid_asn_valsetfieldref();
    /** True if it is a valid ASN objectfieldreference. */
    bool isvalid_asn_objfieldref();
    /** True if it is a valid ASN objectsetfieldreference. */
    bool isvalid_asn_objsetfieldref();
    /** True if it is a valid ASN "word". */
    bool isvalid_asn_word();
  private:
    ~id_data_t() {}
    void decide_asn_kind();
  };

  // =================================
  // ===== internal_data_t
  // =================================

  class internal_data_t {
  private:
    static internal_data_t *instance;
    static const char* const keywords[][3];
    size_t identifier_counter;
  public:
    const string string_invalid;
    /** Container for identifiers, indexed by ID_NAME. */
    map<string, Identifier::id_data_t> id_map_name;
    /** Container for identifiers, indexed by ID_ASN. */
    map<string, Identifier::id_data_t> id_map_asn;
    /** Container for identifiers, indexed by ID_TTCN. */
    map<string, Identifier::id_data_t> id_map_ttcn;
  private:
    internal_data_t();
    internal_data_t(const internal_data_t&);
    ~internal_data_t();
    void add_keyword(const char* const keyword[3]);
    void add_keywords();
  public:
    static internal_data_t *Instance();
    /** Increases the instance counter. Initializes the keywords if
     * this is the first instance. Must be called in every Identifier
     * constructor. */
    void increase_counter();
    /** Decreases the instance counter. Finalizes the keywords if this
     * is the last instance. Must be called in Identifier
     * destructor. */
    void decrease_counter();
  };

  // ======================================================================
  // ======================================================================

  // =================================
  // ===== Identifier::id_data_t
  // =================================

  void Identifier::id_data_t::asn_2_name(string& p_to, const string& p_from)
  {
    p_to = p_from;
    /* "@aaa" -> "_root_aaa" */
    if (p_to.size() > 0 && p_to[0] == '@') p_to.replace(0, 1, "_root_");
    /* "aa.<xxxx>.bb" -> "aa.bb" */
    size_t pos = 0;
    while ((pos = p_to.find(".<", pos)) < p_to.size()) {
      size_t pos2 = p_to.find(">.", pos);
      if (pos2 < p_to.size()) p_to.replace(pos, pos2 + 1 - pos, "");
      else break;
    }
    /* "-" -> "__" */
    pos = 0;
    while ((pos = p_to.find('-', pos)) < p_to.size()) {
      p_to.replace(pos, 1, "__");
      pos += 2;
    }
    /* "." -> "_" */
    pos = 0;
    while ((pos = p_to.find('.', pos)) < p_to.size()) {
      p_to[pos] = '_';
      pos++;
    }
    /* "&" -> "" */
    pos = 0;
    while ((pos = p_to.find('&', pos)) < p_to.size())
      p_to.replace(pos, 1, "");
  }

  void Identifier::id_data_t::name_2_asn(string& p_to, const string& p_from)
  {
    p_to = p_from;
    /* remove leading '_'s */
    size_t pos = 0;
    while (pos < p_to.size() && p_to[pos] == '_') pos++;
    if (pos > 0) p_to.replace(0, pos, "");
    /* remove trailing '_'s */
    pos = p_to.size();
    while (pos > 0 && p_to[pos - 1] == '_') pos--;
    if (pos < p_to.size()) p_to.resize(pos);
    /* "__" -> "-" */
    pos = 0;
    while ((pos = p_to.find("__", pos)) < p_to.size()) {
      p_to.replace(pos, 2, "-");
      pos++;
    }
    /* "_" -> "-" */
    pos = 0;
    while ((pos = p_to.find('_', pos)) < p_to.size()) {
      p_to[pos] = '-';
      pos++;
    }
  }

  void Identifier::id_data_t::ttcn_2_name(string& p_to, const string& p_from)
  {
    p_to = p_from;
    /* "_" -> "__" */
    size_t pos = 0;
    while ((pos = p_to.find('_', pos)) < p_to.size()) {
      p_to.replace(pos, 1, "__");
      pos += 2;
    }
  }

  void Identifier::id_data_t::name_2_ttcn(string& p_to, const string& p_from)
  {
    p_to = p_from;
    /* remove leading '_'s */
    size_t pos = 0;
    while (pos < p_to.size() && p_to[pos] == '_') pos++;
    if (pos > 0) p_to.replace(0, pos, "");
    /* remove trailing '_'s */
    pos = p_to.size();
    while (pos > 0 && p_to[pos - 1] == '_') pos--;
    if (pos < p_to.size()) p_to.resize(pos);
    /* "__" -> "_" */
    pos = 0;
    while ((pos = p_to.find("__", pos)) < p_to.size()) {
      p_to.replace(pos, 1, "");
      pos++;
    }
  }

  const string& Identifier::id_data_t::get_asnname()
  {
    if (asnname.empty()) name_2_asn(asnname, name);
    return asnname;
  }

  const string& Identifier::id_data_t::get_ttcnname()
  {
    if (ttcnname.empty()) name_2_ttcn(ttcnname, name);
    return ttcnname;
  }

  bool Identifier::id_data_t::get_has_valid(id_t p_id_t)
  {
    const string& inval=internal_data_t::Instance()->string_invalid;
    switch(p_id_t) {
    case ID_NAME:
      return get_name()!=inval;
    case ID_ASN:
      return get_asnname()!=inval;
    case ID_TTCN:
      return get_ttcnname()!=inval;
    default:
      FATAL_ERROR("Identifier::id_data_t::get_has_valid()");
      return false;
    }
  }

  void Identifier::id_data_t::delete_this(id_data_t *p_id_data_t)
  {
    p_id_data_t->ref_count--;
    if(p_id_data_t->ref_count==0)
      delete p_id_data_t;
  }

  string Identifier::id_data_t::get_names() const
  {
    const string& inval=internal_data_t::Instance()->string_invalid;
    string s="(C++: `"+name+"'";
    if(!asnname.empty() && asnname!=inval)
      s+=", ASN: `"+asnname+"'";
    if(!ttcnname.empty() && ttcnname!=inval)
      s+=", TTCN: `"+ttcnname+"'";
    s+=")";
    return s;
  }

  void Identifier::id_data_t::decide_asn_kind()
  {
    if(asn_kind!=ASN_UNDEF) return;
    get_asnname();
    if(asnname==internal_data_t::Instance()->string_invalid) return;
    if(asnname[0]=='&') {
      if(asnname.size()>=2) {
        if(isupper(asnname[1]))
          asn_kind=ASN_ampUPPER;
        else if(islower(asnname[1]))
          asn_kind=ASN_ampLOWER;
      }
    }
    else if(islower(asnname[0])) {
      asn_kind=ASN_LOWER;
    }
    else if(isupper(asnname[0])) {
      asn_kind=ASN_UPPER;
      if(asnname.find_if(0, asnname.size(), islower)==asnname.size()) {
        asn_kind=ASN_ALLUPPER;
        if(asnname.find_if(0, asnname.size(), isdigit)==asnname.size()) {
          asn_kind=ASN_WORD;
        }
      }
    }
  }

  bool Identifier::id_data_t::isvalid_asn_modref()
  {
    return isvalid_asn_typeref();
  }

  bool Identifier::id_data_t::isvalid_asn_typeref()
  {
    decide_asn_kind();
    return (asn_kind==ASN_UPPER
            || asn_kind==ASN_ALLUPPER
            || asn_kind==ASN_WORD);
  }

  bool Identifier::id_data_t::isvalid_asn_valref()
  {
    decide_asn_kind();
    return (asn_kind==ASN_LOWER);
  }

  bool Identifier::id_data_t::isvalid_asn_valsetref()
  {
    return isvalid_asn_typeref();
  }

  bool Identifier::id_data_t::isvalid_asn_objclassref()
  {
    decide_asn_kind();
    return (asn_kind==ASN_ALLUPPER
            || asn_kind==ASN_WORD);
  }

  bool Identifier::id_data_t::isvalid_asn_objref()
  {
    decide_asn_kind();
    return (asn_kind==ASN_LOWER);
  }

  bool Identifier::id_data_t::isvalid_asn_objsetref()
  {
    return isvalid_asn_typeref();
  }

  bool Identifier::id_data_t::isvalid_asn_typefieldref()
  {
    decide_asn_kind();
    return (asn_kind==ASN_ampUPPER);
  }

  bool Identifier::id_data_t::isvalid_asn_valfieldref()
  {
    decide_asn_kind();
    return (asn_kind==ASN_ampLOWER);
  }

  bool Identifier::id_data_t::isvalid_asn_valsetfieldref()
  {
    decide_asn_kind();
    return (asn_kind==ASN_ampUPPER);
  }

  bool Identifier::id_data_t::isvalid_asn_objfieldref()
  {
    decide_asn_kind();
    return (asn_kind==ASN_ampLOWER);
  }

  bool Identifier::id_data_t::isvalid_asn_objsetfieldref()
  {
    decide_asn_kind();
    return (asn_kind==ASN_ampUPPER);
  }

  bool Identifier::id_data_t::isvalid_asn_word()
  {
    decide_asn_kind();
    return (asn_kind==ASN_WORD);
  }

  // =================================
  // ===== internal_data_t
  // =================================

  internal_data_t *internal_data_t::instance=0;

  /* format: c++ - asn - ttcn */
  const char* const internal_data_t::keywords[][3] = {
    /* C++ keywords - never can be used */
    {"and", 0, 0},
    {"and_eq", 0, 0},
    {"asm", 0, 0},
    {"auto", 0, 0},
    {"bitand", 0, 0},
    {"bitor", 0, 0},
    {"bool", 0, 0},
    {"break", 0, 0},
    {"case", 0, 0},
    {"catch", 0, 0},
    {"char", 0, 0},
    {"class", 0, 0},
    {"compl", 0, 0},
    {"const", 0, 0},
    {"const_cast", 0, 0},
    {"continue", 0, 0},
    {"default", 0, 0},
    {"delete", 0, 0},
    {"do", 0, 0},
    {"double", 0, 0},
    {"dynamic_cast", 0, 0},
    {"else", 0, 0},
    {"enum", 0, 0},
    {"explicit", 0, 0},
    {"export", 0, 0},
    {"extern", 0, 0},
    {"false", 0, 0},
    {"float", 0, 0},
    {"for", 0, 0},
    {"friend", 0, 0},
    {"goto", 0, 0},
    {"if", 0, 0},
    {"inline", 0, 0},
    {"int", 0, 0},
    {"long", 0, 0},
    {"mutable", 0, 0},
    {"namespace", 0, 0},
    {"new", 0, 0},
    {"not", 0, 0},
    {"not_eq", 0, 0},
    {"operator", 0, 0},
    {"or", 0, 0},
    {"or_eq", 0, 0},
    {"private", 0, 0},
    {"protected", 0, 0},
    {"public", 0, 0},
    {"register", 0, 0},
    {"reinterpret_cast", 0, 0},
    {"return", 0, 0},
    {"short", 0, 0},
    {"signed", 0, 0},
    {"sizeof", 0, 0},
    {"static", 0, 0},
    {"static_cast", 0, 0},
    {"struct", 0, 0},
    {"switch", 0, 0},
    {"template", 0, 0},
    {"this", 0, 0},
    {"throw", 0, 0},
    {"true", 0, 0},
    {"try", 0, 0},
    {"typedef", 0, 0},
    {"typeid", 0, 0},
    {"typename", 0, 0},
    {"union", 0, 0},
    {"unsigned", 0, 0},
    {"using", 0, 0},
    {"virtual", 0, 0},
    {"void", 0, 0},
    {"volatile", 0, 0},
    {"wchar_t", 0, 0},
    {"while", 0, 0},
    {"xor", 0, 0},
    {"xor_eq", 0, 0},
    /* C++ keywords postfixed, avoiding conflicts from valid ASN/TTCN names */
    {"asm_", "asm", "asm"},
    {"auto_", "auto", "auto"},
    {"bitand_", "bitand", "bitand"},
    {"bitor_", "bitor", "bitor"},
    {"bool_", "bool", "bool"},
    {"class_", "class", "class"},
    {"compl_", "compl", "compl"},
    {"delete_", "delete", "delete"},
    {"double_", "double", "double"},
    {"enum_", "enum", "enum"},
    {"explicit_", "explicit", "explicit"},
    {"export_", "export", "export"},
    {"extern_", "extern", "extern"},
    {"friend__", "friend", "friend_"},
    {"inline_", "inline", "inline"},
    {"int_", "int", "int"},
    {"long_", "long", "long"},
    {"mutable_", "mutable", "mutable"},
    {"namespace_", "namespace", "namespace"},
    {"new_", "new", "new"},
    {"operator_", "operator", "operator"},
    {"private__", "private", "private_"},
    {"protected_", "protected", "protected"},
    {"public__", "public", "public_"},
    {"register_", "register", "register"},
    {"short_", "short", "short"},
    {"signed_", "signed", "signed"},
    {"static_", "static", "static"},
    {"struct_", "struct", "struct"},
    {"switch_", "switch", "switch"},
    {"this_", "this", "this"},
    {"throw_", "throw", "throw"},
    {"try_", "try", "try"},
    {"typedef_", "typedef", "typedef"},
    {"typeid_", "typeid", "typeid"},
    {"typename_", "typename", "typename"},
    {"unsigned_", "unsigned", "unsigned"},
    {"using_", "using", "using"},
    {"virtual_", "virtual", "virtual"},
    {"void_", "void", "void"},
    {"volatile_", "volatile", "volatile"},
    /* C++ keywords postfixed - keywords in TTCN */
    {"and__", "and", "and_"},
    {"break__", "break", "break_"},
    {"case__", "case", "case_"},
    {"catch__", "catch", "catch_"},
    {"char__", "char", "char_"},
    {"const__", "const", "const_"},
    {"continue__", "continue", "continue_"},
    {"default__", "default", "default_"},
    {"do__", "do", "do_"},
    {"else__", "else", "else_"},
    {"false__", "false", "false_"},
    {"float__", "float", "float_"},
    {"for__", "for", "for_"},
    {"goto__", "goto", "goto_"},
    {"if__", "if", "if_"},
    {"not__", "not", "not_"},
    {"or__", "or", "or_"},
    {"return__", "return", "return_"},
    {"sizeof__", "sizeof", "sizeof_"},
    {"template__", "template", "template_"},
    {"true__", "true", "true_"},
    {"union__", "union", "union_"},
    {"while__", "while", "while_"},
    {"xor__", "xor", "xor_"},
    /* reserved names of base library */
    {"CHAR", 0, 0},
    {"ERROR", 0, 0},
    {"FAIL", 0, 0},
    {"INCONC", 0, 0},
    {"FALSE", 0, 0},
    {"NONE", 0, 0},
    {"OPTIONAL", 0, 0},
    {"PASS", 0, 0},
    {"PORT", 0, 0},
    {"TIMER", 0, 0},
    {"TRUE", 0, 0},
    {"bit2hex", 0, 0},
    {"bit2int", 0, 0},
    {"bit2oct", 0, 0},
    {"bit2str", 0, 0},
    {"boolean", 0, 0},
    {"char2int", 0, 0},
    {"char2oct", 0, 0},
    {"component", 0, 0},
    {"decomp", 0, 0},
    {"decvalue_unichar", 0, 0},
    {"encvalue_unichar", 0, 0},
    {"float2int", 0, 0},
    {"float2str", 0, 0},
    {"hex2bit", 0, 0},
    {"hex2int", 0, 0},
    {"hex2oct", 0, 0},
    {"hex2str", 0, 0},
    {"hostid", 0, 0},
    {"int2bit", 0, 0},
    {"int2char", 0, 0},
    {"int2float", 0, 0},
    {"int2hex", 0, 0},
    {"int2oct", 0, 0},
    {"int2str", 0, 0},
    {"int2unichar", 0, 0},
    {"ischosen", 0, 0},
    {"ispresent", 0, 0},
    {"istemplatekind", 0, 0},
    {"isvalue", 0, 0},
    {"lengthof", 0, 0},
    {"log", 0, 0},
    {"log2str", 0, 0},
    {"main", 0, 0},
    {"match", 0, 0},
    {"mod", 0, 0},
    {"oct2bit", 0, 0},
    {"oct2char", 0, 0},
    {"oct2hex", 0, 0},
    {"oct2int", 0, 0},
    {"oct2str", 0, 0},
    {"regexp", 0, 0},
    {"replace", 0, 0},
    {"rem", 0, 0},
    {"rnd", 0, 0},
    {"self", 0, 0},
    {"stderr", 0, 0}, // temporary hack
    {"stdin", 0, 0}, // temporary hack
    {"stdout", 0, 0}, // temporary hack
    {"str2bit", 0, 0},
    {"str2float", 0, 0},
    {"str2hex", 0, 0},
    {"str2int", 0, 0},
    {"str2oct", 0, 0},
    {"substr", 0, 0},
    {"unichar2int", 0, 0},
    {"unichar2char", 0, 0},
    {"valueof", 0, 0},
    {"verdicttype", 0, 0},
    {"unichar2oct", 0, 0},
    {"oct2unichar", 0, 0},
    {"get_stringencoding", 0, 0},
    {"remove_bom", 0, 0},
    {"encode_base64", 0, 0},
    {"decode_base64", 0, 0},
    /* reserved names of base library - keywords in TTCN - valid ASN.1 */
    {"bit2hex__", "bit2hex", "bit2hex_"},
    {"bit2int__", "bit2int", "bit2int_"},
    {"bit2oct__", "bit2oct", "bit2oct_"},
    {"bit2str__", "bit2str", "bit2str_"},
    {"boolean__", "boolean", "boolean_"},
    {"char2int__", "char2int", "char2int_"},
    {"char2oct__", "char2oct", "char2oct_"},
    {"component__", "component", "component_"},
    {"decmatch__", "decmatch", "decmatch_"},
    {"decomp__", "decomp", "decomp_"},
     {"decvalue_unichar__", "decvalue_unichar", "decvalue_unichar_"},
    {"encvalue_unichar__", "encvalue_unichar", "encvalue_unichar_"},
    {"float2int__", "float2int", "float2int_"},
    {"float2str__", "float2str", "float2str_"},
    {"hex2bit__", "hex2bit", "hex2bit_"},
    {"hex2int__", "hex2int", "hex2int_"},
    {"hex2oct__", "hex2oct", "hex2oct_"},
    {"hex2str__", "hex2str", "hex2str_"},
    {"hostid__", "hostid", "hostid_"},
    {"int2bit__", "int2bit", "int2bit_"},
    {"int2char__", "int2char", "int2char_"},
    {"int2float__", "int2float", "int2float_"},
    {"int2hex__", "int2hex", "int2hex_"},
    {"int2oct__", "int2oct", "int2oct_"},
    {"int2str__", "int2str", "int2str_"},
    {"int2unichar__", "int2unichar", "int2unichar_"},
    {"ischosen__", "ischosen", "ischosen_"},
    {"ispresent__", "ispresent", "ispresent_"},
    {"istemplatekind__", "istemplatekind", "istemplatekind_"},
    {"isvalue__", "isvalue", "isvalue_"},
    {"lengthof__", "lengthof", "lengthof_"},
    {"log__", "log", "log_"},
    {"log2str__", "log2str", "log2str_"},
    {"match__", "match", "match_"},
    {"mod__", "mod", "mod_"},
    {"oct2bit__", "oct2bit", "oct2bit_"},
    {"oct2char__", "oct2char", "oct2char_"},
    {"oct2hex__", "oct2hex", "oct2hex_"},
    {"oct2int__", "oct2int", "oct2int_"},
    {"oct2str__", "oct2str", "oct2str_"},
    {"regexp__", "regexp", "regexp_"},
    {"replace__", "replace", "replace_"},
    {"rem__", "rem", "rem_"},
    {"rnd__", "rnd", "rnd_"},
    {"self__", "self", "self_"},
    {"str2bit__", "str2bit", "str2bit_"},
    {"str2float__", "str2float", "str2float_"},
    {"str2hex__", "str2hex", "str2hex_"},
    {"str2int__", "str2int", "str2int_"},
    {"str2oct__", "str2oct", "str2oct_"},
    {"substr__", "substr", "substr_"},
    {"unichar2int__", "unichar2int", "unichar2int_"},
    {"unichar2char__", "unichar2char", "unichar2char_"},
    {"valueof__", "valueof", "valueof_"},
    {"verdicttype__", "verdicttype", "verdicttype_"},
    {"ttcn2string__", "ttcn2string", "ttcn2string_"},
    {"string2ttcn__", "string2ttcn", "string2ttcn_"},
    {"unichar2oct__", "unichar2oct", "unichar2oct_"},
    {"oct2unichar__", "oct2unichar", "oct2unichar_"},
    {"remove__bom__", "remove_bom", "remove_bom_"},
    {"encode__base64__", "encode_base64", "encode_base64_"},
    {"decode__base64__", "decode_base64", "decode_base64_"},
    {"get__stringencoding__", "get_stringencoding", "get_stringencoding_"},
    /* reserved names of base library - keywords in ASN.1 */
    {"FALSE_", 0, "FALSE"},
    {"OPTIONAL_", 0, "OPTIONAL"},
    {"TRUE_", 0, "TRUE"},
    /* reserved names of base library - not keywords */
    {"CHAR_", "CHAR", "CHAR"},
    {"ERROR_", "ERROR", "ERROR"},
    {"FAIL_", "FAIL", "FAIL"},
    {"INCONC_", "INCONC", "INCONC"},
    {"NONE_", "NONE", "NONE"},
    {"PASS_", "PASS", "PASS"},
    {"PORT_", "PORT", "PORT"},
    {"TIMER_", "TIMER", "TIMER"},
    {"main_", "main", "main"},
    {"stderr_", "stderr", "stderr"}, // temporary hack
    {"stdin_", "stdin", "stdin"}, // temporary hack
    {"stdout_", "stdout", "stdout"}, // temporary hack
    {"TTCN3_", "TTCN3", "TTCN3"},
    /* built-in types. this is the ASN/TTCN -> C++ name mapping */
    {"ADDRESS", 0, "address"},
    {"ASN_NULL", "NULL", 0},
    {"BITSTRING", "BIT STRING", "bitstring"},
    {"BOOLEAN", "BOOLEAN", "boolean"},
    {"BMPString", "BMPString", 0},
    {"CHARSTRING", 0, "charstring"},
    {"CHARACTER_STRING", "CHARACTER STRING", 0},
    {"COMPONENT", 0, "component"},
    {"DEFAULT", 0, "default"},
    {"EMBEDDED_PDV", "EMBEDDED PDV", 0},
    {"EXTERNAL", "EXTERNAL", 0},
    {"FLOAT", "REAL", "float"},
    {"GraphicString", "GraphicString", 0},
    {"HEXSTRING", 0, "hexstring"},
    {"IA5String", "IA5String", 0},
    {"INTEGER", "INTEGER", "integer"},
    {"ISO646String", "ISO646String", 0},
    {"NumericString", "NumericString", 0},
    {"OBJID", "OBJECT IDENTIFIER", "objid"},
    {"OCTETSTRING", "OCTET STRING", "octetstring"},
    {"ObjectDescriptor", "ObjectDescriptor", 0},
    {"PrintableString", "PrintableString", 0},
    {"T61String", "T61String", 0},
    {"TeletexString", "TeletexString", 0},
    {"UTF8String", "UTF8String", 0},
    {"UniversalString", "UniversalString", 0},
    {"UNIVERSAL_CHARSTRING", 0, "universal charstring"},
    {"VERDICTTYPE", 0, "verdicttype"},
    {"VideotexString", "VideotexString", 0},
    {"VisibleString", "VisibleString", 0},
    /* reserved names of built-in types - valid ASN.1/TTCN names */
    {"ADDRESS_", "ADDRESS", "ADDRESS"},
    {"BITSTRING_", "BITSTRING", "BITSTRING"},
    {"BOOLEAN_", 0, "BOOLEAN"},
    {"BMPString_", 0, "BMPString"},
    {"CHARSTRING_", "CHARSTRING", "CHARSTRING"},
    {"COMPONENT_", "COMPONENT", "COMPONENT"},
    {"DEFAULT_", 0, "DEFAULT"},
    {"EXTERNAL_", 0, "EXTERNAL"},
    {"FLOAT_", "FLOAT", "FLOAT"},
    {"GraphicString_", 0, "GraphicString"},
    {"HEXSTRING_", "HEXSTRING", "HEXSTRING"},
    {"IA5String_", 0, "IA5String"},
    {"INTEGER_", 0, "INTEGER"},
    {"ISO646String_", 0, "ISO646String"},
    {"NumericString_", 0, "NumericString"},
    {"OBJID_", "OBJID", "OBJID"},
    {"OCTETSTRING_", "OCTETSTRING", "OCTETSTRING"},
    {"ObjectDescriptor_", 0, "ObjectDescriptor"},
    {"PrintableString_", 0, "PrintableString"},
    {"T61String_", 0, "T61String"},
    {"TeletexString_", 0, "TeletexString"},
    {"UTF8String_", 0, "UTF8String"},
    {"UniversalString_", 0, "UniversalString"},
    {"VERDICTTYPE_", "VERDICTTYPE", "VERDICTTYPE"},
    {"VideotexString_", 0, "VideotexString"},
    {"VisibleString_", 0, "VisibleString"},
    /* keywords in TTCN-3, not reserved words in C++, postfixed in TTCN */
    {"action__", "action", "action_"},
    {"activate__", "activate", "activate_"},
    {"address__", "address", "address_"},
    {"alive__", "alive", "alive_"},
    {"all__", "all", "all_"},
    {"alt__", "alt", "alt_"},
    {"altstep__", "altstep", "altstep_"},
    {"and4b__", "and4b", "and4b_"},
    {"any__", "any", "any_"},
    {"anytype__", "anytype", "anytype_"},
    {"apply__", "apply", "apply_"},
    {"bitstring__", "bitstring", "bitstring_"},
    {"call__", "call", "call_"},
    {"charstring__", "charstring", "charstring_"},
    {"check__", "check", "check_"},
    {"clear__", "clear", "clear_"},
    {"complement__", "complement", "complement_"},
    {"connect__", "connect", "connect_"},
    {"control__", "control", "control_"},
    {"create__", "create", "create_"},
    {"deactivate__", "deactivate", "deactivate_"},
    {"derefers__", "derefers", "derefers_"},
    {"disconnect__", "disconnect", "disconnect_"},
    {"display__", "display", "display_"},
    {"done__", "done", "done_"},
    {"encode__", "encode", "encode_"},
    {"enumerated__", "enumerated", "enumerated_"},
    {"error__", "error", "error_"},
    {"except__", "except", "except_"},
    {"exception__", "exception", "exception_"},
    {"execute__", "execute", "execute_"},
    {"extends__", "extends", "extends_"},
    {"extension__", "extension", "extension_"},
    {"external__", "external", "external_"},
    {"fail__", "fail", "fail_"},
    {"from__", "from", "from_"},
    {"function__", "function", "function_"},
    {"getcall__", "getcall", "getcall_"},
    {"getreply__", "getreply", "getreply_"},
    {"getverdict__", "getverdict", "getverdict_"},
    {"group__", "group", "group_"},
    {"halt__", "halt", "halt_"},
    {"hexstring__", "hexstring", "hexstring_"},
    {"ifpresent__", "ifpresent", "ifpresent_"},
    {"import__", "import", "import_"},
    {"in__", "in", "in_"},
    {"inconc__", "inconc", "inconc_"},
    {"infinity__", "infinity", "infinity_"},
    {"inout__", "inout", "inout_"},
    {"integer__", "integer", "integer_"},
    {"interleave__", "interleave", "interleave_"},
    {"kill__", "kill", "kill_"},
    {"killed__", "killed", "killed_"},
    {"label__", "label", "label_"},
    {"language__", "language", "language_"},
    {"length__", "length", "length_"},
    {"map__", "map", "map_"},
    {"message__", "message", "message_"},
    {"mixed__", "mixed", "mixed_"},
    {"modifies__", "modifies", "modifies_"},
    {"module__", "module", "module_"},
    {"modulepar__", "modulepar", "modulepar_"},
    {"mtc__", "mtc", "mtc_"},
    {"noblock__", "noblock", "noblock_"},
    {"none__", "none", "none_"},
    {"not4b__", "not4b", "not4b_"},
    {"nowait__", "nowait", "nowait_"},
    {"null__", "null", "null_"},
    {"objid__", "objid", "objid_"},
    {"octetstring__", "octetstring", "octetstring_"},
    {"of__", "of", "of_"},
    {"omit__", "omit", "omit_"},
    {"on__", "on", "on_"},
    {"optional__", "optional", "optional_"},
    {"or4b__", "or4b", "or4b_"},
    {"out__", "out", "out_"},
    {"override__", "override", "override_"},
    {"param__", "param", "param_"},
    {"pass__", "pass", "pass_"},
    {"pattern__", "pattern", "pattern_"},
    {"permutation__", "permutation", "permutation_"},
    {"port__", "port", "port_"},
    {"procedure__", "procedure", "procedure_"},
    {"raise__", "raise", "raise_"},
    {"read__", "read", "read_"},
    {"receive__", "receive", "receive_"},
    {"record__", "record", "record_"},
    {"recursive__", "recursive", "recursive_"},
    {"refers__", "refers", "refers_"},
    {"repeat__", "repeat", "repeat_"},
    {"reply__", "reply", "reply_"},
    {"running__", "running", "running_"},
    {"runs__", "runs", "runs_"},
    {"select__", "select", "select_"},
    {"send__", "send", "send_"},
    {"sender__", "sender", "sender_"},
    {"set__", "set", "set_"},
    {"setstate__", "setstate", "setstate_"},
    {"setverdict__", "setverdict", "setverdict_"},
    {"signature__", "signature", "signature_"},
    {"start__", "start", "start_"},
    {"stop__", "stop", "stop_"},
    {"subset__", "subset", "subset_"},
    {"superset__", "superset", "superset_"},
    {"system__", "system", "system_"},
    {"testcase__", "testcase", "testcase_"},
    {"timeout__", "timeout", "timeout_"},
    {"timer__", "timer", "timer_"},
    {"to__", "to", "to_"},
    {"trigger__", "trigger", "trigger_"},
    {"type__", "type", "type_"},
    {"universal__", "universal", "universal_"},
    {"unmap__", "unmap", "unmap_"},
    {"value__", "value", "value_"},
    {"present__", "present", "present_"},
    {"var__", "var", "var_"},
    {"variant__", "variant", "variant_"},
    {"with__", "with", "with_"},
    {"xor4b__", "xor4b", "xor4b_"},
    /* other names that need to be mapped in a non-uniform manner */
    /* major and minor are macros on some platforms; avoid generating
     * a potentially troublesome C++ identifier */
    {"major_", "major", "major"},
    {"minor_", "minor", "minor"},
    /* internal / error */
    {"<error>", "<error>", "<error>"},
    {"TTCN_internal_", "<internal>", "<internal>"},
    /* the last must be all zeros */
    {0, 0, 0}
  }; // keywords

  internal_data_t::internal_data_t()
    : identifier_counter(0), string_invalid("<invalid>"), id_map_name(),
      id_map_asn(), id_map_ttcn()
  {
  }

  internal_data_t::~internal_data_t()
  {
    for(size_t i=0; i<id_map_name.size(); i++)
      Identifier::id_data_t::delete_this(id_map_name.get_nth_elem(i));
    id_map_name.clear();
    for(size_t i=0; i<id_map_asn.size(); i++)
      Identifier::id_data_t::delete_this(id_map_asn.get_nth_elem(i));
    id_map_asn.clear();
    for(size_t i=0; i<id_map_ttcn.size(); i++)
      Identifier::id_data_t::delete_this(id_map_ttcn.get_nth_elem(i));
    id_map_ttcn.clear();
  }

  void internal_data_t::add_keyword(const char* const keyword[3])
  {
    Identifier::id_data_t *id_data
      =new Identifier::id_data_t(string(keyword[0]));
    bool conflict=false;
    // Pointers to already added (conflicting) keyword
    const Identifier::id_data_t *id_data_name=0;
    const Identifier::id_data_t *id_data_asn=0;
    const Identifier::id_data_t *id_data_ttcn=0;
    if(id_map_name.has_key(id_data->name)) {
      conflict=true;
      id_data_name=id_map_name[id_data->name];
    }
    else {
      id_map_name.add(id_data->name, id_data);
      id_data->ref_count++;
    }

    if(keyword[1]==0) {
      id_data->asnname=string_invalid;
    }
    else {
      // copy the string if possible to avoid memory allocation
      if (id_data->name == keyword[1]) id_data->asnname = id_data->name;
      else id_data->asnname = keyword[1];
      if(id_map_asn.has_key(id_data->asnname)) {
        conflict=true;
        id_data_asn=id_map_asn[id_data->asnname];
      }
      else {
        id_map_asn.add(id_data->asnname, id_data);
        id_data->ref_count++;
      }
    }

    if(keyword[2]==0) {
      id_data->ttcnname=string_invalid;
    }
    else {
      // copy the string if possible to avoid memory allocation
      if (id_data->name == keyword[2]) id_data->ttcnname = id_data->name;
      else if (id_data->asnname == keyword[2])
	id_data->ttcnname = id_data->asnname;
      else id_data->ttcnname = keyword[2];
      if(id_map_ttcn.has_key(id_data->ttcnname)) {
        conflict=true;
        id_data_ttcn=id_map_ttcn[id_data->ttcnname];
      }
      else {
        id_map_ttcn.add(id_data->ttcnname, id_data);
        id_data->ref_count++;
      }
    }

    if(conflict) {
      Location loc;
      loc.error
        ("This pre-defined identifier: %s",
         id_data->get_names().c_str());
      loc.error
        ("conflicts with previous:");
      if(id_data_name)
        loc.error
          ("[C++:] %s",
           id_data_name->get_names().c_str());
      if(id_data_asn)
        loc.error
          ("[ASN:] %s",
           id_data_asn->get_names().c_str());
      if(id_data_ttcn)
        loc.error
          ("[TTCN:] %s",
           id_data_ttcn->get_names().c_str());
    } // if conflict
    Identifier::id_data_t::delete_this(id_data);
  }

  void internal_data_t::add_keywords()
  {
    {
      Location loc;
      Error_Context cntx(&loc, "While adding keywords");
      for(size_t i=0; keywords[i][0]; i++)
        add_keyword(keywords[i]);
    }
    /* Perhaps it were good to read a file which contains
        user-defined mappings :) */
  }

  internal_data_t *internal_data_t::Instance()
  {
    if(!instance) {
      instance=new internal_data_t();
      instance->add_keywords();
    }
    return instance;
  }

  void internal_data_t::increase_counter()
  {
    identifier_counter++;
  }

  void internal_data_t::decrease_counter()
  {
    identifier_counter--;
    if(identifier_counter==0) {
      delete instance;
      instance=0;
    } // if last Identifier instance
  }

  // =================================
  // ===== Identifier
  // =================================

  bool Identifier::is_reserved_word(const string& p_name, id_t p_id_t)
  {
    if (p_name.empty())
      FATAL_ERROR("Identifier::is_reserved_word(): empty name");
    internal_data_t *d = internal_data_t::Instance();
    switch (p_id_t) {
    case ID_NAME:
      if (d->id_map_name.has_key(p_name)) {
        id_data_t *id_data_p = d->id_map_name[p_name];
        if (id_data_p->asnname == d->string_invalid &&
          id_data_p->ttcnname == d->string_invalid) return true;
        else return false;
      } else return false;
    case ID_ASN:
      if (p_name[0] == '&' || d->id_map_asn.has_key(p_name)) return false;
      else {
	string name;
	id_data_t::asn_2_name(name, p_name);
	if (d->id_map_name.has_key(name)) {
	  id_data_t *id_data_p = d->id_map_name[name];
	  if (id_data_p->asnname.empty()) {
	    id_data_p->asnname = p_name;
	    d->id_map_asn.add(p_name, id_data_p);
	    id_data_p->ref_count++;
	    return false;
	  } else if (id_data_p->asnname == p_name) return false;
	  else return true;
	} else return false;
      }
    case ID_TTCN:
      if (d->id_map_ttcn.has_key(p_name)) return false;
      else {
	string name;
	id_data_t::ttcn_2_name(name, p_name);
	if (d->id_map_name.has_key(name)) {
	  id_data_t *id_data_p = d->id_map_name[name];
	  if (id_data_p->ttcnname.empty()) {
	    id_data_p->ttcnname = p_name;
	    d->id_map_ttcn.add(p_name, id_data_p);
	    id_data_p->ref_count++;
	    return false;
	  } else if (id_data_p->ttcnname == p_name) return false;
	  else return true;
	} else return false;
      }
    default:
      FATAL_ERROR("Identifier::is_reserved_word(): invalid language");
      return false;
    }
  }

  string Identifier::asn_2_name(const string& p_name)
  {
    internal_data_t *d = internal_data_t::Instance();
    if (d->id_map_asn.has_key(p_name)) {
      id_data_t *id_data_p = d->id_map_asn[p_name];
      if (id_data_p->name.empty()) {
	id_data_t::asn_2_name(id_data_p->name, p_name);
	d->id_map_name.add(id_data_p->name, id_data_p);
	id_data_p->ref_count++;
      }
      return id_data_p->name;
    } else {
      string name;
      id_data_t::asn_2_name(name, p_name);
      if (d->id_map_name.has_key(name)) {
	id_data_t *id_data_p = d->id_map_name[name];
	if (id_data_p->asnname.empty()) {
	  id_data_p->asnname = p_name;
	  d->id_map_asn.add(p_name, id_data_p);
	  id_data_p->ref_count++;
	}
      }
      return name;
    }
  }

  string Identifier::name_2_asn(const string& p_name)
  {
    internal_data_t *d = internal_data_t::Instance();
    if (d->id_map_name.has_key(p_name)) {
      id_data_t *id_data_p = d->id_map_name[p_name];
      if (id_data_p->asnname.empty()) {
	id_data_t::name_2_asn(id_data_p->asnname, p_name);
	d->id_map_asn.add(id_data_p->asnname, id_data_p);
	id_data_p->ref_count++;
      }
      return id_data_p->asnname;
    } else {
      string asnname;
      id_data_t::name_2_asn(asnname, p_name);
      if (d->id_map_asn.has_key(asnname)) {
	id_data_t *id_data_p = d->id_map_asn[asnname];
	if (id_data_p->name.empty()) {
	  id_data_p->name = p_name;
	  d->id_map_name.add(p_name, id_data_p);
	  id_data_p->ref_count++;
	}
      }
      return asnname;
    }
  }

  string Identifier::ttcn_2_name(const string& p_name)
  {
    internal_data_t *d = internal_data_t::Instance();
    if (d->id_map_ttcn.has_key(p_name)) {
      id_data_t *id_data_p = d->id_map_ttcn[p_name];
      if (id_data_p->name.empty()) {
	id_data_t::ttcn_2_name(id_data_p->name, p_name);
	d->id_map_name.add(id_data_p->name, id_data_p);
	id_data_p->ref_count++;
      }
      return id_data_p->name;
    } else {
      string name;
      id_data_t::ttcn_2_name(name, p_name);
      if (d->id_map_name.has_key(name)) {
	id_data_t *id_data_p = d->id_map_name[name];
	if (id_data_p->ttcnname.empty()) {
	  id_data_p->ttcnname = p_name;
	  d->id_map_ttcn.add(p_name, id_data_p);
	  id_data_p->ref_count++;
	}
      }
      return name;
    }
  }

  string Identifier::name_2_ttcn(const string& p_name)
  {
    internal_data_t *d = internal_data_t::Instance();
    if (d->id_map_name.has_key(p_name)) {
      id_data_t *id_data_p = d->id_map_name[p_name];
      if (id_data_p->ttcnname.empty()) {
	id_data_t::name_2_ttcn(id_data_p->ttcnname, p_name);
	d->id_map_ttcn.add(id_data_p->ttcnname, id_data_p);
	id_data_p->ref_count++;
      }
      return id_data_p->ttcnname;
    } else {
      string ttcnname;
      id_data_t::name_2_ttcn(ttcnname, p_name);
      if (d->id_map_ttcn.has_key(ttcnname)) {
	id_data_t *id_data_p = d->id_map_ttcn[ttcnname];
	if (id_data_p->name.empty()) {
	  id_data_p->name = p_name;
	  d->id_map_name.add(p_name, id_data_p);
	  id_data_p->ref_count++;
	}
      }
      return ttcnname;
    }
  }

  Identifier::Identifier(id_t p_id_t, const string& p_name, bool dontreg)
    : id_data(0), origin(p_id_t)
  {
    if (p_name.empty()) FATAL_ERROR("Identifier::Identifier(): empty name");
    internal_data_t *d=internal_data_t::Instance();
    d->increase_counter();
    switch(p_id_t) {
    case ID_NAME:
      if(d->id_map_name.has_key(p_name)) {
        id_data=d->id_map_name[p_name];
        id_data->ref_count++;
      }
      else {
        id_data=new id_data_t(p_name);
        if(!dontreg) {
          d->id_map_name.add(p_name, id_data);
          id_data->ref_count++;
        }
      }
      break;
    case ID_ASN:
      if(d->id_map_asn.has_key(p_name)) {
        id_data=d->id_map_asn[p_name];
        id_data->ref_count++;
      }
      else if(p_name[0]=='&') { // special amp-identifiers (&)
        string p_name2=p_name.substr(1);
        string name2;
        if(d->id_map_asn.has_key(p_name2))
          name2=d->id_map_asn[p_name2]->get_name();
        else
          id_data_t::asn_2_name(name2, p_name2);
        id_data=new id_data_t(name2);
        id_data->asnname=p_name;
        id_data->ttcnname=d->string_invalid;
        if(!dontreg) {
          d->id_map_asn.add(p_name, id_data);
          id_data->ref_count++;
        }
        /* this id_data should NOT be added to id_map_name. */
      } // if &identifier
      else {
	string name;
	id_data_t::asn_2_name(name, p_name);
        if(!dontreg && d->id_map_name.has_key(name)) {
          id_data=d->id_map_name[name];
          id_data->ref_count++;
          if(id_data->asnname.empty()) {
            id_data->asnname=p_name;
            if(!dontreg) {
              d->id_map_asn.add(p_name, id_data);
              id_data->ref_count++;
            }
          }
          else if(id_data->asnname!=p_name) {
            Location loc;
            loc.error
              ("The ASN identifier `%s' clashes with this id: %s",
               p_name.c_str(), id_data->get_names().c_str());
          }
        }
        else {
          id_data=new id_data_t(name);
          id_data->asnname=p_name;
          if(!dontreg) {
            d->id_map_name.add(name, id_data);
            d->id_map_asn.add(p_name, id_data);
            id_data->ref_count+=2;
          }
        }
      }
      break;
    case ID_TTCN:
      if(d->id_map_ttcn.has_key(p_name)) {
        id_data=d->id_map_ttcn[p_name];
        id_data->ref_count++;
      }
      else {
	string name;
	id_data_t::ttcn_2_name(name, p_name);
        if(!dontreg && d->id_map_name.has_key(name)) {
          id_data=d->id_map_name[name];
          id_data->ref_count++;
          if(id_data->ttcnname.empty()) {
            id_data->ttcnname=p_name;
            if(!dontreg) {
              d->id_map_ttcn.add(p_name, id_data);
              id_data->ref_count++;
            }
          }
          else if(id_data->ttcnname!=p_name) {
            Location loc;
            loc.error
              ("The TTCN identifier `%s' clashes with this id: %s",
               p_name.c_str(), id_data->get_names().c_str());
          }
        }
        else {
          id_data=new id_data_t(name);
          id_data->ttcnname=p_name;
          if(!dontreg) {
            d->id_map_name.add(name, id_data);
            d->id_map_ttcn.add(p_name, id_data);
            id_data->ref_count+=2;
          }
        }
      }
      break;
    default:
      FATAL_ERROR("Identifier::Identifier()");
    } // switch p_id_t
  }

  Identifier::Identifier(const Identifier& p)
    : id_data(p.id_data), origin(p.origin)
  {
    internal_data_t::Instance()->increase_counter();
    id_data->ref_count++;
  }

  Identifier::~Identifier()
  {
    id_data_t::delete_this(id_data);
    /* I don't want to free the id_data structs here. They will be
       deleted in decrease_counter() when the maps are destructed. */
    internal_data_t::Instance()->decrease_counter();
  }

  Identifier& Identifier::operator=(const Identifier& p)
  {
    if(&p!=this) {
      id_data_t::delete_this(id_data);
      id_data=p.id_data;
      id_data->ref_count++;
      origin=p.origin;
    }
    return *this;
  }

  bool Identifier::operator==(const Identifier& p) const
  {
    return id_data->name==p.id_data->name;
  }

  bool Identifier::operator<(const Identifier& p) const
  {
    return id_data->name<p.id_data->name;
  }

  const string& Identifier::get_name() const
  {
    return id_data->get_name();
  }

  const string& Identifier::get_dispname() const
  {
    switch(origin) {
    case ID_NAME:
      return id_data->get_name();
    case ID_ASN:
      return id_data->get_asnname();
    case ID_TTCN:
      return id_data->get_ttcnname();
    default:
      FATAL_ERROR("Identifier::get_dispname()");
      return id_data->get_name();
    }
  }

  const string& Identifier::get_asnname() const
  {
    return id_data->get_asnname();
  }

  const string& Identifier::get_ttcnname() const
  {
    return id_data->get_ttcnname();
  }

  bool Identifier::get_has_valid(id_t p_id_t) const
  {
    return id_data->get_has_valid(p_id_t);
  }

  string Identifier::get_names() const
  {
    return id_data->get_names();
  }

  bool Identifier::isvalid_asn_modref() const
  {
    return id_data->isvalid_asn_modref();
  }

  bool Identifier::isvalid_asn_typeref() const
  {
    return id_data->isvalid_asn_typeref();
  }

  bool Identifier::isvalid_asn_valref() const
  {
    return id_data->isvalid_asn_valref();
  }

  bool Identifier::isvalid_asn_valsetref() const
  {
    return id_data->isvalid_asn_valsetref();
  }

  bool Identifier::isvalid_asn_objclassref() const
  {
    return id_data->isvalid_asn_objclassref();
  }

  bool Identifier::isvalid_asn_objref() const
  {
    return id_data->isvalid_asn_objref();
  }

  bool Identifier::isvalid_asn_objsetref() const
  {
    return id_data->isvalid_asn_objsetref();
  }

  bool Identifier::isvalid_asn_typefieldref() const
  {
    return id_data->isvalid_asn_typefieldref();
  }

  bool Identifier::isvalid_asn_valfieldref() const
  {
    return id_data->isvalid_asn_valfieldref();
  }

  bool Identifier::isvalid_asn_valsetfieldref() const
  {
    return id_data->isvalid_asn_valsetfieldref();
  }

  bool Identifier::isvalid_asn_objfieldref() const
  {
    return id_data->isvalid_asn_objfieldref();
  }

  bool Identifier::isvalid_asn_objsetfieldref() const
  {
    return id_data->isvalid_asn_objsetfieldref();
  }

  bool Identifier::isvalid_asn_word() const
  {
    return id_data->isvalid_asn_word();
  }

  void Identifier::dump(unsigned level) const
  {
    DEBUG(level, "Identifier: %s", id_data->get_names().c_str());
  }

  const Identifier underscore_zero(Identifier::ID_TTCN, string("_0"));

} // namespace Common
