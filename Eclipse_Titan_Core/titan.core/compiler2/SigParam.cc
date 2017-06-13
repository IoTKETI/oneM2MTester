/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Raduly, Csaba
 *
 ******************************************************************************/
#include "SigParam.hh"

#include "Type.hh"
#include "CompilerError.hh"

namespace Common {

// =================================
// ===== SignatureParam
// =================================

SignatureParam::SignatureParam(param_direction_t p_d, Type *p_t,
                               Identifier *p_i)
  : param_direction(p_d), param_type(p_t), param_id(p_i)
{
  if (!p_t || !p_i) FATAL_ERROR("SignatureParam::SignatureParam()");
  param_type->set_ownertype(Type::OT_SIG_PAR, this);
}

SignatureParam::~SignatureParam()
{
  delete param_type;
  delete param_id;
}

SignatureParam *SignatureParam::clone() const
{
  FATAL_ERROR("SignatureParam::clone");
}

void SignatureParam::set_fullname(const string& p_fullname)
{
  Node::set_fullname(p_fullname);
  param_type->set_fullname(p_fullname);
}

void SignatureParam::set_my_scope(Scope *p_scope)
{
  param_type->set_my_scope(p_scope);
}

void SignatureParam::dump(unsigned level) const
{
  switch(param_direction) {
  case PARAM_IN: DEBUG(level,"in"); break;
  case PARAM_OUT: DEBUG(level,"out"); break;
  case PARAM_INOUT: DEBUG(level,"inout");break;
  default: FATAL_ERROR("SignatureParam::dump()"); break;
  }
  param_type->dump(level+1);
  param_id->dump(level+2);
}

// =================================
// ===== SignatureParamList
// =================================

SignatureParamList::~SignatureParamList()
{
  for (size_t i = 0; i < params_v.size(); i++) delete params_v[i];
  params_v.clear();
  params_m.clear();
  in_params_v.clear();
  out_params_v.clear();
}

SignatureParamList *SignatureParamList::clone() const
{
  FATAL_ERROR("SignatureParam::clone");
}

void SignatureParamList::set_fullname(const string& p_fullname)
{
  Node::set_fullname(p_fullname);
  for (size_t i = 0; i < params_v.size(); i++) {
    SignatureParam *param = params_v[i];
    param->set_fullname(p_fullname + "." + param->get_id().get_dispname());
  }
}

void SignatureParamList::set_my_scope(Scope *p_scope)
{
  for (size_t i = 0; i < params_v.size(); i++)
    params_v[i]->set_my_scope(p_scope);
}

void SignatureParamList::add_param(SignatureParam *p_param)
{
  if (!p_param || checked) FATAL_ERROR("SignatureParamList::add_param()");
  params_v.add(p_param);
}

size_t SignatureParamList::get_nof_in_params() const
{
  if (!checked) FATAL_ERROR("SignatureParamList::get_nof_in_params()");
  return in_params_v.size();
}

SignatureParam *SignatureParamList::get_in_param_byIndex(size_t n) const
{
  if (!checked) FATAL_ERROR("SignatureParamList::get_in_param_byIndex()");
  return in_params_v[n];
}

size_t SignatureParamList::get_nof_out_params() const
{
  if (!checked) FATAL_ERROR("SignatureParamList::get_nof_out_params()");
  return out_params_v.size();
}

SignatureParam *SignatureParamList::get_out_param_byIndex(size_t n) const
{
  if (!checked) FATAL_ERROR("SignatureParamList::get_out_param_byIndex()");
  return out_params_v[n];
}

bool SignatureParamList::has_param_withName(const Identifier& p_name) const
{
  if (!checked) FATAL_ERROR("SignatureParamList::has_param_withName()");
  return params_m.has_key(p_name.get_name());
}

const SignatureParam *SignatureParamList::get_param_byName
  (const Identifier& p_name) const
{
  if (!checked) FATAL_ERROR("SignatureParamList::get_param_byName()");
  return params_m[p_name.get_name()];
}

void SignatureParamList::chk(Type *p_signature)
{
  if (checked) return;
  checked = true;
  for (size_t i = 0; i < params_v.size(); i++) {
    SignatureParam *param = params_v[i];
    const Identifier& id = param->get_id();
    const string& name = id.get_name();
    const char *dispname_str = id.get_dispname().c_str();
    if (params_m.has_key(name)) {
      param->error("Duplicate parameter identifier: `%s'", dispname_str);
      params_m[name]->note("Parameter `%s' is already defined here",
        dispname_str);
    } else params_m.add(name, param);
    Error_Context cntxt(param, "In parameter `%s'", dispname_str);
    bool is_nonblock = p_signature->is_nonblocking_signature();
    switch (param->get_direction()) {
    case SignatureParam::PARAM_IN:
      in_params_v.add(param);
      break;
    case SignatureParam::PARAM_OUT:
      if (is_nonblock) param->error("A non-blocking signature cannot have "
        "`out' parameter");
      out_params_v.add(param);
      break;
    case SignatureParam::PARAM_INOUT:
      if (is_nonblock) param->error("A non-blocking signature cannot have "
        "`inout' parameter");
      in_params_v.add(param);
      out_params_v.add(param);
      break;
    default:
      FATAL_ERROR("SignatureParamList::chk()");
    }
    Type *param_type = param->get_type();
    param_type->set_genname(p_signature->get_genname_own(), name);
    param_type->set_parent_type(p_signature);
    param_type->chk();
    param_type->chk_embedded(false, "the type of a signature parameter");
  }
}

void SignatureParamList::dump(unsigned level) const
{
  for (size_t i = 0; i < params_v.size(); i++) params_v[i]->dump(level);
}

// =================================
// ===== SignatureExceptions
// =================================

SignatureExceptions::~SignatureExceptions()
{
  for (size_t i = 0; i < exc_v.size(); i++) delete exc_v[i];
  exc_v.clear();
  exc_m.clear();
}

SignatureExceptions *SignatureExceptions::clone() const
{
  FATAL_ERROR("SignatureExceptions::clone");
}

void SignatureExceptions::add_type(Type *p_type)
{
  if (!p_type) FATAL_ERROR("SignatureExceptions::add_type()");
  exc_v.add(p_type);
}

bool SignatureExceptions::has_type(Type *p_type)
{
  if (!p_type) FATAL_ERROR("SignatureExceptions::has_type()");
  if (p_type->get_type_refd_last()->get_typetype() == Type::T_ERROR)
    return true;
  else return exc_m.has_key(p_type->get_typename());
}

size_t SignatureExceptions::get_nof_compatible_types(Type *p_type)
{
  if (!p_type) FATAL_ERROR("SignatureExceptions::get_nof_compatible_types()");
  if (p_type->get_type_refd_last()->get_typetype() == Type::T_ERROR) {
    // Return a positive answer for erroneous types.
    return 1;
  } else {
    size_t ret_val = 0;
    for (size_t i = 0; i < exc_v.size(); i++)
      // Don't allow type compatibility.  The types must match exactly.
      if (exc_v[i]->is_compatible(p_type, NULL, NULL))
        ret_val++;
    return ret_val;
  }
}

void SignatureExceptions::chk(Type *p_signature)
{
  Error_Context cntxt(this, "In exception list");
  for (size_t i = 0; i < exc_v.size(); i++) {
    Type *type = exc_v[i];
    type->set_genname(p_signature->get_genname_own(), Int2string(i + 1));
    type->set_parent_type(p_signature);
    type->chk();
    if (type->get_typetype() == Type::T_ERROR) continue;
    type->chk_embedded(false, "on the exception list of a signature");
    const string& type_name = type->get_typename();
    if (exc_m.has_key(type_name)) {
      type->error("Duplicate type in exception list");
      exc_m[type_name]->note("Type `%s' is already given here",
        type_name.c_str());
    } else exc_m.add(type_name, type);
  }
}

void SignatureExceptions::set_fullname(const string& p_fullname)
{
  Node::set_fullname(p_fullname);
  for (size_t i = 0; i < exc_v.size(); i++)
    exc_v[i]->set_fullname(p_fullname + ".<type" + Int2string(i + 1) + ">");
}

void SignatureExceptions::set_my_scope(Scope *p_scope)
{
  for (size_t i = 0; i < exc_v.size(); i++)
    exc_v[i]->set_my_scope(p_scope);
}

void SignatureExceptions::dump(unsigned level) const
{
  for (size_t i=0; i < exc_v.size(); i++) exc_v[i]->dump(level);
}

} /* namespace Common */
