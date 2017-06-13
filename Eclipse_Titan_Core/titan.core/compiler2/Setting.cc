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
 *   Beres, Szabolcs
 *   Bibo, Zoltan
 *   Cserveni, Akos
 *   Delic, Adam
 *   Forstner, Matyas
 *   Gecse, Roland
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Tatarka, Gabor
 *
 ******************************************************************************/
#include "../common/dbgnew.hh"
#include "Setting.hh"
#include <stdio.h>
#include <stdarg.h>
#include "map.hh"
#include "Identifier.hh"
#include "CompilerError.hh"
#include "AST.hh"
#include "asn1/AST_asn1.hh"
#include "ttcn3/AST_ttcn3.hh"
#include "Value.hh"
#include "Int.hh"
#include "main.hh"
#include "ttcn3/profiler.h"
#include "ttcn3/Attributes.hh"

namespace Common {

  // =================================
  // ===== Location
  // =================================

  map<string,void> *Location::source_file_names = NULL;
  bool Location::transparency = false;

  const char* Location::add_source_file_name(const string& file_name)
  {
    if (source_file_names==NULL)
      source_file_names = new map<string,void>();
    if (!source_file_names->has_key(file_name))
      source_file_names->add(file_name, NULL);
    return source_file_names->get_key(file_name).c_str();
  }

  void Location::delete_source_file_names()
  {
    if (source_file_names!=NULL)
    {
      source_file_names->clear();
      delete source_file_names;
      source_file_names = NULL;
    }
  }

  Location::Location()
  {
    filename = NULL;
    yyloc.first_line = 0;
    yyloc.last_line = 0;
    yyloc.first_column = 0;
    yyloc.last_column = 0;
  }

  void Location::set_location(const char *p_filename, int p_lineno)
  {
    filename = p_filename;
    yyloc.first_line = p_lineno;
    yyloc.first_column = 0;
    yyloc.last_line = p_lineno;
    yyloc.last_column = 0;
  }

  void Location::set_location(const char *p_filename, const YYLTYPE& p_yyloc)
  {
    filename = p_filename;
    yyloc = p_yyloc;
  }

  void Location::set_location(const char *p_filename, const YYLTYPE& p_firstloc,
                                                      const YYLTYPE& p_lastloc)
  {
    filename = p_filename;
    yyloc.first_line = p_firstloc.first_line;
    yyloc.first_column = p_firstloc.first_column;
    yyloc.last_line = p_lastloc.last_line;
    yyloc.last_column = p_lastloc.last_column;
  }

  void Location::set_location(const char *p_filename, int p_first_line,
    int p_first_column, int p_last_line, int p_last_column)
  {
    filename = p_filename;
    yyloc.first_line = p_first_line;
    yyloc.first_column = p_first_column;
    yyloc.last_line = p_last_line;
    yyloc.last_column = p_last_column;
  }

  void Location::join_location(const Location& p)
  {
    // do nothing if this and p refer to different files
    if (filename) {
      if (!p.filename) return;
      else if (strcmp(filename, p.filename)) return;
    } else if (p.filename) return;
    if (yyloc.last_line < p.yyloc.first_line ||
        (yyloc.last_line == p.yyloc.first_line &&
	yyloc.last_column <= p.yyloc.first_column)) {
      // p is after this
      yyloc.last_line = p.yyloc.last_line;
      yyloc.last_column = p.yyloc.last_column;
    } else if (yyloc.first_line > p.yyloc.last_line ||
	       (yyloc.first_line == p.yyloc.last_line &&
	       yyloc.first_column >= p.yyloc.last_column)) {
      // p is before this
      yyloc.first_line = p.yyloc.first_line;
      yyloc.first_column = p.yyloc.first_column;
    }
  }

  void Location::print_line_info(FILE *fp) const
  {
    if (yyloc.first_line > 0) {
      // at least partial line/column information is available
      if (output_only_linenum || (yyloc.first_line == yyloc.last_line &&
          yyloc.first_column <= 0 && yyloc.last_column <= 0)) {
	// print only the first line
	fprintf(fp, "%d", yyloc.first_line);
      } else if (yyloc.last_line > yyloc.first_line) {
	// multi-line area
	if (yyloc.first_column >= 0 && yyloc.last_column >= 0) {
	  // all line/column fields are valid
	  if (gcc_compat) {
	    fprintf(fp, "%d:%d", yyloc.first_line, yyloc.first_column + 1);
	  }
	  else {
	    fprintf(fp, "%d.%d-%d.%d", yyloc.first_line,
	      yyloc.first_column + 1, yyloc.last_line, yyloc.last_column);
	  }
	} else {
	  // only the line numbers are valid
	  if (gcc_compat) {
	    fprintf(fp, "%d", yyloc.first_line);
	  }
	  else {
	    fprintf(fp, "%d-%d", yyloc.first_line, yyloc.last_line);
	  }
	}
      } else if (yyloc.first_line == yyloc.last_line) {
	// single line area
	if (yyloc.first_column >= 0 && yyloc.last_column > yyloc.first_column) {
	  if (gcc_compat) {
	    fprintf(fp, "%d:%d", yyloc.first_line, yyloc.first_column + 1);
	  }
	  else {
	    if (yyloc.last_column > yyloc.first_column + 1) {
	      // more characters are covered
	      fprintf(fp, "%d.%d-%d", yyloc.first_line, yyloc.first_column + 1,
	        yyloc.last_column);
	    } else {
	      // only a single character is covered
	      fprintf(fp, "%d.%d", yyloc.first_line, yyloc.first_column + 1);
	    }
	  }
	} else {
	  // the column information is invalid, print the line number only
	  fprintf(fp, "%d", yyloc.first_line);
	}
      } else {
	// the last line is smaller than the first line
	// print only the first line
	fprintf(fp, "%d", yyloc.first_line);
      }
    } else {
      // line information is not available
      fputs("<unknown>", fp);
    }
  }

  void Location::print_location(FILE *fp) const
  {
    if (filename) {
      fputs(filename, fp);
      if (yyloc.first_line > 0) {
	// print the line information only if it is available
	putc(':', fp);
	print_line_info(fp);
      }
      fputs(": ", fp);
    }
    // do not print anything if the file name is unknown
  }

  void Location::error(const char *fmt, ...) const
  {
    va_list args;
    va_start(args, fmt);
    Error_Context::report_error(this, fmt, args);
    va_end(args);
  }

  void Location::warning(const char *fmt, ...) const
  {
    va_list args;
    va_start(args, fmt);
    Error_Context::report_warning(this, fmt, args);
    va_end(args);
  }

  void Location::note(const char *fmt, ...) const
  {
    va_list args;
    va_start(args, fmt);
    Error_Context::report_note(this, fmt, args);
    va_end(args);
  }

  char *Location::create_location_object(char *str, const char *entitytype,
    const char *entityname) const
  {
    if (!filename || yyloc.first_line <= 0)
      FATAL_ERROR("Location::create_location_object()");
    if (include_location_info && !transparency) {
      bool tcov_enabled = tcov_file_name && in_tcov_files(get_filename());
      str = mputstr(str,
        !tcov_enabled ? "TTCN_Location current_location(\""
        		      : "TTCN_Location_Statistics current_location(\"");
      str = Code::translate_string(str, filename);
      str = mputprintf(str,
        !tcov_enabled ? "\", %d, TTCN_Location::LOCATION_%s, \"%s\");\n"
        		      : "\", %d, TTCN_Location_Statistics::LOCATION_%s, \"%s\");\n", yyloc.first_line, entitytype, entityname);
      if (tcov_enabled) {
          effective_module_lines =
            mputprintf(effective_module_lines, "%s%d",
            		   (effective_module_lines ? ", " : ""), yyloc.first_line);
          effective_module_functions =
            mputprintf(effective_module_functions, "%s\"%s\"",
            		   (effective_module_functions ? ", " : ""), entityname);
      }
      if (is_file_profiled(filename)) {
        // .ttcnpp -> .ttcn
        size_t file_name_len = strlen(filename);
        if ('p' == filename[file_name_len - 1] && 'p' == filename[file_name_len - 2]) {
          file_name_len -= 2;
        }
        char* file_name2 = mcopystrn(filename, file_name_len);
        str = mputprintf(str,
          "TTCN3_Stack_Depth stack_depth;\n"
          "ttcn3_prof.enter_function(\"%s\", %d);\n", file_name2, yyloc.first_line);
        insert_profiler_code_line(file_name2, 
          (0 == strcmp(entitytype, "CONTROLPART") ? "control" : entityname),
          yyloc.first_line);
        Free(file_name2);
      }
    }
    return str;
  }

  char *Location::update_location_object(char *str) const
  {
    if (filename && yyloc.first_line > 0) {
      if (include_location_info && !transparency) {
        str = mputprintf(str, "current_location.update_lineno(%d);\n",
                         yyloc.first_line);
        const char* file_name = get_filename();
        if (is_file_profiled(file_name)) {
          // .ttcnpp -> .ttcn
          size_t file_name_len = strlen(file_name);
          if ('p' == file_name[file_name_len - 1] && 'p' == file_name[file_name_len - 2]) {
            file_name_len -= 2;
          }
          char* file_name2 = mcopystrn(file_name, file_name_len);
          str = mputprintf(str, "ttcn3_prof.execute_line(\"%s\", %d);\n",
                  file_name2, yyloc.first_line);
          insert_profiler_code_line(file_name2, NULL, yyloc.first_line);
          Free(file_name2);
        }
        if (tcov_file_name && in_tcov_files(file_name)) {
            effective_module_lines =
              mputprintf(effective_module_lines, "%s%d",
              		   (effective_module_lines ? ", " : ""), yyloc.first_line);
        }
        if (debugger_active) {
          str = mputprintf(str, "ttcn3_debugger.breakpoint_entry(%d);\n", yyloc.first_line);
        }
      }

      if (include_line_info)
        str = mputprintf(str, "#line %d \"%s\"\n", yyloc.first_line, filename);
      else str = mputprintf(str, "/* %s, line %d */\n", filename,
                            yyloc.first_line);
    }
    return str;
  }

  // =================================
  // ===== Node
  // =================================

  int Node::counter=0;
#ifdef MEMORY_DEBUG
  static Node *list_head = 0, *list_tail = 0;
#endif

  Node::Node()
  {
#ifdef MEMORY_DEBUG
    prev_node = list_tail;
    next_node = 0;
    if (list_tail) list_tail->next_node = this;
    else list_head = this;
    list_tail = this;
#endif
    counter++;
  }

  Node::Node(const Node&)
    : fullname()
  {
#ifdef MEMORY_DEBUG
    prev_node = list_tail;
    next_node = 0;
    if (list_tail) list_tail->next_node = this;
    else list_head = this;
    list_tail = this;
#endif
    counter++;
  }

  Node::~Node()
  {
    counter--;
#ifdef MEMORY_DEBUG
    if (prev_node) prev_node->next_node = next_node;
    else list_head = next_node;
    if (next_node) next_node->prev_node = prev_node;
    else list_tail = prev_node;
#endif
  }


  void Node::chk_counter()
  {
    DEBUG(1, "Node::counter is %d", counter);
    if(counter)
      WARNING("%d nodes were not deleted."
              " Please send a bug report including"
              " the current input file(s).", counter);
#ifdef MEMORY_DEBUG
    for (Node *iter = list_head; iter; iter = iter->next_node) {
      fprintf(stderr, "Undeleted node: `%s' (address %p).\n",
	iter->get_fullname().c_str(), static_cast<void*>(iter));
    }
    list_head = 0;
    list_tail = 0;
#endif
  }

  void Node::set_fullname(const string& p_fullname)
  {
    fullname = p_fullname;
  }

  void Node::set_my_scope(Scope *)
  {
  }

  void Node::dump(unsigned level) const
  {
    DEBUG(level, "Node: %s", fullname.c_str());
  }

  // =================================
  // ===== Setting
  // =================================

  Setting::Setting(settingtype_t p_st)
    : Node(), Location(),
      st(p_st), my_scope(0), checked(false), recurs_checked(false)
  {
  }

  void Setting::set_my_scope(Scope *p_scope)
  {
    my_scope = p_scope;
  }

  bool Setting::is_asn1() const
  {
    if (!my_scope) FATAL_ERROR("Setting::is_asn1()");
    return my_scope->get_scope_mod()->get_moduletype() == Module::MOD_ASN;
  }

  string Setting::get_temporary_id() const
  {
    if (!my_scope) FATAL_ERROR("Setting::get_temporary_id()");
    return my_scope->get_scope_mod_gen()->get_temporary_id();
  }

  void Setting::set_genname(const string& p_genname)
  {
    if (p_genname.empty()) FATAL_ERROR("Setting::set_genname()");
    genname = p_genname;
  }

  void Setting::set_genname(const string& p_prefix, const string& p_suffix)
  {
    if (p_prefix.empty() || p_suffix.empty())
      FATAL_ERROR("Setting::set_genname()");
    genname = p_prefix;
    // a single underscore character is needed as separator if neither p_prefix
    // ends nor p_suffix begins with a single underscore character
    size_t p_prefix_len = p_prefix.size();
    if ((p_prefix[p_prefix_len - 1] != '_' ||
	(p_prefix_len >= 2 && p_prefix[p_prefix_len - 2] == '_')) &&
	(p_suffix[0] != '_' ||
	(p_suffix.size() >= 2 && p_suffix[1] == '_'))) genname += '_';
    genname += p_suffix;
  }

  const string& Setting::get_genname_own() const
  {
    if (genname.empty())
      FATAL_ERROR("Setting::get_genname_own(): genname is not set in %s", \
        get_fullname().c_str());
    return genname;
  }

  string Setting::get_genname_own(Scope *p_scope) const
  {
    if (!p_scope || !my_scope) FATAL_ERROR("Setting::get_genname_own");
    string ret_val;
    Module *my_mod = my_scope->get_scope_mod_gen();
    if (my_mod != p_scope->get_scope_mod_gen() &&
    !Asn::Assignments::is_spec_asss(my_mod)) {
    // when the definition is referred from another module
    // the reference shall be qualified with the namespace of my module
      ret_val = my_mod->get_modid().get_name();
      ret_val += "::";
    }
    ret_val += get_genname_own();
    return ret_val;
  }

  string Setting::create_stringRepr()
  {
    return string("<string representation not implemented for " +
      get_fullname() + ">");
  }

  // =================================
  // ===== Setting_Error
  // =================================

  Setting_Error* Setting_Error::clone() const
  {
    FATAL_ERROR("Setting_Error::clone");
  }

  // =================================
  // ===== Governor
  // =================================

  // =================================
  // ===== Governed
  // =================================

  // =================================
  // ===== GovernedSimple
  // =================================
  
  GovernedSimple::~GovernedSimple()
  {
    delete err_descrs;
  }
  
  void GovernedSimple::add_err_descr(Ttcn::Statement* p_update_statement,
                                     Ttcn::ErroneousDescriptor* p_err_descr)
  {
    if (p_err_descr != NULL) {
      if (err_descrs == NULL) {
        err_descrs = new Ttcn::ErroneousDescriptors;
      }
      err_descrs->add(p_update_statement, p_err_descr);
    }
  }

  string GovernedSimple::get_lhs_name() const
  {
    string ret_val;
    if (genname_prefix) ret_val += genname_prefix;
    ret_val += get_genname_own();
    return ret_val;
  }

  bool GovernedSimple::needs_init_precede(const GovernedSimple *refd) const
  {
    if (refd->code_generated) return false;
    if (code_section == CS_UNKNOWN || refd->code_section == CS_UNKNOWN)
      FATAL_ERROR("GovernedSimple::needs_init_precede()");
    if (code_section != refd->code_section) return false;
    if (get_my_scope()->get_scope_mod_gen() !=
      refd->get_my_scope()->get_scope_mod_gen()) return false;
    else return true;
  }

  bool GovernedSimple::is_toplevel() const
  {
    const string& name = get_genname_own();
    const char *name_str = name.c_str();
    size_t name_len = name.size();
    for (size_t i = 0; i < name_len; i++) {
      char c = name_str[i];
      if ((c < 'A' || c > 'Z') && (c < 'a' ||c > 'z') &&
	  (c < '0' || c > '9') && c != '_') return false;
    }
    return true;
  }

  // =================================
  // ===== GovdSet
  // =================================

  // =================================
  // ===== Scope
  // =================================

  string Scope::get_scope_name() const
  {
    string s;
    if (parent_scope) s = parent_scope->get_scope_name();
    if (!scope_name.empty()) {
      if (s.empty()) s = scope_name;
      else {
	s += '.';
	s += scope_name;
      }
    }
    return s;
  }

  string Scope::get_scopeMacro_name() const
  {
    if (!scopeMacro_name.empty()) return scopeMacro_name;
    if (parent_scope) return parent_scope->get_scopeMacro_name();
    return scopeMacro_name;
  }

  Ttcn::StatementBlock *Scope::get_statementblock_scope()
  {
    if (parent_scope) return parent_scope->get_statementblock_scope();
    else return 0;
  }

  Ttcn::RunsOnScope *Scope::get_scope_runs_on()
  {
    if (parent_scope) return parent_scope->get_scope_runs_on();
    else return 0;
  }
  
  Ttcn::PortScope *Scope::get_scope_port()
  {
    if (parent_scope) return parent_scope->get_scope_port();
    else return 0;
  }

  Assignments *Scope::get_scope_asss()
  {
    if (parent_scope) return parent_scope->get_scope_asss();
    else
      FATAL_ERROR("The assignments scope is not visible from this scope: " \
                  "`%s'", get_scope_name().c_str());
    return 0;
  }

  Module* Scope::get_scope_mod()
  {
    if(parent_scope) return parent_scope->get_scope_mod();
    else FATAL_ERROR("The module scope is not visible from this scope: `%s'", \
                     get_scope_name().c_str());
    return 0;
  }

  Module* Scope::get_scope_mod_gen()
  {
    if(parent_scope_gen) return parent_scope_gen->get_scope_mod_gen();
    else if(parent_scope) return parent_scope->get_scope_mod_gen();
    else FATAL_ERROR("The module scope is not visible from this scope: `%s'", \
                     get_scope_name().c_str());
    return 0;
  }

  bool Scope::has_ass_withId(const Identifier& p_id)
  {
    if (parent_scope) return parent_scope->has_ass_withId(p_id);
    else return false;
  }

  bool Scope::is_valid_moduleid(const Identifier& p_id)
  {
    if (parent_scope) return parent_scope->is_valid_moduleid(p_id);
    else return false;
  }

  Type *Scope::get_mtc_system_comptype(bool is_system)
  {
    if (parent_scope) return parent_scope->get_mtc_system_comptype(is_system);
    else return 0;
  }

  void Scope::chk_runs_on_clause(Assignment *p_ass, const Location& p_loc,
    const char *p_what)
  {
    // component type of the referred definition
    Type *refd_comptype = p_ass->get_RunsOnType();
    // definitions without 'runs on' can be called from anywhere
    if (!refd_comptype) return;
    Ttcn::RunsOnScope *t_ros = get_scope_runs_on();
    if (t_ros) {
      Type *local_comptype = t_ros->get_component_type();
      if (!refd_comptype->is_compatible(local_comptype, NULL, NULL)) {
	// the 'runs on' clause of the referred definition is not compatible
	// with that of the current scope (i.e. the referring definition)
	p_loc.error("Runs on clause mismatch: A definition that runs on "
          "component type `%s' cannot %s %s, which runs on `%s'",
          local_comptype->get_typename().c_str(), p_what,
          p_ass->get_description().c_str(),
          refd_comptype->get_typename().c_str());
      }
    } else {
      // the current scope unit (i.e. the referring definition) does not have
      // 'runs on' clause
      p_loc.error("A definition without `runs on' clause cannot %s %s, which "
        "runs on component type `%s'", p_what, p_ass->get_description().c_str(),
        refd_comptype->get_typename().c_str());
    }
  }

  void Scope::chk_runs_on_clause(Type *p_fat, const Location& p_loc,
    const char *p_what)
  {
    if (!p_fat) FATAL_ERROR("Scope::chk_runs_on_clause()");
    Type *refd_comptype = p_fat->get_fat_runs_on_type();
    // values of function/altstep types without 'runs on' clause
    // or using 'runs on self' clause can be called from anywhere
    if (!refd_comptype) return;
    const char *typetype_name;
    switch (p_fat->get_typetype()) {
    case Type::T_FUNCTION:
      typetype_name = "function";
      break;
    case Type::T_ALTSTEP:
      typetype_name = "altstep";
      break;
    default:
      FATAL_ERROR("Scope::chk_runs_on_clause()");
      typetype_name = 0;
    }
    Ttcn::RunsOnScope *t_ros = get_scope_runs_on();
    if (t_ros) {
      Type *local_comptype = t_ros->get_component_type();
      if (!refd_comptype->is_compatible(local_comptype, NULL, NULL)) {
	// the 'runs on' clause of the function/altstep type is not compatible
	// with that of the current scope (i.e. the referring definition)
	p_loc.error("Runs on clause mismatch: A definition that runs on "
	  "component type `%s' cannot %s a value of %s type `%s', which runs "
	  "on `%s'", local_comptype->get_typename().c_str(), p_what,
	  typetype_name, p_fat->get_typename().c_str(),
	  refd_comptype->get_typename().c_str());
      }
    } else {
      // the current scope unit (i.e. the referring definition) does not have
      // 'runs on' clause
      p_loc.error("A definition without `runs on' clause cannot %s a value of "
	"%s type `%s', which runs on component type `%s'", p_what,
	typetype_name, p_fat->get_typename().c_str(),
	refd_comptype->get_typename().c_str());
    }
  }

  // =================================
  // ===== Reference
  // =================================

  size_t Reference::_Reference_counter=0;
  Setting_Error *Reference::setting_error = 0;

  Reference::~Reference()
  {
    if (_Reference_counter <= 0) FATAL_ERROR("Reference::~Reference()");
    else if (--_Reference_counter == 0) {
      delete setting_error;
      setting_error = 0;
    }
  }

  void Reference::set_my_scope(Scope *p_scope)
  {
    my_scope = p_scope;
  }

  bool Reference::get_is_erroneous()
  {
    return is_erroneous;
  }

  Setting* Reference::get_refd_setting_error()
  {
    is_erroneous=true;
    if(!setting_error)
      setting_error=new Setting_Error();
    return setting_error;
  }

  bool Reference::refers_to_st(Setting::settingtype_t p_st,
                               ReferenceChain*)
  {
    Setting *t_setting=get_refd_setting();
    if(t_setting) return t_setting->get_st()==p_st;
    else return p_st==Setting::S_ERROR;
  }

  void Reference::set_code_section(GovernedSimple::code_section_t)
  {
  }

  Ttcn::FieldOrArrayRefs *Reference::get_subrefs()
  {
    return 0;
  }

  Ttcn::ActualParList *Reference::get_parlist()
  {
    return 0;
  }

  void Reference::dump(unsigned level) const
  {
    DEBUG(level, "Reference: %s", const_cast<Reference*>(this)->get_dispname().c_str());
  }

  // =================================
  // ===== Ref_simple
  // =================================

  string Ref_simple::get_dispname()
  {
    string ret_val;
    const Identifier *t_modid = get_modid();
    if (t_modid) {
      ret_val += t_modid->get_dispname();
      ret_val += '.';
    }
    ret_val += get_id()->get_dispname();
    return ret_val;
  }

  Setting* Ref_simple::get_refd_setting()
  {
    if(get_is_erroneous()) return get_refd_setting_error();
    Assignment *ass = get_refd_assignment();
    if (ass) return ass->get_Setting();
    else return get_refd_setting_error();
  }

  Assignment* Ref_simple::get_refd_assignment(bool)
  {
    if (!refd_ass) {
      if (!my_scope) FATAL_ERROR("Common::Ref_simple::get_refd_assignment()");
      refd_ass = my_scope->get_ass_bySRef(this);
    }
    return refd_ass;
  }

  bool Ref_simple::has_single_expr()
  {
    return true;
  }

  // =================================
  // ===== ReferenceChain
  // =================================

  ReferenceChain::ReferenceChain(const Location *p_loc, const char *p_str)
    : my_loc(p_loc), err_str(p_str), report_error(true)
  {
    if(!p_loc)
      FATAL_ERROR("ReferenceChain::ReferenceChain()");
  }

  ReferenceChain::~ReferenceChain()
  {
    reset();
  }

  ReferenceChain *ReferenceChain::clone() const
  {
    FATAL_ERROR("ReferenceChain::clone()");
    return 0;
  }

  bool ReferenceChain::exists(const string& s) const
  {
    for (size_t i = 0; i < refs.size(); i++)
      if (*refs[i]==s) return true;
    return false;
  }

  bool ReferenceChain::add(const string& s)
  {
    if (!exists(s)) {
      refs.add(new string(s));
      return true;
    }

    if (report_error) {
      if (err_str) {
        my_loc->error("%s: Circular reference: %s", err_str,
            get_dispstr(s).c_str());
      } else {
        my_loc->error("Circular reference: %s", get_dispstr(s).c_str());
      }
    } else {
      errors.add(get_dispstr(s));
    }
    return false;
  }

  void ReferenceChain::set_error_reporting(bool enable) {
    report_error = enable;
  }

  size_t ReferenceChain::nof_errors() const {
    return errors.size() - (err_stack.empty() ? 0 : *err_stack.top());
  }

  void ReferenceChain::report_errors()
  {
    if (!err_stack.empty() && *err_stack.top() > errors.size())
      FATAL_ERROR("Common::ReferenceChain::report_errors()");

    string err_msg;
    if (err_str) {
      err_msg += err_str;
      err_msg += ": ";
    }

    for (size_t i = (err_stack.empty() ? 0 : *err_stack.top());
                i < errors.size(); ++i) {
    my_loc->error("%sCircular reference: %s",
                  err_msg.c_str(), errors[i].c_str());
    }
  }

  void ReferenceChain::mark_error_state()
  {
    err_stack.push(new size_t(errors.size()));
  }

  void ReferenceChain::prev_error_state()
  {
    if (err_stack.empty())
      FATAL_ERROR("Common::ReferenceChain::prev_error_state()");
    if (errors.size() < *err_stack.top())
      FATAL_ERROR("Common::ReferenceChain::prev_error_state()");

    int state = static_cast<int>(*err_stack.top());
    for (int i = static_cast<int>(errors.size()) - 1; i >= state; --i) {
      errors.remove(i);
    }
    delete err_stack.pop();
  }

  void ReferenceChain::mark_state()
  {
    refstack.push(new size_t(refs.size()));
  }

  void ReferenceChain::prev_state()
  {
    if(refstack.empty())
      FATAL_ERROR("Common::ReferenceChain::prev_state()");
    size_t state=*refstack.top();
    if(refs.size()<state)
      FATAL_ERROR("Common::ReferenceChain::prev_state()");
    for(size_t i=state; i<refs.size(); i++) delete refs[i];
    refs.replace(state, refs.size()-state);
    delete refstack.pop();
  }

  void ReferenceChain::reset()
  {
    for(size_t i=0; i<refs.size(); i++) delete refs[i];
    refs.clear();
    while(!refstack.empty()) delete refstack.pop();

    errors.clear();
    while(!err_stack.empty()) delete err_stack.pop();
  }

  string ReferenceChain::get_dispstr(const string& s) const
  {
    size_t i = 0;
    // skip the elements before the first occurrence of s
    for ( ; i < refs.size(); i++) if (*refs[i] == s) break;
    string str;
    for( ; i < refs.size(); i++) {
      str += '`';
      str += *refs[i];
      str += "' -> ";
    }
    // putting s at the end
    str += '`';
    str += s;
    str += '\'';
    return str;
  }

} // namespace Common
