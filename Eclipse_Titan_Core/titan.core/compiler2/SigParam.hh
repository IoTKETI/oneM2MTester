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
#ifndef SIGPARAM_HH_
#define SIGPARAM_HH_

#include "Setting.hh"

namespace Common {

/**
 * Signature parameter
 */
class SignatureParam : public Node, public Location {
public:
  enum param_direction_t { PARAM_IN, PARAM_OUT, PARAM_INOUT };
private:
  param_direction_t  param_direction;
  Type *param_type;
  Identifier *param_id;
  /** Copy constructor not implemented */
  SignatureParam(const SignatureParam& p);
  /** Assignment disabled */
  SignatureParam& operator=(const SignatureParam& p);
public:
  SignatureParam(param_direction_t p_d, Type *p_t, Identifier *p_i);
  ~SignatureParam();
  virtual SignatureParam *clone() const;
  const Identifier& get_id() const { return *param_id; }
  Type *get_type() const { return param_type; }
  param_direction_t get_direction() const { return param_direction; }
  virtual void set_fullname(const string& p_fullname);
  virtual void set_my_scope(Scope *p_scope);
  virtual void dump(unsigned level) const;
};

/**
 * Signature parameter-list
 */
class SignatureParamList : public Node {
  vector<SignatureParam> params_v;
  map<string, SignatureParam> params_m;
  vector<SignatureParam> in_params_v, out_params_v;
  bool checked;
private:
  /** Copy constructor not implemented */
  SignatureParamList(const SignatureParamList& p);
  /** Assignment disabled */
  SignatureParamList& operator=(const SignatureParamList& p);
public:
  SignatureParamList()
  : Node(), params_v(), params_m(), in_params_v(), out_params_v(),
    checked(false) { }
  ~SignatureParamList();
  virtual SignatureParamList *clone() const;
  virtual void set_fullname(const string& p_fullname);
  virtual void set_my_scope(Scope *p_scope);
  void add_param(SignatureParam *p_param);
  size_t get_nof_params() const { return params_v.size(); }
  SignatureParam *get_param_byIndex(size_t n) const { return params_v[n]; }
  /** Returns the number of in/inout parameters. */
  size_t get_nof_in_params() const;
  /** Returns the n-th in/inout parameter. */
  SignatureParam *get_in_param_byIndex(size_t n) const;
  /** Returns the number of out/inout parameters. */
  size_t get_nof_out_params() const;
  /** Returns the n-th out/inout parameter. */
  SignatureParam *get_out_param_byIndex(size_t n) const;
  bool has_param_withName(const Identifier& p_name) const;
  const SignatureParam *get_param_byName(const Identifier& p_name) const;

  void chk(Type *p_signature);
  virtual void dump(unsigned level) const;
};

/**
 * Signature exception-list
 */
class SignatureExceptions : public Node, public Location {
  vector<Type> exc_v;
  map<string,Type> exc_m;
private:
  /** Copy constructor not implemented */
  SignatureExceptions(const SignatureExceptions& p);
  /** Assignment disabled */
  SignatureExceptions& operator=(const SignatureExceptions& p);
public:
  SignatureExceptions() : Node(), Location(), exc_v(), exc_m() { }
  ~SignatureExceptions();
  SignatureExceptions *clone() const;
  void add_type(Type *p_type);
  size_t get_nof_types() const { return exc_v.size(); }
  Type *get_type_byIndex(size_t n) const { return exc_v[n]; }
  bool has_type(Type *p_type);
  /** Returns the number of types that are compatible with * \a p_type. */
  size_t get_nof_compatible_types(Type *p_type);
  Type *get_type_byName(const string& p_typename) const
   { return exc_m[p_typename]; }

  void chk(Type *p_signature);
  virtual void set_fullname(const string& p_fullname);
  virtual void set_my_scope(Scope *p_scope);
  virtual void dump(unsigned level) const;
};

} /* namespace Common */
#endif /* SIGPARAM_HH_ */
