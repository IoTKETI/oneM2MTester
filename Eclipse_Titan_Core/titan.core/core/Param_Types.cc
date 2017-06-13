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
 *   Beres, Szabolcs
 *   Delic, Adam
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
#include <string.h>

#include "../common/memory.h"
#include "Types.h"
#include "RInt.hh"
#include "Error.hh"

// Needed for logging
#include "openssl/bn.h"
#include "Integer.hh"
#include "Float.hh"
#include "Boolean.hh"
#include "Objid.hh"
#include "Verdicttype.hh"
#include "Bitstring.hh"
#include "Hexstring.hh"
#include "Octetstring.hh"
#include "Charstring.hh"
#include "Universal_charstring.hh"
#include "Logger.hh"
#include "Module_list.hh"
#include "Debugger.hh"
#include "DebugCommands.hh"


size_t Module_Param_Id::get_index() const {
  TTCN_error("Internal error: Module_Param_Id::get_index()");
  return 0;
}

char* Module_Param_Id::get_name() const {
  TTCN_error("Internal error: Module_Param_Id::get_name()");
  return NULL;
}

char* Module_Param_Id::get_current_name() const {
  TTCN_error("Internal error: Module_Param_Id::get_current_name()");
  return NULL;
}

boolean Module_Param_Id::next_name() {
  TTCN_error("Internal error: Module_Param_Id::next_name()");
  return FALSE;
}

void Module_Param_Id::reset() {
  TTCN_error("Internal error: Module_Param_Id::reset()");
}

size_t Module_Param_Id::get_nof_names() const {
  TTCN_error("Internal error: Module_Param_Id::get_nof_names()");
  return 0;
}

char* Module_Param_Name::get_str() const {
  char* result = NULL;
  for (size_t i = 0; i < names.size(); ++i) {
    boolean index = names[i][0] >= '0' && names[i][0] <= '9';
    if (i > 0 && !index) {
      result = mputc(result, '.');
    }
    if (index) {
      result = mputc(result, '[');
    }
    result = mputstr(result, names[i]);
    if (index) {
      result = mputc(result, ']');
    }
  }
  return result;
}

char* Module_Param_FieldName::get_str() const {
  return mcopystr(name);
}

char* Module_Param_Index::get_str() const {
  return mprintf("[%lu]", (unsigned long)index);
}

char* Module_Param_CustomName::get_str() const {
  return mcopystr(name);
}

void Module_Param_Length_Restriction::log() const {
  TTCN_Logger::log_event(" length(%lu", (unsigned long)min);
  if (min!=max) {
    TTCN_Logger::log_event_str("..");
    if (!has_max) TTCN_Logger::log_event_str("infinity");
    else TTCN_Logger::log_event("%lu", (unsigned long)max);
  }
  TTCN_Logger::log_event_str(")");
}

Module_Param_Id* Module_Param::get_id() const {
  return id;
}

const char* Module_Param::get_operation_type_str() const {
  switch (operation_type) {
  case OT_ASSIGN: return "assignment";
  case OT_CONCAT: return "concatenation";
  default: return "<unknown operation>";
  }
}

const char* Module_Param::get_operation_type_sign_str() const {
  switch (operation_type) {
  case OT_ASSIGN: return ":=";
  case OT_CONCAT: return "&=";
  default: return "<unknown operation>";
  }
}

void Module_Param::set_id(Module_Param_Id* p_id) {
  if (id) TTCN_error("Internal error: Module_Param::set_id()");
  id = p_id;
}

void Module_Param::set_length_restriction(Module_Param_Length_Restriction* p_length_restriction) {
  if (length_restriction!=NULL) TTCN_error("Internal error: Module_Param::set_length_restriction()");
  length_restriction = p_length_restriction;
}

void Module_Param::log(boolean log_id) const {
  if (log_id && id && id->is_explicit()) {
    char* id_str = id->get_str();
    TTCN_Logger::log_event_str(id_str);
    Free(id_str);
    TTCN_Logger::log_event_str(get_operation_type_sign_str());
  }
  log_value();
  if (has_ifpresent) {
    TTCN_Logger::log_event_str(" ifpresent");
  }
  if (length_restriction!=NULL) {
    length_restriction->log();
  }
}

void Module_Param::basic_check(int check_bits, const char* what) const {
  boolean is_template = check_bits & BC_TEMPLATE;
  boolean is_list = check_bits & BC_LIST;
  if (is_template || !is_list) {
    if (get_operation_type()!=OT_ASSIGN) error("The %s of %ss is not allowed.", get_operation_type_str(), what);
  }
  if (!is_template) {
    if (has_ifpresent) error("%s cannot have an 'ifpresent' attribute", what);
  }
  if (!is_template || !is_list) {
    if (length_restriction!=NULL) error("%s cannot have a length restriction", what);
  }
}

void Module_Param::add_elem(Module_Param* /*value*/) {
  TTCN_error("Internal error: Module_Param::add_elem()");
}

void Module_Param::add_list_with_implicit_ids(Vector<Module_Param*>* /*mp_list*/) {
  TTCN_error("Internal error: Module_Param::add_list_with_implicit_ids()");
}

boolean Module_Param::get_boolean() const {
  TTCN_error("Internal error: Module_Param::get_boolean()");
  return FALSE;
}

size_t Module_Param::get_size() const {
  TTCN_error("Internal error: Module_Param::get_size()");
  return 0;
}

Module_Param* Module_Param::get_elem(size_t /*index*/) const {
  TTCN_error("Internal error: Module_Param::get_elem()");
  return NULL;
}

int Module_Param::get_string_size() const {
  TTCN_error("Internal error: Module_Param::get_string_size()");
  return 0;
}

void* Module_Param::get_string_data() const {
  TTCN_error("Internal error: Module_Param::get_string_data()");
  return NULL;
}

int_val_t* Module_Param::get_lower_int() const {
  TTCN_error("Internal error: Module_Param::get_lower_int()");
  return NULL;
}

int_val_t* Module_Param::get_upper_int() const {
  TTCN_error("Internal error: Module_Param::get_upper_int()");
  return NULL;
}

boolean Module_Param::get_is_min_exclusive() const {
  TTCN_error("Internal error: Module_Param::get_is_min_exclusive()");
  return FALSE;
}

boolean Module_Param::get_is_max_exclusive() const {
  TTCN_error("Internal error: Module_Param::get_is_max_exclusive()");
  return FALSE;
}

double Module_Param::get_lower_float() const {
  TTCN_error("Internal error: Module_Param::get_lower_float()");
  return 0.0;
}

double Module_Param::get_upper_float() const {
  TTCN_error("Internal error: Module_Param::get_upper_float()");
  return 0.0;
}

boolean Module_Param::has_lower_float() const {
  TTCN_error("Internal error: Module_Param::has_lower_float()");
  return FALSE;
}

boolean Module_Param::has_upper_float() const {
  TTCN_error("Internal error: Module_Param::has_upper_float()");
  return FALSE;
}

universal_char Module_Param::get_lower_uchar() const {
  TTCN_error("Internal error: Module_Param::get_lower_uchar()");
  return universal_char();
}

universal_char Module_Param::get_upper_uchar() const {
  TTCN_error("Internal error: Module_Param::get_upper_uchar()");
  return universal_char();
}

int_val_t* Module_Param::get_integer() const {
  TTCN_error("Internal error: Module_Param::get_integer()");
  return NULL;
}

double Module_Param::get_float() const {
  TTCN_error("Internal error: Module_Param::get_float()");
  return 0.0;
}

char* Module_Param::get_pattern() const {
  TTCN_error("Internal error: Module_Param::get_pattern()");
  return NULL;
}

boolean Module_Param::get_nocase() const {
  TTCN_error("Internal error: Module_Param::get_nocase()");
  return FALSE;
}

verdicttype Module_Param::get_verdict() const {
  TTCN_error("Internal error: Module_Param::get_verdict()");
  return NONE;
}

char* Module_Param::get_enumerated() const {
  TTCN_error("Internal error: Module_Param::get_enumerated()");
  return NULL;
}

#ifdef TITAN_RUNTIME_2
Module_Param_Ptr Module_Param::get_referenced_param() const {
  TTCN_error("Internal error: Module_Param::get_referenced_param()");
  return NULL;
}
#endif

Module_Param::expression_operand_t Module_Param::get_expr_type() const { 
  TTCN_error("Internal error: Module_Param::get_expr_type()");
  return EXPR_ERROR;
}

const char* Module_Param::get_expr_type_str() const {
  TTCN_error("Internal error: Module_Param::get_expr_type_str()");
  return NULL;
}

Module_Param* Module_Param::get_operand1() const {
  TTCN_error("Internal error: Module_Param::get_operand1()");
  return NULL;
}

Module_Param* Module_Param::get_operand2() const {
  TTCN_error("Internal error: Module_Param::get_operand2()");
  return NULL;
}

Module_Param_Ptr::Module_Param_Ptr(Module_Param* p) {
  ptr = new module_param_ptr_struct;
  ptr->mp_ptr = p;
  ptr->temporary = FALSE;
  ptr->ref_count = 1;
}


Module_Param_Ptr::Module_Param_Ptr(const Module_Param_Ptr& r)
: ptr(r.ptr) {
  ++ptr->ref_count;
}

void Module_Param_Ptr::clean_up() {
  if (ptr->ref_count == 1) {
    if (ptr->temporary) {
      delete ptr->mp_ptr;
    }
    delete ptr;
  }
  else {
    --ptr->ref_count;
  }
}

Module_Param_Ptr& Module_Param_Ptr::operator=(const Module_Param_Ptr& r) {
  clean_up();
  ptr = r.ptr;
  ++ptr->ref_count;
  return *this;
}

#ifdef TITAN_RUNTIME_2
Module_Param_Reference::Module_Param_Reference(Module_Param_Name* p): mp_ref(p) {
  if (mp_ref == NULL) {
    TTCN_error("Internal error: Module_Param_Reference::Module_Param_Reference()");
  }
}

Module_Param_Ptr Module_Param_Reference::get_referenced_param() const {
  if (Debugger_Value_Parsing::happening()) {
    error("References to other variables are not allowed.");
  }
  mp_ref->reset();
  Module_Param_Ptr ptr = Module_List::get_param(*mp_ref, this);
  ptr.set_temporary();
  return ptr;
}

char* Module_Param_Reference::get_enumerated() const {
  if (mp_ref->get_nof_names() == (size_t)1) {
    return mp_ref->get_current_name();
  }
  return NULL;
}

void Module_Param_Reference::log_value() const {
  TTCN_Logger::log_event_str(mp_ref->get_str());
}

void Module_Param_Unbound::log_value() const {
  TTCN_Logger::log_event_str("<unbound>");
}
#endif

Module_Param_Expression::Module_Param_Expression(expression_operand_t p_type,
  Module_Param* p_op1, Module_Param* p_op2)
: expr_type(p_type), operand1(p_op1), operand2(p_op2) {
  if (operand1 == NULL || operand2 == NULL) {
    TTCN_error("Internal error: Module_Param_Expression::Module_Param_Expression()");
  }
  operand1->set_parent(this);
  operand2->set_parent(this);
}

Module_Param_Expression::Module_Param_Expression(Module_Param* p_op)
: expr_type(EXPR_NEGATE), operand1(p_op), operand2(NULL) {
  if (operand1 == NULL) {
    TTCN_error("Internal error: Module_Param_Expression::Module_Param_Expression()");
  }
  operand1->set_parent(this);
}

Module_Param_Expression::~Module_Param_Expression() {
  delete operand1;
  if (operand2 != NULL) {
    delete operand2;
  }
}

const char* Module_Param_Expression::get_expr_type_str() const {
  switch (expr_type) {
  case EXPR_ADD:
    return "Adding (+)";
  case EXPR_SUBTRACT:
    return "Subtracting (-)";
  case EXPR_MULTIPLY:
    return "Multiplying (*)";
  case EXPR_DIVIDE:
    return "Dividing (/)";
  case EXPR_NEGATE:
    return "Negating (-)";
  case EXPR_CONCATENATE:
    return "Concatenating (&)";
  default:
    return NULL;
  }
}

void Module_Param_Expression::log_value() const {
  if (expr_type == EXPR_NEGATE) {
    TTCN_Logger::log_event_str("- ");
  }
  operand1->log_value();
  switch (expr_type) {
  case EXPR_ADD:
    TTCN_Logger::log_event_str(" + ");
    break;
  case EXPR_SUBTRACT:
    TTCN_Logger::log_event_str(" - ");
    break;
  case EXPR_MULTIPLY:
    TTCN_Logger::log_event_str(" * ");
    break;
  case EXPR_DIVIDE:
    TTCN_Logger::log_event_str(" / ");
    break;
  case EXPR_CONCATENATE:
    TTCN_Logger::log_event_str(" & ");
    break;
  default:
    break;
  }
  if (expr_type != EXPR_NEGATE) {
    operand2->log_value();
  }
}

void Module_Param_NotUsed::log_value() const {
  TTCN_Logger::log_event_str("-");
}

void Module_Param_Omit::log_value() const {
  TTCN_Logger::log_event_str("omit");
}

Module_Param_Integer::Module_Param_Integer(int_val_t* p): integer_value(p) {
  if (integer_value==NULL) TTCN_error("Internal error: Module_Param_Integer::Module_Param_Integer()");
}

void Module_Param_Integer::log_value() const {
  if (integer_value->is_native()) {
    INTEGER(integer_value->get_val()).log();
  } else {
    INTEGER integer;
    integer.set_val(*integer_value);
    integer.log();
  }
}

void Module_Param_Float::log_value() const {
  FLOAT(float_value).log();
}

void Module_Param_Boolean::log_value() const {
  BOOLEAN(boolean_value).log();
}

void Module_Param_Verdict::log_value() const {
  VERDICTTYPE(verdict_value).log();
}

void Module_Param_Objid::log_value() const {
  OBJID(n_chars, chars_ptr).log();
}

void Module_Param_Bitstring::log_value() const {
  BITSTRING(n_chars, chars_ptr).log();
}

void Module_Param_Hexstring::log_value() const {
  HEXSTRING(n_chars, chars_ptr).log();
}

void Module_Param_Octetstring::log_value() const {
  OCTETSTRING(n_chars, chars_ptr).log();
}

void Module_Param_Charstring::log_value() const {
  CHARSTRING(n_chars, chars_ptr).log();
}

void Module_Param_Universal_Charstring::log_value() const {
  UNIVERSAL_CHARSTRING(n_chars, chars_ptr).log();
}

void Module_Param_Enumerated::log_value() const {
  TTCN_Logger::log_event_str(enum_value);
}

void Module_Param_Ttcn_Null::log_value() const {
  TTCN_Logger::log_event_str("null");
}

void Module_Param_Ttcn_mtc::log_value() const {
  TTCN_Logger::log_event_str("mtc");
}

void Module_Param_Ttcn_system::log_value() const {
  TTCN_Logger::log_event_str("system");
}

void Module_Param_Asn_Null::log_value() const {
  TTCN_Logger::log_event_str("NULL");
}

void Module_Param_Any::log_value() const {
  TTCN_Logger::log_event_str("?");
}

void Module_Param_AnyOrNone::log_value() const {
  TTCN_Logger::log_event_str("*");
}

void Module_Param_IntRange::log_bound(int_val_t* bound, boolean is_lower) {
  if (bound==NULL) {
    if (is_lower) TTCN_Logger::log_event_str("-");
    TTCN_Logger::log_event_str("infinity");
  } else if (bound->is_native()) {
    INTEGER(bound->get_val()).log();
  } else {
    INTEGER integer;
    integer.set_val(*bound);
    integer.log();
  }
}

void Module_Param_IntRange::log_value() const {
  TTCN_Logger::log_event_str("(");
  log_bound(lower_bound, TRUE);
  TTCN_Logger::log_event_str("..");
  log_bound(upper_bound, FALSE);
  TTCN_Logger::log_event_str(")");
}

void Module_Param_FloatRange::log_value() const {
  TTCN_Logger::log_event_str("(");
  if (has_lower) FLOAT(lower_bound).log();
  else TTCN_Logger::log_event_str("-infinity");
  TTCN_Logger::log_event_str("..");
  if (has_upper) FLOAT(upper_bound).log();
  else TTCN_Logger::log_event_str("infinity");
  TTCN_Logger::log_event_str(")");
}

void Module_Param_StringRange::log_value() const {
  TTCN_Logger::log_event_str("(");
  UNIVERSAL_CHARSTRING(lower_bound).log();
  TTCN_Logger::log_event_str("..");
  UNIVERSAL_CHARSTRING(upper_bound).log();
  TTCN_Logger::log_event_str(")");
}

void Module_Param_Pattern::log_value() const {
  TTCN_Logger::log_event_str("pattern ");
  if (nocase) {
    TTCN_Logger::log_event_str("@nocase ");
  }
  TTCN_Logger::log_event_str("\"");
  TTCN_Logger::log_event_str(pattern);
  TTCN_Logger::log_event_str("\"");
}

void Module_Param_Bitstring_Template::log_value() const {
  BITSTRING_template((unsigned int)n_chars, chars_ptr).log();
}

void Module_Param_Hexstring_Template::log_value() const {
  HEXSTRING_template((unsigned int)n_chars, chars_ptr).log();
}

void Module_Param_Octetstring_Template::log_value() const {
  OCTETSTRING_template((unsigned int)n_chars, chars_ptr).log();
}

Module_Param_Compound::~Module_Param_Compound() {
  for (size_t i=0; i<values.size(); i++) {
    delete values[i];
  }
  values.clear();
}

size_t Module_Param_Compound::get_size() const {
  return values.size();
}

Module_Param* Module_Param_Compound::get_elem(size_t index) const {
  if (index>=values.size()) TTCN_error("Internal error: Module_Param::get_elem(): index overflow");
  return values[index];
}

void Module_Param_Compound::log_value_vec(const char* begin_str, const char* end_str) const {
  TTCN_Logger::log_event_str(begin_str);
  TTCN_Logger::log_event_str(" ");
  for (size_t i=0; i<values.size(); ++i) {
    if (i>0) TTCN_Logger::log_event_str(", ");
    values[i]->log();
  }
  if (!values.empty()) TTCN_Logger::log_event_str(" ");
  TTCN_Logger::log_event_str(end_str);
}

void Module_Param_Compound::add_elem(Module_Param* value) {
  value->set_parent(this);
  values.push_back(value);
}

void Module_Param_Compound::add_list_with_implicit_ids(Vector<Module_Param*>* mp_list) {
  for (size_t i=0; i<mp_list->size(); i++) {
    Module_Param* mp_current = mp_list->at(i);
    mp_current->set_id(new Module_Param_Index(get_size(),FALSE));
    add_elem(mp_current);
  }
}

char* Module_Param::get_param_context() const {
  char* result = NULL;
  if (parent != NULL) {
    result = parent->get_param_context();
  }
  if (id) {
    char* id_str = id->get_str();
    if (parent!=NULL && !id->is_index()) {
      result = mputc(result, '.');
    }
    result = mputstr(result, id_str);
    Free(id_str);
  }
  return result;
}

void Module_Param::error(const char* err_msg, ...) const {
  if (Ttcn_String_Parsing::happening()) {
    char* exception_str = mcopystr("Error while setting ");
    char* param_ctx;
    if (id && id->is_custom()) {
      param_ctx = mputstr(id->get_str(), " in module parameter");
    }
    else {
      char* tmp = get_param_context();
      param_ctx = mprintf("parameter field '%s'", tmp ? tmp : "<NULL pointer>");
      Free(tmp);
    }
    exception_str = mputstr(exception_str, param_ctx);
    Free(param_ctx);
    exception_str = mputstr(exception_str, ": ");
    va_list p_var;
    va_start(p_var, err_msg);
    char* error_msg_str = mprintf_va_list(err_msg, p_var);
    va_end(p_var);
    exception_str = mputstr(exception_str, error_msg_str);
    Free(error_msg_str);
    TTCN_error_begin("%s", exception_str);
    Free(exception_str);
    TTCN_error_end();
  }
  else if (Debugger_Value_Parsing::happening()) {
    char* exception_str = mcopystr("Error while overwriting ");
    char* param_ctx;
    if (id && id->is_custom()) {
      param_ctx = mputstr(id->get_str(), " in the variable");
    }
    else {
      char* tmp = get_param_context();
      param_ctx = tmp != NULL ? mprintf("variable field '%s'", tmp) : 
        mcopystr("the variable");
      Free(tmp);
    }
    exception_str = mputstr(exception_str, param_ctx);
    Free(param_ctx);
    exception_str = mputstr(exception_str, ": ");
    va_list p_var;
    va_start(p_var, err_msg);
    char* error_msg_str = mprintf_va_list(err_msg, p_var);
    va_end(p_var);
    exception_str = mputstr(exception_str, error_msg_str);
    Free(error_msg_str);
    ttcn3_debugger.print(DRET_NOTIFICATION, "%s", exception_str);
    Free(exception_str);
    throw TC_Error(); // don't log anything in this case
  }
  TTCN_Logger::begin_event(TTCN_Logger::ERROR_UNQUALIFIED);
  TTCN_Logger::log_event_str("Error while ");
  switch (operation_type) {
  case OT_ASSIGN: TTCN_Logger::log_event_str("setting"); break;
  case OT_CONCAT: TTCN_Logger::log_event_str("concatenating"); break;
  default: TTCN_Logger::log_event_str("???");
  }
  TTCN_Logger::log_event_str(" ");
  if (id && id->is_custom()) {
    char* custom_ctx = id->get_str();
    TTCN_Logger::log_event_str(custom_ctx);
    Free(custom_ctx);
    TTCN_Logger::log_event_str(" in module parameter");
  }
  else {
    TTCN_Logger::log_event_str("parameter field '");
    char* param_ctx = get_param_context();
    TTCN_Logger::log_event_str(param_ctx);
    Free(param_ctx);
    TTCN_Logger::log_event_str("'");
  }
  switch (operation_type) {
  case OT_ASSIGN: TTCN_Logger::log_event_str(" to '"); break;
  case OT_CONCAT: TTCN_Logger::log_event_str(" and '"); break;
  default: TTCN_Logger::log_event_str("' ??? '");
  }
  log(FALSE);
  TTCN_Logger::log_event_str("': ");
  va_list p_var;
  va_start(p_var, err_msg);
  TTCN_Logger::log_event_va_list(err_msg, p_var);
  va_end(p_var);
  TTCN_Logger::send_event_as_error();
  TTCN_Logger::end_event();
  throw TC_Error();
}

void Module_Param::type_error(const char* expected, const char* type_name /* = NULL */) const {
  if (Debugger_Value_Parsing::happening()) {
    // no references when overwriting a variable through the debugger
    error("Type mismatch: %s was expected instead of %s.", expected, get_type_str());
  }
  else {
    const Module_Param* reporter = this;
    // if it's an expression, find its head and use that to report the error
    // (since that's the only parameter with a valid name)
    while (reporter->parent != NULL && reporter->parent->get_type() == MP_Expression) {
      reporter = reporter->parent;
    }
    // either use this parameter's or the referenced parameter's type string
    // (but never the head's type string)
    reporter->error("Type mismatch: %s "
#ifdef TITAN_RUNTIME_2
    "or reference to %s "
#endif
    "was expected%s%s instead of %s%s.",
      expected,
#ifdef TITAN_RUNTIME_2
      expected,
#endif
      (type_name != NULL) ? " for type " : "", (type_name != NULL) ? type_name : "",
#ifdef TITAN_RUNTIME_2
      (get_type() == MP_Reference) ? "reference to " : "",
      (get_type() == MP_Reference) ? get_referenced_param()->get_type_str() : get_type_str()
#else
      "", get_type_str()
#endif
      );
  }
}

boolean Ttcn_String_Parsing::string_parsing = FALSE;
boolean Debugger_Value_Parsing::is_happening = FALSE;
