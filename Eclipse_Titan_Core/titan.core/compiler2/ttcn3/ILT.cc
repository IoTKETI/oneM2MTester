/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#include "ILT.hh"

namespace Ttcn {

  // =================================
  // ===== ILT
  // =================================

  ILT::~ILT()
  {
    for(size_t i=0; i<branches.size(); i++)
      delete branches[i];
    branches.clear();
  }

  ILT_branch* ILT::get_as_branch()
  {
    FATAL_ERROR("ILT::get_as_branch()");
    return 0;
  }

  void ILT::add_branch(ILT_branch *p_branch)
  {
    if(!p_branch) FATAL_ERROR("ILT::add_branch()");
    branches.add(p_branch);
    ILT_root *my_root = get_my_root();
    p_branch->set_my_root(my_root);
    p_branch->set_branch_i(my_root->get_new_branch_num());
  }


  // =================================
  // ===== ILT_root
  // =================================

  ILT_root::ILT_root(Statement *p_il, char*& p_def_glob_vars, char*& p_src_glob_vars)
    : il(p_il), tmpnum(0), c_l(0), c_b(0), out_def_glob_vars(p_def_glob_vars)
    , out_src_glob_vars(p_src_glob_vars)
  {
    if(!p_il || p_il->get_statementtype()!=Statement::S_INTERLEAVE)
      FATAL_ERROR("ILT_root::ILT_root()");
    mytmpid=p_il->get_my_sb()->get_scope_mod_gen()->get_temporary_id();
    out_def=memptystr();
    out_statevars=memptystr();
    out_code=memptystr();
    out_branches=memptystr();
  }

  ILT_root::~ILT_root()
  {
    for(size_t i=0; i<state_var_vals.size(); i++)
      delete state_var_vals[i];
    state_var_vals.clear();
    Free(out_def);
    Free(out_statevars);
    Free(out_code);
    Free(out_branches);
  }

  ILT_root *ILT_root::clone() const
  {
    FATAL_ERROR("ILT_root::clone()");
  }

  ILT_root *ILT_root::get_my_root()
  {
    return this;
  }

  bool ILT_root::is_toplevel()
  {
    return true;
  }

  char*& ILT_root::get_out_def()
  {
    return out_def;
  }

  char*& ILT_root::get_out_code()
  {
    return out_code;
  }

  char*& ILT_root::get_out_branches()
  {
    return out_branches;
  }
  
  char*& ILT_root::get_out_def_glob_vars()
  {
    return out_def_glob_vars;
  }
  
  char*& ILT_root::get_out_src_glob_vars()
  {
    return out_src_glob_vars;
  }

  const string& ILT_root::get_my_tmpid()
  {
    return mytmpid;
  }

  size_t ILT_root::get_new_tmpnum()
  {
    return tmpnum++;
  }

  size_t ILT_root::get_new_state_var(bool toplevel)
  {
    state_var_vals.add(new size_t(1));
    size_t newvar=state_var_vals.size()-1;
    out_statevars=mputprintf(out_statevars, "%s_state[%lu]=%s;\n",
      mytmpid.c_str(), static_cast<unsigned long>( newvar ), toplevel?"2":"0");
    return newvar;
  }

  size_t ILT_root::get_new_state_var_val(size_t p_state_var)
  {
    if(p_state_var>=state_var_vals.size())
      FATAL_ERROR("ILT_root::get_new_state_var_val()");
    return ++*state_var_vals[p_state_var];
  }

  size_t ILT_root::get_new_label_num()
  {
    return c_l++;
  }

  char* ILT_root::generate_code(char *str)
  {
    // everything starts here: the top-level interleave statement
    il->ilt_generate_code(this);
    // generating code for the branches
    for(size_t i=0; i<branches.size(); i++)
      branches[i]->ilt_generate_code(this);
    str=mputstr(str, "{\n"); // (1)
    // the state vars: definition
    str=mputprintf(str, "size_t %s_state[%lu];\n",
                   mytmpid.c_str(), static_cast<unsigned long>( state_var_vals.size()));
    // the state vars: initialization
    str=mputstr(str, out_statevars);
    // the alt status flags: definition
    str=mputprintf(str, "alt_status %s_alt_flag[%lu];\n",
                   mytmpid.c_str(), static_cast<unsigned long>(c_b+1) );
    // the definitions
    str=mputstr(str, out_def);
    // the core of the interleave
    str=mputprintf(str, "%s:\n"
                   "for(size_t tmp_i=0; tmp_i<%lu; tmp_i++)"
                   " %s_alt_flag[tmp_i]=ALT_UNCHECKED;\n"
                   "%s_alt_flag[%lu]=ALT_MAYBE;\n"
                   "TTCN_Snapshot::take_new(FALSE);\n"
                   "for( ; ; ) {\n" // (2)
                   "if(", mytmpid.c_str(),
                   static_cast<unsigned long>( c_b ), mytmpid.c_str(),
                   mytmpid.c_str(), static_cast<unsigned long>( c_b ));
    // checking for final condition
    for(size_t i=0; i<branches.size(); i++) {
      ILT_branch *b=branches[i];
      str=mputprintf(str, "%s%s_state[%lu]==1",
        i?" && ":"", mytmpid.c_str(), static_cast<unsigned long>( b->get_my_state_var() ));
    }
    str=mputstr(str, ") break;\n");
    // the altsteps :)
    str=mputstr(str, out_code);
    // checking defaults
    str=mputprintf(str, "if(%s_alt_flag[%lu]==ALT_MAYBE) {\n"
                   "%s_alt_flag[%lu]=TTCN_Default::try_altsteps();\n"
                 "if(%s_alt_flag[%lu]==ALT_YES || %s_alt_flag[%lu]==ALT_BREAK)"
                     " break;\n"
                   "else if(%s_alt_flag[%lu]==ALT_REPEAT) goto %s;\n"
                   "}\n",
                   mytmpid.c_str(), static_cast<unsigned long>( c_b ),
                   mytmpid.c_str(), static_cast<unsigned long>( c_b ),
                   mytmpid.c_str(), static_cast<unsigned long>( c_b ),
                   mytmpid.c_str(), static_cast<unsigned long>( c_b ),
                   mytmpid.c_str(), static_cast<unsigned long>( c_b ),
                   mytmpid.c_str());
    str=il->update_location_object(str);
    // checking deadlock
    str=mputprintf(str, "for(size_t tmp_i=0; tmp_i<%lu; tmp_i++)"
                   " if(%s_alt_flag[tmp_i]!=ALT_NO) goto %s_newsnapshot;\n",
                   static_cast<unsigned long>(c_b+1), mytmpid.c_str(), mytmpid.c_str());
    str = mputstr(str, "TTCN_error(\"None of the branches can be chosen in the "
      "interleave statement in file ");
    str = Code::translate_string(str, il->get_filename());
    int first_line = il->get_first_line(), last_line = il->get_last_line();
    if (first_line < last_line) str = mputprintf(str,
      " between lines %d and %d", first_line, last_line);
    else str = mputprintf(str, ", line %d", first_line);
    str = mputprintf(str, ".\");\n"
      "%s_newsnapshot:\n"
      "TTCN_Snapshot::take_new(TRUE);\n"
      "continue;\n", mytmpid.c_str());
    // the code of branches
    str=mputstr(str, out_branches);
    str=mputstr(str, "}\n" // (2)
                "}\n"); // (1)
    return str;
  }

  // =================================
  // ===== ILT_branch
  // =================================

  ILT_branch::ILT_branch(branchtype_t p_bt, AltGuard *p_ag,
                         string p_state_cond, size_t p_state_var,
                         size_t p_state_var_val, size_t p_goto_label_num)
    : branchtype(p_bt), root(0), branch_i(static_cast<size_t>(-1)),
      state_cond(p_state_cond), state_var(p_state_var),
      state_var_val(p_state_var_val), goto_label_num(p_goto_label_num)
  {
    switch(p_bt) {
    case BT_ALT:
    case BT_IL:
      if(!p_ag) FATAL_ERROR("ILT_branch::ILT_branch()");
      ag=p_ag;
      // the checker should check this but one never knows :)
      if(ag->get_type()!=AltGuard::AG_OP)
        FATAL_ERROR("ILT_branch::ILT_branch()");
      break;
    default:
      FATAL_ERROR("ILT_branch::ILT_branch()");
    } // switch
  }

  ILT_branch::ILT_branch(branchtype_t p_bt, Statement *p_stmt,
                         string p_state_cond, size_t p_state_var,
                         size_t p_state_var_val, size_t p_goto_label_num)
    : branchtype(p_bt), root(0), branch_i(static_cast<size_t>(-1)),
      state_cond(p_state_cond), state_var(p_state_var),
      state_var_val(p_state_var_val), goto_label_num(p_goto_label_num)
  {
    switch(p_bt) {
    case BT_RECV:
      if(!p_stmt) FATAL_ERROR("ILT_branch::ILT_branch()");
      stmt=p_stmt;
      break;
    default:
      FATAL_ERROR("ILT_branch::ILT_branch()");
    } // switch
  }

  ILT_branch::~ILT_branch()
  {
    // nothing to do
  }

  ILT_branch *ILT_branch::clone() const
  {
    FATAL_ERROR("ILT_branch::clone()");
  }

  void ILT_branch::set_my_root(ILT_root *p_root)
  {
    if(!p_root) FATAL_ERROR("ILT_branch::set_my_root()");
    root=p_root;
  }

  ILT_root* ILT_branch::get_my_root()
  {
    if (!root) FATAL_ERROR("ILT_branch::get_my_root()");
    return root;
  }

  ILT_branch *ILT_branch::get_as_branch()
  {
    return this;
  }

  bool ILT_branch::is_toplevel()
  {
    return false;
  }

  char*& ILT_branch::get_out_def()
  {
    return root->get_out_def();
  }

  char*& ILT_branch::get_out_code()
  {
    return root->get_out_code();
  }

  char*& ILT_branch::get_out_branches()
  {
    return root->get_out_branches();
  }
  
  char*& ILT_branch::get_out_def_glob_vars()
  {
    return root->get_out_def_glob_vars();
  }
  
  char*& ILT_branch::get_out_src_glob_vars()
  {
    return root->get_out_src_glob_vars();
  }

  const string& ILT_branch::get_my_tmpid()
  {
    return root->get_my_tmpid();
  }

  size_t ILT_branch::get_new_tmpnum()
  {
    return root->get_new_tmpnum();
  }

  size_t ILT_branch::get_new_state_var(bool toplevel)
  {
    return root->get_new_state_var(toplevel);
  }

  size_t ILT_branch::get_new_state_var_val(size_t p_state_var)
  {
    return root->get_new_state_var_val(p_state_var);
  }

  size_t ILT_branch::get_new_label_num()
  {
    return root->get_new_label_num();
  }

  void ILT_branch::ilt_generate_code(ILT *ilt)
  {
    const string& mytmpid=get_my_tmpid();
    char*& out_code=get_out_code();
    Statement *recv_stmt
      =branchtype==BT_RECV?stmt:ag->get_guard_stmt();
    out_code=recv_stmt->update_location_object(out_code);
    // ALT_UNCHECKED -> ALT_MAYBE or ALT_NO (state vars & guard expr)
    {
      out_code=mputprintf(out_code,
                          "if(%s_alt_flag[%lu]==ALT_UNCHECKED) {\n" // (1)
                          "if(",
                          mytmpid.c_str(), static_cast<unsigned long>( branch_i ));
      if(!state_cond.empty())
        out_code=mputprintf(out_code, "%s && ", state_cond.c_str());
      out_code=mputprintf(out_code, "%s_state[%lu]==%lu) {\n", // (2)
                          mytmpid.c_str(), static_cast<unsigned long>( state_var ),
                          static_cast<unsigned long>( state_var_val ));
      // only alt branch can have guard expression
      Value *guard_expr=branchtype==BT_ALT?ag->get_guard_expr():0;
      if(guard_expr) {
        out_code=guard_expr->update_location_object(out_code);
        expression_struct expr;
        Code::init_expr(&expr);
        guard_expr->generate_code_expr(&expr);
        out_code=mputstr(out_code, expr.preamble);
        out_code=mputprintf(out_code,
                            "%s_alt_flag[%lu]=(%s)?ALT_MAYBE:ALT_NO;\n",
                            mytmpid.c_str(), static_cast<unsigned long>( branch_i ),
                            expr.expr);
        out_code=mputstr(out_code, expr.postamble);
        Code::free_expr(&expr);
      } // if guard_expr
      else out_code=mputprintf(out_code,
                               "%s_alt_flag[%lu]=ALT_MAYBE;\n",
                               mytmpid.c_str(), static_cast<unsigned long>( branch_i ));
      out_code=mputprintf(out_code,
                          "}\n" // (2)
                          "else %s_alt_flag[%lu]=ALT_NO;\n"
                          "}\n", // (1)
                          mytmpid.c_str(), static_cast<unsigned long>( branch_i ));
    }
    // ALT_MAYBE -> ALT_YES or ALT_REPEAT or ALT_NO (guard stmt)
    {
      out_code=mputprintf(out_code,
                          "if(%s_alt_flag[%lu]==ALT_MAYBE) {\n", // (1)
                          mytmpid.c_str(), static_cast<unsigned long>( branch_i ));
      expression_struct expr;
      Code::init_expr(&expr);
      recv_stmt->generate_code_expr(&expr);
      // indicates whether the guard operation might return ALT_REPEAT
      bool can_repeat=recv_stmt->can_repeat();
      if(expr.preamble || expr.postamble) {
        out_code=mputstr(out_code, "{\n");
        out_code=mputstr(out_code, expr.preamble);
      }
      out_code=mputprintf(out_code, "%s_alt_flag[%lu]=%s;\n",
                          mytmpid.c_str(), static_cast<unsigned long>( branch_i), expr.expr);
      if(expr.preamble || expr.postamble) {
        out_code=mputstr(out_code, expr.postamble);
        out_code=mputstr(out_code, "}\n");
      }
      Code::free_expr(&expr);
      if(can_repeat)
        out_code=mputprintf(out_code,
                            "if(%s_alt_flag[%lu]==ALT_REPEAT) goto %s;\n",
                            mytmpid.c_str(), static_cast<unsigned long>( branch_i),
                            mytmpid.c_str());
    }
    if(branchtype==BT_RECV)
      out_code=mputprintf(out_code,
                          "if(%s_alt_flag[%lu]==ALT_YES) goto %s_l%lu;\n",
                          mytmpid.c_str(), static_cast<unsigned long>( branch_i),
                          mytmpid.c_str(), static_cast<unsigned long>( goto_label_num));
    else {
      out_code=mputprintf(out_code,
                          "if(%s_alt_flag[%lu]==ALT_YES) ",
                          mytmpid.c_str(), static_cast<unsigned long>( branch_i));
      // statement block code generation;
      StatementBlock *block=ag->get_block();
      bool has_recv=block->has_receiving_stmt();
      char*& out_stmt=has_recv?get_out_branches():out_code;
      if(has_recv) {
        out_code=mputprintf(out_code,
                            "goto %s_branch%lu;\n",
                            mytmpid.c_str(), static_cast<unsigned long>( branch_i));
        out_stmt=mputprintf(out_stmt, "%s_branch%lu:\n",
                            mytmpid.c_str(), static_cast<unsigned long>( branch_i));
        block->ilt_generate_code(this);
      }
      else {
        out_stmt=mputstr(out_stmt, "{\n"); // (2)
        out_stmt=block->generate_code(out_stmt, get_out_def_glob_vars(),
          get_out_src_glob_vars());
      }
      if(branchtype==BT_IL) {
        out_stmt=mputprintf(out_stmt, "%s_state[%lu]=1;\n",
                            mytmpid.c_str(), static_cast<unsigned long>( state_var));
        if(!ilt->is_toplevel()) {
          vector<ILT_branch>& ilt_branches = ilt->get_as_branch()->branches;
          bool use_loop=ilt_branches.size() > 8;
          size_t bmin=0, bmax=0;
          if (use_loop) {
            size_t st_var=ilt_branches[0]->get_my_state_var();
            bmin=st_var; bmax=st_var;
            for (size_t i=1; i<ilt_branches.size(); i++) {
              st_var=ilt_branches[i]->get_my_state_var();
              if (st_var<bmin) bmin=st_var;
              if (st_var>bmax) bmax=st_var;
            }
            use_loop=(bmax-bmin+1) == ilt_branches.size();
          }
          if (use_loop) {
            out_stmt=mputprintf(out_stmt,
              "for(size_t tmp_i=%lu; tmp_i<%lu; tmp_i++)"
              " if (%s_state[tmp_i]!=1) goto %s;\n",
              static_cast<unsigned long>( bmin), static_cast<unsigned long>( bmax+1 ),
              mytmpid.c_str(), mytmpid.c_str());
          } else if (ilt_branches.size() > 1) {
            // Check if any branches of non top level interleave need to be exe.
            for(size_t i=0, j=0; i<ilt_branches.size(); i++) {
              ILT_branch *b=ilt_branches[i];
              if (b != this)
                out_stmt=mputprintf(out_stmt, "%s%s_state[%lu]!=1",
                  j++?" || ":"if (", mytmpid.c_str(),
                  static_cast<unsigned long>( b->get_my_state_var() ));
            }
            out_stmt=mputprintf(out_stmt, ") goto %s;\n", mytmpid.c_str());
          }
          // non top level interleave: Handle finish case
          out_stmt=mputprintf(out_stmt, "goto %s_l%lu;\n",
                              mytmpid.c_str(), static_cast<unsigned long>( goto_label_num ));
        } else // top level: finish condition is checked at the beginning
          out_stmt=mputprintf(out_stmt, "goto %s;\n", mytmpid.c_str());
      } else // not interleave: finish after one branch execution
        out_stmt=mputprintf(out_stmt, "goto %s_l%lu;\n",
          mytmpid.c_str(), static_cast<unsigned long>( goto_label_num ));
      if(!has_recv)
        out_stmt=mputstr(out_stmt, "}\n"); // (2)
    }
    out_code=mputstr(out_code, "}\n"); // (1)
    // the nested branches
    for(size_t i=0; i<branches.size(); i++)
      branches[i]->ilt_generate_code(this);
  }

} // namespace Ttcn
