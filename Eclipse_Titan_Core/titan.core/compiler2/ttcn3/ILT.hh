/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Forstner, Matyas
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef _Ttcn_ILT_HH
#define _Ttcn_ILT_HH

#include "Statement.hh"
#include "AST_ttcn3.hh"
#include "../Setting.hh"
#include "../vector.hh"

namespace Ttcn {

  /**
   * \addtogroup AST
   *
   * @{
   */

  using namespace Common;

  class ILT;
  class ILT_root;
  class ILT_branch;

  class ILT : public Node {
  protected: // ILT_root and ILT_branch need access
    vector<ILT_branch> branches;
  private:
    /** Copy constructor not implemented */
    ILT(const ILT& p);
    /** Assignment not implemented */
    ILT& operator=(const ILT& p);
  public:
    ILT() : Node() { }
    virtual ~ILT();

    virtual ILT_root* get_my_root() = 0;
    virtual ILT_branch* get_as_branch();
    virtual bool is_toplevel() = 0;
    virtual char*& get_out_def() = 0;
    virtual char*& get_out_code() = 0;
    virtual char*& get_out_branches() = 0;
    virtual char*& get_out_def_glob_vars() = 0;
    virtual char*& get_out_src_glob_vars() = 0;
    virtual const string& get_my_tmpid() = 0;
    virtual size_t get_new_tmpnum() = 0;
    virtual size_t get_new_state_var(bool toplevel) = 0;
    virtual size_t get_new_state_var_val(size_t p_state_var) = 0;
    virtual size_t get_new_label_num() = 0;
    void add_branch(ILT_branch *p_branch);
  };

  /**
   * Helper class: Interleave Transformation. Used to generate code
   * for an interleave statement.
   */
  class ILT_root : public ILT {
  private:
    Statement *il; /**< interleave */

    string mytmpid;
    size_t tmpnum;
    size_t c_l; /**< counter for l (label) */
    size_t c_b; /**< counter for branch index  */
    vector<size_t> state_var_vals;

    char *out_def;
    char *out_statevars; // 0: unused; 1: ready; 2: toplevel (IL) etc
    char *out_code;
    char *out_branches;
    char*& out_def_glob_vars;
    char*& out_src_glob_vars;
  private:
    /** Copy constructor not implemented */
    ILT_root(const ILT_root& p);
    /** Assignment not implemented */
    ILT_root& operator=(const ILT_root& p);
  public:
    /** Constructor; the parameter should be an InterleavedStatement */
    ILT_root(Statement *p_il, char*& p_def_glob_vars, char*& p_src_glob_vars);
    virtual ~ILT_root();
    virtual ILT_root *clone() const;
    virtual ILT_root* get_my_root();
    virtual bool is_toplevel();
    virtual char*& get_out_def();
    virtual char*& get_out_code();
    virtual char*& get_out_branches();
    virtual char*& get_out_def_glob_vars();
    virtual char*& get_out_src_glob_vars();
    virtual const string& get_my_tmpid();
    virtual size_t get_new_tmpnum();
    virtual size_t get_new_state_var(bool toplevel);
    virtual size_t get_new_state_var_val(size_t p_state_var);
    virtual size_t get_new_label_num();
    char* generate_code(char *str);
    size_t get_new_branch_num() { return c_b++; }
  };

  /**
   * Helper class for ILT. Represents an alt/receiving branch.
   */
  class ILT_branch : public ILT {
  public:
    enum branchtype_t {
      BT_ALT, // ag
      BT_IL, // ag
      BT_RECV // stmt
    };

  private:
    branchtype_t branchtype;
    union {
      AltGuard *ag;
      Statement *stmt;
    };

    ILT_root *root;
    size_t branch_i; /**< branch index */
    string state_cond; /**< code snippet: "s1==3 && s2==1" */
    size_t state_var; /**< the state var number of this branch, e.g.:
                           s5: 5 */
    size_t state_var_val; /**< current value of my state var, e.g.:
                               s5==3: 3 */
    size_t goto_label_num; /**< goto tmp123_l5: 5 */
  private:
    /** Copy constructor not implemented */
    ILT_branch(const ILT_branch& p);
    /** Assignment not implemented */
    ILT_branch& operator=(const ILT_branch& p);
  public:
    /** Constructor used by BT_ALT and BT_IL */
    ILT_branch(branchtype_t p_bt, AltGuard *p_ag, string p_state_cond,
               size_t p_state_var, size_t p_state_var_val,
               size_t p_goto_label_num);
    /** Constructor used by BT_RECV */
    ILT_branch(branchtype_t p_bt, Statement *p_stmt, string p_state_cond,
               size_t p_state_var, size_t p_state_var_val,
               size_t p_goto_label_num);
    virtual ~ILT_branch();
    virtual ILT_branch *clone() const;
    void set_my_root(ILT_root *p_root);
    virtual ILT_root* get_my_root();
    virtual ILT_branch* get_as_branch();
    virtual bool is_toplevel();
    virtual char*& get_out_def();
    virtual char*& get_out_code();
    virtual char*& get_out_branches();
    virtual char*& get_out_def_glob_vars();
    virtual char*& get_out_src_glob_vars();
    virtual const string& get_my_tmpid();
    virtual size_t get_new_tmpnum();
    virtual size_t get_new_state_var(bool toplevel);
    virtual size_t get_new_state_var_val(size_t p_state_var);
    virtual size_t get_new_label_num();
    void set_branch_i(size_t p_i) {branch_i=p_i;}
    const string& get_state_cond() {return state_cond;}
    size_t get_my_state_var() {return state_var;}
    size_t get_my_state_var_val() {return state_var_val;}
    void ilt_generate_code(ILT *ilt);
  };

} // namespace Ttcn

#endif // _Ttcn_ILT_HH

