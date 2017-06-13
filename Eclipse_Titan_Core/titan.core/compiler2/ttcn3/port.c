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
 *   Cserveni, Akos
 *   Delic, Adam
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Kremer, Peter
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../../common/memory.h"
#include "../../common/path.h"
#include "../../common/version_internal.h"
#include "../../common/userinfo.h"

#include "port.h"
#include "../datatypes.h"
#include "compiler.h"
#include "../main.hh"
#include "../error.h"

/* macro used in version_internal.h */
#undef COMMENT_PREFIX
#define COMMENT_PREFIX "// "

#ifndef NDEBUG
static const char *functypes[] = {"convert", "fast", "backtrack", "sliding"};
#endif

static char *generate_send_mapping(char *src, const port_def *pdef,
  const port_msg_mapped_type *mapped_type, boolean has_address)
{
  size_t i;
  boolean has_buffer = FALSE, has_discard = FALSE, report_error = FALSE;
  if (pdef->testport_type == INTERNAL) {
    src = mputstr(src, "if (destination_component == SYSTEM_COMPREF) "
      "TTCN_error(\"Message cannot be sent to system on internal port "
      "%s.\", port_name);\n");
  }
  for (i = 0; i < mapped_type->nTargets; i++) {
    const port_msg_type_mapping_target *target =
      mapped_type->targets + i;
    boolean has_condition = FALSE; /* TRUE when an if statement has been
	generated around the call of the encoder function */
    if (target->mapping_type == M_DISCARD) {
      /* "discard" should always be the last mapping */
      if (i != mapped_type->nTargets - 1)
        FATAL_ERROR("generate_send_mapping(): invalid discard mapping");
      has_discard = TRUE;
      break;
    } else if (target->mapping_type == M_DECODE && !has_buffer) {
      src = mputstr(src, "TTCN_Buffer ttcn_buffer(send_par);\n");
      /* has_buffer will be set to TRUE later */
    }
    if (!pdef->legacy && pdef->port_type == USER) {
      // Mappings should only happen if the port it is mapped to has the same
      // outgoing message type as the mapping target.
      src = mputstr(src, "if (false");
      for (size_t j = 0; j < pdef->provider_msg_outlist.nElements; j++) {
        for (size_t k = 0; k < pdef->provider_msg_outlist.elements[j].n_out_msg_type_names; k++) {
          if (strcmp(target->target_name, pdef->provider_msg_outlist.elements[j].out_msg_type_names[k]) == 0) {
            src = mputprintf(src, " || p_%i != NULL", (int)j);
          }
        }
      }
      src = mputstr(src, ") {\n");
      // Beginning of the loop of the PARTIALLY_TRANSLATED case to process all
      // messages
      src = mputstr(src, "do {\n");
      src = mputstr(src, "TTCN_Runtime::set_translation_mode(TRUE, this);\n");
      // Set to unset
      src = mputstr(src, "TTCN_Runtime::set_port_state(-1, \"by test environment.\", TRUE);\n");
    }
    if (mapped_type->nTargets > 1) src = mputstr(src, "{\n");
    switch (target->mapping_type) {
    case M_FUNCTION:
#ifndef NDEBUG
      src = mputprintf(src, "// out mapping with a prototype(%s) function\n",
        functypes[target->mapping.function.prototype]);
#endif
      switch (target->mapping.function.prototype) {
      case PT_CONVERT:
        src = mputprintf(src, "%s mapped_par(%s(send_par));\n",
          target->target_name, target->mapping.function.name);
        break;
      case PT_FAST:
        src = mputprintf(src, "%s mapped_par;\n"
          "%s(send_par, mapped_par);\n",
          target->target_name, target->mapping.function.name);
        if (!pdef->legacy) {
          has_condition = TRUE;
        }
        break;
      case PT_SLIDING:
        /* Yes, it is possible to use a "prototype(sliding)" decoder
         * for out mapping, even a compiler-generated one.
         * The source must be octetstring for the first parameter
         * of a sliding decoder. */
        src = mputprintf(src, "%s mapped_par;\n"
          "OCTETSTRING send_copy(send_par);\n"
          "if (%s(send_copy, mapped_par) != 1) {\n",
          target->target_name, target->mapping.function.name);
        has_condition = TRUE;
        break;
      case PT_BACKTRACK:
        src = mputprintf(src, "%s mapped_par;\n"
          "if (%s(send_par, mapped_par) == 0) {\n",
          target->target_name, target->mapping.function.name);
        has_condition = TRUE;
        break;
      default:
        FATAL_ERROR("generate_send_mapping(): invalid function type");
      }
      break;
      case M_ENCODE:
#ifndef NDEBUG
        src = mputstr(src, "// out mapping with a built-in encoder\n");
#endif
        src = mputstr(src, target->mapping.encdec.errorbehavior);
        src = mputprintf(src, "TTCN_Buffer ttcn_buffer;\n"
          "send_par.encode(%s_descr_, ttcn_buffer, TTCN_EncDec::CT_%s",
          target->mapping.encdec.typedescr_name,
          target->mapping.encdec.encoding_type);
        if (target->mapping.encdec.encoding_options != NULL)
          src = mputprintf(src, ", %s",
            target->mapping.encdec.encoding_options);
        src = mputprintf(src, ");\n"
          "%s mapped_par;\n"
          "ttcn_buffer.get_string(mapped_par);\n", target->target_name);
        break;
      case M_DECODE:
#ifndef NDEBUG
        src = mputstr(src, "// out mapping with a built-in decoder\n");
#endif
        if (has_buffer) src = mputstr(src, "ttcn_buffer.rewind();\n");
        else has_buffer = TRUE;
        src = mputstr(src, target->mapping.encdec.errorbehavior);
        src = mputprintf(src, "TTCN_EncDec::clear_error();\n"
          "%s mapped_par;\n"
          "mapped_par.decode(%s_descr_, ttcn_buffer, TTCN_EncDec::CT_%s",
          target->target_name, target->mapping.encdec.typedescr_name,
          target->mapping.encdec.encoding_type);
        if (target->mapping.encdec.encoding_options != NULL)
          src = mputprintf(src, ", %s",
            target->mapping.encdec.encoding_options);
        src = mputstr(src, ");\n"
          "if (TTCN_EncDec::get_last_error_type() == "
          "TTCN_EncDec::ET_NONE) {\n");
        has_condition = TRUE;
        break;
      default:
        FATAL_ERROR("generate_send_mapping(): invalid mapping type");
    }
    if (!pdef->legacy && pdef->port_type == USER) {
      src = mputstr(src,
        "TTCN_Runtime::set_translation_mode(FALSE, NULL);\n"
        "if (port_state == TRANSLATED || port_state == PARTIALLY_TRANSLATED) {\n");
    }
    src = mputprintf(src, "if (TTCN_Logger::log_this_event("
      "TTCN_Logger::PORTEVENT_DUALSEND)) {\n"
      "TTCN_Logger::log_dualport_map(0, \"%s\",\n"
      "(TTCN_Logger::begin_event(TTCN_Logger::PORTEVENT_DUALSEND, TRUE),"
      " mapped_par.log(), TTCN_Logger::end_event_log2str()), 0);\n"
      "}\n", target->target_dispname);
    if (has_address) {
      src = mputstr(src,
        "outgoing_send(mapped_par, &destination_address);\n");
    } else {
      if (pdef->testport_type != INTERNAL) {
        src = mputprintf(src, "if (destination_component == "
          "SYSTEM_COMPREF) outgoing_%ssend(mapped_par%s);\n"
          "else {\n",
          pdef->port_type == USER && !pdef->legacy ? "mapped_" : "",
          pdef->testport_type == ADDRESS ? ", NULL" : "");
      }
      src = mputprintf(src, "Text_Buf text_buf;\n"
        "prepare_message(text_buf, \"%s\");\n"
        "mapped_par.encode_text(text_buf);\n"
        "send_data(text_buf, destination_component);\n",
        target->target_dispname);
      if (pdef->testport_type != INTERNAL) src = mputstr(src, "}\n");
    }
    if (has_condition) {
      if (!pdef->legacy && pdef->port_type == USER) {
        src = mputstr(src, "if (port_state != PARTIALLY_TRANSLATED) {\n");
      }
      src = mputstr(src, "return;\n");
      if (!pdef->legacy && pdef->port_type == USER) {
        src = mputstr(src, "}\n");
      }
      if (pdef->legacy) {
        src = mputstr(src, "}\n");
      }
      report_error = TRUE;
    }
    if (!pdef->legacy && pdef->port_type == USER) {
      src = mputprintf(src,
        "} else if (port_state == FRAGMENTED) {\n"
        "return;\n"
        "} else if (port_state == UNSET) {\n"
        "TTCN_error(\"The state of the port %%s remained unset after the mapping function %s finished.\", port_name);\n"
        "}\n", target->mapping.function.dispname);
    }
    if (mapped_type->nTargets > 1) src = mputstr(src, "}\n");
    if (!pdef->legacy && pdef->port_type == USER) {
      // End of the do while loop to process all the messages
      src = mputstr(src, "} while (port_state == PARTIALLY_TRANSLATED);\n");
      // end of the outgoing messages of port with mapping target check
      src = mputstr(src, "}\n");
    }
  }
  if (has_discard) {
    if (mapped_type->nTargets > 1) {
      /* there are other mappings, which failed */
      src = mputprintf(src,
        "TTCN_Logger::log_dualport_discard(0, \"%s\", port_name, TRUE);\n"
        , mapped_type->dispname);
    } else {
      /* this is the only mapping */
      src = mputprintf(src,
        "TTCN_Logger::log_dualport_discard(0, \"%s\", port_name, FALSE);\n"
        , mapped_type->dispname);
    }
  } else if (report_error) {
    src = mputprintf(src, "TTCN_error(\"Outgoing message of type %s could "
      "not be handled by the type mapping rules on port %%s.\", "
      "port_name);\n", mapped_type->dispname);
  }
  return src;
}

static char *generate_incoming_mapping(char *src, const port_def *pdef,
  const port_msg_mapped_type *mapped_type, boolean has_simple)
{
  // If has simple is true, then always the first one is the simple mapping,
  // and the first mapping is taken care elsewhere
  size_t i = has_simple ? 1 : 0;
  boolean has_buffer = FALSE, has_discard = FALSE, report_error = FALSE;
  for (; i < mapped_type->nTargets; i++) {
    const port_msg_type_mapping_target *target =
      mapped_type->targets + i;
    boolean has_condition = FALSE;
    if (target->mapping_type == M_DISCARD) {
      /* "discard" should always be the last mapping */
      if (i != mapped_type->nTargets - 1) FATAL_ERROR( \
        "generate_incoming_mapping(): invalid discard mapping");
      has_discard = TRUE;
      break;
    } else if (target->mapping_type == M_DECODE && !has_buffer) {
      src = mputstr(src, "TTCN_Buffer ttcn_buffer(incoming_par);\n");
      /* has_buffer will be set to TRUE later */
    }
    if (!pdef->legacy && pdef->port_type == USER) {
      // Beginning of the loop of the PARTIALLY_TRANSLATED case to process all
      // messages
      src = mputstr(src, "do {\n");
      src = mputstr(src, "TTCN_Runtime::set_translation_mode(TRUE, this);\n");
      src = mputstr(src, "TTCN_Runtime::set_port_state(-1, \"by test environment.\", TRUE);\n");
    }
    if (mapped_type->nTargets > 1) src = mputstr(src, "{\n");
    switch (target->mapping_type) {
    case M_FUNCTION:
#ifndef NDEBUG
      src = mputprintf(src, "// in mapping with a prototype(%s) function\n",
        functypes[target->mapping.function.prototype]);
#endif
      switch (target->mapping.function.prototype) {
      case PT_CONVERT:
        src = mputprintf(src, "%s *mapped_par = "
          "new %s(%s(incoming_par));\n",
          target->target_name, target->target_name,
          target->mapping.function.name);
        break;
      case PT_FAST:
        src = mputprintf(src, "%s *mapped_par = new %s;\n"
          "try {\n"
          "%s(incoming_par, *mapped_par);\n"
          "} catch (...) {\n"
          "delete mapped_par;\n"
          "throw;\n"
          "}\n", target->target_name, target->target_name,
          target->mapping.function.name);
          if (!pdef->legacy) {
            has_condition = TRUE;
          }
        break;
      case PT_SLIDING:
        src = mputprintf(src,
          "slider += incoming_par;\n" /* hack */
          "for(;;){\n"
          "%s *mapped_par = new %s;\n"
          "int decoding_result;\n"
          "try {\n"
          "decoding_result = %s(slider, *mapped_par);\n"
          "} catch (...) {\n"
          "delete mapped_par;\n"
          "throw;\n"
          "}\n"
          "if (decoding_result==0) {\n", target->target_name,
          target->target_name, target->mapping.function.name);
        has_condition = TRUE;
        break;
      case PT_BACKTRACK:
        src = mputprintf(src, "%s *mapped_par = new %s;\n"
          "boolean success_flag;\n"
          "try {\n"
          "success_flag = %s(incoming_par, *mapped_par) == 0;\n"
          "} catch (...) {\n"
          "delete mapped_par;\n"
          "throw;\n"
          "}\n"
          "if (success_flag) {\n", target->target_name,
          target->target_name, target->mapping.function.name);
        has_condition = TRUE;
        break;
      default:
        FATAL_ERROR("generate_incoming_mapping(): " \
          "invalid function type");
      }
      break;
      case M_ENCODE:
#ifndef NDEBUG
        src = mputstr(src, "// in mapping with a built-in encoder\n");
#endif
        src = mputstr(src, target->mapping.encdec.errorbehavior);
        src = mputprintf(src, "TTCN_Buffer ttcn_buffer;\n"
          "send_par.encode(%s_descr_, ttcn_buffer, TTCN_EncDec::CT_%s",
          target->mapping.encdec.typedescr_name,
          target->mapping.encdec.encoding_type);
        if (target->mapping.encdec.encoding_options != NULL)
          src = mputprintf(src, ", %s",
            target->mapping.encdec.encoding_options);
        src = mputprintf(src, ");\n"
          "%s *mapped_par = new %s;\n"
          "ttcn_buffer.get_string(*mapped_par);\n", target->target_name,
          target->target_name);
        break;
      case M_DECODE:
#ifndef NDEBUG
        src = mputstr(src, "// in mapping with a built-in decoder\n");
#endif
        if (has_buffer) src = mputstr(src, "ttcn_buffer.rewind();\n");
        else has_buffer = TRUE;
        src = mputstr(src, target->mapping.encdec.errorbehavior);
        src = mputprintf(src, "TTCN_EncDec::clear_error();\n"
          "%s *mapped_par = new %s;\n"
          "try {\n"
          "mapped_par->decode(%s_descr_, ttcn_buffer, "
          "TTCN_EncDec::CT_%s", target->target_name,
          target->target_name, target->mapping.encdec.typedescr_name,
          target->mapping.encdec.encoding_type);
        if (target->mapping.encdec.encoding_options != NULL)
          src = mputprintf(src, ", %s",
            target->mapping.encdec.encoding_options);
        src = mputstr(src, ");\n"
          "} catch (...) {\n"
          "delete mapped_par;\n"
          "throw;\n"
          "}\n"
          "if (TTCN_EncDec::get_last_error_type() == "
          "TTCN_EncDec::ET_NONE) {\n");
        has_condition = TRUE;
        break;
      default:
        FATAL_ERROR("generate_incoming_mapping(): invalid mapping type");
    }
    src = mputprintf(src, "%s"
      "msg_tail_count++;\n"
      "if (TTCN_Logger::log_this_event(TTCN_Logger::PORTEVENT_DUALRECV)) "
      "{\n"
      "TTCN_Logger::log_dualport_map(1, \"%s\",\n"
      "(TTCN_Logger::begin_event(TTCN_Logger::PORTEVENT_DUALRECV, TRUE),"
      " mapped_par->log(), TTCN_Logger::end_event_log2str()),\n"
      "msg_tail_count);\n"
      "}\n"
      "msg_queue_item *new_item = new msg_queue_item;\n"
      "new_item->item_selection = MESSAGE_%lu;\n"
      "new_item->message_%lu = mapped_par;\n"
      "new_item->sender_component = sender_component;\n",
      !pdef->legacy && pdef->port_type == USER ?
        "TTCN_Runtime::set_translation_mode(FALSE, NULL);\n"
        "if (port_state == TRANSLATED || port_state == PARTIALLY_TRANSLATED) {\n" : "",
      target->target_dispname,
      (unsigned long) target->target_index,
      (unsigned long) target->target_index);
    if (pdef->testport_type == ADDRESS) {
      src = mputprintf(src, "if (sender_address != NULL) "
        "new_item->sender_address = new %s(*sender_address);\n"
        "else new_item->sender_address = NULL;\n",
        pdef->address_name);
    }
    src = mputstr(src, "append_to_msg_queue(new_item);\n");
    if (has_condition) {
      if (pdef->has_sliding
        && target->mapping_type == M_FUNCTION
        && target->mapping.function.prototype == PT_SLIDING) {
        src = mputstr(src,
          "continue;\n"
          "} else delete mapped_par;\n"
          "if (decoding_result==2) return; }\n");
      }
      else {
        src = mputstr(src, "return;\n"
          "}");
        if (pdef->port_type == USER && !pdef->legacy) {
          src = mputprintf(src,
            " else if (port_state == FRAGMENTED) {\n"
            "delete mapped_par;\n"
            "return;\n"
            "}"
            " else if (port_state == UNSET) {\n"
            "delete mapped_par;\n"
            "TTCN_error(\"The state of the port %%s remained unset after the mapping function %s finished.\", port_name);\n"
            "}\n", target->mapping.function.dispname);
        }
        src = mputstr(src, "else {\ndelete mapped_par;\n}\n");
      }
      report_error = TRUE;
    }
    if (mapped_type->nTargets > 1) src = mputstr(src, "}\n");
    if (pdef->port_type == USER && !pdef->legacy) {
      src = mputstr(src, "} while (port_state == PARTIALLY_TRANSLATED);\n");
    }
  } /* next mapping target */
  if (has_discard) {
    if (mapped_type->nTargets > 1) {
      /* there are other mappings, which failed */
      src = mputprintf(src,
        "TTCN_Logger::log_dualport_discard(1, \"%s\", port_name, TRUE);\n"
        , mapped_type->dispname);
    } else {
      /* this is the only mapping */
      src = mputprintf(src,
        "TTCN_Logger::log_dualport_discard(1, \"%s\", port_name, FALSE);\n"
        , mapped_type->dispname);
    }
  } else if (report_error && !has_simple) { // only report error if no simple mapping is present
    src = mputprintf(src, "TTCN_error(\"Incoming message of type %s could "
      "not be handled by the type mapping rules on port %%s.\", "
      "port_name);\n", mapped_type->dispname);
  }
  return src;
}

/** Generates "receive(const COMPONENT_template&, COMPONENT *)" or
 * "receive(const ADDRESS_template&, ADDRESS *)".
 * These are called from PORT::any_receive for "any port.receive"
 *
 * @param def_ptr pointer to a string which will be written to the header
 * @param src_ptr pointer to a string which will be written as the source file
 * @param pdef pointer to a structure with information about the port
 * @param class_name
 * @param is_check   true if writing the check-receive member
 * @param is_trigger true if writing the trigger member
 * @param is_address true if the port has the "address" extension
 */
static void generate_generic_receive(char **def_ptr, char **src_ptr,
  const port_def *pdef, const char *class_name, boolean is_check,
  boolean is_trigger, boolean is_address)
{
  char *def = *def_ptr, *src = *src_ptr;
  const char *function_name, *operation_name, *failed_str, *failed_status;
  const char *sender_type = is_address ? pdef->address_name : "COMPONENT";
  const char *logger_operation;

  if (is_trigger) {
    function_name = "trigger";
    logger_operation = function_name;
    operation_name = "Trigger";
    failed_str = "will drop a message";
    failed_status = "ALT_REPEAT";
  } else {
    if (is_check) {
      function_name = "check_receive";
      logger_operation = "check__receive";
      operation_name = "Check-receive";
    } else {
      function_name = "receive";
      logger_operation = function_name;
      operation_name = "Receive";
    }
    failed_str = "failed";
    failed_status = "ALT_NO";
  }

  def = mputprintf(def, "alt_status %s(const %s_template& "
    "sender_template, %s *sender_ptr, Index_Redirect*);\n", function_name,
    sender_type, sender_type);
  src = mputprintf(src, "alt_status %s::%s(const %s_template& "
    "sender_template, %s *sender_ptr, Index_Redirect*)\n"
    "{\n"
    "msg_queue_item *my_head = (msg_queue_item*)msg_queue_head;\n"
    "if (msg_queue_head == NULL) {\n"
    "if (is_started) return ALT_MAYBE;\n"
    "else {\n"
    "TTCN_Logger::log(TTCN_Logger::MATCHING_PROBLEM, \"Matching on "
    "port %%s failed: Port is not started and the queue is empty.\","
    " port_name);\n"
    "return ALT_NO;\n"
    "}\n"
    "} else ", class_name, function_name, sender_type, sender_type);
  if (is_address) {
    src = mputprintf(src, "if (my_head->sender_component != "
      "SYSTEM_COMPREF) {\n"
      "TTCN_Logger::log(TTCN_Logger::MATCHING_MMUNSUCC, \"Matching "
      "on port %%s %s: Sender of the first message in the queue is "
      "not the system.\", port_name);\n", failed_str);
    if (is_trigger) src = mputstr(src, "remove_msg_queue_head();\n");
    src = mputprintf(src, "return %s;\n"
      "} else if (my_head->sender_address == NULL) {\n"
      "TTCN_error(\"%s operation on port %%s requires the address "
      "of the sender, which was not given by the test port.\", "
      "port_name);\n"
      "return ALT_NO;\n"
      "} else if (!sender_template.match("
      "*my_head->sender_address)) {\n"
      "if (TTCN_Logger::log_this_event(TTCN_Logger::MATCHING_MMUNSUCC)) "
      "{\n"
      "TTCN_Logger::begin_event(TTCN_Logger::MATCHING_MMUNSUCC);\n"
      "TTCN_Logger::log_event(\"Matching on port %%s %s: "
      "Sender address of the first message in the queue does not "
      "match the from clause: \", port_name);\n"
      "sender_template.log_match(*my_head->sender_address);\n"
      "TTCN_Logger::end_event();\n"
      "TTCN_Logger::log_matching_failure(TitanLoggerApiSimple::PortType::message__,\n"
      "port_name, my_head->sender_component,\n"
      "TitanLoggerApiSimple::MatchingFailureType_reason::message__does__not__match__template,\n"
      "(TTCN_Logger::begin_event(TTCN_Logger::MATCHING_MMUNSUCC, TRUE),"
      " sender_template.log_match(*my_head->sender_address),\n"
      " TTCN_Logger::end_event_log2str())"
      ");\n"
      "}\n", failed_status, operation_name, failed_str);
  } else {
    src = mputprintf(src, "if (!sender_template.match("
      "my_head->sender_component)) {\n"
      "const TTCN_Logger::Severity log_sev = "
      "my_head->sender_component==SYSTEM_COMPREF?"
      "TTCN_Logger::MATCHING_MMUNSUCC:TTCN_Logger::MATCHING_MCUNSUCC;\n"
      "if (TTCN_Logger::log_this_event(log_sev)) {\n"
      "TTCN_Logger::begin_event(log_sev);\n"
      "TTCN_Logger::log_event(\"Matching on port %%s %s: "
      "Sender of the first message in the queue does not match the "
      "from clause: \", port_name);\n"
      "sender_template.log_match(my_head->sender_component);\n"
      "TTCN_Logger::end_event();\n"
      "}\n", failed_str);
  }
  if (is_trigger) src = mputstr(src, "remove_msg_queue_head();\n");
  src = mputprintf(src, "return %s;\n"
    "} else {\n"
    "if (sender_ptr != NULL) *sender_ptr = %smy_head->%s;\n",
    failed_status, is_address ? "*" : "",
      is_address ? "sender_address" : "sender_component");

  if (is_address) {
    src = mputprintf(src, "TTCN_Logger::log(TTCN_Logger::MATCHING_MMSUCCESS"
      ", \"Matching on port %%s succeeded.\", port_name);\n"
      "if (TTCN_Logger::log_this_event(TTCN_Logger::PORTEVENT_MMRECV)) "
      "{\n"
      "TTCN_Logger::log_msgport_recv(port_name, TitanLoggerApiSimple::Msg__port__recv_operation::%s__op,\n"
      "SYSTEM_COMPREF, CHARSTRING(0, NULL),\n"
      "(TTCN_Logger::begin_event(TTCN_Logger::PORTEVENT_MMRECV, TRUE), "
      "my_head->sender_address->log(), TTCN_Logger::end_event_log2str()),\n"
      "msg_head_count+1);\n"
      "}\n", logger_operation);
  } else {
    size_t msg_idx;
    src = mputstr(src, "TTCN_Logger::log("
      "my_head->sender_component==SYSTEM_COMPREF?"
      "TTCN_Logger::MATCHING_MMSUCCESS:TTCN_Logger::MATCHING_MCSUCCESS"
      ", \"Matching on port %s succeeded.\", port_name);\n"
      "const TTCN_Logger::Severity log_sev = "
      "my_head->sender_component==SYSTEM_COMPREF?"
      "TTCN_Logger::PORTEVENT_MMRECV:TTCN_Logger::PORTEVENT_MCRECV;\n"
      "if (TTCN_Logger::log_this_event(log_sev)) {\n"
      "switch (my_head->item_selection) {\n");
    for (msg_idx = 0; msg_idx < pdef->msg_in.nElements; msg_idx++) {
      const port_msg_type *message_type = pdef->msg_in.elements + msg_idx;
      src = mputprintf(src,
        "case MESSAGE_%lu:\n"
        "TTCN_Logger::log_msgport_recv(port_name, TitanLoggerApiSimple::Msg__port__recv_operation::%s__op,\n"
        "my_head->sender_component, CHARSTRING(0, NULL),\n"
        "(TTCN_Logger::begin_event(log_sev,TRUE), TTCN_Logger::log_event_str(\": %s: \"),\n"
        "my_head->message_%lu->log(), TTCN_Logger::end_event_log2str()), msg_head_count+1);\n"
        "break;\n",
        (unsigned long)msg_idx, logger_operation, message_type->dispname, (unsigned long)msg_idx);
    }
    src = mputstr(src,
      "default:\n"
      "TTCN_error(\"Internal error: unknown message\");\n"
      "}\n"
      "}\n");
  }

  if (!is_check) src = mputstr(src, "remove_msg_queue_head();\n");
  src = mputstr(src, "return ALT_YES;\n"
    "}\n"
    "}\n\n");

  *def_ptr = def;
  *src_ptr = src;
}

static void generate_receive(char **def_ptr, char **src_ptr,
  const port_def *pdef, const char *class_name, size_t message_index,
  boolean is_check, boolean is_trigger, boolean is_address)
{
  char *def = *def_ptr, *src = *src_ptr;
  const port_msg_type *message_type = pdef->msg_in.elements + message_index;
  const char *function_name, *operation_name, *failed_str, *failed_status;
  const char *sender_type = is_address ? pdef->address_name : "COMPONENT";
  const char *logger_operation;

  if (is_trigger) {
    function_name = "trigger";
    logger_operation = function_name;
    operation_name = "Trigger";
    failed_str = "will drop a message";
    failed_status = "ALT_REPEAT";
  } else {
    if (is_check) {
      function_name = "check_receive";
      logger_operation = "check__receive";
      operation_name = "Check-receive";
    } else {
      function_name = "receive";
      logger_operation = function_name;
      operation_name = "Receive";
    }
    failed_str = "failed";
    failed_status = "ALT_NO";
  }

  def = mputprintf(def, "alt_status %s(const %s_template& value_template, "
    "%s *value_redirect, const %s_template& sender_template, %s "
    "*sender_ptr, Index_Redirect*);\n", function_name, message_type->name,
    use_runtime_2 ? "Value_Redirect_Interface" : message_type->name,
    sender_type, sender_type);

  src = mputprintf(src, "alt_status %s::%s(const %s_template& "
    "value_template, %s *value_redirect, const %s_template& "
    "sender_template, %s *sender_ptr, Index_Redirect*)\n"
    "{\n"
    "if (value_template.get_selection() == ANY_OR_OMIT) "
    "TTCN_error(\"%s operation using '*' as matching template\");\n"
    "msg_queue_item *my_head = (msg_queue_item*)msg_queue_head;\n"
    "if (msg_queue_head == NULL) {\n"
    "if (is_started) return ALT_MAYBE;\n"
    "else {\n"
    "TTCN_Logger::log(TTCN_Logger::MATCHING_PROBLEM, \"Matching on "
    "port %%s failed: Port is not started and the queue is empty.\", "
    "port_name);\n"
    "return ALT_NO;\n"
    "}\n"
    "} else ", class_name, function_name, message_type->name,
    use_runtime_2 ? "Value_Redirect_Interface" : message_type->name,
    sender_type, sender_type, operation_name);
  if (is_address) {
    src = mputprintf(src, "if (my_head->sender_component != "
      "SYSTEM_COMPREF) {\n"
      "TTCN_Logger::log(TTCN_Logger::MATCHING_MMUNSUCC, \"Matching "
      "on port %%s %s: Sender of the first message in the queue is "
      "not the system.\", port_name);\n", failed_str);
    if (is_trigger) src = mputstr(src, "remove_msg_queue_head();\n");
    src = mputprintf(src, "return %s;\n"
      "} else if (my_head->sender_address == NULL) {\n"
      "TTCN_error(\"%s operation on port %%s requires the address "
      "of the sender, which was not given by the test port.\", "
      "port_name);\n"
      "return ALT_NO;\n"
      "} else if (!sender_template.match("
      "*my_head->sender_address)) {\n"
      "if (TTCN_Logger::log_this_event(TTCN_Logger::MATCHING_MMUNSUCC)) "
      "{\n"
      "TTCN_Logger::begin_event(TTCN_Logger::MATCHING_MMUNSUCC);\n"
      "TTCN_Logger::log_event(\"Matching on port %%s %s: "
      "Sender address of the first message in the queue does not "
      "match the from clause: \", port_name);\n"
      "sender_template.log_match(*my_head->sender_address);\n"
      "TTCN_Logger::end_event();\n"
      "}\n", failed_status, operation_name, failed_str);
  } else {
    src = mputprintf(src, "if (!sender_template.match("
      "my_head->sender_component)) {\n"
      "const TTCN_Logger::Severity log_sev = "
      "my_head->sender_component==SYSTEM_COMPREF?"
      "TTCN_Logger::MATCHING_MMUNSUCC:TTCN_Logger::MATCHING_MCUNSUCC;\n"
      "if (TTCN_Logger::log_this_event(log_sev)) {\n"
      "TTCN_Logger::begin_event(log_sev);\n"
      "TTCN_Logger::log_event(\"Matching on port %%s %s: "
      "Sender of the first message in the queue does not match the "
      "from clause: \", port_name);\n"
      "sender_template.log_match(my_head->sender_component);\n"
      "TTCN_Logger::end_event();\n"
      "}\n", failed_str);
  }
  if (is_trigger) src = mputstr(src, "remove_msg_queue_head();\n");
  src = mputprintf(src, "return %s;\n"
    "} else if (my_head->item_selection != MESSAGE_%lu) {\n"
    "TTCN_Logger::log(%s, \"Matching on port %%s %s: "
    "Type of the first message in the queue is not %s.\", "
    "port_name);\n", failed_status, (unsigned long) message_index,
    is_address ? "TTCN_Logger::MATCHING_MMUNSUCC" :
      "my_head->sender_component==SYSTEM_COMPREF?"
      "TTCN_Logger::MATCHING_MMUNSUCC:TTCN_Logger::MATCHING_MCUNSUCC",
      failed_str, message_type->dispname);
  if (is_trigger) src = mputstr(src, "remove_msg_queue_head();\n");
  src = mputprintf(src, "return %s;\n"
    "} else if (!value_template.match(*my_head->message_%lu%s)) {\n",
    failed_status, (unsigned long) message_index,
    (omit_in_value_list ? ", TRUE" : ""));
  src = mputprintf(src,
    "const TTCN_Logger::Severity log_sev = %s;\n"
    "if (TTCN_Logger::log_this_event(log_sev)) {\n"
    "TTCN_Logger::log_matching_failure(TitanLoggerApiSimple::PortType::message__,\n"
    "port_name, my_head->sender_component,\n"
    "TitanLoggerApiSimple::MatchingFailureType_reason::message__does__not__match__template,\n"
    "(TTCN_Logger::begin_event(log_sev, TRUE),"
    " value_template.log_match(*my_head->message_%lu%s),\n"
    " TTCN_Logger::end_event_log2str())"
    ");\n"
    "}\n",
    (is_address ?
      "TTCN_Logger::MATCHING_MMUNSUCC" :
      "my_head->sender_component==SYSTEM_COMPREF ? "
      "TTCN_Logger::MATCHING_MMUNSUCC : TTCN_Logger::MATCHING_MCUNSUCC"),
      (unsigned long) message_index,
      (omit_in_value_list ? ", TRUE" : ""));
  if (is_trigger) src = mputstr(src, "remove_msg_queue_head();\n");
  src = mputprintf(src, "return %s;\n"
    "} else {\n"
    "if (value_redirect != NULL) {\n", failed_status);
  if (use_runtime_2) {
    src = mputprintf(src,
      "value_redirect->set_values(my_head->message_%lu);\n",
      (unsigned long) message_index);
  }
  else {
    src = mputprintf(src,
      "*value_redirect = *my_head->message_%lu;\n", (unsigned long) message_index);
  }
  src = mputprintf(src,
    "}\n"
    "if (sender_ptr != NULL) *sender_ptr = %smy_head->%s;\n",
    is_address ? "*" : "", is_address ? "sender_address" : "sender_component");

  if (is_address) {
    src = mputprintf(src,
      "if (TTCN_Logger::log_this_event(TTCN_Logger::MATCHING_MMSUCCESS)) "
      "{\n"
      "TTCN_Logger::log_matching_success(TitanLoggerApiSimple::PortType::message__,\n"
      "port_name, SYSTEM_COMPREF,\n"
      "(TTCN_Logger::begin_event(TTCN_Logger::MATCHING_MMSUCCESS, TRUE),"
      " value_template.log_match(*my_head->message_%lu%s),\n"
      " TTCN_Logger::end_event_log2str()));\n"
      "}\n"
      "if (TTCN_Logger::log_this_event(TTCN_Logger::PORTEVENT_MMRECV)) "
      "{\n"
      "TTCN_Logger::log_msgport_recv(port_name, "
      "TitanLoggerApiSimple::Msg__port__recv_operation::%s__op, my_head->sender_component,\n"
      "(TTCN_Logger::begin_event(TTCN_Logger::PORTEVENT_MMRECV,TRUE),"
      "my_head->sender_address->log(), TTCN_Logger::end_event_log2str()),\n"
      "(TTCN_Logger::begin_event(TTCN_Logger::PORTEVENT_MMRECV,TRUE),"
      " TTCN_Logger::log_event_str(\": %s : \"),\n"
      "my_head->message_%lu->log(), TTCN_Logger::end_event_log2str()),\n"
      "msg_head_count+1);\n"
      "}\n", (unsigned long) message_index,
      (omit_in_value_list ? ", TRUE" : ""), logger_operation,
      message_type->dispname, (unsigned long) message_index);
  } else {
    src = mputprintf(src,
      "TTCN_Logger::Severity log_sev = "
      "my_head->sender_component==SYSTEM_COMPREF?"
      "TTCN_Logger::MATCHING_MMSUCCESS:TTCN_Logger::MATCHING_MCSUCCESS;\n"
      "if (TTCN_Logger::log_this_event(log_sev)) {\n"
      "TTCN_Logger::log_matching_success(TitanLoggerApiSimple::PortType::message__,\n"
      "port_name, my_head->sender_component,\n"
      "(TTCN_Logger::begin_event(log_sev, TRUE),"
      " value_template.log_match(*my_head->message_%lu%s),\n"
      " TTCN_Logger::end_event_log2str()));\n"
      "}\n"
      "log_sev = my_head->sender_component==SYSTEM_COMPREF?"
      "TTCN_Logger::PORTEVENT_MMRECV:TTCN_Logger::PORTEVENT_MCRECV;\n"
      "if (TTCN_Logger::log_this_event(log_sev)) {\n"
      "TTCN_Logger::log_msgport_recv(port_name, "
      "TitanLoggerApiSimple::Msg__port__recv_operation::%s__op,\n"
      "my_head->sender_component, CHARSTRING(0, NULL),\n"
      "(TTCN_Logger::begin_event(log_sev,TRUE), TTCN_Logger::log_event_str(\": %s : \"),\n"
      "my_head->message_%lu->log(), TTCN_Logger::end_event_log2str()),\n"
      "msg_head_count+1);\n"
      "}\n", (unsigned long) message_index,
      (omit_in_value_list ? ", TRUE" : ""), logger_operation,
      message_type->dispname, (unsigned long) message_index);
  }

  if (!is_check) src = mputstr(src, "remove_msg_queue_head();\n");
  src = mputstr(src, "return ALT_YES;\n"
    "}\n"
    "}\n\n");

  *def_ptr = def;
  *src_ptr = src;
}

typedef enum { GETCALL, GETREPLY, CATCH } getop_t;
enum { NOT_ADDRESS = 0, IS_ADDRESS };
enum { NOT_CHECK = 0, IS_CHECK };

/** Writes (check_)?getcall, (check_)?getreply, get_exception and check_catch methods.
 * These are called from PORT::any(_check)?_(getcall|getreply|catch) and
 * PORT::check; they are the implementation of "any port.getcall" etc.
 *
 * @param getop the operation (call, reply or catch)
 * @param def_ptr pointer to a string which will be written to the header
 * @param src_ptr pointer to a string which will be written as the source file
 * @param pdef
 * @param class_name
 * @param is_check
 * @param is_address
 */
static void generate_generic_getop(getop_t getop,
  char **def_ptr, char **src_ptr, const port_def *pdef,
  const char *class_name, boolean is_check, boolean is_address)
{
  char *def = *def_ptr, *src = *src_ptr;
  const char *function_name = NULL, *operation_name = NULL,
    *entity_name_prefix = NULL, *entity_name = NULL;
  const char *sender_type = is_address ? pdef->address_name : "COMPONENT";
  size_t i;

  switch (getop) {
  case GETCALL:
    function_name  = is_check ? "check_getcall" : "getcall";
    operation_name = is_check ? "Check-getcall" : "Getcall";
    entity_name_prefix = "a";
    entity_name = "call";
    break;
  case GETREPLY:
    function_name  = is_check ? "check_getreply" : "getreply";
    operation_name = is_check ? "Check-getreply" : "Getreply";
    entity_name_prefix = "a";
    entity_name = "reply";
    break;
  case CATCH:
    function_name  = is_check ? "check_catch" : "get_exception";
    operation_name = is_check ? "Check-catch" : "Catch";
    entity_name_prefix = "an";
    entity_name = "exception";
  }

  def = mputprintf(def, "alt_status %s(const %s_template& "
    "sender_template, %s *sender_ptr, Index_Redirect*);\n", function_name, sender_type,
    sender_type);
  src = mputprintf(src, "alt_status %s::%s(const %s_template& "
    "sender_template, %s *sender_ptr, Index_Redirect*)\n"
    "{\n"
    "if (proc_queue_head == NULL) {\n"
    "if (is_started) return ALT_MAYBE;\n"
    "else {\n"
    "TTCN_Logger::log(TTCN_Logger::MATCHING_PROBLEM, \"Matching on "
    "port %%s failed: Port is not started and the queue is empty.\", "
    "port_name);\n"
    "return ALT_NO;\n"
    "}\n", class_name, function_name, sender_type, sender_type);
  if (is_address) {
    src = mputprintf(src, "} else if (proc_queue_head->sender_component "
      "!= SYSTEM_COMPREF) {\n"
      "TTCN_Logger::log(TTCN_Logger::MATCHING_PMUNSUCC, \"Matching "
      "on port %%s failed: Sender of the first entity in the queue is"
      " not the system.\", port_name);\n"
      "return ALT_NO;\n"
      "} else if (proc_queue_head->sender_address == NULL) {\n"
      "TTCN_error(\"%s operation on port %%s requires the address "
      "of the sender, which was not given by the test port.\", "
      "port_name);\n"
      "return ALT_NO;\n"
      "} else if (!sender_template.match("
      "*proc_queue_head->sender_address)) {\n"
      "if (TTCN_Logger::log_this_event(TTCN_Logger::MATCHING_PMUNSUCC)) "
      "{\n"
      "TTCN_Logger::log_matching_failure(TitanLoggerApiSimple::PortType::procedure__,\n"
      "port_name, SYSTEM_COMPREF,\n"
      "TitanLoggerApiSimple::MatchingFailureType_reason::sender__does__not__match__from__clause,\n"
      "(TTCN_Logger::begin_event(TTCN_Logger::MATCHING_PMUNSUCC, TRUE),"
      " sender_template.log_match(*proc_queue_head->sender_address), "
      " TTCN_Logger::end_event_log2str()));\n"
      "}\n"
      "return ALT_NO;\n", operation_name);
  } else {
    src = mputstr(src, "} else if (!sender_template.match("
      "proc_queue_head->sender_component)) {\n"
      "const TTCN_Logger::Severity log_sev = "
      "proc_queue_head->sender_component==SYSTEM_COMPREF?"
      "TTCN_Logger::MATCHING_PMUNSUCC:TTCN_Logger::MATCHING_PCUNSUCC;\n"
      "if (TTCN_Logger::log_this_event(log_sev)) {\n"
      "TTCN_Logger::begin_event(log_sev);\n"
      "TTCN_Logger::log_event(\"Matching on port %s failed: "
      "Sender of the first entity in the queue does not match the "
      "from clause: \", port_name);\n"
      "sender_template.log_match(proc_queue_head->sender_component);\n"
      "TTCN_Logger::end_event();\n"
      "}\n"
      "return ALT_NO;\n");
  }
  src = mputstr(src, "} else switch (proc_queue_head->item_selection) {\n");
  switch (getop) {
  case GETCALL:
    for (i = 0; i < pdef->proc_in.nElements; i++) {
      src = mputprintf(src, "case CALL_%lu:\n", (unsigned long) i);
    }
    break;
  case GETREPLY:
    for (i = 0; i < pdef->proc_out.nElements; i++) {
      if (!pdef->proc_out.elements[i].is_noblock)
        src = mputprintf(src, "case REPLY_%lu:\n", (unsigned long) i);
    }
    break;
  case CATCH:
    for (i = 0; i < pdef->proc_out.nElements; i++) {
      if (pdef->proc_out.elements[i].has_exceptions)
        src = mputprintf(src, "case EXCEPTION_%lu:\n",
          (unsigned long) i);
    }
  }
  src = mputstr(src, "{\n"
    "if (sender_ptr != NULL) *sender_ptr = ");
  if (is_address) src = mputstr(src, "*proc_queue_head->sender_address;\n");
  else src = mputstr(src, "proc_queue_head->sender_component;\n");

  if (is_address) {
    src = mputprintf(src, "TTCN_Logger::log("
      "TTCN_Logger::MATCHING_PMSUCCESS, \"Matching on port %%s "
      "succeeded.\", port_name);\n"
      "if (TTCN_Logger::log_this_event(TTCN_Logger::PORTEVENT_PMIN)) {\n"
      "TTCN_Logger::log_procport_recv(port_name, "
      "TitanLoggerApiSimple::Port__oper::%s__op, proc_queue_head->sender_component,\n"
      "%s, CHARSTRING(0, NULL), msg_head_count+1);\n"
      "}\n", entity_name, (is_check ? "TRUE" : "FALSE"));
  } else {
    src = mputprintf(src, "TTCN_Logger::log("
      "proc_queue_head->sender_component==SYSTEM_COMPREF?"
      "TTCN_Logger::MATCHING_PMSUCCESS:TTCN_Logger::MATCHING_PCSUCCESS"
      ", \"Matching on port %%s succeeded.\", port_name);\n"
      "const TTCN_Logger::Severity log_sev = "
      "proc_queue_head->sender_component==SYSTEM_COMPREF?"
      "TTCN_Logger::PORTEVENT_PMIN:TTCN_Logger::PORTEVENT_PCIN;\n"
      "if (TTCN_Logger::log_this_event(log_sev)) {\n"
      "TTCN_Logger::log_procport_recv(port_name, "
      "TitanLoggerApiSimple::Port__oper::%s__op, proc_queue_head->sender_component,\n"
      "%s, CHARSTRING(0, NULL), msg_head_count+1);\n"
      "}\n", entity_name, (is_check ? "TRUE" : "FALSE"));
  }

  if (!is_check) src = mputstr(src, "remove_proc_queue_head();\n");
  src = mputprintf(src, "return ALT_YES;\n"
    "}\n"
    "default:\n"
    "TTCN_Logger::log(%s, \"Matching on port %%s "
    "failed: First entity in the queue is not %s %s.\", port_name);\n"
    "return ALT_NO;\n"
    "}\n"
    "}\n\n",
    is_address ? "TTCN_Logger::MATCHING_PMUNSUCC" :
      "proc_queue_head->sender_component==SYSTEM_COMPREF?"
      "TTCN_Logger::MATCHING_PMUNSUCC:TTCN_Logger::MATCHING_PCUNSUCC",
      entity_name_prefix, entity_name);

  *def_ptr = def;
  *src_ptr = src;
}


/** Generate code for logging
 *
 * Called from generate_getcall, generate_getreply, generate_catch
 *
 * @param src_ptr
 * @param op_str "call", "reply" or "exception"
 * @param match_str "catch_template.log_match", "getcall_template.log_match_call"
 *               or "getreply_template.log_match_reply"
 * @param is_address
 * @param is_check
 * @param signature_index
 */
static void generate_proc_incoming_data_logging(char **src_ptr,
  const char *op_str, const char *match_str, boolean is_address,
  boolean is_check, size_t signature_index)
{
  if (is_address) {
    *src_ptr = mputprintf(*src_ptr,
      "if (TTCN_Logger::log_this_event(TTCN_Logger::MATCHING_PMSUCCESS)) "
      "{\n"
      "TTCN_Logger::log_matching_success(TitanLoggerApiSimple::PortType::procedure__,\n"
      "port_name, SYSTEM_COMPREF,\n"
      "(TTCN_Logger::begin_event(TTCN_Logger::MATCHING_PMSUCCESS, TRUE),"
      " %s(*proc_queue_head->%s_%lu%s),\n"
      " TTCN_Logger::end_event_log2str()));\n"
      "}\n"
      "if (TTCN_Logger::log_this_event(TTCN_Logger::PORTEVENT_PMIN)) "
      "{\n"
      "TTCN_Logger::log_procport_recv(port_name, "
      "TitanLoggerApiSimple::Port__oper::%s__op, proc_queue_head->sender_component, %s,\n"
      "(TTCN_Logger::begin_event(TTCN_Logger::PORTEVENT_PMIN, TRUE),"
      " proc_queue_head->%s_%lu->log(),"
      " TTCN_Logger::end_event_log2str()),\n"
      "msg_head_count+1);\n"
      "}\n", match_str, op_str, (unsigned long) signature_index,
      (omit_in_value_list ? ", TRUE" : ""),
      op_str, (is_check ? "TRUE" : "FALSE"), op_str, (unsigned long) signature_index);
  } else {
    *src_ptr = mputprintf(*src_ptr, "TTCN_Logger::Severity log_sev = "
      "proc_queue_head->sender_component==SYSTEM_COMPREF?"
      "TTCN_Logger::MATCHING_PMSUCCESS:TTCN_Logger::MATCHING_PCSUCCESS;\n"
      "if (TTCN_Logger::log_this_event(log_sev)) {\n"
      "TTCN_Logger::begin_event(log_sev);\n"
      "TTCN_Logger::log_event(\"Matching on port %%s "
      "succeeded: \", port_name);\n"
      "%s(*proc_queue_head->%s_%lu);\n"
      "TTCN_Logger::end_event();\n"
      "}\n"
      "log_sev = proc_queue_head->sender_component==SYSTEM_COMPREF?"
      "TTCN_Logger::PORTEVENT_PMIN:TTCN_Logger::PORTEVENT_PCIN;\n"
      "if (TTCN_Logger::log_this_event(log_sev)) {\n"
      "TTCN_Logger::log_procport_recv(port_name, "
      "TitanLoggerApiSimple::Port__oper::%s__op, proc_queue_head->sender_component, %s,\n"
      "(TTCN_Logger::begin_event(log_sev, TRUE),"
      " proc_queue_head->%s_%lu->log(),"
      " TTCN_Logger::end_event_log2str()),\n"
      "msg_head_count+1"
      ");\n"
      "}\n", match_str, op_str, (unsigned long) signature_index,
      op_str, (is_check ? "TRUE" : "FALSE"),
      op_str, (unsigned long) signature_index);
  }
}

static void generate_getcall(char **def_ptr, char **src_ptr,
  const port_def *pdef, const char *class_name, size_t signature_index,
  boolean is_check, boolean is_address)
{
  char *def = *def_ptr, *src = *src_ptr;
  const port_proc_signature *signature =
    pdef->proc_in.elements + signature_index;
  const char *function_name = is_check ? "check_getcall" : "getcall";
  const char *operation_name = is_check ? "Check-getcall" : "Getcall";
  const char *sender_type = is_address ? pdef->address_name : "COMPONENT";

  def = mputprintf(def, "alt_status %s(const %s_template& "
    "getcall_template, const %s_template& sender_template, "
    "const %s_call_redirect& param_ref, %s *sender_ptr, Index_Redirect*);\n",
    function_name, signature->name, sender_type, signature->name,
    sender_type);

  src = mputprintf(src, "alt_status %s::%s(const %s_template& "
    "getcall_template, const %s_template& sender_template, "
    "const %s_call_redirect& param_ref, %s *sender_ptr, Index_Redirect*)\n"
    "{\n"
    "if (proc_queue_head == NULL) {\n"
    "if (is_started) return ALT_MAYBE;\n"
    "else {\n"
    "TTCN_Logger::log(TTCN_Logger::MATCHING_PROBLEM, \"Matching on "
    "port %%s failed: Port is not started and the queue is empty.\", "
    "port_name);\n"
    "return ALT_NO;\n"
    "}\n", class_name, function_name, signature->name, sender_type,
    signature->name, sender_type);
  if (is_address) {
    src = mputprintf(src,
      "} else if (proc_queue_head->sender_component != SYSTEM_COMPREF) "
      "{\n"
      "TTCN_Logger::log_matching_failure(TitanLoggerApiSimple::PortType::procedure__,\n"
      "port_name, proc_queue_head->sender_component,\n"
      "TitanLoggerApiSimple::MatchingFailureType_reason::sender__is__not__system ,\n"
      "CHARSTRING(0,NULL));\n"
      "return ALT_NO;\n"
      "} else if (proc_queue_head->sender_address == NULL) {\n"
      "TTCN_error(\"%s operation on port %%s requires the address "
      "of the sender, which was not given by the test port.\", "
      "port_name);\n"
      "return ALT_NO;\n"
      "} else if (!sender_template.match("
      "*proc_queue_head->sender_address)) {\n"
      "if (TTCN_Logger::log_this_event(TTCN_Logger::MATCHING_PMUNSUCC)) "
      "{\n"
      "TTCN_Logger::log_matching_failure(TitanLoggerApiSimple::PortType::procedure__,\n"
      "port_name, SYSTEM_COMPREF,\n"
      "TitanLoggerApiSimple::MatchingFailureType_reason::sender__does__not__match__from__clause,\n"
      "(TTCN_Logger::begin_event(TTCN_Logger::MATCHING_PMUNSUCC, TRUE),"
      " sender_template.log_match(*proc_queue_head->sender_address), "
      " TTCN_Logger::end_event_log2str()));\n"
      "}\n"
      "return ALT_NO;\n", operation_name);
  } else {
    src = mputstr(src, "} else if (!sender_template.match("
      "proc_queue_head->sender_component)) {\n"
      "const TTCN_Logger::Severity log_sev = "
      "proc_queue_head->sender_component==SYSTEM_COMPREF ? "
      "TTCN_Logger::MATCHING_PMUNSUCC : TTCN_Logger::MATCHING_PCUNSUCC;\n"
      "if (TTCN_Logger::log_this_event(log_sev)) {\n"
      "TTCN_Logger::log_matching_failure(TitanLoggerApiSimple::PortType::procedure__,\n"
      "port_name, proc_queue_head->sender_component,\n"
      "TitanLoggerApiSimple::MatchingFailureType_reason::sender__does__not__match__from__clause,\n"
      "(TTCN_Logger::begin_event(log_sev, TRUE),"
      " sender_template.log_match(proc_queue_head->sender_component), "
      " TTCN_Logger::end_event_log2str()));\n"
      "}\n"
      "return ALT_NO;\n");
  }
  src = mputprintf(src,
    "} else if (proc_queue_head->item_selection != CALL_%lu) {\n"
    "TTCN_Logger::log(%s, \"Matching on port %%s "
    "failed: The first entity in the queue is not a call for "
    "signature %s.\", port_name);\n"
    "return ALT_NO;\n"
    "} else if (!getcall_template.match_call"
    "(*proc_queue_head->call_%lu%s)) {\n",
    (unsigned long) signature_index,
    is_address ? "TTCN_Logger::MATCHING_PMUNSUCC" :
      "proc_queue_head->sender_component==SYSTEM_COMPREF ? "
      "TTCN_Logger::MATCHING_PMUNSUCC:TTCN_Logger::MATCHING_PCUNSUCC",
      signature->dispname, (unsigned long) signature_index,
      (omit_in_value_list ? ", TRUE" : ""));
  src = mputprintf(src,
    "const TTCN_Logger::Severity log_sev = %s;\n"
    "if (TTCN_Logger::log_this_event(log_sev)) {\n"
    "TTCN_Logger::log_matching_failure(TitanLoggerApiSimple::PortType::procedure__,\n"
    "port_name, proc_queue_head->sender_component,\n"
    "TitanLoggerApiSimple::MatchingFailureType_reason::parameters__of__call__do__not__match__template,\n"
    "(TTCN_Logger::begin_event(log_sev, TRUE),"
    " getcall_template.log_match_call(*proc_queue_head->call_%lu%s),"
    " TTCN_Logger::end_event_log2str()));\n"
    "}\n"
    "return ALT_NO;\n"
    "} else {\n"
    "param_ref.set_parameters(*proc_queue_head->call_%lu);\n"
    "if (sender_ptr != NULL) *sender_ptr = ",
    (is_address ?
      "TTCN_Logger::MATCHING_PMUNSUCC" :
      "proc_queue_head->sender_component==SYSTEM_COMPREF ? "
      "TTCN_Logger::MATCHING_PMUNSUCC : TTCN_Logger::MATCHING_PCUNSUCC"),
    (unsigned long) signature_index,
    (omit_in_value_list ? ", TRUE" : ""),
    (unsigned long) signature_index);
  if (is_address) src = mputstr(src, "*proc_queue_head->sender_address;\n");
  else src = mputstr(src, "proc_queue_head->sender_component;\n");

  generate_proc_incoming_data_logging(&src, "call",
    "getcall_template.log_match_call", is_address, is_check, signature_index);

  if (!is_check) src = mputstr(src, "remove_proc_queue_head();\n");
  src = mputstr(src,
    "return ALT_YES;\n"
    "}\n"
    "}\n\n");

  *def_ptr = def;
  *src_ptr = src;
}

static void generate_getreply(char **def_ptr, char **src_ptr,
  const port_def *pdef, const char *class_name, size_t signature_index,
  boolean is_check, boolean is_address)
{
  char *def = *def_ptr, *src = *src_ptr;
  const port_proc_signature *signature =
    pdef->proc_out.elements + signature_index;
  const char *function_name = is_check ? "check_getreply" : "getreply";
  const char *operation_name = is_check ? "Check-getreply" : "Getreply";
  const char *sender_type = is_address ? pdef->address_name : "COMPONENT";

  def = mputprintf(def, "alt_status %s(const %s_template& "
    "getreply_template, const %s_template& sender_template, "
    "const %s_reply_redirect& param_ref, %s *sender_ptr, Index_Redirect*);\n",
    function_name, signature->name, sender_type, signature->name,
    sender_type);

  src = mputprintf(src, "alt_status %s::%s(const %s_template& "
    "getreply_template, const %s_template& sender_template, "
    "const %s_reply_redirect& param_ref, %s *sender_ptr, Index_Redirect*)\n"
    "{\n", class_name, function_name, signature->name, sender_type,
    signature->name, sender_type);
  if (signature->has_return_value) {
    src = mputprintf(src, 
      "if (getreply_template.return_value().get_selection() == ANY_OR_OMIT) "
      "TTCN_error(\"%s operation using '*' as return value matching template\");\n",
      operation_name);
  }
  src = mputstr(src,
    "if (proc_queue_head == NULL) {\n"
    "if (is_started) return ALT_MAYBE;\n"
    "else {\n"
    "TTCN_Logger::log(TTCN_Logger::MATCHING_PROBLEM, \"Matching on "
    "port %s failed: Port is not started and the queue is empty.\", "
    "port_name);\n"
    "return ALT_NO;\n"
    "}\n");
  if (is_address) {
    src = mputprintf(src,
      "} else if (proc_queue_head->sender_component != SYSTEM_COMPREF) "
      "{\n"
      "TTCN_Logger::log(TTCN_Logger::MATCHING_PMUNSUCC, \"Matching "
      "on port %%s failed: Sender of the first entity in the queue "
      "is not the system.\", port_name);\n"
      "return ALT_NO;\n"
      "} else if (proc_queue_head->sender_address == NULL) {\n"
      "TTCN_error(\"%s operation on port %%s requires the address "
      "of the sender, which was not given by the test port.\", "
      "port_name);\n"
      "return ALT_NO;\n"
      "} else if (!sender_template.match("
      "*proc_queue_head->sender_address)) {\n"
      "if (TTCN_Logger::log_this_event(TTCN_Logger::MATCHING_PMUNSUCC)) "
      "{\n"
      "TTCN_Logger::log_matching_failure(TitanLoggerApiSimple::PortType::procedure__,\n"
      "port_name, SYSTEM_COMPREF,\n"
      "TitanLoggerApiSimple::MatchingFailureType_reason::sender__does__not__match__from__clause,\n"
      "(TTCN_Logger::begin_event(TTCN_Logger::MATCHING_PMUNSUCC, TRUE),"
      " sender_template.log_match(*proc_queue_head->sender_address), "
      " TTCN_Logger::end_event_log2str()));\n"
      "}\n"
      "return ALT_NO;\n", operation_name);
  } else {
    src = mputstr(src, "} else if (!sender_template.match("
      "proc_queue_head->sender_component)) {\n"
      "const TTCN_Logger::Severity log_sev = "
      "proc_queue_head->sender_component==SYSTEM_COMPREF?"
      "TTCN_Logger::MATCHING_PMUNSUCC:TTCN_Logger::MATCHING_PCUNSUCC;\n"
      "if (TTCN_Logger::log_this_event(log_sev)) {\n"
      "TTCN_Logger::begin_event(log_sev);\n"
      "TTCN_Logger::log_event(\"Matching on port %s failed: "
      "Sender of the first entity in the queue does not match the "
      "from clause: \", port_name);\n"
      "sender_template.log_match(proc_queue_head->sender_component);\n"
      "TTCN_Logger::end_event();\n"
      "}\n"
      "return ALT_NO;\n");
  }
  src = mputprintf(src,
    "} else if (proc_queue_head->item_selection != REPLY_%lu) {\n"
    "TTCN_Logger::log(%s, \"Matching on port %%s "
    "failed: The first entity in the queue is not a reply for "
    "signature %s.\", port_name);\n"
    "return ALT_NO;\n"
    "} else if (!getreply_template.match_reply"
    "(*proc_queue_head->reply_%lu%s)) {\n", (unsigned long) signature_index,
    is_address ? "TTCN_Logger::MATCHING_PMUNSUCC" :
      "proc_queue_head->sender_component==SYSTEM_COMPREF?"
      "TTCN_Logger::MATCHING_PMUNSUCC:TTCN_Logger::MATCHING_PCUNSUCC",
      signature->dispname, (unsigned long) signature_index,
      (omit_in_value_list ? ", TRUE" : ""));

  src = mputprintf(src,
    "const TTCN_Logger::Severity log_sev = %s;\n"
    "if (TTCN_Logger::log_this_event(log_sev)) {\n"
    "TTCN_Logger::log_matching_failure(TitanLoggerApiSimple::PortType::procedure__,\n"
    "port_name, proc_queue_head->sender_component,\n"
    "TitanLoggerApiSimple::MatchingFailureType_reason::parameters__of__reply__do__not__match__template,\n"
    "(TTCN_Logger::begin_event(log_sev, TRUE),"
    " getreply_template.log_match_reply(*proc_queue_head->reply_%lu%s), "
    " TTCN_Logger::end_event_log2str()));\n"
    "}\n"
    "return ALT_NO;\n"
    "} else {\n"
    "param_ref.set_parameters(*proc_queue_head->reply_%lu);\n"
    "if (sender_ptr != NULL) *sender_ptr = ",
    (is_address ?
      "TTCN_Logger::MATCHING_PMUNSUCC" :
      "proc_queue_head->sender_component==SYSTEM_COMPREF ? "
      "TTCN_Logger::MATCHING_PMUNSUCC : TTCN_Logger::MATCHING_PCUNSUCC"),
      (unsigned long) signature_index,
      (omit_in_value_list ? ", TRUE" : ""),
      (unsigned long) signature_index);
  if (is_address) src = mputstr(src, "*proc_queue_head->sender_address;\n");
  else src = mputstr(src, "proc_queue_head->sender_component;\n");

  generate_proc_incoming_data_logging(&src, "reply",
    "getreply_template.log_match_reply", is_address, is_check, signature_index);

  if (!is_check) src = mputstr(src, "remove_proc_queue_head();\n");
  src = mputstr(src,
    "return ALT_YES;\n"
    "}\n"
    "}\n\n");

  *def_ptr = def;
  *src_ptr = src;
}

static void generate_catch(char **def_ptr, char **src_ptr,
  const port_def *pdef, const char *class_name, size_t signature_index,
  boolean is_check, boolean is_address)
{
  char *def = *def_ptr, *src = *src_ptr;
  const port_proc_signature *signature =
    pdef->proc_out.elements + signature_index;
  const char *function_name = is_check ? "check_catch" : "get_exception";
  const char *operation_name = is_check ? "Check-catch" : "Catch";
  const char *sender_type = is_address ? pdef->address_name : "COMPONENT";

  def = mputprintf(def, "alt_status %s(const %s_exception_template& "
    "catch_template, const %s_template& sender_template, "
    "%s *sender_ptr, Index_Redirect*);\n",
    function_name, signature->name, sender_type, sender_type);

  src = mputprintf(src, "alt_status %s::%s(const %s_exception_template& "
    "catch_template, const %s_template& sender_template, "
    "%s *sender_ptr, Index_Redirect*)\n"
    "{\n"
    "if (catch_template.is_any_or_omit()) TTCN_error(\"%s operation using '*' "
    "as matching template\");\n"
    "if (proc_queue_head == NULL) {\n"
    "if (is_started) return ALT_MAYBE;\n"
    "else {\n"
    "TTCN_Logger::log(TTCN_Logger::MATCHING_PROBLEM, \"Matching on "
    "port %%s failed: Port is not started and the queue is empty.\", "
    "port_name);\n"
    "return ALT_NO;\n"
    "}\n", class_name, function_name, signature->name, sender_type,
    sender_type, operation_name);
  if (is_address) {
    src = mputprintf(src,
      "} else if (proc_queue_head->sender_component != SYSTEM_COMPREF) "
      "{\n"
      "TTCN_Logger::log_matching_failure(TitanLoggerApiSimple::PortType::procedure__,\n"
      "port_name, proc_queue_head->sender_component,\n"
      "TitanLoggerApiSimple::MatchingFailureType_reason::sender__is__not__system,\n"
      "CHARSTRING(0, NULL));\n"

      "return ALT_NO;\n"
      "} else if (proc_queue_head->sender_address == NULL) {\n"
      "TTCN_error(\"%s operation on port %%s requires the address "
      "of the sender, which was not given by the test port.\", "
      "port_name);\n"
      "return ALT_NO;\n"
      "} else if (!sender_template.match("
      "*proc_queue_head->sender_address)) {\n"
      "if (TTCN_Logger::log_this_event(TTCN_Logger::MATCHING_PMUNSUCC)) "
      "{\n"
      "TTCN_Logger::log_matching_failure(TitanLoggerApiSimple::PortType::procedure__,\n"
      "port_name, proc_queue_head->sender_component,\n"
      "TitanLoggerApiSimple::MatchingFailureType_reason::sender__does__not__match__from__clause,\n"
      "(TTCN_Logger::begin_event(TTCN_Logger::MATCHING_PMUNSUCC, TRUE),\n"
      " sender_template.log_match(*proc_queue_head->sender_address),\n"
      " TTCN_Logger::end_event_log2str()));\n"
      "}\n"
      "return ALT_NO;\n", operation_name);
  } else {
    src = mputstr(src, "} else if (!sender_template.match("
      "proc_queue_head->sender_component)) {\n"
      "const TTCN_Logger::Severity log_sev = "
      "proc_queue_head->sender_component==SYSTEM_COMPREF?"
      "TTCN_Logger::MATCHING_PMUNSUCC:TTCN_Logger::MATCHING_PCUNSUCC;\n"
      "if (TTCN_Logger::log_this_event(log_sev)) {\n"
      "TTCN_Logger::log_matching_failure(TitanLoggerApiSimple::PortType::procedure__,\n"
      "port_name, proc_queue_head->sender_component,\n"
      "TitanLoggerApiSimple::MatchingFailureType_reason::sender__does__not__match__from__clause,\n"
      "(TTCN_Logger::begin_event(log_sev, TRUE),\n"
      " sender_template.log_match(proc_queue_head->sender_component),\n"
      " TTCN_Logger::end_event_log2str()));\n"
      "}\n"
      "return ALT_NO;\n");
  }
  src = mputprintf(src,
    "} else if (proc_queue_head->item_selection != EXCEPTION_%lu) {\n"
    "TTCN_Logger::log_matching_failure(TitanLoggerApiSimple::PortType::procedure__,\n"
    "port_name, %s,\n"
    "TitanLoggerApiSimple::MatchingFailureType_reason::not__an__exception__for__signature,\n"
    "CHARSTRING(\"%s\"));\n"
    "return ALT_NO;\n"
    "} else if (!catch_template.match"
    "(*proc_queue_head->exception_%lu%s)) {\n",
    (unsigned long) signature_index,
    (is_address ? "SYSTEM_COMPREF": "proc_queue_head->sender_component"),
    signature->dispname, (unsigned long) signature_index,
    (omit_in_value_list ? ", TRUE" : ""));
  if (is_address) {
    src = mputstr(src, "if (TTCN_Logger::log_this_event("
      "TTCN_Logger::MATCHING_PMUNSUCC)) {\n");
  } else {
    src = mputstr(src, "const TTCN_Logger::Severity log_sev = "
      "proc_queue_head->sender_component==SYSTEM_COMPREF?"
      "TTCN_Logger::MATCHING_PMUNSUCC:TTCN_Logger::MATCHING_PCUNSUCC;\n"
      "if (TTCN_Logger::log_this_event(log_sev)) {\n");
  }
  src = mputprintf(src,
    "TTCN_Logger::log_matching_failure(TitanLoggerApiSimple::PortType::procedure__,\n"
    "port_name, proc_queue_head->sender_component,\n"
    "TitanLoggerApiSimple::MatchingFailureType_reason::exception__does__not__match__template,\n"
    "(TTCN_Logger::begin_event(%s, TRUE),\n"
    " catch_template.log_match(*proc_queue_head->exception_%lu%s),\n"
    " TTCN_Logger::end_event_log2str()));\n"
    "}\n"
    "return ALT_NO;\n"
    "} else {\n"
    "catch_template.set_value(*proc_queue_head->exception_%lu);\n"
    "if (sender_ptr != NULL) *sender_ptr = ",
    (is_address ? "TTCN_Logger::MATCHING_PMUNSUCC" : "log_sev"),
    (unsigned long) signature_index,
    (omit_in_value_list ? ", TRUE" : ""),
    (unsigned long) signature_index);
  if (is_address) src = mputstr(src, "*proc_queue_head->sender_address;\n");
  else src = mputstr(src, "proc_queue_head->sender_component;\n");

  generate_proc_incoming_data_logging(&src, "exception",
    "catch_template.log_match", is_address, is_check, signature_index);

  if (!is_check) src = mputstr(src, "remove_proc_queue_head();\n");
  src = mputstr(src,
    "return ALT_YES;\n"
    "}\n"
    "}\n\n");

  *def_ptr = def;
  *src_ptr = src;
}

#ifdef __SUNPRO_C
#define SUNPRO_PUBLIC  "public:\n"
#define SUNPRO_PRIVATE "private:\n"
#else
#define SUNPRO_PUBLIC
#define SUNPRO_PRIVATE
#endif


void defPortClass(const port_def* pdef, output_struct* output)
{
  char *def = NULL, *src = NULL;
  char *class_name, *base_class_name;
  size_t i;
  boolean has_incoming_call, has_incoming_reply, has_incoming_exception;
  boolean has_msg_queue, has_proc_queue;

  if (pdef->testport_type == INTERNAL) {
    class_name = mcopystr(pdef->name);
    base_class_name = mcopystr("PORT");
  } else {
    switch (pdef->port_type) {
    case USER:
      if (pdef->legacy) {
        class_name = mcopystr(pdef->name);
        // legacy always has one provider_name
        base_class_name = mprintf("%s_PROVIDER", pdef->provider_msg_outlist.elements[0].name);
        break;
      }
      // else fall through
    case REGULAR:
      class_name = mprintf("%s_BASE", pdef->name);
      base_class_name = mcopystr("PORT");
      break;
    case PROVIDER:
      class_name = mcopystr(pdef->name);
      base_class_name = mprintf("%s_PROVIDER", pdef->name);
      break;
    default:
      FATAL_ERROR("defPortClass(): invalid port type");
    }
  }

  has_incoming_call = pdef->proc_in.nElements > 0;

  has_incoming_reply = FALSE;
  for (i = 0; i < pdef->proc_out.nElements; i++) {
    if (!pdef->proc_out.elements[i].is_noblock) {
      has_incoming_reply = TRUE;
      break;
    }
  }

  has_incoming_exception = FALSE;
  for (i = 0; i < pdef->proc_out.nElements; i++) {
    if (pdef->proc_out.elements[i].has_exceptions) {
      has_incoming_exception = TRUE;
      break;
    }
  }

  has_msg_queue = pdef->msg_in.nElements > 0;
  has_proc_queue = has_incoming_call || has_incoming_reply ||
    has_incoming_exception;

  def = mprintf("class %s : public %s {\n", class_name, base_class_name);

  /* private data types and member functions for queue management */
  if (has_msg_queue) {
    /* data structures for the message queue */
    def = mputstr(def,
      SUNPRO_PUBLIC
      "enum msg_selection { ");
    for (i = 0; i < pdef->msg_in.nElements; i++) {
      if (i > 0) def = mputstr(def, ", ");
      def = mputprintf(def, "MESSAGE_%lu", (unsigned long) i);
    }
    def = mputstr(def, " };\n"
      SUNPRO_PRIVATE
      "struct msg_queue_item : public msg_queue_item_base {\n"
      "msg_selection item_selection;\n"
      "union {\n");
    for (i = 0; i < pdef->msg_in.nElements; i++)
      def = mputprintf(def, "%s *message_%lu;\n",
        pdef->msg_in.elements[i].name, (unsigned long) i);

    def = mputstr(def, "};\n"
      "component sender_component;\n");
    if (pdef->testport_type == ADDRESS) {
      def = mputprintf(def, "%s *sender_address;\n", pdef->address_name);
    }
    def = mputstr(def, "};\n\n");
    if (pdef->has_sliding) {
      def = mputprintf(def, "OCTETSTRING sliding_buffer;\n");
    }
    def = mputstr(def, "void remove_msg_queue_head();\n");
    src = mputprintf(src,
      "void %s::remove_msg_queue_head()\n"
      "{\n"
      "msg_queue_item *my_head = (msg_queue_item*)msg_queue_head;\n"
      "switch (my_head->item_selection) {\n", class_name);
    for (i = 0; i < pdef->msg_in.nElements; i++)
      src = mputprintf(src, "case MESSAGE_%lu:\n"
        "delete (my_head)->message_%lu;\n"
        "break;\n", (unsigned long) i, (unsigned long) i);
    src = mputstr(src, "default:\n"
      "TTCN_error(\"Internal error: Invalid message selector in the "
      "queue of port %s.\", port_name);\n"
      "}\n");
    if (pdef->testport_type == ADDRESS) {
      src = mputstr(src, "delete ((msg_queue_item*)msg_queue_head)->sender_address;\n");
    }
    src = mputstr(src,
      "msg_queue_item_base *next_item = msg_queue_head->next_item;\n"
      /* Make sure to delete the derived class; no virtual destructor. */
      "delete (msg_queue_item*)msg_queue_head;\n"
      "msg_queue_head = next_item;\n"
      "if (next_item == NULL) msg_queue_tail = NULL;\n"
      "TTCN_Logger::log_port_queue(TitanLoggerApiSimple::Port__Queue_operation::extract__msg, "
      "port_name, 0, ++msg_head_count, CHARSTRING(0,NULL), CHARSTRING(0,NULL));"
      "}\n\n");
  } /* if has_msg_queue */

  if (has_proc_queue) {
    /* data structures for the procedure queue */
    boolean is_first = TRUE;
    def = mputstr(def,
      SUNPRO_PUBLIC
      "enum proc_selection { ");
    for (i = 0; i < pdef->proc_in.nElements; i++) {
      if (is_first) is_first = FALSE;
      else def = mputstr(def, ", ");
      def = mputprintf(def, "CALL_%lu", (unsigned long) i);
    }
    for (i = 0; i < pdef->proc_out.nElements; i++) {
      if (pdef->proc_out.elements[i].is_noblock) continue;
      if (is_first) is_first = FALSE;
      else def = mputstr(def, ", ");
      def = mputprintf(def, "REPLY_%lu", (unsigned long) i);
    }
    for (i = 0; i < pdef->proc_out.nElements; i++) {
      if (!pdef->proc_out.elements[i].has_exceptions) continue;
      if (is_first) is_first = FALSE;
      else def = mputstr(def, ", ");
      def = mputprintf(def, "EXCEPTION_%lu", (unsigned long) i);
    }
    def = mputstr(def, " };\n"
      SUNPRO_PRIVATE
      "struct proc_queue_item {\n"
      "proc_selection item_selection;\n"
      "union {\n");
    for (i = 0; i < pdef->proc_in.nElements; i++) {
      def = mputprintf(def, "%s_call *call_%lu;\n",
        pdef->proc_in.elements[i].name, (unsigned long) i);
    }
    for (i = 0; i < pdef->proc_out.nElements; i++) {
      if (pdef->proc_out.elements[i].is_noblock) continue;
      def = mputprintf(def, "%s_reply *reply_%lu;\n",
        pdef->proc_out.elements[i].name, (unsigned long) i);
    }
    for (i = 0; i < pdef->proc_out.nElements; i++) {
      if (!pdef->proc_out.elements[i].has_exceptions) continue;
      def = mputprintf(def, "%s_exception *exception_%lu;\n",
        pdef->proc_out.elements[i].name, (unsigned long) i);
    }
    def = mputstr(def, "};\n"
      "component sender_component;\n");
    if (pdef->testport_type == ADDRESS) {
      def = mputprintf(def, "%s *sender_address;\n", pdef->address_name);
    }
    def = mputstr(def, "proc_queue_item *next_item;\n"
      "} *proc_queue_head, *proc_queue_tail;\n");

    def = mputstr(def, "void append_to_proc_queue("
      "proc_queue_item *new_item);\n");
    src = mputprintf(src, "void %s::append_to_proc_queue("
      "proc_queue_item *new_item)\n"
      "{\n"
      "new_item->next_item = NULL;\n"
      "if (proc_queue_tail != NULL) "
      "proc_queue_tail->next_item = new_item;\n"
      "else proc_queue_head = new_item;\n"
      "proc_queue_tail = new_item;\n"
      "}\n\n", class_name);

    def = mputstr(def, "void remove_proc_queue_head();\n");
    src = mputprintf(src,
      "void %s::remove_proc_queue_head()\n"
      "{\n"
      "switch (proc_queue_head->item_selection) {\n", class_name);
    for (i = 0; i < pdef->proc_in.nElements; i++) {
      src = mputprintf(src, "case CALL_%lu:\n"
        "delete proc_queue_head->call_%lu;\n"
        "break;\n", (unsigned long) i, (unsigned long) i);
    }
    for (i = 0; i < pdef->proc_out.nElements; i++) {
      if (pdef->proc_out.elements[i].is_noblock) continue;
      src = mputprintf(src, "case REPLY_%lu:\n"
        "delete proc_queue_head->reply_%lu;\n"
        "break;\n", (unsigned long) i, (unsigned long) i);
    }
    for (i = 0; i < pdef->proc_out.nElements; i++) {
      if (!pdef->proc_out.elements[i].has_exceptions) continue;
      src = mputprintf(src, "case EXCEPTION_%lu:\n"
        "delete proc_queue_head->exception_%lu;\n"
        "break;\n", (unsigned long) i, (unsigned long) i);
    }
    src = mputstr(src, "default:\n"
      "TTCN_error(\"Internal error: Invalid signature selector in "
      "the queue of port %s.\", port_name);\n"
      "}\n");
    if (pdef->testport_type == ADDRESS) {
      src = mputstr(src, "delete proc_queue_head->sender_address;\n");
    }
    src = mputstr(src,
      "proc_queue_item *next_item = proc_queue_head->next_item;\n"
      "delete proc_queue_head;\n"
      "proc_queue_head = next_item;\n"
      "if (next_item == NULL) proc_queue_tail = NULL;\n"
      "TTCN_Logger::log_port_queue(TitanLoggerApiSimple::Port__Queue_operation::extract__op, "
      "port_name, 0, ++proc_head_count, CHARSTRING(0,NULL), CHARSTRING(0,NULL));"
      "}\n\n");
  }

  /* clear_queue */
  if (has_msg_queue || has_proc_queue) {
    def = mputstr(def, "protected:\n"
      "void clear_queue();\n");
    src = mputprintf(src, "void %s::clear_queue()\n"
      "{\n", class_name);
    if (has_msg_queue) src = mputstr(src,
      "while (msg_queue_head != NULL) remove_msg_queue_head();\n");
    if (has_proc_queue) src = mputstr(src,
      "while (proc_queue_head != NULL) remove_proc_queue_head();\n");
    if (pdef->has_sliding) src = mputstr(src,
      "sliding_buffer = OCTETSTRING(0, 0);\n");
    src = mputstr(src, "}\n\n");
  }
  
  if (pdef->port_type == USER && !pdef->legacy) {
    def = mputstr(def, "private:\n");
    // Port type variables which can be the mapped ports for translation
    for (i = 0; i < pdef->provider_msg_outlist.nElements; i++) {
      def = mputprintf(def, "%s* p_%i;\n", pdef->provider_msg_outlist.elements[i].name, (int)i);
    }
    def = mputstr(def, "translation_port_state port_state;\n");
    
    // Declarations of port variables
    if (pdef->var_decls != NULL) {
      def = mputstr(def, pdef->var_decls);
    }
    
    if (pdef->var_defs != NULL) {
      def = mputstr(def, "void init_port_variables();\n");
      src = mputprintf(src,
        "void %s::init_port_variables() {\n%s}\n\n", class_name, pdef->var_defs);
    }
    
    // Declarations of mapping function which have this port in the 'port' clause
    if (pdef->mapping_func_decls != NULL) {
      def = mputstr(def, pdef->mapping_func_decls);
    }
    
    if (pdef->mapping_func_defs != NULL) {
      src = mputstr(src,  pdef->mapping_func_defs);
    }
  }
  
  // Port type variables in the provider types.
  if (pdef->n_mapper_name > 0) {
    def = mputstr(def, "private:\n");
    for (i = 0; i < pdef->n_mapper_name; i++) {
      def = mputprintf(def, "%s* p_%i;\n", pdef->mapper_name[i], (int)i);
    }
  }
  
  def = mputstr(def, "public:\n");

  /* constructor */
  def = mputprintf(def, "%s(const char *par_port_name", class_name);
  if (pdef->testport_type == INTERNAL || pdef->port_type != REGULAR) {
    /* the default argument is needed if the generated class implements
     * the port type (i.e. it is not a base class) */
    def = mputstr(def, " = NULL");
  }
  def = mputstr(def, ");\n");
  src = mputprintf(src, "%s::%s(const char *par_port_name)\n"
    " : %s(par_port_name)\n"
    "%s"
    "{\n", class_name, class_name, base_class_name,
    (pdef->has_sliding ? " , sliding_buffer(0, 0)" : ""));
  if (has_msg_queue) src = mputstr(src, "msg_queue_head = NULL;\n"
    "msg_queue_tail = NULL;\n");
  if (has_proc_queue) src = mputstr(src, "proc_queue_head = NULL;\n"
    "proc_queue_tail = NULL;\n");
  if (pdef->port_type == USER && !pdef->legacy) {
    for (i = 0; i < pdef->provider_msg_outlist.nElements; i++) {
      src = mputprintf(src, "p_%i = NULL;\n", (int)i);
    }
    src = mputprintf(src, "port_state = UNSET;\n");
  }
  // Port type variables in the provider types.
  for (i = 0; i < pdef->n_mapper_name; i++) {
    src = mputprintf(src, "p_%i = NULL;\n", (int)i);
  }
  src = mputstr(src, "}\n\n");

  /* destructor */
  if (has_msg_queue || has_proc_queue) {
    def = mputprintf(def, "~%s();\n", class_name);
    src = mputprintf(src, "%s::~%s()\n"
      "{\n"
      "clear_queue();\n"
      "}\n\n", class_name, class_name);
  }

  /* send functions */
  for (i = 0; i < pdef->msg_out.nElements; i++) {
    const port_msg_mapped_type *msg = pdef->msg_out.elements + i;

    def = mputprintf(def, "void send(const %s& send_par, "
      "const COMPONENT& destination_component);\n", msg->name);

    src = mputprintf(src, "void %s::send(const %s& send_par, "
      "const COMPONENT& destination_component)\n"
      "{\n"
      "if (!is_started) TTCN_error(\"Sending a message on port %%s, which "
      "is not started.\", port_name);\n"
      "if (!destination_component.is_bound()) "
      "TTCN_error(\"Unbound component reference in the to clause of send "
      "operation.\");\n"
      "const TTCN_Logger::Severity log_sev = "
      "destination_component==SYSTEM_COMPREF?"
      "TTCN_Logger::PORTEVENT_MMSEND:TTCN_Logger::PORTEVENT_MCSEND;\n"
      "if (TTCN_Logger::log_this_event(log_sev)) {\n"
      "TTCN_Logger::log_msgport_send(port_name, destination_component,\n"
      "(TTCN_Logger::begin_event(log_sev, TRUE), TTCN_Logger::log_event_str(\" %s : \"),\n"
      "send_par.log(), TTCN_Logger::end_event_log2str()));\n"
      "}\n", class_name, msg->name, msg->dispname);
    if (pdef->port_type != USER || (msg->nTargets == 1 &&
      msg->targets[0].mapping_type == M_SIMPLE) || (pdef->port_type == USER && !pdef->legacy)) {
      // If not in translation mode then send message as normally would.
      if (pdef->port_type == USER && !pdef->legacy &&
         (msg->nTargets > 1 || (msg->nTargets > 0 && msg->targets[0].mapping_type != M_SIMPLE))) {
        src = mputstr(src, "if (!in_translation_mode()) {\n");
      }
      /* the same message type goes through the external interface */
      src = mputstr(src, "if (destination_component == SYSTEM_COMPREF) ");
      if (pdef->testport_type == INTERNAL) {
        src = mputstr(src, "TTCN_error(\"Message cannot be sent to system "
          "on internal port %s.\", port_name);\n");
      } else {
        src = mputprintf(src,
          "{\n"
          // To generate DTE-s if not mapped or connected.
          "(void)get_default_destination();\n"
          "outgoing_%ssend(send_par%s);\n"
          "}\n",
          pdef->port_type == USER && !pdef->legacy ? "mapped_" : "",
          pdef->testport_type == ADDRESS ? ", NULL" : "");
      }
      src = mputprintf(src, "else {\n"
        "Text_Buf text_buf;\n"
        "prepare_message(text_buf, \"%s\");\n"
        "send_par.encode_text(text_buf);\n"
        "send_data(text_buf, destination_component);\n"
        "}\n", msg->dispname);
      if (pdef->port_type == USER && !pdef->legacy &&
         (msg->nTargets > 1 || (msg->nTargets > 0 && msg->targets[0].mapping_type != M_SIMPLE))) {
        // If in translation mode then generate the send mappings and send
        // according to them.
        src = mputstr(src, "} else {\n");
        src = generate_send_mapping(src, pdef, msg, FALSE);
        src = mputstr(src, "}\n");
      }
    } else {
      /* the message type is mapped to another outgoing type of the
       * external interface */
      src = generate_send_mapping(src, pdef, msg, FALSE);
    }
    src = mputstr(src, "}\n\n");

    if (pdef->testport_type == ADDRESS) {
      def = mputprintf(def, "void send(const %s& send_par, "
        "const %s& destination_address);\n", msg->name,
        pdef->address_name);

      src = mputprintf(src, "void %s::send(const %s& send_par, "
        "const %s& destination_address)\n"
        "{\n"
        "if (!is_started) TTCN_error(\"Sending a message on port %%s, "
        "which is not started.\", port_name);\n"
        "if (TTCN_Logger::log_this_event(TTCN_Logger::PORTEVENT_MMSEND))"
        "{\n"
        "TTCN_Logger::log_msgport_send(port_name, SYSTEM_COMPREF,\n"
        "(TTCN_Logger::begin_event(TTCN_Logger::PORTEVENT_MMSEND, TRUE),"
        " TTCN_Logger::log_event_str(\"(\"),"
        " destination_address.log(),"
        " TTCN_Logger::log_event_str(\") %s : \"),"
        " send_par.log(),"
        " TTCN_Logger::end_event_log2str()));\n"
        "}\n", class_name, msg->name, pdef->address_name, msg->dispname);
      // To generate DTE-s if not mapped or connected.
      src = mputstr(src, "(void)get_default_destination();\n");
      if (pdef->port_type != USER || (msg->nTargets == 1 &&
        msg->targets[0].mapping_type == M_SIMPLE)) {
        src = mputstr(src, "outgoing_send(send_par, "
          "&destination_address);\n");
      } else src = generate_send_mapping(src, pdef, msg, TRUE);
      src = mputstr(src, "}\n\n");
    }

    def = mputprintf(def, "void send(const %s& send_par);\n", msg->name);
    src = mputprintf(src, "void %s::send(const %s& send_par)\n"
      "{\n"
      "send(send_par, COMPONENT(get_default_destination()));\n"
      "}\n\n", class_name, msg->name);

    def = mputprintf(def, "void send(const %s_template& send_par, "
      "const COMPONENT& destination_component);\n", msg->name);
    src = mputprintf(src, "void %s::send(const %s_template& send_par, "
      "const COMPONENT& destination_component)\n"
      "{\n"
      "const %s& send_par_value = %s(send_par.valueof());\n"
      "send(send_par_value, destination_component);\n"
      "}\n\n", class_name, msg->name, msg->name, msg->name);

    if (pdef->testport_type == ADDRESS) {
      def = mputprintf(def, "void send(const %s_template& send_par, "
        "const %s& destination_address);\n", msg->name,
        pdef->address_name);
      src = mputprintf(src, "void %s::send(const %s_template& send_par, "
        "const %s& destination_address)\n"
        "{\n"
        "const %s& send_par_value = %s(send_par.valueof());\n"
        "send(send_par_value, destination_address);\n"
        "}\n\n", class_name, msg->name, pdef->address_name, msg->name, msg->name);
    }

    def = mputprintf(def, "void send(const %s_template& send_par);\n",
      msg->name);
    src = mputprintf(src, "void %s::send(const %s_template& send_par)\n"
      "{\n"
      "const %s& send_par_value = %s(send_par.valueof());\n"
      "send(send_par_value, COMPONENT(get_default_destination()));\n"
      "}\n\n", class_name, msg->name, msg->name, msg->name);
  }

  /* call functions */
  for (i = 0; i < pdef->proc_out.nElements; i++) {
    const port_proc_signature *sig = pdef->proc_out.elements + i;
    def = mputprintf(def, "void call(const %s_template& call_template, "
      "const COMPONENT& destination_component);\n", sig->name);
    src = mputprintf(src, "void %s::call(const %s_template& "
      "call_template, const COMPONENT& destination_component)\n"
      "{\n"
      "if (!is_started) TTCN_error(\"Calling a signature on port %%s, "
      "which is not started.\", port_name);\n"
      "if (!destination_component.is_bound()) TTCN_error(\"Unbound "
      "component reference in the to clause of call operation.\");\n"
      "const %s_call& call_tmp = call_template.create_call();\n"
      "const TTCN_Logger::Severity log_sev = "
      "destination_component==SYSTEM_COMPREF?"
      "TTCN_Logger::PORTEVENT_PMOUT:TTCN_Logger::PORTEVENT_PCOUT;\n"
      "if (TTCN_Logger::log_this_event(log_sev)) {\n"

      "TTCN_Logger::log_procport_send(port_name,"
      "TitanLoggerApiSimple::Port__oper::call__op, destination_component,\n"
      "CHARSTRING(0,NULL),"
      "(TTCN_Logger::begin_event(TTCN_Logger::PORTEVENT_PMOUT, TRUE),"
      " call_tmp.log(),"
      " TTCN_Logger::end_event_log2str()));\n"
      "}\n"
      "if (destination_component == SYSTEM_COMPREF) ",
      class_name, sig->name, sig->name);
    if (pdef->testport_type == INTERNAL) {
      src = mputstr(src, "TTCN_error(\"Internal port %s cannot send "
        "call to system.\", port_name);\n");
    } else {
      src = mputprintf(src, "outgoing_call(call_tmp%s);\n",
        pdef->testport_type == ADDRESS ? ", NULL" : "");
    }
    src = mputprintf(src, "else {\n"
      "Text_Buf text_buf;\n"
      "prepare_call(text_buf, \"%s\");\n"
      "call_tmp.encode_text(text_buf);\n"
      "send_data(text_buf, destination_component);\n"
      "}\n"
      "}\n\n", sig->dispname);

    if (pdef->testport_type == ADDRESS) {
      def = mputprintf(def, "void call(const %s_template& "
        "call_template, const %s& destination_address);\n",
        sig->name, pdef->address_name);
      src = mputprintf(src, "void %s::call(const %s_template& "
        "call_template, const %s& destination_address)\n"
        "{\n"
        "if (!is_started) TTCN_error(\"Calling a signature on port "
        "%%s, which is not started.\", port_name);\n"
        "const %s_call& call_tmp = call_template.create_call();\n"
        "if (TTCN_Logger::log_this_event(TTCN_Logger::PORTEVENT_PMOUT))"
        " {\n"
        "TTCN_Logger::log_procport_send(port_name, "
        "TitanLoggerApiSimple::Port__oper::call__op, SYSTEM_COMPREF,\n"
        "(TTCN_Logger::begin_event(TTCN_Logger::PORTEVENT_PMOUT, TRUE),"
        " destination_address.log(),"
        " TTCN_Logger::end_event_log2str()),\n"
        "(TTCN_Logger::begin_event(TTCN_Logger::PORTEVENT_PMOUT, TRUE),"
        " call_tmp.log(),"
        " TTCN_Logger::end_event_log2str()));\n"
        "}\n"
        "outgoing_call(call_tmp, &destination_address);\n"
        "}\n\n",
        class_name, sig->name, pdef->address_name, sig->name);
    }

    def = mputprintf(def, "void call(const %s_template& "
      "call_template);\n", sig->name);
    src = mputprintf(src, "void %s::call(const %s_template& "
      "call_template)\n"
      "{\n"
      "call(call_template, COMPONENT(get_default_destination()));\n"
      "}\n\n", class_name, sig->name);
  }

  /* reply functions */
  for (i = 0; i < pdef->proc_in.nElements; i++) {
    const port_proc_signature *sig = pdef->proc_in.elements + i;
    if (sig->is_noblock) continue;
    def = mputprintf(def,"void reply(const %s_template& "
      "reply_template, const COMPONENT& destination_component);\n",
      sig->name);
    src = mputprintf(src, "void %s::reply(const %s_template& "
      "reply_template, const COMPONENT& destination_component)\n"
      "{\n"
      "if (!is_started) TTCN_error(\"Replying to a signature on port "
      "%%s, which is not started.\", port_name);\n"
      "if (!destination_component.is_bound()) TTCN_error(\"Unbound "
      "component reference in the to clause of reply operation.\");\n"
      "const %s_reply& reply_tmp = reply_template.create_reply();\n"
      "const TTCN_Logger::Severity log_sev = "
      "destination_component==SYSTEM_COMPREF?"
      "TTCN_Logger::PORTEVENT_PMOUT:TTCN_Logger::PORTEVENT_PCOUT;\n"
      "if (TTCN_Logger::log_this_event(log_sev)) {\n"
      "TTCN_Logger::log_procport_send(port_name, "
      "TitanLoggerApiSimple::Port__oper::reply__op, destination_component,\n"
      " CHARSTRING(0, NULL),\n"
      "(TTCN_Logger::begin_event(TTCN_Logger::PORTEVENT_PMOUT, TRUE),"
      " reply_tmp.log(),"
      " TTCN_Logger::end_event_log2str()));\n"
      "}\n"
      "if (destination_component == SYSTEM_COMPREF) ",
      class_name, sig->name, sig->name);
    if (pdef->testport_type == INTERNAL) {
      src = mputstr(src, "TTCN_error(\"Internal port %s cannot send "
        "reply to system.\", port_name);\n");
    } else {
      src = mputprintf(src, "outgoing_reply(reply_tmp%s);\n",
        pdef->testport_type == ADDRESS ? ", NULL" : "");
    }
    src = mputprintf(src, "else {\n"
      "Text_Buf text_buf;\n"
      "prepare_reply(text_buf, \"%s\");\n"
      "reply_tmp.encode_text(text_buf);\n"
      "send_data(text_buf, destination_component);\n"
      "}\n"
      "}\n\n", sig->dispname);

    if (pdef->testport_type == ADDRESS) {
      def = mputprintf(def, "void reply(const %s_template& "
        "reply_template, const %s& destination_address);\n",
        sig->name, pdef->address_name);
      src = mputprintf(src, "void %s::reply(const %s_template& "
        "reply_template, const %s& destination_address)\n"
        "{\n"
        "if (!is_started) TTCN_error(\"Replying to a call on port "
        "%%s, which is not started.\", port_name);\n"
        "const %s_reply& reply_tmp = reply_template.create_reply();\n"
        "if (TTCN_Logger::log_this_event(TTCN_Logger::PORTEVENT_PMOUT))"
        " {\n"
        "TTCN_Logger::log_procport_send(port_name, "
        "TitanLoggerApiSimple::Port__oper::reply__op, SYSTEM_COMPREF,\n"
        "(TTCN_Logger::begin_event(TTCN_Logger::PORTEVENT_PMOUT, TRUE),"
        " destination_address.log(),"
        " TTCN_Logger::end_event_log2str()),\n"
        "(TTCN_Logger::begin_event(TTCN_Logger::PORTEVENT_PMOUT, TRUE),"
        " reply_tmp.log(),"
        " TTCN_Logger::end_event_log2str()));\n"
        "}\n"
        "outgoing_reply(reply_tmp, &destination_address);\n"
        "}\n\n",
        class_name, sig->name, pdef->address_name, sig->name);
    }

    def = mputprintf(def, "void reply(const %s_template& "
      "reply_template);\n", sig->name);
    src = mputprintf(src, "void %s::reply(const %s_template& "
      "reply_template)\n"
      "{\n"
      "reply(reply_template, COMPONENT(get_default_destination()));\n"
      "}\n\n", class_name, sig->name);
  }

  /* raise functions */
  for(i = 0; i < pdef->proc_in.nElements; i++) {
    const port_proc_signature *sig = pdef->proc_in.elements + i;
    if (!sig->has_exceptions) continue;
    def = mputprintf(def, "void raise(const %s_exception& "
      "raise_exception, const COMPONENT& destination_component);\n",
      sig->name);
    src = mputprintf(src, "void %s::raise(const %s_exception& "
      "raise_exception, const COMPONENT& destination_component)\n"
      "{\n"
      "if (!is_started) TTCN_error(\"Raising an exception on port %%s, "
      "which is not started.\", port_name);\n"
      "if (!destination_component.is_bound()) TTCN_error(\"Unbound "
      "component reference in the to clause of raise operation.\");\n"
      "const TTCN_Logger::Severity log_sev = "
      "destination_component==SYSTEM_COMPREF?"
      "TTCN_Logger::PORTEVENT_PMOUT:TTCN_Logger::PORTEVENT_PCOUT;\n"
      "if (TTCN_Logger::log_this_event(log_sev)) {\n"
      "TTCN_Logger::log_procport_send(port_name, "
      "TitanLoggerApiSimple::Port__oper::exception__op, destination_component,\n"
      " CHARSTRING(0, NULL),\n"
      "(TTCN_Logger::begin_event(TTCN_Logger::PORTEVENT_PMOUT, TRUE),"
      " raise_exception.log(),"
      " TTCN_Logger::end_event_log2str()));\n"
      "}\n"
      "if (destination_component == SYSTEM_COMPREF) ",
      class_name, sig->name);
    if (pdef->testport_type == INTERNAL) {
      src = mputstr(src, "TTCN_error(\"Internal port %s cannot raise an "
        "exception to system.\", port_name);\n");
    } else {
      src = mputprintf(src, "outgoing_raise(raise_exception%s);\n",
        pdef->testport_type == ADDRESS ? ", NULL" : "");
    }
    src = mputprintf(src, "else {\n"
      "Text_Buf text_buf;\n"
      "prepare_exception(text_buf, \"%s\");\n"
      "raise_exception.encode_text(text_buf);\n"
      "send_data(text_buf, destination_component);\n"
      "}\n"
      "}\n\n", sig->dispname);

    if (pdef->testport_type == ADDRESS) {
      def = mputprintf(def, "void raise(const %s_exception& "
        "raise_exception, const %s& destination_address);\n",
        sig->name, pdef->address_name);
      src = mputprintf(src, "void %s::raise(const %s_exception& "
        "raise_exception, const %s& destination_address)\n"
        "{\n"
        "if (!is_started) TTCN_error(\"Raising an exception on port "
        "%%s, which is not started.\", port_name);\n"
        "if (TTCN_Logger::log_this_event(TTCN_Logger::PORTEVENT_PMOUT))"
        " {\n"
        "TTCN_Logger::log_procport_send(port_name, "
        "TitanLoggerApiSimple::Port__oper::exception__op, SYSTEM_COMPREF,\n"
        "(TTCN_Logger::begin_event(TTCN_Logger::PORTEVENT_PMOUT, TRUE),"
        " destination_address.log(),"
        " TTCN_Logger::end_event_log2str()),\n"
        "(TTCN_Logger::begin_event(TTCN_Logger::PORTEVENT_PMOUT, TRUE),"
        " raise_exception.log(),"
        " TTCN_Logger::end_event_log2str()));\n"
        "}\n"
        "outgoing_raise(raise_exception, &destination_address);\n"
        "}\n\n",
        class_name, sig->name, pdef->address_name);
    }

    def = mputprintf(def, "void raise(const %s_exception& "
      "raise_exception);\n", sig->name);
    src = mputprintf(src, "void %s::raise(const %s_exception& "
      "raise_exception)\n"
      "{\n"
      "raise(raise_exception, COMPONENT(get_default_destination()));\n"
      "}\n\n", class_name, sig->name);
  }

    
  if (pdef->port_type == USER && !pdef->legacy) {
    def = mputstr(def, "public:\n");
    // add_port and remove_port is called after the map and unmap statements.
    for (i = 0; i < pdef->provider_msg_outlist.nElements; i++) {
      def = mputprintf(def, "void add_port(%s* p);\n", pdef->provider_msg_outlist.elements[i].name);
      src = mputprintf(src, "void %s::add_port(%s*p) {\np_%i = p;\n}\n\n", class_name, pdef->provider_msg_outlist.elements[i].name, (int)i);
      
      def = mputprintf(def, "void remove_port(%s*);\n", pdef->provider_msg_outlist.elements[i].name);
      src = mputprintf(src, "void %s::remove_port(%s*) {\np_%i = NULL;\n}\n\n", class_name, pdef->provider_msg_outlist.elements[i].name, (int)i);
    }
    
    // in_translation_mode returns true if one of the port type variables are not null
    def = mputstr(def, "boolean in_translation_mode() const;\n");
    src = mputprintf(src, "boolean %s::in_translation_mode() const {\nreturn ", class_name);
    for (i = 0; i < pdef->provider_msg_outlist.nElements; i++) {
      src = mputprintf(src, "p_%i != NULL%s",
        (int)i,
        i != pdef->provider_msg_outlist.nElements - 1 ? " || " : "");
    }
    src = mputstr(src, ";\n}\n\n");
    
    def = mputstr(def, "void change_port_state(translation_port_state state);\n");
    src = mputprintf(src,
      "void %s::change_port_state(translation_port_state state) {\n"
      "port_state = state;\n"
      "}\n\n", class_name);
    
    def = mputstr(def, "private:\n");
    // Resets all port type variables to NULL
    def = mputstr(def, "void reset_port_variables();\n");
    src = mputprintf(src, "void %s::reset_port_variables() {\n", class_name);
    for (i = 0; i < pdef->provider_msg_outlist.nElements; i++) {
      src = mputprintf(src, "p_%i = NULL;\n", (int)i);
    }
    src = mputstr(src, "}\n\n");
    
  }
  // Port type variables in the provider types.
  if (pdef->n_mapper_name > 0) {
    def = mputstr(def, "public:\n");
    // add_port and remove_port is called after the map and unmap statements.
    for (i = 0; i < pdef->n_mapper_name; i++) {
      def = mputprintf(def, "void add_port(%s* p);\n", pdef->mapper_name[i]);
      src = mputprintf(src, "void %s::add_port(%s*p) {\n p_%i = p;\n}\n\n", class_name, pdef->mapper_name[i], (int)i);
      
      def = mputprintf(def, "void remove_port(%s*);\n", pdef->mapper_name[i]);
      src = mputprintf(src, "void %s::remove_port(%s*) {\n p_%i = NULL;\n}\n\n", class_name, pdef->mapper_name[i], (int)i);
    }
    def = mputstr(def, "private:\n");
    // Resets all port type variables to NULL
    def = mputstr(def, "void reset_port_variables();\n");
    src = mputprintf(src, "void %s::reset_port_variables() {\n", class_name);
    for (i = 0; i < pdef->n_mapper_name; i++) {
      src = mputprintf(src, "p_%i = NULL;\n", (int)i);
    }
    src = mputstr(src, "}\n\n");
  }
  
  if (pdef->testport_type != INTERNAL &&
     (pdef->port_type == REGULAR || (pdef->port_type == USER && !pdef->legacy))) {
    /* virtual functions for transmission (implemented by the test port) */
    def = mputprintf(def, "%s:\n",
      (pdef->port_type == USER && !pdef->legacy) ? "public" : "protected");
    /* outgoing_send functions */
    size_t n_used = 0;
    const char** used = NULL;
    // Only print one outgoing_send for each type
    for (i = 0; i < pdef->msg_out.nElements; i++) {
      used = (const char**)Realloc(used, (n_used + 1) * sizeof(const char*));
      boolean found = FALSE;
      for (size_t j = 0; j < n_used; j++) {
        if (strcmp(used[j], pdef->msg_out.elements[i].name) == 0) {
          found = TRUE;
          break;
        }
      }
      if (!found) {
        def = mputprintf(def, "virtual void outgoing_send("
          "const %s& send_par", pdef->msg_out.elements[i].name);
        if (pdef->testport_type == ADDRESS) {
          def = mputprintf(def, ", const %s *destination_address",
            pdef->address_name);
        }
        def = mputstr(def, ") = 0;\n");
        // When port translation is enabled
        // we call the outgoing_mapped_send instead of outgoing_send,
        // and this function will call one of the mapped port's outgoing_send
        // functions, or its own outgoing_send function.
        // This is for the types that are present in the out message list of the port
        if (pdef->port_type == USER && !pdef->legacy) {
          def = mputprintf(def, "void outgoing_mapped_send("
          "const %s& send_par);\n", pdef->msg_out.elements[i].name);
        
          src = mputprintf(src, "void %s::outgoing_mapped_send("
            "const %s& send_par", class_name, pdef->msg_out.elements[i].name);
          if (pdef->testport_type == ADDRESS) {
            def = mputprintf(def, ", const %s *destination_address",
              pdef->address_name);
          }
          src = mputstr(src, ") {\n");
          for (size_t j = 0; j < pdef->provider_msg_outlist.nElements; j++) {
            found = FALSE;
            for (size_t k = 0; k < pdef->provider_msg_outlist.elements[j].n_out_msg_type_names; k++) {
              if (strcmp(pdef->msg_out.elements[i].name, pdef->provider_msg_outlist.elements[j].out_msg_type_names[k]) == 0) {
                found = TRUE;
                break;
              }
            }
            if (found) {
              src = mputprintf(src,
                "if (p_%i != NULL) {\n"
                "p_%i->outgoing_send(send_par);\n"
                "return;\n}\n", (int)j, (int)j);
            }
          }
          found = FALSE;
          for (size_t j = 0; j < pdef->msg_out.nElements; j++) {
            if (strcmp(pdef->msg_out.elements[j].name, pdef->msg_out.elements[i].name) == 0) {
              found = TRUE;
              break;
            }
          }
          if (found) {
            src = mputprintf(src, "outgoing_send(send_par);\n");
          } else {
            src = mputprintf(src, "TTCN_error(\"Cannot send message correctly with type %s\");\n", pdef->msg_out.elements[i].name);
          }
          src = mputstr(src, "}\n\n");
        }
        used[n_used] = pdef->msg_out.elements[i].name;
        n_used++;
      }
    }
    
    if (pdef->port_type == USER && !pdef->legacy) {
      for (i = 0; i < pdef->msg_out.nElements; i++) {
        for (size_t j = 0; j < pdef->msg_out.elements[i].nTargets; j++) {
          boolean found = FALSE;
          used = (const char**)Realloc(used, (n_used + 1) * sizeof(const char*));
          for (size_t k = 0; k < n_used; k++) {
            if (strcmp(used[k], pdef->msg_out.elements[i].targets[j].target_name) == 0) {
              found = TRUE;
              break;
            }
          }
          if (!found) {
            // When standard like port translated port is present,
            // We call the outgoing_mapped_send instead of outgoing_send,
            // and this function will call one of the mapped port's outgoing_send
            // functions, or its own outgoing_send function.
            // This is for the mapping target types.
            def = mputprintf(def, "void outgoing_mapped_send("
            "const %s& send_par);\n", pdef->msg_out.elements[i].targets[j].target_name);

            src = mputprintf(src, "void %s::outgoing_mapped_send("
              "const %s& send_par", class_name, pdef->msg_out.elements[i].targets[j].target_name);
            if (pdef->testport_type == ADDRESS) {
              def = mputprintf(def, ", const %s *destination_address",
                pdef->address_name);
            }
            src = mputstr(src, ") {\n");
            for (size_t k = 0; k < pdef->provider_msg_outlist.nElements; k++) {
              found = FALSE;
              for (size_t l = 0; l < pdef->provider_msg_outlist.elements[k].n_out_msg_type_names; l++) {
                if (strcmp(pdef->msg_out.elements[i].targets[j].target_name, pdef->provider_msg_outlist.elements[k].out_msg_type_names[l]) == 0) {
                  found = TRUE;
                  break;
                }
              }
              if (found) {
                src = mputprintf(src,
                  "if (p_%i != NULL) {\n"
                  "p_%i->outgoing_send(send_par);\n"
                  "return;\n}\n", (int)k, (int)k);
              }
            }
            found = FALSE;
            for (size_t k = 0; k < pdef->msg_out.nElements; k++) {
              if (strcmp(pdef->msg_out.elements[k].name, pdef->msg_out.elements[i].targets[j].target_name) == 0) {
                found = TRUE;
                break;
              }
            }
            if (found) {
              src = mputprintf(src, "outgoing_send(send_par);\n");
            } else {
              // This should never happen
              src = mputprintf(src, "TTCN_error(\"Cannot send message correctly %s\");\n", pdef->msg_out.elements[i].targets[j].target_name);
            }
            src = mputstr(src, "}\n\n");
            used[n_used] = pdef->msg_out.elements[i].targets[j].target_name;
            n_used++;
          }
        }
      }
    }
    Free(used); // do not delete pointers
    if (pdef->port_type == USER && !pdef->legacy) {
      def = mputstr(def, "protected:\n");
    }
    /* outgoing_call functions */
    for (i = 0; i < pdef->proc_out.nElements; i++) {
      def = mputprintf(def, "virtual void outgoing_call("
        "const %s_call& call_par", pdef->proc_out.elements[i].name);
      if (pdef->testport_type == ADDRESS) {
        def = mputprintf(def, ", const %s *destination_address",
          pdef->address_name);
      }
      def = mputstr(def, ") = 0;\n");
    }
    /* outgoing_reply functions */
    for (i = 0; i < pdef->proc_in.nElements; i++) {
      if (pdef->proc_in.elements[i].is_noblock) continue;
      def = mputprintf(def, "virtual void outgoing_reply("
        "const %s_reply& reply_par", pdef->proc_in.elements[i].name);
      if (pdef->testport_type == ADDRESS) {
        def = mputprintf(def, ", const %s *destination_address",
          pdef->address_name);
      }
      def = mputstr(def, ") = 0;\n");
    }
    /* outgoing_raise functions */
    for(i = 0; i < pdef->proc_in.nElements; i++) {
      if (!pdef->proc_in.elements[i].has_exceptions) continue;
      def = mputprintf(def, "virtual void outgoing_raise("
        "const %s_exception& raise_exception",
        pdef->proc_in.elements[i].name);
      if (pdef->testport_type == ADDRESS) {
        def = mputprintf(def, ", const %s *destination_address",
          pdef->address_name);
      }
      def = mputstr(def, ") = 0;\n");
    }
    def = mputstr(def, "public:\n");
  }

  /* Generic receive routines (without message type) */
  if (has_msg_queue) {
    /* generic receive function */
    generate_generic_receive(&def, &src, pdef, class_name, FALSE, FALSE,
      FALSE);
    /* generic check_receive function */
    generate_generic_receive(&def, &src, pdef, class_name, TRUE, FALSE,
      FALSE);
    /* generic trigger function */
    generate_generic_receive(&def, &src, pdef, class_name, FALSE, TRUE,
      FALSE);
    if (pdef->testport_type == ADDRESS) {
      /* generic receive with address */
      generate_generic_receive(&def, &src, pdef, class_name, FALSE,
        FALSE, TRUE);
      /* generic check_receive with address */
      generate_generic_receive(&def, &src, pdef, class_name, TRUE,
        FALSE, TRUE);
      /* generic trigger with address */
      generate_generic_receive(&def, &src, pdef, class_name, FALSE,
        TRUE, TRUE);
    }
  }

  /* Receive routines with message type */
  for (i = 0; i < pdef->msg_in.nElements; i++) {
    /* receive */
    generate_receive(&def, &src, pdef, class_name, i, FALSE, FALSE, FALSE);
    /* check_receive */
    generate_receive(&def, &src, pdef, class_name, i, TRUE, FALSE, FALSE);
    /* trigger */
    generate_receive(&def, &src, pdef, class_name, i, FALSE, TRUE, FALSE);
    if (pdef->testport_type == ADDRESS) {
      /* receive with address */
      generate_receive(&def, &src, pdef, class_name, i, FALSE, FALSE,
        TRUE);
      /* check_receive with address */
      generate_receive(&def, &src, pdef, class_name, i, TRUE, FALSE,
        TRUE);
      /* trigger with address */
      generate_receive(&def, &src, pdef, class_name, i, FALSE, TRUE,
        TRUE);
    }
  }

  if (has_incoming_call) {
    /* generic getcall function */
    generate_generic_getop(GETCALL, &def, &src, pdef, class_name, FALSE,
      FALSE);
    /* generic check_getcall function */
    generate_generic_getop(GETCALL, &def, &src, pdef, class_name, TRUE,
      FALSE);
    if (pdef->testport_type == ADDRESS) {
      /* generic getcall with address */
      generate_generic_getop(GETCALL, &def, &src, pdef, class_name,
        FALSE, TRUE);
      /* generic check_getcall with address */
      generate_generic_getop(GETCALL, &def, &src, pdef, class_name,
        TRUE, TRUE);
    }
  }

  /* getcall functions */
  for (i = 0; i < pdef->proc_in.nElements; i++) {
    /* getcall */
    generate_getcall(&def, &src, pdef, class_name, i, FALSE, FALSE);
    /* check_getcall */
    generate_getcall(&def, &src, pdef, class_name, i, TRUE, FALSE);
    if (pdef->testport_type == ADDRESS) {
      /* getcall with address */
      generate_getcall(&def, &src, pdef, class_name, i, FALSE, TRUE);
      /* check_getcall with address */
      generate_getcall(&def, &src, pdef, class_name, i, TRUE, TRUE);
    }
  }

  if (has_incoming_reply) {
    /* generic getreply function */
    generate_generic_getop(GETREPLY, &def, &src, pdef, class_name, FALSE,
      FALSE);
    /* generic check_getreply function */
    generate_generic_getop(GETREPLY, &def, &src, pdef, class_name, TRUE,
      FALSE);
    if (pdef->testport_type == ADDRESS) {
      /* generic getreply with address */
      generate_generic_getop(GETREPLY, &def, &src, pdef, class_name,
        FALSE, TRUE);
      /* generic check_getreply with address */
      generate_generic_getop(GETREPLY, &def, &src, pdef, class_name,
        TRUE, TRUE);
    }
  }

  /* getreply functions */
  for (i = 0; i < pdef->proc_out.nElements; i++) {
    if (pdef->proc_out.elements[i].is_noblock) continue;
    /* getreply */
    generate_getreply(&def, &src, pdef, class_name, i, FALSE, FALSE);
    /* check_getreply */
    generate_getreply(&def, &src, pdef, class_name, i, TRUE, FALSE);
    if (pdef->testport_type == ADDRESS) {
      /* getreply with address */
      generate_getreply(&def, &src, pdef, class_name, i, FALSE, TRUE);
      /* check_getreply with address */
      generate_getreply(&def, &src, pdef, class_name, i, TRUE, TRUE);
    }
  }

  if (has_incoming_exception) {
    /* generic catch (get_exception) function */
    generate_generic_getop(CATCH, &def, &src, pdef, class_name, FALSE,
      FALSE);
    /* generic check_catch function */
    generate_generic_getop(CATCH, &def, &src, pdef, class_name, TRUE,
      FALSE);
    if (pdef->testport_type == ADDRESS) {
      /* generic catch (get_exception) with address */
      generate_generic_getop(CATCH, &def, &src, pdef, class_name, FALSE,
        TRUE);
      /* generic check_catch with address */
      generate_generic_getop(CATCH, &def, &src, pdef, class_name, TRUE,
        TRUE);
    }
  }

  /* catch functions */
  for (i = 0; i < pdef->proc_out.nElements; i++) {
    if (!pdef->proc_out.elements[i].has_exceptions) continue;
    /* catch (get_exception) */
    generate_catch(&def, &src, pdef, class_name, i, FALSE, FALSE);
    /* check_catch */
    generate_catch(&def, &src, pdef, class_name, i, TRUE, FALSE);
    if (pdef->testport_type == ADDRESS) {
      /* catch (get_exception) with address */
      generate_catch(&def, &src, pdef, class_name, i, FALSE, TRUE);
      /* check_catch with address */
      generate_catch(&def, &src, pdef, class_name, i, TRUE, TRUE);
    }
  }

  def = mputstr(def, "private:\n");

  if (pdef->port_type == USER) {
    /* incoming_message() functions for the incoming types of the provider
     * port type */
    if (!pdef->legacy) {
      def = mputstr(def, "public:\n");
    }
    for (i = 0; i < pdef->provider_msg_in.nElements; i++) {
      const port_msg_mapped_type *mapped_type =
        pdef->provider_msg_in.elements + i;
      boolean is_simple = (!pdef->legacy || mapped_type->nTargets == 1) &&
        mapped_type->targets[0].mapping_type == M_SIMPLE;
      def = mputprintf(def, "void incoming_message(const %s& "
        "incoming_par, component sender_component%s",
        mapped_type->name,
        (pdef->has_sliding ? ", OCTETSTRING& slider" : ""));
      if (pdef->testport_type == ADDRESS) {
        def = mputprintf(def, ", const %s *sender_address",
          pdef->address_name);
      }
      def = mputstr(def, ");\n");

      src = mputprintf(src, "void %s::incoming_message(const %s& "
        "incoming_par, component sender_component%s", class_name,
        mapped_type->name,
        (pdef->has_sliding ? ", OCTETSTRING& slider" : ""));
      if (pdef->testport_type == ADDRESS) {
        src = mputprintf(src, ", const %s *sender_address",
          pdef->address_name);
      }
      src = mputstr(src, ")\n"
        "{\n"
        "if (!is_started) TTCN_error(\"Port %s is not started but a "
        "message has arrived on it.\", port_name);\n");
      if (pdef->has_sliding) src = mputstr(src, "(void)slider;\n");
      if (is_simple) src = mputstr(src, "msg_tail_count++;\n");
      src = mputprintf(src, "if (TTCN_Logger::log_this_event("
      "TTCN_Logger::PORTEVENT_MQUEUE)) {\n"
      "TTCN_Logger::log_port_queue(TitanLoggerApiSimple::Port__Queue_operation::enqueue__msg,"
      " port_name, sender_component, msg_tail_count%s,\n",
      /* if is_simple==FALSE then this log message is a LIE! */
      is_simple?"":"+1"
      );
    if (pdef->testport_type == ADDRESS) {
      src = mputstr(src,
        "sender_address ?"
        /* Use the comma operator to get an expression of type CHARSTRING */
        " (TTCN_Logger::begin_event(TTCN_Logger::PORTEVENT_MQUEUE, TRUE),"
        " TTCN_Logger::log_char('('), sender_address->log(),"
        " TTCN_Logger::log_char(')'), TTCN_Logger::end_event_log2str()) : "
        );
    }
    src = mputprintf(src,
      /* This empty string may be an operand to a conditional operator from above */
      "CHARSTRING(0, NULL),\n"
      /* Protect the comma operators from being interpreted as
       * function argument separators. */
      "(TTCN_Logger::begin_event(TTCN_Logger::PORTEVENT_MQUEUE, TRUE),"
      " TTCN_Logger::log_event_str(\" %s : \"),"
      " incoming_par.log(), TTCN_Logger::end_event_log2str()));\n"
      "}\n", mapped_type->dispname);
      // Print the simple mapping after the not simple mappings
      if (!is_simple || !pdef->legacy) {
        if (!pdef->legacy && mapped_type->nTargets > (is_simple ? 1 : 0)) {
          // If in translation mode then receive according to incoming mappings
          src = mputstr(src, "if (in_translation_mode()) {\n");
        }
        src = generate_incoming_mapping(src, pdef, mapped_type, is_simple);
        if (!pdef->legacy && mapped_type->nTargets > (is_simple ? 1 : 0)) {
          src = mputstr(src, "}\n");
        }
      }
      if (is_simple) {
        src = mputprintf(src,
#ifndef NDEBUG
          "// simple mapping\n"
#endif
          "msg_queue_item *new_item = new msg_queue_item;\n"
          "new_item->item_selection = MESSAGE_%lu;\n"
          "new_item->message_%lu = new %s(incoming_par);\n"
          "new_item->sender_component = sender_component;\n",
          (unsigned long) mapped_type->targets[0].target_index,
          (unsigned long) mapped_type->targets[0].target_index,
          mapped_type->name);
        if (pdef->testport_type == ADDRESS) {
          src = mputprintf(src, "if (sender_address != NULL) "
            "new_item->sender_address = new %s(*sender_address);\n"
            "else new_item->sender_address = NULL;\n",
            pdef->address_name);
        }
        src = mputstr(src, "append_to_msg_queue(new_item);\n");
      }
      src = mputstr(src, "}\n\n");
    }
  } else { /* not user */
    /* incoming_message functions */
    if (pdef->port_type == PROVIDER && pdef->n_mapper_name > 0) {
      def = mputstr(def, "public:\n");
    }
    for (i = 0; i < pdef->msg_in.nElements; i++) {
      def = mputprintf(def, "void incoming_message(const %s& "
        "incoming_par, component sender_component",
        pdef->msg_in.elements[i].name);
      if (pdef->testport_type == ADDRESS) {
        def = mputprintf(def, ", const %s *sender_address",
          pdef->address_name);
      }
      def = mputstr(def, ");\n");

      src = mputprintf(src, "void %s::incoming_message(const %s& "
        "incoming_par, component sender_component", class_name,
        pdef->msg_in.elements[i].name);
      if (pdef->testport_type == ADDRESS) {
        src = mputprintf(src, ", const %s *sender_address",
          pdef->address_name);
      }
      src = mputstr(src, ")\n{\n");
      if (pdef->port_type == PROVIDER && pdef->n_mapper_name > 0) {
        // We forward the incoming_message to the mapped port
        for (size_t j = 0; j < pdef->n_mapper_name; j++) {
          src = mputprintf(src,
            "if (p_%i != NULL) {\n"
            "p_%i->incoming_message(incoming_par, sender_component);\n"
            "return;\n}\n",
            (int)j, (int)j);
        }
      }
      src = mputstr(src,
        "if (!is_started) TTCN_error(\"Port %s is not started but a "
        "message has arrived on it.\", port_name);\n"
        "msg_tail_count++;\n"
        "if (TTCN_Logger::log_this_event(TTCN_Logger::PORTEVENT_MQUEUE)) "
        "{\n"
        "TTCN_Logger::log_port_queue(TitanLoggerApiSimple::Port__Queue_operation::enqueue__msg,"
        " port_name, sender_component, msg_tail_count,\n"
        );
      if (pdef->testport_type == ADDRESS) {
        src = mputstr(src,
          "sender_address ?"
          /* Use the comma operator to get an expression of type CHARSTRING */
          " (TTCN_Logger::begin_event(TTCN_Logger::PORTEVENT_MQUEUE, TRUE),"
          " TTCN_Logger::log_char('('), sender_address->log(),"
          " TTCN_Logger::log_char(')'), TTCN_Logger::end_event_log2str()) : "
          );
      }
      src = mputprintf(src,
        /* This empty string may be an operand to a conditional operator from above */
        "CHARSTRING(0, NULL),\n"
        /* Protect the comma operators from being interpreted as
         * function argument separators. */
        "(TTCN_Logger::begin_event(TTCN_Logger::PORTEVENT_MQUEUE, TRUE),"
        " TTCN_Logger::log_event_str(\" %s : \"),"
        " incoming_par.log(), TTCN_Logger::end_event_log2str()));\n"
        "}\n"
        "msg_queue_item *new_item = new msg_queue_item;\n"
        "new_item->item_selection = MESSAGE_%lu;\n"
        "new_item->message_%lu = new %s(incoming_par);\n"
        "new_item->sender_component = sender_component;\n",
        pdef->msg_in.elements[i].dispname,
        (unsigned long) i, (unsigned long) i,
        pdef->msg_in.elements[i].name);
      if (pdef->testport_type == ADDRESS) {
        src = mputprintf(src, "if (sender_address != NULL) "
          "new_item->sender_address = new %s(*sender_address);\n"
          "else new_item->sender_address = NULL;\n",
          pdef->address_name);
      }
      src = mputstr(src, "append_to_msg_queue(new_item);\n"
        "}\n\n");
    }
  }
  
  if ((pdef->port_type == PROVIDER && pdef->n_mapper_name > 0) ||
      (pdef->port_type == USER && !pdef->legacy)) {
    def = mputstr(def, "private:\n");
  }

  /* incoming_call functions */
  for (i = 0; i < pdef->proc_in.nElements; i++) {
    const port_proc_signature *sig = pdef->proc_in.elements + i;
    def = mputprintf(def, "void incoming_call(const %s_call& incoming_par, "
      "component sender_component", sig->name);
    if (pdef->testport_type == ADDRESS) {
      def = mputprintf(def, ", const %s *sender_address",
        pdef->address_name);
    }
    def = mputstr(def, ");\n");
    src = mputprintf(src, "void %s::incoming_call(const %s_call& "
      "incoming_par, component sender_component", class_name, sig->name);
    if (pdef->testport_type == ADDRESS) {
      src = mputprintf(src, ", const %s *sender_address",
        pdef->address_name);
    }
    src = mputstr(src, ")\n"
      "{\n"
      "if (!is_started) TTCN_error(\"Port %s is not started but a call "
      "has arrived on it.\", port_name);\n"
      "proc_tail_count++;\n"
      "if (TTCN_Logger::log_this_event(TTCN_Logger::PORTEVENT_PQUEUE)) "
      "{\n"
      "TTCN_Logger::log_port_queue(TitanLoggerApiSimple::Port__Queue_operation::enqueue__call,"
      " port_name, sender_component, proc_tail_count,\n"
      );
    if (pdef->testport_type == ADDRESS) {
      src = mputstr(src,
        "sender_address ?"
        /* Use the comma operator to get an expression of type CHARSTRING */
        " (TTCN_Logger::begin_event(TTCN_Logger::PORTEVENT_PQUEUE, TRUE),"
        " TTCN_Logger::log_char('('), sender_address->log(),"
        " TTCN_Logger::log_char(')'), TTCN_Logger::end_event_log2str()) : "
        );
    }
    src = mputprintf(src,
      /* This empty string may be an operand to a conditional operator from above */
      "CHARSTRING(0, NULL),\n"
      /* Protect the comma operators from being interpreted as
       * function argument separators. */
      "(TTCN_Logger::begin_event(TTCN_Logger::PORTEVENT_PQUEUE, TRUE),"
      " TTCN_Logger::log_char(' '),"
      " incoming_par.log(), TTCN_Logger::end_event_log2str()));\n"
      "}\n"
      "proc_queue_item *new_item = new proc_queue_item;\n"
      "new_item->item_selection = CALL_%lu;\n"
      "new_item->call_%lu = new %s_call(incoming_par);\n"
      "new_item->sender_component = sender_component;\n",
      (unsigned long) i, (unsigned long) i, sig->name);
    if (pdef->testport_type == ADDRESS) {
      src = mputprintf(src, "if (sender_address != NULL) "
        "new_item->sender_address = new %s(*sender_address);\n"
        "else new_item->sender_address = NULL;\n", pdef->address_name);
    }
    src = mputstr(src, "append_to_proc_queue(new_item);\n"
      "}\n\n");
  }

  /* incoming_reply functions */
  for (i = 0; i < pdef->proc_out.nElements; i++) {
    const port_proc_signature *sig = pdef->proc_out.elements + i;
    if (sig->is_noblock) continue;
    def = mputprintf(def, "void incoming_reply(const %s_reply& "
      "incoming_par, component sender_component", sig->name);
    if (pdef->testport_type == ADDRESS) {
      def = mputprintf(def, ", const %s *sender_address",
        pdef->address_name);
    }
    def = mputstr(def, ");\n");
    src = mputprintf(src, "void %s::incoming_reply(const %s_reply& "
      "incoming_par, component sender_component", class_name,
      sig->name);
    if (pdef->testport_type == ADDRESS) {
      src = mputprintf(src, ", const %s *sender_address",
        pdef->address_name);
    }
    src = mputstr(src, ")\n"
      "{\n"
      "if (!is_started) TTCN_error(\"Port %s is not started but a reply "
      "has arrived on it.\", port_name);\n"
      "proc_tail_count++;\n"
      "if (TTCN_Logger::log_this_event(TTCN_Logger::PORTEVENT_PQUEUE)) "
      "{\n"
      "TTCN_Logger::log_port_queue(TitanLoggerApiSimple::Port__Queue_operation::enqueue__reply,"
      " port_name, sender_component, proc_tail_count,\n");
    if (pdef->testport_type == ADDRESS) {
      src = mputstr(src,
        "sender_address ?"
        /* Use the comma operator to get an expression of type CHARSTRING */
        " (TTCN_Logger::begin_event(TTCN_Logger::PORTEVENT_PQUEUE, TRUE),"
        " TTCN_Logger::log_char('('), sender_address->log(),"
        " TTCN_Logger::log_char(')'), TTCN_Logger::end_event_log2str()) : ");
    }
    src = mputprintf(src,
      /* This empty string may be an operand to a conditional operator from above */
      "CHARSTRING(0, NULL),\n"
      "(TTCN_Logger::begin_event(TTCN_Logger::PORTEVENT_PQUEUE, TRUE),"
      " TTCN_Logger::log_char(' '),"
      " incoming_par.log(), TTCN_Logger::end_event_log2str()));\n"
      "}\n"
      "proc_queue_item *new_item = new proc_queue_item;\n"
      "new_item->item_selection = REPLY_%lu;\n"
      "new_item->reply_%lu = new %s_reply(incoming_par);\n"
      "new_item->sender_component = sender_component;\n",
      (unsigned long) i, (unsigned long) i, sig->name);
    if (pdef->testport_type == ADDRESS) {
      src = mputprintf(src, "if (sender_address != NULL) "
        "new_item->sender_address = new %s(*sender_address);\n"
        "else new_item->sender_address = NULL;\n",
        pdef->address_name);
    }
    src = mputstr(src, "append_to_proc_queue(new_item);\n"
      "}\n\n");
  }

  /* incoming_exception functions */
  for (i = 0; i < pdef->proc_out.nElements; i++) {
    const port_proc_signature *sig = pdef->proc_out.elements + i;
    if (!sig->has_exceptions) continue;
    def = mputprintf(def, "void incoming_exception(const %s_exception& "
      "incoming_par, component sender_component", sig->name);
    if (pdef->testport_type == ADDRESS) {
      def = mputprintf(def, ", const %s *sender_address",
        pdef->address_name);
    }
    def = mputstr(def, ");\n");
    src = mputprintf(src,
      "void %s::incoming_exception(const %s_exception& incoming_par, "
      "component sender_component", class_name, sig->name);
    if (pdef->testport_type == ADDRESS) {
      src = mputprintf(src, ", const %s *sender_address",
        pdef->address_name);
    }
    src = mputstr(src, ")\n"
      "{\n"
      "if (!is_started) TTCN_error(\"Port %s is not started but an "
      "exception has arrived on it.\", port_name);\n"
      "proc_tail_count++;\n"
      "if (TTCN_Logger::log_this_event(TTCN_Logger::PORTEVENT_PQUEUE)) "
      "{\n"
      "TTCN_Logger::log_port_queue(TitanLoggerApiSimple::Port__Queue_operation::enqueue__exception,"
      " port_name, sender_component, proc_tail_count,\n"
      );
    if (pdef->testport_type == ADDRESS) {
      src = mputstr(src,
        "sender_address ?"
        /* Use the comma operator to get an expression of type CHARSTRING */
        " (TTCN_Logger::begin_event(TTCN_Logger::PORTEVENT_PQUEUE, TRUE),"
        " TTCN_Logger::log_char('('), sender_address->log(),"
        " TTCN_Logger::log_char(')'), TTCN_Logger::end_event_log2str()) : "
      );
    }
    src = mputprintf(src,
      /* This empty string may be an operand to a conditional operator from above */
      "CHARSTRING(0, NULL),\n"
      "(TTCN_Logger::begin_event(TTCN_Logger::PORTEVENT_PQUEUE, TRUE),"
      " TTCN_Logger::log_char(' '),"
      " incoming_par.log(), TTCN_Logger::end_event_log2str()));\n"
      "}\n"
      "proc_queue_item *new_item = new proc_queue_item;\n"
      "new_item->item_selection = EXCEPTION_%lu;\n"
      "new_item->exception_%lu = new %s_exception(incoming_par);\n"
      "new_item->sender_component = sender_component;\n",
      (unsigned long) i, (unsigned long) i, sig->name);
    if (pdef->testport_type == ADDRESS) {
      src = mputprintf(src, "if (sender_address != NULL) "
        "new_item->sender_address = new %s(*sender_address);\n"
        "else new_item->sender_address = NULL;\n", pdef->address_name);
    }
    src = mputstr(src, "append_to_proc_queue(new_item);\n"
      "}\n\n");
  }

  def = mputstr(def, "protected:\n");

  if (pdef->testport_type != INTERNAL) {
    /** functions provided for the test port to pass incoming messages or
     * procedure operations into the queue */
    if (pdef->port_type == REGULAR) {
      /* inline functions that are used through simple inheritance */
      for (i = 0; i < pdef->msg_in.nElements; i++) {
        /* wrapper for incoming_message */
        def = mputprintf(def, "inline void incoming_message("
          "const %s& incoming_par", pdef->msg_in.elements[i].name);
        if (pdef->testport_type == ADDRESS) {
          def = mputprintf(def, ", const %s *sender_address = NULL",
            pdef->address_name);
        }
        def = mputprintf(def, ") { incoming_message(incoming_par, "
          "SYSTEM_COMPREF%s); }\n",
          pdef->testport_type == ADDRESS ? ", sender_address" : "");
      }
      for (i = 0; i < pdef->proc_in.nElements; i++) {
        /* wrapper for incoming_call */
        def = mputprintf(def, "inline void incoming_call("
          "const %s_call& incoming_par",
          pdef->proc_in.elements[i].name);
        if (pdef->testport_type == ADDRESS) {
          def = mputprintf(def, ", const %s *sender_address = NULL",
            pdef->address_name);
        }
        def = mputprintf(def, ") { incoming_call(incoming_par, "
          "SYSTEM_COMPREF%s); }\n",
          pdef->testport_type == ADDRESS ? ", sender_address" : "");
      }
      for (i = 0; i < pdef->proc_out.nElements; i++) {
        /* wrapper for incoming_reply */
        if (pdef->proc_out.elements[i].is_noblock) continue;
        def = mputprintf(def, "inline void incoming_reply("
          "const %s_reply& incoming_par",
          pdef->proc_out.elements[i].name);
        if (pdef->testport_type == ADDRESS) {
          def = mputprintf(def, ", const %s *sender_address = NULL",
            pdef->address_name);
        }
        def = mputprintf(def, ") { incoming_reply(incoming_par, "
          "SYSTEM_COMPREF%s); }\n",
          pdef->testport_type == ADDRESS ? ", sender_address" : "");
      }
      for (i = 0; i < pdef->proc_out.nElements; i++) {
        /* wrapper for incoming_exception */
        if (!pdef->proc_out.elements[i].has_exceptions) continue;
        def = mputprintf(def, "inline void incoming_exception("
          "const %s_exception& incoming_par",
          pdef->proc_out.elements[i].name);
        if (pdef->testport_type == ADDRESS) {
          def = mputprintf(def, ", const %s *sender_address = NULL",
            pdef->address_name);
        }
        def = mputprintf(def, ") { incoming_exception(incoming_par, "
          "SYSTEM_COMPREF%s); }\n",
          pdef->testport_type == ADDRESS ? ", sender_address" : "");
      }
    } else {
      /** implementation of pure virtual functions that are defined in
       * the test port class */
      /* in case of PROVIDER port types the functions must handle the own
       * incoming messages, but in case of USER port types the incoming
       * messages of the provider port type must be handled */
      size_t nof_messages = pdef->port_type == USER ?
        pdef->provider_msg_in.nElements : pdef->msg_in.nElements;
      for (i = 0; i < nof_messages; i++) {
        /* incoming_message */
        const char *message_type = pdef->port_type == USER ?
          pdef->provider_msg_in.elements[i].name :
        pdef->msg_in.elements[i].name;
        def = mputprintf(def, "void incoming_message(const %s& "
          "incoming_par", message_type);
        if (pdef->testport_type == ADDRESS) {
          def = mputprintf(def, ", const %s *sender_address",
            pdef->address_name);
        }
        def = mputstr(def, ");\n");
        src = mputprintf(src, "void %s::incoming_message("
          "const %s& incoming_par", class_name, message_type);
        if (pdef->testport_type == ADDRESS) {
          src = mputprintf(src, ", const %s *sender_address",
            pdef->address_name);
        }
        src = mputprintf(src, ")\n"
          "{\n"
          "incoming_message(incoming_par, SYSTEM_COMPREF%s%s);\n"
          "}\n\n",
          (pdef->has_sliding ? ", sliding_buffer": ""),
          pdef->testport_type == ADDRESS ? ", sender_address" : "");
      }
      for (i = 0; i < pdef->proc_in.nElements; i++) {
        /* incoming_call */
        def = mputprintf(def, "void incoming_call(const %s_call& "
          "incoming_par", pdef->proc_in.elements[i].name);
        if (pdef->testport_type == ADDRESS) {
          def = mputprintf(def, ", const %s *sender_address",
            pdef->address_name);
        }
        def = mputstr(def, ");\n");
        src = mputprintf(src, "void %s::incoming_call(const %s_call& "
          "incoming_par", class_name, pdef->proc_in.elements[i].name);
        if (pdef->testport_type == ADDRESS) {
          src = mputprintf(src, ", const %s *sender_address",
            pdef->address_name);
        }
        src = mputprintf(src, ")\n"
          "{\n"
          "incoming_call(incoming_par, SYSTEM_COMPREF%s);\n"
          "}\n\n",
          pdef->testport_type == ADDRESS ? ", sender_address" : "");
      }
      for (i = 0; i < pdef->proc_out.nElements; i++) {
        /* incoming_reply */
        if (pdef->proc_out.elements[i].is_noblock) continue;
        def = mputprintf(def, "void incoming_reply(const %s_reply& "
          "incoming_par", pdef->proc_out.elements[i].name);
        if (pdef->testport_type == ADDRESS) {
          def = mputprintf(def, ", const %s *sender_address",
            pdef->address_name);
        }
        def = mputstr(def, ");\n");
        src = mputprintf(src, "void %s::incoming_reply(const %s_reply& "
          "incoming_par", class_name,
          pdef->proc_out.elements[i].name);
        if (pdef->testport_type == ADDRESS) {
          src = mputprintf(src, ", const %s *sender_address",
            pdef->address_name);
        }
        src = mputprintf(src, ")\n"
          "{\n"
          "incoming_reply(incoming_par, SYSTEM_COMPREF%s);\n"
          "}\n\n",
          pdef->testport_type == ADDRESS ? ", sender_address" : "");
      }
      for (i = 0; i < pdef->proc_out.nElements; i++) {
        /* incoming_exception */
        if (!pdef->proc_out.elements[i].has_exceptions) continue;
        def = mputprintf(def, "void incoming_exception("
          "const %s_exception& incoming_par",
          pdef->proc_out.elements[i].name);
        if (pdef->testport_type == ADDRESS) {
          def = mputprintf(def, ", const %s *sender_address",
            pdef->address_name);
        }
        def = mputstr(def, ");\n");
        src = mputprintf(src, "void %s::incoming_exception("
          "const %s_exception& incoming_par", class_name,
          pdef->proc_out.elements[i].name);
        if (pdef->testport_type == ADDRESS) {
          src = mputprintf(src, ", const %s *sender_address",
            pdef->address_name);
        }
        src = mputprintf(src, ")\n"
          "{\n"
          "incoming_exception(incoming_par, SYSTEM_COMPREF%s);\n"
          "}\n\n",
          pdef->testport_type == ADDRESS ? ", sender_address" : "");
      }
    }
  }

  if (has_msg_queue) {
    size_t nof_messages = pdef->port_type == USER ?
      pdef->provider_msg_in.nElements : pdef->msg_in.nElements;
    def = mputstr(def, "boolean process_message(const char *message_type, "
      "Text_Buf& incoming_buf, component sender_component, OCTETSTRING& slider);\n");
    src = mputprintf(src, "boolean %s::process_message("
      "const char *message_type, Text_Buf& incoming_buf, "
      "component sender_component, OCTETSTRING&%s)\n"
      "{\n", class_name,
      pdef->has_sliding ? " slider" : "");
    for (i = 0; i < nof_messages; i++) {
      const char *msg_name, *msg_dispname;
      if (pdef->port_type == USER) {
        msg_name = pdef->provider_msg_in.elements[i].name;
        msg_dispname = pdef->provider_msg_in.elements[i].dispname;
      } else {
        msg_name = pdef->msg_in.elements[i].name;
        msg_dispname = pdef->msg_in.elements[i].dispname;
      }
      src = mputprintf(src, "if (!strcmp(message_type, \"%s\")) {\n"
        "%s incoming_par;\n"
        "incoming_par.decode_text(incoming_buf);\n"
        "incoming_message(incoming_par, sender_component%s%s);\n"
        "return TRUE;\n"
        "} else ", msg_dispname, msg_name,
        (pdef->has_sliding ? ", slider" : ""),
        pdef->testport_type == ADDRESS ? ", NULL" : "");
    }
    src = mputstr(src, "return FALSE;\n"
      "}\n\n");
  }

  if (has_incoming_call) {
    def = mputstr(def, "boolean process_call(const char *signature_name, "
      "Text_Buf& incoming_buf, component sender_component);\n");
    src = mputprintf(src, "boolean %s::process_call("
      "const char *signature_name, Text_Buf& incoming_buf, "
      "component sender_component)\n"
      "{\n", class_name);
    for (i = 0; i < pdef->proc_in.nElements; i++) {
      src = mputprintf(src, "if (!strcmp(signature_name, \"%s\")) {\n"
        "%s_call incoming_par;\n"
        "incoming_par.decode_text(incoming_buf);\n"
        "incoming_call(incoming_par, sender_component%s);\n"
        "return TRUE;\n"
        "} else ", pdef->proc_in.elements[i].dispname,
        pdef->proc_in.elements[i].name,
        pdef->testport_type == ADDRESS ? ", NULL" : "");
    }
    src = mputstr(src, "return FALSE;\n"
      "}\n\n");
  }

  if (has_incoming_reply) {
    def = mputstr(def, "boolean process_reply(const char *signature_name, "
      "Text_Buf& incoming_buf, component sender_component);\n");
    src = mputprintf(src, "boolean %s::process_reply("
      "const char *signature_name, Text_Buf& incoming_buf, "
      "component sender_component)\n"
      "{\n", class_name);
    for (i = 0; i < pdef->proc_out.nElements; i++) {
      if (pdef->proc_out.elements[i].is_noblock) continue;
      src = mputprintf(src, "if (!strcmp(signature_name, \"%s\")) {\n"
        "%s_reply incoming_par;\n"
        "incoming_par.decode_text(incoming_buf);\n"
        "incoming_reply(incoming_par, sender_component%s);\n"
        "return TRUE;\n"
        "} else ", pdef->proc_out.elements[i].dispname,
        pdef->proc_out.elements[i].name,
        pdef->testport_type == ADDRESS ? ", NULL" : "");
    }
    src = mputstr(src, "return FALSE;\n"
      "}\n\n");
  }

  if (has_incoming_exception) {
    def = mputstr(def, "boolean process_exception("
      "const char *signature_name, Text_Buf& incoming_buf, "
      "component sender_component);\n");
    src = mputprintf(src, "boolean %s::process_exception("
      "const char *signature_name, Text_Buf& incoming_buf, "
      "component sender_component)\n"
      "{\n", class_name);
    for (i = 0; i < pdef->proc_out.nElements; i++) {
      if (!pdef->proc_out.elements[i].has_exceptions) continue;
      src = mputprintf(src, "if (!strcmp(signature_name, \"%s\")) {\n"
        "%s_exception incoming_par;\n"
        "incoming_par.decode_text(incoming_buf);\n"
        "incoming_exception(incoming_par, sender_component%s);\n"
        "return TRUE;\n"
        "} else ", pdef->proc_out.elements[i].dispname,
        pdef->proc_out.elements[i].name,
        pdef->testport_type == ADDRESS ? ", NULL" : "");
    }
    src = mputstr(src, "return FALSE;\n"
      "}\n\n");
  }

  /* Event handler prototype and empty implementation is not generated.
       When a port does not wait any events it is correct.
       (Call to the event handler will cause a dynamic test case error.) */

  def = mputstr(def, "};\n\n");
  /* end of base class */

  /* Putting everything to the output */

  output->header.class_decls = mputprintf(output->header.class_decls,
    "class %s;\n", class_name);
  if (pdef->testport_type != INTERNAL &&
     (pdef->port_type == REGULAR || (pdef->port_type == USER && !pdef->legacy)))
    output->header.class_decls = mputprintf(output->header.class_decls,
      "class %s;\n", pdef->name);

  output->header.class_defs = mputstr(output->header.class_defs, def);
  Free(def);

  if (pdef->testport_type != INTERNAL) {
    switch (pdef->port_type) {
    case USER:
      if (pdef->legacy) {
        break;
      }
      // else fall through
    case REGULAR:
      output->header.testport_includes = mputprintf(
        output->header.testport_includes, "#include \"%s.hh\"\n",
        duplicate_underscores ? pdef->name : pdef->filename);
      break;
    case PROVIDER:
      output->header.includes = mputprintf(output->header.includes,
        "#include \"%s.hh\"\n",
        duplicate_underscores ? pdef->name : pdef->filename);
    default:
      break;
    }
  }

  output->source.methods = mputstr(output->source.methods, src);
  Free(src);

  Free(class_name);
  Free(base_class_name);
}

void generateTestPortSkeleton(const port_def *pdef)
{
  char *class_name, *base_class_name;
  const char *file_prefix =
    duplicate_underscores ? pdef->name : pdef->filename;
  size_t i;
  char *header_name, *source_name;
  char *user_info = NULL;

  DEBUG(1, "Generating test port skeleton for port type `%s' ...",
    pdef->dispname);

  if (pdef->port_type == PROVIDER) {
    class_name = mprintf("%s_PROVIDER", pdef->name);
    base_class_name = mcopystr("PORT");
  } else {
    class_name = mcopystr(pdef->name);
    base_class_name = mprintf("%s_BASE", pdef->name);
  }

  if (output_dir != NULL) {
    header_name = mprintf("%s/%s.hh", output_dir, file_prefix);
    source_name = mprintf("%s/%s.cc", output_dir, file_prefix);
  } else {
    header_name = mprintf("%s.hh", file_prefix);
    source_name = mprintf("%s.cc", file_prefix);
  }

  if (force_overwrite || get_path_status(header_name) == PS_NONEXISTENT) {
    FILE *fp = fopen(header_name, "w");
    if (fp == NULL) {
      ERROR("Cannot open output test port skeleton header file `%s' for "
        "writing: %s", header_name, strerror(errno));
      exit(EXIT_FAILURE);
    }
    if (user_info == NULL) user_info = get_user_info();
    fprintf(fp,
      "// This Test Port skeleton header file was generated by the\n"
      "// TTCN-3 Compiler of the TTCN-3 Test Executor version "
      PRODUCT_NUMBER "\n"
      "// for %s\n"
      COPYRIGHT_STRING "\n\n"
      "// You may modify this file. Add your attributes and "
      "prototypes of your\n"
      "// member functions here.\n\n"
      "#ifndef %s_HH\n"
      "#define %s_HH\n\n", user_info, pdef->name, pdef->name);
    if (pdef->port_type == PROVIDER) {
      fprintf(fp, "#include <TTCN3.hh>\n\n"
        "// Note: Header file %s.hh must not be included into this "
        "file!\n"
        "// (because it includes this file)\n"
        "// Please add the declarations of message types manually.\n\n",
        duplicate_underscores ? pdef->module_name :
      pdef->module_dispname);
    } else {
      fprintf(fp, "#include \"%s.hh\"\n\n",
        duplicate_underscores ? pdef->module_name :
      pdef->module_dispname);
    }
    fprintf(fp, "namespace %s {\n\n", pdef->module_name);
    fprintf(fp,
      "class %s : public %s {\n"
      "public:\n"
      "\t%s(const char *par_port_name%s);\n"
      "\t~%s();\n\n"
      "\tvoid set_parameter(const char *parameter_name,\n"
      "\t\tconst char *parameter_value);\n\n"
      "private:\n"
      "\t/* void Handle_Fd_Event(int fd, boolean is_readable,\n"
      "\t\tboolean is_writable, boolean is_error); */\n"
      "\tvoid Handle_Fd_Event_Error(int fd);\n"
      "\tvoid Handle_Fd_Event_Writable(int fd);\n"
      "\tvoid Handle_Fd_Event_Readable(int fd);\n"
      "\t/* void Handle_Timeout(double time_since_last_call); */\n"
      "protected:\n"
      "\tvoid user_map(const char *system_port);\n"
      "\tvoid user_unmap(const char *system_port);\n\n"
      "\tvoid user_start();\n"
      "\tvoid user_stop();\n\n",
      class_name, base_class_name, class_name,
      pdef->port_type == REGULAR || pdef->port_type == USER ? " = NULL" : "", class_name);

    if (pdef->port_type == PROVIDER && pdef->n_mapper_name > 0) {
      fprintf(fp, "public:\n");
    }
    for (i = 0; i < pdef->msg_out.nElements; i++) {
      fprintf(fp, "\tvoid outgoing_send(const %s& send_par",
        pdef->msg_out.elements[i].name);
      if (pdef->testport_type == ADDRESS) {
        fprintf(fp, ",\n"
          "\t\tconst %s *destination_address", pdef->address_name);
      }
      fputs(");\n", fp);
    }
    for (i = 0; i < pdef->proc_out.nElements; i++) {
      fprintf(fp, "\tvoid outgoing_call(const %s_call& call_par",
        pdef->proc_out.elements[i].name);
      if (pdef->testport_type == ADDRESS) {
        fprintf(fp, ",\n"
          "\t\tconst %s *destination_address", pdef->address_name);
      }
      fputs(");\n" , fp);
    }
    for (i = 0; i < pdef->proc_in.nElements; i++) {
      if (pdef->proc_in.elements[i].is_noblock) continue;
      fprintf(fp, "\tvoid outgoing_reply(const %s_reply& reply_par",
        pdef->proc_in.elements[i].name);
      if (pdef->testport_type == ADDRESS) {
        fprintf(fp, ",\n"
          "\t\tconst %s *destination_address", pdef->address_name);
      }
      fputs(");\n", fp);
    }
    for (i = 0; i < pdef->proc_in.nElements; i++) {
      if (!pdef->proc_in.elements[i].has_exceptions) continue;
      fprintf(fp, "\tvoid outgoing_raise(const %s_exception& "
        "raise_exception", pdef->proc_in.elements[i].name);
      if (pdef->testport_type == ADDRESS) {
        fprintf(fp, ",\n"
          "\t\tconst %s *destination_address", pdef->address_name);
      }
      fputs(");\n", fp);
    }
    if (pdef->port_type == PROVIDER) {
      /* pure virtual functions for incoming operations */
      for (i = 0; i < pdef->msg_in.nElements; i++) {
        fprintf(fp, "\tvirtual void incoming_message(const %s& "
          "incoming_par", pdef->msg_in.elements[i].name);
        if (pdef->testport_type == ADDRESS) {
          fprintf(fp, ",\n"
            "\t\tconst %s *sender_address = NULL",
            pdef->address_name);
        }
        fputs(") = 0;\n", fp);
      }
      for (i = 0; i < pdef->proc_in.nElements; i++) {
        fprintf(fp, "\tvirtual void incoming_call(const %s_call& "
          "incoming_par", pdef->proc_in.elements[i].name);
        if (pdef->testport_type == ADDRESS) {
          fprintf(fp, ",\n"
            "\t\tconst %s *sender_address = NULL",
            pdef->address_name);
        }
        fputs(") = 0;\n", fp);
      }
      for (i = 0; i < pdef->proc_out.nElements; i++) {
        if (pdef->proc_out.elements[i].is_noblock) continue;
        fprintf(fp, "\tvirtual void incoming_reply(const %s_reply& "
          "incoming_par", pdef->proc_out.elements[i].name);
        if (pdef->testport_type == ADDRESS) {
          fprintf(fp, ",\n"
            "\t\tconst %s *sender_address = NULL",
            pdef->address_name);
        }
        fputs(") = 0;\n", fp);
      }
      for (i = 0; i < pdef->proc_out.nElements; i++) {
        if (!pdef->proc_out.elements[i].has_exceptions) continue;
        fprintf(fp, "\tvirtual void incoming_exception("
          "const %s_exception& incoming_par",
          pdef->proc_out.elements[i].name);
        if (pdef->testport_type == ADDRESS) {
          fprintf(fp, ",\n"
            "\t\tconst %s *sender_address = NULL",
            pdef->address_name);
        }
        fputs(") = 0;\n", fp);
      }
    }
    fputs("};\n\n", fp);
	fputs("} /* end of namespace */\n\n", fp);
    fputs("#endif\n", fp);
    fclose(fp);
    NOTIFY("Test port skeleton header file `%s' was generated for port "
      "type `%s'.", header_name, pdef->dispname);
  } else {
    DEBUG(1, "Test port header file `%s' already exists. It was not "
      "overwritten.", header_name);
  }

  if (force_overwrite || get_path_status(source_name) == PS_NONEXISTENT) {
    FILE *fp = fopen(source_name, "w");
    if (fp == NULL) {
      ERROR("Cannot open output test port skeleton source file `%s' for "
        "writing: %s", source_name, strerror(errno));
      exit(EXIT_FAILURE);
    }
    if (user_info == NULL) user_info = get_user_info();
    fprintf(fp,
      "// This Test Port skeleton source file was generated by the\n"
      "// TTCN-3 Compiler of the TTCN-3 Test Executor version "
      PRODUCT_NUMBER "\n"
      "// for %s\n"
      COPYRIGHT_STRING "\n\n"
      "// You may modify this file. Complete the body of empty functions and\n"
      "// add your member functions here.\n\n"
      "#include \"%s.hh\"\n", user_info, file_prefix);
    if (pdef->port_type == PROVIDER) {
      fprintf(fp, "#include \"%s.hh\"\n",
        duplicate_underscores ? pdef->module_name :
      pdef->module_dispname);
    }
    putc('\n', fp);
    fprintf(fp, "namespace %s {\n\n", pdef->module_name);
    fprintf(fp, "%s::%s(const char *par_port_name)\n"
      "\t: %s(par_port_name)\n"
      "{\n\n"
      "}\n\n"
      "%s::~%s()\n"
      "{\n\n"
      "}\n\n"
      "void %s::set_parameter(const char * /*parameter_name*/,\n"
      "\tconst char * /*parameter_value*/)\n"
      "{\n\n"
      "}\n\n"
      "/*void %s::Handle_Fd_Event(int fd, boolean is_readable,\n"
      "\tboolean is_writable, boolean is_error) {}*/\n\n"
      "void %s::Handle_Fd_Event_Error(int /*fd*/)\n"
      "{\n\n"
      "}\n\n"
      "void %s::Handle_Fd_Event_Writable(int /*fd*/)\n"
      "{\n\n"
      "}\n\n"
      "void %s::Handle_Fd_Event_Readable(int /*fd*/)\n"
      "{\n\n"
      "}\n\n"
      "/*void %s::Handle_Timeout(double time_since_last_call) {}*/\n\n"
      "void %s::user_map(const char * /*system_port*/)\n"
      "{\n\n"
      "}\n\n"
      "void %s::user_unmap(const char * /*system_port*/)\n"
      "{\n\n"
      "}\n\n"
      "void %s::user_start()\n"
      "{\n\n"
      "}\n\n"
      "void %s::user_stop()\n"
      "{\n\n"
      "}\n\n", class_name, class_name, base_class_name, class_name,
      class_name, class_name, class_name, class_name, class_name,
      class_name, class_name, class_name, class_name, class_name,
      class_name);
    for (i = 0; i < pdef->msg_out.nElements; i++) {
      fprintf(fp, "void %s::outgoing_send(const %s& /*send_par*/",
        class_name, pdef->msg_out.elements[i].name);
      if (pdef->testport_type == ADDRESS) {
        fprintf(fp, ",\n"
          "\tconst %s * /*destination_address*/", pdef->address_name);
      }
      fputs(")\n"
        "{\n\n"
        "}\n\n", fp);
    }
    for (i = 0; i < pdef->proc_out.nElements; i++) {
      fprintf(fp, "void %s::outgoing_call(const %s_call& /*call_par*/",
        class_name, pdef->proc_out.elements[i].name);
      if (pdef->testport_type == ADDRESS) {
        fprintf(fp, ",\n"
          "\tconst %s * /*destination_address*/", pdef->address_name);
      }
      fputs(")\n"
        "{\n\n"
        "}\n\n", fp);
    }
    for (i = 0; i < pdef->proc_in.nElements; i++) {
      if (pdef->proc_in.elements[i].is_noblock) continue;
      fprintf(fp, "void %s::outgoing_reply(const %s_reply& /*reply_par*/",
        class_name, pdef->proc_in.elements[i].name);
      if (pdef->testport_type == ADDRESS) {
        fprintf(fp, ",\n"
          "\tconst %s * /*destination_address*/", pdef->address_name);
      }
      fputs(")\n"
        "{\n\n"
        "}\n\n", fp);
    }
    for (i = 0; i < pdef->proc_in.nElements; i++) {
      if (!pdef->proc_in.elements[i].has_exceptions) continue;
      fprintf(fp, "void %s::outgoing_raise"
        "(const %s_exception& /*raise_exception*/",
        class_name, pdef->proc_in.elements[i].name);
      if (pdef->testport_type == ADDRESS) {
        fprintf(fp, ",\n"
          "\tconst %s * /*destination_address*/", pdef->address_name);
      }
      fputs(")\n"
        "{\n\n"
        "}\n\n", fp);
    }
	fputs("} /* end of namespace */\n\n", fp);
    fclose(fp);
    NOTIFY("Test port skeleton source file `%s' was generated for port "
      "type `%s'.", source_name, pdef->dispname);
  } else {
    DEBUG(1, "Test port source file `%s' already exists. It was not "
      "overwritten.", source_name);
  }

  Free(class_name);
  Free(base_class_name);
  Free(header_name);
  Free(source_name);
  Free(user_info);
}
