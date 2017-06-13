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
 *   Delic, Adam
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#ifndef ARRAY_HH
#define ARRAY_HH

#include "Types.h"
#include "Param_Types.hh"
#include "Error.hh"
#include "Logger.hh"
#include "Basetype.hh"
#include "Template.hh"
#include "Optional.hh"
#include "Parameters.h"
#include "Integer.hh"
#include "Struct_of.hh"
#include "memory.h"
#include "Component.hh"

class INTEGER;

extern unsigned int get_timer_array_index(int index_value,
  unsigned int array_size, int index_offset);
extern unsigned int get_timer_array_index(const INTEGER& index_value,
  unsigned int array_size, int index_offset);

/** @brief Runtime implementation of timer arrays.
 *
 * @param T_type the type of the array element. This can be \c TIMER_ARRAY
 * 		 for multi-dimensional arrays.
 * @param array_size the number of elements in the array
 * @param index_offset the lowest index
 *
 * */
template <typename T_type, unsigned int array_size, int index_offset>
class TIMER_ARRAY 
#ifdef TITAN_RUNTIME_2
  : public RefdIndexInterface
#endif
{
  T_type array_elements[array_size];
  char * names[array_size];

  /// Copy constructor disallowed.
  TIMER_ARRAY(const TIMER_ARRAY& other_value);
  /// Assignment disallowed.
  TIMER_ARRAY& operator=(const TIMER_ARRAY& other_value);

public:
  TIMER_ARRAY() { }

  ~TIMER_ARRAY() {
    for (unsigned int i = 0; i < array_size; ++i) {
      Free(names[i]);
    }
  }

  T_type& operator[](int index_value) { return array_elements[
    get_timer_array_index(index_value, array_size, index_offset)]; }
  T_type& operator[](const INTEGER& index_value) { return array_elements[
    get_timer_array_index(index_value, array_size, index_offset)]; }

  int n_elem()   const { return array_size; }
  int size_of()  const { return array_size; }
  int lengthof() const { return array_size; }

  T_type& array_element(unsigned int index_value)
    { return array_elements[index_value]; }

  void set_name(const char * name_string)
  {
    for (int i = 0; i < (int)array_size; ++i) {
      // index_offset may be negative, hence i must be int (not size_t)
      // to ensure that signed arithmetic is used.
      names[i] = mputprintf(mcopystr(name_string), "[%d]", index_offset+i);
      array_elements[i].set_name(names[i]);
    }
  }

  void log() const
  {
    TTCN_Logger::log_event_str("{ ");
    for (unsigned int v_index = 0; v_index < array_size; v_index++) {
      if (v_index > 0) TTCN_Logger::log_event_str(", ");
      array_elements[v_index].log();
    }
    TTCN_Logger::log_event_str(" }");
  }
  
  // alt-status priority: ALT_YES (return immediately) > ALT_REPEAT > ALT_MAYBE > ALT_NO
  alt_status timeout(Index_Redirect* index_redirect)
  {
    if (index_redirect != NULL) {
      index_redirect->incr_pos();
    }
    alt_status result = ALT_NO;
    for (unsigned int i = 0; i < array_size; ++i) {
      alt_status ret_val = array_elements[i].timeout(index_redirect);
      if (ret_val == ALT_YES) {
        if (index_redirect != NULL) {
          index_redirect->add_index((int)i + index_offset);
        }
        result = ret_val;
        break;
      }
      else if (ret_val == ALT_REPEAT ||
               (ret_val == ALT_MAYBE && result == ALT_NO)) {
        result = ret_val;
      }
    }
    if (index_redirect != NULL) {
      index_redirect->decr_pos();
    }
    return result;
  }
  
  boolean running(Index_Redirect* index_redirect) const
  {
    if (index_redirect != NULL) {
      index_redirect->incr_pos();
    }
    boolean ret_val = FALSE;
    for (unsigned int i = 0; i < array_size; ++i) {
      ret_val = array_elements[i].running(index_redirect);
      if (ret_val) {
        if (index_redirect != NULL) {
          index_redirect->add_index((int)i + index_offset);
        }
        break;
      }
    }
    if (index_redirect != NULL) {
      index_redirect->decr_pos();
    }
    return ret_val;
  }
};

extern unsigned int get_port_array_index(int index_value,
  unsigned int array_size, int index_offset);
extern unsigned int get_port_array_index(const INTEGER& index_value,
  unsigned int array_size, int index_offset);

template <typename T_type, unsigned int array_size, int index_offset>
class PORT_ARRAY
#ifdef TITAN_RUNTIME_2
  : public RefdIndexInterface
#endif
{
  T_type array_elements[array_size];
  char * names[array_size];

  /// Copy constructor disallowed.
  PORT_ARRAY(const PORT_ARRAY& other_value);
  /// Assignment disallowed.
  PORT_ARRAY& operator=(const PORT_ARRAY& other_value);

public:
  PORT_ARRAY() { }

  ~PORT_ARRAY() {
    for (unsigned int i = 0; i < array_size; ++i) {
      Free(names[i]);
    }
  }

  T_type& operator[](int index_value) { return array_elements[
    get_port_array_index(index_value, array_size, index_offset)]; }
  T_type& operator[](const INTEGER& index_value) { return array_elements[
    get_port_array_index(index_value, array_size, index_offset)]; }

  int n_elem()   const { return array_size; }
  int size_of() const { return array_size; }
  int lengthof()const { return array_size; }

  void set_name(const char * name_string)
  {
    for (int i = 0; i < (int)array_size; ++i) {
      // i must be int, see comment in TIMER_ARRAY::set_name
      names[i] = mputprintf(mcopystr(name_string), "[%d]", index_offset+i);
      array_elements[i].set_name(names[i]);
    }
  }

  void activate_port()
  {
    for (unsigned int v_index = 0; v_index < array_size; v_index++) {
      array_elements[v_index].activate_port();
    }
  }

  void log() const
  {
    TTCN_Logger::log_event_str("{ ");
    for (unsigned int v_index = 0; v_index < array_size; v_index++) {
      if (v_index > 0) TTCN_Logger::log_event_str(", ");
      array_elements[v_index].log();
    }
    TTCN_Logger::log_event_str(" }");
  }
  
  // alt-status priority: ALT_YES (return immediately) > ALT_REPEAT > ALT_MAYBE > ALT_NO
  alt_status receive(const COMPONENT_template& sender_template,
                     COMPONENT *sender_ptr, Index_Redirect* index_redirect)
  {
    if (index_redirect != NULL) {
      index_redirect->incr_pos();
    }
    alt_status result = ALT_NO;
    for (unsigned int i = 0; i < array_size; ++i) {
      alt_status ret_val = array_elements[i].receive(sender_template,
        sender_ptr, index_redirect);
      if (ret_val == ALT_YES) {
        if (index_redirect != NULL) {
          index_redirect->add_index((int)i + index_offset);
        }
        result = ret_val;
        break;
      }
      else if (ret_val == ALT_REPEAT ||
               (ret_val == ALT_MAYBE && result == ALT_NO)) {
        result = ret_val;
      }
    }
    if (index_redirect != NULL) {
      index_redirect->decr_pos();
    }
    return result;
  }
  
  template <typename T_template>
#ifdef TITAN_RUNTIME_2
  alt_status receive(const T_template& value_template,
                     Value_Redirect_Interface* value_redirect,
#else
  // the C++ compiler cannot determine the type of the template argument
  // if it's NULL, so a separate function is needed for this case in RT1
  alt_status receive(const T_template& value_template, int /* NULL */,
                     const COMPONENT_template& sender_template,
                     COMPONENT* sender_ptr, Index_Redirect* index_redirect)
  {
    if (index_redirect != NULL) {
      index_redirect->incr_pos();
    }
    alt_status result = ALT_NO;
    for (unsigned int i = 0; i < array_size; ++i) {
      alt_status ret_val = array_elements[i].receive(value_template, NULL,
        sender_template, sender_ptr, index_redirect);
      if (ret_val == ALT_YES) {
        if (index_redirect != NULL) {
          index_redirect->add_index((int)i + index_offset);
        }
        result = ret_val;
        break;
      }
      else if (ret_val == ALT_REPEAT ||
               (ret_val == ALT_MAYBE && result == ALT_NO)) {
        result = ret_val;
      }
    }
    if (index_redirect != NULL) {
      index_redirect->decr_pos();
    }
    return result;
  }
  
  template <typename T_value_redirect, typename T_template>
  alt_status receive(const T_template& value_template,
                     T_value_redirect* value_redirect,
#endif
                     const COMPONENT_template& sender_template,
                     COMPONENT* sender_ptr, Index_Redirect* index_redirect)
  {
    if (index_redirect != NULL) {
      index_redirect->incr_pos();
    }
    alt_status result = ALT_NO;
    for (unsigned int i = 0; i < array_size; ++i) {
      alt_status ret_val = array_elements[i].receive(value_template,
        value_redirect, sender_template, sender_ptr, index_redirect);
      if (ret_val == ALT_YES) {
        if (index_redirect != NULL) {
          index_redirect->add_index((int)i + index_offset);
        }
        result = ret_val;
        break;
      }
      else if (ret_val == ALT_REPEAT ||
               (ret_val == ALT_MAYBE && result == ALT_NO)) {
        result = ret_val;
      }
    }
    if (index_redirect != NULL) {
      index_redirect->decr_pos();
    }
    return result;
  }
  
  alt_status check_receive(const COMPONENT_template& sender_template,
                           COMPONENT* sender_ptr, Index_Redirect* index_redirect)
  {
    if (index_redirect != NULL) {
      index_redirect->incr_pos();
    }
    alt_status result = ALT_NO;
    for (unsigned int i = 0; i < array_size; ++i) {
      alt_status ret_val = array_elements[i].check_receive(sender_template,
        sender_ptr, index_redirect);
      if (ret_val == ALT_YES) {
        if (index_redirect != NULL) {
          index_redirect->add_index((int)i + index_offset);
        }
        result = ret_val;
        break;
      }
      else if (ret_val == ALT_REPEAT ||
               (ret_val == ALT_MAYBE && result == ALT_NO)) {
        result = ret_val;
      }
    }
    if (index_redirect != NULL) {
      index_redirect->decr_pos();
    }
    return result;
  }
  
  template <typename T_template>
#ifdef TITAN_RUNTIME_2
  alt_status check_receive(const T_template& value_template,
                           Value_Redirect_Interface* value_redirect,
#else
  alt_status check_receive(const T_template& value_template, int /* NULL */,
                           const COMPONENT_template& sender_template,
                           COMPONENT* sender_ptr, Index_Redirect* index_redirect)
  {
    if (index_redirect != NULL) {
      index_redirect->incr_pos();
    }
    alt_status result = ALT_NO;
    for (unsigned int i = 0; i < array_size; ++i) {
      alt_status ret_val = array_elements[i].check_receive(value_template, NULL,
        sender_template, sender_ptr, index_redirect);
      if (ret_val == ALT_YES) {
        if (index_redirect != NULL) {
          index_redirect->add_index((int)i + index_offset);
        }
        result = ret_val;
        break;
      }
      else if (ret_val == ALT_REPEAT ||
               (ret_val == ALT_MAYBE && result == ALT_NO)) {
        result = ret_val;
      }
    }
    if (index_redirect != NULL) {
      index_redirect->decr_pos();
    }
    return result;
  }
  
  template <typename T_value_redirect, typename T_template>
  alt_status check_receive(const T_template& value_template,
                           T_value_redirect* value_redirect,
#endif
                           const COMPONENT_template& sender_template,
                           COMPONENT* sender_ptr, Index_Redirect* index_redirect)
  {
    if (index_redirect != NULL) {
      index_redirect->incr_pos();
    }
    alt_status result = ALT_NO;
    for (unsigned int i = 0; i < array_size; ++i) {
      alt_status ret_val = array_elements[i].check_receive(value_template,
        value_redirect, sender_template, sender_ptr, index_redirect);
      if (ret_val == ALT_YES) {
        if (index_redirect != NULL) {
          index_redirect->add_index((int)i + index_offset);
        }
        result = ret_val;
        break;
      }
      else if (ret_val == ALT_REPEAT ||
               (ret_val == ALT_MAYBE && result == ALT_NO)) {
        result = ret_val;
      }
    }
    if (index_redirect != NULL) {
      index_redirect->decr_pos();
    }
    return result;
  }
  
  alt_status trigger(const COMPONENT_template& sender_template,
                     COMPONENT* sender_ptr, Index_Redirect* index_redirect)
  {
    if (index_redirect != NULL) {
      index_redirect->incr_pos();
    }
    alt_status result = ALT_NO;
    for (unsigned int i = 0; i < array_size; ++i) {
      alt_status ret_val = array_elements[i].trigger(sender_template,
        sender_ptr, index_redirect);
      if (ret_val == ALT_YES) {
        if (index_redirect != NULL) {
          index_redirect->add_index((int)i + index_offset);
        }
        result = ret_val;
        break;
      }
      else if (ret_val == ALT_REPEAT ||
               (ret_val == ALT_MAYBE && result == ALT_NO)) {
        result = ret_val;
      }
    }
    if (index_redirect != NULL) {
      index_redirect->decr_pos();
    }
    return result;
  }
  
  template <typename T_template>
#ifdef TITAN_RUNTIME_2
  alt_status trigger(const T_template& value_template,
                     Value_Redirect_Interface* value_redirect,
#else
  alt_status trigger(const T_template& value_template, int /* NULL */,
                     const COMPONENT_template& sender_template,
                     COMPONENT* sender_ptr, Index_Redirect* index_redirect)
  {
    if (index_redirect != NULL) {
      index_redirect->incr_pos();
    }
    alt_status result = ALT_NO;
    for (unsigned int i = 0; i < array_size; ++i) {
      alt_status ret_val = array_elements[i].trigger(value_template, NULL,
        sender_template, sender_ptr, index_redirect);
      if (ret_val == ALT_YES) {
        if (index_redirect != NULL) {
          index_redirect->add_index((int)i + index_offset);
        }
        result = ret_val;
        break;
      }
      else if (ret_val == ALT_REPEAT ||
               (ret_val == ALT_MAYBE && result == ALT_NO)) {
        result = ret_val;
      }
    }
    if (index_redirect != NULL) {
      index_redirect->decr_pos();
    }
    return result;
  }
  
  template <typename T_value_redirect, typename T_template>
  alt_status trigger(const T_template& value_template,
                     T_value_redirect* value_redirect,
#endif
                     const COMPONENT_template& sender_template,
                     COMPONENT* sender_ptr, Index_Redirect* index_redirect)
  {
    if (index_redirect != NULL) {
      index_redirect->incr_pos();
    }
    alt_status result = ALT_NO;
    for (unsigned int i = 0; i < array_size; ++i) {
      alt_status ret_val = array_elements[i].trigger(value_template,
        value_redirect, sender_template, sender_ptr, index_redirect);
      if (ret_val == ALT_YES) {
        if (index_redirect != NULL) {
          index_redirect->add_index((int)i + index_offset);
        }
        result = ret_val;
        break;
      }
      else if (ret_val == ALT_REPEAT ||
               (ret_val == ALT_MAYBE && result == ALT_NO)) {
        result = ret_val;
      }
    }
    if (index_redirect != NULL) {
      index_redirect->decr_pos();
    }
    return result;
  }
  
  alt_status getcall(const COMPONENT_template& sender_template,
                     COMPONENT* sender_ptr, Index_Redirect* index_redirect)
  {
    if (index_redirect != NULL) {
      index_redirect->incr_pos();
    }
    alt_status result = ALT_NO;
    for (unsigned int i = 0; i < array_size; ++i) {
      alt_status ret_val = array_elements[i].getcall(sender_template,
        sender_ptr, index_redirect);
      if (ret_val == ALT_YES) {
        if (index_redirect != NULL) {
          index_redirect->add_index((int)i + index_offset);
        }
        result = ret_val;
        break;
      }
      else if (ret_val == ALT_REPEAT ||
               (ret_val == ALT_MAYBE && result == ALT_NO)) {
        result = ret_val;
      }
    }
    if (index_redirect != NULL) {
      index_redirect->decr_pos();
    }
    return result;
  }
  
  template <typename T_template, typename T_parameter_redirect>
  alt_status getcall(const T_template& getcall_template,
                     const COMPONENT_template& sender_template,
                     const T_parameter_redirect& param_ref,
                     COMPONENT* sender_ptr, Index_Redirect* index_redirect)
  {
    if (index_redirect != NULL) {
      index_redirect->incr_pos();
    }
    alt_status result = ALT_NO;
    for (unsigned int i = 0; i < array_size; ++i) {
      alt_status ret_val = array_elements[i].getcall(getcall_template,
        sender_template, param_ref, sender_ptr, index_redirect);
      if (ret_val == ALT_YES) {
        if (index_redirect != NULL) {
          index_redirect->add_index((int)i + index_offset);
        }
        result = ret_val;
        break;
      }
      else if (ret_val == ALT_REPEAT ||
               (ret_val == ALT_MAYBE && result == ALT_NO)) {
        result = ret_val;
      }
    }
    if (index_redirect != NULL) {
      index_redirect->decr_pos();
    }
    return result;
  }
  
  alt_status check_getcall(const COMPONENT_template& sender_template,
                           COMPONENT* sender_ptr, Index_Redirect* index_redirect)
  {
    if (index_redirect != NULL) {
      index_redirect->incr_pos();
    }
    alt_status result = ALT_NO;
    for (unsigned int i = 0; i < array_size; ++i) {
      alt_status ret_val = array_elements[i].check_getcall(sender_template,
        sender_ptr, index_redirect);
      if (ret_val == ALT_YES) {
        if (index_redirect != NULL) {
          index_redirect->add_index((int)i + index_offset);
        }
        result = ret_val;
        break;
      }
      else if (ret_val == ALT_REPEAT ||
               (ret_val == ALT_MAYBE && result == ALT_NO)) {
        result = ret_val;
      }
    }
    if (index_redirect != NULL) {
      index_redirect->decr_pos();
    }
    return result;
  }
  
  template <typename T_template, typename T_parameter_redirect>
  alt_status check_getcall(const T_template& getcall_template,
                           const COMPONENT_template& sender_template,
                           const T_parameter_redirect& param_ref,
                           COMPONENT* sender_ptr, Index_Redirect* index_redirect)
  {
    if (index_redirect != NULL) {
      index_redirect->incr_pos();
    }
    alt_status result = ALT_NO;
    for (unsigned int i = 0; i < array_size; ++i) {
      alt_status ret_val = array_elements[i].check_getcall(getcall_template,
        sender_template, param_ref, sender_ptr, index_redirect);
      if (ret_val == ALT_YES) {
        if (index_redirect != NULL) {
          index_redirect->add_index((int)i + index_offset);
        }
        result = ret_val;
        break;
      }
      else if (ret_val == ALT_REPEAT ||
               (ret_val == ALT_MAYBE && result == ALT_NO)) {
        result = ret_val;
      }
    }
    if (index_redirect != NULL) {
      index_redirect->decr_pos();
    }
    return result;
  }
  
  alt_status getreply(const COMPONENT_template& sender_template,
                      COMPONENT* sender_ptr, Index_Redirect* index_redirect)
  {
    if (index_redirect != NULL) {
      index_redirect->incr_pos();
    }
    alt_status result = ALT_NO;
    for (unsigned int i = 0; i < array_size; ++i) {
      alt_status ret_val = array_elements[i].getreply(sender_template,
        sender_ptr, index_redirect);
      if (ret_val == ALT_YES) {
        if (index_redirect != NULL) {
          index_redirect->add_index((int)i + index_offset);
        }
        result = ret_val;
        break;
      }
      else if (ret_val == ALT_REPEAT ||
               (ret_val == ALT_MAYBE && result == ALT_NO)) {
        result = ret_val;
      }
    }
    if (index_redirect != NULL) {
      index_redirect->decr_pos();
    }
    return result;
  }
  
  template <typename T_template, typename T_parameter_redirect>
  alt_status getreply(const T_template& getreply_template,
                      const COMPONENT_template& sender_template,
                      const T_parameter_redirect& param_ref,
                      COMPONENT* sender_ptr, Index_Redirect* index_redirect)
  {
    if (index_redirect != NULL) {
      index_redirect->incr_pos();
    }
    alt_status result = ALT_NO;
    for (unsigned int i = 0; i < array_size; ++i) {
      alt_status ret_val = array_elements[i].getreply(getreply_template,
        sender_template, param_ref, sender_ptr, index_redirect);
      if (ret_val == ALT_YES) {
        if (index_redirect != NULL) {
          index_redirect->add_index((int)i + index_offset);
        }
        result = ret_val;
        break;
      }
      else if (ret_val == ALT_REPEAT ||
               (ret_val == ALT_MAYBE && result == ALT_NO)) {
        result = ret_val;
      }
    }
    if (index_redirect != NULL) {
      index_redirect->decr_pos();
    }
    return result;
  }
  
  alt_status check_getreply(const COMPONENT_template& sender_template,
                            COMPONENT* sender_ptr, Index_Redirect* index_redirect)
  {
    if (index_redirect != NULL) {
      index_redirect->incr_pos();
    }
    alt_status result = ALT_NO;
    for (unsigned int i = 0; i < array_size; ++i) {
      alt_status ret_val = array_elements[i].check_getreply(sender_template,
        sender_ptr, index_redirect);
      if (ret_val == ALT_YES) {
        if (index_redirect != NULL) {
          index_redirect->add_index((int)i + index_offset);
        }
        result = ret_val;
        break;
      }
      else if (ret_val == ALT_REPEAT ||
               (ret_val == ALT_MAYBE && result == ALT_NO)) {
        result = ret_val;
      }
    }
    if (index_redirect != NULL) {
      index_redirect->decr_pos();
    }
    return result;
  }
  
  template <typename T_template, typename T_parameter_redirect>
  alt_status check_getreply(const T_template& getreply_template,
                            const COMPONENT_template& sender_template,
                            const T_parameter_redirect& param_ref,
                            COMPONENT* sender_ptr, Index_Redirect* index_redirect)
  {
    if (index_redirect != NULL) {
      index_redirect->incr_pos();
    }
    alt_status result = ALT_NO;
    for (unsigned int i = 0; i < array_size; ++i) {
      alt_status ret_val = array_elements[i].check_getreply(getreply_template,
        sender_template, param_ref, sender_ptr, index_redirect);
      if (ret_val == ALT_YES) {
        if (index_redirect != NULL) {
          index_redirect->add_index((int)i + index_offset);
        }
        result = ret_val;
        break;
      }
      else if (ret_val == ALT_REPEAT ||
               (ret_val == ALT_MAYBE && result == ALT_NO)) {
        result = ret_val;
      }
    }
    if (index_redirect != NULL) {
      index_redirect->decr_pos();
    }
    return result;
  }
  
  alt_status get_exception(const COMPONENT_template& sender_template,
                           COMPONENT* sender_ptr, Index_Redirect* index_redirect)
  {
    if (index_redirect != NULL) {
      index_redirect->incr_pos();
    }
    alt_status result = ALT_NO;
    for (unsigned int i = 0; i < array_size; ++i) {
      alt_status ret_val = array_elements[i].get_exception(sender_template,
        sender_ptr, index_redirect);
      if (ret_val == ALT_YES) {
        if (index_redirect != NULL) {
          index_redirect->add_index((int)i + index_offset);
        }
        result = ret_val;
        break;
      }
      else if (ret_val == ALT_REPEAT ||
               (ret_val == ALT_MAYBE && result == ALT_NO)) {
        result = ret_val;
      }
    }
    if (index_redirect != NULL) {
      index_redirect->decr_pos();
    }
    return result;
  }
  
  template <typename T_template>
  alt_status get_exception(const T_template& catch_template,
                           const COMPONENT_template& sender_template,
                           COMPONENT* sender_ptr, Index_Redirect* index_redirect)
  {
    if (index_redirect != NULL) {
      index_redirect->incr_pos();
    }
    alt_status result = ALT_NO;
    for (unsigned int i = 0; i < array_size; ++i) {
      alt_status ret_val = array_elements[i].get_exception(catch_template,
        sender_template, sender_ptr, index_redirect);
      if (ret_val == ALT_YES) {
        if (index_redirect != NULL) {
          index_redirect->add_index((int)i + index_offset);
        }
        result = ret_val;
        break;
      }
      else if (ret_val == ALT_REPEAT ||
               (ret_val == ALT_MAYBE && result == ALT_NO)) {
        result = ret_val;
      }
    }
    if (index_redirect != NULL) {
      index_redirect->decr_pos();
    }
    return result;
  }
  
  alt_status check_catch(const COMPONENT_template& sender_template,
                         COMPONENT* sender_ptr, Index_Redirect* index_redirect)
  {
    if (index_redirect != NULL) {
      index_redirect->incr_pos();
    }
    alt_status result = ALT_NO;
    for (unsigned int i = 0; i < array_size; ++i) {
      alt_status ret_val = array_elements[i].check_catch(sender_template,
        sender_ptr, index_redirect);
      if (ret_val == ALT_YES) {
        if (index_redirect != NULL) {
          index_redirect->add_index((int)i + index_offset);
        }
        result = ret_val;
        break;
      }
      else if (ret_val == ALT_REPEAT ||
               (ret_val == ALT_MAYBE && result == ALT_NO)) {
        result = ret_val;
      }
    }
    if (index_redirect != NULL) {
      index_redirect->decr_pos();
    }
    return result;
  }
  
  template <typename T_template>
  alt_status check_catch(const T_template& catch_template,
                         const COMPONENT_template& sender_template,
                         COMPONENT* sender_ptr, Index_Redirect* index_redirect)
  {
    if (index_redirect != NULL) {
      index_redirect->incr_pos();
    }
    alt_status result = ALT_NO;
    for (unsigned int i = 0; i < array_size; ++i) {
      alt_status ret_val = array_elements[i].check_catch(catch_template,
        sender_template, sender_ptr, index_redirect);
      if (ret_val == ALT_YES) {
        if (index_redirect != NULL) {
          index_redirect->add_index((int)i + index_offset);
        }
        result = ret_val;
        break;
      }
      else if (ret_val == ALT_REPEAT ||
               (ret_val == ALT_MAYBE && result == ALT_NO)) {
        result = ret_val;
      }
    }
    if (index_redirect != NULL) {
      index_redirect->decr_pos();
    }
    return result;
  }
  
  alt_status check(const COMPONENT_template& sender_template,
                   COMPONENT* sender_ptr, Index_Redirect* index_redirect)
  {
    if (index_redirect != NULL) {
      index_redirect->incr_pos();
    }
    alt_status result = ALT_NO;
    for (unsigned int i = 0; i < array_size; ++i) {
      alt_status ret_val = array_elements[i].check(sender_template, sender_ptr,
        index_redirect);
      if (ret_val == ALT_YES) {
        if (index_redirect != NULL) {
          index_redirect->add_index((int)i + index_offset);
        }
        result = ret_val;
        break;
      }
      else if (ret_val == ALT_REPEAT ||
               (ret_val == ALT_MAYBE && result == ALT_NO)) {
        result = ret_val;
      }
    }
    if (index_redirect != NULL) {
      index_redirect->decr_pos();
    }
    return result;
  }
};

////////////////////////////////////////////////////////////////////////////////

extern unsigned int get_array_index(int index_value,
  unsigned int array_size, int index_offset);
extern unsigned int get_array_index(const INTEGER& index_value,
  unsigned int array_size, int index_offset);

template <typename T_type, unsigned int array_size, int index_offset>
class VALUE_ARRAY : public Base_Type
#ifdef TITAN_RUNTIME_2
  , public RefdIndexInterface
#endif
{
  T_type array_elements[array_size];
public:
  // This class use the compiler-generated copy constructor and
  // copy assignment.
    
  // User defined default constructor
  VALUE_ARRAY() : array_elements(){};
  boolean operator==(const VALUE_ARRAY& other_value) const;
  inline boolean operator!=(const VALUE_ARRAY& other_value) const
    { return !(*this == other_value); }

  T_type& operator[](int index_value) { return array_elements[
    get_array_index(index_value, array_size, index_offset)]; }
  T_type& operator[](const INTEGER& index_value) { return array_elements[
    get_array_index(index_value, array_size, index_offset)]; }
  const T_type& operator[](int index_value) const { return array_elements[
      get_array_index(index_value, array_size, index_offset)]; }
  const T_type& operator[](const INTEGER& index_value) const {
    return array_elements[
      get_array_index(index_value, array_size, index_offset)];
  }

  // rotation
  VALUE_ARRAY operator<<=(int rotate_count) const;
  VALUE_ARRAY operator<<=(const INTEGER& rotate_count) const;
  VALUE_ARRAY operator>>=(int rotate_count) const;
  VALUE_ARRAY operator>>=(const INTEGER& rotate_count) const;

  T_type& array_element(unsigned int index_value)
    { return array_elements[index_value]; }
  const T_type& array_element(unsigned int index_value) const
    { return array_elements[index_value]; }

  void set_implicit_omit();

  boolean is_bound() const;
  boolean is_value() const;
  void clean_up();
  void log() const;

  inline int n_elem()  const { return array_size; }
  inline int size_of() const { return array_size; }
  int lengthof() const;

  void set_param(Module_Param& param);

#ifdef TITAN_RUNTIME_2
  Module_Param* get_param(Module_Param_Name& param_name) const;
  boolean is_equal(const Base_Type* other_value) const { return *this == *(static_cast<const VALUE_ARRAY*>(other_value)); }
  void set_value(const Base_Type* other_value) { *this = *(static_cast<const VALUE_ARRAY*>(other_value)); }
  Base_Type* clone() const { return new VALUE_ARRAY(*this); }
  const TTCN_Typedescriptor_t* get_descriptor() const { TTCN_error("Internal error: VALUE_ARRAY<>::get_descriptor() called."); }
#else
  inline boolean is_present() const { return is_bound(); }
#endif

  void encode_text(Text_Buf& text_buf) const;
  void decode_text(Text_Buf& text_buf);

  void encode(const TTCN_Typedescriptor_t&, TTCN_Buffer&, TTCN_EncDec::coding_t, ...) const;
  void decode(const TTCN_Typedescriptor_t&, TTCN_Buffer&, TTCN_EncDec::coding_t, ...);
  
  virtual const TTCN_Typedescriptor_t* get_elem_descr() const 
  { TTCN_error("Internal error: VALUE_ARRAY<>::get_elem_descr() called."); }
  
  virtual ~VALUE_ARRAY() { } // just to avoid warnings
  
  /** Encodes accordingly to the JSON encoding rules.
    * Returns the length of the encoded data. */
  int JSON_encode(const TTCN_Typedescriptor_t&, JSON_Tokenizer&) const;
  
  /** Decodes accordingly to the JSON encoding rules.
    * Returns the length of the decoded data. */
  int JSON_decode(const TTCN_Typedescriptor_t&, JSON_Tokenizer&, boolean);
  
  // alt-status priority: ALT_YES (return immediately) > ALT_REPEAT > ALT_MAYBE > ALT_NO
  alt_status done(Index_Redirect* index_redirect) const
  {
    if (index_redirect != NULL) {
      index_redirect->incr_pos();
    }
    alt_status result = ALT_NO;
    for (unsigned int i = 0; i < array_size; ++i) {
      alt_status ret_val = array_elements[i].done(index_redirect);
      if (ret_val == ALT_YES) {
        if (index_redirect != NULL) {
          index_redirect->add_index((int)i + index_offset);
        }
        result = ret_val;
        break;
      }
      else if (ret_val == ALT_REPEAT ||
               (ret_val == ALT_MAYBE && result == ALT_NO)) {
        result = ret_val;
      }
    }
    if (index_redirect != NULL) {
      index_redirect->decr_pos();
    }
    return result;
  }
  
  alt_status killed(Index_Redirect* index_redirect) const
  {
    if (index_redirect != NULL) {
      index_redirect->incr_pos();
    }
    alt_status result = ALT_NO;
    for (unsigned int i = 0; i < array_size; ++i) {
      alt_status ret_val = array_elements[i].killed(index_redirect);
      if (ret_val == ALT_YES) {
        if (index_redirect != NULL) {
          index_redirect->add_index((int)i + index_offset);
        }
        result = ret_val;
        break;
      }
      else if (ret_val == ALT_REPEAT ||
               (ret_val == ALT_MAYBE && result == ALT_NO)) {
        result = ret_val;
      }
    }
    if (index_redirect != NULL) {
      index_redirect->decr_pos();
    }
    return result;
  }
  
  boolean running(Index_Redirect* index_redirect) const
  {
    if (index_redirect != NULL) {
      index_redirect->incr_pos();
    }
    boolean ret_val = FALSE;
    for (unsigned int i = 0; i < array_size; ++i) {
      ret_val = array_elements[i].running(index_redirect);
      if (ret_val) {
        if (index_redirect != NULL) {
          index_redirect->add_index((int)i + index_offset);
        }
        break;
      }
    }
    if (index_redirect != NULL) {
      index_redirect->decr_pos();
    }
    return ret_val;
  }
  
  boolean alive(Index_Redirect* index_redirect) const
  {
    if (index_redirect != NULL) {
      index_redirect->incr_pos();
    }
    boolean ret_val = FALSE;
    for (unsigned int i = 0; i < array_size; ++i) {
      ret_val = array_elements[i].alive(index_redirect);
      if (ret_val) {
        if (index_redirect != NULL) {
          index_redirect->add_index((int)i + index_offset);
        }
        break;
      }
    }
    if (index_redirect != NULL) {
      index_redirect->decr_pos();
    }
    return ret_val;
  }
};

template <typename T_type, unsigned int array_size, int index_offset>
boolean VALUE_ARRAY<T_type,array_size,index_offset>::operator==
  (const VALUE_ARRAY& other_value) const
{
  for (unsigned int elem_count = 0; elem_count < array_size; elem_count++)
    if (array_elements[elem_count] != other_value.array_elements[elem_count])
      return FALSE;
  return TRUE;
}

template <typename T_type, unsigned int array_size, int index_offset>
VALUE_ARRAY<T_type,array_size,index_offset>
VALUE_ARRAY<T_type,array_size,index_offset>::
operator<<=(int rotate_count) const {
  return *this >>= (-rotate_count);
}

template <typename T_type, unsigned int array_size, int index_offset>
VALUE_ARRAY<T_type,array_size,index_offset>
VALUE_ARRAY<T_type,array_size,index_offset>::
operator<<=(const INTEGER& rotate_count) const {
  rotate_count.must_bound("Unbound integer operand of rotate left "
                          "operator.");
  return *this >>= (int)(-rotate_count);
}

template <typename T_type, unsigned int array_size, int index_offset>
VALUE_ARRAY<T_type,array_size,index_offset>
VALUE_ARRAY<T_type,array_size,index_offset>::
operator>>=(int rotate_count) const {
  unsigned int rc;
  if (rotate_count>=0) rc = static_cast<unsigned int>(rotate_count) % array_size;
  else rc = array_size - (static_cast<unsigned int>(-rotate_count) % array_size);
  if (rc == 0) return *this;
  VALUE_ARRAY<T_type,array_size,index_offset> ret_val;
  for (unsigned int i=0; i<array_size; i++) {
      ret_val.array_elements[(i+rc)%array_size] = array_elements[i];
  }
  return ret_val;
}

template <typename T_type, unsigned int array_size, int index_offset>
VALUE_ARRAY<T_type,array_size,index_offset>
VALUE_ARRAY<T_type,array_size,index_offset>::
operator>>=(const INTEGER& rotate_count) const {
  rotate_count.must_bound("Unbound integer operand of rotate right "
                          "operator.");
  return *this >>= (int)rotate_count;
}

template <typename T_type, unsigned int array_size, int index_offset>
void VALUE_ARRAY<T_type,array_size,index_offset>::set_implicit_omit()
{
  for (unsigned int i = 0; i < array_size; ++i) {
    if (array_elements[i].is_bound())
      array_elements[i].set_implicit_omit();
  }
}

template <typename T_type, unsigned int array_size, int index_offset>
boolean VALUE_ARRAY<T_type,array_size,index_offset>::is_bound() const
{
  for (unsigned int i = 0; i < array_size; ++i) {
    if (!array_elements[i].is_bound()) {
    	return FALSE;
    }
  }
  return TRUE;
}

template <typename T_type, unsigned int array_size, int index_offset>
boolean VALUE_ARRAY<T_type,array_size,index_offset>::is_value() const
{
  for (unsigned int i = 0; i < array_size; ++i) {
    if (!array_elements[i].is_value()) {
    	return FALSE;
    }
  }
  return TRUE;
}

template <typename T_type, unsigned int array_size, int index_offset>
void VALUE_ARRAY<T_type,array_size,index_offset>::clean_up()
{
  for (unsigned int i = 0; i < array_size; ++i) {
    array_elements[i].clean_up();
  }
}

template <typename T_type, unsigned int array_size, int index_offset>
void VALUE_ARRAY<T_type,array_size,index_offset>::log() const
{
  TTCN_Logger::log_event_str("{ ");
  for (unsigned int elem_count = 0; elem_count < array_size; elem_count++)
  {
    if (elem_count > 0) TTCN_Logger::log_event_str(", ");
    array_elements[elem_count].log();
  }
  TTCN_Logger::log_event_str(" }");
}

template <typename T_type, unsigned int array_size, int index_offset>
int VALUE_ARRAY<T_type,array_size,index_offset>::lengthof() const
{

  for (unsigned int my_length=array_size; my_length>0; my_length--)
  {
    if (array_elements[my_length-1].is_bound()) return my_length;
  }
  return 0;
}

template <typename T_type, unsigned int array_size, int index_offset>
void VALUE_ARRAY<T_type,array_size,index_offset>::set_param(
  Module_Param& param)
{
#ifdef TITAN_RUNTIME_2
  if (dynamic_cast<Module_Param_Name*>(param.get_id()) != NULL &&
      param.get_id()->next_name()) {
    // Haven't reached the end of the module parameter name
    // => the name refers to one of the elements, not to the whole array
    char* param_field = param.get_id()->get_current_name();
    if (param_field[0] < '0' || param_field[0] > '9') {
      param.error("Unexpected record field name in module parameter, expected a valid"
        " array index");
    }
    unsigned int param_index = -1;
    sscanf(param_field, "%u", &param_index);
    if (param_index >= array_size) {
      param.error("Invalid array index: %u. The array only has %u elements.", param_index, array_size);
    }
    array_elements[param_index].set_param(param);
    return;
  }
#endif
  
  param.basic_check(Module_Param::BC_VALUE, "array value");
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  switch (mp->get_type()) {
  case Module_Param::MP_Value_List:
    if (mp->get_size()!=array_size) {
      param.error("The array value has incorrect number of elements: %lu was expected instead of %lu.", (unsigned long)mp->get_size(), (unsigned long)array_size);
    }
    for (size_t i=0; i<mp->get_size(); ++i) {
      Module_Param* const curr = mp->get_elem(i);
      if (curr->get_type()!=Module_Param::MP_NotUsed) {
        array_elements[i].set_param(*curr);
      }
    }
    break;
  case Module_Param::MP_Indexed_List:
    for (size_t i=0; i<mp->get_size(); ++i) {
      Module_Param* const curr = mp->get_elem(i);
      array_elements[curr->get_id()->get_index()].set_param(*curr);
    }
    break;
  default:
    param.type_error("array value");
  }
}

#ifdef TITAN_RUNTIME_2
template <typename T_type, unsigned int array_size, int index_offset>
Module_Param* VALUE_ARRAY<T_type,array_size,index_offset>::get_param
  (Module_Param_Name& param_name) const
{
  if (!is_bound()) {
    return new Module_Param_Unbound();
  }
  if (param_name.next_name()) {
    // Haven't reached the end of the module parameter name
    // => the name refers to one of the elements, not to the whole array
    char* param_field = param_name.get_current_name();
    if (param_field[0] < '0' || param_field[0] > '9') {
      TTCN_error("Unexpected record field name in module parameter reference, "
        "expected a valid array index");
    }
    unsigned int param_index = -1;
    sscanf(param_field, "%u", &param_index);
    if (param_index >= array_size) {
      TTCN_error("Invalid array index: %u. The array only has %u elements.", param_index, array_size);
    }
    return array_elements[param_index].get_param(param_name);
  }
  Vector<Module_Param*> values;
  for (unsigned int i = 0; i < array_size; ++i) {
    values.push_back(array_elements[i].get_param(param_name));
  }
  Module_Param_Value_List* mp = new Module_Param_Value_List();
  mp->add_list_with_implicit_ids(&values);
  values.clear();
  return mp;
}
#endif

template <typename T_type, unsigned int array_size, int index_offset>
void VALUE_ARRAY<T_type,array_size,index_offset>::encode_text
  (Text_Buf& text_buf) const
{
  for (unsigned int elem_count = 0; elem_count < array_size; elem_count++)
    array_elements[elem_count].encode_text(text_buf);
}

template <typename T_type, unsigned int array_size, int index_offset>
void VALUE_ARRAY<T_type,array_size,index_offset>::decode_text
  (Text_Buf& text_buf)
{
  for (unsigned int elem_count = 0; elem_count < array_size; elem_count++)
    array_elements[elem_count].decode_text(text_buf);
}

template <typename T_type, unsigned int array_size, int index_offset>
void VALUE_ARRAY<T_type,array_size,index_offset>::encode(
  const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf, TTCN_EncDec::coding_t p_coding, ...) const
{
  va_list pvar;
  va_start(pvar, p_coding);
  switch(p_coding) {
  case TTCN_EncDec::CT_JSON: {
    TTCN_EncDec_ErrorContext ec("While JSON-encoding type '%s': ", p_td.name);
    if(!p_td.json) TTCN_EncDec_ErrorContext::error_internal
                     ("No JSON descriptor available for type '%s'.", p_td.name);
    JSON_Tokenizer tok(va_arg(pvar, int) != 0);
    JSON_encode(p_td, tok);
    p_buf.put_s(tok.get_buffer_length(), (const unsigned char*)tok.get_buffer());
    break;}
  default:
    TTCN_error("Unknown coding method requested to encode type '%s'", p_td.name);
  }
  va_end(pvar);
}

template <typename T_type, unsigned int array_size, int index_offset>
void VALUE_ARRAY<T_type,array_size,index_offset>::decode(
  const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& p_buf, TTCN_EncDec::coding_t p_coding, ...)
{
  switch(p_coding) {
  case TTCN_EncDec::CT_JSON: {
    TTCN_EncDec_ErrorContext ec("While JSON-decoding type '%s': ", p_td.name);
    if(!p_td.json) TTCN_EncDec_ErrorContext::error_internal
                     ("No JSON descriptor available for type '%s'.", p_td.name);
    JSON_Tokenizer tok((const char*)p_buf.get_data(), p_buf.get_len());
    if(JSON_decode(p_td, tok, FALSE)<0)
      ec.error(TTCN_EncDec::ET_INCOMPL_MSG,"Can not decode type '%s', "
        "because invalid or incomplete message was received", p_td.name);
    p_buf.set_pos(tok.get_buf_pos());
    break;}
  default:
    TTCN_error("Unknown coding method requested to decode type '%s'", p_td.name);
  }
}

template <typename T_type, unsigned int array_size, int index_offset>
int VALUE_ARRAY<T_type,array_size,index_offset>::JSON_encode(
  const TTCN_Typedescriptor_t& p_td, JSON_Tokenizer& p_tok) const
{
  if (!is_bound() && (NULL == p_td.json || !p_td.json->metainfo_unbound)) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound array value.");
    return -1;
  }
  
  int enc_len = p_tok.put_next_token(JSON_TOKEN_ARRAY_START, NULL);
  
  for (unsigned int i = 0; i < array_size; ++i) {
    if (NULL != p_td.json && p_td.json->metainfo_unbound && !array_elements[i].is_bound()) {
      // unbound elements are encoded as { "metainfo []" : "unbound" }
      enc_len += p_tok.put_next_token(JSON_TOKEN_OBJECT_START, NULL);
      enc_len += p_tok.put_next_token(JSON_TOKEN_NAME, "metainfo []");
      enc_len += p_tok.put_next_token(JSON_TOKEN_STRING, "\"unbound\"");
      enc_len += p_tok.put_next_token(JSON_TOKEN_OBJECT_END, NULL);
    }
    else {
      int ret_val = array_elements[i].JSON_encode(*get_elem_descr(), p_tok);
      if (0 > ret_val) break;
      enc_len += ret_val;
    }
  }
  
  enc_len += p_tok.put_next_token(JSON_TOKEN_ARRAY_END, NULL);
  return enc_len;
}

template <typename T_type, unsigned int array_size, int index_offset>
int VALUE_ARRAY<T_type,array_size,index_offset>::JSON_decode(
  const TTCN_Typedescriptor_t& p_td, JSON_Tokenizer& p_tok, boolean p_silent)
{
  json_token_t token = JSON_TOKEN_NONE;
  size_t dec_len = p_tok.get_next_token(&token, NULL, NULL);
  if (JSON_TOKEN_ERROR == token) {
    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_BAD_TOKEN_ERROR, "");
    return JSON_ERROR_FATAL;
  }
  else if (JSON_TOKEN_ARRAY_START != token) {
    return JSON_ERROR_INVALID_TOKEN;
  } 
  
  for (unsigned int i = 0; i < array_size; ++i) {
    size_t buf_pos = p_tok.get_buf_pos();
    size_t ret_val;
    if (NULL != p_td.json && p_td.json->metainfo_unbound) {
      // check for metainfo object
      ret_val = p_tok.get_next_token(&token, NULL, NULL);
      if (JSON_TOKEN_OBJECT_START == token) {
        char* value = NULL;
        size_t value_len = 0;
        ret_val += p_tok.get_next_token(&token, &value, &value_len);
        if (JSON_TOKEN_NAME == token && 11 == value_len &&
            0 == strncmp(value, "metainfo []", 11)) {
          ret_val += p_tok.get_next_token(&token, &value, &value_len);
          if (JSON_TOKEN_STRING == token && 9 == value_len &&
              0 == strncmp(value, "\"unbound\"", 9)) {
            ret_val = p_tok.get_next_token(&token, NULL, NULL);
            if (JSON_TOKEN_OBJECT_END == token) {
              dec_len += ret_val;
              continue;
            }
          }
        }
      }
      // metainfo object not found, jump back and let the element type decode it
      p_tok.set_buf_pos(buf_pos);
    }
    int ret_val2 = array_elements[i].JSON_decode(*get_elem_descr(), p_tok, p_silent);
    if (JSON_ERROR_INVALID_TOKEN == ret_val2) {
      JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_ARRAY_ELEM_TOKEN_ERROR,
        array_size - i, (array_size - i > 1) ? "s" : "");
      return JSON_ERROR_FATAL;
    } 
    else if (JSON_ERROR_FATAL == ret_val2) {
      if (p_silent) {
        clean_up();
      }
      return JSON_ERROR_FATAL;
    }
    dec_len += (size_t)ret_val2;
  }
  
  dec_len += p_tok.get_next_token(&token, NULL, NULL);
  if (JSON_TOKEN_ARRAY_END != token) {
    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_ARRAY_END_TOKEN_ERROR, "");
    if (p_silent) {
      clean_up();
    }
    return JSON_ERROR_FATAL;
  }

  return dec_len;
}


template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
class TEMPLATE_ARRAY : public Restricted_Length_Template
{
private:
  union {
    struct {
      int n_elements;
      T_template_type **value_elements;
    } single_value;
    struct {
      unsigned int n_values;
      TEMPLATE_ARRAY *list_value;
    } value_list;
  };

  struct Pair_of_elements;
  Pair_of_elements *permutation_intervals;
  unsigned int number_of_permutations;

  void clean_up_intervals();
  void copy_value(const VALUE_ARRAY<T_value_type, array_size, index_offset>&
                  other_value);
  void copy_template(const TEMPLATE_ARRAY& other_value);
  void set_selection(template_sel new_selection);
  void set_selection(const TEMPLATE_ARRAY& other_value);
  void encode_text_permutation(Text_Buf& text_buf) const;
  void decode_text_permutation(Text_Buf& text_buf);

public:
  TEMPLATE_ARRAY() 
    {
      number_of_permutations = 0;
      permutation_intervals = NULL;
    }
  TEMPLATE_ARRAY(template_sel other_value)
    : Restricted_Length_Template(other_value)
    { 
      check_single_selection(other_value);
      number_of_permutations = 0;
      permutation_intervals = NULL;
    }
  TEMPLATE_ARRAY(null_type other_value);
  TEMPLATE_ARRAY(const VALUE_ARRAY<T_value_type, array_size, index_offset>&
                 other_value)
    {
      number_of_permutations = 0;
      permutation_intervals = NULL;
      copy_value(other_value); 
    }
  TEMPLATE_ARRAY(const
    OPTIONAL< VALUE_ARRAY<T_value_type,array_size,index_offset> >& other_value);
  TEMPLATE_ARRAY(const TEMPLATE_ARRAY& other_value)
    : Restricted_Length_Template()
    {
      number_of_permutations = 0;
      permutation_intervals = NULL;
      copy_template(other_value);
    }

  ~TEMPLATE_ARRAY() 
    {
      clean_up_intervals();
      clean_up();
    }
  void clean_up();

  TEMPLATE_ARRAY& operator=(template_sel other_value);
  TEMPLATE_ARRAY& operator=(null_type other_value);
  TEMPLATE_ARRAY& operator=(const
    VALUE_ARRAY<T_value_type, array_size, index_offset>& other_value);
  TEMPLATE_ARRAY& operator=(const
    OPTIONAL< VALUE_ARRAY<T_value_type,array_size,index_offset> >& other_value);
  TEMPLATE_ARRAY& operator=(const TEMPLATE_ARRAY& other_value);

  T_template_type& operator[](int index_value);
  T_template_type& operator[](const INTEGER& index_value);
  const T_template_type& operator[](int index_value) const;
  const T_template_type& operator[](const INTEGER& index_value) const;

  void set_size(int new_size);

  int n_elem() const;
  int size_of(boolean is_size) const;
  inline int size_of() const { return size_of(TRUE); }
  inline int lengthof() const { return size_of(FALSE); }

  void add_permutation(unsigned int start_index, unsigned int end_index);

  /** Removes all permutations set on this template, used when template variables
    *  are given new values. */
  void remove_all_permutations() { clean_up_intervals(); }

  unsigned int get_number_of_permutations() const;
  unsigned int get_permutation_start(unsigned int index_value) const;
  unsigned int get_permutation_end(unsigned int index_value) const;
  unsigned int get_permutation_size(unsigned int index_value) const;
  boolean permutation_starts_at(unsigned int index_value) const;
  boolean permutation_ends_at(unsigned int index_value) const;

private:
  static boolean match_function_specific(
    const Base_Type *value_ptr, int value_index,
    const Restricted_Length_Template *template_ptr, int template_index,
    boolean legacy);
public:
  boolean match(const VALUE_ARRAY<T_value_type, array_size, index_offset>&
                other_value, boolean legacy = FALSE) const;

  boolean is_value() const;
  VALUE_ARRAY<T_value_type, array_size, index_offset> valueof() const;

  void set_type(template_sel template_type, unsigned int list_length);
  TEMPLATE_ARRAY& list_item(unsigned int list_index);

  void log() const;
  void log_match(const VALUE_ARRAY<T_value_type, array_size, index_offset>&
                 match_value, boolean legacy = FALSE) const;

  void set_param(Module_Param& param);

  void encode_text(Text_Buf& text_buf) const;
  void decode_text(Text_Buf& text_buf);

  boolean is_present(boolean legacy = FALSE) const;
  boolean match_omit(boolean legacy = FALSE) const;

#ifdef TITAN_RUNTIME_2
  Module_Param* get_param(Module_Param_Name& param_name) const;
  void valueofv(Base_Type* value) const { *(static_cast<VALUE_ARRAY<T_value_type, array_size, index_offset>*>(value)) = valueof(); }
  void set_value(template_sel other_value) { *this = other_value; }
  void copy_value(const Base_Type* other_value) { *this = *(static_cast<const VALUE_ARRAY<T_value_type, array_size, index_offset>*>(other_value)); }
  Base_Template* clone() const { return new TEMPLATE_ARRAY(*this); }
  const TTCN_Typedescriptor_t* get_descriptor() const { TTCN_error("Internal error: TEMPLATE_ARRAY<>::get_descriptor() called."); }
  boolean matchv(const Base_Type* other_value, boolean legacy) const { return match(*(static_cast<const VALUE_ARRAY<T_value_type, array_size, index_offset>*>(other_value)), legacy); }
  void log_matchv(const Base_Type* match_value, boolean legacy) const  { log_match(*(static_cast<const VALUE_ARRAY<T_value_type, array_size, index_offset>*>(match_value)), legacy); }
#else
  void check_restriction(template_res t_res, const char* t_name=NULL, boolean legacy = FALSE) const;
#endif
};

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
struct TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
 Pair_of_elements{
  unsigned int start_index, end_index; //beginning and ending index
};

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
void TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
clean_up()
{
  switch (template_selection)
  {
  case SPECIFIC_VALUE:
    for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++)
      delete single_value.value_elements[elem_count];
    free_pointers((void**)single_value.value_elements);
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    delete [] value_list.list_value;
    break;
  default:
    break;
  }
  template_selection = UNINITIALIZED_TEMPLATE;
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
void TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
clean_up_intervals()
{
  number_of_permutations = 0;
  Free(permutation_intervals);
  permutation_intervals = NULL;
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
void TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
set_selection(template_sel other_value)
{
  Restricted_Length_Template::set_selection(other_value);
  clean_up_intervals();
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
void TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
set_selection(const TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>& other_value)
{
  Restricted_Length_Template::set_selection(other_value);
  clean_up_intervals();
  if(other_value.template_selection == SPECIFIC_VALUE)
  {
    number_of_permutations = other_value.number_of_permutations;
    permutation_intervals = (Pair_of_elements*) Malloc(number_of_permutations * sizeof(Pair_of_elements));
    memcpy(permutation_intervals,other_value.permutation_intervals,number_of_permutations*sizeof(Pair_of_elements));
  }
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
void TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
encode_text_permutation(Text_Buf& text_buf) const
{
  encode_text_restricted(text_buf);
  text_buf.push_int(number_of_permutations);

  for(unsigned int i = 0; i < number_of_permutations; i++)
  {
    text_buf.push_int(permutation_intervals[i].start_index);
    text_buf.push_int(permutation_intervals[i].end_index);
  }
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
void TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
decode_text_permutation(Text_Buf& text_buf)
{
  decode_text_restricted(text_buf);

  number_of_permutations = text_buf.pull_int().get_val();
  permutation_intervals = (Pair_of_elements *)Malloc
    (number_of_permutations * sizeof(Pair_of_elements));

  for (unsigned int i = 0; i < number_of_permutations; i++)
  {
    permutation_intervals[i].start_index =
      text_buf.pull_int().get_val();
    permutation_intervals[i].end_index =
      text_buf.pull_int().get_val();
  }
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
void TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
add_permutation(unsigned int start_index, unsigned int end_index)
{
  if(start_index > end_index)
    TTCN_error("wrong permutation interval settings start (%d)"
      "can not be greater than end (%d)",start_index, end_index);

  if(number_of_permutations > 0 &&
    permutation_intervals[number_of_permutations - 1].end_index >= start_index)
    TTCN_error("the %dth permutation overlaps the previous one", number_of_permutations);

  permutation_intervals = (Pair_of_elements*)Realloc(permutation_intervals, (number_of_permutations + 1) * sizeof(Pair_of_elements));
  permutation_intervals[number_of_permutations].start_index = start_index;
  permutation_intervals[number_of_permutations].end_index = end_index;
  number_of_permutations++;
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
unsigned int TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
get_number_of_permutations(void) const
{
  return number_of_permutations;
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
unsigned int TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
get_permutation_start(unsigned int index_value) const
{
  if(index_value >= number_of_permutations)
    TTCN_error("Index overflow (%d)", index_value);

  return permutation_intervals[index_value].start_index;
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
unsigned int TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
get_permutation_end(unsigned int index_value) const
{
  if(index_value >= number_of_permutations)
    TTCN_error("Index overflow (%d)", index_value);

  return permutation_intervals[index_value].end_index;
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
unsigned int TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
get_permutation_size(unsigned int index_value) const
{
  if(index_value >= number_of_permutations)
    TTCN_error("Index overflow (%d)", index_value);

  return permutation_intervals[index_value].end_index - permutation_intervals[index_value].start_index + 1;
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
boolean TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
permutation_starts_at(unsigned int index_value) const
{
  for(unsigned int i = 0; i < number_of_permutations; i++)
  {
    if(permutation_intervals[i].start_index == index_value)
      return TRUE;
  }

  return FALSE;
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
boolean TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
permutation_ends_at(unsigned int index_value) const
{
  for(unsigned int i = 0; i < number_of_permutations; i++)
  {
    if(permutation_intervals[i].end_index == index_value)
      return TRUE;
  }

  return FALSE;
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
void TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
copy_value(const VALUE_ARRAY<T_value_type, array_size, index_offset>&
             other_value)
{
  single_value.n_elements = array_size;
  single_value.value_elements =
    (T_template_type**)allocate_pointers(array_size);
  for (unsigned int elem_count = 0; elem_count < array_size; elem_count++)
    single_value.value_elements[elem_count] =
      new T_template_type(other_value.array_element(elem_count));
  set_selection(SPECIFIC_VALUE);
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
void TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
copy_template(const TEMPLATE_ARRAY& other_value)
{
  switch (other_value.template_selection)
  {
  case SPECIFIC_VALUE:
    single_value.n_elements = other_value.single_value.n_elements;
    single_value.value_elements =
      (T_template_type**)allocate_pointers(single_value.n_elements);
    for (int elem_count = 0; elem_count < single_value.n_elements; elem_count++)
      single_value.value_elements[elem_count] = new
        T_template_type(*other_value.single_value.value_elements[elem_count]);
    break;
  case OMIT_VALUE:
  case ANY_VALUE:
  case ANY_OR_OMIT:
    break;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = other_value.value_list.n_values;
    value_list.list_value = new TEMPLATE_ARRAY[value_list.n_values];
    for (unsigned int list_count = 0; list_count < value_list.n_values;
         list_count++)
      value_list.list_value[list_count].copy_template(
        other_value.value_list.list_value[list_count]);
    break;
  default:
    TTCN_error("Copying an uninitialized/unsupported array template.");
  }
  set_selection(other_value);
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
TEMPLATE_ARRAY(null_type)
  : Restricted_Length_Template(SPECIFIC_VALUE)
{
  single_value.n_elements = 0;
  single_value.value_elements = NULL;
  number_of_permutations = 0;
  permutation_intervals = NULL;
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
TEMPLATE_ARRAY(const
  OPTIONAL< VALUE_ARRAY<T_value_type,array_size,index_offset> >& other_value)
{
  number_of_permutations = 0;
  permutation_intervals = NULL;
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    copy_value((const VALUE_ARRAY<T_value_type, array_size, index_offset>&)
      other_value);
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  default:
    TTCN_error("Creating an array template from an unbound optional field.");
  }
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>&
TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
operator=(template_sel other_value)
{
  check_single_selection(other_value);
  clean_up();
  set_selection(other_value);
  return *this;
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>&
TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
operator=(null_type)
{
  clean_up();
  set_selection(SPECIFIC_VALUE);
  single_value.n_elements = 0;
  single_value.value_elements = NULL;
  return *this;
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>&
TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::operator=
  (const VALUE_ARRAY<T_value_type, array_size, index_offset>& other_value)
{
  clean_up();
  copy_value(other_value);
  return *this;
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>&
TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
operator=(const
  OPTIONAL< VALUE_ARRAY<T_value_type,array_size,index_offset> >& other_value)
{
  clean_up();
  switch (other_value.get_selection()) {
  case OPTIONAL_PRESENT:
    copy_value((const VALUE_ARRAY<T_value_type, array_size, index_offset>&)
      other_value);
    break;
  case OPTIONAL_OMIT:
    set_selection(OMIT_VALUE);
    break;
  default:
    TTCN_error("Assignment of an unbound optional field to an array template.");
  }
  return *this;
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>&
TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
operator=(const
  TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>&
  other_value)
{
  if (&other_value != this)
  {
    clean_up();
    copy_template(other_value);
  }
  return *this;
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
T_template_type&
TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::operator[]
  (int index_value)
{
  if (index_value < index_offset || index_value >= index_offset+(int)array_size)
    TTCN_error(
      "Accessing an element of an array template using invalid index: %d. "
      "Index range is [%d,%d].",
      index_value, index_offset, index_offset+(int)array_size);
  // transform index value according to given offset
  index_value -= index_offset;
  // the template of an array is not restricted to array_size, allow any length
  // in case of * or ? expand to full size to avoid uninitialized values
  switch (template_selection)
  {
  case SPECIFIC_VALUE:
    if (index_value >= single_value.n_elements) set_size(index_value + 1);
    break;
  case ANY_VALUE:
  case ANY_OR_OMIT:
    set_size(array_size);
    break;
  default:
    set_size(index_value + 1);
  }
  return *single_value.value_elements[index_value];
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
T_template_type&
TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::operator[]
  (const INTEGER& index_value)
{
  index_value.must_bound(
    "Using an unbound integer value for indexing an array template.");
  return (*this)[(int)index_value];
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
const T_template_type&
TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::operator[]
  (int index_value) const
{
  if (index_value < index_offset)
    TTCN_error(
      "Accessing an element of an array template using invalid index: %d. "
      "Index range starts at %d.",
      index_value, index_offset);
  // transform index value according to given offset
  index_value -= index_offset;
  // const is specific template
  if (template_selection != SPECIFIC_VALUE)
    TTCN_error("Accessing an element of a non-specific array template.");
  if (index_value >= single_value.n_elements)
    TTCN_error("Index overflow in an array template: "
               "The index is %d (starting at %d),"
               " but the template has only %d elements.",
               index_value+index_offset, index_offset, single_value.n_elements);
  return *single_value.value_elements[index_value];
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
const T_template_type&
TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::operator[]
  (const INTEGER& index_value) const
{
  index_value.must_bound(
    "Using an unbound integer value for indexing an array template.");
  return (*this)[(int)index_value];
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
void TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
set_size(int new_size)
{
  if (new_size < 0) TTCN_error("Internal error: Setting a negative size "
                               "for an array template.");
  template_sel old_selection = template_selection;
  if (old_selection != SPECIFIC_VALUE)
  {
    clean_up();
    set_selection(SPECIFIC_VALUE);
    single_value.n_elements = 0;
    single_value.value_elements = NULL;
  }
  if (new_size > single_value.n_elements)
  {
    single_value.value_elements =
      (T_template_type**)reallocate_pointers(
        (void**)single_value.value_elements, single_value.n_elements, new_size);
    if (old_selection == ANY_VALUE || old_selection == ANY_OR_OMIT)
    {
      for (int elem_count = single_value.n_elements;
           elem_count < new_size; elem_count++)
      single_value.value_elements[elem_count] = new T_template_type(ANY_VALUE);
    }
    else
    {
      for (int elem_count = single_value.n_elements;
           elem_count < new_size; elem_count++)
      single_value.value_elements[elem_count] = new T_template_type;
    }
    single_value.n_elements = new_size;
  }
  else if (new_size < single_value.n_elements)
  {
    for (int elem_count = new_size; elem_count < single_value.n_elements;
         elem_count++)
      delete single_value.value_elements[elem_count];
    single_value.value_elements =
      (T_template_type**)reallocate_pointers(
        (void**)single_value.value_elements, single_value.n_elements, new_size);
    single_value.n_elements = new_size;
  }
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
int TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
n_elem() const
{
  switch (template_selection) {
  case SPECIFIC_VALUE:
    return single_value.n_elements;
  case VALUE_LIST:
    return value_list.n_values;
  default:
    TTCN_error("Performing n_elem");
  }
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
int TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
size_of(boolean is_size) const
{
  const char* op_name = is_size ? "size" : "length";
  int min_size;
  boolean has_any_or_none;
  if (is_ifpresent)
    TTCN_error("Performing %sof() operation on an array template "
               "which has an ifpresent attribute.", op_name);
  switch (template_selection)
  {
  case SPECIFIC_VALUE: {
    min_size = 0;
    has_any_or_none = FALSE;
    int elem_count = single_value.n_elements;
    if (!is_size) { // lengthof()
      while (elem_count>0 &&
        !single_value.value_elements[elem_count-1]->is_bound()) elem_count--;
    }
    for (int i=0; i<elem_count; i++)
    {
      switch (single_value.value_elements[i]->get_selection())
      {
      case OMIT_VALUE:
        TTCN_error("Performing %sof() operation on an array template "
                   "containing omit element.", op_name);
      case ANY_OR_OMIT:
        has_any_or_none = TRUE;
        break;
      default:
        min_size++;
        break;
      }
    }
  } break;
  case OMIT_VALUE:
    TTCN_error("Performing %sof() operation on an array template "
               "containing omit value.", op_name);
  case ANY_VALUE:
  case ANY_OR_OMIT:
    min_size = 0;
    has_any_or_none = TRUE; // max. size is infinity
    break;
  case VALUE_LIST: {
    // error if any element does not have size or the sizes differ
    if (value_list.n_values<1)
      TTCN_error("Performing %sof() operation on an array template "
                 "containing an empty list.", op_name);
    int item_size = value_list.list_value[0].size_of(is_size);
    for (unsigned int i = 1; i < value_list.n_values; i++) {
      if (value_list.list_value[i].size_of(is_size)!=item_size)
        TTCN_error("Performing %sof() operation on an array template "
                   "containing a value list with different sizes.", op_name);
    }
    min_size = item_size;
    has_any_or_none = FALSE;
  } break;
  case COMPLEMENTED_LIST:
    TTCN_error("Performing %sof() operation on an array template "
               "containing complemented list.", op_name);
  default:
    TTCN_error("Performing %sof() operation on an "
               "uninitialized/unsupported array template.", op_name);
  }
  return check_section_is_single(min_size, has_any_or_none,
                                 op_name, "an", "array template");
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
boolean TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
match_function_specific(const Base_Type *value_ptr, int value_index,
                        const Restricted_Length_Template *template_ptr,
                        int template_index, boolean legacy)
{
  if (value_index >= 0)
    return ((const TEMPLATE_ARRAY*)template_ptr)->
      single_value.value_elements[template_index]->
        match(
          ((const VALUE_ARRAY<T_value_type,array_size,index_offset>*)value_ptr)
            ->array_element(value_index), legacy);
  else
    return ((const TEMPLATE_ARRAY*)template_ptr)->
      single_value.value_elements[template_index]->is_any_or_omit();
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
boolean TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
match(const VALUE_ARRAY<T_value_type, array_size, index_offset>&
      other_value, boolean legacy) const
{
  if (!match_length(array_size)) return FALSE;
  switch (template_selection)
  {
  case SPECIFIC_VALUE:
    return match_permutation_array(&other_value, array_size, this, single_value.n_elements,
                       match_function_specific, legacy);
  case OMIT_VALUE:
    return FALSE;
  case ANY_VALUE:
  case ANY_OR_OMIT:
    return TRUE;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    for (unsigned int list_count = 0; list_count < value_list.n_values;
         list_count++)
      if (value_list.list_value[list_count].match(other_value, legacy))
        return template_selection == VALUE_LIST;
    return template_selection == COMPLEMENTED_LIST;
  default:
    TTCN_error("Matching with an uninitialized/unsupported array template.");
  }
  return FALSE;
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
boolean TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
is_value() const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent) return FALSE;
  for (int i=0; i<single_value.n_elements; i++)
    if (!single_value.value_elements[i]->is_value()) return FALSE;
  return TRUE;
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
VALUE_ARRAY<T_value_type, array_size, index_offset>
TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
valueof() const
{
  if (template_selection != SPECIFIC_VALUE || is_ifpresent)
    TTCN_error("Performing a valueof or send operation on a "
               "non-specific array template.");
  // the size of the template must be the size of the value
  if (single_value.n_elements!=array_size)
    TTCN_error("Performing a valueof or send operation on a "
               "specific array template with invalid size.");
  VALUE_ARRAY<T_value_type, array_size, index_offset> ret_val;
  for (unsigned int elem_count = 0; elem_count < array_size; elem_count++)
    ret_val.array_element(elem_count) =
      single_value.value_elements[elem_count]->valueof();
  return ret_val;
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
void TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
set_type(template_sel template_type, unsigned int list_length)
{
  clean_up();
  switch (template_type) {
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    value_list.n_values = list_length;
    value_list.list_value = new TEMPLATE_ARRAY[list_length];
    break;
  default:
    TTCN_error(
      "Internal error: Setting an invalid type for an array template.");
  }
  set_selection(template_type);
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>&
TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
list_item(unsigned int list_index)
{
  if (template_selection != VALUE_LIST &&
      template_selection != COMPLEMENTED_LIST)
    TTCN_error("Internal error: Accessing a list element of a non-list "
               "array template.");
  if (list_index >= value_list.n_values)
    TTCN_error("Internal error: Index overflow in a value list "
               "array template.");
  return value_list.list_value[list_index];
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
void TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
log() const
{
  switch (template_selection)
  {
  case SPECIFIC_VALUE:
    if (single_value.n_elements > 0)
    {
      TTCN_Logger::log_event_str("{ ");
      for (int elem_count=0; elem_count < single_value.n_elements; elem_count++)
      {
        if (elem_count > 0) TTCN_Logger::log_event_str(", ");
        if (permutation_starts_at(elem_count)) TTCN_Logger::log_event_str("permutation(");
        single_value.value_elements[elem_count]->log();
        if (permutation_ends_at(elem_count)) TTCN_Logger::log_char(')');
      }
      TTCN_Logger::log_event_str(" }");
    }
    else
      TTCN_Logger::log_event_str("{ }");
    break;
  case COMPLEMENTED_LIST:
    TTCN_Logger::log_event_str("complement");
  case VALUE_LIST:
    TTCN_Logger::log_char('(');
    for (unsigned int list_count = 0; list_count < value_list.n_values;
         list_count++)
    {
      if (list_count > 0) TTCN_Logger::log_event_str(", ");
      value_list.list_value[list_count].log();
    }
    TTCN_Logger::log_char(')');
    break;
  default:
    log_generic();
    break;
  }
  log_restricted();
  log_ifpresent();
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
void TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
log_match(const VALUE_ARRAY<T_value_type, array_size, index_offset>&
          match_value, boolean legacy) const
{
  if(TTCN_Logger::VERBOSITY_COMPACT == TTCN_Logger::get_matching_verbosity()){
    if(match(match_value, legacy)){
      TTCN_Logger::print_logmatch_buffer();
      TTCN_Logger::log_event_str(" matched");
    }else{
      if (template_selection == SPECIFIC_VALUE &&
          single_value.n_elements == array_size) {
        size_t previous_size = TTCN_Logger::get_logmatch_buffer_len();
        for (unsigned int elem_count = 0; elem_count < array_size;
          elem_count++) {
          if(!single_value.value_elements[elem_count]->
              match(match_value.array_element(elem_count), legacy)){
            TTCN_Logger::log_logmatch_info("[%d]", elem_count);
            single_value.value_elements[elem_count]->
              log_match(match_value.array_element(elem_count), legacy);
            TTCN_Logger::set_logmatch_buffer_len(previous_size);
          }
        }
        log_match_length(array_size);
      } else {
        TTCN_Logger::print_logmatch_buffer();
        match_value.log();
        TTCN_Logger::log_event_str(" with ");
        log();
        TTCN_Logger::log_event_str(" unmatched");
      }
    }
    return;
  }
  if (template_selection == SPECIFIC_VALUE &&
      single_value.n_elements == array_size) {
    TTCN_Logger::log_event_str("{ ");
    for (unsigned int elem_count = 0; elem_count < array_size; elem_count++) {
      if (elem_count > 0) TTCN_Logger::log_event_str(", ");
      single_value.value_elements[elem_count]->log_match(
        match_value.array_element(elem_count), legacy);
    }
    TTCN_Logger::log_event_str(" }");
    log_match_length(array_size);
  } else {
    match_value.log();
    TTCN_Logger::log_event_str(" with ");
    log();
    if (match(match_value, legacy)) TTCN_Logger::log_event_str(" matched");
    else TTCN_Logger::log_event_str(" unmatched");
  }
}

template <typename T_value_type, typename T_template_type, unsigned int array_size, int index_offset>
void TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::set_param(Module_Param& param)
{
  if (dynamic_cast<Module_Param_Name*>(param.get_id()) != NULL &&
      param.get_id()->next_name()) {
    // Haven't reached the end of the module parameter name
    // => the name refers to one of the elements, not to the whole array
    char* param_field = param.get_id()->get_current_name();
    if (param_field[0] < '0' || param_field[0] > '9') {
      param.error("Unexpected record field name in module parameter, expected a valid"
        " array template index");
    }
    unsigned int param_index = -1;
    sscanf(param_field, "%u", &param_index);
    if (param_index >= array_size) {
      param.error("Invalid array index: %u. The array only has %u elements.", param_index, array_size);
    }
    (*this)[param_index].set_param(param);
    return;
  }
  
  param.basic_check(Module_Param::BC_TEMPLATE, "array template");
  
  Module_Param_Ptr mp = &param;
#ifdef TITAN_RUNTIME_2
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
#endif
  
  switch (mp->get_type()) {
  case Module_Param::MP_Omit:
    *this = OMIT_VALUE;
    break;
  case Module_Param::MP_Any:
    *this = ANY_VALUE;
    break;
  case Module_Param::MP_AnyOrNone:
    *this = ANY_OR_OMIT;
    break;
  case Module_Param::MP_List_Template:
  case Module_Param::MP_ComplementList_Template: {
    TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset> temp;
    temp.set_type(mp->get_type() == Module_Param::MP_List_Template ?
      VALUE_LIST : COMPLEMENTED_LIST, mp->get_size());
    for (size_t i=0; i<mp->get_size(); i++) {
      temp.list_item(i).set_param(*mp->get_elem(i));
    }
    *this = temp;
    break; }
  case Module_Param::MP_Value_List:
    set_size(mp->get_size());
    for (size_t i=0; i<mp->get_size(); ++i) {
      Module_Param* const curr = mp->get_elem(i);
      if (curr->get_type()!=Module_Param::MP_NotUsed) {
        (*this)[(int)i+index_offset].set_param(*curr);
      }
    }
    break;
  case Module_Param::MP_Indexed_List:
    for (size_t i=0; i<mp->get_size(); ++i) {
      Module_Param* const curr = mp->get_elem(i);
      (*this)[curr->get_id()->get_index()].set_param(*curr);
    }
    break;
  default:
    param.type_error("array template");
  }
  is_ifpresent = param.get_ifpresent() || mp->get_ifpresent();
}

#ifdef TITAN_RUNTIME_2
template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
Module_Param* TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
  get_param(Module_Param_Name& param_name) const
{
  if (param_name.next_name()) {
    // Haven't reached the end of the module parameter name
    // => the name refers to one of the elements, not to the whole record of
    char* param_field = param_name.get_current_name();
    if (param_field[0] < '0' || param_field[0] > '9') {
      TTCN_error("Unexpected record field name in module parameter reference, "
        "expected a valid array index");
    }
    unsigned int param_index = -1;
    sscanf(param_field, "%u", &param_index);
    if (param_index >= array_size) {
      TTCN_error("Invalid array index: %u. The array only has %u elements.", param_index, array_size);
    }
    return single_value.value_elements[param_index]->get_param(param_name);
  }
  Module_Param* mp = NULL;
  switch (template_selection) {
  case UNINITIALIZED_TEMPLATE:
    mp = new Module_Param_Unbound();
    break;
  case OMIT_VALUE:
    mp = new Module_Param_Omit();
    break;
  case ANY_VALUE:
    mp = new Module_Param_Any();
    break;
  case ANY_OR_OMIT:
    mp = new Module_Param_AnyOrNone();
    break;
  case SPECIFIC_VALUE: {
    Vector<Module_Param*> values;
    for (unsigned int i = 0; i < array_size; ++i) {
      values.push_back(single_value.value_elements[i]->get_param(param_name));
    }
    mp = new Module_Param_Value_List();
    mp->add_list_with_implicit_ids(&values);
    values.clear();
    break; }
  case VALUE_LIST:
  case COMPLEMENTED_LIST: {
    if (template_selection == VALUE_LIST) {
      mp = new Module_Param_List_Template();
    }
    else {
      mp = new Module_Param_ComplementList_Template();
    }
    for (size_t i = 0; i < value_list.n_values; ++i) {
      mp->add_elem(value_list.list_value[i].get_param(param_name));
    }
    break; }
  default:
    break;
  }
  if (is_ifpresent) {
    mp->set_ifpresent();
  }
  return mp;
}
#endif

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
void TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
encode_text(Text_Buf& text_buf) const
{
  encode_text_restricted(text_buf);
  switch (template_selection)
  {
    case SPECIFIC_VALUE:
      text_buf.push_int(single_value.n_elements);
      for (int elem_count=0; elem_count < single_value.n_elements; elem_count++)
        single_value.value_elements[elem_count]->encode_text(text_buf);
      break;
    case OMIT_VALUE:
    case ANY_VALUE:
    case ANY_OR_OMIT:
      break;
    case VALUE_LIST:
    case COMPLEMENTED_LIST:
      text_buf.push_int(value_list.n_values);
      for (unsigned int list_count = 0; list_count < value_list.n_values;
           list_count++)
        value_list.list_value[list_count].encode_text(text_buf);
      break;
    default:
      TTCN_error("Text encoder: Encoding an uninitialized/unsupported "
                 "array template.");
  }
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
void TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
decode_text(Text_Buf& text_buf)
{
  clean_up();
  decode_text_restricted(text_buf);
  switch (template_selection)
  {
    case SPECIFIC_VALUE:
      single_value.n_elements = text_buf.pull_int().get_val();
      if (single_value.n_elements < 0)
        TTCN_error("Text decoder: Negative size was received for an "
                   "array template.");
      single_value.value_elements =
        (T_template_type**)allocate_pointers(single_value.n_elements);
      for (int elem_count=0; elem_count < single_value.n_elements; elem_count++)
      {
        single_value.value_elements[elem_count] = new T_template_type;
        single_value.value_elements[elem_count]->decode_text(text_buf);
      }
      break;
    case OMIT_VALUE:
    case ANY_VALUE:
    case ANY_OR_OMIT:
      break;
    case VALUE_LIST:
    case COMPLEMENTED_LIST:
      value_list.n_values = text_buf.pull_int().get_val();
      value_list.list_value = new TEMPLATE_ARRAY[value_list.n_values];
      for (unsigned int list_count = 0; list_count < value_list.n_values;
           list_count++)
        value_list.list_value[list_count].decode_text(text_buf);
      break;
    default:
      TTCN_error("Text decoder: An unknown/unsupported selection was received "
                 "for an array template.");
  }
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
boolean TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
is_present(boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return FALSE;
  return !match_omit(legacy);
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
boolean TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
match_omit(boolean legacy /* = FALSE */) const
{
  if (is_ifpresent) return TRUE;
  switch (template_selection) {
  case OMIT_VALUE:
  case ANY_OR_OMIT:
    return TRUE;
  case VALUE_LIST:
  case COMPLEMENTED_LIST:
    if (legacy) {
      // legacy behavior: 'omit' can appear in the value/complement list
      for (unsigned int i=0; i<value_list.n_values; i++)
        if (value_list.list_value[i].match_omit())
          return template_selection==VALUE_LIST;
      return template_selection==COMPLEMENTED_LIST;
    }
    // else fall through
  default:
    return FALSE;
  }
  return FALSE;
}

#ifndef TITAN_RUNTIME_2
template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
void TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>::
check_restriction(template_res t_res, const char* t_name, boolean legacy /* = FALSE */) const
{
  if (template_selection==UNINITIALIZED_TEMPLATE) return;
  switch ((t_name&&(t_res==TR_VALUE))?TR_OMIT:t_res) {
  case TR_OMIT:
    if (template_selection==OMIT_VALUE) return;
  case TR_VALUE:
    if (template_selection!=SPECIFIC_VALUE || is_ifpresent) break;
    for (int i=0; i<single_value.n_elements; i++)
      single_value.value_elements[i]->check_restriction(t_res, t_name ? t_name : "array");
    return;
  case TR_PRESENT:
    if (!match_omit(legacy)) return;
    break;
  default:
    return;
  }
  TTCN_error("Restriction `%s' on template of type %s violated.",
             get_res_name(t_res), t_name ? t_name : "array");
}
#endif

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
answer recursive_permutation_match(const Base_Type *value_ptr,
  unsigned int value_start_index,
  unsigned int value_size,
  const TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>*template_ptr,
  unsigned int template_start_index,
  unsigned int template_size,
  unsigned int permutation_index,
  match_function_t match_function,
  unsigned int& shift_size,
  boolean legacy)
{
  unsigned int nof_permutations = template_ptr->get_number_of_permutations();
  if (permutation_index > nof_permutations)
    TTCN_error("Internal error: recursive_permutation_match: "
      "invalid argument.");

  if (permutation_index < nof_permutations &&
    template_ptr->get_permutation_end(permutation_index) >
  template_start_index + template_size)
    TTCN_error("Internal error: recursive_permutation_match: wrong "
      "permutation interval settings for permutation %d.",
      permutation_index);

  shift_size = 0;

  //trivial cases
  if(template_size == 0)
  {
    //reached the end of templates
    // if we reached the end of values => good
    // else => bad
    if(value_size == 0)
      return SUCCESS;
    else
      return FAILURE;
  }

  //are we at an asterisk or at the beginning of a permutation interval
  boolean is_asterisk;
  boolean permutation_begins = permutation_index < nof_permutations &&
    template_start_index ==
      template_ptr->get_permutation_start(permutation_index);

  if (permutation_begins ||
    match_function(value_ptr, -1, template_ptr, template_start_index, legacy))
  {
    unsigned int smallest_possible_size;
    unsigned int largest_possible_size;
    boolean has_asterisk;
    boolean already_superset;
    unsigned int permutation_size;

    //check how many values might be associated with this permutation
    //if we are at a permutation start
    if (permutation_begins)
    {
      is_asterisk = FALSE;
      permutation_size =
        template_ptr->get_permutation_size(permutation_index);
      smallest_possible_size = 0;
      has_asterisk = FALSE;

      //count how many non asterisk elements are in the permutation
      for(unsigned int i = 0; i < permutation_size; i++)
      {
        if(match_function(value_ptr, -1, template_ptr,
          i + template_start_index, legacy))
        {
          has_asterisk = TRUE;
        }else{
          smallest_possible_size++;
        }
      }

      //the real permutation size is bigger then the value size
      if(smallest_possible_size > value_size)
        return NO_CHANCE;

      //if the permutation has an asterisk then it can grow
      if(has_asterisk)
      {
        largest_possible_size = value_size;

        //if there are only asterisks in the permutation
        if(smallest_possible_size == 0)
          already_superset = TRUE;
        else
          already_superset = FALSE;
      }else{
        //without asterisks its size is fixed
        largest_possible_size = smallest_possible_size;
        already_superset = FALSE;
      }
    }else{
      //or at an asterisk
      is_asterisk = TRUE;
      already_superset = TRUE;
      permutation_size = 1;
      smallest_possible_size = 0;
      largest_possible_size = value_size;
      has_asterisk = TRUE;
    }

    unsigned int temp_size = smallest_possible_size;

    {
      //this is to make match_set_of incremental,
      // we store the already found pairs in this vector
      // so we wouldn't try to find a pair for those templates again
      // and we can set the covered state of values too
      // to not waste memory it is only created if needed
      int* pair_list = NULL;
      unsigned int old_temp_size = 0;

      if(!already_superset)
      {
        pair_list = new int[permutation_size];
        for(unsigned int i = 0 ; i < permutation_size; i++)
        {
          //in the beginning we haven't found a template to any values
          pair_list[i] = -1;
        }
      }

      while(!already_superset)
      {
        //must be a permutation having other values than asterisks

        int x = 0;

        //our set matching is extended with 2 more parameters
        // giving back how many templates
        // (other than asterisk) couldn't be matched
        // and setting / giving back the value-template pairs

        boolean found = match_set_of_internal(value_ptr, value_start_index,
          temp_size, template_ptr,
          template_start_index, permutation_size,
          match_function, SUPERSET, &x, pair_list, old_temp_size, legacy);

        if(found)
        {
          already_superset = TRUE;
        }else{
          //as we didn't found a match we have to try
          // a larger set of values
          //x is the number of templates we couldn't find
          // a matching pair for
          // the next must be at least this big to fully cover
          // on the other side if it would be bigger than it might miss
          // the smallest possible match.

          //if we can match with more values
          if(has_asterisk && temp_size + x <= largest_possible_size)
          {
            old_temp_size = temp_size;
            temp_size += x;
          }else{
            delete[] pair_list;
            return FAILURE; //else we failed
          }
        }
      }

      delete[] pair_list;
    }

    //we reach here only if we found a match

    //can only go on recursively if we haven't reached the end

    //reached the end of templates
    if(permutation_size == template_size)
    {
      if(has_asterisk || value_size == temp_size)
        return SUCCESS;
      else
        return FAILURE;
    }

    for(unsigned int i = temp_size; i <= largest_possible_size;)
    {
      answer result;

      if(is_asterisk)
      {
        //don't step the permutation index
        result = recursive_permutation_match(value_ptr,value_start_index+i,
          value_size - i, template_ptr,
          template_start_index +
          permutation_size,
          template_size -
          permutation_size,
          permutation_index,
          match_function, shift_size, legacy);
      }else{
        //try with the next permutation
        result = recursive_permutation_match(value_ptr,value_start_index+i,
          value_size - i, template_ptr,
          template_start_index +
          permutation_size,
          template_size - permutation_size,
          permutation_index + 1,
          match_function, shift_size, legacy);
      }

      if(result == SUCCESS)
        return SUCCESS;             //we finished
      else if(result == NO_CHANCE)
        return NO_CHANCE;           //matching is not possible
      else if(i == value_size)      //we failed
      {
        //if there is no chance of matching
        return NO_CHANCE;
      }else{
        i += shift_size > 1 ? shift_size : 1;

        if(i > largest_possible_size)
          shift_size = i - largest_possible_size;
        else
          shift_size = 0;
      }
    }

    //this level failed;
    return FAILURE;
  }else{
    //we are at the beginning of a non permutation, non asterisk interval

    //the distance to the next permutation or the end of templates
    // so the longest possible match
    unsigned int distance;

    if (permutation_index < nof_permutations)
    {
      distance = template_ptr->get_permutation_start(permutation_index)
                          - template_start_index;
    }else{
      distance = template_size;
    }

    //if there are no more values, but we still have templates
    // and the template is not an asterisk or a permutation start
    if(value_size == 0)
      return FAILURE;

    //we try to match as many values as possible
    //an asterisk is handled like a 0 length permutation
    boolean good;
    unsigned int i = 0;
    do{
      good = match_function(value_ptr, value_start_index + i,
        template_ptr, template_start_index + i, legacy);
      i++;
      //bad stop: something can't be matched
      //half bad half good stop: the end of values is reached
      //good stop: matching on the full distance or till an asterisk
    }while(good && i < value_size && i < distance &&
      !match_function(value_ptr, -1, template_ptr,
        template_start_index + i, legacy));

    //if we matched on the full distance or till an asterisk
    if(good && (i == distance ||
      match_function(value_ptr, -1, template_ptr,
        template_start_index + i, legacy)))
    {
      //reached the end of the templates
      if(i == template_size)
      {
        if(i < value_size)
        {
          //the next level would return FAILURE so we don't step it
          return FAILURE;
        }else{
          //i == value_size, so we matched everything
          return SUCCESS;
        }
      }else{
        //we reached the next asterisk or permutation,
        // so step to the next level
        return recursive_permutation_match(value_ptr,value_start_index + i,
          value_size - i,
          template_ptr,
          template_start_index + i,
          template_size - i,
          permutation_index,
          match_function, shift_size, legacy);
      }
    }else{
      //something bad happened, so we have to check how bad the situation is
      if( i == value_size)
      {
        //the aren't values left, meaning that the match is not possible
        return NO_CHANCE;
      }else{
        //we couldn't match, but there is still a chance of matching

        //try to find a matching value for the last checked (and failed)
        // template.
        // smaller jumps would fail so we skip them
        shift_size = 0;
        i--;
        do{
          good = match_function(value_ptr,
            value_start_index + i + shift_size,
            template_ptr, template_start_index + i, legacy);
          shift_size++;
        }while(!good && i + shift_size < value_size);

        if(good)
        {
          shift_size--;
          return FAILURE;
        }else{
          // the template can not be matched later
          return NO_CHANCE;
        }
      }
    }
  }
}

template <typename T_value_type, typename T_template_type,
          unsigned int array_size, int index_offset>
boolean match_permutation_array(const Base_Type *value_ptr,
  int value_size,
  const TEMPLATE_ARRAY<T_value_type,T_template_type,array_size,index_offset>* template_ptr,
  int template_size,
  match_function_t match_function,
  boolean legacy)
{
  if (value_ptr == NULL || value_size < 0 ||
    template_ptr == NULL || template_size < 0 ||
    template_ptr->get_selection() != SPECIFIC_VALUE)
    TTCN_error("Internal error: match_permutation_arry: invalid argument.");

  unsigned int nof_permutations = template_ptr->get_number_of_permutations();
  // use the simplified algorithm if the template does not contain permutation
  if (nof_permutations == 0)
    return match_array(value_ptr, value_size,
      template_ptr, template_size, match_function, legacy);
  // use 'set of' matching if all template elements are grouped into one
  // permutation
  if (nof_permutations == 1 && template_ptr->get_permutation_start(0) == 0 &&
    template_ptr->get_permutation_end(0) ==
      static_cast<unsigned int>(template_size - 1))
    return match_set_of(value_ptr, value_size, template_ptr, template_size,
      match_function, legacy);

  unsigned int shift_size = 0;
  return recursive_permutation_match(value_ptr, 0, value_size, template_ptr,
    0, template_size, 0, match_function, shift_size, legacy) == SUCCESS;
}

#endif
