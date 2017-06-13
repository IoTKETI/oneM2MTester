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
 *   Feher, Csaba
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *   Szalai, Gabor
 *   Tatarka, Gabor
 *
 ******************************************************************************/
#include "../../common/memory.h"
#include "signature.h"
#include "../datatypes.h"
#include "../main.hh"
#include "compiler.h"

void defSignatureClasses(const signature_def *sdef, output_struct *output)
{
  char *decl = NULL, *def = NULL, *src = NULL;
  const char *name = sdef->name, *dispname = sdef->dispname;
  size_t i, num_in = 0, num_out = 0;

  for (i = 0; i < sdef->parameters.nElements; i++) {
    if (sdef->parameters.elements[i].direction != PAR_OUT) num_in++;
    if (sdef->parameters.elements[i].direction != PAR_IN) num_out++;
  }

  /*
   *
   * xxx_call class
   *
   */

  decl = mputprintf(decl, "class %s_call;\n", name);

  /* class definition */
  def = mputprintf(def, "class %s_call {\n", name);

  /* signature's in and inout parameters */
  for (i = 0; i < sdef->parameters.nElements; i++) {
    if (sdef->parameters.elements[i].direction != PAR_OUT)
      def = mputprintf(def, "%s param_%s;\n",
        sdef->parameters.elements[i].type,
        sdef->parameters.elements[i].name);
  }

  def = mputstr(def, "public:\n");

  /* parameters' access functions */
  for (i = 0; i < sdef->parameters.nElements; i++) {
    if (sdef->parameters.elements[i].direction != PAR_OUT) {
      def = mputprintf(def, "inline %s& %s() { return param_%s; }\n"
        "inline const %s& %s() const { return param_%s; }\n",
        sdef->parameters.elements[i].type,
        sdef->parameters.elements[i].name,
        sdef->parameters.elements[i].name,
        sdef->parameters.elements[i].type,
        sdef->parameters.elements[i].name,
        sdef->parameters.elements[i].name);
    }
  }

  if (num_in > 0) {
    /* encode_text function */
    def = mputstr(def, "void encode_text(Text_Buf& text_buf) const;\n");
    src = mputprintf(src, "void %s_call::encode_text(Text_Buf& text_buf) "
      "const\n"
      "{\n", name);
    for(i = 0; i < sdef->parameters.nElements; i++) {
      if(sdef->parameters.elements[i].direction != PAR_OUT)
        src = mputprintf(src, "param_%s.encode_text(text_buf);\n",
          sdef->parameters.elements[i].name);
    }
    src = mputstr(src, "}\n\n");
    /* decode_text function */
    def = mputstr(def, "void decode_text(Text_Buf& text_buf);\n");
    src = mputprintf(src, "void %s_call::decode_text(Text_Buf& text_buf)\n"
      "{\n", name);
    for (i = 0; i < sdef->parameters.nElements; i++) {
      if (sdef->parameters.elements[i].direction!= PAR_OUT)
        src = mputprintf(src, "param_%s.decode_text(text_buf);\n",
          sdef->parameters.elements[i].name);
    }
    src = mputstr(src, "}\n\n");
  } else {
    def = mputstr(def, "inline void encode_text(Text_Buf&) const "
      "{ }\n"
      "inline void decode_text(Text_Buf&) { }\n");
  }

  /* log function */
  def = mputstr(def, "void log() const;\n");
  src = mputprintf(src, "void %s_call::log() const\n"
    "{\n", name);
  if (num_in > 0) {
    boolean first_param = TRUE;
    src = mputprintf(src, "TTCN_Logger::log_event_str(\"%s : { \");\n",
      dispname);
    for (i = 0; i < sdef->parameters.nElements; i++) {
      if (sdef->parameters.elements[i].direction != PAR_OUT) {
        src = mputstr(src, "TTCN_Logger::log_event_str(\"");
        if (first_param) first_param = FALSE;
        else src = mputstr(src, ", ");
        src = mputprintf(src, "%s := \");\n"
          "param_%s.log();\n",
          sdef->parameters.elements[i].dispname,
          sdef->parameters.elements[i].name);
      }
    }
    src = mputstr(src, "TTCN_Logger::log_event_str(\" }\");\n");
  } else {
    src = mputprintf(src, "TTCN_Logger::log_event_str(\"%s : { }\");\n",
      dispname);
  }
  src = mputstr(src, "}\n\n");

  def = mputstr(def, "};\n\n");
  /* end of xxx_call class*/

  /*
   *
   * xxx_call_redirect class (for getcall port-operation)
   *
   */
  decl = mputprintf(decl, "class %s_call_redirect;\n", name);

  /* class definition */
  def = mputprintf(def, "class %s_call_redirect {\n", name);

  /* parameter pointers */
  for (i = 0; i < sdef->parameters.nElements; i++) {
    if (sdef->parameters.elements[i].direction != PAR_OUT) {
      def = mputprintf(def, "%s *ptr_%s;\n",
        sdef->parameters.elements[i].type,
        sdef->parameters.elements[i].name);
    }
  }

  def = mputstr(def, "public:\n");

  if (num_in > 0) {
    /* constructor */
    boolean first_param = TRUE;
    def = mputprintf(def, "%s_call_redirect(", name);
    for (i = 0;i<sdef->parameters.nElements;i++) {
      if (sdef->parameters.elements[i].direction != PAR_OUT) {
        if (first_param) first_param = FALSE;
        else def = mputstr(def, ", ");
        def = mputprintf(def, "%s *par_%s = NULL",
          sdef->parameters.elements[i].type,
          sdef->parameters.elements[i].name);
      }
    }
    def = mputstr(def, ")\n"
      " : ");
    first_param = TRUE;
    for (i = 0; i < sdef->parameters.nElements; i++) {
      if (sdef->parameters.elements[i].direction != PAR_OUT) {
        if (first_param) first_param = FALSE;
        else def = mputstr(def, ", ");
        def = mputprintf(def, "ptr_%s(par_%s)",
          sdef->parameters.elements[i].name,
          sdef->parameters.elements[i].name);
      }
    }
    def = mputstr(def, " { }\n");
  }
  /* otherwise constructor is not needed */

  /* set_parameters function (used for param redirect in getcall) */
  if (num_in > 0) {
    /* empty virtual destructor (for the virtual set_parameters function) */
    def = mputprintf(def, "virtual ~%s_call_redirect() { }\n"
      "virtual void set_parameters(const %s_call& call_par) const;\n", name, name);
    src = mputprintf(src, "void %s_call_redirect::set_parameters(const "
      "%s_call& call_par) const\n"
      "{\n", name, name);
    for (i = 0; i < sdef->parameters.nElements; i++) {
      if (sdef->parameters.elements[i].direction != PAR_OUT) {
        src = mputprintf(src, "if (ptr_%s != NULL) "
          "*ptr_%s = call_par.%s();\n",
          sdef->parameters.elements[i].name,
          sdef->parameters.elements[i].name,
          sdef->parameters.elements[i].name);
      }
    }
    src = mputstr(src, "}\n\n");
  } else {
    def = mputprintf(def, "inline void set_parameters(const %s_call&"
      ") const { }\n", name);
  }

  def = mputstr(def, "};\n\n");
  /* end of class xxx_call_redirect */


  if (!sdef->is_noblock) {
    /*
     *
     * xxx_reply class
     *
     */
    decl = mputprintf(decl, "class %s_reply;\n", name);

    /* class definition */
    def = mputprintf(def, "class %s_reply {\n", name);

    /* signature's out and inout parameters */
    for (i = 0; i < sdef->parameters.nElements; i++) {
      if (sdef->parameters.elements[i].direction != PAR_IN) {
        def = mputprintf(def, "%s param_%s;\n",
          sdef->parameters.elements[i].type,
          sdef->parameters.elements[i].name);
      }
    }
    if (sdef->return_type != NULL) {
      def = mputprintf(def, "%s reply_value;\n", sdef->return_type);
    }

    def = mputstr(def, "public:\n");

    /* parameters' access functions */
    for (i = 0; i < sdef->parameters.nElements; i++) {
      if (sdef->parameters.elements[i].direction != PAR_IN) {
        def = mputprintf(def, "inline %s& %s() { return param_%s; }\n"
          "inline const %s& %s() const { return param_%s; }\n",
          sdef->parameters.elements[i].type,
          sdef->parameters.elements[i].name,
          sdef->parameters.elements[i].name,
          sdef->parameters.elements[i].type,
          sdef->parameters.elements[i].name,
          sdef->parameters.elements[i].name);
      }
    }
    if (sdef->return_type != NULL) {
      def = mputprintf(def, "inline %s& return_value() "
        "{ return reply_value; }\n"
        "inline const %s& return_value() const "
        "{ return reply_value; }\n",
        sdef->return_type, sdef->return_type);
    }

    if (num_out > 0 || sdef->return_type != NULL) {
      /* encode_text function */
      def = mputstr(def, "void encode_text(Text_Buf& text_buf) const;\n");
      src = mputprintf(src, "void %s_reply::encode_text(Text_Buf& "
        "text_buf) const\n"
        "{\n", name);
      for (i = 0; i < sdef->parameters.nElements; i++) {
        if (sdef->parameters.elements[i].direction != PAR_IN)
          src = mputprintf(src, "param_%s.encode_text(text_buf);\n",
            sdef->parameters.elements[i].name);
      }
      if (sdef->return_type != NULL)
        src = mputstr(src, "reply_value.encode_text(text_buf);\n");
      src = mputstr(src, "}\n\n");
      /* decode_text function */
      def = mputstr(def, "void decode_text(Text_Buf& text_buf);\n");
      src = mputprintf(src, "void %s_reply::decode_text(Text_Buf& "
        "text_buf)\n"
        "{\n", name);
      for (i = 0; i < sdef->parameters.nElements; i++) {
        if (sdef->parameters.elements[i].direction != PAR_IN)
          src = mputprintf(src, "param_%s.decode_text(text_buf);\n",
            sdef->parameters.elements[i].name);
      }
      if (sdef->return_type != NULL)
        src = mputstr(src, "reply_value.decode_text(text_buf);\n");
      src = mputstr(src, "}\n\n");
    } else {
      def = mputstr(def, "inline void encode_text(Text_Buf&) "
        "const { }\n"
        "inline void decode_text(Text_Buf&) { }\n");
    }

    /* log function */
    def = mputstr(def, "void log() const;\n");
    src = mputprintf(src, "void %s_reply::log() const\n"
      "{\n", name);
    if (num_out > 0) {
      boolean first_param = TRUE;
      src = mputprintf(src, "TTCN_Logger::log_event_str(\"%s : { \");\n",
        dispname);
      for (i = 0; i < sdef->parameters.nElements; i++) {
        if (sdef->parameters.elements[i].direction != PAR_IN) {
          src = mputstr(src, "TTCN_Logger::log_event_str(\"");
          if (first_param) first_param = FALSE;
          else src = mputstr(src, ", ");
          src = mputprintf(src, "%s := \");\n"
            "param_%s.log();\n",
            sdef->parameters.elements[i].dispname,
            sdef->parameters.elements[i].name);
        }
      }
      if (sdef->return_type != NULL) {
        src = mputstr(src, "TTCN_Logger::log_event_str(\" } "
          "value \");\n"
          "reply_value.log();\n");
      } else {
        src = mputstr(src, "TTCN_Logger::log_event_str(\" }\");\n");
      }
    } else if (sdef->return_type != NULL) {
      src = mputprintf(src, "TTCN_Logger::log_event_str(\"%s : { } "
        "value \");\n"
        "reply_value.log();\n", dispname);
    } else {
      src = mputprintf(src, "TTCN_Logger::log_event_str(\"%s : "
        "{ }\");\n", dispname);
    }
    src = mputstr(src, "}\n\n");

    def = mputstr(def, "};\n\n");
    /* end of xxx_reply class*/

    /*
     *
     * xxx_reply_redirect class (for getreply port-operation)
     *
     */
    decl = mputprintf(decl, "class %s_reply_redirect;\n", name);

    /* class definition */
    def = mputprintf(def, "class %s_reply_redirect {\n", name);

    /* parameter pointers */
    if (sdef->return_type != NULL) {
      def = mputprintf(def, "%s *ret_val_redir;\n",
        use_runtime_2 ? "Value_Redirect_Interface" : sdef->return_type);
    }
    for (i = 0; i < sdef->parameters.nElements; i++) {
      if (sdef->parameters.elements[i].direction != PAR_IN) {
        def = mputprintf(def, "%s *ptr_%s;\n",
          sdef->parameters.elements[i].type,
          sdef->parameters.elements[i].name);
      }
    }

    def = mputstr(def, "public:\n");
    
    if (num_out > 0 || sdef->return_type != NULL) {
      boolean first_param = TRUE;
      /* constructor */
      def = mputprintf(def, "%s_reply_redirect(", name);
      if (sdef->return_type != NULL) {
        def = mputprintf(def, "%s *return_redirect",
          use_runtime_2 ? "Value_Redirect_Interface" : sdef->return_type);
        first_param = FALSE;
      }
      for (i = 0; i < sdef->parameters.nElements; i++) {
        if(sdef->parameters.elements[i].direction != PAR_IN) {
          if (first_param) first_param = FALSE;
          else def = mputstr(def, ", ");
          def = mputprintf(def, "%s *par_%s = NULL",
            sdef->parameters.elements[i].type,
            sdef->parameters.elements[i].name);
        }
      }
      def = mputstr(def, ")\n"
        " : ");
      first_param = TRUE;
      if (sdef->return_type != NULL) {
        def = mputstr(def, "ret_val_redir(return_redirect)");
        first_param = FALSE;
      }
      for (i = 0; i < sdef->parameters.nElements; i++) {
        if(sdef->parameters.elements[i].direction != PAR_IN) {
          if (first_param) first_param = FALSE;
          else def = mputstr(def, ", ");
          def = mputprintf(def, "ptr_%s(par_%s)",
            sdef->parameters.elements[i].name,
            sdef->parameters.elements[i].name);
        }
      }
      def = mputstr(def, " { }\n");
    }
    /* otherwise constructor is not needed */

    /* set_parameters function (used for param redirect in getreply) */
    if (num_out > 0 || sdef->return_type != NULL) {
      /* empty virtual destructor (for the virtual set_parameters function) */
      def = mputprintf(def, "virtual ~%s_reply_redirect() { }\n", name);
      /* if there are "out" or "inout" parameters or a "return" ... */
      def = mputprintf(def, "virtual void set_parameters(const %s_reply& reply_par) "
        "const;\n", name);
      src = mputprintf(src, "void %s_reply_redirect::set_parameters(const "
        "%s_reply& reply_par) const\n"
        "{\n", name, name);
      for (i = 0; i < sdef->parameters.nElements; i++) {
        if (sdef->parameters.elements[i].direction != PAR_IN) {
          src = mputprintf(src, "if (ptr_%s != NULL) "
            "*ptr_%s = reply_par.%s();\n",
            sdef->parameters.elements[i].name,
            sdef->parameters.elements[i].name,
            sdef->parameters.elements[i].name);
        }
      }
      if (sdef->return_type!=NULL) {
        src = mputstr(src, "if (ret_val_redir != NULL) ");
        if (use_runtime_2) {
          src = mputstr(src, "ret_val_redir->set_values(&reply_par.return_value());\n");
        }
        else {
          src = mputstr(src, "*ret_val_redir = reply_par.return_value();\n");
        }
      }
      src = mputstr(src, "}\n\n");
    }
    else {
      def = mputprintf(def, "inline void set_parameters(const %s_reply&) "
        "const {}\n", name);
    }

    def = mputstr(def, "};\n\n");
    /* end of class xxx_reply_redirect */
  } /* if (!sdeff->is_noblock) */


  if (sdef->exceptions.nElements > 0) {
    char *selection_type, *unbound_value, *selection_prefix;

    selection_type = mcopystr("exception_selection_type");
    unbound_value = mcopystr("UNBOUND_VALUE");
    selection_prefix = mcopystr("ALT");

    /*
     *
     * xxx_exception class
     *
     */
    decl = mputprintf(decl, "class %s_exception;\n", name);
    /* class definition */
    def = mputprintf(def, "class %s_exception {\n", name);

    /* enum type */
    def = mputstr(def, "public:\n"
      "enum exception_selection_type { ");
    for (i = 0; i < sdef->exceptions.nElements; i++) {
      def = mputprintf(def, "ALT_%s = %lu, ",
        sdef->exceptions.elements[i].altname, (unsigned long) i);
    }
    def = mputprintf(def, " UNBOUND_VALUE = %lu };\n"
          "private:\n", (unsigned long) sdef->exceptions.nElements);

    /* data members */
    def = mputprintf(def, "%s exception_selection;\n"
      "union {\n", selection_type);
    for (i = 0; i < sdef->exceptions.nElements; i++) {
      def = mputprintf(def, "%s *field_%s;\n",
        sdef->exceptions.elements[i].name,
        sdef->exceptions.elements[i].altname);
    }
    def = mputstr(def, "};\n");

    /* clean_up function */
    def = mputstr(def, "void clean_up();\n");
    src = mputprintf(src, "void %s_exception::clean_up()\n"
      "{\n"
      "switch (exception_selection) {\n", name);
    for (i = 0; i < sdef->exceptions.nElements; i++) {
      src = mputprintf(src, "case %s_%s:\n"
        "delete field_%s;\n", selection_prefix,
        sdef->exceptions.elements[i].altname,
        sdef->exceptions.elements[i].altname);
      if (i < sdef->exceptions.nElements - 1)
        src = mputstr(src, "break;\n");
    }
    src = mputprintf(src, "default:\n"
      "break;\n"
      "}\n"
      "exception_selection = %s;\n"
      "}\n\n", unbound_value);

    /* copy_value function */
    def = mputprintf(def, "void copy_value(const %s_exception& "
      "other_value);\n", name);
    src = mputprintf(src, "void %s_exception::copy_value(const "
      "%s_exception& other_value)\n"
      "{\n"
      "switch (other_value.exception_selection) {\n", name, name);
    for (i = 0; i < sdef->exceptions.nElements; i++) {
      src = mputprintf(src, "case %s_%s:\n"
        "field_%s = new %s(*other_value.field_%s);\n"
        "break;\n", selection_prefix,
        sdef->exceptions.elements[i].altname,
        sdef->exceptions.elements[i].altname,
        sdef->exceptions.elements[i].name,
        sdef->exceptions.elements[i].altname);
    }
    src = mputprintf(src, "default:\n"
      "TTCN_error(\"Copying an uninitialized exception of signature "
      "%s.\");\n"
      "}\n"
      "exception_selection = other_value.exception_selection;\n"
      "}\n\n", name);

    /* default constructor */
    def = mputprintf(def, "public:\n"
      "%s_exception() : exception_selection(%s) { }\n", name,
      unbound_value);

    /* copy constructor */
    def = mputprintf(def, "%s_exception(const %s_exception& "
      "other_value) { copy_value(other_value); }\n", name, name);

    /* constructors (from exception types) */
    for (i = 0; i < sdef->exceptions.nElements; i++) {
      def = mputprintf(def, "%s_exception(const %s& other_value);\n",
        name, sdef->exceptions.elements[i].name);
      src = mputprintf(src, "%s_exception::%s_exception(const "
        "%s& other_value)\n"
        "{\n"
        "field_%s = new %s(other_value);\n"
        "exception_selection = %s_%s;\n"
        "}\n\n", name, name, sdef->exceptions.elements[i].name,
        sdef->exceptions.elements[i].altname,
        sdef->exceptions.elements[i].name, selection_prefix,
        sdef->exceptions.elements[i].altname);
      def = mputprintf(def, "%s_exception(const %s_template& "
        "other_value);\n", name, sdef->exceptions.elements[i].name);
      src = mputprintf(src, "%s_exception::%s_exception(const "
        "%s_template& other_value)\n"
        "{\n"
        "field_%s = new %s(other_value.valueof());\n"
        "exception_selection = %s_%s;\n"
        "}\n\n", name, name, sdef->exceptions.elements[i].name,
        sdef->exceptions.elements[i].altname,
        sdef->exceptions.elements[i].name, selection_prefix,
        sdef->exceptions.elements[i].altname);
    }

    /* destructor */
    def = mputprintf(def, "~%s_exception() { clean_up(); }\n", name);

    /* assignment operator */
    def = mputprintf(def, "%s_exception& operator=(const %s_exception& "
      "other_value);\n", name, name);
    src = mputprintf(src, "%s_exception& %s_exception::operator=(const "
      "%s_exception& other_value)\n"
      "{\n"
      "if (this != &other_value) {\n"
      "clean_up();\n"
      "copy_value(other_value);\n"
      "}\n"
      "return *this;\n"
      "}\n\n", name, name, name);

    /* field (type) access functions */
    for (i = 0; i < sdef->exceptions.nElements; i++) {
      def = mputprintf(def, "%s& %s_field();\n",
        sdef->exceptions.elements[i].name,
        sdef->exceptions.elements[i].altname);
      src = mputprintf(src, "%s& %s_exception::%s_field()\n"
        "{\n"
        "if (exception_selection != %s_%s) {\n"
        "clean_up();\n"
        "field_%s = new %s;\n"
        "exception_selection = %s_%s;\n"
        "}\n"
        "return *field_%s;\n"
        "}\n\n", sdef->exceptions.elements[i].name, name,
        sdef->exceptions.elements[i].altname, selection_prefix,
        sdef->exceptions.elements[i].altname,
        sdef->exceptions.elements[i].altname,
        sdef->exceptions.elements[i].name,
        selection_prefix, sdef->exceptions.elements[i].altname,
        sdef->exceptions.elements[i].altname);
      def = mputprintf(def, "const %s& %s_field() const;\n",
        sdef->exceptions.elements[i].name,
        sdef->exceptions.elements[i].altname);
      src = mputprintf(src, "const %s& %s_exception::%s_field() const\n"
        "{\n"
        "if (exception_selection != %s_%s) "
        "TTCN_error(\"Referencing to non-selected type %s in an "
        "exception of signature %s.\");\n"
        "return *field_%s;\n"
        "}\n\n", sdef->exceptions.elements[i].name, name,
        sdef->exceptions.elements[i].altname, selection_prefix,
        sdef->exceptions.elements[i].altname,
        sdef->exceptions.elements[i].dispname, dispname,
        sdef->exceptions.elements[i].altname);
    }

    /* get_selection function */
    def = mputprintf(def, "inline %s get_selection() const "
      "{ return exception_selection; }\n", selection_type);

    /* encode_text function */
    def = mputstr(def, "void encode_text(Text_Buf& text_buf) const;\n");
    src = mputprintf(src, "void %s_exception::encode_text(Text_Buf& "
      "text_buf) const\n"
      "{\n"
      "text_buf.push_int(exception_selection);\n"
      "switch (exception_selection) {\n", name);
    for (i = 0; i < sdef->exceptions.nElements; i++) {
      src = mputprintf(src, "case %s_%s:\n"
        "field_%s->encode_text(text_buf);\n"
        "break;\n", selection_prefix,
        sdef->exceptions.elements[i].altname,
        sdef->exceptions.elements[i].altname);
    }
    src = mputprintf(src, "default:\n"
      "TTCN_error(\"Text encoder: Encoding an uninitialized exception "
      "of signature %s.\");\n"
      "}\n"
      "}\n\n", dispname);

    /* decode_text function */
    def = mputstr(def, "void decode_text(Text_Buf& text_buf);\n");
    src = mputprintf(src, "void %s_exception::decode_text(Text_Buf& "
      "text_buf)\n"
      "{\n"
      "switch ((%s)text_buf.pull_int().get_val()) {\n", name, selection_type);
    for (i = 0; i < sdef->exceptions.nElements; i++) {
      src = mputprintf(src, "case %s_%s:\n"
        "%s_field().decode_text(text_buf);\n"
        "break;\n", selection_prefix,
        sdef->exceptions.elements[i].altname,
        sdef->exceptions.elements[i].altname);
    }
    src = mputprintf(src, "default:\n"
      "TTCN_error(\"Text decoder: Unrecognized selector was received "
      "for an exception of signature %s.\");\n"
      "}\n"
      "}\n\n", dispname);

    /* log function */
    def = mputstr(def, "void log() const;\n");
    src = mputprintf(src, "void %s_exception::log() const\n"
      "{\n"
      "TTCN_Logger::log_event_str(\"%s, \");\n"
      "switch (exception_selection) {\n",
      name, dispname);
    for (i = 0; i < sdef->exceptions.nElements; i++) {
      src = mputprintf(src, "case %s_%s:\n"
        "TTCN_Logger::log_event_str(\"%s : \");\n"
        "field_%s->log();\n"
        "break;\n", selection_prefix,
        sdef->exceptions.elements[i].altname,
        sdef->exceptions.elements[i].dispname,
        sdef->exceptions.elements[i].altname);
    }
    src = mputstr(src, "default:\n"
      "TTCN_Logger::log_event_str(\"<uninitialized exception>\");\n"
      "}\n"
      "}\n\n");

    def = mputstr(def, "};\n\n");
    /* end of xxx_exception class*/

    Free(selection_type);
    selection_type = mprintf("%s_exception::exception_selection_type", name);
    Free(unbound_value);
    unbound_value = mprintf("%s_exception::UNBOUND_VALUE", name);
    Free(selection_prefix);
    selection_prefix = mprintf("%s_exception::ALT", name);

    /*
     *
     * simple xxx_exception_template for the catch port-operator
     * only with constructors, a log/match function and value redirect
     *
     */
    decl = mputprintf(decl, "class %s_exception_template;\n", name);

    def = mputprintf(def,
      "class %s_exception_template {\n"
      "public:\n", name);

    /* data members */
    /* exception-selection enum */
    def = mputprintf(def,
      "private:\n"
      "%s exception_selection;\n", selection_type);
    /* union of all possible exceptions (templates) */
    def = mputstr(def, "union {\n");
    for (i = 0; i < sdef->exceptions.nElements; i++) {
      def = mputprintf(def, "const %s_template *field_%s;\n",
        sdef->exceptions.elements[i].name,
        sdef->exceptions.elements[i].altname);
    }
    def = mputstr(def, "};\n");
    /* union of all possible value redirect objects */
    def = mputstr(def, "union {\n");
    for (i = 0; i < sdef->exceptions.nElements; i++) {
      def = mputprintf(def, "%s *%s_redir;\n",
        use_runtime_2 ? "Value_Redirect_Interface" :
        sdef->exceptions.elements[i].name,
        sdef->exceptions.elements[i].altname);
    }
    def = mputstr(def, "};\n"
      "public:\n");
    /* constructors (for all possible template + redirect object pairs) */
    for (i = 0; i < sdef->exceptions.nElements; i++) {
      def = mputprintf(def, "%s_exception_template(const %s_template& "
        "init_template, %s *value_redirect = NULL);\n", name,
        sdef->exceptions.elements[i].name,
        use_runtime_2 ? "Value_Redirect_Interface" :
        sdef->exceptions.elements[i].name);
      src = mputprintf(src, "%s_exception_template::%s_exception_template"
        "(const %s_template& init_template, %s *value_redirect)\n"
        "{\n"
        "exception_selection = %s_%s;\n"
        "field_%s = &init_template;\n"
        "%s_redir = value_redirect;\n"
        "}\n\n", name, name, sdef->exceptions.elements[i].name,
        use_runtime_2 ? "Value_Redirect_Interface" :
        sdef->exceptions.elements[i].name, selection_prefix,
        sdef->exceptions.elements[i].altname,
        sdef->exceptions.elements[i].altname,
        sdef->exceptions.elements[i].altname);
    }

    /* match function */
    def = mputprintf(def, "boolean match(const %s_exception& other_value,"
      "boolean legacy = FALSE) const;\n", name);
    src = mputprintf(src, "boolean %s_exception_template::match(const "
      "%s_exception& other_value, boolean legacy) const\n"
      "{\n"
      "if (exception_selection != other_value.get_selection()) "
      "return FALSE;\n"
      "switch (exception_selection) {\n", name, name);
    for (i = 0; i < sdef->exceptions.nElements; i++) {
      src = mputprintf(src, "case %s_%s:\n"
        "return field_%s->match(other_value.%s_field(), legacy);\n",
        selection_prefix, sdef->exceptions.elements[i].altname,
        sdef->exceptions.elements[i].altname,
        sdef->exceptions.elements[i].altname);
    }
    src = mputprintf(src, "default:\n"
      "TTCN_error(\"Internal error: Invalid selector when matching an "
      "exception of signature %s.\");\n"
      "return FALSE;\n"
      "}\n"
      "}\n\n", dispname);

    /* log_match function */
    def = mputprintf(def, "void log_match(const %s_exception& other_value, "
      "boolean legacy = FALSE) const;\n", name);
    src = mputprintf(src, "void %s_exception_template::log_match(const "
      "%s_exception& other_value, boolean legacy) const\n"
      "{\n"
      "TTCN_Logger::log_event_str(\"%s, \");\n"
      "if (exception_selection == other_value.get_selection()) {\n"
      "switch (exception_selection) {\n", name, name, dispname);
    for (i = 0; i < sdef->exceptions.nElements; i++) {
      src = mputprintf(src, "case %s_%s:\n"
        "TTCN_Logger::log_event_str(\"%s : \");\n"
        "field_%s->log_match(other_value.%s_field(), legacy);\n"
        "break;\n", selection_prefix,
        sdef->exceptions.elements[i].altname,
        sdef->exceptions.elements[i].dispname,
        sdef->exceptions.elements[i].altname,
        sdef->exceptions.elements[i].altname);
    }
    src = mputstr(src, "default:\n"
      "TTCN_Logger::log_event_str(\"<invalid selector>\");\n"
      "}\n"
      "} else {\n"
      "other_value.log();\n"
      "TTCN_Logger::log_event_str(\" with \");\n"
      "switch (exception_selection) {\n");
    for (i = 0; i < sdef->exceptions.nElements; i++) {
      src = mputprintf(src, "case %s_%s:\n"
        "TTCN_Logger::log_event_str(\"%s : \");\n"
        "field_%s->log();\n"
        "break;\n", selection_prefix,
        sdef->exceptions.elements[i].altname,
        sdef->exceptions.elements[i].dispname,
        sdef->exceptions.elements[i].altname);
    }
    src = mputstr(src, "default:\n"
      "TTCN_Logger::log_event_str(\"<invalid selector>\");\n"
      "}\n"
      "if (match(other_value, legacy)) "
      "TTCN_Logger::log_event_str(\" matched\");\n"
      "else TTCN_Logger::log_event_str(\" unmatched\");\n"
      "}\n"
      "}\n\n");

    /* set_value function */
    def = mputprintf(def, "void set_value(const %s_exception& "
      "source_value) const;\n", name);
    src = mputprintf(src, "void %s_exception_template::set_value(const "
      "%s_exception& source_value) const\n"
      "{\n"
      "if (exception_selection == source_value.get_selection()) {\n"
      "switch (exception_selection) {\n", name, name);
    for (i = 0; i < sdef->exceptions.nElements; i++) {
      src = mputprintf(src, "case %s_%s:\n"
        "if (%s_redir != NULL) ", selection_prefix,
        sdef->exceptions.elements[i].altname,
        sdef->exceptions.elements[i].altname);
      if (use_runtime_2) {
        src = mputprintf(src, "%s_redir->set_values(&source_value.%s_field());\n",
          sdef->exceptions.elements[i].altname,
          sdef->exceptions.elements[i].altname);
      }
      else {
        src = mputprintf(src, "*%s_redir = source_value.%s_field();\n",
          sdef->exceptions.elements[i].altname,
          sdef->exceptions.elements[i].altname);
      }
      src = mputstr(src, "return;\n");
    }
    src = mputprintf(src, "default:\n"
      "break;\n"
      "}\n"
      "}\n"
      "TTCN_error(\"Internal error: Invalid selector when performing "
      "value redirect on an exception of signature %s.\");\n"
      "}\n\n", dispname);
    
    /* is_any_or_omit function */
    def = mputprintf(def, "boolean is_any_or_omit() const;\n");
    src = mputprintf(src, 
      "boolean %s_exception_template::is_any_or_omit() const\n"
      "{\n"
      "switch (exception_selection) {\n", name);
    for (i = 0; i < sdef->exceptions.nElements; i++) {
      src = mputprintf(src,
        "case %s_%s:\n"
        "return field_%s->get_selection() == ANY_OR_OMIT;\n",
        selection_prefix, sdef->exceptions.elements[i].altname,
        sdef->exceptions.elements[i].altname);
    }
    src = mputprintf(src,
      "default:\n"
      "break;\n"
      "}\n"
      "TTCN_error(\"Internal error: Invalid selector when checking for '*' in "
      "an exception template of signature %s.\");\n"
      "}\n\n", dispname);

    def = mputstr(def, "};\n\n");
    /* end of class xxx_exception_template */

    Free(selection_type);
    Free(unbound_value);
    Free(selection_prefix);
  } /* if (sdef->exceptions.nElements > 0) */

  /*
   * template class
   */

  decl = mputprintf(decl, "class %s_template;\n", name);

  def = mputprintf(def, "class %s_template {\n", name);

  /* parameters */
  for (i = 0; i < sdef->parameters.nElements; i++) {
    def = mputprintf(def, "%s_template param_%s;\n",
      sdef->parameters.elements[i].type,
      sdef->parameters.elements[i].name);
  }
  if (sdef->return_type != NULL) {
    def = mputprintf(def, "mutable %s_template reply_value;\n",
      sdef->return_type);
  }

  def = mputstr(def, "public:\n");
  /* constructor */
  if (sdef->parameters.nElements > 0 || sdef->return_type != NULL) {
    boolean first_param = TRUE;
    def = mputprintf(def, "%s_template();\n", name);
    src = mputprintf(src, "%s_template::%s_template()\n"
      " : ", name, name);
    for (i = 0; i < sdef->parameters.nElements; i++) {
      if (first_param) first_param = FALSE;
      else src = mputstr(src, ", ");
      src = mputprintf(src, "param_%s(ANY_VALUE)",
        sdef->parameters.elements[i].name);
    }
    if (sdef->return_type != NULL) {
      if (first_param) /*first_param = FALSE*/;
      else src = mputstr(src, ", ");
      src = mputstr(src, "reply_value(ANY_VALUE)");
    }
    src = mputstr(src, "\n"
      "{\n"
      "}\n\n");
  } else {
    def = mputprintf(def, "%s_template() { }\n", name);
  }

  if (sdef->parameters.nElements == 0) {
    /* constructor and assignment operator for parameter-less signatures */
    if (sdef->return_type != NULL) {
      def = mputprintf(def, "%s_template(null_type null_value);\n",
        name);
      src = mputprintf(src,
        "%s_template::%s_template(null_type)\n"
        " : reply_value(ANY_VALUE)\n"
        "{\n"
        "}\n\n", name, name);
    } else {
      def = mputprintf(def, "%s_template(null_type) { }\n",
        name);
    }
    def = mputprintf(def, "inline %s_template& operator=(null_type)"
      " { return *this; }\n", name);
  }

  /* parameter access functions */
  for (i = 0; i < sdef->parameters.nElements; i++) {
    def = mputprintf(def, "inline %s_template& %s() { return param_%s; }\n"
      "inline const %s_template& %s() const { return param_%s; }\n",
      sdef->parameters.elements[i].type,
      sdef->parameters.elements[i].name,
      sdef->parameters.elements[i].name,
      sdef->parameters.elements[i].type,
      sdef->parameters.elements[i].name,
      sdef->parameters.elements[i].name);
  }
  if (sdef->return_type != NULL) {
    def = mputprintf(def, "inline %s_template& return_value() const "
      "{ return reply_value; }\n", sdef->return_type);
  }

  /* create_call function for creating xxx_call signature-value */
  if (num_in > 0) {
    def = mputprintf(def, "%s_call create_call() const;\n", name);
    src = mputprintf(src, "%s_call %s_template::create_call() const\n"
      "{\n"
      "%s_call ret_val;\n", name, name, name);
    for (i = 0; i < sdef->parameters.nElements; i++) {
      if (sdef->parameters.elements[i].direction != PAR_OUT) {
        src = mputprintf(src, "ret_val.%s() = param_%s.valueof();\n",
          sdef->parameters.elements[i].name,
          sdef->parameters.elements[i].name);
      }
    }
    src = mputstr(src, "return ret_val;\n"
      "}\n\n");
  } else {
    def = mputprintf(def, "inline %s_call create_call() const "
      "{ return %s_call(); }\n", name, name);
  }

  if (!sdef->is_noblock) {
    /* create_reply function for creating xxx_reply signature-value */
    if (num_out > 0 || sdef->return_type != NULL) {
      def = mputprintf(def, "%s_reply create_reply() const;\n", name);
      src = mputprintf(src, "%s_reply %s_template::create_reply() const\n"
        "{\n"
        "%s_reply ret_val;\n", name, name, name);
      for (i = 0; i < sdef->parameters.nElements; i++) {
        if (sdef->parameters.elements[i].direction != PAR_IN) {
          src = mputprintf(src, "ret_val.%s() = "
            "param_%s.valueof();\n",
            sdef->parameters.elements[i].name,
            sdef->parameters.elements[i].name);
        }
      }
      if (sdef->return_type != NULL) {
        src = mputstr(src, "ret_val.return_value() = "
          "reply_value.valueof();\n");
      }
      src = mputstr(src, "return ret_val;\n"
        "}\n\n");
    } else {
      def = mputprintf(def, "inline %s_reply create_reply() const "
        "{ return %s_reply(); }\n", name, name);
    }
  }

  /* match_call function for matching with xxx_call signature-value */
  if (num_in > 0) {
    boolean first_param = TRUE;
    def = mputprintf(def, "boolean match_call(const %s_call& match_value, "
      "boolean legacy = FALSE) const;\n", name);
    src = mputprintf(src, "boolean %s_template::match_call(const %s_call& "
      "match_value, boolean legacy) const\n"
      "{\n"
      "return ", name, name);
    for (i = 0; i < sdef->parameters.nElements; i++) {
      if (sdef->parameters.elements[i].direction != PAR_OUT) {
        if (first_param) first_param = FALSE;
        else src = mputstr(src, " &&\n");
        src = mputprintf(src, "param_%s.match(match_value.%s(), legacy)",
          sdef->parameters.elements[i].name,
          sdef->parameters.elements[i].name);
      }
    }
    src = mputstr(src, ";\n"
      "}\n\n");
  } else {
    def = mputprintf(def, "inline boolean match_call(const %s_call&"
      ", boolean legacy = FALSE) const { return TRUE; }\n", name);
  }

  if (!sdef->is_noblock) {
    if (num_out > 0 || sdef->return_type != NULL) {
      boolean first_param = TRUE;
      /* match_reply function for matching with xxx_reply value */
      def = mputprintf(def, "boolean match_reply(const %s_reply& "
        "match_value, boolean legacy = FALSE) const;\n", name);
      src = mputprintf(src, "boolean %s_template::match_reply(const "
        "%s_reply& match_value, boolean legacy) const\n"
        "{\n"
        "return ", name, name);
      for (i = 0; i < sdef->parameters.nElements; i++) {
        if(sdef->parameters.elements[i].direction != PAR_IN) {
          if (first_param) first_param = FALSE;
          else src = mputstr(src, " &&\n");
          src = mputprintf(src, "param_%s.match(match_value.%s(), legacy)",
            sdef->parameters.elements[i].name,
            sdef->parameters.elements[i].name);
        }
      }
      if (sdef->return_type != NULL) {
        if (first_param) /*first_param = FALSE*/;
        else src = mputstr(src, " &&\n");
        src = mputstr(src,
          "reply_value.match(match_value.return_value(), legacy)");
      }
      src = mputstr(src, ";\n"
        "}\n\n");
    } else {
      def = mputprintf(def, "inline boolean match_reply(const %s_reply&"
        ", boolean legacy = FALSE) const { return TRUE; }\n", name);
    }
  }

  /* set_value_template function */
  if (sdef->return_type != NULL) {
    def = mputprintf(def, "const %s_template& set_value_template(const "
      "%s_template& new_template) const;\n", name, sdef->return_type);
    src = mputprintf(src,
      "const %s_template& %s_template::set_value_template"
      "(const %s_template& new_template) const\n"
      "{\n"
      "reply_value = new_template;\n"
      "return *this;\n"
      "}\n\n", name, name, sdef->return_type);
  }

  /* log function */
  def = mputstr(def, "void log() const;\n");
  src = mputprintf(src, "void %s_template::log() const\n"
    "{\n", name);
  if (sdef->parameters.nElements >= 1) {
    for (i = 0; i < sdef->parameters.nElements; i++) {
      src = mputstr(src, "TTCN_Logger::log_event_str(\"");
      if (i == 0) src = mputc(src, '{');
      else src = mputc(src, ',');
      src = mputprintf(src, " %s := \");\n"
        "param_%s.log();\n", sdef->parameters.elements[i].dispname,
        sdef->parameters.elements[i].name);
    }
    src = mputstr(src, "TTCN_Logger::log_event_str(\" }\");\n");
  } else {
    src = mputstr(src, "TTCN_Logger::log_event_str(\"{ }\");\n");
  }
  src = mputstr(src, "}\n\n");

  /* log_match_call function */
  def = mputprintf(def, "void log_match_call(const %s_call& match_value, "
    "boolean legacy = FALSE) const;\n", name);
  src = mputprintf(src, "void %s_template::log_match_call(const %s_call& "
    , name, name);
  if (num_in > 0) {
    boolean first_param = TRUE;
    src = mputstr(src, "match_value, boolean legacy) const\n{\n");
    for (i = 0; i < sdef->parameters.nElements; i++) {
      if (sdef->parameters.elements[i].direction != PAR_OUT) {
        src = mputstr(src, "TTCN_Logger::log_event_str(\"");
        if (first_param) {
          src = mputc(src, '{');
          first_param = FALSE;
        } else src = mputc(src, ',');
        src = mputprintf(src, " %s := \");\n"
          "param_%s.log_match(match_value.%s(), legacy);\n",
          sdef->parameters.elements[i].dispname,
          sdef->parameters.elements[i].name,
          sdef->parameters.elements[i].name);
      }
    }
    src = mputstr(src, "TTCN_Logger::log_event_str(\" }\");\n");
  } else {
    src = mputstr(src, ", boolean) const\n{\n"
      "TTCN_Logger::log_event_str(\"{ } with { } matched\");\n");
  }
  src = mputstr(src, "}\n\n");

  if (!sdef->is_noblock) {
    /* log_match_reply function */
    def = mputprintf(def, "void log_match_reply(const %s_reply& match_value, "
      "boolean legacy = FALSE) const;\n", name);
    src = mputprintf(src, "void %s_template::log_match_reply(const %s_reply& "
      , name, name);
    if (num_out > 0) {
      boolean first_param = TRUE;
      src = mputstr(src, "match_value, boolean legacy) const\n{\n");
      for (i = 0; i < sdef->parameters.nElements; i++) {
        if (sdef->parameters.elements[i].direction != PAR_IN) {
          src = mputstr(src, "TTCN_Logger::log_event_str(\"");
          if (first_param) {
            src = mputc(src, '{');
            first_param = FALSE;
          } else src = mputc(src, ',');
          src = mputprintf(src, " %s := \");\n"
            "param_%s.log_match(match_value.%s(), legacy);\n",
            sdef->parameters.elements[i].dispname,
            sdef->parameters.elements[i].name,
            sdef->parameters.elements[i].name);
        }
      }
      if (sdef->return_type != NULL) {
        src = mputstr(src, "TTCN_Logger::log_event_str(\" } value "
          "\");\n"
          "reply_value.log_match(match_value.return_value(), legacy);\n");
      } else {
        src = mputstr(src, "TTCN_Logger::log_event_str(\" }\");\n");
      }
    } else {
      if (sdef->return_type != NULL) {
        src = mputstr(src, "match_value, boolean legacy) const\n{\n"
          "TTCN_Logger::log_event_str(\"{ } with { } "
          "matched value \");\n"
          "reply_value.log_match(match_value.return_value(), legacy);\n");
      } else {
        src = mputstr(src, ", boolean) const\n{\n"
          "TTCN_Logger::log_event_str(\"{ } with { } "
          "matched\");\n");
      }
    }
    src = mputstr(src, "}\n\n");
  }

  if (sdef->parameters.nElements > 0) {
    /* encode_text function */
    def = mputstr(def, "void encode_text(Text_Buf& text_buf) const;\n");
    src = mputprintf(src, "void %s_template::encode_text(Text_Buf& "
      "text_buf) const\n"
      "{\n", name);
    for (i = 0; i < sdef->parameters.nElements; i++) {
      src = mputprintf(src, "param_%s.encode_text(text_buf);\n",
        sdef->parameters.elements[i].name);
    }
    src = mputstr(src, "}\n\n");
    /* decode_text function */
    def = mputstr(def, "void decode_text(Text_Buf& text_buf);\n");
    src = mputprintf(src, "void %s_template::decode_text(Text_Buf& "
      "text_buf)\n"
      "{\n", name);
    for (i = 0; i < sdef->parameters.nElements; i++) {
      src = mputprintf(src, "param_%s.decode_text(text_buf);\n",
        sdef->parameters.elements[i].name);
    }
    src = mputstr(src, "}\n\n");
  } else {
    def = mputstr(def, "inline void encode_text(Text_Buf&) const "
      "{ }\n"
      "inline void decode_text(Text_Buf&) { }\n");
  }

  /* end of template definition */
  def = mputstr(def, "};\n\n");

  output->header.class_decls = mputstr(output->header.class_decls, decl);
  Free(decl);

  output->header.class_defs = mputstr(output->header.class_defs, def);
  Free(def);

  output->source.methods = mputstr(output->source.methods, src);
  Free(src);
}
