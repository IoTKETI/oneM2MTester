/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Baranyi, Botond
 *   Raduly, Csaba
 *   Szabo, Bence Janos
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#include "CodeGenHelper.hh"
#include "Code.hh"
#include "error.h"
#include "main.hh"
#include <cstdio>
#include <cstring>
#include <stdlib.h>

namespace Common {
    
CodeGenHelper* CodeGenHelper::instance = 0;

CodeGenHelper::generated_output_t::generated_output_t() :
  is_module(false),
  is_ttcn(true),
  has_circular_import(false)
{
  Code::init_output(&os);
}

CodeGenHelper::generated_output_t::~generated_output_t() {
  Code::free_output(&os);
}

// from Type.cc
const char* const CodeGenHelper::typetypemap[] = {
  "", /**< undefined */
  "", /**< erroneous (e.g. nonexistent reference) */
  "", /**< null (ASN.1) */
  "", /**< boolean */
  "", /**< integer */
  "", /**< integer / ASN */
  "", /**< real/float */
  "", /**< enumerated / ASN */
  "", /**< enumerated / TTCN */
  "", /**< bitstring */
  "", /**< bitstring */
  "", /**< hexstring (TTCN-3) */
  "", /**< octetstring */
  "", /**< charstring (TTCN-3) */
  "", /**< universal charstring (TTCN-3) */
  "", /**< UTF8String (ASN.1) */
  "", /**< NumericString (ASN.1) */
  "", /**< PrintableString (ASN.1) */
  "", /**< TeletexString (ASN.1) */
  "", /**< VideotexString (ASN.1) */
  "", /**< IA5String (ASN.1) */
  "", /**< GraphicString (ASN.1) */
  "", /**< VisibleString (ASN.1) */
  "", /**< GeneralString (ASN.1) */
  "",  /**< UniversalString (ASN.1) */
  "", /**< BMPString (ASN.1) */
  "", /**< UnrestrictedCharacterString (ASN.1) */
  "", /**< UTCTime (ASN.1) */
  "", /**< GeneralizedTime (ASN.1) */
  "", /** Object descriptor, a kind of string (ASN.1) */
  "", /**< object identifier */
  "", /**< relative OID (ASN.1) */
  "_union", /**< choice /ASN, uses u.secho */
  "_union", /**< union /TTCN, uses u.secho */
  "_seqof", /**< sequence (record) of */
  "_setof", /**< set of */
  "_seq", /**< sequence /ASN, uses u.secho */
  "_seq", /**< record /TTCN, uses u.secho */
  "_set", /**< set /ASN, uses u.secho */
  "_set", /**< set /TTCN, uses u.secho */
  "", /**< ObjectClassFieldType (ASN.1) */
  "", /**< open type (ASN.1) */
  "", /**< ANY (deprecated ASN.1) */
  "", /**< %EXTERNAL (ASN.1) */
  "", /**< EMBEDDED PDV (ASN.1) */
  "", /**< referenced */
  "", /**< special referenced (by pointer, not by name) */
  "", /**< selection type (ASN.1) */
  "", /**< verdict type (TTCN-3) */
  "", /**< port type (TTCN-3) */
  "", /**< component type (TTCN-3) */
  "", /**< address type (TTCN-3) */
  "", /**< default type (TTCN-3) */
  "", /**< array (TTCN-3), uses u.array */
  "", /**< signature (TTCN-3) */
  "", /**< function reference (TTCN-3) */
  "", /**< altstep reference (TTCN-3) */
  "", /**< testcase reference (TTCN-3) */
  "", /**< anytype (TTCN-3) */
  0
};

CodeGenHelper::CodeGenHelper() :
  split_mode(SPLIT_NONE),
  slice_num(1)
{
  if (instance != 0)
    FATAL_ERROR("Attempted to create a second code generator.");
  instance = this;
}

CodeGenHelper& CodeGenHelper::GetInstance() {
  if (instance == 0)
    FATAL_ERROR("Trying to access to the already destroyed code generator.");
  return *instance;
}

void CodeGenHelper::set_split_mode(split_type st) {
  split_mode = st;
  
  if (split_mode == SPLIT_TO_SLICES) {
    split_to_slices = true;
  } else {
    split_to_slices = false;
  }
}

bool CodeGenHelper::set_split_mode(const char* type) {
  int n;
  if (strcmp(type, "none") == 0) {
    split_mode = SPLIT_NONE;
    split_to_slices = false;
  } else if (strcmp(type, "type") == 0) {
    split_mode = SPLIT_BY_KIND;
    split_to_slices = false;
  } else if ((n = atoi(type))) {
    size_t length = strlen(type);
    for (size_t i=0;i<length; i++) {
      if (!isdigit(type[i])) {
        ERROR("The number argument of -U must be a valid number.");
        break;
      }
    }
    split_mode = SPLIT_TO_SLICES;
    if (n < 1 || n > 999999) {
      ERROR("The number argument of -U must be between 1 and 999999.");
      return false;
    }
    slice_num = n;
    split_to_slices = slice_num > 1; // slice_num == 1 has no effect
  } else
    return false;
  return true;
}

CodeGenHelper::split_type CodeGenHelper::get_split_mode() const {
  return split_mode;
}

void CodeGenHelper::add_module(const string& name, const string& dispname,
  bool is_ttcn, bool has_circular_import) {
  generated_output_t* go = new generated_output_t;
  go->filename.clear();
  go->modulename = name;
  go->module_dispname = dispname;
  go->is_module = true;
  go->is_ttcn = is_ttcn;
  go->has_circular_import = has_circular_import;
  generated_code.add(dispname, go);
  module_names_t* mod_names = new module_names_t;
  mod_names->name = name;
  mod_names->dispname = dispname;
  modules.add(mod_names);
}

output_struct* CodeGenHelper::get_outputstruct(const string& name) {
  return &generated_code[name]->os;
}

void CodeGenHelper::set_current_module(const string& name) {
  current_module = name;
}

void CodeGenHelper::update_intervals(output_struct* const output) {
  if(instance->split_mode != SPLIT_TO_SLICES) return;
  
  size_t tmp;
  
  // 1. check if some characters are added to the charstring
  // 2. increment size variable
  // 3. if size is bigger than the array's size, then double the array size
  // 4. store new end position
  
  // class_defs are not counted as they will be in the header
  tmp = mstrlen(output->source.function_bodies);
  if (output->intervals.function_bodies[output->intervals.function_bodies_size] < tmp) {
    output->intervals.function_bodies_size++;
    if (output->intervals.function_bodies_size >= output->intervals.function_bodies_max_size) {
      output->intervals.function_bodies_max_size *= 2;
      output->intervals.function_bodies = static_cast<size_t*>(
        Realloc(output->intervals.function_bodies, output->intervals.function_bodies_max_size * sizeof(size_t)) );
    }
    output->intervals.function_bodies[output->intervals.function_bodies_size] = tmp;
  }
  tmp = mstrlen(output->source.methods);
  if (output->intervals.methods[output->intervals.methods_size] < tmp) {
    output->intervals.methods_size++;
    if (output->intervals.methods_size >= output->intervals.methods_max_size) {
      output->intervals.methods_max_size *= 2;
      output->intervals.methods = static_cast<size_t*>(
        Realloc(output->intervals.methods, output->intervals.methods_max_size * sizeof(size_t)));
    }
    output->intervals.methods[output->intervals.methods_size] = tmp;
  }
  tmp = mstrlen(output->source.static_conversion_function_bodies);
  if (output->intervals.static_conversion_function_bodies[output->intervals.static_conversion_function_bodies_size] < tmp) {
    output->intervals.static_conversion_function_bodies_size++;
    if (output->intervals.static_conversion_function_bodies_size >= output->intervals.static_conversion_function_bodies_max_size) {
      output->intervals.static_conversion_function_bodies_max_size *= 2;
      output->intervals.static_conversion_function_bodies = static_cast<size_t*>(
        Realloc(output->intervals.static_conversion_function_bodies, output->intervals.static_conversion_function_bodies_max_size * sizeof(size_t)) );
    }
    output->intervals.static_conversion_function_bodies[output->intervals.static_conversion_function_bodies_size] = tmp;
  }
  tmp = mstrlen(output->source.static_function_bodies);
  if (output->intervals.static_function_bodies[output->intervals.static_function_bodies_size] < tmp) {
    output->intervals.static_function_bodies_size++;
    if (output->intervals.static_function_bodies_size >= output->intervals.static_function_bodies_max_size) {
      output->intervals.static_function_bodies_max_size *= 2;
      output->intervals.static_function_bodies = static_cast<size_t*>(
        Realloc(output->intervals.static_function_bodies, output->intervals.static_function_bodies_max_size * sizeof(size_t)) );
    }
    output->intervals.static_function_bodies[output->intervals.static_function_bodies_size] = tmp;
  }
}
//Advised to call update_intervals before this
size_t CodeGenHelper::size_of_sources(output_struct * const output) {
  size_t size = 0;
  // Calculate global var and string literals size
  output->intervals.pre_things_size = mstrlen(output->source.global_vars) + mstrlen(output->source.string_literals);
  
  // Class_defs, static_conversion_function_prototypes, static_function_prototypes are in the header,
  // and includes are not counted
  size = output->intervals.pre_things_size +
         output->intervals.function_bodies[output->intervals.function_bodies_size] +
         output->intervals.methods[output->intervals.methods_size] +
         output->intervals.static_conversion_function_bodies[output->intervals.static_conversion_function_bodies_size] +
         output->intervals.static_function_bodies[output->intervals.static_function_bodies_size];
  return size;
}

size_t CodeGenHelper::get_next_chunk_pos(const output_struct * const from, output_struct * const to, const size_t base_pos, const size_t chunk_size) {
  size_t pos = 0; // Holds the position from the beginning
  
  pos += from->intervals.pre_things_size;

  if (pos > base_pos) {
    to->source.global_vars = mputstr(to->source.global_vars, from->source.global_vars);
    to->source.string_literals = mputstr(to->source.string_literals, from->source.string_literals);
  }
  
  get_chunk_from_poslist(from->source.methods, to->source.methods, from->intervals.methods, from->intervals.methods_size, base_pos, chunk_size, pos);
  get_chunk_from_poslist(from->source.function_bodies, to->source.function_bodies, from->intervals.function_bodies, from->intervals.function_bodies_size, base_pos, chunk_size, pos);
  get_chunk_from_poslist(from->source.static_function_bodies, to->source.static_function_bodies, from->intervals.static_function_bodies, from->intervals.static_function_bodies_size, base_pos, chunk_size, pos);
  get_chunk_from_poslist(from->source.static_conversion_function_bodies, to->source.static_conversion_function_bodies, from->intervals.static_conversion_function_bodies, from->intervals.static_conversion_function_bodies_size, base_pos, chunk_size, pos);

  return pos;
}
//if from null return.
void CodeGenHelper::get_chunk_from_poslist(const char* from, char *& to, const size_t interval_array[], const size_t interval_array_size, const size_t base_pos, const size_t chunk_size, size_t& pos) {
  if (from == NULL) return;
  // If we have enough to form a chunk
  
  // pos is unsigned so it can't be negative
  if (pos > base_pos && pos - base_pos >= chunk_size) return;
  
  size_t tmp = pos;
  
  pos += interval_array[interval_array_size];

  if (pos > base_pos) { // If we haven't finished with this interval_array
    if (pos - base_pos >= chunk_size) { // It is a good chunk, but make it more precise because it may be too big
      int ind = 0;
      for (size_t i = 0; i <= interval_array_size; i++) {
        if (tmp + interval_array[i] <= base_pos) { // Find the pos where we left off
          ind = i;
        } else if (tmp + interval_array[i] - base_pos >= chunk_size) {
          // Found the first optimal position that is a good chunk
          to = mputstrn(to, from + interval_array[ind], interval_array[i] - interval_array[ind]);
          pos = tmp + interval_array[i];
          return;
        }
      }
    } else { // We can't form a new chunk from the remaining characters
      int ind = 0;
      for (size_t i = 0; i <= interval_array_size; i++) {
        if (tmp + interval_array[i] <= base_pos) {
          ind = i;
        } else {
          break;
        }
      }
      // Put the remaining characters
      to = mputstrn(to, from + interval_array[ind], interval_array[interval_array_size] - interval_array[ind]);
      pos = tmp + interval_array[interval_array_size]; 
    }
  }
}

output_struct* CodeGenHelper::get_outputstruct(Ttcn::Definition* def) {
  string key = get_key(*def);
  const string& new_name = current_module + key;
  if (!generated_code.has_key(new_name)) {
    generated_output_t* go = new generated_output_t;
    go->filename = key;
    go->modulename = generated_code[current_module]->modulename;
    go->module_dispname = generated_code[current_module]->module_dispname;
    generated_code.add(new_name, go);
    go->os.source.includes = mprintf("\n#include \"%s.hh\"\n"
      , current_module.c_str());
  }
  return &generated_code[new_name]->os;
}

output_struct* CodeGenHelper::get_outputstruct(Type* type) {
  string key = get_key(*type);
  const string& new_name = current_module + key;
  if (!generated_code.has_key(new_name)) {
    generated_output_t* go = new generated_output_t;
    go->filename = key;
    go->modulename = generated_code[current_module]->modulename;
    go->module_dispname = generated_code[current_module]->module_dispname;
    generated_code.add(new_name, go);
    go->os.source.includes = mprintf("\n#include \"%s.hh\"\n"
      , current_module.c_str());
  }
  return &generated_code[new_name]->os;
}

output_struct* CodeGenHelper::get_current_outputstruct() {
  return &generated_code[current_module]->os;
}

void CodeGenHelper::transfer_value(char* &dst, char* &src) {
  dst = mputstr(dst, src);
  Free(src);
  src = 0;
}

void CodeGenHelper::finalize_generation(Type* type) {
  string key = get_key(*type);
  if (key.empty()) return;

  output_struct& dst = *get_current_outputstruct();
  output_struct& src = *get_outputstruct(current_module + key);
  // key is not empty so these can never be the same

  transfer_value(dst.header.includes,     src.header.includes);
  transfer_value(dst.header.class_decls,  src.header.class_decls);
  transfer_value(dst.header.typedefs,     src.header.typedefs);
  transfer_value(dst.header.class_defs,   src.header.class_defs);
  transfer_value(dst.header.function_prototypes, src.header.function_prototypes);
  transfer_value(dst.header.global_vars,  src.header.global_vars);
  transfer_value(dst.header.testport_includes, src.header.testport_includes);

  transfer_value(dst.source.global_vars,  src.source.global_vars);

  transfer_value(dst.functions.pre_init,  src.functions.pre_init);
  transfer_value(dst.functions.post_init, src.functions.post_init);

  transfer_value(dst.functions.set_param, src.functions.set_param);
  transfer_value(dst.functions.get_param, src.functions.get_param);
  transfer_value(dst.functions.log_param, src.functions.log_param);
  transfer_value(dst.functions.init_comp, src.functions.init_comp);
  transfer_value(dst.functions.start,     src.functions.start);
  transfer_value(dst.functions.control,   src.functions.control);
}

string CodeGenHelper::get_key(Ttcn::Definition& def) const {
  string retval;
  switch (split_mode) {
  case SPLIT_NONE:
    // returns the current module
    break;
  case SPLIT_BY_KIND:
    break;
  case SPLIT_BY_NAME:
    retval += "_" + def.get_id().get_name();
    break;
  case SPLIT_BY_HEURISTICS:
    break;
  case SPLIT_TO_SLICES:
    break;
  }
  return retval;
}

string CodeGenHelper::get_key(Type& type) const {
  string retval;
  switch (split_mode) {
  case SPLIT_NONE:
    break;
  case SPLIT_BY_KIND: {
    Type::typetype_t tt = type.get_typetype();
    switch(tt) {
    case Type::T_CHOICE_A:
    case Type::T_CHOICE_T:
    case Type::T_SEQOF:
    case Type::T_SETOF:
    case Type::T_SEQ_A:
    case Type::T_SEQ_T:
    case Type::T_SET_A:
    case Type::T_SET_T:
      retval += typetypemap[static_cast<int>(tt)];
      break;
    default:
      // put it into the module (no suffix)
      break;
    }
    break; }
  case SPLIT_BY_NAME:
    break;
  case SPLIT_BY_HEURISTICS:
    break;
  case SPLIT_TO_SLICES:
    break;
  }
  return retval;
}

void CodeGenHelper::write_output() {
  size_t i, j;
  if (split_mode == SPLIT_BY_KIND) {
    // Create empty files to have a fix set of files to compile
    string fname;
    for (j = 0; j < modules.size(); j++) {
      for (i = 0; typetypemap[i]; i++) {
        fname = modules[j]->dispname + typetypemap[i];
        if (!generated_code.has_key(fname)) {
          generated_output_t* go = new generated_output_t;
          go->filename = typetypemap[i];
          go->modulename = modules[j]->name;
          go->module_dispname = modules[j]->dispname;
          go->os.source.includes = mcopystr(
            "\n//This file is intentionally empty."
            "\n#include <version.h>\n");
          generated_code.add(fname, go);
        }
      }
    }
  } else if (split_mode == SPLIT_TO_SLICES && slice_num > 0) {
    // The strategy is the following:
    // Goal: Get files with equal size
    // Get the longest file's length and divide it by slice_num (chunk_size)
    // We split every file to chunk_size sized chunks
    size_t max = 0;
    // Calculate maximum character length
    for (j = 0; j < generated_code.size(); j++) {
      update_intervals(&generated_code.get_nth_elem(j)->os);
      size_t num_of_chars = size_of_sources(&generated_code.get_nth_elem(j)->os);
      if (max < num_of_chars) {
        max = num_of_chars;
      }
    }
    // Calculate ideal chunk size
    size_t chunk_size = max / slice_num;
    string fname;
    for (j = 0; j < modules.size(); j++) {
      generated_output_t *output = generated_code[modules[j]->dispname];

      // Just to be sure that everything is in the right place
      update_intervals(&output->os);
      
      // Move static function prototypes to header (no longer static)
      output->os.header.function_prototypes = mputstr(output->os.header.function_prototypes, output->os.source.static_function_prototypes);
      Free(output->os.source.static_function_prototypes);
      output->os.source.static_function_prototypes = NULL;
      
      output->os.header.function_prototypes = mputstr(output->os.header.function_prototypes, output->os.source.static_conversion_function_prototypes);
      Free(output->os.source.static_conversion_function_prototypes);
      output->os.source.static_conversion_function_prototypes = NULL;
      
      // Move internal class definitions to the header
      output->os.header.class_defs = mputstr(output->os.header.class_defs, output->os.source.class_defs);
      Free(output->os.source.class_defs);
      output->os.source.class_defs = NULL;
      
      update_intervals(&output->os);
      size_t num_of_chars = size_of_sources(&output->os);
      char buffer[13]=  ""; // Max is 999999 should be enough (checked in main.cc) | 6 digit + 2 underscore + part
      // If we need to split
      if (num_of_chars >= chunk_size) {
        size_t base_pos = 0;
        for (i = 0; i < slice_num; i++) {
          if (i == 0) { // The first slice has the module's name
            fname = output->module_dispname;
          } else {
            sprintf(buffer, "_part_%d", static_cast<int>(i));
            fname = output->module_dispname + "_" + buffer;
          }
          if (i == 0 || !generated_code.has_key(fname)) {
            generated_output_t* go = new generated_output_t;
            go->filename = buffer;
            go->modulename = output->modulename;
            go->module_dispname = output->module_dispname;
            size_t act_pos = get_next_chunk_pos(&output->os, &go->os, base_pos, chunk_size);
            // True if the file is not empty
            if (act_pos > base_pos) {
              go->os.source.includes = mputstr(go->os.source.includes, output->os.source.includes);
            } else {
              go->os.source.includes = mcopystr(
                "\n//This file is intentionally empty."
                "\n#include <version.h>\n");
            }
            // First slice: copy header and other properties and replace the original output struct
            if (i == 0) {
              go->has_circular_import = output->has_circular_import;
              go->is_module = output->is_module;
              go->is_ttcn = output->is_ttcn;
              go->os.header.class_decls = mputstr(go->os.header.class_decls, output->os.header.class_decls);
              go->os.header.class_defs = mputstr(go->os.header.class_defs, output->os.header.class_defs);
              go->os.header.function_prototypes = mputstr(go->os.header.function_prototypes, output->os.header.function_prototypes);
              go->os.header.global_vars = mputstr(go->os.header.global_vars, output->os.header.global_vars);
              go->os.header.includes = mputstr(go->os.header.includes, output->os.header.includes);
              go->os.header.testport_includes = mputstr(go->os.header.testport_includes, output->os.header.testport_includes);
              go->os.header.typedefs = mputstr(go->os.header.typedefs, output->os.header.typedefs);
              generated_code[modules[j]->dispname] = go;
            } else {
              generated_code.add(fname, go);
            }
            base_pos = act_pos;
          } else {
            // TODO: error handling: there is a module which has the same name as the
            // numbered splitted file. splitting by type does not have this error
            // handling so don't we
          }
        }
        // Extra safety. If something is missing after the splitting, put the remaining 
        // things to the last file. (Should never happen)
        if (base_pos < num_of_chars) {
          get_next_chunk_pos(&output->os, &generated_code[fname]->os, base_pos, num_of_chars);
        }
        delete output;
      } else {
        // Create empty files.
        for (i = 1; i < slice_num; i++) {
          sprintf(buffer, "_part_%d", static_cast<int>(i));
          fname = output->module_dispname + "_" + buffer;
          if (!generated_code.has_key(fname)) {
            generated_output_t* go = new generated_output_t;
            go->filename = buffer;
            go->modulename = modules[j]->name;
            go->module_dispname = modules[j]->dispname;
            go->os.source.includes = mcopystr(
              "\n//This file is intentionally empty."
              "\n#include <version.h>\n");
            generated_code.add(fname, go);
          }
        }
      }
    }
  }
  generated_output_t* go;
  for (i = 0; i < generated_code.size(); i++) {
    go = generated_code.get_nth_elem(i);
    ::write_output(&go->os, go->modulename.c_str(), go->module_dispname.c_str(),
      go->filename.c_str(), go->is_ttcn, go->has_circular_import, go->is_module);
  }
}

CodeGenHelper::~CodeGenHelper() {
  size_t i;
  for (i = 0; i < generated_code.size(); i++)
    delete generated_code.get_nth_elem(i);
  generated_code.clear();
  for (i = 0; i < modules.size(); i++)
    delete modules[i];
  modules.clear();
  instance = 0;
}

}
