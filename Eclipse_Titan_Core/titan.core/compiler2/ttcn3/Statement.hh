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
 *   Delic, Adam
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#ifndef _Ttcn_Statement_HH
#define _Ttcn_Statement_HH

#include "AST_ttcn3.hh"

namespace Ttcn {

  /**
   * \addtogroup AST
   *
   * @{
   */

  using namespace Common;

  class StatementBlock;
  class Statement;
  class Assignment;
  class Assignments;
  class ParamAssignment;
  class ParamAssignments;
  class VariableEntry;
  class VariableEntries;
  class ParamRedirect;
  class ValueRedirect;
  class LogArgument;
  class LogArguments;
  class IfClause;
  class IfClauses;
  class SelectCase;
  class SelectCases;
  class SelectUnion;
  class SelectUnions;
  class AltGuard;
  class AltGuards;
  class WithAttribPath;
  /* not defined here: */
  class ILT;
  class ILT_branch;
  class WithAttribPath;
  class ErroneousAttributes;

  /**
   * Represents a %StatementBlock.
   */
  class StatementBlock : public Scope, public Location {
  public:
    enum returnstatus_t {
      RS_NO, /**< indicates that the block does not have return statement
                  at all */
      RS_MAYBE, /**< indicates that some branches of the embedded statements
                     have a return statement but others do not */
      RS_YES /**< indicates that the block or all branches of the
                  embedded statements have a return statement */
    };

    enum exception_handling_t {
      EH_NONE, /**< this is a normal statement block */
      EH_TRY,  /**< this is a try{} block */
      EH_CATCH /**< this is a catch(exc_str){} block */
    };

  private:
    vector<Statement> stmts;
    /** to store pointers to definitions defined in scope */
    map<Identifier, Definition> defs;
    /** to store goto labels and pointer to their statement */
    map<Identifier, Statement> labels;
    bool checked, labels_checked;
    /** parent sb, if any */
    StatementBlock *my_sb;
    /** my index in the parent sb, if applicable */
    size_t my_sb_index;
    /** The function/altstep/testcase/etc. the StatementBlock belongs to. */
    Definition *my_def;
    /** */
    exception_handling_t exception_handling;

    StatementBlock(const StatementBlock& p);
    StatementBlock& operator=(const StatementBlock& p);
  public:
    StatementBlock();
    virtual ~StatementBlock();
    virtual StatementBlock* clone() const;
    void add_stmt(Statement *p_stmt, bool to_front=false);
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_fullname(const string& p_fullname);
    size_t get_nof_stmts() const { return stmts.size(); }
    Statement *get_stmt_byIndex(size_t p_i) const { return stmts[p_i]; }
    /** Returns the first statement of the block or NULL if there is no such
     * statement. It ignores labels and goes recursively into nested blocks */
    Statement *get_first_stmt() const;
    /* Scope-related functions */
    void register_def(Definition *p_def);
    virtual bool has_ass_withId(const Identifier& p_id);
    virtual Common::Assignment* get_ass_bySRef(Ref_simple *p_ref);
    virtual Type *get_mtc_system_comptype(bool is_system);
    Definition* get_my_def() const { return my_def; }
    virtual Ttcn::StatementBlock *get_statementblock_scope();
    void set_my_sb(StatementBlock *p_sb, size_t p_index);
    StatementBlock* get_my_sb() const { return my_sb; }
    size_t get_my_sb_index() const;
    void set_my_def(Definition *p_def);
    void set_my_ags(AltGuards *p_ags);
    void set_my_laic_stmt(AltGuards *p_ags, Statement *p_loop_stmt);
    returnstatus_t has_return() const;
    /** Used when generating code for interleaved statement. If has
     *  not recv stmt, then the general code generation can be used
     *  (which may use blocks). */
    bool has_receiving_stmt(size_t start_i=0) const;
    /** Used when generating code for interleaved statement. If has
     *  not S_DEF stmt, then there is no need for {}. */
    bool has_def_stmt_i(size_t start_i=0) const;

    void set_exception_handling(exception_handling_t eh) { exception_handling = eh; }
    exception_handling_t get_exception_handling() const { return exception_handling; }

    virtual void dump(unsigned int level) const;
  private:
    /** Returns the index of the last receiving statement after
     *  start_i, or the number of statements if has not recv stmt. */
    size_t last_receiving_stmt_i() const;
    StatementBlock *get_parent_block() const;
    void chk_trycatch_blocks(Statement* prev, Statement* act);
  public:
    /* checking functions */
    void chk();
    /** checks whether all embedded statements are allowed in an interleaved
     * construct */
    void chk_allowed_interleave();
  private:
    void chk_labels();
    void chk_unused_labels();
  public:
    bool has_label(const Identifier& p_id) const;
    Statement *get_label(const Identifier& p_id) const;
    /** Sets the code section selector of all embedded values and
     *  templates to \a p_code_section. */
    void set_code_section(GovernedSimple::code_section_t p_code_section);
    /** Generates code for this statement block.
      * '@update' statements may need to generate code into the global variables
      * sections of the module's header and source file, so pointer references to
      * these strings are passed to every statement block that may contain an
      * '@update' statement. */
    char* generate_code(char *str, char*& def_glob_vars, char*& src_glob_vars);
    void ilt_generate_code(ILT *ilt);

    virtual void set_parent_path(WithAttribPath* p_path);
  };

  /**
   * Represents a statement (FunctionStatement) in dynamic part of
   * TTCN-3.
   */
  class Statement : public Node, public Location {
  public:
    enum statementtype_t {
      /* erroneous statement */
      S_ERROR,
      /* ambiguous statements */
      S_START_UNDEF, // undefstartstop
      S_STOP_UNDEF, // undefstartstop
      S_UNKNOWN_INSTANCE, // ref_pard
      S_UNKNOWN_INVOKED, // fau_refd
      /* basic statements */
      S_DEF, // def (const or template def; variable or timer instance)
      S_ASSIGNMENT, // ass
      S_FUNCTION_INSTANCE, // ref_pard
      S_FUNCTION_INVOKED, // fau_refd
      S_BLOCK, // block
      S_LOG, // logargs
      S_LABEL, // label
      S_GOTO, // go_to
      S_BREAK, // brk_cnt
      S_CONTINUE, // brk_cnt
      S_IF, // if_stmt
      S_SELECT, // select
      S_SELECTUNION, // select union
      S_FOR, // loop
      S_WHILE, // loop
      S_DOWHILE, // loop
      S_STOP_EXEC, // -
      S_STOP_TESTCASE, // logargs
      /* behaviour statements */
      S_ALT, // ags
      S_REPEAT, // ags BUT only a pointer (my_ags)
      S_INTERLEAVE, // ags
      S_ALTSTEP_INSTANCE, // ref_pard
      S_ALTSTEP_INVOKED, // fau_refd
      S_RETURN, // returnexpr
      /* default statements */
      S_ACTIVATE, // ref_pard
      S_ACTIVATE_REFD, //activate_refd
      S_DEACTIVATE, // deactivate
      /* communication (port) statements */
      S_SEND, // port send_par to_clause
      S_CALL, // port send_par call to_clause
      S_REPLY, // port send_par reply_val to_clause
      S_RAISE, // port raise_signature send_par to_clause
      S_RECEIVE, // port rcv_par from_clause redirect.value
		 // redirect.sender
      S_TRIGGER, // port rcv_par from_clause redirect.value
		 // redirect.sender
      S_GETCALL, // port rcv_par from_clause redirect.value
		 // redirect.sender redirect.par_spec
      S_GETREPLY, // port rcv_par getreply_valuematch from_clause
		  // redirect.value redirect.sender redirect.par_spec
      S_CATCH, // port ctch.signature rcv_par ctch.timeout from_clause
	       // redirect.value redirect.sender
      S_CHECK, // port from_clause redirect.sender
      S_CHECK_RECEIVE,
      S_CHECK_GETCALL,
      S_CHECK_GETREPLY,
      S_CHECK_CATCH,
      S_CLEAR, // port
      S_START_PORT, // port
      S_STOP_PORT, // port
      S_HALT, // port
      /* component statements */
      S_START_COMP, // comp_op
      S_START_COMP_REFD, // comp_op
      S_STOP_COMP, // comp_op
      S_DONE, // comp_op
      S_KILL, // comp_op
      S_KILLED, // comp_op
      /* configuration statements */
      S_CONNECT, // config
      S_MAP, // config
      S_DISCONNECT, // config
      S_UNMAP, // config
      /* timer statements */
      S_START_TIMER, // timer
      S_STOP_TIMER, // timer
      S_TIMEOUT, // timer
      /* verdict statement */
      S_SETVERDICT, // verdictval
      /* SUT statement */
      S_ACTION, // logargs
      /* control statement */
      S_TESTCASE_INSTANCE, // testcase_inst
      S_TESTCASE_INSTANCE_REFD, //execute_refd
      /* TTCN-3 Profiler statements */
      S_START_PROFILER,
      S_STOP_PROFILER,
      /* Conversion statements */
      S_STRING2TTCN, // convert_op
      S_INT2ENUM, // convert_op
      /* update statement */
      S_UPDATE, // update_op
      S_SETSTATE // setstate_op
    };

    enum component_t {
      C_ALL,
      C_ANY
    };

  private:
    statementtype_t statementtype;
    /// The statement block this statement belongs to.
    StatementBlock *my_sb;
    union {
      Assignment *ass; ///< used by S_ASSIGNMENT
      Definition *def; ///< used by S_DEF
      StatementBlock *block; ///< used by S_BLOCK
      LogArguments *logargs; ///< used by S_ACTION, S_LOG, S_STOP_TESTCASE
      struct {
        Reference *portref; /**< NULL means any/all */
        bool anyfrom;
	union {
	  struct {
	    TemplateInstance *sendpar;
	    Value *toclause;
	    union {
	      struct {
		Value *timer;
		bool nowait;
		AltGuards *body;
	      } call;
	      Value *replyval;
	      struct {
		Reference *signature_ref;
		Type *signature; /**< only for caching */
	      } raise;
	    };
	  } s;
	  struct {
	    TemplateInstance *rcvpar;
	    TemplateInstance *fromclause;
	    struct {
	      ValueRedirect *value;
	      ParamRedirect *param;
	      Reference *sender;
	      Reference* index;
	    } redirect;
	    union {
	      TemplateInstance *getreply_valuematch;
	      struct {
		Reference *signature_ref;
		Type *signature; /**< only for caching */
		bool timeout;
		bool in_call, call_has_timer;
	      } ctch; /**< catch */
	    };
	  } r;
	};
      } port_op; /**< used by S_SEND, S_CALL, S_REPLY, S_RAISE,
      S_RECEIVE, S_CHECK_RECEIVE, S_TRIGGER,
      S_GETCALL, S_CHECK_GETCALL, S_GETREPLY, S_CHECK_GETREPLY,
      S_CATCH, S_CHECK_CATCH, S_CHECK, S_CLEAR,
      S_START_PORT, S_STOP_PORT, S_HALT */

      struct {
        Reference *timerref; /**< 0 means any/all */
        Value *value;
        bool any_from;
        Reference* index_redirect;
      } timer_op; ///< used by S_START_TIMER, S_STOP_TIMER, S_TIMEOUT

      struct {
        Value *compref1;
        Reference *portref1;
        Value *compref2;
        Reference *portref2;
        bool translate; // true if a map statement enables translation mode
      } config_op; ///< used by S_CONNECT, S_MAP, S_DISCONNECT, S_UNMAP

      struct {
        Value *compref; /**< if S_STOP_COMP, S_KILL then compref==0
                             means ALL */
        bool any_from;
        Reference* index_redirect;

        union {
          component_t any_or_all;
          /**< used if S_DONE, S_KILLED and compref==0 */
          struct {
            TemplateInstance *donematch;
            ValueRedirect *redirect;
          } donereturn; /**< used if S_DONE and compref!=0 */
          Ref_base *funcinstref; /**< used if S_START_COMP */
          struct {/** used if S_START_COMP_REFD */
            Value *value;
            union {
              ParsedActualParameters *t_list1;
              ActualParList *ap_list2;
            };
          } derefered;
        };
      } comp_op; /**< used by S_START_COMP, S_START_COMP_REFD, S_STOP_COMP,
      S_KILL, S_KILLED, S_DONE */

      struct {
        Value *verdictval;
        LogArguments *logargs;
      } setverdict;

      Value *deactivate; ///< used by S_DEACTIVATE
      AltGuards *ags;    ///< used by S_ALT and S_INTERLEAVE

      struct {
        Ref_pard *tcref;
        Value *timerval;
      } testcase_inst;

      struct {
        Value *value;
        union {
          TemplateInstances *t_list1;
          ActualParList *ap_list2;
        };
        Value *timerval;
      } execute_refd;

      Ref_pard *ref_pard;

      struct {
        Value *value;
        union {
          ParsedActualParameters *t_list1;
          ActualParList *ap_list2;
        };
      } fau_refd; /**< used by S_ACTIVATE_REFD, S_UNKNOWN_INVOKED,
      S_FUNCTION_INVOKED, S_ALTSTEP_INVOKED (function, altstep, or unknown) */

      struct {
        union {
          Value *expr;
          struct {
            bool varinst;
            union {
              Assignment *init_ass;
              Definitions *init_varinst;
            };
            Value *finalexpr;
            Assignment *step;
          } for_stmt;
        };
        StatementBlock *block;
        string *label_next; /**< label for continue, left as 0 if not needed */
        string *il_label_end; /**< label for break in interleave - if needed */
        bool has_cnt; /**< Has continue with effect on the loop */
        bool has_brk; /**< Has break with effect on the loop */
        bool has_cnt_in_ags; /**< Has continue inside embedded ags(AltGuards)*/
        bool iterate_once; /**< do-while loop has constant false expr */
        bool is_ilt; /**< Is compiled specially with ilt_generate_code_... */
      } loop;

      struct {
        IfClauses *ics; /**< if-clauses */
        StatementBlock *elseblock; /**< the last else-block */
	Location *elseblock_location; /**< location of the last else-block */
      } if_stmt;

      struct {
        Value *expr;
        SelectCases *scs;
      } select;
      
      struct {
        Value *expr;
        SelectUnions *sus;
      } select_union;

      struct {
        Value *v;
        Template *t;
        bool gen_restriction_check; // set in chk_return()
      } returnexpr;

      struct {
        Identifier *id;
        size_t stmt_idx; /**< index of the statement within its block */
        string *clabel; /**< C++ label identifier in the generated code */
	bool used;
      } label;

      struct {
        Identifier *id;
        size_t stmt_idx; /**< index of the statement within its block */
        Statement *label; /**< points to the referenced label */
	bool jumps_forward; /**< indicates the direction of the jump */
      } go_to;

      struct {
        Statement *loop_stmt; /**< loop that can be exited by the break*/
        AltGuards *ags; /**< altstep, alt, interleave, call */
      } brk_cnt; // break or continue

      struct {
        Ref_base *ref;
        Value *val;
      } undefstartstop;

      struct { // S_STRING2TTCN
        Value* val;
        Reference* ref;
      } convert_op;
      
      struct {
        Reference* ref;
        WithAttribPath* w_attrib_path;
        ErroneousAttributes* err_attrib;
      } update_op; /**< S_UPDATE */
      
      struct {
        Value* val;
        TemplateInstance* ti;
      } setstate_op; /**< S_SETSTATE */
    };

    Statement(const Statement& p); ///< copy disabled
    Statement& operator=(const Statement& p); ///< assignment disabled
    void clean_up();
  public:
    /** Constructor used by S_ERROR, S_STOP_EXEC and S_REPEAT, S_BREAK,
     *  S_CONTINUE, S_START_PROFILER and S_STOP_PROFILER */
    Statement(statementtype_t p_st);
    /** Constructor used by S_START_UNDEF and S_STOP_UNDEF */
    Statement(statementtype_t p_st, Ref_base *p_ref, Value *p_val);
    /** Constructor used by S_FUNCTION_INSTANCE, S_ALTSTEP_INSTANCE,
     *  S_UNKNOWN_INSTANCE, S_ACTIVATE */
    Statement(statementtype_t p_st, Ref_pard *p_ref);
    /** S_ACTIVATE_REFD , S_FUNCTION_INSTANCE_REFD */
    Statement(statementtype_t p_st, Value *p_derefered_value,
              ParsedActualParameters *p_ap_list);
    /** Constructor used by S_DEF */
    Statement(statementtype_t p_st, Definition *p_def);
    /** Constructor used by S_ASSIGNMENT */
    Statement(statementtype_t p_st, Assignment *p_ass);
    /** Constructor used by S_BLOCK */
    Statement(statementtype_t p_st, StatementBlock *p_block);
    /** Constructor used by S_LOG and S_ACTION */
    Statement(statementtype_t p_st, LogArguments *p_logargs);
    /** Constructor used by S_LABEL and S_GOTO */
    Statement(statementtype_t p_st, Identifier *p_id);
    /** Constructor used by S_IF */
    Statement(statementtype_t p_st, IfClauses *p_ics, StatementBlock *p_block,
              Location *p_loc);
    /** Constructor used by S_SELECT */
    Statement(statementtype_t p_st, Value *p_expr, SelectCases *p_scs);
    /** Constructor used by S_SELECTUNION */
    Statement(statementtype_t p_st, Value *p_expr, SelectUnions *p_sus);
    /** Constructor used by S_FOR */
    Statement(statementtype_t p_st, Definitions *p_defs, Assignment *p_ass,
              Value *p_final, Assignment *p_step, StatementBlock *p_block);
    /** Constructor used by S_WHILE, S_DOWHILE */
    Statement(statementtype_t p_st, Value *p_val, StatementBlock *p_block);
    /** Constructor used by S_ALT and S_INTERLEAVE */
    Statement(statementtype_t p_st, AltGuards *p_ags);
    /** Constructor used by S_RETURN */
    Statement(statementtype_t p_st, Template *p_temp);
    /** Constructor used by S_DEACTIVATE, S_STOP_COMP, S_KILL, S_SETSTATE */
    Statement(statementtype_t p_st, Value *p_val);
    /** Constructor used by S_KILLED */
    Statement(statementtype_t p_st, Value *p_val, bool p_any_from,
              Reference* p_index_redirect);
    /** Constructor used by S_SETVERDICT */
    Statement(statementtype_t p_st, Value *p_val, LogArguments *p_logargs);
    /** Constructor used by S_SEND */
    Statement(statementtype_t p_st, Reference *p_ref,
              TemplateInstance *p_templinst, Value *p_val);
    /** Constructor used by S_CALL */
    Statement(statementtype_t p_st, Reference *p_ref,
              TemplateInstance *p_templinst, Value *p_timerval,
              bool p_nowait, Value *p_toclause, AltGuards *p_callbody);
    /** Constructor used by S_REPLY */
    Statement(statementtype_t p_st, Reference *p_ref,
              TemplateInstance *p_templinst, Value *p_replyval,
              Value *p_toclause);
    /** Constructor used by S_RAISE */
    Statement(statementtype_t p_st, Reference *p_ref,
              Reference *p_sig, TemplateInstance *p_templinst,
              Value *p_toclause);
    /** Constructor used by S_RECEIVE, S_CHECK_RECEIVE and
     *  S_TRIGGER. p_ref==0 means any port. */
    Statement(statementtype_t p_st, Reference *p_ref, bool p_anyfrom,
              TemplateInstance *p_templinst, TemplateInstance *p_fromclause,
              ValueRedirect *p_redirectval, Reference *p_redirectsender,
              Reference* p_redirectindex);
    /** Constructor used by S_GETCALL and S_CHECK_GETCALL. p_ref==0
     *  means any port. */
    Statement(statementtype_t p_st, Reference *p_ref, bool p_anyfrom,
              TemplateInstance *p_templinst, TemplateInstance *p_fromclause,
              ParamRedirect *p_redirectparam, Reference *p_redirectsender,
              Reference* p_redirectindex);
    /** Constructor used by S_GETREPLY and S_CHECK_GETREPLY. p_ref==0
     *  means any port. */
    Statement(statementtype_t p_st, Reference *p_ref, bool p_anyfrom,
              TemplateInstance *p_templinst, TemplateInstance *p_valuematch,
              TemplateInstance *p_fromclause,
              ValueRedirect *p_redirectval, ParamRedirect *p_redirectparam,
              Reference *p_redirectsender, Reference* p_redirectindex);
    /** Constructor used by S_CATCH and S_CHECK_CATCH. p_ref==0 means
     *  any port. */
    Statement(statementtype_t p_st, Reference *p_ref,  bool p_anyfrom,
              Reference *p_sig, TemplateInstance *p_templinst,
              bool p_timeout, TemplateInstance *p_fromclause,
              ValueRedirect *p_redirectval, Reference *p_redirectsender,
              Reference* p_redirectindex);
    /** Constructor used by S_CHECK. p_ref==0 means any port. */
    Statement(statementtype_t p_st, Reference *p_ref, bool p_anyfrom,
              TemplateInstance *p_fromclause, Reference *p_redirectsender,
              Reference* p_redirectindex);
    /** Constructor used by S_CLEAR, S_START_PORT and S_STOP_PORT.
     *  p_ref==0 means all port. S_STOP_TIMER (p_ref==0: all timer). */
    Statement(statementtype_t p_st, Reference *p_ref);
    /** Constructor used by S_TIMEOUT (p_ref==0: any timer)*/
    Statement(statementtype_t p_st, Reference *p_ref, bool p_any_from,
              Reference* p_redirectindex);
    /** Constructor used by S_START_COMP */
    Statement(statementtype_t p_st, Value *p_compref, Ref_pard *p_funcinst);
    /** Constructor used by S_START_COMP_REFD */
    Statement(statementtype_t p_st, Value *p_compref, Value *p_derefered_value,
              ParsedActualParameters *p_ap_list);
    /** Constructor used by S_DONE */
    Statement(statementtype_t p_st, Value *p_compref,
              TemplateInstance *p_donematch, ValueRedirect *p_redirect,
              Reference* p_index_redirect, bool p_any_from);
    /** Constructor used by S_DONE, S_KILLED */
    Statement(statementtype_t p_st, component_t p_anyall);
    /** Constructor used by S_CONNECT, S_DISCONNECT, S_MAP, S_UNMAP */
    Statement(statementtype_t p_st,
              Value *p_compref1, Reference *p_portref1,
              Value *p_compref2, Reference *p_portref2);
    /** Constructor used by S_TESTCASE_INSTANCE */
    Statement(statementtype_t p_st, Ref_pard *p_ref, Value *p_val);
    /** Constructor used by S_ACTIVATE_REFD */
    Statement(statementtype_t p_st, Value *p_derefered_value,
               TemplateInstances *p_ap_list, Value *p_val);
    /** Constructor used by S_STRING2TTCN, S_INT2ENUM */
    Statement(statementtype_t p_st, Value* p_val, Reference* p_ref);
    /** Constructor used by S_UPDATE */
    Statement(statementtype_t p_st, Reference* p_ref, MultiWithAttrib* p_attrib);
    /** Constructor used by S_SETSTATE */
    Statement(statementtype_t p_st, Value* p_val, TemplateInstance* p_ti);
    virtual ~Statement();
    virtual Statement* clone() const;
    virtual void dump(unsigned int level) const;
    statementtype_t get_statementtype() const { return statementtype; }
    StatementBlock *get_my_sb() const { return my_sb; }
    size_t get_my_sb_index() const;
    const char *get_stmt_name() const;
    const Identifier& get_labelid() const;
    /** Returns whether the label is used by goto statements, applicable if
     * \a this is a label (S_LABEL) */
    bool label_is_used() const;
    /** Returns whether the goto statement jumps forward in the statement block,
     * applicable if \a this is a goto (S_GOTO) */
    bool goto_jumps_forward() const;
  private:
    /** Returns the c++ identifier of the label, applicable if \a this is a
     * label (S_LABEL) */
    const string& get_clabel();
  public:
    /** Applicable if \a this is a definition (S_DEF) */
    Definition *get_def() const;
    /** Applicable if \a this is an alt (S_ALT) or interleave (S_INTERLEAVE) */
    AltGuards *get_ags() const;
    /** Applicable if \a this is a block (S_BLOCK) or a loop
     * (S_FOR, S_WHILE, S_DOWHILE) */
    StatementBlock *get_block() const;
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_fullname(const string& p_fullname);
    /** pass down to embedded statementblocks */
    void set_my_sb(StatementBlock *p_sb, size_t p_index);
    void set_my_def(Definition *p_def);
    /** used in S_REPEAT */
    void set_my_ags(AltGuards *p_ags);
    /** used in S_BREAK and S_CONTINUE */
    void set_my_laic_stmt(AltGuards *p_ags, Statement *p_loop_stmt);
    /** Returns true if the statement following \a this in a statement
     *  block will never be executed sequentially after \a this. */
    bool is_terminating() const;
    StatementBlock::returnstatus_t has_return() const;
    bool is_receiving_stmt() const;
    bool has_receiving_stmt() const;
    /** Indicates whether the C++ equivalent of the statement can
     *  return ALT_REPEAT. Applicable to receiving statements only. */
    bool can_repeat() const;
    /* checking functions */
    void chk();
    /** checks whether the statement (including all embedded statements) are
     * allowed in an interleaved construct */
    void chk_allowed_interleave();
    
    /** Checks the index redirect in a port, timer or component array operation.
      *
      * @param p_index_ref reference to variable or parameter for storing the index
      * @param p_array_dims dimensions of the port, timer or component array
      * @param p_any_from presence of the 'any from' clause before the array
      * @param p_array_type string containing "port", "timer" or "component"
      * (used in error messages)
      *
      * @note This function is also called by expressions containing timer or
      * component operations in the Value class. */
    static void chk_index_redirect(Reference* p_index_ref,
      ArrayDimensions* p_array_dims, bool p_any_from, const char* p_array_type);
  private:
    void chk_start_undef();
    void chk_stop_undef();
    void chk_unknown_instance();
    void chk_unknown_invoke();
    void chk_assignment();
    void chk_function();
    void chk_block();
    /* checks log, action */
    void chk_log_action(LogArguments *logargs);
    void chk_goto();
    void chk_if();
    void chk_select();
    void chk_select_union();
    void chk_for();
    void chk_while();
    void chk_do_while();
    void chk_break();
    void chk_continue();
    void chk_alt();
    void chk_repeat();
    void chk_interleave();
    void chk_altstep();
    void chk_return();
    void chk_activate();
    void chk_activate_refd();
    void chk_deactivate();
    void chk_send();
    void chk_call();
    void chk_reply();
    void chk_raise();
    /* checks receive, check-receive trigger */
    void chk_receive();
    /* checks getcall, check-getcall */
    void chk_getcall();
    /* checks getreply, check-getreply */
    void chk_getreply();
    /* checks catch, check-catch */
    void chk_catch();
    void chk_check();
    void chk_clear();
    /** checks start port, stop port, halt */
    void chk_start_stop_port();
    void chk_start_comp();
    void chk_start_comp_refd();
    /* checks stop comp, kill, killed */
    void chk_stop_kill_comp();
    void chk_done();
    void chk_killed();
    /* checks connect and disconnect */
    void chk_connect();
    /* checks map and unmap */
    void chk_map();
    void chk_start_timer();
    void chk_stop_timer();
    void chk_timer_timeout();
    void chk_setverdict();
    void chk_execute();
    void chk_execute_refd();

    /** Checks whether \a p_ref points to a port, port parameter or port array.
     *  If \a p_ref refers to a port array the array indices within \a
     *  p_ref are also checked. If \a p_any_from is true, then the reference
     *  must point to a port array, otherwise it must point to a port.
     *  Returns a pointer to the port type if available or NULL otherwise. */
    Type *chk_port_ref(Reference *p_ref, bool p_any_from = false);
    /** Checks the `to' clause of a port operation. Field
     *  port_op.s.toclause shall be a component reference (or an
     *  address) depending on the extension attributes of \a
     *  port_type. */
    void chk_to_clause(Type *port_type);
    /** Checks the `from' clause and `sender' redirect of a port
     *  operation.  Type of field port_op.r.fromclause and
     *  port_op.r.redirect.sender shall be a component type (or
     *  address) depending on the extension attributes of \a
     *  port_type. */
    void chk_from_clause(Type *port_type);
    /** Checks the response and exception handling part of a call
     *  operation.  Arguments \a port_type \a signature point to the
     *  port type and signature used in the call operation. */
    void chk_call_body(Type *port_type, Type *signature);
    /** Determines and returns the type of the incoming or outgoing message or
     *  signature based on a template instance \a p_ti. */
    static Type *get_msg_sig_type(TemplateInstance *p_ti);
    /** Checks the variable reference of a sender redirect.  The type
     *  of the variable (or NULL in case of non-existent sender clause
     *  or error) is returned.  The type of the variable is also
     *  checked: it shall be a component type or \a address_type (if
     *  it is not NULL). */
    Type *chk_sender_redirect(Type *address_type);
    /** Checks whether \a p_ref points to a signature. The type
     *  describing the respective signature is returned or NULL in
     *  case of error. */
    static Type *chk_signature_ref(Reference *p_ref);
    /** Checks whether \a p_ref points to a timer, timer parameter or timer array.
     *  If \a p_ref refers to a timer array the array indices within
     *  \a p_ref are also checked. If \a p_any_from is true, then the reference
     *  must point to a port array, otherwise it must point to a port.*/
    static void chk_timer_ref(Reference *p_ref, bool p_any_from = false);
    /** Checks whether \a p_val is a component or component array reference
     *  (i.e. its type is a component or component array type).
     *  Returns a pointer to the component or component array type if available
     *  or NULL otherwise. Flags \a allow_mtc and \a allow_system indicate whether
     *  the mtc or system component reference is acceptable in this context.
     *  Flag p_any_from indicates whether the 'any from' clause was used on \a p_val
     *  (in which case it must be a component array) or not (in which case it
     *  must be a component). */
    Type *chk_comp_ref(Value *p_val, bool allow_mtc, bool allow_system, bool p_any_from = false);
    /** Checks an endpoint for a port connection or mapping. Parameter
     *  \a p_compref is a component reference, \a p_portref refers to
     *  a port within the corresponding component type.  A pointer to
     *  the referred port type (or NULL in case of error) is
     *  returned. */
    Type *chk_conn_endpoint(Value *p_compref, Reference *p_portref,
                            bool allow_system);
    void chk_string2ttcn();
    void chk_int2enum();
    void chk_update();
    void chk_setstate();
  public:
    /** Sets the code section selector of all embedded values and
     *  templates to \a p_code_section. */
    void set_code_section(GovernedSimple::code_section_t p_code_section);
    char* generate_code(char *str, char*& def_glob_vars, char*& src_glob_vars);
    void generate_code_expr(expression_struct *expr);
    void ilt_generate_code(ILT *ilt);
    
    /** Generates code for an index redirect. (A new class is generated for each
      * redirect, inheriting the Index_Redirect class.)
      *
      * @note This function is also called by expressions containing timer or
      * component operations in the Value class. */
    static void generate_code_index_redirect(expression_struct* expr,
      Reference* p_ref, Scope* p_scope);

    /** Needed by implicit omit. Pushes attrib path down to definitions
     */
    virtual void set_parent_path(WithAttribPath* p_path);
  private:
    char *generate_code_standalone(char *str);
    /** used for function and altstep instances */
    char *generate_code_funcinst(char *str);
    char *generate_code_invoke(char *str);
    char *generate_code_block(char *str, char*& def_glob_vars, char*& src_glob_vars);
    char *generate_code_log(char *str);
    char* generate_code_string2ttcn(char *str);
    char *generate_code_testcase_stop(char *str);
    char *generate_code_label(char *str);
    char *generate_code_goto(char *str);
    char *generate_code_if(char *str, char*& def_glob_vars, char*& src_glob_vars);
    char *generate_code_select(char *str, char*& def_glob_vars, char*& src_glob_vars);
    char *generate_code_select_union(char *str, char*& def_glob_vars, char*& src_glob_vars);
    char *generate_code_for(char *str, char*& def_glob_vars, char*& src_glob_vars);
    char *generate_code_while(char *str, char*& def_glob_vars, char*& src_glob_vars);
    char *generate_code_dowhile(char *str, char*& def_glob_vars, char*& src_glob_vars);
    char *generate_code_break(char *str);
    char *generate_code_continue(char *str);
    char *generate_code_repeat(char *str);
    char *generate_code_interleave(char *str, char*& def_glob_vars, char*& src_glob_vars);
    void ilt_generate_code_interleave(ILT *ilt);
    void ilt_generate_code_alt(ILT *ilt);
    void ilt_generate_code_receiving(ILT *ilt);
    void ilt_generate_code_def(ILT *ilt);
    void ilt_generate_code_if(ILT *ilt);
    void ilt_generate_code_select(ILT *ilt);
    void ilt_generate_code_select_union(ILT *ilt);
    void ilt_generate_code_call(ILT *ilt);
    void ilt_generate_code_for(ILT *ilt);
    void ilt_generate_code_while(ILT *ilt);
    void ilt_generate_code_dowhile(ILT *ilt);
    char *generate_code_return(char *str);
    char *generate_code_activate(char *str);
    char *generate_code_activate_refd(char *str);
    char *generate_code_deactivate(char *str);
    char *generate_code_send(char *str);
    char *generate_code_call(char *str, char*& def_glob_vars, char*& src_glob_vars);
    char *generate_code_reply(char *str);
    char *generate_code_raise(char *str);
    char *generate_code_portop(char *str, const char *opname);
    char *generate_code_startcomp(char *str);
    char *generate_code_startcomp_refd(char *str);
    char *generate_code_compop(char *str, const char *opname);
    char *generate_code_configop(char *str, const char *opname);
    char *generate_code_starttimer(char *str);
    char *generate_code_stoptimer(char *str);
    char *generate_code_setverdict(char *str);
    char *generate_code_action(char *str);
    char *generate_code_testcaseinst(char *str);
    char *generate_code_execute_refd(char *str);
    char* generate_code_update(char *str, char*& def_glob_vars, char*& src_glob_vars);
    char* generate_code_setstate(char *str);
    /** used for receive, check-receive, trigger */
    void generate_code_expr_receive(expression_struct *expr,
      const char *opname);
    /** used for getcall, check-getcall */
    void generate_code_expr_getcall(expression_struct *expr,
      const char *opname);
    /** used for getreply, check-getreply */
    void generate_code_expr_getreply(expression_struct *expr,
      const char *opname);
    /** used for catch, check-catch */
    void generate_code_expr_catch(expression_struct *expr);
    void generate_code_expr_check(expression_struct *expr);
    void generate_code_expr_done(expression_struct *expr);
    void generate_code_expr_killed(expression_struct *expr);
    void generate_code_expr_timeout(expression_struct *expr);
    /** Generates the C++ equivalent of \a port_op.s.sendpar into \a expr.
     *  When \a sendpar contains a simple specific value the value -> template
     * conversion is eliminated for better run-time performance. */
    void generate_code_expr_sendpar(expression_struct *expr);
    void generate_code_expr_fromclause(expression_struct *expr);
    void generate_code_expr_senderredirect(expression_struct *expr);
    /** Creates the string equivalent of port reference \a p_ref and
     *  appends it to \a expr->expr. Used in configuration operations
     *  when the component type cannot be determined from the
     *  component reference. */
    static void generate_code_portref(expression_struct *expr,
      Reference *p_ref);
    char* generate_code_int2enum(char* str);
  };

  /**
   * Class to store a VariableAssignment or TemplateAssignment.
   * Class is entirely unrelated to Common::Assignment and Asn::Assignment.
   */
  class Assignment : public Node, public Location {
  public:
    enum asstype_t {
      ASS_UNKNOWN,  ///< unknown assignment
      ASS_VAR,      ///< variable assignment
      ASS_TEMPLATE, ///< template assignment
      ASS_ERROR     ///< erroneous
    };

  private:
    asstype_t asstype;
    Reference *ref; ///< reference to the "left hand side"
    union {
      Value    *val;   ///< RHS for a variable assignment
      Template *templ; ///< RHS for a template assignment
    };
    /** true if the LHS is mentioned in the RHS */
    bool self_ref;
    /** template restriction on ref, set in chk() for faster code generation */
    template_restriction_t template_restriction;
    /** set in chk(), used by code generation */
    bool gen_restriction_check;

    Assignment(const Assignment& p);
    Assignment& operator=(const Assignment& p);
  public:
    /** Creates a new template assignment. */
    Assignment(Reference *p_ref, Template *p_templ);
    /** Creates a new variable assignment. */
    Assignment(Reference *p_ref, Value *p_val);
    virtual ~Assignment();
    virtual Assignment *clone() const;
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_fullname(const string& p_fullname);
    virtual void dump(unsigned int level) const;
  private:
    void chk_unknown_ass();
    void chk_var_ass();
    void chk_template_ass();
    void chk_template_restriction();
  public:
    void chk();
    /** Sets the code section selector of all embedded values and templates
     * to \a p_code_section. */
    void set_code_section(GovernedSimple::code_section_t p_code_section);
    char *generate_code(char *str);
  };

  /**
   * Class to store a ParamAssignment.
   */
  class ParamAssignment : public Node, public Location {
  private:
    Identifier *id;
    Reference *ref;
    /** indicates whether the redirected parameter should be decoded */
    bool decoded;
    /** encoding format for decoded universal charstring parameter redirects */
    Value* str_enc;
    /** pointer to the type the redirected parameter is decoded into, if the
      * '@decoded' modifier is used (not owned) */
    Type* dec_type;

    ParamAssignment(const ParamAssignment& p);
    ParamAssignment& operator=(const ParamAssignment& p);
  public:
    ParamAssignment(Identifier *p_id, Reference *p_ref, bool p_decoded,
      Value* p_str_enc);
    virtual ~ParamAssignment();
    virtual ParamAssignment* clone() const;
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_fullname(const string& p_fullname);
    const Identifier& get_id() const { return *id; }
    Reference *get_ref() const;
    Reference *steal_ref();
    bool is_decoded() const { return decoded; }
    Value* get_str_enc() const;
    Value* steal_str_enc();
    void set_dec_type(Type* p_dec_type) { dec_type = p_dec_type; }
    Type* get_dec_type() const { return dec_type; }
  };

  /**
   * Class to store a ParamAssignmentList
   */
  class ParamAssignments : public Node {
  private:
    vector<ParamAssignment> parasss;

    ParamAssignments(const ParamAssignments& p);
    ParamAssignments& operator=(const ParamAssignments& p);
  public:
    ParamAssignments() : Node() { }
    virtual ~ParamAssignments();
    virtual ParamAssignments* clone() const;
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_fullname(const string& p_fullname);
    void add_parass(ParamAssignment *p_parass);
    size_t get_nof_parasss() const { return parasss.size(); }
    ParamAssignment *get_parass_byIndex(size_t p_i) const
      { return parasss[p_i]; }
  };

  /**
   * Class to represent a variable or notused-symbol in
   * ParamAssignmentList of port redirect stuff.
   */
  class VariableEntry : public Node, public Location {
  private:
    Reference *ref; /**< varref or notused if NULL. */
    /** indicates whether the redirected parameter should be decoded */
    bool decoded;
    /** encoding format for decoded universal charstring parameter redirects
      * (is only set when the AssignmentList is converted to a VariableList) */
    Value* str_enc;
    /** pointer to the type the redirected parameter is decoded into, if the
      * '@decoded' modifier is used
      * (is only set when the AssignmentList is converted to a VariableList,
      * not owned) */
    Type* dec_type;

    VariableEntry(const VariableEntry& p);
    VariableEntry& operator=(const VariableEntry& p);
  public:
    /** Creates a notused entry */
    VariableEntry() : Node(), Location(), ref(0), decoded(false), str_enc(NULL), dec_type(NULL) { }
    VariableEntry(Reference *p_ref);
    VariableEntry(Reference* p_ref, bool p_decoded, Value* p_str_enc, Type* p_dec_type);
    virtual ~VariableEntry();
    virtual VariableEntry* clone() const;
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_fullname(const string& p_fullname);
    Reference *get_ref() const { return ref; }
    bool is_decoded() const { return decoded; }
    Value* get_str_enc() const { return str_enc; }
    Type* get_dec_type() const { return dec_type; }
  };

  /**
   * Class to store a VariableEntries
   */
  class VariableEntries : public Node {
  private:
    vector<VariableEntry> ves;

    VariableEntries(const VariableEntries& p);
    VariableEntries& operator=(const VariableEntries& p);
  public:
    VariableEntries() : Node() { }
    virtual ~VariableEntries();
    virtual VariableEntries* clone() const;
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_fullname(const string& p_fullname);
    void add_ve(VariableEntry *p_ve);
    size_t get_nof_ves() const { return ves.size(); }
    VariableEntry *get_ve_byIndex(size_t p_i) const { return ves[p_i]; }
  };

  /**
   * Class to represent ParamAssignmentList in a param redirect of
   * getcall/getreply port operation.
   */
  class ParamRedirect : public Node, public Location {
  public:
    enum parredirtype_t {
      P_ASS, /**< AssignmentList */
      P_VAR /**< VariableList */
    };
  private:
    parredirtype_t parredirtype;
    union {
      ParamAssignments *parasss;
      VariableEntries *ves;
    };

    ParamRedirect(const ParamRedirect& p);
    ParamRedirect& operator=(const ParamRedirect& p);
  public:
    ParamRedirect(ParamAssignments *p_parasss);
    ParamRedirect(VariableEntries *p_ves);
    virtual ~ParamRedirect();
    virtual ParamRedirect* clone() const;
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_fullname(const string& p_fullname);
    /** Performs some trivial checks on the variable references.
     * Used when the signature cannot be determined in a getcall or getreply
     * operation because of an error. */
    void chk_erroneous();
    /** Performs the checks on the param redirect clause. Parameter \a p_sig
     * points to the respective signature. Flag \a is_out is true if the
     * redirect belongs to a getreply operation (i.e. it may refer to out/inout
     * parameters) and false in case of getcall. */
    void chk(Type *p_sig, bool is_out);
  private:
    /** Helper function for \a chk(). Used when AssignmentList is selected.
     * If the redirect is correct it converts the AssignmentList to an
     * equivalent VariableList for easier code generation. */
    void chk_parasss(Type *p_sig, SignatureParamList *p_parlist, bool is_out);
    /** Helper function for \a chk(). Used when VariableList is selected. */
    void chk_ves(Type *p_sig, SignatureParamList *p_parlist, bool is_out);
    /** Checks whether the reference \a p_ref points to a variable of type
     * \a p_type. */
    static void chk_variable_ref(Reference *p_ref, Type *p_type);
  public:
    /** Sets the code section selector of all embedded values and templates
     * to \a p_code_section. */
    void set_code_section(GovernedSimple::code_section_t p_code_section);
    void generate_code(expression_struct_t *expr, TemplateInstance* matched_ti,
      bool is_out);
    /** generates a new redirect class, inherited from the signature type's
      * original redirect class, which extends its functionality to also
      * decode the redirected parameters that have the '@decoded' modifier */
    char* generate_code_decoded(char* str, TemplateInstance* matched_ti,
      const char* tmp_id, bool is_out);
    /** returns true if at least one of the parameter redirects has the 
      * '@decoded' modifier */
    bool has_decoded_modifier() const;
  };
  
  /** 
    * Class for storing a single element of a value redirect
    * Each of these elements can be:
    * - a lone variable reference (in this case the whole value is redirected to
    *   the referenced variable), or
    * - the assignment of a field or a field's sub-reference to a variable
    *   (in this case one of the value's fields, or a sub-reference of one of the
    *   fields is redirected to the referenced variable; this can only happen if
    *   the value is a record or set).
    */
  class SingleValueRedirect : public Node, public Location {    
  private:
    /** reference to the variable the value is redirected to */
    Reference* var_ref;
    /** indicates which part (record field or array element) of the value is 
      * redirected (optional) */
    FieldOrArrayRefs* subrefs;
    /** indicates whether the redirected field or element should be decoded 
      * (only used if subrefs is not null) */
    bool decoded;
    /** encoding format for decoded universal charstring value redirects
      * (only used if subrefs is not null and decoded is true) */
    Value* str_enc;
    /** pointer to the type the redirected field or element is decoded into
      * (only used if subrefs is not null and decoded is true), not owned*/
    Type* dec_type;
    
    SingleValueRedirect(const SingleValueRedirect&);
    SingleValueRedirect& operator=(const SingleValueRedirect&);
  public:
    SingleValueRedirect(Reference* p_var_ref);
    SingleValueRedirect(Reference* p_var_ref, FieldOrArrayRefs* p_subrefs,
      bool p_decoded, Value* p_str_enc);
    virtual ~SingleValueRedirect();
    virtual SingleValueRedirect* clone() const;
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_fullname(const string& p_fullname);
    Reference *get_var_ref() const { return var_ref; }
    FieldOrArrayRefs *get_subrefs() const { return subrefs; }
    bool is_decoded() const { return decoded; }
    Value* get_str_enc() const { return str_enc; }
    void set_dec_type(Type* p_dec_type) { dec_type = p_dec_type; }
    Type* get_dec_type() const { return dec_type; }
  };
  
  /**
    * Class for storing a value redirect
    */
  class ValueRedirect : public Node, public Location {
    /** list of single value redirect elements */
    vector<SingleValueRedirect> v;
    /** pointer to the type of the redirected value, not owned */
    Type* value_type;
    
    ValueRedirect(const ValueRedirect&);
    ValueRedirect& operator=(const ValueRedirect&);
  public:
    ValueRedirect(): v(), value_type(NULL) { }
    virtual ~ValueRedirect();
    virtual ValueRedirect* clone() const;
    virtual void set_my_scope(Scope* p_scope);
    virtual void set_fullname(const string& p_fullname);
    void set_code_section(GovernedSimple::code_section_t p_code_section);
    void add(SingleValueRedirect* ptr);
    /** Attempts to identify the type of the redirected value. Only those single
      * redirects are checked, which redirect the whole value, not just a field.
      * If multiple whole-value-redirects of separate types are found, then an
      * error is displayed. */
    Type* get_type() const;
    /** Checks Runtime1 restrictions and returns false if they are not met. */
    bool chk_RT1_restrictions() const;
    /** Performs semantic analysis on the value redirect without knowing the
      * type of the redirected value. Called when the value's type cannot be
      * determined or is erroneous. */
    void chk_erroneous();
    /** Performs the full semantic analysis on the value redirect.
      * @param p_type the type of the redirected value */
    void chk(Type* p_type);
    /** Generates code for the value redirect in the specified expression
      * structure. A new class is generated for every value redirect, which
      * handles the redirecting.
      * @param matched_ti the template instance used for matching the redirected
      * value (if NULL, then the template instance is an 'any value' template) */
    void generate_code(expression_struct* expr, TemplateInstance* matched_ti);
    /** returns true if at least one of the value redirects has the 
      * '@decoded' modifier*/
    bool has_decoded_modifier() const;
  };

  /**
   * Class to represent a LogArgument.
   */
  class LogArgument : public Node, public Location {
  public:
    enum logargtype_t {
      L_UNDEF, /**< undefined (set during parsing) */
      L_ERROR, /**< erroneous */
      L_TI, /**< template instance */
      L_VAL, /**< literal value or expression */
      L_MATCH,  /**< match expression */
      L_MACRO, /**< macro value (%something) */
      L_REF, /**< reference */
      L_STR /**< literal character string */
    };

  private:
    logargtype_t logargtype;
    union {
      TemplateInstance *ti; ///< used by L_UNDEF and L_TI
      Value *val;           ///< used by L_VAL, L_MATCH, L_MACRO
      Ref_base *ref;        ///< used by L_REF
      string *cstr;         ///< used by L_STR
    };

    LogArgument(const LogArgument& p);
    LogArgument& operator=(const LogArgument& p);
  public:
    LogArgument(TemplateInstance *p_ti);
    virtual ~LogArgument();
    virtual LogArgument *clone() const;
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_fullname(const string& p_fullname);
    logargtype_t get_type() const { return logargtype; }
    const string&     get_str() const;
    Value            *get_val() const;
    Ref_base         *get_ref() const;
    TemplateInstance *get_ti () const;
    /** Appends the string \a p_str to \a cstr. Applicable only if
     * \a logargtype is L_STR. */
    void append_str(const string& p_str);
    void chk();
    virtual void dump(unsigned int level) const;
  private:
    void chk_ref();
    void chk_val();
  public:
    /** Sets the code section selector of all embedded values and
     *  templates to \a p_code_section. */
    void set_code_section(GovernedSimple::code_section_t p_code_section);
    char *generate_code_log(char *str);
    void chk_recursions(ReferenceChain& refch);
    bool has_single_expr();
    void generate_code_expr(expression_struct *expr);
  };

  /**
   * Class to represent LogArgumentList.
   */
  class LogArguments : public Node {
  private:
    vector<LogArgument> logargs;

    LogArguments(const LogArguments& p);
    LogArguments& operator=(const LogArguments& p);
  public:
    LogArguments() : Node() { }
    virtual ~LogArguments();
    virtual LogArguments *clone() const;
    void add_logarg(LogArgument *p_logarg);
    size_t get_nof_logargs() const { return logargs.size(); }
    LogArgument *get_logarg_byIndex(size_t p_i) const { return logargs[p_i]; }
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_fullname(const string& p_fullname);
    void chk();
    /** Joins adjacent strings into one LogArgument for more efficient
     * code generation. */
    void join_strings();
    /** Sets the code section selector of all embedded values and
     *  templates to \a p_code_section. */
    void set_code_section(GovernedSimple::code_section_t p_code_section);
    char *generate_code(char *str);
    void chk_recursions(ReferenceChain& refch);
    bool has_single_expr();
    void generate_code_expr(expression_struct *expr);
  };

  /**
   * Class to represent an if-clause: the condition and the statement
   * block. Note that the else block is kept by Statement::if_stmt.
   */
  class IfClause : public Node, public Location {
  private:
    Value *expr; /**< conditional expression */
    StatementBlock *block;

    IfClause(const IfClause& p);
    IfClause& operator=(const IfClause& p);
  public:
    IfClause(Value *p_expr, StatementBlock *p_block);
    virtual ~IfClause();
    virtual IfClause *clone() const;
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_fullname(const string& p_fullname);
    Value *get_expr() const { return expr; }
    StatementBlock *get_block() const { return block; }
    /* checking functions */
    bool has_receiving_stmt() const;
    void chk(bool& unreach);
    /** Sets the code section selector of all embedded values and
     *  templates to \a p_code_section. */
    void set_code_section(GovernedSimple::code_section_t p_code_section);
    char* generate_code(char *str, char*& def_glob_vars, char*& src_glob_vars,
      size_t& blockcount, bool& unreach, bool& eachfalse);
    void ilt_generate_code(ILT *ilt, const char *end_label, bool& unreach);

    /** Needed by implicit omit. Pushes attrib path down to definitions
     */
    virtual void set_parent_path(WithAttribPath* p_path);
    virtual void dump(unsigned int level) const;
  };

  /**
   * Class to represent IfClauseList, a chain of if-else-if-else-if
   */
  class IfClauses : public Node {
  private:
    vector<IfClause> ics;

    IfClauses(const IfClauses& p);
    IfClauses& operator=(const IfClauses& p);
  public:
    IfClauses() : Node() { }
    virtual ~IfClauses();
    virtual IfClauses* clone() const;
    /// Called to add an "else-if" to a chain of "else-if"s
    void add_ic(IfClause *p_ic);
    /// Called to add the first "if" to a chain of "else-if"s
    void add_front_ic(IfClause *p_ic);
    size_t get_nof_ics() const {return ics.size();}
    IfClause *get_ic_byIndex(size_t p_i) const {return ics[p_i];}
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_fullname(const string& p_fullname);
    void set_my_sb(StatementBlock *p_sb, size_t p_index);
    void set_my_def(Definition *p_def);
    void set_my_ags(AltGuards *p_ags);
    void set_my_laic_stmt(AltGuards *p_ags, Statement *p_loop_stmt);
    StatementBlock::returnstatus_t has_return(StatementBlock *elseblock) const;
    bool has_receiving_stmt() const;
    /* checking functions */
    void chk(bool& unreach);
    /** checks whether all embedded statements are allowed in an interleaved
     * construct */
    void chk_allowed_interleave();
    /** Sets the code section selector of all embedded values and
     *  templates to \a p_code_section. */
    void set_code_section(GovernedSimple::code_section_t p_code_section);
    char* generate_code(char *str, char*& def_glob_vars, char*& src_glob_vars,
      size_t& blockcount, bool& unreach, bool& eachfalse);
    void ilt_generate_code(ILT *ilt, const char *end_label, bool& unreach);

    /** Needed by implicit omit. Pushes attrib path down to definitions
     */
    virtual void set_parent_path(WithAttribPath* p_path);
    virtual void dump(unsigned int level) const;
  };

  /**
   * Class to represent a select-case: the template instance list and
   * the statement block.
   */
  class SelectCase : public Node, public Location {
  private:
    TemplateInstances *tis;
    StatementBlock *block;

    SelectCase(const SelectCase& p);
    SelectCase& operator=(const SelectCase& p);
  public:
    /** tis==0 means "else" case */
    SelectCase(TemplateInstances *p_tis, StatementBlock *p_block);
    virtual ~SelectCase();
    virtual SelectCase* clone() const;
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_fullname(const string& p_fullname);
    TemplateInstances* get_tis() const { return tis; }
    StatementBlock *get_block() const { return block; }
    /* checking functions */
    void chk(Type *p_gov, bool& unreach);
    /** Sets the code section selector of all embedded values and
     *  templates to \a p_code_section. */
    void set_code_section(GovernedSimple::code_section_t p_code_section);
    char* generate_code_if(char *str, const char *tmp_prefix,
                           const char *expr_name, size_t idx, bool& unreach);
    char* generate_code_case(char *str, char*& def_glob_vars, char*& src_glob_vars,
      bool& else_branch, vector<const Int>& used_numbers);
    char* generate_code_stmt(char *str, char*& def_glob_vars, char*& src_glob_vars,
      const char *tmp_prefix, size_t idx, bool& unreach);
    void ilt_generate_code_stmt(ILT *ilt, const char *tmp_prefix,
                                size_t idx, bool& unreach);

    /** Needed by implicit omit. Pushes attrib path down to definitions
     */
    virtual void set_parent_path(WithAttribPath* p_path);
  };

  /**
   * Class to represent SelectCaseList.
   */
  class SelectCases : public Node {
  private:
    vector<SelectCase> scs;

    SelectCases(const SelectCases& p);
    SelectCases& operator=(const SelectCases& p);
  public:
    SelectCases() : Node() { }
    virtual ~SelectCases();
    virtual SelectCases* clone() const;
    void add_sc(SelectCase *p_sc);
    size_t get_nof_scs() const {return scs.size();}
    SelectCase *get_sc_byIndex(size_t p_i) const {return scs[p_i];}
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_fullname(const string& p_fullname);
    void set_my_sb(StatementBlock *p_sb, size_t p_index);
    void set_my_def(Definition *p_def);
    void set_my_ags(AltGuards *p_ags);
    void set_my_laic_stmt(AltGuards *p_ags, Statement *p_loop_stmt);
    StatementBlock::returnstatus_t has_return() const;
    bool has_receiving_stmt() const;
    bool can_generate_switch() const;
    /* checking functions */
    /** p_gov is the governor type of select expression */
    void chk(Type *p_gov);
    /** checks whether all embedded statements are allowed in an interleaved
     * construct */
    void chk_allowed_interleave();
    /** Sets the code section selector of all embedded values and
     *  templates to \a p_code_section. */
    void set_code_section(GovernedSimple::code_section_t p_code_section);
    char *generate_code(char *str, char*& def_glob_vars, char*& src_glob_vars,
      const char *tmp_prefix, const char *expr_name);   
    void ilt_generate_code(ILT *ilt, const char *tmp_prefix,
                           const char *expr_init, const char *head_expr,
                           const char *expr_name);
    /** generates code with switch c++ statement only for integer 
     *  compatible types*/
    char *generate_code_switch(char *str, char*& def_glob_vars, char*& src_glob_vars, const char *expr_name);
    void ilt_generate_code_switch(ILT *ilt, const char *expr_init, const char *head_expr, const char *expr_name);
    

    /** Needed by implicit omit. Pushes attrib path down to definitions
     */
    virtual void set_parent_path(WithAttribPath* p_path);
  };
  
   /**
   * Class to represent a select-union branch: the identifier list and
   * the statement block.
   */
  class SelectUnion : public Node, public Location {
  private:
    vector<Identifier> ids;
    StatementBlock *block;

    SelectUnion(const SelectUnion& p);
    SelectUnion& operator=(const SelectUnion& p);
  public:
    /** ids.empty() == true means "else" case */
    SelectUnion(StatementBlock *p_block);
    virtual ~SelectUnion();
    virtual SelectUnion* clone() const;
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_fullname(const string& p_fullname);
    const vector<Identifier>& get_ids() const { return ids; }
    Identifier* get_id_byIndex(size_t index) const { return ids[index]; }
    StatementBlock *get_block() const { return block; }
    void add_id(Identifier* id);
    /* checking functions */
    void chk(Type *p_gov);
    void set_code_section(GovernedSimple::code_section_t p_code_section);
    char* generate_code_case(char *str, char*& def_glob_vars, char*& src_glob_vars,
      const char *type_name, bool &else_branch);

    /** Needed by implicit omit. Pushes attrib path down to definitions
     */
    virtual void set_parent_path(WithAttribPath* p_path);
  };

  /**
   * Class to represent SelectUnions.
   */
  class SelectUnions : public Node {
  private:
    vector<SelectUnion> sus;

    SelectUnions(const SelectUnions& p);
    SelectUnions& operator=(const SelectUnions& p);
  public:
    SelectUnions() : Node() { }
    virtual ~SelectUnions();
    virtual SelectUnions* clone() const;
    void add_su(SelectUnion *p_su);
    size_t get_nof_sus() const {return sus.size();}
    SelectUnion *get_su_byIndex(size_t p_i) const {return sus[p_i];}
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_fullname(const string& p_fullname);
    void set_my_sb(StatementBlock *p_sb, size_t p_index);
    void set_my_def(Definition *p_def);
    void set_my_ags(AltGuards *p_ags);
    void set_my_laic_stmt(AltGuards *p_ags, Statement *p_loop_stmt);
    StatementBlock::returnstatus_t has_return() const;
    bool has_receiving_stmt() const;
    /* checking functions */
    /** p_gov is the governor type of select expression */
    void chk(Type *p_gov);
    /** checks whether all embedded statements are allowed in an interleaved
     * construct */
    void chk_allowed_interleave();
    void set_code_section(GovernedSimple::code_section_t p_code_section);
    char *generate_code(char *str, char*& def_glob_vars, char*& src_glob_vars,
      const char *type_name, const char* loc);

    /** Needed by implicit omit. Pushes attrib path down to definitions
     */
    virtual void set_parent_path(WithAttribPath* p_path);
  };


  /**
   * Class to represent an alt guard.
   */
  class AltGuard : public Node, public Location {
  public:
    enum altguardtype_t {
      AG_OP,
      AG_REF,
      AG_INVOKE,
      AG_ELSE
    };

  private:
    altguardtype_t altguardtype;
    Value *expr; /**< conditional expression */
    union {
      Ref_pard *ref;
      Statement *stmt;
      void *dummy;
      struct {
        Value *v;
        Ttcn::TemplateInstances *t_list;
        Ttcn::ActualParList *ap_list;
      } invoke;
    };
    StatementBlock *block;

    AltGuard(const AltGuard& p);
    AltGuard& operator=(const AltGuard& p);
  public:
    /** Constructor used by AG_OP */
    AltGuard(Value *p_expr, Statement *p_stmt, StatementBlock *p_block);
    /** Constructor used by AG_REF */
    AltGuard(Value *p_expr, Ref_pard *p_ref, StatementBlock *p_block);
    /** Constructor used by AG_INVOKE */
    AltGuard(Value *p_expr,  Value *p_v,
      Ttcn::TemplateInstances *p_t_list, StatementBlock *p_block);
    /** Constructor used by AG_ELSE */
    AltGuard(StatementBlock *p_block);
    virtual ~AltGuard();
    virtual AltGuard* clone() const;
    virtual void set_my_scope(Scope *p_scope);
    void set_my_sb(StatementBlock *p_sb, size_t p_index);
    virtual void set_fullname(const string& p_fullname);
    altguardtype_t get_type() const { return altguardtype; }
    Value *get_guard_expr() const;
    Ref_pard *get_guard_ref() const;
    Statement *get_guard_stmt() const;
    StatementBlock *get_block() const { return block; }
    void set_my_def(Definition *p_def);
    void set_my_ags(AltGuards *p_ags);
    void set_my_laic_stmt(AltGuards *p_ags, Statement *p_loop_stmt);
    /* checking functions */
    void chk();
    /** Sets the code section selector of all embedded values and templates
     * to \a p_code_section. */
    void set_code_section(GovernedSimple::code_section_t p_code_section);
    /** generates altstep instantiation call */
    void generate_code_invoke_instance(expression_struct *p_expr);
  };

  /**
   * Class to represent AltGuardList.
   */
  class AltGuards : public Node {
  private:
    vector<AltGuard> ags;
    Scope *my_scope;
    /** Indicates whether a repeat statement was found within the branches. */
    bool has_repeat;
    /** C++ label identifier that points to the beginning of the alternative.
     * Used only in alt constructs and within call statements. */
    string *label;
    bool is_altstep;
    string *il_label_end; /** label for break when ags is not compiled to loop*/

    AltGuards(const AltGuards& p);
    AltGuards& operator=(const AltGuards& p);
  public:
    AltGuards() : Node(), my_scope(0), has_repeat(false), label(0),
                  is_altstep(false), il_label_end(0) { }
    virtual ~AltGuards();
    virtual AltGuards* clone() const;
    void add_ag(AltGuard *p_ag);
    size_t get_nof_ags() const {return ags.size();}
    AltGuard *get_ag_byIndex(size_t p_i) const {return ags[p_i];}
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_fullname(const string& p_fullname);
    void set_my_sb(StatementBlock *p_sb, size_t p_index);
    void set_my_def(Definition *p_def);
    /** Sets the ags pointer of all embedded repeat statements to \a p_ags. */
    void set_my_ags(AltGuards *p_ags);
    void set_my_laic_stmt(AltGuards *p_ags, Statement *p_loop_stmt);
    void repeat_found() { has_repeat = true; }
    /** Returns the C++ label identifier that is used in code generation for
     * repeat statements. NULL pointer indicates that \a this belongs to an
     * altstep so the C++ equivalent of repeat shall be a return statement
     * instead of goto. */
    string* get_label() const { return label; }
    void set_is_altstep () { is_altstep = true; }
    bool get_is_altstep () { return is_altstep; }
    void set_il_label_end (const char *lbl) {
      il_label_end = new string (lbl); }
    const string* get_il_label_end () { return il_label_end; }
    bool has_else() const;
    StatementBlock::returnstatus_t has_return() const;
    bool has_receiving_stmt() const;
    /* checking functions */
    void chk();
    /** checks whether all embedded statements are allowed in an interleaved
     * construct */
    void chk_allowed_interleave();
    /** Sets the code section selector of all embedded values and templates
     * to \a p_code_section. */
    void set_code_section(GovernedSimple::code_section_t p_code_section);
    /** Generates the equivalent C++ code for the branches of an alt construct,
     * appends it to \a str and returns the resulting string. Parameter \a loc
     * shall contain the location of the alt construct. */
    char *generate_code_alt(char *str, char*& def_glob_vars, char*& src_glob_vars,
      const Location& loc);
    /** Generates the equivalent C++ code for the branches of an altstep,
     * appends it to \a str and returns the resulting string. */
    char *generate_code_altstep(char *str, char*& def_glob_vars, char*& src_glob_vars);
    /** Generates the equivalent C++ code for the response and
     *  exception handling part of a call statement, appends it to \a
     *  str and returns the resulting string. Parameter \a loc
     *  contains the location of the call statement. \a temp_id is the
     *  temporary id used as prefix for local temporary variables and
     *  stuff. If used in interleave (in_interleave is true), then
     *  those alt branches that contain receiving statement(s) are not
     *  generated but a "goto label_str_branch_n" where 'n' is the
     *  branch number. */
    char* generate_code_call_body(char *str, char*& def_glob_vars, char*& src_glob_vars,
      const Location& loc, const string& temp_id, bool in_interleave);
    void ilt_generate_code_call_body(ILT *ilt, const char *label_str);
  };

} // namespace Ttcn

#endif // _Ttcn_Statement_HH
