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
 *   Delic, Adam
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szalai, Gabor
 *   Zalanyi, Balazs Andor
 *   Pandi, Krisztian
 *
 ******************************************************************************/
#include "AST_asn1.hh"
#include "../Identifier.hh"
#include "../CompilerError.hh"
#include "Block.hh"
#include "TokenBuf.hh" // Ass_pard needs that
#include "../Value.hh"
#include "Object.hh"
#include "../main.hh"
#include "../CodeGenHelper.hh"
#include "../../common/JSON_Tokenizer.hh"
#include "../DebuggerStuff.hh"

/* defined in asn1p.y */
extern int asn1_parse_string(const char* p_str);

extern Common::Modules *modules; // in main.cc

namespace Asn {

  Module *Assignments::_spec_asss=0;
  Assignments *parsed_assignments;

  // =================================
  // ===== Symbols
  // =================================
  Symbols::~Symbols()
  {
    for (size_t i = 0; i < syms_v.size(); i++) delete syms_v[i];
    syms_v.clear();
    syms_m.clear();
  }

  Symbols* Symbols::clone() const
  {
    FATAL_ERROR("Asn::Symbols::clone()");
    return 0;
  }

  void Symbols::add_sym(Identifier *p_id)
  {
    if(!p_id)
      FATAL_ERROR("NULL parameter: Asn::Symbols::add_sym()");
    syms_v.add(p_id);
  }

  void Symbols::chk_uniq(const Location& p_loc)
  {
    for(size_t i=0; i<syms_v.size(); i++) {
      Identifier *id = syms_v[i];
      const string& name = id->get_name();
      if (syms_m.has_key(name)) {
        p_loc.error("Duplicate identifier in SymbolList: `%s'",
          id->get_dispname().c_str());
      } else syms_m.add(name, id);
    }
  }

  // =================================
  // ===== Exports
  // =================================

  Exports::Exports(bool p_expall)
    : Node(), Location(), checked(false), my_mod(0), expall(p_expall),
      symbols(expall ? 0 : new Symbols())
  {}

  Exports::Exports(Symbols *p_symlist)
    : Node(), Location(), checked(false), my_mod(0), expall(false),
      symbols(p_symlist ? p_symlist : new Symbols())
  {}

  Exports::~Exports()
  {
    delete symbols;
  }

  Exports* Exports::clone() const
  {
    FATAL_ERROR("Asn::Exports::clone()");
    return 0;
  }

  bool Exports::exports_sym(const Identifier& p_id)
  {
    if(!checked) chk_exp();
    if (expall) return true;
    else return symbols->syms_m.has_key(p_id.get_name());
  }

  void Exports::chk_exp()
  {
    if(checked) return;
    if(expall) return;
    Ref_defd_simple *ref=0;
    Error_Context cntxt(this, "In EXPORTS of module `%s'",
      my_mod->get_modid().get_dispname().c_str());
    symbols->chk_uniq(*this);
    for(size_t i=0; i<symbols->syms_m.size(); i++) {
      ref=new Ref_defd_simple(0, symbols->syms_m.get_nth_elem(i)->clone());
      /* check whether exists or not */
      my_mod->get_ass_bySRef(ref);
      delete ref; ref=0;
    }
    checked=true;
  }

  // =================================
  // ===== ImpMod
  // =================================

  ImpMod::ImpMod(Identifier *p_modid, Symbols *p_symlist)
    : Node(), Location(), my_mod(0), modid(p_modid), mod(0)
  {
    if(!p_modid)
      FATAL_ERROR("NULL parameter: Asn::ImpMod::ImpMod()");
    set_fullname("<imports>."+modid->get_dispname());
    symbols = p_symlist ? p_symlist : new Symbols();
  }

  ImpMod::~ImpMod()
  {
    delete modid;
    delete symbols;
  }

  ImpMod* ImpMod::clone() const
  {
    FATAL_ERROR("Asn::ImpMod::clone()");
    return 0;
  }

  bool ImpMod::has_sym(const Identifier& p_id) const
  {
    return symbols->syms_m.has_key(p_id.get_name());
  }

  void ImpMod::chk_imp(ReferenceChain& refch)
  {
    if(!my_mod)
      FATAL_ERROR("Asn::ImpMod::chk_imp(): my_mod is NULL");
    Common::Module *m=modules->get_mod_byId(*modid);
    symbols->chk_uniq(*this);
    vector<Common::Module> modules;
    m->chk_imp(refch, modules);
    modules.clear();
    if(m->get_gen_code()) my_mod->set_gen_code();
    for(size_t i=0; i<symbols->syms_m.size(); i++) {
      const Identifier *id=symbols->syms_m.get_nth_elem(i);
      Ref_defd_simple ref(0, new Identifier(*id));
      ref.set_location(*this);
      bool err=false;
      if(!m->get_ass_bySRef(&ref)) err=true;
      if(!err && !m->exports_sym(*id)) {
        error("Symbol `%s' is not exported from module `%s'",
              id->get_dispname().c_str(),
              m->get_modid().get_dispname().c_str());
        /* to avoid more error messages do not set err to true */
        // err=true;
      }
      if(err) {
        symbols->syms_m.erase(symbols->syms_m.get_nth_key(i));
        // do not delete id; it is stored in the vector
        i--;
      }
    }
  }

  void ImpMod::generate_code(output_struct *target)
  {
    const char *module_name = modid->get_name().c_str();

    target->header.includes = mputprintf(target->header.includes,
      "#include \"%s.hh\"\n",
      duplicate_underscores ? module_name : modid->get_ttcnname().c_str());

    target->functions.pre_init = mputprintf(target->functions.pre_init,
      "%s%s.pre_init_module();\n", module_name,
      "::module_object");
  }

  // =================================
  // ===== Imports
  // =================================

  Imports::~Imports()
  {
    for(size_t i=0; i<impmods_v.size(); i++)
      delete impmods_v[i];
    impmods_v.clear();
    impmods.clear();
    impsyms_1.clear();
    impsyms_m.clear();
  }

  Imports* Imports::clone() const
  {
    FATAL_ERROR("Asn::Imports::clone()");
    return 0;
  }

  void Imports::add_impmod(ImpMod *p_impmod)
  {
    if(!p_impmod)
      FATAL_ERROR("NULL parameter: Asn::Imports::add_impmod(): my_mod is NULL");
    impmods_v.add(p_impmod);
    p_impmod->set_my_mod(my_mod);
  }

  void Imports::set_my_mod(Module *p_mod)
  {
    my_mod=p_mod;
    for(size_t i=0; i<impmods_v.size(); i++)
      impmods_v[i]->set_my_mod(my_mod);
  }

  void Imports::chk_imp(ReferenceChain& refch)
  {
    if (checked) return;
    checked = true;
    if (impmods_v.size() <= 0) return;

    if (!my_mod) FATAL_ERROR("Asn::Imports::chk_imp()");

    for (size_t n = 0; n < impmods_v.size(); n++) {
      ImpMod *im = impmods_v[n];
      const Identifier& im_id = im->get_modid();
      Error_Context cntxt(this, "In IMPORTS FROM `%s'",
	im_id.get_dispname().c_str());
      if (!modules->has_mod_withId(im_id)) {
	im->error("There is no module with identifier `%s'",
	  im_id.get_dispname().c_str());
	continue;
      }
      Common::Module *m = modules->get_mod_byId(im_id);
      im->set_mod(m);
      if (m->get_moduletype() != Module::MOD_ASN) {
	im->error("An ASN.1 module cannot import from a TTCN-3 module");
	continue;
      }
      else if (m == my_mod) {
	im->error("A module cannot import from itself");
	continue;
      }
      // check the imports recursively
      refch.mark_state();
      im->chk_imp(refch);
      refch.prev_state();
      // detect circular imports
      if (!is_circular && m->is_visible(my_mod)) is_circular = true;
      const string& im_name = im_id.get_name();
      if (impmods.has_key(im_name)) {
        const char *dispname_str = im_id.get_dispname().c_str();
        im->error("Duplicate import from module `%s'", dispname_str);
	impmods[im_name]->note("Previous import from `%s' is here",
	  dispname_str);
      } else impmods.add(im_name, im);
      Symbols *syms = im->symbols;
      for (size_t i=0; i<syms->syms_m.size(); i++) {
	const Identifier *id = syms->syms_m.get_nth_elem(i);
        const string& key = id->get_name();
        if(impsyms_1.has_key(key)) {
          if(impsyms_1[key]!=m) {
            impsyms_1.erase(key);
            impsyms_m.add(key, 0);
          }
        }
        else if(!impsyms_m.has_key(key)) {
          impsyms_1.add(key, m);
        }
      }
    }
  }

  bool Imports::has_impsym_withId(const Identifier& p_id) const
  {
    if (!checked) FATAL_ERROR("Imports::has_impsym_withId()");
    const string& name = p_id.get_name();
    return impsyms_1.has_key(name) || impsyms_m.has_key(name);
  }

  void Imports::get_imported_mods(Module::module_set_t& p_imported_mods)
  {
    for (size_t i = 0; i < impmods_v.size(); i++) {
      Common::Module *m = impmods_v[i]->get_mod();
      if (!m) continue;
      if (!p_imported_mods.has_key(m)) {
        p_imported_mods.add(m, 0);
	m->get_visible_mods(p_imported_mods);
      }
    }
  }

  void Imports::generate_code(output_struct *target)
  {
    target->header.includes = mputstr(target->header.includes,
      "#include <TTCN3.hh>\n");
    for (size_t i = 0; i < impmods_v.size(); i++) {
      ImpMod *im = impmods_v[i];
      Common::Module *m = im->get_mod();
      // inclusion of m's header file can be eliminated if we find another
      // imported module that imports m
      bool covered = false;
      for (size_t j = 0; j < impmods_v.size(); j++) {
	// skip over the same import definition
	if (j == i) continue;
	Common::Module *m2 = impmods_v[j]->get_mod();
	// a module that is equivalent to the current module due to
	// circular imports cannot be used to cover anything
	if (m2->is_visible(my_mod)) continue;
	if (m2->is_visible(m) && !m->is_visible(m2)) {
	  // m2 covers m (i.e. m is visible from m2)
	  // and they are not in the same import loop
	  covered = true;
	  break;
	}
      }
      // do not generate the #include if a covering module is found
      if (!covered) im->generate_code(target);
    }
  }

  // =================================
  // ===== Module
  // =================================

  Module::Module(Identifier *p_modid, TagDefault::tagdef_t p_tagdef,
                 bool p_extens_impl, Exports *p_exp, Imports *p_imp,
		 Assignments *p_asss)
    : Common::Module(MOD_ASN, p_modid), tagdef(p_tagdef),
      extens_impl(p_extens_impl), exp(p_exp), imp(p_imp), asss(p_asss)
  {
    if (!p_exp || !p_imp || !p_asss)
      FATAL_ERROR("NULL parameter: Asn::Module::Module()");
    if(!p_modid->isvalid_asn_modref()
       && p_modid->get_dispname()!="<internal>")
      error("`%s' is not a valid module identifier",
        p_modid->get_dispname().c_str());
    asss->set_parent_scope(this);
    exp->set_my_mod(this);
    imp->set_my_mod(this);
  }

  Module::~Module()
  {
    delete exp;
    delete imp;
    delete asss;
  }

  Module *Module::clone() const
  {
    FATAL_ERROR("Asn::Module::clone");
    return 0;
  }

  Common::Assignment *Module::importAssignment(
    const Identifier& p_source_modid, const Identifier& p_id) const
  {
    (void)p_source_modid;
    if (asss->has_local_ass_withId(p_id)) {
      return asss->get_local_ass_byId(p_id);
    } else return 0;
  }

  void Module::set_fullname(const string& p_fullname)
  {
    Common::Module::set_fullname(p_fullname);
    exp->set_fullname(p_fullname+".<exports>");
    imp->set_fullname(p_fullname+".<imports>");
    asss->set_fullname(p_fullname);
  }

  Common::Assignments *Module::get_scope_asss()
  {
    return asss;
  }

  bool Module::has_imported_ass_withId(const Identifier& p_id)
  {
    return imp->has_impsym_withId(p_id);
  }

  Common::Assignment* Module::get_ass_bySRef(Ref_simple *p_ref)
  {
    const Identifier *r_modid = p_ref->get_modid();
    const Identifier *r_id = p_ref->get_id();

    // return NULL if the reference is erroneous
    if (!r_id) return 0;

    Common::Module *r_mod=0;

    if(!r_modid || *r_modid==*modid) {
      if(asss->has_local_ass_withId(*r_id))
	return asss->get_local_ass_byId(*r_id);
      if(r_modid) {
        p_ref->error("There is no assignment with name `%s'"
                     " in module `%s'", r_id->get_dispname().c_str(),
                     modid->get_dispname().c_str());
        return 0;
      }
      if(imp->impsyms_1.has_key(r_id->get_name()))
        r_mod=imp->impsyms_1[r_id->get_name()];
      else if(imp->impsyms_m.has_key(r_id->get_name())) {
        p_ref->error("There are more imported symbols with name `%s'"
                     " in module `%s'", r_id->get_dispname().c_str(),
                     modid->get_dispname().c_str());
        return 0;
      }
      else {
        p_ref->error("There is no assignment or imported symbol"
                     " with name `%s' in module `%s'",
                     r_id->get_dispname().c_str(),
                     modid->get_dispname().c_str());
        return 0;
      }
    }
    if(!r_mod) {
      if(!imp->has_impmod_withId(*r_modid)) {
        p_ref->error("There is no imported module with name `%s'",
                     r_modid->get_dispname().c_str());
        return 0;
      }
      if(!imp->get_impmod_byId(*r_modid)->has_sym(*r_id)) {
        p_ref->error("There is no symbol with name `%s'"
                     " imported from module `%s'",
                     r_id->get_dispname().c_str(),
                     r_modid->get_dispname().c_str());
        return 0;
      }
      r_mod=modules->get_mod_byId(*r_modid);
    }
    Ref_defd_simple t_ref(0, r_id->clone());
    return r_mod->get_ass_bySRef(&t_ref);
  }

  Assignments *Module::get_asss()
  {
    return asss;
  }

  bool Module::exports_sym(const Identifier& p_id)
  {
    return exp->exports_sym(p_id);
  }

  void Module::chk_imp(ReferenceChain& refch, vector<Common::Module>& /*moduleStack*/)
  {
    if (imp_checked) return;
    const string& module_name = modid->get_dispname();
    if (refch.exists(module_name)) {
    // Do not warning for circular import in ASN.1 module. It is legal
//      warning("Circular import chain is not recommended: %s", 
//              refch.get_dispstr(module_name).c_str());
      return;
    }
    refch.add(module_name);
    Error_Context backup;
    Error_Context cntxt(this, "In ASN.1 module `%s'", module_name.c_str());
    asss->chk_uniq();
    imp->chk_imp(refch);
    imp_checked = true;
    collect_visible_mods();
  }

  void Module::chk()
  {
    DEBUG(1, "Checking ASN.1 module `%s'", modid->get_dispname().c_str());
    Error_Context cntxt(this, "In ASN.1 module `%s'",
      modid->get_dispname().c_str());
    asss->chk_uniq();
    exp->chk_exp();
    asss->chk();
  }

  void Module::get_imported_mods(module_set_t& p_imported_mods)
  {
    imp->get_imported_mods(p_imported_mods);
  }

  bool Module::has_circular_import()
  {
    return imp->get_is_circular();
  }

  void Module::generate_code_internal(CodeGenHelper& cgh) {
    imp->generate_code(cgh.get_current_outputstruct());
    asss->generate_code(cgh);
  }

  void Module::dump(unsigned level) const
  {
    DEBUG(level, "ASN.1 module: %s", modid->get_dispname().c_str());
    asss->dump(level + 1);
  }

  void Module::add_ass(Assignment *p_ass)
  {
    asss->add_ass(p_ass);
  }
  
  void Module::generate_json_schema(JSON_Tokenizer& json, map<Type*, JSON_Tokenizer>& json_refs)
  {
    // add a new property for this module
    json.put_next_token(JSON_TOKEN_NAME, modid->get_ttcnname().c_str());
    
    // add type definitions into an object
    json.put_next_token(JSON_TOKEN_OBJECT_START);
    
    // cycle through all type assignments, insert schema segments and references
    // when needed
    for (size_t i = 0; i < asss->get_nof_asss(); ++i) {
      Asn::Assignment* asn_ass = dynamic_cast<Asn::Assignment*>(asss->get_ass_byIndex(i));
      if (asn_ass == NULL || asn_ass->get_ass_pard() != NULL) {
        // skip parameterized types
        continue;
      }
      if (Common::Assignment::A_TYPE == asn_ass->get_asstype()) {
        Type* t = asn_ass->get_Type();
        // skip instances of parameterized types
        if (!t->is_pard_type_instance() && t->has_encoding(Type::CT_JSON)) {
          // insert type's schema segment
          t->generate_json_schema(json, false, false);

          if (json_refs_for_all_types && !json_refs.has_key(t)) {
            // create JSON schema reference for the type
            JSON_Tokenizer* json_ref = new JSON_Tokenizer;
            json_refs.add(t, json_ref);
            t->generate_json_schema_ref(*json_ref);
          }
        }
      }
    }
    
    // end of type definitions
    json.put_next_token(JSON_TOKEN_OBJECT_END);
  }
  
  void Module::generate_debugger_init(output_struct */*output*/)
  {
    // no debugging in ASN.1 modules
  }
  
  char* Module::generate_debugger_global_vars(char* str, Common::Module* current_mod)
  {
    for (size_t i = 0; i < asss->get_nof_asss(); ++i) {
      Asn::Assignment* asn_ass = dynamic_cast<Asn::Assignment*>(asss->get_ass_byIndex(i));
      if (asn_ass->get_ass_pard() != NULL) {
        // this check must be done before get_asstype() is called
        continue;
      }
      if (asn_ass->get_asstype() == Common::Assignment::A_CONST) {
        str = generate_code_debugger_add_var(str, asn_ass, current_mod, "global");
      }
    }
    return str;
  }
  
  void Module::generate_debugger_functions(output_struct *output)
  {
    char* print_str = NULL;
    char* overwrite_str = NULL;
    for (size_t i = 0; i < asss->get_nof_asss(); ++i) {
      Asn::Assignment* asn_ass = dynamic_cast<Asn::Assignment*>(asss->get_ass_byIndex(i));
      if (asn_ass->get_ass_pard() != NULL) {
        // skip parameterized types
        // this check must be done before get_asstype() is called
        continue;
      }
      if (Common::Assignment::A_TYPE == asn_ass->get_asstype()) {
        Type* t = asn_ass->get_Type();
        if (!t->is_pard_type_instance() && (t->is_structured_type() ||
            t->get_typetype() == Type::T_ENUM_A ||
            (t->is_ref() && t->get_type_refd()->is_pard_type_instance()))) {
          // only structured types and enums are needed
          // for instances of parameterized types, the last reference, which is
          // not itself an instance of a parameterized type, holds the type's display name
          print_str = mputprintf(print_str, 
            "  %sif (!strcmp(p_var.type_name, \"%s\")) {\n"
            "    ((const %s*)ptr)->log();\n"
            "  }\n"
            "  else if (!strcmp(p_var.type_name, \"%s template\")) {\n"
            "    ((const %s_template*)ptr)->log();\n"
            "  }\n"
            , (print_str != NULL) ? "else " : ""
            , t->get_dispname().c_str(), t->get_genname_value(this).c_str()
            , t->get_dispname().c_str(), t->get_genname_value(this).c_str());
          overwrite_str = mputprintf(overwrite_str, 
            "  %sif (!strcmp(p_var.type_name, \"%s\")) {\n"
            "    ((%s*)p_var.value)->set_param(p_new_value);\n"
            "  }\n"
            "  else if (!strcmp(p_var.type_name, \"%s template\")) {\n"
            "    ((%s_template*)p_var.value)->set_param(p_new_value);\n"
            "  }\n"
            , (overwrite_str != NULL) ? "else " : ""
            , t->get_dispname().c_str(), t->get_genname_value(this).c_str()
            , t->get_dispname().c_str(), t->get_genname_value(this).c_str());
        }
      }
    }
    if (print_str != NULL) {
      // don't generate an empty printing function
      output->header.class_defs = mputprintf(output->header.class_defs,
        "/* Debugger printing and overwriting functions for types declared in this module */\n\n"
        "extern CHARSTRING print_var_%s(const TTCN3_Debugger::variable_t& p_var);\n"
        "extern boolean set_var_%s(TTCN3_Debugger::variable_t& p_var, Module_Param& p_new_value);\n",
        get_modid().get_ttcnname().c_str(), get_modid().get_ttcnname().c_str());
      output->source.global_vars = mputprintf(output->source.global_vars,
        "\n/* Debugger printing function for types declared in this module */\n"
        "CHARSTRING print_var_%s(const TTCN3_Debugger::variable_t& p_var)\n"
        "{\n"
        "  const void* ptr = p_var.set_function != NULL ? p_var.value : p_var.cvalue;\n"
        "  TTCN_Logger::begin_event_log2str();\n"
        "%s"
        "  else {\n"
        "    TTCN_Logger::log_event_str(\"<unrecognized value or template>\");\n"
        "  }\n"
        "  return TTCN_Logger::end_event_log2str();\n"
        "}\n\n"
        "/* Debugger overwriting function for types declared in this module */\n"
        "boolean set_var_%s(TTCN3_Debugger::variable_t& p_var, Module_Param& p_new_value)\n"
        "{\n"
        "%s"
        "  else {\n"
        "    return FALSE;\n"
        "  }\n"
        "  return TRUE;\n"
        "}\n", get_modid().get_ttcnname().c_str(), print_str,
        get_modid().get_ttcnname().c_str(), overwrite_str);
      Free(print_str);
      Free(overwrite_str);
    }
  }

  // =================================
  // ===== Assignments
  // =================================

  Assignments::Assignments(const Assignments& p)
    : Common::Assignments(p), checked(false)
  {
    for(size_t i = 0; i < p.asss_v.size(); i++)
      add_ass(p.asss_v[i]->clone());
  }

  Assignments::~Assignments()
  {
    for (size_t i = 0; i < asss_v.size(); i++) delete asss_v[i];
    asss_v.clear();
    asss_m.clear();
  }

  Assignments *Assignments::clone() const
  {
    return new Assignments(*this);
  }

  void Assignments::set_fullname(const string& p_fullname)
  {
    Common::Assignments::set_fullname(p_fullname);
    string s(p_fullname);
    if (s != "@") s += '.';
    for (size_t i = 0; i < asss_v.size(); i++) {
      Assignment *ass = asss_v[i];
      ass->set_fullname(s+ass->get_id().get_dispname());
    }
  }

  bool Assignments::has_local_ass_withId(const Identifier& p_id)
  {
    if (!checked) chk_uniq();
    if (asss_m.has_key(p_id.get_name())) return true;
    Assignments *spec_asss = _spec_asss->get_asss();
    if (spec_asss != this) return spec_asss->has_ass_withId(p_id);
    else return false;
  }

  Assignment* Assignments::get_local_ass_byId(const Identifier& p_id)
  {
    if (!checked) chk_uniq();
    const string& name = p_id.get_name();
    if (asss_m.has_key(name)) return asss_m[name];
    Assignments *spec_asss = _spec_asss->get_asss();
    if (spec_asss != this) return spec_asss->get_local_ass_byId(p_id);
    else return 0;
  }

  size_t Assignments::get_nof_asss()
  {
    if (!checked) chk_uniq();
    return asss_m.size();
  }

  Common::Assignment* Assignments::get_ass_byIndex(size_t p_i)
  {
    if (!checked) chk_uniq();
    return asss_m.get_nth_elem(p_i);
  }

  void Assignments::add_ass(Assignment *p_ass)
  {
    if (!p_ass) FATAL_ERROR("Asn::Assignments::add_ass()");
    asss_v.add(p_ass);
    p_ass->set_my_scope(this);
    if(checked) {
      const Identifier& id = p_ass->get_id();
      const string& name = id.get_name();
      if(asss_m.has_key(name)) {
        const char *dispname_str = id.get_dispname().c_str();
        p_ass->error("Duplicate assignment with identifier `%s'", dispname_str);
	asss_m[name]->note("Previous assignment with identifier `%s' is here",
	  dispname_str);
      } else asss_m.add(name, p_ass);
    }
  }

  void Assignments::chk()
  {
    for(size_t i = 0; i < asss_v.size(); i++) asss_v[i]->chk();
  }

  void Assignments::chk_uniq()
  {
    if (checked) return;
    asss_m.clear();
    Assignments *spec_asss = _spec_asss->get_asss();
    for(size_t i = 0; i < asss_v.size(); i++) {
      Assignment *ass = asss_v[i];
      const Identifier& id = ass->get_id();
      const string& name = id.get_name();
      if (this != spec_asss && spec_asss->has_ass_withId(id)) {
        ass->error("`%s' is a reserved identifier", id.get_dispname().c_str());
      } else if (asss_m.has_key(name)) {
        const char *dispname_str = id.get_dispname().c_str();
        ass->error("Duplicate assignment with identifier `%s'", dispname_str);
        asss_m[name]->note("Previous assignment with identifier `%s' is here",
	  dispname_str);
      } else asss_m.add(name, ass);
    }
    checked = true;
  }

  void Assignments::set_right_scope(Scope *p_scope)
  {
    for(size_t i = 0; i < asss_v.size(); i++)
      asss_v[i]->set_right_scope(p_scope);
  }

  void Assignments::create_spec_asss()
  {
    if (_spec_asss)
      FATAL_ERROR("Assignments::create_spec_asss(): duplicate initialization");

    const char *s_asss = "$#&&&(#TITAN$#&&^#% Assignments\n"
      "$#&&&(#TITAN$#&&^#% UpperIdentifier\"EXTERNAL\""
      " ::= [UNIVERSAL 8] IMPLICIT SEQUENCE {\n"
      "  identification CHOICE {\n"
      "    syntaxes SEQUENCE {\n"
      "      abstract OBJECT IDENTIFIER,\n"
      "      transfer OBJECT IDENTIFIER\n"
      "    },\n"
      "    syntax OBJECT IDENTIFIER,\n"
      "    presentation-context-id INTEGER,\n"
      "    context-negotiation SEQUENCE {\n"
      "      presentation-context-id INTEGER,\n"
      "      transfer-syntax OBJECT IDENTIFIER\n"
      "    },\n"
      "    transfer-syntax OBJECT IDENTIFIER,\n"
      "    fixed NULL\n"
      "  },\n"
      "  data-value-descriptor ObjectDescriptor OPTIONAL,\n"
      "  data-value OCTET STRING\n"
      "} (WITH COMPONENTS {\n"
      "  ...,\n"
      "  identification (WITH COMPONENTS {\n"
      "    ...,\n"
      "    syntaxes        ABSENT,\n"
      "    transfer-syntax ABSENT,\n"
      "    fixed           ABSENT\n"
      "  })\n"
      "})\n"

      "$#&&&(#TITAN$#&&^#% UpperIdentifier\"EMBEDDED PDV\""
      " ::= [UNIVERSAL 11] IMPLICIT SEQUENCE {\n"
      "  identification CHOICE {\n"
      "    syntaxes SEQUENCE {\n"
      "      abstract OBJECT IDENTIFIER,\n"
      "      transfer OBJECT IDENTIFIER\n"
      "    },\n"
      "    syntax OBJECT IDENTIFIER,\n"
      "    presentation-context-id INTEGER,\n"
      "    context-negotiation SEQUENCE {\n"
      "      presentation-context-id INTEGER,\n"
      "      transfer-syntax OBJECT IDENTIFIER\n"
      "    },\n"
      "    transfer-syntax OBJECT IDENTIFIER,\n"
      "    fixed NULL\n"
      "  },\n"
      "  data-value-descriptor ObjectDescriptor OPTIONAL,\n"
      "  data-value OCTET STRING\n"
      "} (WITH COMPONENTS {\n"
      "  ...,\n"
      "  data-value-descriptor ABSENT\n"
      "})\n"

      "$#&&&(#TITAN$#&&^#% UpperIdentifier\"CHARACTER STRING\""
      " ::= [UNIVERSAL 29] IMPLICIT SEQUENCE {\n"
      "  identification CHOICE {\n"
      "    syntaxes SEQUENCE {\n"
      "      abstract OBJECT IDENTIFIER,\n"
      "      transfer OBJECT IDENTIFIER\n"
      "    },\n"
      "    syntax OBJECT IDENTIFIER,\n"
      "    presentation-context-id INTEGER,\n"
      "    context-negotiation SEQUENCE {\n"
      "      presentation-context-id INTEGER,\n"
      "      transfer-syntax OBJECT IDENTIFIER\n"
      "    },\n"
      "    transfer-syntax OBJECT IDENTIFIER,\n"
      "    fixed NULL\n"
      "  },\n"
      "  data-value-descriptor ObjectDescriptor OPTIONAL,\n"
      "  string-value OCTET STRING\n"
      "} (WITH COMPONENTS {\n"
      "  ...,\n"
      "  data-value-descriptor ABSENT\n"
      "})\n"

      "$#&&&(#TITAN$#&&^#% UpperIdentifier\"REAL\""
      " ::= [UNIVERSAL 9] IMPLICIT SEQUENCE {\n"
      "  mantissa INTEGER,\n"
      "  base INTEGER (2|10),\n"
      "  exponent INTEGER\n"
      "}\n"

      "$#&&&(#TITAN$#&&^#% UpperIdentifier\"TYPE-IDENTIFIER\"\n"
      "::= CLASS\n"
      "{\n"
      "  &id OBJECT IDENTIFIER UNIQUE,\n"
      "  &Type\n"
      "}\n"
      "WITH SYNTAX {\n"
      "  &Type IDENTIFIED BY &id\n"
      "}\n"

      "$#&&&(#TITAN$#&&^#% UpperIdentifier\"ABSTRACT-SYNTAX\""
      " ::= CLASS {\n"
      "  &id OBJECT IDENTIFIER UNIQUE,\n"
      "  &Type,\n"
      "  &property BIT STRING {handles-invalid-encodings(0)} DEFAULT {}\n"
      "}\n"
      "WITH SYNTAX {\n"
      "  &Type IDENTIFIED BY &id [HAS PROPERTY &property]\n"
      "}\n"
      ;

    if(asn1_parse_string(s_asss) || !parsed_assignments)
      FATAL_ERROR("special assignments");
    _spec_asss=new Module
      (new Identifier(Identifier::ID_ASN, string("<internal>")),
       TagDefault::AUTOMATIC, false, new Exports(true), new Imports(),
       parsed_assignments);
    _spec_asss->set_location("<internal>");
    _spec_asss->set_scope_name(_spec_asss->get_modid().get_dispname());

    parsed_assignments->set_fullname(string('@'));
    _spec_asss->chk();

    /*
      // this is used to generate the files which are then
      // included/copied/edited in core library
    _spec_asss->set_gen_code();
    _spec_asss->generate_code();
    */
  }

  void Assignments::destroy_spec_asss()
  {
    if (!_spec_asss)
      FATAL_ERROR("Assignments::destroy_spec_asss(): duplicate cleanup");

    delete _spec_asss;
    _spec_asss = 0;
  }

  bool Assignments::is_spec_asss(Common::Module *p_mod)
  {
    if (!p_mod) FATAL_ERROR("Assignments::is_spec_asss()");
    if (_spec_asss) return p_mod == static_cast<Common::Module*>(_spec_asss);
    else return false;
  }

  void Assignments::generate_code(output_struct* target)
  {
    for (size_t i = 0; i < asss_v.size(); i++) {
      Assignment *ass = asss_v[i];
      if (!top_level_pdu || ass->get_checked()) ass->generate_code(target);
    }
  }

  void Assignments::generate_code(CodeGenHelper& cgh) {
    for (size_t i = 0; i < asss_v.size(); i++) {
      Assignment *ass = asss_v[i];
      if (!top_level_pdu || ass->get_checked()) {
        ass->generate_code(cgh);
        CodeGenHelper::update_intervals(cgh.get_current_outputstruct());
      }
    }
  }

  void Assignments::dump(unsigned level) const
  {
    DEBUG(level, "Assignments (%lu pcs.)", static_cast<unsigned long>( asss_v.size()));
    for(size_t i = 0; i < asss_v.size(); i++) asss_v[i]->dump(level + 1);
  }

  // =================================
  // ===== Ass_pard
  // =================================

  Ass_pard::Ass_pard(Block *p_parlist_block)
    : Common::Node(), parlist_block(p_parlist_block)
  {
    if(!p_parlist_block)
      FATAL_ERROR("NULL parameter: Asn::Ass_pard::Ass_pard()");
  }

  Ass_pard::Ass_pard(const Ass_pard& p)
    : Common::Node(p)
  {
    parlist_block=p.parlist_block?p.parlist_block->clone():0;
    for(size_t i=0; i<p.dummyrefs.size(); i++)
      dummyrefs.add(p.dummyrefs[i]->clone());
    for(size_t i=0; i<p.governors.size(); i++)
      governors.add(p.governors[i]->clone());
  }

  Ass_pard::~Ass_pard()
  {
    delete parlist_block;
    for (size_t i = 0; i < dummyrefs.size(); i++) delete dummyrefs[i];
    dummyrefs.clear();
    for (size_t i = 0; i < governors.size(); i++) delete governors[i];
    governors.clear();
    for (size_t i = 0; i < inst_cnts.size(); i++)
      delete inst_cnts.get_nth_elem(i);
    inst_cnts.clear();
  }

  void Ass_pard::preparse_pars()
  {
    if(!parlist_block) return;
    Error_Context cntxt
      (my_ass, "While checking formal parameters of parameterized"
       " assignment `%s'", my_ass->get_fullname().c_str());
    TokenBuf *parlist_tb=parlist_block->get_TokenBuf();
    enum state_type {S_START, S_GOV, S_DUMMYREF, S_COMMA, S_RDY, S_ERR};
    TokenBuf *gov_tb=0;
    for(state_type st=S_START; st!=S_RDY; ) {
      switch(st) {
      case S_START:
        gov_tb=new TokenBuf();
        switch(parlist_tb->get_at(1)->get_token()) {
        case ',':
        case '\0': // EOF
          st=S_DUMMYREF;
          break;
        default:
          st=S_GOV;
        } // switch
        break; // S_START
      case S_GOV: {
        Token *token=parlist_tb->pop_front_token();
        switch(token->get_token()) {
        case ',':
        case '\0': // EOF
          token->error("Syntax error, premature end of parameter"
                       " (missing `:')");
          delete token;
          st=S_ERR;
          break;
        case ':':
          delete token;
          st=S_DUMMYREF;
          break;
        default:
          gov_tb->push_back_token(token);
        } // switch token
        break;} // S_GOV
      case S_DUMMYREF: {
        Token *token=parlist_tb->pop_front_token();
        switch(token->get_token()) {
        case TOK_UpperIdentifier:
        case TOK_LowerIdentifier:
          dummyrefs.add(token->get_semval_id().clone());
          gov_tb->push_front_token(token);
          gov_tb->push_back_kw_token(TOK_Assignment);
          governors.add(gov_tb);
          gov_tb=0;
          st=S_COMMA;
          break;
        case ',':
        case '\0': // EOF
        default:
          token->error("Syntax error, DummyReference was expected");
          delete token;
          st=S_ERR;
        } // switch token
        break;} // S_DUMMYREF
      case S_COMMA: {
        Token *token=parlist_tb->pop_front_token();
        switch(token->get_token()) {
        case ',':
          st=S_START;
          break;
        case '\0': // EOF
          st=S_RDY;
          break;
        default:
          token->error("Syntax error, `,' was expected");
          st=S_ERR;
        } // switch token
        delete token;
        break;} // S_COMMA
      case S_ERR:
        delete gov_tb;
        for (size_t i = 0; i < dummyrefs.size(); i++) delete dummyrefs[i];
        dummyrefs.clear();
        for (size_t i = 0; i < governors.size(); i++) delete governors[i];
        governors.clear();
        st=S_RDY;
        break; // S_ERR
      case S_RDY:
      default:
        FATAL_ERROR("Ass_pard::preparse_pars()");
      } // switch st
    } // for st
    delete parlist_block;
    parlist_block=0;
  }

  size_t Ass_pard::get_nof_pars()
  {
    if (parlist_block) preparse_pars();
    return dummyrefs.size();
  }

  const Identifier& Ass_pard::get_nth_dummyref(size_t i)
  {
    if (parlist_block) preparse_pars();
    return *(dummyrefs[i]);
  }

  TokenBuf* Ass_pard::clone_nth_governor(size_t i)
  {
    if (parlist_block) preparse_pars();
    return governors[i]->clone();
  }

  size_t Ass_pard::new_instnum(Common::Module *p_mod)
  {
    if (!p_mod) FATAL_ERROR("Ass_pard::new_instnum()");
    if (inst_cnts.has_key(p_mod)) return ++(*inst_cnts[p_mod]);
    else {
      inst_cnts.add(p_mod, new size_t(1));
      return 1;
    }
  }

  // =================================
  // ===== Assignment
  // =================================

  Assignment::Assignment(const Assignment& p)
    : Common::Assignment(p), dontgen(false)
  {
    if(p.ass_pard) {
      ass_pard=p.ass_pard->clone();
      ass_pard->set_my_ass(this);
    }
    else ass_pard=0;
  }

  string Assignment::get_genname() const
  {
    if (!my_scope ||
	my_scope->get_parent_scope() == my_scope->get_scope_mod()) {
      // use the simple identifier if the assignment does not have scope
      // or it is a simple assignment at module scope
      return id->get_name();
    } else {
      // this assignment belongs to an instantiation of a parameterized
      // assignment: use the name of the parent scope to obtain genname
      string genname_asn("@");
      genname_asn += my_scope->get_scope_name();
      const string& id_dispname = id->get_dispname();
      bool is_parass = id_dispname.find('.') == id_dispname.size();
      if (is_parass) {
	// the assignment has a normal identifier:
	// it represents a formal parameter -> actual parameter binding
	// the id (which is the dummy reference) must be used to get a
	// unique genname
	genname_asn += '.';
	genname_asn += id_dispname;
      }
      // otherwise the assignment represents an instance of the parameterized
      // assignment itself: the scope name can be used alone as genname
      string ret_val(Identifier::asn_2_name(genname_asn));
      // in case of parameter assignments a suffix is appended to avoid name
      // clash with the embedded settings of the same instantiation
      if (is_parass) ret_val += "_par_";
      return ret_val;
    }
  }

  Assignment* Assignment::new_instance0()
  {
    // Classes derived from Assignment must implement new_instance0.
    // See Asn::Ass_*::new_instance0
    FATAL_ERROR("Asn::Assignment::new_instance0()");
    return 0;
  }

  Assignment::Assignment(asstype_t p_asstype, Identifier *p_id,
                         Ass_pard *p_ass_pard)
    : Common::Assignment(p_asstype, p_id),
      ass_pard(p_ass_pard), dontgen(false)
  {
    if(ass_pard) ass_pard->set_my_ass(this);
  }

  Assignment::~Assignment()
  {
    delete ass_pard;
  }

  bool Assignment::is_asstype(asstype_t p_asstype, ReferenceChain* refch)
  {
    bool destroy_refch=false;
    if(!refch) {
      refch=new ReferenceChain(this, "While examining kind of assignment");
      destroy_refch=true;
    } else refch->mark_state();
    bool b=(p_asstype==asstype);
    if(!refch->add(get_fullname())) b=p_asstype==A_ERROR;
    if(destroy_refch) delete refch;
    else refch->prev_state();
    return b;
  }

  Assignment* Assignment::new_instance(Common::Module *p_mod)
  {
    if(!ass_pard) {
      error("`%s' is not a parameterized assignment",
            get_fullname().c_str());
      return 0;
    }
    Assignment *new_ass=new_instance0();
    delete new_ass->id; // it was just a temporary, containing "<error>"
    string new_name(id->get_asnname());
    new_name += '.';
    new_name += p_mod->get_modid().get_asnname();
    new_name += ".inst";
    new_name += Int2string(ass_pard->new_instnum(p_mod));
    new_ass->id=new Identifier(Identifier::ID_ASN, new_name);
    return new_ass;
  }

  Type* Assignment::get_Type()
  {
    error("`%s' is not a type assignment", get_fullname().c_str());
    return 0;
  }

  Value* Assignment::get_Value()
  {
    error("`%s' is not a value assignment", get_fullname().c_str());
    return 0;
  }

  ValueSet* Assignment::get_ValueSet()
  {
    error("`%s' is not a valueset assignment", get_fullname().c_str());
    return 0;
  }

  ObjectClass* Assignment::get_ObjectClass()
  {
    error("`%s' is not a objectclass assignment", get_fullname().c_str());
    return 0;
  }

  Object* Assignment::get_Object()
  {
    error("`%s' is not a object assignment", get_fullname().c_str());
    return 0;
  }

  ObjectSet* Assignment::get_ObjectSet()
  {
    error("`%s' is not a objectset assignment", get_fullname().c_str());
    return 0;
  }

  void Assignment::chk()
  {
    if(ass_pard) {
      ass_pard->get_nof_pars();
      checked=true;
      return;
    }
    DEBUG(7, "`%s' assignment not checked.", get_fullname().c_str());
  }

  void Assignment::dump(unsigned level) const
  {
      DEBUG(level, "Assignment(%d): %s%s", get_asstype(),
            id->get_dispname().c_str(), ass_pard?"{}":"");
  }

  // =================================
  // ===== Ass_Undef
  // =================================

  Ass_Undef::Ass_Undef(Identifier *p_id, Ass_pard *p_ass_pard,
                       Node *p_left, Node *p_right)
    : Assignment(A_UNDEF, p_id, p_ass_pard),
      left(p_left), right(p_right), right_scope(0), ass(0)
  {
    if(!p_right)
      FATAL_ERROR("NULL parameter: Asn::Ass_Undef::Ass_Undef()");
  }

  Ass_Undef::Ass_Undef(const Ass_Undef& p)
    : Assignment(p), right_scope(0)
  {
    left=p.left?p.left->clone():0;
    right=p.right?p.right->clone():0;
    ass=p.ass?p.ass->clone():0;
  }

  Ass_Undef::~Ass_Undef()
  {
    delete left;
    delete right;
    delete ass;
  }

  Assignment* Ass_Undef::clone() const
  {
    if(ass) return ass->clone();
    else return new Ass_Undef(*this);
  }

  Assignment* Ass_Undef::new_instance0()
  {
    if(ass) FATAL_ERROR("Asn::Ass_Undef::new_instance0()");
    return new Ass_Undef
      (new Identifier(Identifier::ID_ASN, string("<error>")),
       0, left?left->clone():0, right->clone());
  }

  Assignment::asstype_t Ass_Undef::get_asstype() const
  {
    const_cast<Ass_Undef*>(this)->classify_ass();
    return asstype;
  }

  void Ass_Undef::set_fullname(const string& p_fullname)
  {
    Assignment::set_fullname(p_fullname);
    if (left) left->set_fullname(p_fullname);
    if (right) right->set_fullname(p_fullname);
    if (ass) ass->set_fullname(p_fullname);
  }

  void Ass_Undef::set_my_scope(Scope *p_scope)
  {
    Assignment::set_my_scope(p_scope);
    if(ass) ass->set_my_scope(p_scope);
    right_scope=p_scope;
  }

  void Ass_Undef::set_right_scope(Scope *p_scope)
  {
    if(ass) ass->set_right_scope(p_scope);
    right_scope=p_scope;
  }

  bool Ass_Undef::is_asstype(asstype_t p_asstype, ReferenceChain* refch)
  {
    classify_ass(refch);
    return asstype != A_ERROR ? ass->is_asstype(p_asstype, refch) : false;
  }
  
  Ass_pard* Ass_Undef::get_ass_pard() const
  {
    if (NULL != ass) {
      return ass->get_ass_pard();
    }
    return ass_pard;
  }

  bool Ass_Undef::_error_if_pard()
  {
    if(ass_pard) {
      error("`%s' is a parameterized assignment", get_fullname().c_str());
      return true;
    }
    return false;
  }

  Setting* Ass_Undef::get_Setting()
  {
    if(_error_if_pard()) return 0;
    if(!checked) chk();
    return ass->get_Setting();
  }

  Type* Ass_Undef::get_Type()
  {
    if(_error_if_pard()) return 0;
    if(!checked) chk();
    return ass->get_Type();
  }

  Value* Ass_Undef::get_Value()
  {
    if(_error_if_pard()) return 0;
    if(!checked) chk();
    return ass->get_Value();
  }

  ValueSet* Ass_Undef::get_ValueSet()
  {
    if(_error_if_pard()) return 0;
    if(!checked) chk();
    return ass->get_ValueSet();
  }

  ObjectClass* Ass_Undef::get_ObjectClass()
  {
    if(_error_if_pard()) return 0;
    if(!checked) chk();
    return ass->get_ObjectClass();
  }

  Object* Ass_Undef::get_Object()
  {
    if(_error_if_pard()) return 0;
    if(!checked) chk();
    return ass->get_Object();
  }

  ObjectSet* Ass_Undef::get_ObjectSet()
  {
    if(_error_if_pard()) return 0;
    if(!checked) chk();
    return ass->get_ObjectSet();
  }

  void Ass_Undef::chk()
  {
    if(checked) return;
    if(ass_pard) {
      ass_pard->get_nof_pars();
      checked=true;
      return;
    }
    classify_ass();
    ass->chk();
    checked=true;
  }

  void Ass_Undef::generate_code(output_struct *target, bool)
  {
    if (ass_pard || dontgen) return;
    classify_ass();
    ass->generate_code(target);
  }

  void Ass_Undef::generate_code(CodeGenHelper& cgh) {
    if (ass_pard || dontgen) return;
    classify_ass();
    ass->generate_code(cgh);
    CodeGenHelper::update_intervals(cgh.get_current_outputstruct());
  }

  void Ass_Undef::dump(unsigned level) const
  {
    if(ass)
      ass->dump(level);
    else {
      DEBUG(level, "Undef assignment: %s%s",
            id->get_dispname().c_str(), ass_pard?"{}":"");
    }
  }

  void Ass_Undef::classify_ass(ReferenceChain *refch)
  {
    if(asstype!=A_UNDEF) return;
    bool destroy_refch=false;
    if(!refch) {
      refch=new ReferenceChain(this, "While examining kind of assignment");
      destroy_refch=true;
    } else refch->mark_state();

    Error_Context ec_backup(1);
    Error_Context cntxt(this, "In assignment `%s'",
      id->get_dispname().c_str());

    /* temporary pointers */
    Reference *t_ref=0;
    Reference *t_ref2=0;
    Block *t_block=0;

    if(!refch->add(get_fullname()))
      goto error;
    if((t_ref=dynamic_cast<Reference*>(left))) {
      t_ref->set_my_scope(my_scope);
      if(t_ref->refers_to_st(Setting::S_ERROR, refch))
        goto error;
    }
    if((t_ref=dynamic_cast<Reference*>(right))) {
      t_ref->set_my_scope(right_scope);
      /*
      if(t_ref->refers_to_st(Setting::S_ERROR, refch))
        t_ref->error("Cannot recognize assignment `%s'",
          t_ref->get_dispname().c_str());
      */
    }

    if(id->isvalid_asn_objclassref()
       && !left
       && (t_ref=dynamic_cast<Ref_defd*>(right))
       && t_ref->refers_to_st(Setting::S_OC, refch)
       ) {
      ass=new Ass_OC(id->clone(), ass_pard, new OC_refd(t_ref));
      ass_pard=0;
      right=0;
      asstype=A_OC;
    }
    else if(id->isvalid_asn_typeref()
            && !left
            && (t_ref=dynamic_cast<Ref_defd*>(right))
            && (t_ref->refers_to_st(Setting::S_T, refch)
                || t_ref->refers_to_st(Setting::S_VS, refch))
            ) {
      Type *t_type=new Type(Type::T_REFD, t_ref);
      t_type->set_location(*t_ref);
      ass=new Ass_T(id->clone(), ass_pard, t_type);
      ass_pard=0;
      right=0;
      asstype=A_TYPE;
    }
    else if(id->isvalid_asn_objsetref()
            && (t_ref=dynamic_cast<Ref_simple*>(left))
            && (t_block=dynamic_cast<Block*>(right))
            && t_ref->refers_to_st(Setting::S_OC, refch)
            ) {
      ass=new Ass_OS(id->clone(), ass_pard,
                     new OC_refd(t_ref), new OS_defn(t_block));
      ass_pard=0;
      left=0;
      right=0;
      asstype=A_OS;
    }
    else if(id->isvalid_asn_valsetref()
            && (t_ref=dynamic_cast<Ref_simple*>(left))
            && (t_block=dynamic_cast<Block*>(right))
            && (t_ref->refers_to_st(Setting::S_T, refch)
                || t_ref->refers_to_st(Setting::S_VS, refch))
              ) {
      Type *t_type=new Type(Type::T_REFD, t_ref);
      t_type->set_location(*t_ref);
      ass=new Ass_VS(id->clone(), ass_pard, t_type, t_block);
      ass_pard=0;
      left=0;
      right=0;
      asstype=A_VS;
    }
    else if(id->isvalid_asn_objref()
            && (t_ref=dynamic_cast<Ref_simple*>(left))
            && ((t_block=dynamic_cast<Block*>(right))
                || (t_ref2=dynamic_cast<Reference*>(right)))
            && t_ref->refers_to_st(Setting::S_OC, refch)
            ) {
      OC_refd *t_oc=new OC_refd(t_ref);
      t_oc->set_location(*t_ref);
      if(t_block) {
        Obj_defn *t_obj=new Obj_defn(t_block);
        t_obj->set_location(*t_block);
        ass=new Ass_O(id->clone(), ass_pard, t_oc, t_obj);
      }
      else {
        Obj_refd *t_obj=new Obj_refd(t_ref2);
        t_obj->set_location(*t_ref2);
        ass=new Ass_O(id->clone(), ass_pard, t_oc, t_obj);
      }
      ass_pard=0;
      left=0;
      right=0;
      asstype=A_OBJECT;
    }
    else if(id->isvalid_asn_valref()
            && (t_ref=dynamic_cast<Ref_simple*>(left))
            && ((t_block=dynamic_cast<Block*>(right))
                || (t_ref2=dynamic_cast<Reference*>(right)))
            && (t_ref->refers_to_st(Setting::S_T, refch)
                || t_ref->refers_to_st(Setting::S_VS, refch))
            ) {
      Type *t_type=new Type(Type::T_REFD, t_ref);
      t_type->set_location(*t_ref);
      if(t_block) {
        Value *t_value=new Value(Value::V_UNDEF_BLOCK, t_block);
        t_value->set_location(*t_block);
        ass=new Ass_V(id->clone(), ass_pard, t_type, t_value);
      }
      else {
        Ref_defd_simple *t_ref3=dynamic_cast<Ref_defd_simple*>(t_ref2);
        if(t_ref3 && !t_ref3->get_modid()) {
          Value *t_val=new Value(Value::V_UNDEF_LOWERID,
                                 t_ref3->get_id()->clone());
          t_val->set_location(*t_ref3);
          ass=new Ass_V(id->clone(), ass_pard, t_type, t_val);
          delete right;
        }
        else {
          Value *t_val=new Value(Value::V_REFD, t_ref2);
          t_val->set_location(*t_ref2);
          ass=new Ass_V(id->clone(), ass_pard, t_type, t_val);
        }
      }
      ass_pard=0;
      left=0;
      right=0;
      asstype=A_CONST;
    }
    else {
      goto error;
    }
    goto end;
  error:
    error("Cannot recognize assignment");
    ass = new Ass_Error(id->clone(), ass_pard);
    asstype=A_ERROR;
  end:
    ass->set_location(*this);
    ass->set_my_scope(my_scope);
    ass->set_right_scope(right_scope);
    ass->set_fullname(get_fullname());
    if(destroy_refch) delete refch;
    else refch->prev_state();
  }

  // =================================
  // ===== Ass_Error
  // =================================

  Ass_Error::Ass_Error(Identifier *p_id, Ass_pard *p_ass_pard)
    : Assignment(A_ERROR, p_id, p_ass_pard),
      setting_error(0), type_error(0), value_error(0)
  {
  }

  Ass_Error::~Ass_Error()
  {
    delete setting_error;
    delete type_error;
    delete value_error;
  }

  Assignment* Ass_Error::clone() const
  {
    return new Ass_Error(id->clone(), ass_pard);
  }

  Assignment* Ass_Error::new_instance0()
  {
    return new Ass_Error
      (new Identifier(Identifier::ID_ASN, string("<error>")), 0);
  }

  bool Ass_Error::is_asstype(asstype_t p_asstype, ReferenceChain*)
  {
    return p_asstype==A_ERROR;
  }

  Setting* Ass_Error::get_Setting()
  {
    if(!setting_error)
      setting_error = new Common::Setting_Error();
    return setting_error;
  }

  Type* Ass_Error::get_Type()
  {
    if(!type_error)
      type_error = new Type(Type::T_ERROR);
    return type_error;
  }

  Value* Ass_Error::get_Value()
  {
    if(!value_error)
      value_error = new Value(Value::V_ERROR);
    return value_error;
  }

  ValueSet* Ass_Error::get_ValueSet()
  {
    FATAL_ERROR("Ass_Error::get_ValueSet()");
    return 0;
  }

  ObjectClass* Ass_Error::get_ObjectClass()
  {
    FATAL_ERROR("Ass_Error::get_ObjectClass()");
    return 0;
  }

  Object* Ass_Error::get_Object()
  {
    FATAL_ERROR("Ass_Error::get_Object()");
    return 0;
  }

  ObjectSet* Ass_Error::get_ObjectSet()
  {
    FATAL_ERROR("Ass_Error::get_ObjectSet()");
    return 0;
  }

  void Ass_Error::chk()
  {
    checked=true;
  }

  void Ass_Error::dump(unsigned level) const
  {
    DEBUG(level, "Erroneous assignment: %s%s",
      id->get_dispname().c_str(), ass_pard?"{}":"");
  }

  // =================================
  // ===== Ass_T
  // =================================

  Ass_T::Ass_T(Identifier *p_id, Ass_pard *p_ass_pard, Type *p_right)
    : Assignment(A_TYPE, p_id, p_ass_pard)
  {
    if(!p_right)
      FATAL_ERROR("NULL parameter: Asn::Ass_T::Ass_T()");
    p_right->set_ownertype(Type::OT_TYPE_ASS, this);
    right=p_right;
  }

  Ass_T::Ass_T(const Ass_T& p)
    : Assignment(p)
  {
    right=p.right->clone();
  }

  Ass_T::~Ass_T()
  {
    delete right;
  }

  Assignment* Ass_T::new_instance0()
  {
    return new Ass_T
      (new Identifier(Identifier::ID_ASN, string("<error>")), 0, right->clone());
  }

  void Ass_T::set_fullname(const string& p_fullname)
  {
    Assignment::set_fullname(p_fullname);
    right->set_fullname(p_fullname);
  }

  void Ass_T::set_my_scope(Scope *p_scope)
  {
    Assignment::set_my_scope(p_scope);
    right->set_my_scope(p_scope);
  }

  void Ass_T::set_right_scope(Scope *p_scope)
  {
    right->set_my_scope(p_scope);
  }

  Type* Ass_T::get_Type()
  {
    if(ass_pard) {
      error("`%s' is a parameterized type assignment",
            get_fullname().c_str());
      return 0;
    }
    chk();
    return right;
  }

  void Ass_T::chk()
  {
    if (checked) return;
    checked = true;
    Error_Context cntxt(this, "In type assignment `%s'",
                        id->get_dispname().c_str());
    if (ass_pard) {
      ass_pard->get_nof_pars();
      return;
    }
    chk_ttcn_id();
    right->set_genname(get_genname());
    right->chk();
    ReferenceChain refch(right, "While checking embedded recursions");
    right->chk_recursions(refch);
  }

  void Ass_T::generate_code(output_struct *target, bool)
  {
    right->generate_code(target);
  }

  void Ass_T::generate_code(CodeGenHelper& cgh) {
    if (ass_pard || dontgen) return;
    generate_code(cgh.get_outputstruct(right));
    cgh.finalize_generation(right);
  }

  void Ass_T::dump(unsigned level) const
  {
    DEBUG(level, "Type assignment: %s%s",
          id->get_dispname().c_str(), ass_pard?"{}":"");
    level++;
    id->dump(level);
    if(!ass_pard)
      right->dump(level);
  }

  // =================================
  // ===== Ass_V
  // =================================

  Ass_V::Ass_V(Identifier *p_id, Ass_pard *p_ass_pard,
               Type *p_left, Value *p_right)
    : Assignment(A_CONST, p_id, p_ass_pard)
  {
    if(!p_left || !p_right)
      FATAL_ERROR("NULL parameter: Asn::Ass_V::Ass_V()");
    left=p_left;
    left->set_ownertype(Type::OT_VAR_ASS, this);
    right=p_right;
  }

  Ass_V::Ass_V(const Ass_V& p)
    : Assignment(p)
  {
    left=p.left->clone();
    right=p.right->clone();
  }

  Ass_V::~Ass_V()
  {
    delete left;
    delete right;
  }

  Assignment* Ass_V::new_instance0()
  {
    return new Ass_V
      (new Identifier(Identifier::ID_ASN, string("<error>")), 0,
       left->clone(), right->clone());
  }

  void Ass_V::set_fullname(const string& p_fullname)
  {
    Assignment::set_fullname(p_fullname);
    left->set_fullname(p_fullname + ".<type>");
    right->set_fullname(p_fullname);
  }

  void Ass_V::set_my_scope(Scope *p_scope)
  {
    Assignment::set_my_scope(p_scope);
    left->set_my_scope(p_scope);
    right->set_my_scope(p_scope);
  }

  void Ass_V::set_right_scope(Scope *p_scope)
  {
    right->set_my_scope(p_scope);
  }


  Type* Ass_V::get_Type()
  {
    chk();
    return left;
  }

  Value* Ass_V::get_Value()
  {
    if(ass_pard) {
      error("`%s' is a parameterized value assignment",
            get_fullname().c_str());
      return 0;
    }
    chk();
    return right;
  }

  void Ass_V::chk()
  {
    if(checked) return;
    Error_Context cntxt(this, "In value assignment `%s'",
                        id->get_dispname().c_str());
    if(ass_pard) {
      ass_pard->get_nof_pars();
      checked=true;
      return;
    }
    chk_ttcn_id();
    static const string _T_("_T_");
    left->set_genname(_T_, get_genname());
    left->chk();
    right->set_my_governor(left);
    left->chk_this_value_ref(right);
    checked=true;
    left->chk_this_value(right, 0, Type::EXPECTED_CONSTANT, INCOMPLETE_NOT_ALLOWED,
      OMIT_NOT_ALLOWED, SUB_CHK, IMPLICIT_OMIT);
    {
      ReferenceChain refch(right, "While checking embedded recursions");
      right->chk_recursions(refch);
    }
    if (!semantic_check_only) {
      right->set_genname_prefix("const_");
      right->set_genname_recursive(get_genname());
      right->set_code_section(GovernedSimple::CS_PRE_INIT);
    }
  }

  void Ass_V::generate_code(output_struct *target, bool)
  {
    if (ass_pard || dontgen) return;
    left->generate_code(target);
    const_def cdef;
    Code::init_cdef(&cdef);
    left->generate_code_object(&cdef, right);
    cdef.init = right->generate_code_init(cdef.init,
      right->get_lhs_name().c_str());
    Code::merge_cdef(target, &cdef);
    Code::free_cdef(&cdef);
  }

  void Ass_V::generate_code(CodeGenHelper& cgh) {
    generate_code(cgh.get_current_outputstruct());
  }

  void Ass_V::dump(unsigned level) const
  {
    DEBUG(level, "Value assignment: %s%s",
          id->get_dispname().c_str(), ass_pard?"{}":"");
    level++;
    left->dump(level);
    if(!ass_pard)
      right->dump(level);
  }

  // =================================
  // ===== Ass_VS
  // =================================

  Ass_VS::Ass_VS(Identifier *p_id, Ass_pard *p_ass_pard,
                 Type *p_left, Block *p_right)
    : Assignment(A_VS, p_id, p_ass_pard), right_scope(0)
  {
    if(!p_left || !p_right)
      FATAL_ERROR("NULL parameter: Asn::Ass_VS::Ass_VS()");
    left=p_left;
    left->set_ownertype(Type::OT_VSET_ASS, this);
    right=p_right;
  }

  Ass_VS::Ass_VS(const Ass_VS& p)
    : Assignment(p), right_scope(0)
  {
    left=p.left->clone();
    right=p.right->clone();
  }

  Ass_VS::~Ass_VS()
  {
    delete left;
    delete right;
  }

  Assignment* Ass_VS::new_instance0()
  {
    return new Ass_VS
      (new Identifier(Identifier::ID_ASN, string("<error>")), 0,
       left->clone(), right->clone());
  }

  void Ass_VS::set_fullname(const string& p_fullname)
  {
    Assignment::set_fullname(p_fullname);
    left->set_fullname(p_fullname);
    right->set_fullname(p_fullname);
  }

  void Ass_VS::set_my_scope(Scope *p_scope)
  {
    Assignment::set_my_scope(p_scope);
    left->set_my_scope(p_scope);
  }

  void Ass_VS::set_right_scope(Scope *p_scope)
  {
    right_scope=p_scope;
  }

  Type* Ass_VS::get_Type()
  {
    if(ass_pard) {
      error("`%s' is a parameterized value set assignment",
            get_fullname().c_str());
      return 0;
    }
    chk();
    return left;
  }

  void Ass_VS::chk()
  {
    if (checked) return;
    checked = true;
    Error_Context cntxt(this, "In value set assignment `%s'",
                        id->get_dispname().c_str());
    if (ass_pard) {
      ass_pard->get_nof_pars();
      return;
    }

    // parse the content of right and add it to left as one more constraint
    Node *node = right->parse(KW_Block_ValueSet);
    Constraint* vs_constr = dynamic_cast<Constraint*>(node);
    if (vs_constr) { // if we have a constraint add it to the type
      if (right_scope) { // if this is a parameter of a pard type
        vs_constr->set_my_scope(right_scope);
      }
      if (!left->get_constraints()) left->add_constraints(new Constraints());
      left->get_constraints()->add_con(vs_constr);
    }

    chk_ttcn_id();
    left->set_genname(get_genname());
    left->chk();
    ReferenceChain refch(left, "While checking embedded recursions");
    left->chk_recursions(refch);
  }

  void Ass_VS::generate_code(output_struct *target, bool)
  {
    left->generate_code(target);
  }

  void Ass_VS::generate_code(CodeGenHelper& cgh) {
    if (ass_pard || dontgen) return;
    generate_code(cgh.get_outputstruct(left));
    cgh.finalize_generation(left);
  }

  void Ass_VS::dump(unsigned level) const
  {
    DEBUG(level, "Value set assignment: %s%s",
          id->get_dispname().c_str(), ass_pard?"{}":"");
    level++;
    id->dump(level);
    if(!ass_pard) {
      left->dump(level);
      right->dump(level);
    }
  }

  // =================================
  // ===== Ass_OC
  // =================================

  Ass_OC::Ass_OC(Identifier *p_id, Ass_pard *p_ass_pard,
                 ObjectClass *p_right)
    : Assignment(A_OC, p_id, p_ass_pard)
  {
    if(!p_right)
      FATAL_ERROR("NULL parameter: Asn::Ass_OC::Ass_OC()");
    right=p_right;
  }

  Ass_OC::Ass_OC(const Ass_OC& p)
    : Assignment(p)
  {
    right=p.right->clone();
  }

  Ass_OC::~Ass_OC()
  {
    delete right;
  }

  Assignment* Ass_OC::new_instance0()
  {
    return new Ass_OC
      (new Identifier(Identifier::ID_ASN, string("<error>")), 0, right->clone());
  }

  void Ass_OC::set_fullname(const string& p_fullname)
  {
    Assignment::set_fullname(p_fullname);
    right->set_fullname(p_fullname);
  }

  void Ass_OC::set_my_scope(Scope *p_scope)
  {
    Assignment::set_my_scope(p_scope);
    right->set_my_scope(p_scope);
  }

  void Ass_OC::set_right_scope(Scope *p_scope)
  {
    right->set_my_scope(p_scope);
  }

  void Ass_OC::chk()
  {
    if(checked) return;
    Error_Context cntxt(this, "In information object class assignment `%s'",
                        id->get_dispname().c_str());
    if(ass_pard) {
      ass_pard->get_nof_pars();
      checked=true;
      return;
    }
    right->set_genname(get_genname());
    right->chk();
    checked=true;
  }

  ObjectClass* Ass_OC::get_ObjectClass()
  {
    if(ass_pard) {
      error("`%s' is a parameterized objectclass assignment",
            get_fullname().c_str());
      return 0;
    }
    chk();
    return right;
  }

  void Ass_OC::generate_code(output_struct *target, bool)
  {
    if (ass_pard || dontgen) return;
    right->generate_code(target);
  }

  void Ass_OC::generate_code(CodeGenHelper& cgh) {
    generate_code(cgh.get_current_outputstruct());
  }

  void Ass_OC::dump(unsigned level) const
  {
    DEBUG(level, "ObjectClass assignment: %s%s",
          id->get_dispname().c_str(), ass_pard?"{}":"");
    level++;
    if(!ass_pard)
      right->dump(level);
  }

  // =================================
  // ===== Ass_O
  // =================================

  Ass_O::Ass_O(Identifier *p_id, Ass_pard *p_ass_pard,
               ObjectClass *p_left, Object *p_right)
    : Assignment(A_OBJECT, p_id, p_ass_pard)
  {
    if(!p_left || !p_right)
      FATAL_ERROR("NULL parameter: Asn::Ass_O::Ass_O()");
    left=p_left;
    right=p_right;
  }

  Ass_O::Ass_O(const Ass_O& p)
    : Assignment(p)
  {
    left=p.left->clone();
    right=p.right->clone();
  }

  Ass_O::~Ass_O()
  {
    delete left;
    delete right;
  }

  Assignment* Ass_O::new_instance0()
  {
    return new Ass_O
      (new Identifier(Identifier::ID_ASN, string("<error>")), 0,
       left->clone(), right->clone());
  }

  void Ass_O::set_fullname(const string& p_fullname)
  {
    Assignment::set_fullname(p_fullname);
    left->set_fullname(p_fullname);
    right->set_fullname(p_fullname);
  }

  void Ass_O::set_my_scope(Scope *p_scope)
  {
    Assignment::set_my_scope(p_scope);
    left->set_my_scope(p_scope);
    right->set_my_scope(p_scope);
  }

  void Ass_O::set_right_scope(Scope *p_scope)
  {
    right->set_my_scope(p_scope);
  }

  /*
    ObjectClass* Ass_O::get_ObjectClass()
    {
    return left;
    }
  */

  Object* Ass_O::get_Object()
  {
    if(ass_pard) {
      error("`%s' is a parameterized object assignment",
            get_fullname().c_str());
      return 0;
    }
    chk();
    return right;
  }

  void Ass_O::chk()
  {
    if(checked) return;
    Error_Context cntxt(this, "In information object assignment `%s'",
                        id->get_dispname().c_str());
    if(ass_pard) {
      ass_pard->get_nof_pars();
      checked=true;
      return;
    }
    left->chk();
    right->set_my_governor(left);
    right->set_genname(get_genname());
    right->chk();
    checked=true;
  }

  void Ass_O::generate_code(output_struct *target, bool)
  {
    if (ass_pard || dontgen) return;
    left->generate_code(target);
    right->generate_code(target);
  }

  void Ass_O::generate_code(CodeGenHelper& cgh) {
    generate_code(cgh.get_current_outputstruct());
  }

  void Ass_O::dump(unsigned level) const
  {
    DEBUG(level, "Object assignment: %s%s",
          id->get_dispname().c_str(), ass_pard?"{}":"");
    level++;
    left->dump(level);
    if(!ass_pard)
      right->dump(level);
  }

  // =================================
  // ===== Ass_OS
  // =================================

  Ass_OS::Ass_OS(Identifier *p_id, Ass_pard *p_ass_pard,
                 ObjectClass *p_left, ObjectSet *p_right)
    : Assignment(A_OS, p_id, p_ass_pard)
  {
    if(!p_left || !p_right)
      FATAL_ERROR("NULL parameter: Asn::Ass_OS::Ass_OS()");
    left=p_left;
    right=p_right;
  }

  Ass_OS::Ass_OS(const Ass_OS& p)
    : Assignment(p)
  {
    left=p.left->clone();
    right=p.right->clone();
  }

  Ass_OS::~Ass_OS()
  {
    delete left;
    delete right;
  }

  Assignment* Ass_OS::new_instance0()
  {
    return new Ass_OS
      (new Identifier(Identifier::ID_ASN, string("<error>")), 0,
       left->clone(), right->clone());
  }

  void Ass_OS::set_fullname(const string& p_fullname)
  {
    Assignment::set_fullname(p_fullname);
    left->set_fullname(p_fullname);
    right->set_fullname(p_fullname);
  }

  void Ass_OS::set_my_scope(Scope *p_scope)
  {
    Assignment::set_my_scope(p_scope);
    left->set_my_scope(p_scope);
    right->set_my_scope(p_scope);
  }

  void Ass_OS::set_right_scope(Scope *p_scope)
  {
    right->set_my_scope(p_scope);
  }

  /*
    ObjectClass* Ass_OS::get_ObjectClass()
    {
    return left;
    }
  */

  ObjectSet* Ass_OS::get_ObjectSet()
  {
    if(ass_pard) {
      error("`%s' is a parameterized objectset assignment",
            get_fullname().c_str());
      return 0;
    }
    chk();
    return right;
  }

  void Ass_OS::chk()
  {
    if(checked) return;
    Error_Context cntxt(this, "In information object set assignment `%s'",
                        id->get_dispname().c_str());
    if(ass_pard) {
      ass_pard->get_nof_pars();
      checked=true;
      return;
    }
    left->chk();
    right->set_my_governor(left);
    right->set_genname(get_genname());
    right->chk();
    checked=true;
  }

  void Ass_OS::generate_code(output_struct *target, bool)
  {
    if (ass_pard || dontgen) return;
    left->generate_code(target);
    right->generate_code(target);
  }

  void Ass_OS::generate_code(CodeGenHelper& cgh) {
    generate_code(cgh.get_current_outputstruct());
  }

  void Ass_OS::dump(unsigned level) const
  {
    DEBUG(level, "ObjectSet assignment: %s%s",
          id->get_dispname().c_str(), ass_pard?"{}":"");
    level++;
    left->dump(level);
    if(!ass_pard)
      right->dump(level);
  }

  // =================================
  // ===== Reference
  // =================================

  // =================================
  // ===== Ref_defd
  // =================================

  const Identifier* Ref_defd::get_modid()
  {
    Ref_defd_simple *t_ref = get_ref_defd_simple();
    if (t_ref) return t_ref->get_modid();
    else return 0;
  }

  const Identifier* Ref_defd::get_id()
  {
    Ref_defd_simple *t_ref = get_ref_defd_simple();
    if (t_ref) return t_ref->get_id();
    else return 0;
  }

  bool Ref_defd::refers_to_st(Setting::settingtype_t p_st,
                              ReferenceChain* refch)
  {
    if(get_is_erroneous()) return p_st==Setting::S_ERROR;
    bool b=false;
    Error_Context cntxt(this, "In reference `%s'", get_dispname().c_str());
    if(!my_scope)
      FATAL_ERROR("NULL parameter");
    Common::Assignment* c_ass=my_scope->get_ass_bySRef(this);
    if(!c_ass) {
      is_erroneous=true;
      return false;
    }
    Assignment* ass=dynamic_cast<Assignment*>(c_ass);
    if(!ass) {
      error("Reference to a non-ASN setting");
      is_erroneous=true;
      return false;
    }
    switch(p_st) {
    case Setting::S_OC:
      b=ass->is_asstype(Assignment::A_OC, refch);
      break;
    case Setting::S_T:
      b=ass->is_asstype(Assignment::A_TYPE, refch);
      break;
    case Setting::S_O:
      b=ass->is_asstype(Assignment::A_OBJECT, refch);
      break;
    case Setting::S_V:
      b=ass->is_asstype(Assignment::A_CONST, refch);
      break;
    case Setting::S_OS:
      b=ass->is_asstype(Assignment::A_OS, refch);
      break;
    case Setting::S_VS:
      b=ass->is_asstype(Assignment::A_VS, refch);
      break;
    case Setting::S_ERROR:
      b=ass->is_asstype(Assignment::A_ERROR, refch);
      break;
    default:
      FATAL_ERROR("Asn::Ref_defd::refers_to_st()");
    } // switch
    return b;
  }

  void Ref_defd::generate_code(expression_struct_t *)
  {
    FATAL_ERROR("Ref_defd::generate_code()");
  }

  void Ref_defd::generate_code_const_ref(expression_struct */*expr*/)
  {
    FATAL_ERROR("Ref_defd::generate_code_const_ref()");
  }

  // =================================
  // ===== Ref_defd_simple
  // =================================

  Ref_defd_simple::Ref_defd_simple(Identifier *p_modid,
                                   Identifier *p_id)
    : Ref_defd(), modid(p_modid), id(p_id)
  {
    if(!p_id)
      FATAL_ERROR("NULL parameter: Asn::Ref_defd_simple::Ref_defd_simple()");
  }

  Ref_defd_simple::Ref_defd_simple(const Ref_defd_simple& p)
    : Ref_defd(p)
  {
    modid=p.modid?p.modid->clone():0;
    id=p.id->clone();
  }

  Ref_defd_simple::~Ref_defd_simple()
  {
    delete modid;
    delete id;
  }

  Assignment* Ref_defd_simple::get_refd_ass()
  {
    if(get_is_erroneous()) return 0;
    Error_Context cntxt(this, "In reference `%s'", get_dispname().c_str());
    if(!my_scope)
      FATAL_ERROR("NULL parameter: Asn::Ref_defd_simple::get_refd_ass():"
		  " my_scope is not set");
    Common::Assignment* c_ass=my_scope->get_ass_bySRef(this);
    if(!c_ass) return 0;
    Assignment* ass=dynamic_cast<Assignment*>(c_ass);
    if(!ass)
      this->error("Reference to a non-ASN assignment");
    return ass;
  }

} // namespace Asn
