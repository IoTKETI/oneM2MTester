/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   
 *   Baji, Laszlo
 *   Balasko, Jeno
 *   Baranyi, Botond
 *   Beres, Szabolcs
 *   Cserveni, Akos
 *   Czerman, Oliver
 *   Delic, Adam
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Gecse, Roland
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *   Zalanyi, Balazs Andor
 *   Pandi, Krisztian
 *
 ******************************************************************************/
#include <set>
#include <string>
#include <sstream>

#include "../common/dbgnew.hh"
#include "AST.hh"
#include "asn1/AST_asn1.hh"
#include "Identifier.hh"
#include "Type.hh"
#include "TypeCompat.hh"
#include "Value.hh"
#include "ustring.hh"
#include "main.hh"
#include "asn1/Object0.hh"
#include "PredefFunc.hh"
#include "../common/version.h"
#include "CodeGenHelper.hh"
#include <limits.h>
#include "ttcn3/profiler.h"

reffer::reffer(const char*) {}

namespace Common {
    
  // =================================
  // ===== Modules
  // =================================
  
  vector<Modules::type_enc_t> Modules::delayed_type_enc_v;

  Modules::Modules()
    : Node(), mods_v(), mods_m()
  {
    set_fullname(string('@'));
  }

  Modules::~Modules()
  {
    for(size_t i = 0; i < mods_v.size(); i++) delete mods_v[i];
    mods_v.clear();
    mods_m.clear();
  }

  Modules *Modules::clone() const
  {
    FATAL_ERROR("Modules::clone()");
    return 0;
  }

  void Modules::add_mod(Module *p_mod)
  {
    if (!p_mod) FATAL_ERROR("NULL parameter: Common::Modules::add_mod()");
    p_mod->set_fullname("@"+p_mod->get_modid().get_dispname());
    p_mod->set_scope_name(p_mod->get_modid().get_dispname());
    mods_v.add(p_mod);
  }

  bool Modules::has_mod_withId(const Identifier& p_modid)
  {
    return mods_m.has_key(p_modid.get_name());
  }

  Module* Modules::get_mod_byId(const Identifier& p_modid)
  {
    const string& name = p_modid.get_name();
    return mods_m.has_key(name)?mods_m[name]:0;
  }

  Assignment* Modules::get_ass_bySRef(Ref_simple *p_ref)
  {
    if(!p_ref)
      FATAL_ERROR("NULL parameter: Common::Modules::get_ass_bySRef()");
    const Identifier *modid=p_ref->get_modid();
    if(modid) {
      if(has_mod_withId(*modid))
	return get_mod_byId(*modid)->get_ass_bySRef(p_ref);
      else {
        p_ref->error("There is no module with identifier `%s'",
                     modid->get_dispname().c_str());
	return 0;
      }
    }
    else {
      p_ref->error("`%s' entity not found in global scope",
                   p_ref->get_dispname().c_str());
      return 0;
    }
  }

  void Modules::chk_uniq()
  {
    for(size_t i = 0; i < mods_v.size(); i++) {
      Module *m = mods_v[i];
      const Identifier& id = m->get_modid();
      const string& name = id.get_name();
      if (mods_m.has_key(name)) {
	Module *m2 = mods_m[name];
	m->error("A module with identifier `%s' already exists",
	  id.get_dispname().c_str());
	m2->error("This is the first module with the same name");
	if (m->get_moduletype() == m2->get_moduletype() &&
	  !strcmp(m->get_filename(), m2->get_filename())) {
	  // the same file was given twice -> drop the entire module
	  delete m;
	  mods_v.replace(i, 1);
	  i--;
	}
      } else mods_m.add(name, m);
    }
  }

  void Modules::chk()
  {
    // first check the uniqueness of module names
    chk_uniq();
    // check the import chains
    size_t nof_mods = mods_v.size();
    for (size_t i = 0; i < nof_mods; i++) {
      Module *m = mods_v[i];
      ReferenceChain refch(m, "While checking import chains");
      vector<Common::Module> modules;
      m->chk_imp(refch, modules);
      modules.clear();
      //clear the reference chain, get a fresh start
      refch.reset();
    }
    // check the modules
    Module::module_set_t checked_modules;
    if (nof_top_level_pdus > 0) {
      chk_top_level_pdus();
      // do not check ASN.1 modules, but assume they are already checked
      for (size_t i = 0; i < nof_mods; i++) {
	Module *module = mods_v[i];
	if (module->get_moduletype() == Module::MOD_ASN)
	  checked_modules.add(module, 0);
      }
      for (size_t i = 0; i < nof_mods; i++) {
	Module *module = mods_v[i];
	if (module->get_moduletype() != Module::MOD_ASN)
	  module->chk_recursive(checked_modules);
      }
    } else {
      // walk through all modules in bottom-up order
      for (size_t i = 0; i < nof_mods; i++)
	mods_v[i]->chk_recursive(checked_modules);
    }
    checked_modules.clear();
    // run delayed Type::chk_coding() calls
    if (!delayed_type_enc_v.empty()) {
      for (size_t i = 0; i < delayed_type_enc_v.size(); ++i) {
        delayed_type_enc_v[i]->t->chk_coding(delayed_type_enc_v[i]->enc,
          delayed_type_enc_v[i]->mod, true);
        delete delayed_type_enc_v[i];
      }
      delayed_type_enc_v.clear();
    }
  }

  void Modules::chk_top_level_pdus()
  {
    Location loc("<command line>");
    for(size_t i=0; i<nof_top_level_pdus; i++) {
      string pduname(top_level_pdu[i]);
      size_t dotpos=pduname.find('.');
      if(dotpos>=pduname.size()) {
        loc.error("While searching top-level pdu `%s': "
                  "Please use the `modulename.identifier' format",
                  pduname.c_str());
        continue;
      }
      Module *module=0;
      Identifier *pdu_id=0;
      { // searching the module
        const string& pduname_mod = pduname.substr(0, dotpos);
	const string& pduname_id = pduname.substr(dotpos + 1);
	{ // ASN
          Identifier modid(Identifier::ID_ASN, pduname_mod, true);
          module = get_mod_byId(modid);
	}
        if (module && module->get_moduletype() == Module::MOD_ASN) {
          pdu_id = new Identifier(Identifier::ID_ASN, pduname_id, true);
          goto mod_ok;
        }
	{ // TTCN
          Identifier modid(Identifier::ID_TTCN, pduname_mod, true);
          module = get_mod_byId(modid);
	}
        if (module && module->get_moduletype() == Module::MOD_TTCN) {
          pdu_id = new Identifier(Identifier::ID_TTCN, pduname_id, true);
          goto mod_ok;
        }
	{ // C++
          Identifier modid(Identifier::ID_NAME, pduname_mod, true);
          module = get_mod_byId(modid);
	}
        if(module) {
          pdu_id = new Identifier(Identifier::ID_NAME, pduname_id, true);
          goto mod_ok;
        }
        // error - no such module
        loc.error("While searching top-level pdu `%s': "
                  "No module with name `%s'",
                  pduname.c_str(), pduname_mod.c_str());
        continue;
      }
  mod_ok:
      Assignments *asss=module->get_asss();
      if(asss->has_local_ass_withId(*pdu_id)) {
        Assignment *ass=asss->get_local_ass_byId(*pdu_id);
        ass->chk();
      }
      else {
        loc.error("While searching top-level pdu `%s': "
                  "No assignment with identifier `%s'",
                  pduname.c_str(), pdu_id->get_dispname().c_str());
      }
      delete pdu_id;
    } // for top-level pdus
  }

  void Modules::write_checksums()
  {
    fputs("Module name        Language        MD5 checksum                     Version\n"
	  "---------------------------------------------------------------------------\n", stderr);
    size_t nof_mods = mods_v.size();
    for (size_t i = 0; i < nof_mods; i++) {
      mods_v[i]->write_checksum();
    }
  }

  void Modules::generate_code(CodeGenHelper& cgh)
  {
    Common::Module::rename_default_namespace(); // if needed
    /*
        The  White Rabbit put on his spectacles.
        "Where shall  I  begin, please your Majesty ?" he asked.
        "Begin at the beginning,", the King said, very gravely, "and go on
        till you come to the end: then stop."
                -- Lewis Carroll
    */
    for (size_t i = 0; i < mods_v.size(); i++) {
      mods_v[i]->generate_code(cgh);
      if (tcov_file_name && in_tcov_files(mods_v[i]->get_filename())) {
        Free(effective_module_lines);
        Free(effective_module_functions);
        effective_module_lines = effective_module_functions = NULL;
      }
    }

    cgh.write_output();
  }


  void Modules::dump(unsigned level) const
  {
    DEBUG(level, "Modules (%lu pcs.)", (unsigned long) mods_v.size());
    for(size_t i = 0; i < mods_v.size(); i++) mods_v[i]->dump(level);
  }

  std::set<ModuleVersion> Modules::getVersionsWithProductNumber() const {
    std::set<ModuleVersion> versions;
    for (size_t i = 0; i < mods_v.size(); ++i) {
      const ModuleVersion version = mods_v[i]->getVersion();
      if (version.hasProductNumber()) {
        versions.insert(version);
      }
    }
    return versions;
  }
  
  void Modules::generate_json_schema(JSON_Tokenizer& json, map<Type*, JSON_Tokenizer>& json_refs)
  {
    for(size_t i = 0; i < mods_v.size(); ++i) {
      mods_v[i]->generate_json_schema(json, json_refs);
    }
  }
  
  void Modules::delay_type_encode_check(Type* p_type, Module* p_module, bool p_encode)
  {
    for (size_t i = 0; i < delayed_type_enc_v.size(); ++i) {
      if (delayed_type_enc_v[i]->t == p_type &&
          delayed_type_enc_v[i]->mod == p_module &&
          delayed_type_enc_v[i]->enc == p_encode) {
        // it's already in the list of delayed checks
        return;
      }
    }
    type_enc_t* elem = new type_enc_t;
    elem->t = p_type;
    elem->mod = p_module;
    elem->enc = p_encode;
    delayed_type_enc_v.add(elem);
  }


  // =================================
  // ===== Module
  // =================================
  
  ModuleVersion Module::getVersion() const {
    return ModuleVersion(product_number, suffix, release, patch, build, extra);
  }

  void Module::generate_literals(output_struct *target)
  {
    char *src = NULL;
    char *hdr = NULL;
    generate_bs_literals(src, hdr); // implementations follow directly below
    generate_bp_literals(src, hdr);
    generate_hs_literals(src, hdr);
    generate_hp_literals(src, hdr);
    generate_os_literals(src, hdr);
    generate_op_literals(src, hdr);
    generate_cs_literals(src, hdr);
    generate_us_literals(src, hdr);
    generate_oid_literals(src, hdr);
    generate_pp_literals(src, hdr);
    generate_mp_literals(src, hdr);
    target->source.string_literals =
      mputstr(target->source.string_literals, src);
    if (CodeGenHelper::GetInstance().get_split_mode() != CodeGenHelper::SPLIT_NONE) {
      target->header.global_vars = mputstr(target->header.global_vars, hdr);
    }
    Free(src);
    Free(hdr);
  }

  void Module::generate_bs_literals(char *&src, char *&hdr)
  {
    if (bs_literals.size() == 0) return;
    // indicates whether we have found at least one non-empty bitstring
    bool is_nonempty = false;
    bool splitting =
      CodeGenHelper::GetInstance().get_split_mode() != CodeGenHelper::SPLIT_NONE;
    for (size_t i = 0; i < bs_literals.size(); i++) {
      const string& str = bs_literals.get_nth_key(i);
      size_t bits = str.size();
      if (bits == 0) continue;
      if (is_nonempty) src = mputstr(src, ",\n");
      else {
        src = mputprintf(src, "%sconst unsigned char ", split_to_slices ? "" : "static ");
        is_nonempty = true;
      }
      src = mputprintf(src, "%s_bits[] = { ",
        bs_literals.get_nth_elem(i)->c_str());
      // Filling up the octets one-by-one
      for (size_t j = 0; j < (bits + 7) / 8; j++) {
        size_t offset = 8 * j;
        unsigned char value = 0;
        for (size_t k = 0; k < 8 && k < bits - offset; k++) {
            if (str[offset + k] == '1') value |= (1 << k);
        }
        if (j > 0) src = mputstr(src, ", ");
        src = mputprintf(src, "%d", value);
      }
      src = mputstr(src, " }");
    }
    if (is_nonempty) src = mputstr(src, ";\n");
    for (size_t i = 0; i < bs_literals.size(); i++) {
      if (i > 0) {
        src = mputstr(src, ",\n");
        if (splitting) hdr = mputstr(hdr, ",\n");
      }
      else {
        src = mputprintf(src, "%s const BITSTRING ",
          splitting ? "extern" : "static");
        if (splitting) hdr = mputstr(hdr, "extern const BITSTRING ");
      }
      size_t bits = bs_literals.get_nth_key(i).size();
      const char *object_name = bs_literals.get_nth_elem(i)->c_str();
      if (bits > 0) src = mputprintf(src, "%s(%lu, %s_bits)",
          object_name, (unsigned long) bits, object_name);
      else src = mputprintf(src, "%s(0, NULL)", object_name);
      if (splitting) hdr = mputstr(hdr, object_name);
    }
    src = mputstr(src, ";\n");
    if (splitting) hdr = mputstr(hdr, ";\n");
  }

  void Module::generate_bp_literals(char *&src, char *& hdr)
  {
    if (bp_literals.size() == 0) return;
    for (size_t i = 0; i < bp_literals.size(); i++) {
      if (i > 0) src = mputstr(src, ",\n");
      else {
        src = mputprintf(src, "%sconst unsigned char ", split_to_slices ? "" : "static ");
      }
      src = mputprintf(src, "%s_elements[] = { ",
        bp_literals.get_nth_elem(i)->c_str());
      const string& str = bp_literals.get_nth_key(i);
      for (size_t j = 0; j < str.size(); j++) {
        if (j > 0) src = mputstr(src, ", ");
        switch (str[j]) {
        case '0':
          src = mputc(src, '0');
          break;
        case '1':
          src = mputc(src, '1');
          break;
        case '?':
          src = mputc(src, '2');
          break;
        case '*':
          src = mputc(src, '3');
          break;
        default:
          FATAL_ERROR("Invalid character in bitstring pattern.");
        }
      }
      src = mputstr(src, " }");
    }
    src = mputstr(src, ";\n");
    for (size_t i = 0; i < bp_literals.size(); i++) {
      if (i > 0) {
        src = mputstr(src, ",\n");
        if (split_to_slices) {
          hdr = mputstr(hdr, ",\n");
        }
      }
      else {
        src = mputprintf(src, "%sconst BITSTRING_template ", split_to_slices ? "" : "static ");
        if (split_to_slices) {
            hdr = mputprintf(hdr, "extern const BITSTRING_template ");
        }
      }
      const char *name = bp_literals.get_nth_elem(i)->c_str();
      src = mputprintf(src, "%s(%lu, %s_elements)",
        name, (unsigned long) bp_literals.get_nth_key(i).size(), name);
      if (split_to_slices) {
        hdr = mputstr(hdr, name);
      }
    }
    src = mputstr(src, ";\n");
    if (split_to_slices) {
      hdr = mputstr(hdr, ";\n");
    }
  }

  void Module::generate_hs_literals(char *&src, char *&hdr)
  {
    if (hs_literals.size() == 0) return;
    // indicates whether we have found at least one non-empty hexstring
    bool is_nonempty = false;
    bool splitting =
      CodeGenHelper::GetInstance().get_split_mode() != CodeGenHelper::SPLIT_NONE;
    for (size_t i = 0; i < hs_literals.size(); i++) {
      const string& str = hs_literals.get_nth_key(i);
      size_t nibbles = str.size();
      if (nibbles == 0) continue;
      size_t octets = (nibbles + 1) / 2;
      const char *str_ptr = str.c_str();
      if (is_nonempty) src = mputstr(src, ",\n");
      else {
        src = mputprintf(src, "%sconst unsigned char ", split_to_slices ? "" : "static ");
        is_nonempty = true;
      }
      src = mputprintf(src, "%s_nibbles[] = { ",
        hs_literals.get_nth_elem(i)->c_str());
      for (size_t j = 0; j < octets; j++) {
        // Hex digit with even index always goes to the least significant
        // 4 bits of the octet.
        unsigned char value = char_to_hexdigit(str_ptr[2 * j]);
        if (2 * j + 1 < nibbles) {
          // Hex digit with odd index always goes to the most significant
          // 4 bits of the octet.
          // This digit is not present (bits are set to zero) if the length
          // of hexstring is odd.
          value += 16 * char_to_hexdigit(str_ptr[2 * j + 1]);
        }
        if (j > 0) src = mputstr(src, ", ");
        src = mputprintf(src, "%u", value);
      }
      src = mputstr(src, " }");
    }
    if (is_nonempty) src = mputstr(src, ";\n");
    for (size_t i = 0; i < hs_literals.size(); i++) {
      if (i > 0) {
        src = mputstr(src, ",\n");
        if (splitting) hdr = mputstr(hdr, ",\n");
      }
      else {
        src = mputprintf(src, "%s const HEXSTRING ",
          splitting ? "extern" : "static");
        if (splitting) hdr = mputstr(hdr, "extern const HEXSTRING ");
      }
      size_t nibbles = hs_literals.get_nth_key(i).size();
      const char *object_name = hs_literals.get_nth_elem(i)->c_str();
      if (nibbles > 0) src = mputprintf(src, "%s(%lu, %s_nibbles)",
          object_name, (unsigned long) nibbles, object_name);
      else src = mputprintf(src, "%s(0, NULL)", object_name);
      if (splitting) hdr = mputstr(hdr, object_name);
    }
    src = mputstr(src, ";\n");
    if (splitting) hdr = mputstr(hdr, ";\n");
  }

  void Module::generate_hp_literals(char *&src, char *& hdr)
  {
    if (hp_literals.size() == 0) return;
    for (size_t i = 0; i < hp_literals.size(); i++) {
      if (i > 0) {
        src = mputstr(src, ",\n");
      } else {
        src = mputprintf(src, "%sconst unsigned char ", split_to_slices ? "" : "static ");
      }
      src = mputprintf(src, "%s_elements[] = { ",
        hp_literals.get_nth_elem(i)->c_str());
      const string& str = hp_literals.get_nth_key(i);
      size_t size = str.size();
      const char *str_ptr = str.c_str();
      for (size_t j = 0; j < size; j++) {
        if (j > 0) src = mputstr(src, ", ");
        unsigned char num;
        if (str_ptr[j] == '?') num = 16;
        else if (str_ptr[j] == '*') num = 17;
        else num = char_to_hexdigit(str_ptr[j]);
        src = mputprintf(src, "%u", num);
      }
      src = mputstr(src, " }");
    }
    src = mputstr(src, ";\n");
    for (size_t i = 0; i < hp_literals.size(); i++) {
      if (i > 0) {
        src = mputstr(src, ",\n");
        if (split_to_slices) {
          hdr = mputstr(hdr, ",\n");
        }
      } else {
        src = mputprintf(src, "%sconst HEXSTRING_template ", split_to_slices ? "" : "static ");
        if (split_to_slices) {
          hdr = mputprintf(hdr, "extern const HEXSTRING_template ");
        }
      }
      const char *name = hp_literals.get_nth_elem(i)->c_str();
      src = mputprintf(src, "%s(%lu, %s_elements)",
        name, (unsigned long) hp_literals.get_nth_key(i).size(), name);
      if (split_to_slices) {
        hdr = mputstr(hdr, name);
      }
    }
    src = mputstr(src, ";\n");
    if (split_to_slices) {
      hdr = mputstr(hdr, ";\n");
    }
  }

  void Module::generate_os_literals(char *&src, char *&hdr)
  {
    if (os_literals.size() == 0) return;
    // indicates whether we have found at least one non-empty octetstring
    bool is_nonempty = false;
    bool splitting =
      CodeGenHelper::GetInstance().get_split_mode() != CodeGenHelper::SPLIT_NONE;
    for (size_t i = 0; i < os_literals.size(); i++) {
      const string& str = os_literals.get_nth_key(i);
      size_t size = str.size();
      if (size % 2) FATAL_ERROR("Invalid length for an octetstring.");
      size_t octets = size / 2;
      if (octets == 0) continue;
      const char *str_ptr = str.c_str();
      if (is_nonempty) src = mputstr(src, ",\n");
      else {
        src = mputprintf(src, "%sconst unsigned char ", split_to_slices ? "" : "static ");
        is_nonempty = true;
      }
      src = mputprintf(src, "%s_octets[] = { ",
        os_literals.get_nth_elem(i)->c_str());
      for (size_t j = 0; j < octets; j++) {
        if (j > 0) src = mputstr(src, ", ");
        src = mputprintf(src, "%u", 16 * char_to_hexdigit(str_ptr[2 * j]) +
          char_to_hexdigit(str_ptr[2 * j + 1]));
      }
      src = mputstr(src, " }");
    }
    if (is_nonempty) src = mputstr(src, ";\n");
    for (size_t i = 0; i < os_literals.size(); i++) {
      if (i > 0) {
        src = mputstr(src, ",\n");
        if (splitting) hdr = mputstr(hdr, ",\n");
      }
      else {
        src = mputprintf(src, "%s const OCTETSTRING ",
          splitting ? "extern" : "static");
        if (splitting) hdr = mputstr(hdr, "extern const OCTETSTRING ");
      }
      size_t octets = os_literals.get_nth_key(i).size() / 2;
      const char *object_name = os_literals.get_nth_elem(i)->c_str();
      if (octets > 0) src = mputprintf(src, "%s(%lu, %s_octets)",
          object_name, (unsigned long) octets, object_name);
      else src = mputprintf(src, "%s(0, NULL)", object_name);
      if (splitting) hdr = mputstr(hdr, object_name);
    }
    src = mputstr(src, ";\n");
    if (splitting) hdr = mputstr(hdr, ";\n");
  }

  void Module::generate_op_literals(char *&src, char *& hdr)
  {
    if (op_literals.size() == 0) return;
    vector<size_t> pattern_lens;
    for(size_t i = 0; i < op_literals.size(); i++) {
      if (i > 0) {
        src = mputstr(src, ",\n");
      } else {
        src = mputprintf(src, "%sconst unsigned short ", split_to_slices ? "" : "static ");
      }
      src = mputprintf(src, "%s_elements[] = { ",
        op_literals.get_nth_elem(i)->c_str());
      const string& str = op_literals.get_nth_key(i);
      size_t size = str.size();
      size_t pattern_len = 0;
      const char *str_ptr = str.c_str();
      for (size_t j = 0; j < size; j++) {
        if (j > 0) src = mputstr(src, ", ");
        unsigned short num;
        if (str_ptr[j] == '?') num = 256;
        else if (str_ptr[j] == '*') num = 257;
        else {
          // first digit
          num = 16 * char_to_hexdigit(str_ptr[j]);
          j++;
          if (j >= size) FATAL_ERROR("Unexpected end of octetstring pattern.");
          // second digit
          num += char_to_hexdigit(str_ptr[j]);
        }
        src = mputprintf(src, "%u", num);
        pattern_len++;
      }
      src = mputstr(src, " }");
      pattern_lens.add(new size_t(pattern_len));
    }
    src = mputstr(src, ";\n");
    for (size_t i = 0; i < op_literals.size(); i++) {
      if (i > 0) {
        src = mputstr(src, ",\n");
        if (split_to_slices) {
          hdr = mputstr(hdr, ",\n");
        }
      }
      else {
        src = mputprintf(src, "%sconst OCTETSTRING_template ", split_to_slices ? "" : "static ");
        if (split_to_slices) {
            hdr = mputprintf(hdr, "extern const OCTETSTRING_template ");
        }
      }
      const char *name = op_literals.get_nth_elem(i)->c_str();
      src = mputprintf(src, "%s(%lu, %s_elements)",
        name, (unsigned long) *pattern_lens[i], name);
      if (split_to_slices) {
        hdr = mputstr(hdr, name);
      }
    }
    src = mputstr(src, ";\n");
    if (split_to_slices) {
      hdr = mputstr(hdr, ";\n");
    }
    for (size_t i = 0; i < pattern_lens.size(); i++) delete pattern_lens[i];
    pattern_lens.clear();
  }

  void Module::generate_cs_literals(char *&src, char *&hdr)
  {
    if (cs_literals.size() == 0) return;
    bool splitting =
      CodeGenHelper::GetInstance().get_split_mode() != CodeGenHelper::SPLIT_NONE;
    for (size_t i = 0; i < cs_literals.size(); i++) {
      const string& str = cs_literals.get_nth_key(i);
      size_t str_len = str.size();
      const char *str_ptr = str.c_str();
      const char *str_name = cs_literals.get_nth_elem(i)->c_str();

      if (i > 0) {
        src = mputstr(src, ",\n");
        if (splitting) hdr = mputstr(hdr, ",\n");
      }
      else {
        src = mputprintf(src, "%s const CHARSTRING ",
          splitting ? "extern" : "static");
        if (splitting) hdr = mputstr(hdr, "extern const CHARSTRING ");
      }

      switch (str_len) {
      case 0:
        src = mputprintf(src, "%s(0, NULL)", str_name);
        break;
      case 1:
        src = mputprintf(src, "%s('", str_name);
        src = Code::translate_character(src, *str_ptr, false);
        src = mputstr(src, "')");
        break;
      default:
        src = mputprintf(src, "%s(%lu, \"", str_name, (unsigned long) str_len);
        // Note: Code::translate_string() is not suitable because the string
        // may contain NUL characters at which translate_string() stops
        // immediately
        for (size_t j = 0; j < str_len; j++)
          src = Code::translate_character(src, str_ptr[j], true);
        src = mputstr(src, "\")");
        break;
      } // switch
      if (splitting) hdr = mputstr(hdr, str_name);
    }
    src = mputstr(src, ";\n");
    if (splitting) hdr = mputstr(hdr, ";\n");
  }

  void Module::generate_pp_literals(char *&src, char *&) // padding patterns
  {
    if (pp_literals.size() == 0) return;
    for (size_t i = 0; i < pp_literals.size(); i++) {
      const string& pattern = pp_literals.get_nth_key(i);
      size_t pattern_len = pattern.size();
      const char *pattern_ptr = pattern.c_str();
      if (i > 0) {
        src = mputstr(src, ",\n");
      }
      else {
        src = mputprintf(src, "%sconst unsigned char ", split_to_slices ? "" : "static ");
      }
      src = mputprintf(src, "%s[] = { ", pp_literals.get_nth_elem(i)->c_str());
      if (pattern_len % 8 != 0) FATAL_ERROR("Module::generate_pp_literals()");
      size_t nof_octets = pattern_len / 8;
      for (size_t j = 0; j < nof_octets; j++) {
        if (j > 0) src = mputstr(src, ", ");
        unsigned char octet = 0;
        for (size_t k = 0; k < 8; k++) {
          // take the octets in reverse order
          // MSB is the first character of the string
          octet <<= 1;
          if (pattern_ptr[8 * (nof_octets - j - 1) + k] == '1') octet |= 0x01;
        }
        src = mputprintf(src, "0x%02x", octet);
      }
      src = mputstr(src, " }");
    }
    src = mputstr(src, ";\n");
  }

  void Module::generate_mp_literals(char *&src, char *&) // matching patt.
  {
    if (mp_literals.size() == 0) return;
    for (size_t i = 0; i < mp_literals.size(); i++) {
      const string& str = mp_literals.get_nth_key(i);
      if (str.size() < 1) FATAL_ERROR("Module::generate_mp_literals()");
      const char *str_ptr = str.c_str();

      if (i > 0) src = mputstr(src, ",\n");
      else src = mputprintf(src, "%sconst Token_Match ", split_to_slices ? "" : "static ");

      src = mputprintf(src, "%s(\"", mp_literals.get_nth_elem(i)->c_str());
      src = Code::translate_string(src, str_ptr + 1);
      // The first character of the string is case-sensitiveness flag:
      // 'I' for yes, 'N' for no,
      // 'F' for fixed string matching which is always case sensitive.
      src = mputprintf(src, "\", %s%s)", (str_ptr[0]!='N') ? "TRUE" : "FALSE",
        (str_ptr[0] == 'F') ? ", TRUE" : "");
    }
    src = mputstr(src, ";\n");
  }

  void Module::generate_us_literals(char *&src, char *&hdr) // univ.cs
  {
    size_t n_literals = us_literals.size();
    if (n_literals == 0) return;
    bool array_needed = false;
    bool splitting =
      CodeGenHelper::GetInstance().get_split_mode() != CodeGenHelper::SPLIT_NONE;
    for (size_t i = 0; i < n_literals; i++) {
      const ustring& value = us_literals.get_nth_key(i);
      size_t value_size = value.size();
      if (value_size < 2) continue;
      if (array_needed) src = mputstr(src, ",\n");
      else {
        src = mputprintf(src, "%sconst universal_char ", split_to_slices ? "" : "static ");
        array_needed = true;
      }
      src = mputprintf(src, "%s_uchars[] = { ",
        us_literals.get_nth_elem(i)->c_str());
      const ustring::universal_char *uchars_ptr = value.u_str();
      for (size_t j = 0; j < value_size; j++) {
        if (j > 0) src = mputstr(src, ", ");
        src = mputprintf(src, "{ %u, %u, %u, %u }", uchars_ptr[j].group,
          uchars_ptr[j].plane, uchars_ptr[j].row, uchars_ptr[j].cell);
      }
      src = mputstr(src, " }");
    }
    if (array_needed) src = mputstr(src, ";\n");
    for (size_t i = 0; i < n_literals; i++) {
      if (i > 0) {
        src = mputstr(src, ",\n");
        if (splitting) hdr = mputstr(hdr, ",\n");
      }
      else {
        src = mputprintf(src, "%s const UNIVERSAL_CHARSTRING ",
          splitting ? "extern" : "static");
        if (splitting) hdr = mputstr(hdr, "extern const UNIVERSAL_CHARSTRING ");
      }
      const char *value_name = us_literals.get_nth_elem(i)->c_str();
      const ustring& value = us_literals.get_nth_key(i);
      size_t value_size = value.size();
      switch (value_size) {
      case 0:
        src = mputprintf(src, "%s(0, (const universal_char*)NULL)", value_name);
        break;
      case 1: {
        const ustring::universal_char& uchar = value.u_str()[0];
        src = mputprintf(src, "%s(%u, %u, %u, %u)", value_name,
          uchar.group, uchar.plane, uchar.row, uchar.cell);
        break; }
      default:
        src = mputprintf(src, "%s(%lu, %s_uchars)", value_name,
          (unsigned long) value_size, value_name);
        break;
      }
      if (splitting) hdr = mputstr(hdr, value_name);
    }
    src = mputstr(src, ";\n");
    if (splitting) hdr = mputstr(hdr, ";\n");
  }

  void Module::generate_oid_literals(char *&src, char *& hdr)
  {
    if (oid_literals.size() == 0) return;
    for (size_t i = 0; i < oid_literals.size(); i++) {
       if (i > 0) src = mputstr(src, ",\n");
       else src = mputprintf(src, "%sconst OBJID::objid_element ", split_to_slices ? "" : "static ");

       src = mputprintf(src, "%s_comps[] = { %s }",
         oid_literals.get_nth_elem(i)->oid_id.c_str(),
         oid_literals.get_nth_key(i).c_str());
    }
    src = mputstr(src, ";\n");
    for(size_t i = 0; i < oid_literals.size(); i++) {
       const OID_literal *litstruct = oid_literals.get_nth_elem(i);

       if (i > 0) { 
         src = mputstr(src, ",\n");
         if (split_to_slices) {
           hdr = mputstr(hdr, ",\n");
         }
       }
       else {
         src = mputprintf(src, "%sconst OBJID ", split_to_slices ? "" : "static ");
         if (split_to_slices) {
           hdr = mputstr(hdr, "extern const OBJID ");
         }
       }

       src = mputprintf(src, "%s(%lu, %s_comps)",
         litstruct->oid_id.c_str(), (unsigned long) litstruct->nof_elems,
         litstruct->oid_id.c_str());
       if (split_to_slices) {
         hdr = mputstr(hdr, litstruct->oid_id.c_str());
       }
    }
    src = mputstr(src, ";\n");
    if (split_to_slices) {
      hdr = mputstr(hdr, ";\n");
    }
  }

  void Module::generate_functions(output_struct *output)
  {
    bool tcov_enabled = tcov_file_name && in_tcov_files(get_filename());
    bool has_pre_init_before_tcov = output->functions.pre_init != NULL;
    if (tcov_enabled) {
      output->functions.pre_init = mputprintf(output->functions.pre_init,
        "TTCN_Location_Statistics::init_file_lines(\"%s\", effective_module_lines, sizeof(effective_module_lines) / sizeof(int));\n" \
        "TTCN_Location_Statistics::init_file_functions(\"%s\", effective_module_functions, sizeof(effective_module_functions) / sizeof(char *));\n",
        get_tcov_file_name(get_filename()), get_tcov_file_name(get_filename()));
    }
    // pre_init function
    bool has_pre_init = false;
    bool profiled = MOD_TTCN == get_moduletype() && is_file_profiled(get_filename());
    bool debugged = debugger_active && MOD_TTCN == get_moduletype();
    // always generate pre_init_module if the file is profiled
    if (output->functions.pre_init || profiled || debugged) {
      output->source.static_function_prototypes =
	  mputprintf(output->source.static_function_prototypes,
	    "%svoid pre_init_module();\n", split_to_slices ? "extern " : "static ");
    output->source.static_function_bodies = mputprintf(output->source.static_function_bodies,
	    "%svoid pre_init_module()\n"
          "{\n", split_to_slices ? "" : "static ");
      if (include_location_info) {
        output->source.static_function_bodies =
          mputstr(output->source.static_function_bodies,
        		  (tcov_enabled && has_pre_init_before_tcov) ? "TTCN_Location_Statistics current_location(\""
        			 	                                     : "TTCN_Location current_location(\"");
        output->source.static_function_bodies =
          Code::translate_string(output->source.static_function_bodies, (tcov_enabled && has_pre_init_before_tcov) ? get_tcov_file_name(get_filename()) : get_filename());
        output->source.static_function_bodies =
          mputprintf(output->source.static_function_bodies,
                     (tcov_enabled && has_pre_init_before_tcov) ? "\", 0, TTCN_Location_Statistics::LOCATION_UNKNOWN, \"%s\");\n"
                       	                                        : "\", 0, TTCN_Location::LOCATION_UNKNOWN, \"%s\");\n", get_modid().get_dispname().c_str());
        if (tcov_enabled && has_pre_init_before_tcov) {
            effective_module_lines =
              mputprintf(effective_module_lines, "%s0",
              		   (effective_module_lines ? ", " : ""));
            effective_module_functions =
              mputprintf(effective_module_functions, "%s\"%s\"",
              		   (effective_module_functions ? ", " : ""), get_modid().get_dispname().c_str());
        }
        if (profiled) {
          output->source.static_function_bodies = mputprintf(output->source.static_function_bodies,
            "%s::init_ttcn3_profiler();\n"
            "TTCN3_Stack_Depth stack_depth;\n"
            "ttcn3_prof.execute_line(\"%s\", 0);\n", get_modid().get_name().c_str(), get_filename());
        }
        if (debugged) {
          output->source.static_function_bodies = mputprintf(output->source.static_function_bodies,
            "%s::init_ttcn3_debugger();\n", get_modid().get_name().c_str());
        }
      }
      output->source.static_function_bodies =
        mputstr(output->source.static_function_bodies, output->functions.pre_init);
      output->source.static_function_bodies =
        mputstr(output->source.static_function_bodies, "}\n\n");
      Free(output->functions.pre_init);
      output->functions.pre_init = NULL;
      has_pre_init = true;
    }
    bool has_post_init_before_tcov = output->functions.post_init != NULL;
    // post_init function
    bool has_post_init = false;
    if (output->functions.post_init) {
      output->source.static_function_prototypes = mputprintf(output->source.static_function_prototypes,
          "%svoid post_init_module();\n", split_to_slices ? "extern " : "static ");
      output->source.static_function_bodies = mputprintf(output->source.static_function_bodies,
	  "%svoid post_init_module()\n"
          "{\n", split_to_slices ? "" : "static ");
      if (include_location_info) {
        output->source.static_function_bodies =
          mputstr(output->source.static_function_bodies,
        		  (tcov_enabled && has_post_init_before_tcov) ? "TTCN_Location_Statistics current_location(\""
        				                                      : "TTCN_Location current_location(\"");
        output->source.static_function_bodies =
          Code::translate_string(output->source.static_function_bodies, (tcov_enabled && has_post_init_before_tcov) ? get_tcov_file_name(get_filename()) : get_filename());
        output->source.static_function_bodies =
          mputprintf(output->source.static_function_bodies,
        		     (tcov_enabled && has_post_init_before_tcov) ? "\", 0, TTCN_Location_Statistics::LOCATION_UNKNOWN, \"%s\");\n"
                      		                                     : "\", 0, TTCN_Location::LOCATION_UNKNOWN, \"%s\");\n", get_modid().get_dispname().c_str());
        if (tcov_enabled && has_post_init_before_tcov) {
          effective_module_lines =
            mputprintf(effective_module_lines, "%s0",
            		   (effective_module_lines ? ", " : ""));
          effective_module_functions =
            mputprintf(effective_module_functions, "%s\"%s\"",
            		   (effective_module_functions ? ", " : ""), get_modid().get_dispname().c_str());
        }
        if (MOD_TTCN == get_moduletype() && is_file_profiled(get_filename())) {
          output->source.static_function_bodies = mputprintf(output->source.static_function_bodies,
            "TTCN3_Stack_Depth stack_depth;\n"
            "ttcn3_prof.execute_line(\"%s\", 0);\n", get_filename());
        }
      }
      output->source.static_function_bodies =
        mputstr(output->source.static_function_bodies, output->functions.post_init);
      output->source.static_function_bodies =
        mputstr(output->source.static_function_bodies, "}\n\n");
      Free(output->functions.post_init);
      output->functions.post_init = NULL;
      has_post_init = true;
    }
    // set_param function
    bool has_set_param;
    if (output->functions.set_param) {
      output->source.static_function_prototypes = mputprintf(output->source.static_function_prototypes,
        "%sboolean set_module_param(Module_Param& param);\n", split_to_slices ? "extern " : "static ");
      output->source.static_function_bodies = mputprintf(output->source.static_function_bodies,
        "%sboolean set_module_param(Module_Param& param)\n"
        "{\n"
           "const char* const par_name = param.get_id()->get_current_name();\n", split_to_slices ? "" : "static ");
      output->source.static_function_bodies =
        mputstr(output->source.static_function_bodies, output->functions.set_param);
      output->source.static_function_bodies =
	mputstr(output->source.static_function_bodies, "return FALSE;\n"
	  "}\n\n");
      Free(output->functions.set_param);
      output->functions.set_param = NULL;
      has_set_param = true;
    } else has_set_param = false;
    // get_param function
    bool has_get_param;
    if (use_runtime_2 && output->functions.get_param) {
      output->source.static_function_prototypes = mputprintf(output->source.static_function_prototypes,
        "%sModule_Param* get_module_param(Module_Param_Name& param_name);\n", split_to_slices ? "extern " : "static ");
      output->source.static_function_bodies = mputprintf(output->source.static_function_bodies,
        "%sModule_Param* get_module_param(Module_Param_Name& param_name)\n"
        "{\n"
           "const char* const par_name = param_name.get_current_name();\n", split_to_slices ? "" : "static ");
      output->source.static_function_bodies =
        mputstr(output->source.static_function_bodies, output->functions.get_param);
      output->source.static_function_bodies =
	mputstr(output->source.static_function_bodies, "return NULL;\n"
	  "}\n\n");
      Free(output->functions.get_param);
      output->functions.get_param = NULL;
      has_get_param = true;
    } else has_get_param = false;
    // log_param function
    bool has_log_param;
    if (output->functions.log_param) {
      output->source.static_function_prototypes = mputprintf(output->source.static_function_prototypes,
	  "%svoid log_module_param();\n", split_to_slices ? "extern " : "static ");
      output->source.static_function_bodies = mputprintf(output->source.static_function_bodies,
	  "%svoid log_module_param()\n"
	  "{\n", split_to_slices ? "" : "static ");
      output->source.static_function_bodies =
        mputstr(output->source.static_function_bodies, output->functions.log_param);
      output->source.static_function_bodies = mputstr(output->source.static_function_bodies,
	"}\n\n");
      Free(output->functions.log_param);
      output->functions.log_param = NULL;
      has_log_param = true;
    } else has_log_param = false;
    // init_comp function
    bool has_init_comp;
    if (output->functions.init_comp) {
      output->source.static_function_prototypes =
	mputprintf(output->source.static_function_prototypes,
	  "%sboolean init_comp_type("
	  "const char *component_type, boolean init_base_comps);\n", split_to_slices ? "extern " : "static ");
      output->source.static_function_bodies =
	mputprintf(output->source.static_function_bodies,
	  "%sboolean init_comp_type(const char *component_type, "
	    "boolean init_base_comps)\n"
	  "{\n(void)init_base_comps;\n", split_to_slices ? "" : "static ");
      output->source.static_function_bodies =
        mputstr(output->source.static_function_bodies,
	  output->functions.init_comp);
      output->source.static_function_bodies =
	mputstr(output->source.static_function_bodies, "return FALSE;\n"
	  "}\n\n");
      Free(output->functions.init_comp);
      output->functions.init_comp = NULL;
      has_init_comp = true;
    } else has_init_comp = false;
    // start function
    bool has_start;
    if (output->functions.start) {
      output->source.static_function_prototypes = mputprintf(output->source.static_function_prototypes,
	 "%sboolean start_ptc_function(const char *function_name, "
	 "Text_Buf& function_arguments);\n", split_to_slices ? "extern " : "static ");
      output->source.static_function_bodies = mputprintf(output->source.static_function_bodies,
	 "%sboolean start_ptc_function(const char *function_name, "
	 "Text_Buf& function_arguments)\n"
	 "{\n", split_to_slices ? "" : "static ");
      output->source.static_function_bodies =
        mputstr(output->source.static_function_bodies, output->functions.start);
      output->source.static_function_bodies =
	mputstr(output->source.static_function_bodies, "return FALSE;\n"
	  "}\n\n");
      Free(output->functions.start);
      output->functions.start = NULL;
      has_start = true;
    } else has_start = false;
    // control part
    bool has_control;
    if (output->functions.control) {
      output->source.static_function_prototypes = mputprintf(output->source.static_function_prototypes,
	  "%svoid module_control_part();\n", split_to_slices ? "extern " : "static ");
      output->source.static_function_bodies = mputprintf(output->source.static_function_bodies,
	  "%svoid module_control_part()\n"
	  "{\n", split_to_slices ? "" : "static ");
      output->source.static_function_bodies =
        mputstr(output->source.static_function_bodies, output->functions.control);
      output->source.static_function_bodies =
	mputstr(output->source.static_function_bodies, "}\n\n");
      Free(output->functions.control);
      output->functions.control = NULL;
      has_control = true;
    } else has_control = false;
    // module checksum
    if (has_checksum) {
      if (split_to_slices) {
        output->header.global_vars = mputprintf(output->header.global_vars, "extern const unsigned char module_checksum[];\n");
      }
      output->source.string_literals = mputprintf(output->source.string_literals,
	"%sconst unsigned char module_checksum[] = {", split_to_slices ? "" : "static ");
      for (size_t i = 0; i < sizeof(module_checksum); i++) {
        if (i > 0) output->source.string_literals =
	  mputc(output->source.string_literals, ',');
	output->source.string_literals =
	  mputprintf(output->source.string_literals, " 0x%02x",
	    module_checksum[i]);
      }
      output->source.string_literals = mputstr(output->source.string_literals,
        " };\n");
    }
    const char *module_name = modid->get_name().c_str();

    // XML namespaces. Written in the order they are stored:
    // sorted ASCIIbetically by the prefix.
    // Not all namespaces are used by every module. Unfortunately, the array
    // (which has the same size in all modules) cannot be compacted, because
    // the indexes have already been used when the XER descriptors were written.
    // All we can do is store NULLs for the unused namespaces.
    size_t num_xml_namespaces = namespaces.size();
    if (moduletype == MOD_TTCN) { //TODO remove this when ASN.1 gets EXER
#ifndef NDEBUG
      output->source.global_vars = mputprintf(output->source.global_vars,
        "// written by %s in " __FILE__ " at %d\n"
        , __FUNCTION__, __LINE__
      );
#endif

      if (num_xml_namespaces != 0 || (control_ns && control_ns_prefix)) {
        output->source.global_vars = mputprintf(output->source.global_vars,
          "%sconst size_t num_namespaces = %lu;\n"
          "%sconst namespace_t xml_namespaces[num_namespaces+1] = {\n"
          , split_to_slices ? "" : "static ", (unsigned long)num_xml_namespaces
          , split_to_slices ? "" : "static "
        );
        if (split_to_slices) {
          output->header.global_vars = mputprintf(output->header.global_vars,
            "extern const size_t num_namespaces;\n");
          output->header.global_vars = mputprintf(output->header.global_vars,
            "extern const namespace_t xml_namespaces[];\n");
        }
        for (size_t i=0; i < namespaces.size(); ++i) {
          if (used_namespaces.has_key(i)) {
            output->source.global_vars = mputprintf(output->source.global_vars,
              "  { \"%s\", \"%s\" },\n", // ns, then prefix
              namespaces.get_nth_elem(i), namespaces.get_nth_key(i).c_str());
          }
          else {
            output->source.global_vars = mputstr(output->source.global_vars,
              "  { NULL, NULL },\n"); // not used in this module
          }
        } // next namespace
        output->source.global_vars = mputprintf(output->source.global_vars,
          "  { \"%s\", \"%s\" }\n};\n\n",
          (control_ns ? control_ns : ""),
          (control_ns_prefix ? control_ns_prefix : ""));
      } // if there are namespaces
    } // if TTCN


    // module object
    output->header.global_vars = mputprintf(output->header.global_vars,
	"extern TTCN_Module %s;\n",
  "module_object");

    output->source.global_vars = mputprintf(output->source.global_vars,
	"TTCN_Module %s(\"%s\", __DATE__, __TIME__, %s, %s",
  "module_object",

	modid->get_dispname().c_str(),
	(has_checksum ? "module_checksum" : "NULL"),
	has_pre_init ? "pre_init_module" : "NULL");

    if (moduletype == MOD_TTCN) {
      // TTCN-3 specific function pointers
      if (product_number == NULL) {
        output->source.global_vars = mputstr(output->source.global_vars, ", NULL");
      } else {
        output->source.global_vars = mputprintf(output->source.global_vars, ", \"%s\"", product_number);
      }
      string extra_str = extra ? ( string('"') + extra + string('"') ) : string("NULL");
      output->source.global_vars = mputprintf(output->source.global_vars,
	", %uU, %uU, %uU, %uU, %s, %luLU, %s, %s, %s, %s, %s, %s, %s, %s",
        suffix, release, patch, build, extra_str.c_str(),
        (unsigned long)num_xml_namespaces,
        ((num_xml_namespaces || (control_ns && control_ns_prefix)) ? "xml_namespaces" : "0"),
	has_post_init ? "post_init_module" : "NULL",
	has_set_param ? "set_module_param" : "NULL",
  has_get_param ? "get_module_param" : "NULL",
	has_log_param ? "log_module_param" : "NULL",
	has_init_comp ? "init_comp_type" : "NULL",
	has_start ? "start_ptc_function" : "NULL",
	has_control ? "module_control_part" : "NULL");
    } else {
      // self checks for ASN.1 modules
      if (has_post_init)
	FATAL_ERROR("Module::generate_functions(): post_init function in ASN.1 module");
      if (has_set_param)
	FATAL_ERROR("Module::generate_functions(): set_param function in ASN.1 module");
      if (has_get_param)
	FATAL_ERROR("Module::generate_functions(): get_param function in ASN.1 module");
      if (has_log_param)
	FATAL_ERROR("Module::generate_functions(): log_param function in ASN.1 module");
      if (has_init_comp)
	FATAL_ERROR("Module::generate_functions(): init_comp function in ASN.1 module");
      if (has_start)
	FATAL_ERROR("Module::generate_functions(): startable function in ASN.1 module");
      if (has_control)
	FATAL_ERROR("Module::generate_functions(): control part in ASN.1 module");
    }
    output->source.global_vars = mputstr(output->source.global_vars, ");\n");
    // #include into the source file
    output->source.includes = mputprintf(output->source.includes,
      "#include \"%s.hh\"\n",
      duplicate_underscores ? module_name : modid->get_ttcnname().c_str());

    output->source.global_vars = mputprintf(output->source.global_vars,
      "\n%sconst RuntimeVersionChecker ver_checker("
      "  current_runtime_version.requires_major_version_%d,\n"
      "  current_runtime_version.requires_minor_version_%d,\n"
      "  current_runtime_version.requires_patch_level_%d,"
      "  current_runtime_version.requires_runtime_%d);\n",
      split_to_slices ? "" : "static ",
      TTCN3_MAJOR, TTCN3_MINOR, TTCN3_PATCHLEVEL, use_runtime_2 ? 2 : 1
      );
    if (split_to_slices) {
      output->header.global_vars = mputprintf(output->header.global_vars,
        "extern const RuntimeVersionChecker ver_checker;\n");
    }
    if (tcov_enabled) {
      output->source.global_vars = mputprintf(output->source.global_vars,
        "\n%sconst int effective_module_lines[] = { %s };\n" \
        "%sconst char *effective_module_functions[] = { %s };\n",
        split_to_slices ? "" : "static ",
        effective_module_lines ? static_cast<const char *>(effective_module_lines) : "",
        split_to_slices ? "" : "static ",     
        effective_module_functions ? static_cast<const char *>(effective_module_functions) : "");
      if (split_to_slices) {
        output->header.global_vars = mputprintf(output->header.global_vars,
          "extern const int effective_module_lines[];\n" \
          "extern const char *effective_module_functions[];\n");
      }
    }
  }

  void Module::generate_conversion_functions(output_struct *output)
  {
    for (size_t i = 0; i < type_conv_v.size(); i++)
      type_conv_v[i]
        ->gen_conv_func(&output->source.static_conversion_function_prototypes,
                        &output->source.static_conversion_function_bodies,
                        this);
  }

  string Module::add_literal(map<string, string>& literals, const string& str,
      const char *prefix)
  {
    if (literals.has_key(str)) return *literals[str];
    else {
       string *literal = new string(prefix+Int2string(literals.size()));
       literals.add(str, literal);
       return *literal;
    }
  }

  void Module::clear_literals(map<string, string>& literals)
  {
    for (size_t i = 0; i < literals.size(); i++)
      delete literals.get_nth_elem(i);
    literals.clear();
  }

  map<string, const char> Module::namespaces;
  map<string, const char> Module::invented_prefixes;
  size_t Module::default_namespace_attempt = 0;
  size_t Module::replacement_for_empty_prefix = (size_t)-1;

  Module::Module(moduletype_t p_mt, Identifier *p_modid)
  : Scope(), moduletype(p_mt), modid(p_modid),
    imp_checked(false), gen_code(false), has_checksum(false),
    visible_mods(), module_checksum(),
    bs_literals(), bp_literals(), hs_literals(), hp_literals(), os_literals(),
    op_literals(), cs_literals(), us_literals(), pp_literals(), mp_literals(),
    oid_literals(), tmp_id_count(0),
    control_ns(p_mt == MOD_ASN ? mcopystr("urn:oid:2.1.5.2.0.1") : NULL),
    control_ns_prefix(p_mt == MOD_ASN ? mcopystr("asn1") : NULL),
    // only ASN.1 modules have default control namespace (X.693 amd1, 16.9)
    used_namespaces(), type_conv_v(), product_number(NULL),
    suffix(0), release(UINT_MAX), patch(UINT_MAX), build(UINT_MAX), extra(NULL)
  {
    if(!p_modid)
      FATAL_ERROR("NULL parameter: Common::Module::Module()");
    memset(module_checksum, 0, sizeof(module_checksum));
    set_scopeMacro_name(modid->get_dispname());
  }

  Module::~Module()
  {
    delete modid;
    visible_mods.clear();
    clear_literals(bs_literals);
    clear_literals(bp_literals);
    clear_literals(hs_literals);
    clear_literals(hp_literals);
    clear_literals(os_literals);
    clear_literals(op_literals);
    clear_literals(cs_literals);
    clear_literals(pp_literals);
    clear_literals(mp_literals);
    for (size_t i = 0; i < us_literals.size(); i++)
      delete us_literals.get_nth_elem(i);
    us_literals.clear();
    for (size_t i = 0; i < oid_literals.size(); i++)
      delete oid_literals.get_nth_elem(i);
    oid_literals.clear();
    for (size_t i = 0; i < type_conv_v.size(); i++)
      delete type_conv_v[i];
    type_conv_v.clear();
    Free(control_ns);
    Free(control_ns_prefix);
    used_namespaces.clear(); // all the values are NULL, no need to free
    // static members below; repeated clear()s are redundant but harmless
    namespaces.clear();
    invented_prefixes.clear();
    Free(product_number);
    Free(extra);
  }

  Type *Module::get_address_type()
  {
    FATAL_ERROR("Common::Module::get_address_type()");
    return 0;
  }

  string Module::add_ustring_literal(const ustring& ustr)
  {
    if (us_literals.has_key(ustr)) return *us_literals[ustr];
    else {
       string *literal = new string("us_" + Int2string(us_literals.size()));
       us_literals.add(ustr, literal);
       return *literal;
    }
  }

  string Module::add_objid_literal(const string& oi_str, const size_t nof_elems)
  {
    if(oid_literals.has_key(oi_str)) return oid_literals[oi_str]->oid_id;
    else {
      OID_literal *oi_struct = new OID_literal;
      oi_struct->nof_elems = nof_elems;
      oi_struct->oid_id = "oi_" + Int2string(oid_literals.size());
      oid_literals.add(oi_str, oi_struct);
      return oi_struct->oid_id;
    }
  }

  void Module::add_type_conv(TypeConv *p_conv)
  {
    if (p_conv == NULL) FATAL_ERROR("Module::add_type_conv()");
    Type *p_from_type = p_conv->get_from_type();
    Type *p_to_type = p_conv->get_to_type();
    if (!p_from_type->is_structured_type()
        || !p_to_type->is_structured_type())
      FATAL_ERROR("Module::add_type_conv()");
    if (p_from_type == p_to_type) {
      // Never add the same types.
      delete p_conv;
      return;
    }
    for (size_t i = 0; i < type_conv_v.size(); i++) {
      TypeConv *conv = type_conv_v[i];
      if (conv->get_from_type() == p_from_type
          && conv->get_to_type() == p_to_type
          && conv->is_temp() == p_conv->is_temp()) {
        // Same pair of types, both for values or templates.  We're the
        // owners, deallocate.
        delete p_conv;
        return;
      }
    }
    type_conv_v.add(p_conv);
  }

  bool Module::needs_type_conv(Type *p_from_type, Type *p_to_type) const
  {
    for (size_t i = 0; i < type_conv_v.size(); i++) {
      TypeConv *conv = type_conv_v[i];
      if (conv->get_from_type() == p_from_type
          && conv->get_to_type() == p_to_type)
        return true;
    }
    return false;
  }

  void Module::chk_recursive(module_set_t& checked_modules)
  {
    if (checked_modules.has_key(this)) return;
    // this must be added to the set at the beginning
    // in order to deal with circular imports
    checked_modules.add(this, 0);
    // check the imported modules first
    size_t nof_visible_mods = visible_mods.size();
    for (size_t i = 0; i < nof_visible_mods; i++) {
      Module *m = visible_mods.get_nth_key(i);
      if (m != this) m->chk_recursive(checked_modules);
    }
    // then check the module itself
    chk(); // this is the only virtual call
  }

  bool Module::is_visible(Module *m)
  {
    collect_visible_mods();
    return visible_mods.has_key(m);
  }

  void Module::get_visible_mods(module_set_t& p_visible_mods)
  {
    if (visible_mods.has_key(this)) {
      size_t nof_visible_mods = visible_mods.size();
      for (size_t i = 0; i < nof_visible_mods; i++) {
	Module *m = visible_mods.get_nth_key(i);
	if (!p_visible_mods.has_key(m)) p_visible_mods.add(m, 0);
      }
    } else {
      get_imported_mods(p_visible_mods);
    }
  }

  void Module::write_checksum()
  {
    fprintf(stderr, "%-18s ", modid->get_dispname().c_str());
    switch (moduletype) {
      case MOD_TTCN: fprintf(stderr, "%-15s ", "TTCN-3"); break;
      case MOD_ASN: fprintf(stderr, "%-15s ", "ASN.1"); break;
      case MOD_UNKNOWN: fprintf(stderr, "%-15s ", "OTHER"); break;
    }

    if (!has_checksum) {
      fputc('\n', stderr);
      return;
    }

    size_t nof_checksum = sizeof(module_checksum);
    for (size_t i = 0; i < nof_checksum; i++) {
      fprintf(stderr, "%02x", module_checksum[i]);
    }

    if (release <= 999999 && patch < 30 && build < 100) {
      char *product_identifier =
        get_product_identifier(product_number, suffix, release, patch, build, extra);
      fprintf(stderr, " %s", product_identifier);
      Free(product_identifier);
    }

    fputc('\n', stderr);
  }

  char* Module::get_product_identifier(const char* product_number,
        const unsigned int suffix, unsigned int release, unsigned int patch,
        unsigned int build, const char* extra)
  {
    expstring_t ret_val = memptystr();
    if ( product_number == NULL
      && suffix == UINT_MAX
      && release == UINT_MAX
      && patch == UINT_MAX
      && build == UINT_MAX) {
      ret_val = mputstr(ret_val, "<RnXnn>");
      return ret_val;
    }
    if (product_number != NULL) {
      ret_val = mputstr(ret_val, product_number);
      if (suffix != 0) {
        ret_val = mputprintf(ret_val, "/%d", suffix);
      }
      ret_val = mputc(ret_val, ' ');
    }

    char* build_str = buildstr(build);
    ret_val = mputprintf(ret_val, "R%u%c%s%s", release, eri(patch), build_str, extra ? extra : "");
    Free(build_str);
    return ret_val;
  }

  void Module::collect_visible_mods()
  {
    if (!visible_mods.has_key(this)) {
      get_imported_mods(visible_mods);
      if (!visible_mods.has_key(this)) visible_mods.add(this, 0);
    }
  }

  void Module::set_checksum(size_t checksum_len,
    const unsigned char* checksum_ptr)
  {
    if (checksum_len != sizeof(module_checksum))
      FATAL_ERROR("Module::set_checksum(): invalid length");
    memcpy(module_checksum, checksum_ptr, sizeof(module_checksum));
    has_checksum = true;
  }

  void Module::set_controlns(char *ns, char *prefix)
  {
    Free(control_ns);
    control_ns = ns;
    Free(control_ns_prefix);
    control_ns_prefix = prefix;
  }

  void Module::get_controlns(const char *&ns, const char *&prefix)
  {
    ns = control_ns;
    prefix = control_ns_prefix;
  }

  const size_t max_invented_prefixes = 10000;
  void Module::add_namespace(const char *new_uri, char *&new_prefix)
  {
    const bool prefix_is_empty = new_prefix && !*new_prefix;
    const string key(new_prefix ? new_prefix : "");
    if (new_prefix && !namespaces.has_key(key)) {
      namespaces.add(key, new_uri);
      if (*new_prefix == 0) { // first add of default namespace
        ++default_namespace_attempt;
      }
      return; // newly added
    }
    else { // prefix already present (or we are required to invent one)
      if (new_prefix) {
        const char *uri_value = namespaces[key];
        if (!strcmp(uri_value, new_uri)) return; // same URI, same prefix: no-op
      }

      // prefix already present but different URI,
      // or prefix is NULL (which means we must invent a prefix)

      if (new_prefix && *new_prefix) {
        Free(new_prefix); // prefix is not empty, discard it and start fresh
        new_prefix = memptystr();
      }

      const string uri_key(new_uri);
      if (invented_prefixes.has_key(uri_key)) {
        // we already made one up for this URI
        new_prefix = mputstr(new_prefix, invented_prefixes[uri_key]);
        return; // already there
      }
      else {
        // make one up on the spot
        size_t iidx = invented_prefixes.size(); // "invented index"
        new_prefix = mputprintf(new_prefix, "tq%04lu", (unsigned long)iidx++);
        string made_up_prefix(new_prefix);
        for (; iidx < max_invented_prefixes; ++iidx) {
          if (namespaces.has_key(made_up_prefix)) {
            // Some pervert wrote an XSD with a namespace prefix like tq0007!
            // Make up another one in the same memory spot.
            sprintf(new_prefix, "tq%04lu", (unsigned long)iidx);
            made_up_prefix = new_prefix;
          }
          else break;
        }

        if (iidx >= max_invented_prefixes) {
          Location loc; // no particular location
          loc.error("Internal limit: too many assigned prefixes");
          return; // not added
        }
        invented_prefixes.add(uri_key, new_prefix);
        namespaces.add(made_up_prefix, new_uri);

        // Search for the newly added prefix and remember it.
        replacement_for_empty_prefix = namespaces.find_key(made_up_prefix);

        if (prefix_is_empty) {
          ++default_namespace_attempt;
        }
        return; // newly added
      }
    } // if (present)
  }

  static const string empty_prefix;
  void Module::rename_default_namespace()
  {
    if (default_namespace_attempt < 2) return;
    // There was more than one default namespace. However, all but the first
    // are already renamed to tq%d.
    size_t di = namespaces.find_key(empty_prefix); // keys are prefixes
    if (di < namespaces.size()) { // found it
      const char *last_remaining_def_namespace = namespaces.get_nth_elem(di);
      // we can't change a key, we can only remove and re-add it
      namespaces.erase(empty_prefix);

      expstring_t empty_prefix_string = NULL; // force a made-up prefix
      add_namespace(last_remaining_def_namespace, empty_prefix_string);
      Free(empty_prefix_string);
    }
    else FATAL_ERROR("Module::rename_default_namespace");
  }

  size_t Module::get_ns_index(const char *prefix)
  {
    size_t idx = namespaces.find_key(string(prefix));
    if (idx >= namespaces.size()) { // not found
      // If the the caller asked for the empty prefix and it wasn't found
      // because it has been replaced, use the replacement.
      if (*prefix == '\0' && replacement_for_empty_prefix != (size_t)-1) {
        idx = replacement_for_empty_prefix;
      }
      else FATAL_ERROR("Module::get_ns_index()");
    }

    // Remember that the index is used by this module
    if (!used_namespaces.has_key(idx)) {
      used_namespaces.add(idx, NULL);
    }
    return idx;
  }

  string Module::get_temporary_id()
  {
    static const string tmp_prefix("tmp_");
    return tmp_prefix + Int2string(tmp_id_count++);
  }

  void Module::generate_code(CodeGenHelper& cgh)
  {
    if (!gen_code) {
      nof_notupdated_files += 2;
      DEBUG(1, "Code not generated for module `%s'.",
        modid->get_dispname().c_str());
      return;
    }
    DEBUG(1, "Generating code for module `%s'.",
      modid->get_dispname().c_str());

    // TODO: Always assume to have circular imports until
    // full program optimization is available,
    // this increases the size of the generated code,
    // but otherwise it is possible to create uncompilable code:
    // 1) let the project of module A and B refer to each other,
    // 2) A refers B, and compile A
    // 3) edit B to refer to A and compile it ...
    // As the code for A can not be rewritten the code will not compile
    cgh.add_module(modid->get_name(), modid->get_ttcnname(),
      moduletype == MOD_TTCN, true);
    cgh.set_current_module(modid->get_ttcnname());
    
    // language specific parts (definitions, imports, etc.)
    //generate_code_internal(&target);  <- needed to pass cgh
    generate_code_internal(cgh);
    
    output_struct* output = cgh.get_current_outputstruct();
    
    CodeGenHelper::update_intervals(output);

    // string literals
    generate_literals(output);
    // module level entry points
    generate_functions(output);
    
    CodeGenHelper::update_intervals(output); // maybe deeper in generate_functions
    
    // type conversion functions for type compatibility
    generate_conversion_functions(output);
    
    CodeGenHelper::update_intervals(output); // maybe deeper in conv_funcs
    
    /* generate the initializer function for the TTCN-3 profiler
     * (this is done at the end of the code generation, to make sure all code 
     * lines have been added to the profiler database) */
    if (is_file_profiled(get_filename())) {
      output->source.global_vars = mputstr(output->source.global_vars,
        "\n/* Initializing TTCN-3 profiler */\n"
        "void init_ttcn3_profiler()\n"
        "{\n");
      char* function_name = 0;
      int line_no = -1;
      while (get_profiler_code_line(get_filename(), &function_name, &line_no)) {
        output->source.global_vars = mputprintf(output->source.global_vars,
          "  ttcn3_prof.create_line(ttcn3_prof.get_element(\"%s\"), %d);\n",
          get_filename(), line_no);
        if (0 != function_name) {
          output->source.global_vars = mputprintf(output->source.global_vars,
            "  ttcn3_prof.create_function(ttcn3_prof.get_element(\"%s\"), %d, \"%s\");\n",
            get_filename(), line_no, function_name);
        }
      }
      output->source.global_vars = mputstr(output->source.global_vars, "}\n");
      if (split_to_slices) {
        output->header.global_vars = mputstr(output->header.global_vars,
          "extern void init_ttcn3_profiler();\n");
      }
    }
    /* TTCN-3 debugger:
       generate the printing function for the types defined in this module
       and initialize the debugger with this module's global variables,
       component types and the components' variables */
    if (debugger_active) {
      generate_debugger_functions(output);
      generate_debugger_init(output);
    }
  }

  void Module::dump(unsigned level) const
  {
    DEBUG(level, "Module: %s", get_modid().get_dispname().c_str());
  }

  // =================================
  // ===== Assignments
  // =================================

  Assignments *Assignments::get_scope_asss()
  {
    return this;
  }

  Assignment *Assignments::get_ass_bySRef(Ref_simple *p_ref)
  {
    if (!p_ref || !parent_scope)
      FATAL_ERROR("NULL parameter: Common::Assignments::get_ass_bySRef()");
    if (!p_ref->get_modid()) {
      const Identifier *id = p_ref->get_id();
      if (id && has_local_ass_withId(*id)) return get_local_ass_byId(*id);
    }
    return parent_scope->get_ass_bySRef(p_ref);
  }

  bool Assignments::has_ass_withId(const Identifier& p_id)
  {
    if (has_local_ass_withId(p_id)) return true;
    else if (parent_scope) return parent_scope->has_ass_withId(p_id);
    else return false;
  }

  // =================================
  // ===== Assignment
  // =================================

  Assignment::Assignment(asstype_t p_asstype, Identifier *p_id)
    : asstype(p_asstype), id(p_id), my_scope(0), checked(false),
      visibilitytype(PUBLIC)
  {
    if (!id) FATAL_ERROR("Assignment::Assignment(): NULL parameter");
  }

  Assignment::Assignment(const Assignment& p)
    : Node(p), Location(p), asstype(p.asstype), id(p.id->clone()), my_scope(0),
      checked(false), visibilitytype(p.visibilitytype)
  {

  }

  Assignment::~Assignment()
  {
    delete id;
  }

  Assignment::asstype_t Assignment::get_asstype() const
  {
    return asstype;
  }

  const char *Assignment::get_assname() const
  {
    switch (get_asstype()) {
    case A_TYPE:
      return "type";
    case A_CONST:
      if (my_scope && my_scope->get_scope_mod()->get_moduletype() ==
          Module::MOD_ASN) return "value";
      else return "constant";
    case A_ERROR:
      return "erroneous assignment";
    case A_OC:
      return "information object class";
    case A_OBJECT:
      return "information object";
    case A_OS:
      return "information object set";
    case A_VS:
      return "value set";
    case A_EXT_CONST:
      return "external constant";
    case A_MODULEPAR:
      return "module parameter";
    case A_MODULEPAR_TEMP:
      return "template module parameter";
    case A_TEMPLATE:
      return "template";
    case A_VAR:
      return "variable";
    case A_VAR_TEMPLATE:
      return "template variable";
    case A_TIMER:
      return "timer";
    case A_PORT:
      return "port";
    case A_FUNCTION:
    case A_FUNCTION_RVAL:
    case A_FUNCTION_RTEMP:
      return "function";
    case A_EXT_FUNCTION:
    case A_EXT_FUNCTION_RVAL:
    case A_EXT_FUNCTION_RTEMP:
      return "external function";
    case A_ALTSTEP:
      return "altstep";
    case A_TESTCASE:
      return "testcase";
    case A_PAR_VAL:
    case A_PAR_VAL_IN:
      return "value parameter";
    case A_PAR_VAL_OUT:
      return "`out' value parameter";
    case A_PAR_VAL_INOUT:
      return "`inout' value parameter";
    case A_PAR_TEMPL_IN:
      return "template parameter";
    case A_PAR_TEMPL_OUT:
      return "`out' template parameter";
    case A_PAR_TEMPL_INOUT:
      return "`inout' template parameter";
    case A_PAR_TIMER:
      return "timer parameter";
    case A_PAR_PORT:
      return "port parameter";
    default:
      return "<unknown>";
    }
  }

  string Assignment::get_description()
  {
    string ret_val(get_assname());
    ret_val += " `";
    switch (asstype) {
    case A_PAR_VAL:
    case A_PAR_VAL_IN:
    case A_PAR_VAL_OUT:
    case A_PAR_VAL_INOUT:
    case A_PAR_TEMPL_IN:
    case A_PAR_TEMPL_OUT:
    case A_PAR_TEMPL_INOUT:
    case A_PAR_TIMER:
    case A_PAR_PORT:
      // parameter is identified using its id
      ret_val += id->get_dispname();
      break;
    case A_CONST:
    case A_TEMPLATE:
    case A_VAR:
    case A_VAR_TEMPLATE:
    case A_TIMER:
      // these can be both local and global
      if (is_local()) ret_val += id->get_dispname();
      else ret_val += get_fullname();
      break;
    default:
      // the rest is always global
      ret_val += get_fullname();
    }
    ret_val += "'";
    return ret_val;
  }

  void Assignment::set_my_scope(Scope *p_scope)
  {
    my_scope=p_scope;
  }

  bool Assignment::is_local() const
  {
    return false;
  }

  Setting *Assignment::get_Setting()
  {
    FATAL_ERROR("Common::Assignment::get_Setting()");
    return 0;
  }

  Type *Assignment::get_Type()
  {
    FATAL_ERROR("Common::Assignment::get_Type()");
    return 0;
  }

  Value *Assignment::get_Value()
  {
    FATAL_ERROR("Common::Assignment::get_Value()");
    return 0;
  }

  Ttcn::Template *Assignment::get_Template()
  {
    FATAL_ERROR("Common::Assignment::get_Template()");
    return 0;
  }

  param_eval_t Assignment::get_eval_type() const
  {
    FATAL_ERROR("Common::Assignment::get_eval_type()");
    return NORMAL_EVAL;
  }

  Ttcn::FormalParList *Assignment::get_FormalParList()
  {
    return 0;
  }

  Ttcn::ArrayDimensions *Assignment::get_Dimensions()
  {
    return 0;
  }

  Type *Assignment::get_RunsOnType()
  {
    return 0;
  }
  
  Type *Assignment::get_PortType()
  {
    return 0;
  }

  void Assignment::chk_ttcn_id()
  {
    if(!my_scope) return;
    if(!get_id().get_has_valid(Identifier::ID_TTCN)
       && my_scope->get_parent_scope()==my_scope->get_scope_mod()
       // <internal> or <error> ...
       && my_scope->get_scope_mod()->get_modid().get_dispname()[0]!='<')
      warning("The identifier `%s' is not reachable from TTCN-3",
              get_id().get_dispname().c_str());
  }

  // *this is the (var/const/modulepar/etc.) definition we want to access.
  // p_scope is the location from where we want to reach the definition.
  string Assignment::get_genname_from_scope(Scope *p_scope,
    const char *p_prefix)
  {
    if (!p_scope || !my_scope)
      FATAL_ERROR("Assignment::get_genname_from_scope()");
    string ret_val;

    Module *my_mod = my_scope->get_scope_mod_gen();
    if ((my_mod != p_scope->get_scope_mod_gen()) &&
    !Asn::Assignments::is_spec_asss(my_mod)) {
    // when the definition is referred from another module
    // the reference shall be qualified with the namespace of my module
    ret_val = my_mod->get_modid().get_name();
    ret_val += "::";
    }
    if (p_prefix) ret_val += p_prefix;
    ret_val += get_genname();
    // add the cast to real type if it's a lazy or fuzzy formal parameter
    switch (asstype) {
    case A_PAR_VAL:
    case A_PAR_VAL_IN:
    case A_PAR_TEMPL_IN:
      if (get_eval_type() != NORMAL_EVAL && p_prefix==NULL) {
        Type* type = get_Type();
        string type_genname = (asstype==A_PAR_TEMPL_IN) ? type->get_genname_template(p_scope) : type->get_genname_value(p_scope);
        ret_val = string("((") + type_genname + string("&)") + ret_val + string(")");
      }
      break;
    default:
      // nothing to do
      break;
    }
    return ret_val;
  }

  const char *Assignment::get_module_object_name()
  {
    if (!my_scope) FATAL_ERROR("Assignment::get_module_object_name()");
    return "module_object";
  }

  void Assignment::use_as_lvalue(const Location&)
  {
    FATAL_ERROR("Common::Assignment::use_as_lvalue()");
  }

  void Assignment::generate_code(output_struct *, bool)
  {
  }

  void Assignment::generate_code(CodeGenHelper&)
  {
  }

  void Assignment::dump(unsigned level) const
  {
    DEBUG(level, "Assignment: %s (%d)",
          id->get_dispname().c_str(), asstype);
  }

  Ttcn::Group* Assignment::get_parent_group()
  {
    return NULL;
  }

} // namespace Common
