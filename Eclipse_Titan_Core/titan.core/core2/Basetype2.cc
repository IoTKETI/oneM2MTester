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
 *   Szabo, Bence Janos
 *   Pandi, Krisztian
 *
 ******************************************************************************/
#include "Basetype.hh"
#include "TEXT.hh"
#include "BER.hh"
#include "XER.hh"
#include "RAW.hh"
#include "memory.h"
#include "Module_list.hh"
#include "Vector.hh"
#include "JSON.hh"

#ifdef TITAN_RUNTIME_2

#include "Integer.hh"
#include "Charstring.hh"
#include "Universal_charstring.hh"
#include "Addfunc.hh"
#include "PreGenRecordOf.hh"

////////////////////////////////////////////////////////////////////////////////

const Erroneous_values_t* Erroneous_descriptor_t::get_field_err_values(int field_idx) const
{
  for (int i=0; i<values_size; i++) {
    if (values_vec[i].field_index==field_idx) return (values_vec+i);
    if (values_vec[i].field_index>field_idx) return NULL;
  }
  return NULL;
}

const Erroneous_values_t* Erroneous_descriptor_t::next_field_err_values(
  const int field_idx, int& values_idx) const
{
  const Erroneous_values_t* err_vals = NULL;
  if ( (values_idx<values_size) && (values_vec[values_idx].field_index==field_idx) ) {
    err_vals = values_vec+values_idx;
    values_idx++;
  }
  return err_vals;
}

const Erroneous_descriptor_t* Erroneous_descriptor_t::get_field_emb_descr(int field_idx) const
{
  for (int i=0; i<embedded_size; i++) {
    if (embedded_vec[i].field_index==field_idx) return (embedded_vec+i);
    if (embedded_vec[i].field_index>field_idx) return NULL;
  }
  return NULL;
}

const Erroneous_descriptor_t* Erroneous_descriptor_t::next_field_emb_descr(
  const int field_idx, int& edescr_idx) const
{
  const Erroneous_descriptor_t* emb_descr = NULL;
  if ( (edescr_idx<embedded_size) && (embedded_vec[edescr_idx].field_index==field_idx) ) {
    emb_descr = embedded_vec+edescr_idx;
    edescr_idx++;
  }
  return emb_descr;
}

void Erroneous_descriptor_t::log() const
{
  TTCN_Logger::log_event_str(" with erroneous { ");
  log_();
  TTCN_Logger::log_event_str("}");
}

void Erroneous_descriptor_t::log_() const
{
  if (omit_before!=-1) {
    if (omit_before_qualifier==NULL) TTCN_error(
      "internal error: Erroneous_descriptor_t::log()");
    TTCN_Logger::log_event("{ before %s := omit all } ", omit_before_qualifier);
  }
  if (omit_after!=-1) {
    if (omit_after_qualifier==NULL) TTCN_error(
      "internal error: Erroneous_descriptor_t::log()");
    TTCN_Logger::log_event("{ after %s := omit all } ", omit_after_qualifier);
  }
  for (int i=0; i<values_size; i++) {
    if (values_vec[i].field_qualifier==NULL) TTCN_error(
      "internal error: Erroneous_descriptor_t::log()");
    if (values_vec[i].before) {
      TTCN_Logger::log_event("{ before%s %s := ",
        values_vec[i].before->raw ? "(raw)" : "", values_vec[i].field_qualifier);
      if (values_vec[i].before->errval) values_vec[i].before->errval->log();
      else TTCN_Logger::log_event_str("omit");
      TTCN_Logger::log_event_str(" } ");
    }
    if (values_vec[i].value) {
      TTCN_Logger::log_event("{ value%s %s := ",
        values_vec[i].value->raw ? "(raw)" : "", values_vec[i].field_qualifier);
      if (values_vec[i].value->errval) values_vec[i].value->errval->log();
      else TTCN_Logger::log_event_str("omit");
      TTCN_Logger::log_event_str(" } ");
    }
    if (values_vec[i].after) {
      TTCN_Logger::log_event("{ after%s %s := ",
        values_vec[i].after->raw ? "(raw)" : "", values_vec[i].field_qualifier);
      if (values_vec[i].after->errval) values_vec[i].after->errval->log();
      else TTCN_Logger::log_event_str("omit");
      TTCN_Logger::log_event_str(" } ");
    }
  }
  for (int i=0; i<embedded_size; i++) {
    embedded_vec[i].log_();
  }
}

void Base_Type::set_to_omit()
{
  TTCN_error("Internal error: trying to set a non-optional field to OMIT.");
}

void Base_Type::set_to_present()
{
  TTCN_error("Internal error: calling set_to_present() on a non-optional value.");
}

boolean Base_Type::is_present() const
{
  return is_bound();
}

Base_Type* Base_Type::get_opt_value()
{
  TTCN_error("Internal error: calling get_opt_value() on a non-optional value.");
  return NULL;
}

const Base_Type* Base_Type::get_opt_value() const
{
  TTCN_error("Internal error: calling get_opt_value() const on a non-optional value.");
  return NULL;
}

ASN_BER_TLV_t* Base_Type::BER_encode_TLV_negtest(const Erroneous_descriptor_t* /*p_err_descr*/,
  const TTCN_Typedescriptor_t& /*p_td*/, unsigned /*p_coding*/) const
{
  TTCN_error("Internal error: calling Base_Type::BER_encode_TLV_negtest().");
  return NULL;
}

int Base_Type::TEXT_encode_negtest(const Erroneous_descriptor_t* /*p_err_descr*/,
  const TTCN_Typedescriptor_t& /*p_td*/, TTCN_Buffer& /*p_buf*/) const
{
  TTCN_error("Internal error: calling Base_Type::TEXT_encode_negtest().");
  return 0;
}

ASN_BER_TLV_t* Base_Type::BER_encode_negtest_raw() const
{
  TTCN_error("A value of type %s cannot be used as erroneous raw value for BER encoding.",
             get_descriptor()->name);
  return NULL;
}

int Base_Type::encode_raw(TTCN_Buffer&) const
{
  TTCN_error("A value of type %s cannot be used as erroneous raw value for encoding.",
             get_descriptor()->name);
  return 0;
}

int Base_Type::RAW_encode_negtest_raw(RAW_enc_tree&) const
{
  TTCN_error("A value of type %s cannot be used as erroneous raw value for encoding.",
             get_descriptor()->name);
  return 0;
}

int Base_Type::JSON_encode_negtest_raw(JSON_Tokenizer&) const
{
  TTCN_error("A value of type %s cannot be used as erroneous raw value for JSON encoding.",
             get_descriptor()->name);
  return 0;
}

int Base_Type::XER_encode_negtest(const Erroneous_descriptor_t* /*p_err_descr*/,
  const XERdescriptor_t& p_td, TTCN_Buffer& p_buf, unsigned int flavor, unsigned int flavor2, int indent, embed_values_enc_struct_t*) const
{
  return XER_encode(p_td, p_buf, flavor, flavor2, indent, 0); // ignore erroneous
}

int Base_Type::RAW_encode_negtest(const Erroneous_descriptor_t *,
  const TTCN_Typedescriptor_t&, RAW_enc_tree&) const
{
  TTCN_error("Internal error: calling Base_Type::RAW_encode_negtest().");
  return 0;
}

int Base_Type::JSON_encode_negtest(const Erroneous_descriptor_t* /*p_err_descr*/,
  const TTCN_Typedescriptor_t& /*p_td*/, JSON_Tokenizer& /*p_tok*/) const
{
  TTCN_error("Internal error: calling Base_Type::JSON_encode_negtest().");
  return 0;
}

#else
#error this is for RT2 only
#endif

#ifdef TITAN_RUNTIME_2

boolean Record_Of_Type::compare_function(const Record_Of_Type *left_ptr,
  int left_index, const Record_Of_Type *right_ptr, int right_index)
{
  if (left_ptr->val_ptr == NULL)
    TTCN_error("The left operand of comparison is an unbound value of type %s.",
               left_ptr->get_descriptor()->name);
  if (right_ptr->val_ptr == NULL)
    TTCN_error("The right operand of comparison is an unbound value of type %s.",
               right_ptr->get_descriptor()->name);

  const Base_Type* elem = left_ptr->val_ptr->value_elements[left_index];
  const Base_Type* other_elem = right_ptr->val_ptr->value_elements[right_index];
  if (elem != NULL) {
    if (other_elem != NULL) { // both are bound, compare them
      return elem->is_equal(other_elem);
    }
    else return FALSE;
  }
  else { // elem unbound, they can be equal only if other_elem is unbound too
    return other_elem == NULL;
  }
}

void Record_Of_Type::clean_up()
{
  if (val_ptr != NULL) {
    if (val_ptr->ref_count > 1) {
      val_ptr->ref_count--;
      val_ptr = NULL;
    }
    else if (val_ptr->ref_count == 1) {
      if (NULL == refd_ind_ptr) {
        for (int elem_count = 0; elem_count < val_ptr->n_elements; elem_count++) {
          if (val_ptr->value_elements[elem_count] != NULL) {
            delete val_ptr->value_elements[elem_count];
          }
        }
        free_pointers((void**)val_ptr->value_elements);
        delete val_ptr;
        val_ptr = NULL;
      }
      else {
        set_size(0);
      }
    }
    else {
      TTCN_error("Internal error: Invalid reference counter in "
                 "a record of/set of value.");
    }
  }
}

Record_Of_Type::Record_Of_Type(null_type /*other_value*/)
: Base_Type(), val_ptr(new recordof_setof_struct), err_descr(NULL), refd_ind_ptr(NULL)
{
  val_ptr->ref_count = 1;
  val_ptr->n_elements = 0;
  val_ptr->value_elements = NULL;
}

Record_Of_Type::Record_Of_Type(const Record_Of_Type& other_value)
: Base_Type(other_value), RefdIndexInterface(other_value)
, val_ptr(NULL), err_descr(other_value.err_descr), refd_ind_ptr(NULL)
{
  if (!other_value.is_bound())
    TTCN_error("Copying an unbound record of/set of value.");
  // Increment ref_count only if val_ptr is not NULL
  if (other_value.val_ptr != NULL) {
    if (NULL == other_value.refd_ind_ptr) {
      val_ptr = other_value.val_ptr;
      val_ptr->ref_count++;
    }
    else {
      // there are references to at least one element => the array must be copied
      int nof_elements = other_value.get_nof_elements();
      set_size(nof_elements);
      for (int i = 0; i < nof_elements; ++i) {
        if (other_value.is_elem_bound(i)) {
          val_ptr->value_elements[i] = other_value.val_ptr->value_elements[i]->clone();
        }
      }
    }
  }
}

int Record_Of_Type::get_nof_elements() const
{
  int nof_elements = (val_ptr != NULL) ? val_ptr->n_elements : 0;
  if (NULL != refd_ind_ptr) {
    while (nof_elements > 0) {
      if (is_elem_bound(nof_elements - 1)) {
        break;
      }
      --nof_elements;
    }
  }
  return nof_elements;
}

boolean Record_Of_Type::is_elem_bound(int index) const
{
  return val_ptr->value_elements[index] != NULL && 
         val_ptr->value_elements[index]->is_bound();
}

int Record_Of_Type::get_max_refd_index()
{
  if (NULL == refd_ind_ptr) {
    return -1;
  } 
  if (-1 == refd_ind_ptr->max_refd_index) {
    for (size_t i = 0; i < refd_ind_ptr->refd_indices.size(); ++i) {
      if (refd_ind_ptr->refd_indices[i] > refd_ind_ptr->max_refd_index) {
        refd_ind_ptr->max_refd_index = refd_ind_ptr->refd_indices[i];
      }
    }
  } 
  return refd_ind_ptr->max_refd_index;
}

boolean Record_Of_Type::is_index_refd(int index)
{
  if (NULL == refd_ind_ptr) {
    return FALSE;
  }
  for (size_t i = 0; i < refd_ind_ptr->refd_indices.size(); ++i) {
    if (index == refd_ind_ptr->refd_indices[i]) {
      return TRUE;
    }
  }
  return FALSE;
}

void Record_Of_Type::set_val(null_type)
{
  set_size(0);
}

boolean Record_Of_Type::is_equal(const Base_Type* other_value) const
{
  const Record_Of_Type* other_recof = static_cast<const Record_Of_Type*>(other_value);
  if (val_ptr == NULL)
    TTCN_error("The left operand of comparison is an unbound value of type %s.",
               get_descriptor()->name);
  if (other_recof->val_ptr == NULL)
    TTCN_error("The right operand of comparison is an unbound value of type %s.",
               other_value->get_descriptor()->name);
  if (val_ptr == other_recof->val_ptr) return TRUE;
  if (is_set()) {
    return compare_set_of(this, get_nof_elements(), other_recof,
             other_recof->get_nof_elements(), compare_function);
  } else {
    if (get_nof_elements() != other_recof->get_nof_elements()) return FALSE;
    for (int elem_count = 0; elem_count < get_nof_elements(); elem_count++) {
      if (is_elem_bound(elem_count)) {
        if (other_recof->is_elem_bound(elem_count)) {
           if (!val_ptr->value_elements[elem_count]->is_equal(other_recof->val_ptr->value_elements[elem_count]))
             return FALSE;
        } else return FALSE;
      } else if (other_recof->is_elem_bound(elem_count)) return FALSE;
    }
    return TRUE;
  }
}

void Record_Of_Type::set_value(const Base_Type* other_value)
{
  const Record_Of_Type* other_recof = static_cast<const Record_Of_Type*>(other_value);
  if (!other_recof->is_bound())
    TTCN_error("Assigning an unbound value of type %s.",
               other_value->get_descriptor()->name);
  if (this != other_recof) {
    if (NULL == refd_ind_ptr && NULL == other_recof->refd_ind_ptr) {
      clean_up();
      val_ptr = other_recof->val_ptr;
      val_ptr->ref_count++;
    }
    else {
      // there are references to at least one element => the array must be copied
      int nof_elements = other_recof->get_nof_elements();
      set_size(nof_elements);
      for (int i = 0; i < nof_elements; ++i) {
        if (other_recof->is_elem_bound(i)) {
          if (val_ptr->value_elements[i] == NULL) {
            val_ptr->value_elements[i] = create_elem();
          }
          val_ptr->value_elements[i]->set_value(other_recof->val_ptr->value_elements[i]);
        }
        else if (val_ptr->value_elements[i] != NULL) {
          if (is_index_refd(i)) {
            val_ptr->value_elements[i]->clean_up();
          }
          else {
            delete val_ptr->value_elements[i];
            val_ptr->value_elements[i] = NULL;
          }
        }
      }
    }
  }
  err_descr = other_recof->err_descr;
}

boolean Record_Of_Type::operator!=(null_type other_value) const
{
  return !(*this == other_value);
}

Base_Type* Record_Of_Type::get_at(int index_value)
{
  if (index_value < 0)
    TTCN_error("Accessing an element of type %s using a negative index: %d.",
               get_descriptor()->name, index_value);
  if (val_ptr == NULL) {
    val_ptr = new recordof_setof_struct;
    val_ptr->ref_count = 1;
    val_ptr->n_elements = 0;
    val_ptr->value_elements = NULL;
  } else if (val_ptr->ref_count > 1) {
    struct recordof_setof_struct *new_val_ptr = new recordof_setof_struct;
    new_val_ptr->ref_count = 1;
    new_val_ptr->n_elements = (index_value >= val_ptr->n_elements) ?
      index_value + 1 : val_ptr->n_elements;
    new_val_ptr->value_elements =
      (Base_Type**)allocate_pointers(new_val_ptr->n_elements);
    for (int elem_count = 0; elem_count < val_ptr->n_elements; elem_count++)
    {
      if (val_ptr->value_elements[elem_count] != NULL) {
        new_val_ptr->value_elements[elem_count] =
          val_ptr->value_elements[elem_count]->clone();
      }
    }
    val_ptr->ref_count--;
    val_ptr = new_val_ptr;
  }
  if (index_value >= val_ptr->n_elements) set_size(index_value + 1);
  if (val_ptr->value_elements[index_value] == NULL) {
    val_ptr->value_elements[index_value] = create_elem();
  }
  return val_ptr->value_elements[index_value];
}

Base_Type* Record_Of_Type::get_at(const INTEGER& index_value)
{
  if (!index_value.is_bound())
    TTCN_error("Using an unbound integer value for indexing a value "
               "of type %s.", get_descriptor()->name);
  return get_at((int)index_value);
}

const Base_Type* Record_Of_Type::get_at(int index_value) const
{
  if (val_ptr == NULL)
    TTCN_error("Accessing an element in an unbound value of type %s.",
               get_descriptor()->name);
  if (index_value < 0)
    TTCN_error("Accessing an element of type %s using a negative index: %d.",
               get_descriptor()->name, index_value);
  if (index_value >= get_nof_elements())
    TTCN_error("Index overflow in a value of type %s: The index is %d, but the "
               "value has only %d elements.", get_descriptor()->name, index_value,
               get_nof_elements());
  return (val_ptr->value_elements[index_value] != NULL) ?
    val_ptr->value_elements[index_value] : get_unbound_elem();
}

const Base_Type* Record_Of_Type::get_at(const INTEGER& index_value) const
{
  if (!index_value.is_bound())
    TTCN_error("Using an unbound integer value for indexing a value "
               "of type %s.", get_descriptor()->name);
  return get_at((int)index_value);
}

Record_Of_Type* Record_Of_Type::rotl(const INTEGER& rotate_count, Record_Of_Type* rec_of) const
{
  if (!rotate_count.is_bound())
    TTCN_error("Unbound integer operand of rotate left operator of type %s.",
               get_descriptor()->name);
  return rotr((int)(-rotate_count), rec_of);
}

Record_Of_Type* Record_Of_Type::rotr(const INTEGER& rotate_count, Record_Of_Type* rec_of) const
{
  if (!rotate_count.is_bound())
    TTCN_error("Unbound integer operand of rotate right operator of type %s.",
               get_descriptor()->name);
  return rotr((int)rotate_count, rec_of);
}

Record_Of_Type* Record_Of_Type::rotr(int rotate_count, Record_Of_Type* rec_of) const
{
  if (val_ptr == NULL)
    TTCN_error("Performing rotation operation on an unbound value of type %s.",
               get_descriptor()->name);
  int nof_elements = get_nof_elements();
  if (nof_elements == 0) return const_cast<Record_Of_Type*>(this);
  int rc;
  if (rotate_count>=0) rc = rotate_count % nof_elements;
  else rc = nof_elements - ((-rotate_count) % nof_elements);
  if (rc == 0) return const_cast<Record_Of_Type*>(this);
  rec_of->set_size(nof_elements);
  int rot_i;
  for (int i=0; i<nof_elements; i++) {
    rot_i = (i+rc) % nof_elements;
    if (is_elem_bound(i)) {
      if (rec_of->val_ptr->value_elements[rot_i] == NULL) {
        rec_of->val_ptr->value_elements[rot_i] = rec_of->create_elem();
      }
      rec_of->val_ptr->value_elements[rot_i]->set_value(val_ptr->value_elements[i]);
    } else if (rec_of->is_elem_bound(rot_i)) {
      delete rec_of->val_ptr->value_elements[rot_i];
      rec_of->val_ptr->value_elements[rot_i] = NULL;
    }
  }
  return rec_of;
}

Record_Of_Type* Record_Of_Type::concat(const Record_Of_Type* other_value,
                                          Record_Of_Type* rec_of) const
{
  if (val_ptr == NULL || other_value->val_ptr == NULL)
    TTCN_error("Unbound operand of %s concatenation.", get_descriptor()->name);
  int nof_elements = get_nof_elements();
  if (nof_elements == 0) return const_cast<Record_Of_Type*>(other_value);
  int other_value_nof_elements = other_value->get_nof_elements();
  if (other_value_nof_elements == 0) return const_cast<Record_Of_Type*>(this);
  rec_of->set_size(nof_elements + other_value_nof_elements);
  for (int i=0; i<nof_elements; i++) {
    if (is_elem_bound(i)) {
      if (rec_of->val_ptr->value_elements[i] == NULL) {
        rec_of->val_ptr->value_elements[i] = rec_of->create_elem();
      }
      rec_of->val_ptr->value_elements[i]->set_value(val_ptr->value_elements[i]);
    } else if (rec_of->val_ptr->value_elements[i] != NULL) {
      if (rec_of->is_index_refd(i)) {
        rec_of->val_ptr->value_elements[i]->clean_up();
      }
      else {
        delete rec_of->val_ptr->value_elements[i];
        rec_of->val_ptr->value_elements[i] = NULL;
      }
    }
  }
  int cat_i;
  for (int i=0; i<other_value_nof_elements; i++) {
    cat_i = i + nof_elements;
    if (other_value->is_elem_bound(i)) {
      if (rec_of->val_ptr->value_elements[cat_i] == NULL) {
        rec_of->val_ptr->value_elements[cat_i] = rec_of->create_elem();
      }
      rec_of->val_ptr->value_elements[cat_i]->
        set_value(other_value->val_ptr->value_elements[i]);
    } else if (rec_of->val_ptr->value_elements[cat_i] != NULL) {
      if (rec_of->is_index_refd(cat_i)) {
        rec_of->val_ptr->value_elements[cat_i]->clean_up();
      }
      else {
        delete rec_of->val_ptr->value_elements[cat_i];
        rec_of->val_ptr->value_elements[cat_i] = NULL;
      }
    }
  }
  return rec_of;
}

void Record_Of_Type::substr_(int index, int returncount,
                                       Record_Of_Type* rec_of) const
{
  if (val_ptr == NULL)
    TTCN_error("The first argument of substr() is an unbound value of type %s.",
               get_descriptor()->name);
  check_substr_arguments(get_nof_elements(), index, returncount,
                         get_descriptor()->name, "element");
  rec_of->set_size(returncount);
  for (int i=0; i<returncount; i++) {
    if (is_elem_bound(i + index)) {
      if (rec_of->val_ptr->value_elements[i] == NULL) {
        rec_of->val_ptr->value_elements[i] = rec_of->create_elem();
      }
      rec_of->val_ptr->value_elements[i]->
        set_value(val_ptr->value_elements[i+index]);
    } else if (rec_of->val_ptr->value_elements[i] != NULL) {
      if (rec_of->is_index_refd(i)) {
        rec_of->val_ptr->value_elements[i]->clean_up();
      }
      else {
        delete rec_of->val_ptr->value_elements[i];
        rec_of->val_ptr->value_elements[i] = NULL;
      }
    }
  }
}

void Record_Of_Type::replace_(int index, int len,
  const Record_Of_Type* repl, Record_Of_Type* rec_of) const
{
  if (val_ptr == NULL)
    TTCN_error("The first argument of replace() is an unbound value "
               "of type %s.", get_descriptor()->name);
  if (repl->val_ptr == NULL)
    TTCN_error("The fourth argument of replace() is an unbound value of "
               "type %s.", get_descriptor()->name);
  int nof_elements = get_nof_elements();
  check_replace_arguments(nof_elements, index, len,
                          get_descriptor()->name, "element");
  int repl_nof_elements = repl->get_nof_elements();
  rec_of->set_size(nof_elements + repl_nof_elements - len);
  for (int i = 0; i < index; i++) {
    if (is_elem_bound(i)) {
      if (rec_of->val_ptr->value_elements[i] == NULL) {
        rec_of->val_ptr->value_elements[i] = rec_of->create_elem();
      }
      rec_of->val_ptr->value_elements[i]->set_value(val_ptr->value_elements[i]);
    } else if (rec_of->val_ptr->value_elements[i] != NULL) {
      if (rec_of->is_index_refd(i)) {
        rec_of->val_ptr->value_elements[i]->clean_up();
      }
      else {
        delete rec_of->val_ptr->value_elements[i];
        rec_of->val_ptr->value_elements[i] = NULL;
      }
    }
  }
  for (int i = 0; i < repl_nof_elements; i++) {
    if (repl->is_elem_bound(i)) {
      if (rec_of->val_ptr->value_elements[i+index] == NULL) {
        rec_of->val_ptr->value_elements[i+index] = rec_of->create_elem();
      }
      rec_of->val_ptr->value_elements[i+index]->
        set_value(repl->val_ptr->value_elements[i]);
    } else if (rec_of->val_ptr->value_elements[i+index] != NULL) {
      if (rec_of->is_index_refd(i+index)) {
        rec_of->val_ptr->value_elements[i+index]->clean_up();
      }
      else {
        delete rec_of->val_ptr->value_elements[i+index];
        rec_of->val_ptr->value_elements[i+index] = NULL;
      }
    }
  }
  int repl_i;
  for (int i = 0; i < nof_elements - index - len; i++) {
    repl_i = index+i+repl_nof_elements;
    if (is_elem_bound(index+i+len)) {
      if (rec_of->val_ptr->value_elements[repl_i] == NULL) {
        rec_of->val_ptr->value_elements[repl_i] = rec_of->create_elem();
      }
      rec_of->val_ptr->value_elements[repl_i]->
        set_value(val_ptr->value_elements[index+i+len]);
    } else if (rec_of->val_ptr->value_elements[repl_i] != NULL) {
      if (rec_of->is_index_refd(repl_i)) {
        rec_of->val_ptr->value_elements[repl_i]->clean_up();
      }
      else {
        delete rec_of->val_ptr->value_elements[repl_i];
        rec_of->val_ptr->value_elements[repl_i] = NULL;
      }
    }
  }
}

void Record_Of_Type::replace_(int index, int len,
  const Record_Of_Template* repl, Record_Of_Type* rec_of) const
{
  if (!repl->is_value())
    TTCN_error("The fourth argument of function replace() is a template "
               "of type %s with non-specific value.", get_descriptor()->name);
  rec_of->set_val(NULL_VALUE);
  Base_Type* repl_value = rec_of->clone();
  repl->valueofv(repl_value);
  replace_(index, len, static_cast<Record_Of_Type*>(repl_value), rec_of);
  delete repl_value;
}

void Record_Of_Type::replace_(int index, int len,
  const Set_Of_Template* repl, Record_Of_Type* rec_of) const
{
  if (!repl->is_value())
    TTCN_error("The fourth argument of function replace() is a template "
               "of type %s with non-specific value.", get_descriptor()->name);
  rec_of->set_val(NULL_VALUE);
  Base_Type* repl_value = rec_of->clone();
  repl->valueofv(repl_value);
  replace_(index, len, static_cast<Record_Of_Type*>(repl_value), rec_of);
  delete repl_value;
}

void Record_Of_Type::set_size(int new_size)
{
  if (new_size < 0)
    TTCN_error("Internal error: Setting a negative size for a value of "
               "type %s.", get_descriptor()->name);
  if (val_ptr == NULL) {
    val_ptr = new recordof_setof_struct;
    val_ptr->ref_count = 1;
    val_ptr->n_elements = 0;
    val_ptr->value_elements = NULL;
  } else if (val_ptr->ref_count > 1) {
    struct recordof_setof_struct *new_val_ptr = new recordof_setof_struct;
    new_val_ptr->ref_count = 1;
    new_val_ptr->n_elements = (new_size < val_ptr->n_elements) ?
      new_size : val_ptr->n_elements;
    new_val_ptr->value_elements =
      (Base_Type**)allocate_pointers(new_val_ptr->n_elements);
    for (int elem_count = 0; elem_count < new_val_ptr->n_elements; elem_count++) {
      if (val_ptr->value_elements[elem_count] != NULL) {
        new_val_ptr->value_elements[elem_count] =
          val_ptr->value_elements[elem_count]->clone();
      }
    }
    clean_up();
    val_ptr = new_val_ptr;
  }
  if (new_size > val_ptr->n_elements) {
    val_ptr->value_elements = (Base_Type**)
      reallocate_pointers((void**)val_ptr->value_elements, val_ptr->n_elements, new_size);
    val_ptr->n_elements = new_size;
  } else if (new_size < val_ptr->n_elements) {
    for (int elem_count = new_size; elem_count < val_ptr->n_elements; elem_count++) {
      if (val_ptr->value_elements[elem_count] != NULL) {
        if (is_index_refd(elem_count)) {
          val_ptr->value_elements[elem_count]->clean_up();
        }
        else {
          delete val_ptr->value_elements[elem_count];
          val_ptr->value_elements[elem_count] = 0;
        }
      }
    }
    if (new_size <= get_max_refd_index()) {
      new_size = get_max_refd_index() + 1;
    }
    if (new_size < val_ptr->n_elements) {
      val_ptr->value_elements = (Base_Type**)
        reallocate_pointers((void**)val_ptr->value_elements, val_ptr->n_elements, new_size);
      val_ptr->n_elements = new_size;
    }
  }
}

boolean Record_Of_Type::is_bound() const
{
  if (NULL == refd_ind_ptr) {
    return (val_ptr != NULL);
  }
  return (get_nof_elements() != 0);
}

boolean Record_Of_Type::is_value() const
{
  if (val_ptr == NULL) return FALSE;
  for (int i=0; i < get_nof_elements(); ++i)
    if (!is_elem_bound(i) ||
        !val_ptr->value_elements[i]->is_value()) return FALSE;
  return TRUE;
}

int Record_Of_Type::size_of() const
{
  if (val_ptr == NULL)
    TTCN_error("Performing sizeof operation on an unbound value of type %s.",
               get_descriptor()->name);
  return get_nof_elements();
}

int Record_Of_Type::lengthof() const
{
  if (val_ptr == NULL)
    TTCN_error("Performing lengthof operation on an unbound value of "
               "type %s.", get_descriptor()->name);
  for (int my_length=get_nof_elements(); my_length>0; my_length--)
    if (is_elem_bound(my_length - 1)) return my_length;
  return 0;
}

void Record_Of_Type::log() const
{
  if (val_ptr == NULL) {
    TTCN_Logger::log_event_unbound();
    return;
  }
  if (get_nof_elements()==0) {
    TTCN_Logger::log_event_str("{ }");
  } else {
    TTCN_Logger::log_event_str("{ ");
    for (int elem_count = 0; elem_count < get_nof_elements(); elem_count++) {
      if (elem_count > 0) TTCN_Logger::log_event_str(", ");
      get_at(elem_count)->log();
    }
    TTCN_Logger::log_event_str(" }");
  }
  if (err_descr) err_descr->log();
}

void Record_Of_Type::encode_text(Text_Buf& text_buf) const
{
  if (val_ptr == NULL)
    TTCN_error("Text encoder: Encoding an unbound value of type %s.",
               get_descriptor()->name);
  text_buf.push_int(get_nof_elements());
  for (int elem_count = 0; elem_count < get_nof_elements(); elem_count++)
    get_at(elem_count)->encode_text(text_buf);
}

void Record_Of_Type::decode_text(Text_Buf& text_buf)
{
  int new_size = text_buf.pull_int().get_val();
  if (new_size < 0)
    TTCN_error("Text decoder: Negative size was received for a value of "
               "type %s.", get_descriptor()->name);
  set_size(new_size);
  for (int elem_count = 0; elem_count < new_size; elem_count++) {
    if (val_ptr->value_elements[elem_count] == NULL) {
      val_ptr->value_elements[elem_count] = create_elem();
    }
    val_ptr->value_elements[elem_count]->decode_text(text_buf);
  }
}

boolean Record_Of_Type::operator==(null_type /*other_value*/) const
{
  if (val_ptr == NULL)
    TTCN_error("The left operand of comparison is an unbound value of type %s.",
               get_descriptor()->name);
  return get_nof_elements() == 0;
}

int Record_Of_Type::rawdec_ebv() const
{
  TTCN_error("Internal error: Record_Of_Type::rawdec_ebv() called.");
}

boolean Record_Of_Type::isXerAttribute() const
{
  TTCN_error("Internal error: Record_Of_Type::isXerAttribute() called.");
}

boolean Record_Of_Type::isXmlValueList() const
{
  TTCN_error("Internal error: Record_Of_Type::isXmlValueList() called.");
}

int Record_Of_Type::TEXT_encode(const TTCN_Typedescriptor_t& p_td,
                                TTCN_Buffer& buff) const
{
  if(err_descr){
	  return TEXT_encode_negtest(err_descr, p_td, buff);
  }
  int encoded_length=0;
  if(p_td.text->begin_encode) {
    buff.put_cs(*p_td.text->begin_encode);
    encoded_length+=p_td.text->begin_encode->lengthof();
  }
  if(val_ptr==NULL) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound value.");
    if(p_td.text->end_encode) {
      buff.put_cs(*p_td.text->end_encode);
      encoded_length+=p_td.text->end_encode->lengthof();
    }
    return encoded_length;
  }
  const TTCN_Typedescriptor_t* elem_descr = p_td.oftype_descr;
  for(int a=0;a<get_nof_elements();a++) {
    if(a!=0 && p_td.text->separator_encode) {
      buff.put_cs(*p_td.text->separator_encode);
      encoded_length+=p_td.text->separator_encode->lengthof();
    }
    encoded_length+=get_at(a)->TEXT_encode(*elem_descr,buff);
  }
  if(p_td.text->end_encode) {
    buff.put_cs(*p_td.text->end_encode);
    encoded_length+=p_td.text->end_encode->lengthof();
  }
  return encoded_length;
}

/**
 * TEXT encode for negative testing
 */
int Record_Of_Type::TEXT_encode_negtest(const Erroneous_descriptor_t* p_err_descr, const TTCN_Typedescriptor_t& p_td,
                                TTCN_Buffer& buff) const
{
  boolean need_separator=FALSE;
  int encoded_length=0;
  if(p_td.text->begin_encode) {
    buff.put_cs(*p_td.text->begin_encode);
    encoded_length+=p_td.text->begin_encode->lengthof();
  }
  if(val_ptr==NULL) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound value.");
    if(p_td.text->end_encode) {
      buff.put_cs(*p_td.text->end_encode);
      encoded_length+=p_td.text->end_encode->lengthof();
    }
    return encoded_length;
  }

  int values_idx = 0;
  int edescr_idx = 0;

  for(int a=0;a<get_nof_elements();a++) {
    if ( (p_err_descr->omit_before!=-1) && (a<p_err_descr->omit_before) ) continue;
    const Erroneous_values_t* err_vals = p_err_descr->next_field_err_values(a, values_idx);
    const Erroneous_descriptor_t* emb_descr = p_err_descr->next_field_emb_descr(a, edescr_idx);

    if (err_vals && err_vals->before) {
      if (err_vals->before->errval==NULL) TTCN_error(
        "internal error: erroneous before value missing");
      if (need_separator && p_td.text->separator_encode) {
        buff.put_cs(*p_td.text->separator_encode);
        encoded_length+=p_td.text->separator_encode->lengthof();
      }
      if (err_vals->before->raw) {
        encoded_length += err_vals->before->errval->encode_raw(buff);
      } else {
        if (err_vals->before->type_descr==NULL) TTCN_error(
          "internal error: erroneous before typedescriptor missing");
        encoded_length += err_vals->before->errval->TEXT_encode(
          *(err_vals->before->type_descr),buff);
      }
      need_separator=TRUE;
    }

    if (err_vals && err_vals->value) {
      if (err_vals->value->errval) {
        if (need_separator && p_td.text->separator_encode) {
          buff.put_cs(*p_td.text->separator_encode);
          encoded_length+=p_td.text->separator_encode->lengthof();
        }
        if (err_vals->value->raw) {
          encoded_length += err_vals->value->errval->encode_raw(buff);
        } else {
          if (err_vals->value->type_descr==NULL) TTCN_error(
            "internal error: erroneous value typedescriptor missing");
          encoded_length += err_vals->value->errval->TEXT_encode(
            *(err_vals->value->type_descr),buff);
        }
        need_separator=TRUE;
      } // else -> omit
    } else {
      if (need_separator && p_td.text->separator_encode) {
        buff.put_cs(*p_td.text->separator_encode);
        encoded_length+=p_td.text->separator_encode->lengthof();
      }
      if (emb_descr) {
        encoded_length += get_at(a)->TEXT_encode_negtest(
          emb_descr,*p_td.oftype_descr,buff);
      } else {
        encoded_length += get_at(a)->TEXT_encode(*p_td.oftype_descr,buff);
      }
      need_separator=TRUE;
    }

    if (err_vals && err_vals->after) {
      if (err_vals->after->errval==NULL) TTCN_error(
        "internal error: erroneous after value missing");
      if (need_separator && p_td.text->separator_encode) {
        buff.put_cs(*p_td.text->separator_encode);
        encoded_length+=p_td.text->separator_encode->lengthof();
      }
      if (err_vals->after->raw) {
        encoded_length += err_vals->after->errval->encode_raw(buff);
      } else {
        if (err_vals->after->type_descr==NULL) TTCN_error(
          "internal error: erroneous after typedescriptor missing");
        encoded_length += err_vals->after->errval->TEXT_encode(
          *(err_vals->after->type_descr),buff);
      }
      need_separator=TRUE;
    }

    if ( (p_err_descr->omit_after!=-1) && (a>=p_err_descr->omit_after) ) break;
  }
  if(p_td.text->end_encode) {
    buff.put_cs(*p_td.text->end_encode);
    encoded_length+=p_td.text->end_encode->lengthof();
  }
  return encoded_length;
}

int Record_Of_Type::TEXT_decode(const TTCN_Typedescriptor_t& p_td,
  TTCN_Buffer& buff, Limit_Token_List& limit, boolean no_err,
  boolean first_call)
{
  int decoded_length=0;
  size_t pos;
  boolean sep_found=FALSE;
  int sep_length=0;
  int ml=0;
  if(p_td.text->begin_decode){
    int tl;
    if((tl=p_td.text->begin_decode->match_begin(buff))<0){
      if(no_err)return -1;
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TOKEN_ERR,
        "The specified token '%s' not found for '%s': ",
        (const char*)*(p_td.text->begin_decode), p_td.name);
      return 0;
    }
    decoded_length+=tl;
    buff.increase_pos(tl);
  }
  if(p_td.text->end_decode){
    limit.add_token(p_td.text->end_decode);
    ml++;
  }
  if(p_td.text->separator_decode){
    limit.add_token(p_td.text->separator_decode);
    ml++;
  }
  if(first_call) {
    set_size(0);
  }
  int more=get_nof_elements();
  while(TRUE){
    Base_Type* val = create_elem();
    pos=buff.get_pos();
    int len = val->TEXT_decode(*p_td.oftype_descr,buff,limit,TRUE);
    if(len==-1 || (len==0 && !limit.has_token())){
      buff.set_pos(pos);
      delete val;
      if(sep_found){
        buff.set_pos(buff.get_pos()-sep_length);
        decoded_length-=sep_length;
      }
      break;
    }
    sep_found=FALSE;
    if (NULL == refd_ind_ptr) {
      val_ptr->value_elements = (Base_Type**)reallocate_pointers(
        (void**)val_ptr->value_elements, val_ptr->n_elements, val_ptr->n_elements + 1);
      val_ptr->value_elements[val_ptr->n_elements]=val;
      val_ptr->n_elements++;
    }
    else {
      get_at(get_nof_elements())->set_value(val);
      delete val;
    }
    decoded_length+=len;
    if(p_td.text->separator_decode){
      int tl;
      if((tl=p_td.text->separator_decode->match_begin(buff))<0){
        break;
      }
      decoded_length+=tl;
      buff.increase_pos(tl);
      sep_length=tl;
      sep_found=TRUE;
    } else if(p_td.text->end_decode){
      int tl;
      if((tl=p_td.text->end_decode->match_begin(buff))!=-1){
        decoded_length+=tl;
        buff.increase_pos(tl);
        limit.remove_tokens(ml);
        return decoded_length;
      }
    } else if(limit.has_token(ml)){
      int tl;
      if((tl=limit.match(buff,ml))==0){
        //sep_found=FALSE;
        break;
      }
    }
  }
  limit.remove_tokens(ml);
  if(p_td.text->end_decode){
    int tl;
    if((tl=p_td.text->end_decode->match_begin(buff))<0){
      if(no_err){
        if(!first_call){
          set_size(more);
        }
        return -1;
      }
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TOKEN_ERR,
        "The specified token '%s' not found for '%s': ",
        (const char*)*(p_td.text->end_decode), p_td.name);
      return decoded_length;
    }
    decoded_length+=tl;
    buff.increase_pos(tl);
  }
  if(get_nof_elements()==0){
    if (!p_td.text->end_decode && !p_td.text->begin_decode) {
      if(no_err)return -1;
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TOKEN_ERR,
        "No record/set of member found.");
      return decoded_length;
    }
  }
  if(!first_call && more==get_nof_elements() &&
     !(p_td.text->end_decode || p_td.text->begin_decode)) return -1;
  return decoded_length;
}

ASN_BER_TLV_t* Record_Of_Type::BER_encode_TLV(const TTCN_Typedescriptor_t& p_td,
                                              unsigned p_coding) const
{
  if (err_descr) {
    return BER_encode_TLV_negtest(err_descr, p_td, p_coding);
  }
  BER_chk_descr(p_td);
  ASN_BER_TLV_t *new_tlv=BER_encode_chk_bound(is_bound());
  if(!new_tlv) {
    new_tlv=ASN_BER_TLV_t::construct(NULL);
    TTCN_EncDec_ErrorContext ec;
    for(int elem_i=0; elem_i<get_nof_elements(); elem_i++) {
      ec.set_msg("Component #%d: ", elem_i);
      new_tlv->add_TLV(get_at(elem_i)->BER_encode_TLV(*p_td.oftype_descr, p_coding));
    }
    if (is_set()) new_tlv->sort_tlvs();
  }
  new_tlv=ASN_BER_V2TLV(new_tlv, p_td, p_coding);
  return new_tlv;
}

ASN_BER_TLV_t* Record_Of_Type::BER_encode_TLV_negtest(const Erroneous_descriptor_t* p_err_descr,
  const TTCN_Typedescriptor_t& p_td, unsigned p_coding) const
{
  BER_chk_descr(p_td);
  ASN_BER_TLV_t *new_tlv=BER_encode_chk_bound(is_bound());
  if(!new_tlv) {
    new_tlv=ASN_BER_TLV_t::construct(NULL);
    TTCN_EncDec_ErrorContext ec;
    int values_idx = 0;
    int edescr_idx = 0;
    for (int elem_i=0; elem_i<get_nof_elements(); elem_i++) {
      if ( (p_err_descr->omit_before!=-1) && (elem_i<p_err_descr->omit_before) ) continue;
      const Erroneous_values_t* err_vals = p_err_descr->next_field_err_values(elem_i, values_idx);
      const Erroneous_descriptor_t* emb_descr = p_err_descr->next_field_emb_descr(elem_i, edescr_idx);

      if (err_vals && err_vals->before) {
        if (err_vals->before->errval==NULL) TTCN_error(
          "internal error: erroneous before value missing");
        ec.set_msg("Erroneous value before component #%d: ", elem_i);
        if (err_vals->before->raw) {
          new_tlv->add_TLV(err_vals->before->errval->BER_encode_negtest_raw());
        } else {
          if (err_vals->before->type_descr==NULL) TTCN_error(
            "internal error: erroneous before typedescriptor missing");
          new_tlv->add_TLV(err_vals->before->errval->BER_encode_TLV(
            *err_vals->before->type_descr, p_coding));
        }
      }

      if (err_vals && err_vals->value) {
        if (err_vals->value->errval) { // replace
          ec.set_msg("Erroneous value for component #%d: ", elem_i);
          if (err_vals->value->raw) {
            new_tlv->add_TLV(err_vals->value->errval->BER_encode_negtest_raw());
          } else {
            if (err_vals->value->type_descr==NULL) TTCN_error(
              "internal error: erroneous value typedescriptor missing");
            new_tlv->add_TLV(err_vals->value->errval->BER_encode_TLV(
              *err_vals->value->type_descr, p_coding));
          }
        } // else -> omit
      } else {
        ec.set_msg("Component #%d: ", elem_i);
        if (emb_descr) {
          new_tlv->add_TLV(get_at(elem_i)->BER_encode_TLV_negtest(
            emb_descr, *p_td.oftype_descr, p_coding));
        } else {
          new_tlv->add_TLV(get_at(elem_i)->BER_encode_TLV(
            *p_td.oftype_descr, p_coding));
        }
      }

      if (err_vals && err_vals->after) {
        if (err_vals->after->errval==NULL) TTCN_error(
          "internal error: erroneous after value missing");
        ec.set_msg("Erroneous value after component #%d: ", elem_i);
        if (err_vals->after->raw) {
          new_tlv->add_TLV(err_vals->after->errval->BER_encode_negtest_raw());
        } else {
          if (err_vals->after->type_descr==NULL) TTCN_error(
            "internal error: erroneous after typedescriptor missing");
          new_tlv->add_TLV(err_vals->after->errval->BER_encode_TLV(
            *err_vals->after->type_descr, p_coding));
        }
      }

      if ( (p_err_descr->omit_after!=-1) && (elem_i>=p_err_descr->omit_after) ) break;
    }
    if (is_set()) new_tlv->sort_tlvs();
  }
  new_tlv=ASN_BER_V2TLV(new_tlv, p_td, p_coding);
  return new_tlv;
}

boolean Record_Of_Type::BER_decode_TLV(const TTCN_Typedescriptor_t& p_td,
  const ASN_BER_TLV_t& p_tlv, unsigned L_form)
{
  BER_chk_descr(p_td);
  ASN_BER_TLV_t stripped_tlv;
  BER_decode_strip_tags(*p_td.ber, p_tlv, L_form, stripped_tlv);
  TTCN_EncDec_ErrorContext ec_0("While decoding '%s' type: ", p_td.name);
  stripped_tlv.chk_constructed_flag(TRUE);
  set_size(0);
  size_t V_pos=0;
  ASN_BER_TLV_t tmp_tlv;
  TTCN_EncDec_ErrorContext ec_1("Component #");
  TTCN_EncDec_ErrorContext ec_2("0: ");
  while(BER_decode_constdTLV_next(stripped_tlv, V_pos, L_form, tmp_tlv)) {
    get_at(get_nof_elements())->BER_decode_TLV(*p_td.oftype_descr, tmp_tlv, L_form);
    ec_2.set_msg("%d: ", val_ptr->n_elements);
  }
  return TRUE;
}

void Record_Of_Type::BER_decode_opentypes(TTCN_Type_list& p_typelist,
                                          unsigned L_form)
{
  p_typelist.push(this);
  TTCN_EncDec_ErrorContext ec_0("Component #");
  TTCN_EncDec_ErrorContext ec_1;
  for(int elem_i=0; elem_i<get_nof_elements(); elem_i++) {
    ec_1.set_msg("%d: ", elem_i);
    get_at(elem_i)->BER_decode_opentypes(p_typelist, L_form);
  }
  p_typelist.pop();
}


int Record_Of_Type::RAW_decode(const TTCN_Typedescriptor_t& p_td,
  TTCN_Buffer& buff, int limit, raw_order_t top_bit_ord, boolean /*no_err*/,
  int sel_field, boolean first_call)
{
  int prepaddlength = buff.increase_pos_padd(p_td.raw->prepadding);
  limit -= prepaddlength;
  int decoded_length = 0;
  int decoded_field_length = 0;
  size_t start_of_field = 0;
  if (first_call) {
    set_size(0);
  }
  int start_field = get_nof_elements(); // append at the end
  TTCN_Typedescriptor_t const& elem_descr = *p_td.oftype_descr;
  if (p_td.raw->fieldlength || sel_field != -1) {
    if (sel_field == -1) sel_field = p_td.raw->fieldlength;
    for (int a = 0; a < sel_field; a++) {
      Base_Type* field_bt = get_at(a + start_field);
      decoded_field_length = field_bt->RAW_decode(elem_descr, buff, limit,
        top_bit_ord, TRUE);
      if (decoded_field_length < 0) return decoded_field_length;
      decoded_length += decoded_field_length;
      limit -= decoded_field_length;
    }
  }
  else {
    int a = start_field;
    if (limit == 0) {
      if (!first_call) return -1;
      goto finished;
    }
    while (limit > 0) {
      start_of_field = buff.get_pos_bit();
      Base_Type* field_bt = get_at(a); // non-const, extend the record-of
      decoded_field_length = field_bt->RAW_decode(elem_descr, buff, limit,
        top_bit_ord, TRUE);
      if (decoded_field_length < 0) { // decoding failed, shorten the record-of
        set_size(get_nof_elements() - 1);
        buff.set_pos_bit(start_of_field);
        if (a > start_field) {
          goto finished;
        }
        else return decoded_field_length;
      }
      decoded_length += decoded_field_length;
      limit -= decoded_field_length;
      a++;
      if (EXT_BIT_NO != p_td.raw->extension_bit) {
        // (EXT_BIT_YES != p_td.raw->extension_bit) is 0 or 1
        // This is the opposite value of what the bit needs to be to signal
        // the end of decoding, because x-or is the equivalent of !=
        if ((EXT_BIT_YES != p_td.raw->extension_bit) ^ buff.get_last_bit()) {
          goto finished;
        }
      }
    }
  }
finished:
  return decoded_length + buff.increase_pos_padd(p_td.raw->padding) + prepaddlength;
}

int Record_Of_Type::RAW_encode(const TTCN_Typedescriptor_t& p_td, RAW_enc_tree& myleaf) const
{
  if (err_descr) return RAW_encode_negtest(err_descr, p_td, myleaf);
  int encoded_length = 0;
  int nof_elements = get_nof_elements();
  int encoded_num_of_records =
    p_td.raw->fieldlength ? (nof_elements < p_td.raw->fieldlength ? nof_elements : p_td.raw->fieldlength)
                          : nof_elements;
  myleaf.isleaf = FALSE;
  myleaf.rec_of = TRUE;
  myleaf.body.node.num_of_nodes = encoded_num_of_records;
  myleaf.body.node.nodes = init_nodes_of_enc_tree(encoded_num_of_records);
  TTCN_Typedescriptor_t const& elem_descr = *p_td.oftype_descr;
  for (int a = 0; a < encoded_num_of_records; a++) {
    const Base_Type *field_bt = get_at(a);
    myleaf.body.node.nodes[a] = new RAW_enc_tree(TRUE, &myleaf, &(myleaf.curr_pos), a, elem_descr.raw);
    encoded_length += field_bt->RAW_encode(elem_descr, *myleaf.body.node.nodes[a]);
  }
  return myleaf.length = encoded_length;
}

int Record_Of_Type::RAW_encode_negtest(const Erroneous_descriptor_t *p_err_descr,
  const TTCN_Typedescriptor_t& p_td, RAW_enc_tree& myleaf) const
{
  int values_idx = 0;
  int edescr_idx = 0;
  int nof_elements = get_nof_elements();
  // It can be more, of course...
  int encoded_num_of_records =
    p_td.raw->fieldlength ? (nof_elements < p_td.raw->fieldlength ? nof_elements : p_td.raw->fieldlength)
                          : nof_elements;
  for (int i = 0; i < nof_elements; ++i) {
    if ((p_err_descr->omit_before != -1) && (i < p_err_descr->omit_before)) {
      --encoded_num_of_records;
      continue;
    }
    const Erroneous_values_t *err_vals =
      p_err_descr->next_field_err_values(i, values_idx);
    // Not checking any further, `internal error' will be given anyway in the
    // next round.  Please note that elements can be removed, `omitted'.
    if (err_vals && err_vals->before)
      ++encoded_num_of_records;
    if (err_vals && err_vals->value && !err_vals->value->errval)
      --encoded_num_of_records;
    if (err_vals && err_vals->after)
      ++encoded_num_of_records;
    if ((p_err_descr->omit_after != -1) && (i >= p_err_descr->omit_after)) {
      encoded_num_of_records = encoded_num_of_records - (nof_elements - i) + 1;
      break;
    }
  }
  myleaf.body.node.num_of_nodes = encoded_num_of_records;
  myleaf.body.node.nodes = init_nodes_of_enc_tree(encoded_num_of_records);
  int encoded_length = 0;
  myleaf.isleaf = FALSE;
  myleaf.rec_of = TRUE;
  int node_pos = 0;
  values_idx = 0;
  for (int i = 0; i < nof_elements; ++i) {
    if ((p_err_descr->omit_before != -1) && (i < p_err_descr->omit_before))
      continue;
    const Erroneous_values_t *err_vals = p_err_descr->next_field_err_values(i, values_idx);
    const Erroneous_descriptor_t *emb_descr = p_err_descr->next_field_emb_descr(i, edescr_idx);
    TTCN_Typedescriptor_t const& elem_descr = *p_td.oftype_descr;
    if (err_vals && err_vals->before) {
      if (err_vals->before->errval == NULL)
        TTCN_error("internal error: erroneous before value missing");
      if (err_vals->before->raw) {
        myleaf.body.node.nodes[node_pos] = new RAW_enc_tree(TRUE, &myleaf,
          &(myleaf.curr_pos), node_pos,
          err_vals->before->errval->get_descriptor()->raw);
        encoded_length += err_vals->before->errval->
          RAW_encode_negtest_raw(*myleaf.body.node.nodes[node_pos++]);
      } else {
        if (err_vals->before->type_descr == NULL)
          TTCN_error("internal error: erroneous before typedescriptor missing");
        myleaf.body.node.nodes[node_pos] = new RAW_enc_tree(TRUE, &myleaf,
          &(myleaf.curr_pos), node_pos, elem_descr.raw);
        encoded_length += err_vals->before->errval->
          RAW_encode(*(err_vals->before->type_descr),
                     *myleaf.body.node.nodes[node_pos++]);
      }
    }
    if (err_vals && err_vals->value) {
      if (err_vals->value->errval) {
        if (err_vals->value->raw) {
          myleaf.body.node.nodes[node_pos] = new RAW_enc_tree(TRUE, &myleaf,
            &(myleaf.curr_pos), node_pos,
            err_vals->value->errval->get_descriptor()->raw);
          encoded_length += err_vals->value->errval->
            RAW_encode_negtest_raw(*myleaf.body.node.nodes[node_pos++]);
        } else {
          if (err_vals->value->type_descr == NULL)
            TTCN_error("internal error: erroneous value typedescriptor missing");
          myleaf.body.node.nodes[node_pos] = new RAW_enc_tree(TRUE, &myleaf,
            &(myleaf.curr_pos), node_pos, elem_descr.raw);
          encoded_length += err_vals->value->errval->
            RAW_encode(*(err_vals->value->type_descr),
                       *myleaf.body.node.nodes[node_pos++]);
        }
      } // else -> omit
    } else {
      if (emb_descr) {
        myleaf.body.node.nodes[node_pos] = new RAW_enc_tree(TRUE, &myleaf,
          &(myleaf.curr_pos), node_pos, elem_descr.raw);
        encoded_length += get_at(i)->RAW_encode_negtest(emb_descr, 
          *p_td.oftype_descr, *myleaf.body.node.nodes[node_pos++]);
      } else {
        myleaf.body.node.nodes[node_pos] = new RAW_enc_tree(TRUE, &myleaf,
          &(myleaf.curr_pos), node_pos, elem_descr.raw);
        encoded_length += get_at(i)->RAW_encode(*p_td.oftype_descr, 
          *myleaf.body.node.nodes[node_pos++]);
      }
    }
    if (err_vals && err_vals->after) {
      if (err_vals->after->errval == NULL)
        TTCN_error("internal error: erroneous after value missing");
      if (err_vals->after->raw) {
        myleaf.body.node.nodes[node_pos] = new RAW_enc_tree(TRUE, &myleaf,
          &(myleaf.curr_pos), node_pos,
          err_vals->after->errval->get_descriptor()->raw);
        encoded_length += err_vals->after->errval->
          RAW_encode_negtest_raw(*myleaf.body.node.nodes[node_pos++]);
      } else {
        if (err_vals->after->type_descr == NULL)
          TTCN_error("internal error: erroneous after typedescriptor missing");
        myleaf.body.node.nodes[node_pos] = new RAW_enc_tree(TRUE, &myleaf,
          &(myleaf.curr_pos), node_pos, elem_descr.raw);
        encoded_length += err_vals->after->errval->
          RAW_encode(*(err_vals->after->type_descr),
                     *myleaf.body.node.nodes[node_pos++]);
      }
    }
    if ((p_err_descr->omit_after != -1) && (i >= p_err_descr->omit_after))
      break;
  }
  return myleaf.length = encoded_length;
}

int Record_Of_Type::JSON_encode(const TTCN_Typedescriptor_t& p_td, JSON_Tokenizer& p_tok) const
{
  if (err_descr) {
	  return JSON_encode_negtest(err_descr, p_td, p_tok);
  }
  
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound %s of value.", is_set() ? "set" : "record");
    return -1;
  }
  
  int enc_len = p_tok.put_next_token(JSON_TOKEN_ARRAY_START, NULL);
  
  for (int i = 0; i < get_nof_elements(); ++i) {
    if (NULL != p_td.json && p_td.json->metainfo_unbound && !get_at(i)->is_bound()) {
      // unbound elements are encoded as { "metainfo []" : "unbound" }
      enc_len += p_tok.put_next_token(JSON_TOKEN_OBJECT_START, NULL);
      enc_len += p_tok.put_next_token(JSON_TOKEN_NAME, "metainfo []");
      enc_len += p_tok.put_next_token(JSON_TOKEN_STRING, "\"unbound\"");
      enc_len += p_tok.put_next_token(JSON_TOKEN_OBJECT_END, NULL);
    }
    else {
      int ret_val = get_at(i)->JSON_encode(*p_td.oftype_descr, p_tok);
      if (0 > ret_val) break;
      enc_len += ret_val;
    }
  }
  
  enc_len += p_tok.put_next_token(JSON_TOKEN_ARRAY_END, NULL);
  return enc_len;
}

int Record_Of_Type::JSON_encode_negtest(const Erroneous_descriptor_t* p_err_descr,
                                        const TTCN_Typedescriptor_t& p_td,
                                        JSON_Tokenizer& p_tok) const 
{
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound %s of value.", is_set() ? "set" : "record");
    return -1;
  }
  
  int enc_len = p_tok.put_next_token(JSON_TOKEN_ARRAY_START, NULL);
  
  int values_idx = 0;
  int edescr_idx = 0;
  
  for (int i = 0; i < get_nof_elements(); ++i) {
    if (-1 != p_err_descr->omit_before && p_err_descr->omit_before > i) {
      continue;
    }
    
    const Erroneous_values_t* err_vals = p_err_descr->next_field_err_values(i, values_idx);
    const Erroneous_descriptor_t* emb_descr = p_err_descr->next_field_emb_descr(i, edescr_idx);
    
    if (NULL != err_vals && NULL != err_vals->before) {
      if (NULL == err_vals->before->errval) {
        TTCN_error("internal error: erroneous before value missing");
      }
      if (err_vals->before->raw) {
        enc_len += err_vals->before->errval->JSON_encode_negtest_raw(p_tok);
      } else {
        if (NULL == err_vals->before->type_descr) {
          TTCN_error("internal error: erroneous before typedescriptor missing");
        }
        enc_len += err_vals->before->errval->JSON_encode(*(err_vals->before->type_descr), p_tok);
      }
    }
    
    if (NULL != err_vals && NULL != err_vals->value) {
      if (NULL != err_vals->value->errval) {
        if (err_vals->value->raw) {
          enc_len += err_vals->value->errval->JSON_encode_negtest_raw(p_tok);
        } else {
          if (NULL == err_vals->value->type_descr) {
            TTCN_error("internal error: erroneous before typedescriptor missing");
          }
          enc_len += err_vals->value->errval->JSON_encode(*(err_vals->value->type_descr), p_tok);
        }
      }
    }
    else if (NULL != p_td.json && p_td.json->metainfo_unbound && !get_at(i)->is_bound()) {
      // unbound elements are encoded as { "metainfo []" : "unbound" }
      enc_len += p_tok.put_next_token(JSON_TOKEN_OBJECT_START, NULL);
      enc_len += p_tok.put_next_token(JSON_TOKEN_NAME, "metainfo []");
      enc_len += p_tok.put_next_token(JSON_TOKEN_STRING, "\"unbound\"");
      enc_len += p_tok.put_next_token(JSON_TOKEN_OBJECT_END, NULL);
    }
    else {
      int ret_val;
      if (NULL != emb_descr) {
        ret_val = get_at(i)->JSON_encode_negtest(emb_descr, *p_td.oftype_descr, p_tok);
      } else {
        ret_val = get_at(i)->JSON_encode(*p_td.oftype_descr, p_tok);
      }
      if (0 > ret_val) break;
      enc_len += ret_val;
    }
    
    if (NULL != err_vals && NULL != err_vals->after) {
      if (NULL == err_vals->after->errval) {
        TTCN_error("internal error: erroneous after value missing");
      }
      if (err_vals->after->raw) {
        enc_len += err_vals->after->errval->JSON_encode_negtest_raw(p_tok);
      } else {
        if (NULL == err_vals->after->type_descr) {
          TTCN_error("internal error: erroneous before typedescriptor missing");
        }
        enc_len += err_vals->after->errval->JSON_encode(*(err_vals->after->type_descr), p_tok);
      }
    }
    
    if (-1 != p_err_descr->omit_after && p_err_descr->omit_after <= i) {
      break;
    }
  }
  
  enc_len += p_tok.put_next_token(JSON_TOKEN_ARRAY_END, NULL);
  return enc_len;
}

int Record_Of_Type::JSON_decode(const TTCN_Typedescriptor_t& p_td, JSON_Tokenizer& p_tok, boolean p_silent)
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
  
  set_size(0);
  for (int nof_elements = 0; TRUE; ++nof_elements) {
    // Read value tokens until we reach some other token
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
    Base_Type* val = create_elem();
    int ret_val2 = val->JSON_decode(*p_td.oftype_descr, p_tok, p_silent);
    if (JSON_ERROR_INVALID_TOKEN == ret_val2) {
      // undo the last action on the buffer
      p_tok.set_buf_pos(buf_pos);
      delete val;
      break;
    } 
    else if (JSON_ERROR_FATAL == ret_val2) {
      delete val;
      if (p_silent) {
        clean_up();
      }
      return JSON_ERROR_FATAL;
    }
    if (NULL == refd_ind_ptr) {
      val_ptr->value_elements = (Base_Type**)reallocate_pointers(
        (void**)val_ptr->value_elements, val_ptr->n_elements, nof_elements + 1);
      val_ptr->value_elements[nof_elements] = val;
      val_ptr->n_elements = nof_elements + 1;
    }
    else {
      get_at(nof_elements)->set_value(val);
      delete val;
    }
    dec_len += (size_t)ret_val2;
  }
  
  dec_len += p_tok.get_next_token(&token, NULL, NULL);
  if (JSON_TOKEN_ARRAY_END != token) {
    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_REC_OF_END_TOKEN_ERROR, "");
    if (p_silent) {
      clean_up();
    }
    return JSON_ERROR_FATAL;
  }
  
  return (int)dec_len;
}

void Record_Of_Type::encode(const TTCN_Typedescriptor_t& p_td,
  TTCN_Buffer& p_buf, TTCN_EncDec::coding_t p_coding, ...) const
{
  va_list pvar;
  va_start(pvar, p_coding);
  switch(p_coding) {
  case TTCN_EncDec::CT_BER: {
    TTCN_EncDec_ErrorContext ec("While BER-encoding type '%s': ", p_td.name);
    unsigned BER_coding=va_arg(pvar, unsigned);
    BER_encode_chk_coding(BER_coding);
    ASN_BER_TLV_t *tlv=BER_encode_TLV(p_td, BER_coding);
    tlv->put_in_buffer(p_buf);
    ASN_BER_TLV_t::destruct(tlv);
    break;}
  case TTCN_EncDec::CT_RAW: {
    TTCN_EncDec_ErrorContext ec("While RAW-encoding type '%s': ", p_td.name);
    if (!p_td.raw)
      TTCN_EncDec_ErrorContext::error_internal("No RAW descriptor available for type '%s'.", p_td.name);
    RAW_enc_tr_pos rp;
    rp.level = 0;
    rp.pos = NULL;
    RAW_enc_tree root(FALSE, NULL, &rp, 1, p_td.raw);
    RAW_encode(p_td, root);
    root.put_to_buf(p_buf);
    break; }
  case TTCN_EncDec::CT_TEXT: {
    TTCN_EncDec_ErrorContext ec("While TEXT-encoding type '%s': ", p_td.name);
    if(!p_td.text) TTCN_EncDec_ErrorContext::error_internal
                     ("No TEXT descriptor available for type '%s'.", p_td.name);
    TEXT_encode(p_td,p_buf);
    break;}
  case TTCN_EncDec::CT_XER: {
    TTCN_EncDec_ErrorContext ec("While XER-encoding type '%s': ", p_td.name);
    unsigned XER_coding=va_arg(pvar, unsigned);
    XER_encode(*(p_td.xer),p_buf, XER_coding, 0, 0, 0);
    p_buf.put_c('\n');
    break;}
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

void Record_Of_Type::decode(const TTCN_Typedescriptor_t& p_td,
  TTCN_Buffer& p_buf, TTCN_EncDec::coding_t p_coding, ...)
{
  va_list pvar;
  va_start(pvar, p_coding);
  switch(p_coding) {
  case TTCN_EncDec::CT_BER: {
    TTCN_EncDec_ErrorContext ec("While BER-decoding type '%s': ", p_td.name);
    unsigned L_form=va_arg(pvar, unsigned);
    ASN_BER_TLV_t tlv;
    BER_decode_str2TLV(p_buf, tlv, L_form);
    BER_decode_TLV(p_td, tlv, L_form);
    if(tlv.isComplete) p_buf.increase_pos(tlv.get_len());
    break;}
  case TTCN_EncDec::CT_RAW: {
    TTCN_EncDec_ErrorContext ec("While RAW-decoding type '%s': ", p_td.name);
    if(!p_td.raw) TTCN_EncDec_ErrorContext::error_internal
                    ("No RAW descriptor available for type '%s'.", p_td.name);
    raw_order_t order;
    switch(p_td.raw->top_bit_order) {
    case TOP_BIT_LEFT:
      order=ORDER_LSB;
      break;
    case TOP_BIT_RIGHT:
    default:
      order=ORDER_MSB;
    }
    if(RAW_decode(p_td, p_buf, p_buf.get_len()*8, order)<0)
      ec.error(TTCN_EncDec::ET_INCOMPL_MSG,"Can not decode type '%s', "
                 "because invalid or incomplete message was received", p_td.name);
      break;}
  case TTCN_EncDec::CT_TEXT: {
    Limit_Token_List limit;
    TTCN_EncDec_ErrorContext ec("While TEXT-decoding type '%s': ", p_td.name);
    if(!p_td.text) TTCN_EncDec_ErrorContext::error_internal
                     ("No TEXT descriptor available for type '%s'.", p_td.name);
    const unsigned char *b=p_buf.get_data();
    if(b[p_buf.get_len()-1]!='\0'){
      p_buf.set_pos(p_buf.get_len());
      p_buf.put_zero(8,ORDER_LSB);
      p_buf.rewind();
    }
    if(TEXT_decode(p_td,p_buf,limit)<0)
      ec.error(TTCN_EncDec::ET_INCOMPL_MSG,"Can not decode type '%s', "
        "because invalid or incomplete message was received", p_td.name);
    break;}
  case TTCN_EncDec::CT_XER: {
    TTCN_EncDec_ErrorContext ec("While XER-decoding type '%s': ", p_td.name);
    unsigned XER_coding=va_arg(pvar, unsigned);
    XmlReaderWrap reader(p_buf);
    for (int success=reader.Read(); success==1; success=reader.Read()) {
      if (reader.NodeType() == XML_READER_TYPE_ELEMENT) break;
    }
    XER_decode(*(p_td.xer), reader, XER_coding | XER_TOPLEVEL, XER_NONE, 0);
    size_t bytes = reader.ByteConsumed();
    p_buf.set_pos(bytes);
    break;}
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
  va_end(pvar);
}

char **Record_Of_Type::collect_ns(const XERdescriptor_t& p_td, size_t& num, bool& def_ns, unsigned int flavor) const
{
  size_t num_collected = 0;
  // First, our own namespace. Sets num_collected to 0 or 1.
  // If it throws, nothing was allocated.
  char **collected_ns = Base_Type::collect_ns(p_td, num_collected, def_ns, flavor);

  // Then the embedded type
  try {
    boolean def_ns_1 = FALSE;
    if (val_ptr) for (int i = 0; i < get_nof_elements(); ++i) {
      size_t num_new = 0;
      char **new_namespaces = get_at(i)->collect_ns(
        *p_td.oftype_descr, num_new, def_ns_1, flavor);
      merge_ns(collected_ns, num_collected, new_namespaces, num_new);
      def_ns = def_ns || def_ns_1; // alas, no ||=
    }
  }
  catch (...) {
    // Probably a TC_Error thrown from the element's collect_ns(),
    // e.g. if encoding an unbound value.
    while (num_collected > 0) Free(collected_ns[--num_collected]);
    Free(collected_ns);
    throw;
  }

  num = num_collected;
  return collected_ns;
}

static const universal_char sp = { 0,0,0,' ' };
static const universal_char tb = { 0,0,0,9 };

int Record_Of_Type::XER_encode(const XERdescriptor_t& p_td, TTCN_Buffer& p_buf,
  unsigned int flavor, unsigned int flavor2, int indent, embed_values_enc_struct_t* emb_val) const
{
  if (err_descr) {
    return XER_encode_negtest(err_descr, p_td, p_buf, flavor, flavor2, indent, emb_val);
  }

  if (val_ptr == 0) TTCN_error(
    "Attempt to XER-encode an unbound record of type %s", get_descriptor()->name);
  int encoded_length = (int)p_buf.get_len();

  const int exer = is_exer(flavor);
  const boolean own_tag =
    !(exer && indent && (p_td.xer_bits & (ANY_ELEMENT|ANY_ATTRIBUTES|UNTAGGED)));

  const int indenting = !is_canonical(flavor) && own_tag;
  const boolean xmlValueList = isXmlValueList();

  flavor = flavor
    | ( (exer && (p_td.xer_bits & XER_LIST))
      || is_exerlist(flavor) ? SIMPLE_TYPE : XER_NONE /* = 0 */);
  flavor &= ~XER_RECOF; // record-of doesn't care
  int nof_elements = get_nof_elements();
  Base_Type::begin_xml(p_td, p_buf, flavor, indent, !nof_elements,
    (collector_fn)&Record_Of_Type::collect_ns);

  if (xmlValueList && nof_elements && indenting && !exer) { /* !exer or GDMO */
    do_indent(p_buf, indent+1);
  }

  if (exer && (p_td.xer_bits & ANY_ATTRIBUTES)) {
    // Back up over the '>' and the '\n' that may follow it
    size_t buf_len = p_buf.get_len(), shorter = 0;
    const unsigned char * const buf_data = p_buf.get_data();
    if (buf_data[buf_len - 1 - shorter] == '\n') ++shorter;
    if (buf_data[buf_len - 1 - shorter] == '>' ) ++shorter;

    unsigned char saved [4];
    if (shorter) {
      memcpy(saved, buf_data + (buf_len - shorter), shorter);
      p_buf.increase_length(-shorter);
    }

    // ANY_ATTRIBUTES means it's a record of universal charstring.
    // They are in AnyAttributeFormat (X.693/2008 18.2.6):
    // "URI(optional), space, NCName, equals, \"xmlcstring\""
    // They need to be written as an XML attribute and namespace declaration:
    // xmlns:b0="URI" b0:NCName="xmlcstring"
    //
    for (int i = 0; i < nof_elements; ++i) {
      TTCN_EncDec_ErrorContext ec_0("Attribute %d: ", i);
      if (!is_elem_bound(i)) {
        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
          "Encoding an unbound universal charstring value.");
        continue;
      }
      const UNIVERSAL_CHARSTRING *elem
      = static_cast<const UNIVERSAL_CHARSTRING *>(val_ptr->value_elements[i]);
      size_t len = elem->lengthof();
      for (;;) {
        const UNIVERSAL_CHARSTRING_ELEMENT& ue = (*elem)[len - 1];
        if (sp == ue || tb == ue) --len;
        else break;
      }
      // sp_at: indexes the first space
      // j is left to point at where the attribute name begins (just past the space)
      size_t j, sp_at = 0;
      for (j = 0; j < len; j++) {
        const UNIVERSAL_CHARSTRING_ELEMENT& ue = (*elem)[j];
        if (sp_at) { // already found a space
          if (sp == ue || tb == ue) {} // another space, do nothing
          else break; // found a non-space after a space
        }
        else {
          if (sp == ue || tb == ue) sp_at = j;
        }
      } // next j

      size_t buf_start = p_buf.get_len();
      if (sp_at > 0) {
        char * ns = mprintf(" xmlns:b%d='", i);
        size_t ns_len = mstrlen(ns);
        p_buf.put_s(ns_len, (cbyte*)ns);

        UNIVERSAL_CHARSTRING before(sp_at, (const universal_char*)(*elem));
        
        before.XER_encode(UNIVERSAL_CHARSTRING_xer_, p_buf,
          flavor | ANY_ATTRIBUTES, flavor2, indent, 0);

        p_buf.put_c('\'');
        p_buf.put_c(' ');

        // Keep just the "b%d" part from ns
        p_buf.put_s(ns_len - 9, (cbyte*)ns + 7);
        p_buf.put_c(':');
        Free(ns);
        
        if (p_td.xer_bits & (ANY_FROM | ANY_EXCEPT)) {
          // Ensure the namespace abides to its restrictions
          TTCN_Buffer ns_buf;
          before.encode_utf8(ns_buf);
          CHARSTRING cs;
          ns_buf.get_string(cs);
          check_namespace_restrictions(p_td, (const char*)cs);
        }
      }
      else {
        p_buf.put_c(' ');
        j = 0;
        
        if (p_td.xer_bits & (ANY_FROM | ANY_EXCEPT)) {
          // Make sure the unqualified namespace is allowed
          check_namespace_restrictions(p_td, NULL);
        }
      }

      UNIVERSAL_CHARSTRING after(len - j, (const universal_char*)(*elem) + j);
      after.XER_encode(UNIVERSAL_CHARSTRING_xer_, p_buf,
        flavor | ANY_ATTRIBUTES, flavor2, indent, 0);
      
      // Put this attribute in a dummy element and walk through it to check its validity
      TTCN_Buffer check_buf;
      check_buf.put_s(2, (const unsigned char*)"<a");
      check_buf.put_s(p_buf.get_len() - buf_start, p_buf.get_data() + buf_start);
      check_buf.put_s(2, (const unsigned char*)"/>");
      XmlReaderWrap checker(check_buf);
      while (1 == checker.Read());
    }
    if (shorter) {
      p_buf.put_s(shorter, saved); // restore the '>' and anything after
    }
  }
  else { // not ANY-ATTRIBUTES
    unsigned int sub_flavor = flavor | XER_RECOF | (p_td.xer_bits & (XER_LIST));
    TTCN_EncDec_ErrorContext ec_0("Index ");
    TTCN_EncDec_ErrorContext ec_1;

    for (int i = 0; i < nof_elements; ++i) {
      if (i > 0 && !own_tag && 0 != emb_val &&
          emb_val->embval_index < emb_val->embval_array->size_of()) {
        emb_val->embval_array->get_at(emb_val->embval_index)->XER_encode(
          UNIVERSAL_CHARSTRING_xer_, p_buf, flavor | EMBED_VALUES, flavor2, indent+1, 0);
        ++emb_val->embval_index;
      }
      ec_1.set_msg("%d: ", i);
      if (exer && (p_td.xer_bits & XER_LIST) && i>0) p_buf.put_c(' ');
      get_at(i)->XER_encode(*p_td.oftype_descr, p_buf,
        sub_flavor, flavor2, indent+own_tag, emb_val);
    }

    if (indenting && nof_elements && !is_exerlist(flavor)) {
      if (xmlValueList && !exer) p_buf.put_c('\n'); /* !exer or GDMO */
      //do_indent(p_buf, indent);
    }
  }

  Base_Type::end_xml(p_td, p_buf, flavor, indent, !nof_elements);
  return (int)p_buf.get_len() - encoded_length;
}

// XERSTUFF Record_Of_Type::encode_element
/** Helper for Record_Of_Type::XER_encode_negtest
 *
 * The main purpose of this method is to allow another type to request
 * encoding of a single element of the record-of. Used by Record_Type
 * to encode individual strings of the EMBED-VALUES member.
 *
 * @param i index of the element
 * @param ev erroneous descriptor for the element itself
 * @param ed deeper erroneous values
 * @param p_buf buffer containing the encoded value
 * @param sub_flavor flags
 * @param indent indentation level
 * @return number of bytes generated
 */
int Record_Of_Type::encode_element(int i, const XERdescriptor_t& p_td,
  const Erroneous_values_t* ev, const Erroneous_descriptor_t* ed,
  TTCN_Buffer& p_buf, unsigned int sub_flavor, unsigned int flavor2, int indent, embed_values_enc_struct_t* emb_val) const
{
  int enc_len = p_buf.get_len();
  TTCN_EncDec_ErrorContext ec;
  const int exer = is_exer(sub_flavor);

  if (ev && ev->before) {
    if (ev->before->errval==NULL) {
      TTCN_error("internal error: erroneous before value missing");
    }
    ec.set_msg("Erroneous value before component #%d: ", i);
    if (ev->before->raw) {
      ev->before->errval->encode_raw(p_buf);
    } else {
      if (ev->before->type_descr==NULL) TTCN_error(
        "internal error: erroneous before type descriptor missing");
      ev->before->errval->XER_encode(*ev->before->type_descr->xer,
        p_buf, sub_flavor, flavor2, indent, 0);
    }
  }

  if (exer && (sub_flavor & XER_LIST)
    && (i > 0 || (ev && ev->before && !ev->before->raw))){
    // Ensure a separator is written after the "erroneous before"
    // of the first element (except for "raw before").
    p_buf.put_c(' ');
  }

  if (ev && ev->value) {
    if (ev->value->errval) { // replace
      ec.set_msg("Erroneous value for component #%d: ", i);
      if (ev->value->raw) {
        ev->value->errval->encode_raw(p_buf);
      } else {
        if (ev->value->type_descr==NULL) TTCN_error(
          "internal error: erroneous value type descriptor missing");
        ev->value->errval->XER_encode(*ev->value->type_descr->xer,
          p_buf, sub_flavor, flavor2, indent, 0);
      }
    } // else -> omit
  } else {
    ec.set_msg("Component #%d: ", i);
    if (ed) {
      get_at(i)->XER_encode_negtest(ed, p_td, p_buf, sub_flavor, flavor2, indent, emb_val);
    } else {
      // the "real" encoder
      get_at(i)->XER_encode(p_td, p_buf, sub_flavor, flavor2, indent, emb_val);
    }
  }

  if (ev && ev->after) {
    if (ev->after->errval==NULL) {
      TTCN_error("internal error: erroneous after value missing");
    }
    ec.set_msg("Erroneous value after component #%d: ", i);
    if (ev->after->raw) {
      ev->after->errval->encode_raw(p_buf);
    } else {
      if (ev->after->type_descr==NULL) TTCN_error(
        "internal error: erroneous after type descriptor missing");
      ev->after->errval->XER_encode(*ev->after->type_descr->xer,
        p_buf, sub_flavor, flavor2, indent, 0);
    }
  }

  return enc_len;
}

// XERSTUFF Record_Of_Type::XER_encode_negtest
int Record_Of_Type::XER_encode_negtest(const Erroneous_descriptor_t* p_err_descr,
  const XERdescriptor_t& p_td, TTCN_Buffer& p_buf, unsigned flavor, unsigned int flavor2, int indent,
  embed_values_enc_struct_t* emb_val) const
{
  if (val_ptr == 0) TTCN_error(
    "Attempt to XER-encode an unbound record of type %s", get_descriptor()->name);
  int encoded_length = (int)p_buf.get_len();

  const int exer = is_exer(flavor);
  const boolean own_tag =
    !(exer && indent && (p_td.xer_bits & (ANY_ELEMENT|ANY_ATTRIBUTES|UNTAGGED)));

  const int indenting = !is_canonical(flavor) && own_tag;
  const boolean xmlValueList = isXmlValueList();

  flavor = flavor
    | ( (exer && (p_td.xer_bits & XER_LIST))
      || is_exerlist(flavor) ? SIMPLE_TYPE : XER_NONE /* = 0 */);
  flavor &= ~XER_RECOF; // record-of doesn't care
  int nof_elements = get_nof_elements();
  Base_Type::begin_xml(p_td, p_buf, flavor, indent, !nof_elements,
    (collector_fn)&Record_Of_Type::collect_ns);

  if (xmlValueList && nof_elements && indenting && !exer) { /* !exer or GDMO */
    do_indent(p_buf, indent+1);
  }

  int values_idx = 0;
  int edescr_idx = 0;
  if (exer && (p_td.xer_bits & ANY_ATTRIBUTES)) {
    // Back up over the '>' and the '\n' that may follow it
    size_t buf_len = p_buf.get_len(), shorter = 0;
    const unsigned char * const buf_data = p_buf.get_data();
    if (buf_data[buf_len - 1 - shorter] == '\n') ++shorter;
    if (buf_data[buf_len - 1 - shorter] == '>' ) ++shorter;

    unsigned char * saved = 0;
    if (shorter) {
      saved = new unsigned char[shorter];
      memcpy(saved, buf_data + (buf_len - shorter), shorter);
      p_buf.increase_length(-shorter);
    }

    // ANY_ATTRIBUTES means it's a record of universal charstring.
    // They are in AnyAttributeFormat (X.693/2008 18.2.6):
    // "URI(optional), space, NCName, equals, \"xmlcstring\""
    // They need to be written as an XML attribute and namespace declaration:
    // xmlns:b0="URI" b0:NCName="xmlcstring"
    //
    for (int i = 0; i < nof_elements; ++i) {
      if (i < p_err_descr->omit_before) continue;

      const Erroneous_values_t     *ev = p_err_descr->next_field_err_values(i, values_idx);
      const Erroneous_descriptor_t *ed = p_err_descr->next_field_emb_descr(i, edescr_idx);

      if (ev && ev->before) {
        if (ev->before->errval==NULL) TTCN_error("internal error: erroneous value missing");

        // ec.set_msg
        if (ev->before->raw) ev->before->errval->encode_raw(p_buf);
        else {
          if (ev->before->type_descr==NULL) TTCN_error(
            "internal error: erroneous before type descriptor missing");
          else ev->before->errval->XER_encode(*ev->before->type_descr->xer,
            p_buf, flavor, flavor2, indent, 0);
        }
      }

      if (ev && ev->value) { //value replacement
        if (ev->value->errval) {
          if (ev->value->raw) ev->value->errval->encode_raw(p_buf);
          else {
            if (ev->value->type_descr==NULL) TTCN_error(
              "internal error: erroneous value type descriptor missing");
            else ev->value->errval->XER_encode(*ev->value->type_descr->xer,
              p_buf, flavor, flavor2, indent, 0);
          }
        }
      }
      else {
        if (ed) {
          // embedded descr.. call negtest (except UNIVERSAL_CHARSTRING
          // doesn't have XER_encode_negtest)
          TTCN_error("internal error: embedded descriptor for scalar");
        }
        else {
          // the original encoding
          const UNIVERSAL_CHARSTRING *elem
          = static_cast<const UNIVERSAL_CHARSTRING *>(get_at(i));
          size_t len = elem->lengthof();
          for (;;) {
            const UNIVERSAL_CHARSTRING_ELEMENT& ue = (*elem)[len - 1];
            if (sp == ue || tb == ue) --len;
            else break;
          }
          // sp_at: indexes the first space
          // j is left to point at where the attribute name begins (just past the space)
          size_t j, sp_at = 0;
          for (j = 0; j < len; j++) {
            const UNIVERSAL_CHARSTRING_ELEMENT& ue = (*elem)[j];
            if (sp_at) { // already found a space
              if (sp == ue || tb == ue) {} // another space, do nothing
              else break; // found a non-space after a space
            }
            else {
              if (sp == ue || tb == ue) sp_at = j;
            }
          } // next j

          if (sp_at > 0) {
            char * ns = mprintf(" xmlns:b%d='", i);
            size_t ns_len = mstrlen(ns);
            p_buf.put_s(ns_len, (cbyte*)ns);

            UNIVERSAL_CHARSTRING before(sp_at, (const universal_char*)(*elem));
            before.XER_encode(UNIVERSAL_CHARSTRING_xer_, p_buf,
              flavor | ANY_ATTRIBUTES, flavor2, indent, 0);

            p_buf.put_c('\'');
            p_buf.put_c(' ');

            // Keep just the "b%d" part from ns
            p_buf.put_s(ns_len - 9, (cbyte*)ns + 7);
            p_buf.put_c(':');
            Free(ns);
          }
          else {
            p_buf.put_c(' ');
            j = 0;
          }

          UNIVERSAL_CHARSTRING after(len - j, (const universal_char*)(*elem) + j);
          after.XER_encode(UNIVERSAL_CHARSTRING_xer_, p_buf,
            flavor | ANY_ATTRIBUTES, flavor2, indent, 0);
        }
      }

      if (ev && ev->after) {
        if (ev->after->errval==NULL) TTCN_error(
          "internal error: erroneous after value missing");
        else {
          if (ev->after->raw) ev->after->errval->encode_raw(p_buf);
          else {
            if (ev->after->type_descr==NULL) TTCN_error(
              "internal error: erroneous after type descriptor missing");
            else ev->after->errval->XER_encode(*ev->after->type_descr->xer,
              p_buf, flavor, flavor2, indent, 0);
          }
        }
      }
      // omit_after value -1 becomes "very big"
      if ((unsigned int)i >= (unsigned int)p_err_descr->omit_after) break;
    }
    if (shorter) {
      p_buf.put_s(shorter, saved); // restore the '>' and anything after
      delete[] saved;
    }
  }
  else { // not ANY-ATTRIBUTES
    unsigned int sub_flavor = flavor | XER_RECOF | (p_td.xer_bits & (XER_LIST|ANY_ATTRIBUTES));

    TTCN_EncDec_ErrorContext ec;

    for (int i = 0; i < nof_elements; ++i) {
      if (i < p_err_descr->omit_before) continue;
      
      if (0 != emb_val && i > 0 && !own_tag &&
          emb_val->embval_index < emb_val->embval_array->size_of()) {
        const Erroneous_values_t    * ev0_i = NULL;
        const Erroneous_descriptor_t* ed0_i = NULL;
        if (emb_val->embval_err) {
          ev0_i = emb_val->embval_err->next_field_err_values(emb_val->embval_index, emb_val->embval_err_val_idx);
          ed0_i = emb_val->embval_err->next_field_emb_descr (emb_val->embval_index, emb_val->embval_err_descr_idx);
        }
        emb_val->embval_array->encode_element(emb_val->embval_index, UNIVERSAL_CHARSTRING_xer_,
          ev0_i, ed0_i, p_buf, flavor | EMBED_VALUES, flavor2, indent + own_tag, 0);
        ++emb_val->embval_index;
      }

      const Erroneous_values_t*     err_vals =
        p_err_descr->next_field_err_values(i, values_idx);
      const Erroneous_descriptor_t* emb_descr =
        p_err_descr->next_field_emb_descr (i, edescr_idx);

      encode_element(i, *p_td.oftype_descr, err_vals, emb_descr, p_buf, sub_flavor, flavor2, indent+own_tag, emb_val);

      // omit_after value -1 becomes "very big"
      if ((unsigned int)i >= (unsigned int)p_err_descr->omit_after) break;
    }

    if (indenting && nof_elements && !is_exerlist(flavor)) {
      if (xmlValueList && !exer) p_buf.put_c('\n'); /* !exer or GDMO */
      //do_indent(p_buf, indent);
    }
  }

  Base_Type::end_xml(p_td, p_buf, flavor, indent, !nof_elements);
  return (int)p_buf.get_len() - encoded_length;
}

int Record_Of_Type::XER_decode(const XERdescriptor_t& p_td,
  XmlReaderWrap& reader, unsigned int flavor, unsigned int flavor2, embed_values_dec_struct_t* emb_val)
{
  boolean exer = is_exer(flavor);
  unsigned long xerbits = p_td.xer_bits;
  if (flavor & XER_TOPLEVEL) xerbits &= ~UNTAGGED;
  boolean own_tag =
    !(exer && ((xerbits & (ANY_ELEMENT | ANY_ATTRIBUTES | UNTAGGED))
    || (flavor & USE_TYPE_ATTR))); /* incase the parent has USE-UNION */
  /* not toplevel anymore and remove the flags for USE-UNION the oftype doesn't need them */
  flavor &= ~XER_TOPLEVEL & ~XER_LIST & ~USE_TYPE_ATTR;
  int success=1, depth=-1;
  set_val(NULL_VALUE); // empty but initialized array, val_ptr != NULL
  int type;
  if (own_tag) for (success = 1; success == 1; success = reader.Read()) {
    type = reader.NodeType();
    if (exer && (p_td.xer_bits & XER_ATTRIBUTE)) {
      if (XML_READER_TYPE_ATTRIBUTE == type) break;
    }
    if (exer && (p_td.xer_bits & XER_LIST)) {
      if (XML_READER_TYPE_TEXT == type) break;
    }
    else {
      if (XML_READER_TYPE_ELEMENT == type) {
        verify_name(reader, p_td, exer);
        depth = reader.Depth();
        break;
      }
    } /* endif(exer && list) */
  } /* next read */
  else depth = reader.Depth();
  TTCN_EncDec_ErrorContext ec_0("Index ");
  TTCN_EncDec_ErrorContext ec_1;
  flavor |= XER_RECOF;
  if (exer && (p_td.xer_bits & ANY_ATTRIBUTES)) {
    // The enclosing type should handle the decoding.
    TTCN_error("Incorrect decoding of ANY-ATTRIBUTES");
  }
  else if (exer && (p_td.xer_bits & XER_LIST)) { /* LIST decoding*/
    char *val = (char*)reader.NewValue(); /* we own it */
    size_t pos = 0;
    size_t len = strlen(val);
    /* The string contains a bunch of values separated by whitespace.
     * Tokenize the string and create a new buffer which looks like
     * an XML element (<ns:name xmlns:ns='uri'>value</ns:name>), then use that
     * to decode the value. */
    for(char * str = strtok(val, " \t\x0A\x0D"); str != 0; str = strtok(val + pos, " \t\x0A\x0D")) {
      // Calling strtok with NULL won't work here, since the decoded element can have strtok calls aswell
      pos += strlen(str) + 1;
      // Construct a new XML Reader with the current token.
      TTCN_Buffer buf2;
      const XERdescriptor_t& sub_xer = *p_td.oftype_descr;
      buf2.put_c('<');
      write_ns_prefix(sub_xer, buf2);

      boolean i_can_has_ns = sub_xer.my_module != 0 && sub_xer.ns_index != -1;
      const char * const exer_name = sub_xer.names[1];
      buf2.put_s((size_t)sub_xer.namelens[1]-1-i_can_has_ns, (cbyte*)exer_name);
      if (i_can_has_ns) {
        const namespace_t * const pns = sub_xer.my_module->get_ns(sub_xer.ns_index);
        buf2.put_s(7 - (*pns->px == 0), (cbyte*)" xmlns:");
        buf2.put_s(strlen(pns->px), (cbyte*)pns->px);
        buf2.put_s(2, (cbyte*)"='");
        buf2.put_s(strlen(pns->ns), (cbyte*)pns->ns);
        buf2.put_s(2, (cbyte*)"'>");
      }
      // start tag completed
      buf2.put_s(strlen(str), (cbyte*)str);

      buf2.put_c('<');
      buf2.put_c('/');
      write_ns_prefix(sub_xer, buf2);
      buf2.put_s((size_t)sub_xer.namelens[1], (cbyte*)exer_name);
      XmlReaderWrap reader2(buf2);
      reader2.Read(); // Move to the start element.
      // Don't move to the #text, that's the callee's responsibility.
      ec_1.set_msg("%d: ", get_nof_elements());
      // The call to the non-const operator[], I mean get_at(), creates
      // a new element (because it is indexing one past the last element).
      // Then we call its XER_decode with the temporary XML reader.
      get_at(get_nof_elements())->XER_decode(sub_xer, reader2, flavor, flavor2, 0);
      if (flavor & EXIT_ON_ERROR && !is_elem_bound(get_nof_elements() - 1)) {
        if (1 == get_nof_elements()) {
          // Failed to decode even the first element
          clean_up();
        } else {
          // Some elements were successfully decoded -> only delete the last one
          set_size(get_nof_elements() - 1);
        }
        xmlFree(val);
        return -1;
      }
      if (pos >= len) break;
    }
    xmlFree(val);
    if (p_td.xer_bits & XER_ATTRIBUTE) {
      //Let the caller do reader.AdvanceAttribute();
    }
    else if (own_tag) {
      reader.Read(); // on closing tag
      reader.Read(); // past it
    }
  }
  else { // not LIST
    if (flavor & PARENT_CLOSED) {
      // Nothing to do. We are probably untagged; do not advance in the XML
      // because it would move past the parent.
    }
    else if (own_tag && reader.IsEmptyElement()) { // Nothing to do
      reader.Read(); // This is our own empty tag, move past it
    }
    else {
      /* Note: there is no reader.Read() at the end of the loop below.
       * Each element is supposed to consume enough to leave the next element
       * well-positioned. */
      for (success = own_tag ? reader.Read() : reader.Ok(); success == 1; ) {
        type = reader.NodeType();
        if (XML_READER_TYPE_ELEMENT == type)
        {
          if (exer && (p_td.xer_bits & ANY_ELEMENT)) {
            /* This is a (record-of UNIVERSAL_CHARSTRING) with ANY-ELEMENT.
             * The ANY-ELEMENT is really meant for the element type,
             * so behave like a record-of (string with ANY-ELEMENT):
             * call the non-const operator[], I mean get_at(), to create
             * a new element, then read the entire XML element into it. */
            UNIVERSAL_CHARSTRING* uc =
              static_cast<UNIVERSAL_CHARSTRING*>(get_at(val_ptr->n_elements));
            const xmlChar* outer = reader.ReadOuterXml();
            uc->decode_utf8(strlen((const char*)outer), outer);
            // consume the element
            for (success = reader.Read(); success == 1 && reader.Depth() > depth; success = reader.Read()) {}
            if (reader.NodeType() != XML_READER_TYPE_ELEMENT) success = reader.Read(); // one last time
          }
          else {
            /* If this is an untagged record-of and the start element does not
             * belong to the embedded type, the record-of has already ended. */
            if (!own_tag && !can_start_v(
              (const char*)reader.LocalName(), (const char*)reader.NamespaceUri(),
              p_td, flavor | UNTAGGED, flavor2))
            {
              for (; success == 1 && reader.Depth() > depth; success = reader.Read()) ;
              // We should now be back at the same depth as we started.
              break;
            }
            ec_1.set_msg("%d: ", get_nof_elements());
            /* The call to the non-const get_at() creates the element */
            get_at(get_nof_elements())->XER_decode(*p_td.oftype_descr, reader, flavor, flavor2, emb_val);
            if (get_at(get_nof_elements()-1)->is_bound()) {
              flavor &= ~XER_OPTIONAL;
            }
          }
          if (0 != emb_val && !own_tag && get_nof_elements() > 1 && !(p_td.oftype_descr->xer_bits & UNTAGGED)) {
            ++emb_val->embval_index;
          }
        }
        else if (XML_READER_TYPE_END_ELEMENT == type) {
          for (; success == 1 && reader.Depth() > depth; success = reader.Read()) ;
          // If the depth just decreased, this must be an end element
          // (but a different one from what we had before the loop)
          if (own_tag) {
            verify_end(reader, p_td, depth, exer);
            reader.Read(); // move forward one last time
          }
          break;
        } 
        else if (XML_READER_TYPE_TEXT == type && 0 != emb_val && !own_tag && get_nof_elements() > 0) {
          UNIVERSAL_CHARSTRING emb_ustr((const char*)reader.Value());
          emb_val->embval_array->get_at(emb_val->embval_index)->set_value(&emb_ustr);
          success = reader.Read();
          if (p_td.oftype_descr->xer_bits & UNTAGGED) ++emb_val->embval_index;
        }
        else {
          success = reader.Read();
        }
      } /* next read */
    } /* if not empty element */
  } /* if not LIST */
  if (!own_tag && exer && (p_td.xer_bits & XER_OPTIONAL) && get_nof_elements() == 0) {
    // set it to unbound, so the OPTIONAL class sets it to omit
    clean_up();
  }
  return 1; // decode successful
}

void Record_Of_Type::set_param(Module_Param& param) {
  if (dynamic_cast<Module_Param_Name*>(param.get_id()) != NULL &&
      param.get_id()->next_name()) {
    // Haven't reached the end of the module parameter name
    // => the name refers to one of the elements, not to the whole record of
    char* param_field = param.get_id()->get_current_name();
    if (param_field[0] < '0' || param_field[0] > '9') {
      param.error("Unexpected record field name in module parameter, expected a valid"
        " index for %s type `%s'", is_set() ? "set of" : "record of", get_descriptor()->name);
    }
    int param_index = -1;
    sscanf(param_field, "%d", &param_index);
    get_at(param_index)->set_param(param);
    return;
  }
  
  param.basic_check(Module_Param::BC_VALUE|Module_Param::BC_LIST, is_set()?"set of value":"record of value");
  
  Module_Param_Ptr mp = &param;
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
  
  switch (param.get_operation_type()) {
  case Module_Param::OT_ASSIGN:
    if (mp->get_type()==Module_Param::MP_Value_List && mp->get_size()==0) {
      set_val(NULL_VALUE);
      return;
    }
    switch (mp->get_type()) {
    case Module_Param::MP_Value_List:
      set_size(mp->get_size());
      for (size_t i=0; i<mp->get_size(); ++i) {
        Module_Param* const curr = mp->get_elem(i);
        if (curr->get_type()!=Module_Param::MP_NotUsed) {
          get_at(i)->set_param(*curr);
          if (!get_at(i)->is_bound()) {
            // use null pointers for unbound elements
            delete val_ptr->value_elements[i];
            val_ptr->value_elements[i] = NULL;
          }
        }
      }
      break;
    case Module_Param::MP_Indexed_List:
      for (size_t i=0; i<mp->get_size(); ++i) {
        Module_Param* const current = mp->get_elem(i);
        get_at(current->get_id()->get_index())->set_param(*current);
        if (!get_at(current->get_id()->get_index())->is_bound()) {
          // use null pointers for unbound elements
          delete val_ptr->value_elements[current->get_id()->get_index()];
          val_ptr->value_elements[current->get_id()->get_index()] = NULL;
        }
      }
      break;
    default:
      param.type_error(is_set()?"set of value":"record of value", get_descriptor()->name);
    }
    break;
  case Module_Param::OT_CONCAT:
    switch (mp->get_type()) {
    case Module_Param::MP_Value_List: {
      if (!is_bound()) set_val(NULL_VALUE);
      int start_idx = lengthof();
      for (size_t i=0; i<mp->get_size(); ++i) {
        Module_Param* const curr = mp->get_elem(i);
        if ((curr->get_type()!=Module_Param::MP_NotUsed)) {
          get_at(start_idx+(int)i)->set_param(*curr);
        }
      }
    } break;
    case Module_Param::MP_Indexed_List:
      param.error("Cannot concatenate an indexed value list");
      break;
    default:
      param.type_error(is_set()?"set of value":"record of value", get_descriptor()->name);
    }
    break;
  default:
    TTCN_error("Internal error: Record_Of_Type::set_param()");
  }
}

Module_Param* Record_Of_Type::get_param(Module_Param_Name& param_name) const
{
  if (!is_bound()) {
    return new Module_Param_Unbound();
  }
  if (param_name.next_name()) {
    // Haven't reached the end of the module parameter name
    // => the name refers to one of the elements, not to the whole record of
    char* param_field = param_name.get_current_name();
    if (param_field[0] < '0' || param_field[0] > '9') {
      TTCN_error("Unexpected record field name in module parameter reference, "
        "expected a valid index for %s type `%s'",
        is_set() ? "set of" : "record of", get_descriptor()->name);
    }
    int param_index = -1;
    sscanf(param_field, "%d", &param_index);
    return get_at(param_index)->get_param(param_name);
  }
  Vector<Module_Param*> values;
  for (int i = 0; i < val_ptr->n_elements; ++i) {
    values.push_back(val_ptr->value_elements[i]->get_param(param_name));
  }
  Module_Param_Value_List* mp = new Module_Param_Value_List();
  mp->add_list_with_implicit_ids(&values);
  values.clear();
  return mp;
}

void Record_Of_Type::set_implicit_omit()
{
  for (int i = 0; i < get_nof_elements(); ++i) {
    if (is_elem_bound(i))
      val_ptr->value_elements[i]->set_implicit_omit();
  }
}

void Record_Of_Type::add_refd_index(int index)
{
  if (NULL == refd_ind_ptr) {
    refd_ind_ptr = new refd_index_struct;
    refd_ind_ptr->max_refd_index = -1;
  }
  refd_ind_ptr->refd_indices.push_back(index);
  if (index > get_max_refd_index()) {
    refd_ind_ptr->max_refd_index = index;
  }
}

void Record_Of_Type::remove_refd_index(int index)
{
  for (size_t i = refd_ind_ptr->refd_indices.size(); i > 0; --i) {
    if (refd_ind_ptr->refd_indices[i - 1] == index) {
      refd_ind_ptr->refd_indices.erase_at(i - 1);
      break;
    }
  }
  if (refd_ind_ptr->refd_indices.empty()) {
    delete refd_ind_ptr;
    refd_ind_ptr = NULL;
  }
  else if (get_max_refd_index() == index) {
    refd_ind_ptr->max_refd_index = -1;
  }
}

boolean operator==(null_type /*null_value*/, const Record_Of_Type& other_value)
{
  if (other_value.val_ptr == NULL)
    TTCN_error("The right operand of comparison is an unbound value of type %s.",
               other_value.get_descriptor()->name);
  return other_value.get_nof_elements() == 0;
}

boolean operator!=(null_type null_value,
                          const Record_Of_Type& other_value)
{
  return !(null_value == other_value);
}

////////////////////////////////////////////////////////////////////////////////

boolean Record_Type::is_bound() const
{
  int field_cnt = get_count();
  for (int field_idx=0; field_idx<field_cnt; field_idx++) {
    const Base_Type* temp = get_at(field_idx);
    if(temp->is_optional()) {
      if(temp->is_present() && temp->get_opt_value()->is_bound()) return TRUE;
    }
    if(temp->is_bound()) return TRUE;
  }
  return FALSE;
}

boolean Record_Type::is_value() const
{
  int field_cnt = get_count();
  for (int field_idx=0; field_idx<field_cnt; field_idx++) {
    const Base_Type* temp = get_at(field_idx);
    if(temp->is_optional()) {
      if(!temp->is_bound()) return FALSE;
      if(temp->is_present() && !temp->is_value()) return FALSE;
    } else {
      if(!temp->is_value()) return FALSE;
    }
  }
  return TRUE;
}

void Record_Type::clean_up()
{
  int field_cnt = get_count();
  for (int field_idx=0; field_idx<field_cnt; field_idx++) {
    get_at(field_idx)->clean_up();
  }
}

void Record_Type::log() const
{
  if (!is_bound()) {
    TTCN_Logger::log_event_unbound();
    return;
  }
  TTCN_Logger::log_event_str("{ ");
  int field_cnt = get_count();
  for (int field_idx=0; field_idx<field_cnt; field_idx++) {
    if (field_idx) TTCN_Logger::log_event_str(", ");
    TTCN_Logger::log_event_str(fld_name(field_idx));
    TTCN_Logger::log_event_str(" := ");
    get_at(field_idx)->log();
  }
  TTCN_Logger::log_event_str(" }");
  if (err_descr) err_descr->log();
}

void Record_Type::set_param(Module_Param& param) {
  if (dynamic_cast<Module_Param_Name*>(param.get_id()) != NULL &&
      param.get_id()->next_name()) {
    // Haven't reached the end of the module parameter name
    // => the name refers to one of the fields, not to the whole record
    char* param_field = param.get_id()->get_current_name();
    if (param_field[0] >= '0' && param_field[0] <= '9') {
      param.error("Unexpected array index in module parameter, expected a valid field"
        " name for %s type `%s'", is_set() ? "set" : "record", get_descriptor()->name);
    }
    int field_cnt = get_count();
    for (int field_idx = 0; field_idx < field_cnt; field_idx++) {
      if (strcmp(fld_name(field_idx), param_field) == 0) {
        get_at(field_idx)->set_param(param);
        return;
      }
    }
    param.error("Field `%s' not found in %s type `%s'",
      param_field, is_set() ? "set" : "record", get_descriptor()->name);
  }

  param.basic_check(Module_Param::BC_VALUE, is_set()?"set value":"record value");
  
  Module_Param_Ptr mp = &param;
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
  
  switch (mp->get_type()) {
  case Module_Param::MP_Value_List:
    if (get_count()<(int)mp->get_size()) {
      param.error("%s value of type %s has %d fields but list value has %d fields", is_set()?"Set":"Record", get_descriptor()->name, get_count(), (int)mp->get_size());
    }
    for (size_t i=0; i<mp->get_size(); i++) {
      Module_Param* mp_elem = mp->get_elem(i);
      if (mp_elem->get_type()!=Module_Param::MP_NotUsed) {
        get_at((int)i)->set_param(*mp_elem);
      }
    }
    break;
  case Module_Param::MP_Assignment_List:
    for (size_t i=0; i<mp->get_size(); ++i) {
      Module_Param* const current = mp->get_elem(i);
      boolean found = FALSE;
      for (int j=0; j<get_count(); ++j) {
        if (!strcmp(fld_name(j), current->get_id()->get_name())) {
          if (current->get_type()!=Module_Param::MP_NotUsed) {
            get_at(j)->set_param(*current);
          }
          found = TRUE;
          break;
        }
      }
      if (!found) {
        current->error("Non existent field name in type %s: %s.", get_descriptor()->name, current->get_id()->get_name());
      }
    }
    break;
  default:
    param.type_error(is_set()?"set value":"record value", get_descriptor()->name);
  }
}

Module_Param* Record_Type::get_param(Module_Param_Name& param_name) const
{
  if (!is_bound()) {
    return new Module_Param_Unbound();
  }
  if (param_name.next_name()) {
    // Haven't reached the end of the module parameter name
    // => the name refers to one of the fields, not to the whole record
    char* param_field = param_name.get_current_name();
    if (param_field[0] >= '0' && param_field[0] <= '9') {
      TTCN_error("Unexpected array index in module parameter reference, "
        "expected a valid field name for %s type `%s'",
        is_set() ? "set" : "record", get_descriptor()->name);
    }
    int field_cnt = get_count();
    for (int field_idx = 0; field_idx < field_cnt; field_idx++) {
      if (strcmp(fld_name(field_idx), param_field) == 0) {
        return get_at(field_idx)->get_param(param_name);
      }
    }
    TTCN_error("Field `%s' not found in %s type `%s'",
      param_field, is_set() ? "set" : "record", get_descriptor()->name);
  }
  Module_Param_Assignment_List* mp = new Module_Param_Assignment_List();
  for (int i = 0; i < get_count(); ++i) {
    Module_Param* mp_field = get_at(i)->get_param(param_name);
    mp_field->set_id(new Module_Param_FieldName(mcopystr(fld_name(i))));
    mp->add_elem(mp_field);
  }
  return mp;
}

void Record_Type::set_implicit_omit()
{
  int field_cnt = get_count();
  for (int field_idx = 0; field_idx < field_cnt; field_idx++) {
    Base_Type *temp = get_at(field_idx);
    if (temp->is_optional()) {
      if (temp->is_bound()) temp->set_implicit_omit();
      else temp->set_to_omit();
    } else if (temp->is_bound()) {
      temp->set_implicit_omit();
    }
  }
}

int Record_Type::size_of() const
{
  int opt_count = optional_count();
  if (opt_count==0) return get_count();
  const int* optional_indexes = get_optional_indexes();
  int my_size = get_count();
  for (int i=0; i<opt_count; i++) {
    if (!get_at(optional_indexes[i])->ispresent()) my_size--;
  }
  return my_size;
}

void Record_Type::encode_text(Text_Buf& text_buf) const
{
  if (!is_bound()) {
    TTCN_error("Text encoder: Encoding an unbound record/set value of type %s.",
      get_descriptor()->name);
  }
  int field_cnt = get_count();
  for (int field_idx=0; field_idx<field_cnt; field_idx++)
    get_at(field_idx)->encode_text(text_buf);
}

void Record_Type::decode_text(Text_Buf& text_buf)
{
  int field_cnt = get_count();
  for (int field_idx=0; field_idx<field_cnt; field_idx++)
    get_at(field_idx)->decode_text(text_buf);
}

boolean Record_Type::is_equal(const Base_Type* other_value) const
{
  const Record_Type* other_record = static_cast<const Record_Type*>(other_value);
  int field_cnt = get_count();
  for (int field_idx=0; field_idx<field_cnt; field_idx++) {
    const Base_Type* elem = get_at(field_idx);
    const Base_Type* other_elem = other_record->get_at(field_idx);
    if (elem->is_bound()) {
      if (other_elem->is_bound()) {
         if (!elem->is_equal(other_elem))
           return FALSE;
      } else return FALSE;
    } else if (other_elem->is_bound()) return FALSE;
  }
  return TRUE;
}

void Record_Type::set_value(const Base_Type* other_value)
{
  if (this==other_value) return;
  if (!other_value->is_bound())
    TTCN_error("Copying an unbound record/set value of type %s.",
               other_value->get_descriptor()->name);
  const Record_Type* other_record = static_cast<const Record_Type*>(other_value);
  int field_cnt = get_count();
  for (int field_idx=0; field_idx<field_cnt; field_idx++) {
    const Base_Type* elem = other_record->get_at(field_idx);
    if (elem->is_bound()) {
      get_at(field_idx)->set_value(elem);
    } else {
      get_at(field_idx)->clean_up();
    }
  }
  err_descr = other_record->err_descr;
}

void Record_Type::encode(const TTCN_Typedescriptor_t& p_td,
  TTCN_Buffer& p_buf, TTCN_EncDec::coding_t p_coding, ...) const
{
  va_list pvar;
  va_start(pvar, p_coding);
  switch(p_coding) {
  case TTCN_EncDec::CT_BER: {
    TTCN_EncDec_ErrorContext ec("While BER-encoding type '%s': ", p_td.name);
    unsigned BER_coding=va_arg(pvar, unsigned);
    BER_encode_chk_coding(BER_coding);
    ASN_BER_TLV_t *tlv=BER_encode_TLV(p_td, BER_coding);
    tlv->put_in_buffer(p_buf);
    ASN_BER_TLV_t::destruct(tlv);
    break;}
  case TTCN_EncDec::CT_RAW: {
    TTCN_EncDec_ErrorContext ec("While RAW-encoding type '%s': ", p_td.name);
    if(!p_td.raw) TTCN_EncDec_ErrorContext::error_internal
                    ("No RAW descriptor available for type '%s'.", p_td.name);
    RAW_enc_tr_pos rp;
    rp.level=0;
    rp.pos=NULL;
    RAW_enc_tree root(FALSE, NULL, &rp, 1, p_td.raw);
    RAW_encode(p_td, root);
    root.put_to_buf(p_buf);
    break;}
  case TTCN_EncDec::CT_TEXT: {
    TTCN_EncDec_ErrorContext ec("While TEXT-encoding type '%s': ", p_td.name);
    if(!p_td.text) TTCN_EncDec_ErrorContext::error_internal
                     ("No TEXT descriptor available for type '%s'.", p_td.name);
    TEXT_encode(p_td,p_buf);
    break;}
  case TTCN_EncDec::CT_XER: {
    TTCN_EncDec_ErrorContext ec("While XER-encoding type '%s': ", p_td.name);
    unsigned XER_coding=va_arg(pvar, unsigned);
    XER_encode(*(p_td.xer),p_buf, XER_coding, 0, 0, 0);
    p_buf.put_c('\n');
    break;}
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

void Record_Type::decode(const TTCN_Typedescriptor_t& p_td,
  TTCN_Buffer& p_buf, TTCN_EncDec::coding_t p_coding, ...)
{
  va_list pvar;
  va_start(pvar, p_coding);
  switch(p_coding) {
  case TTCN_EncDec::CT_BER: {
    TTCN_EncDec_ErrorContext ec("While BER-decoding type '%s': ", p_td.name);
    unsigned L_form=va_arg(pvar, unsigned);
    ASN_BER_TLV_t tlv;
    BER_decode_str2TLV(p_buf, tlv, L_form);
    BER_decode_TLV(p_td, tlv, L_form);
    if(tlv.isComplete) p_buf.increase_pos(tlv.get_len());
    break;}
  case TTCN_EncDec::CT_RAW: {
    TTCN_EncDec_ErrorContext ec("While RAW-decoding type '%s': ", p_td.name);
    if(!p_td.raw)
      TTCN_EncDec_ErrorContext::error_internal
        ("No RAW descriptor available for type '%s'.", p_td.name);
    raw_order_t order;
    switch(p_td.raw->top_bit_order) {
    case TOP_BIT_LEFT:
      order=ORDER_LSB;
      break;
    case TOP_BIT_RIGHT:
    default:
      order=ORDER_MSB;
    }
    int rawr = RAW_decode(p_td, p_buf, p_buf.get_len()*8, order);
    if (rawr < 0) switch (-rawr) {
    case TTCN_EncDec::ET_INCOMPL_MSG:
    case TTCN_EncDec::ET_LEN_ERR:
      ec.error(TTCN_EncDec::ET_INCOMPL_MSG,
               "Can not decode type '%s', because incomplete"
               " message was received", p_td.name);
      break;
    case 1:
      // The RAW/TEXT decoders return -1 for anything not a length error.
      // This is the value for ET_UNBOUND, which can't happen in decoding.
    default:
      ec.error(TTCN_EncDec::ET_INVAL_MSG,
               "Can not decode type '%s', because invalid"
               " message was received", p_td.name);
      break;
    }
    break;}
  case TTCN_EncDec::CT_TEXT: {
    Limit_Token_List limit;
    TTCN_EncDec_ErrorContext ec("While TEXT-decoding type '%s': ", p_td.name);
    if(!p_td.text) TTCN_EncDec_ErrorContext::error_internal
                     ("No TEXT descriptor available for type '%s'.", p_td.name);
    const unsigned char *b=p_buf.get_data();
    if(b[p_buf.get_len()-1]!='\0'){
      p_buf.set_pos(p_buf.get_len());
      p_buf.put_zero(8,ORDER_LSB);
      p_buf.rewind();
    }
    if(TEXT_decode(p_td,p_buf,limit)<0)
      ec.error(TTCN_EncDec::ET_INCOMPL_MSG,
        "Can not decode type '%s', because invalid or incomplete"
        " message was received", p_td.name);
    break;}
  case TTCN_EncDec::CT_XER: {
    TTCN_EncDec_ErrorContext ec("While XER-decoding type '%s': ", p_td.name);
    unsigned XER_coding=va_arg(pvar, unsigned);
    XmlReaderWrap reader(p_buf);
    for (int success=reader.Read(); success==1; success=reader.Read()) {
      if (reader.NodeType() == XML_READER_TYPE_ELEMENT) break;
    }
    XER_decode(*(p_td.xer), reader, XER_coding | XER_TOPLEVEL, XER_NONE, 0);
    size_t bytes = reader.ByteConsumed();
    p_buf.set_pos(bytes);
    break;}
  case TTCN_EncDec::CT_JSON: {
    TTCN_EncDec_ErrorContext ec("While JSON-decoding type '%s': ", p_td.name);
    if(!p_td.json) TTCN_EncDec_ErrorContext::error_internal
                     ("No JSON descriptor available for type '%s'.", p_td.name);
    JSON_Tokenizer tok((const char*)p_buf.get_data(), p_buf.get_len());
    if(JSON_decode(p_td, tok, FALSE)<0)
      ec.error(TTCN_EncDec::ET_INCOMPL_MSG,
        "Can not decode type '%s', because invalid or incomplete"
        " message was received", p_td.name);
    p_buf.set_pos(tok.get_buf_pos());
    break;}
  default:
    TTCN_error("Unknown coding method requested to decode type '%s'", p_td.name);
  }
  va_end(pvar);
}

ASN_BER_TLV_t* Record_Type::BER_encode_TLV(const TTCN_Typedescriptor_t& p_td, unsigned p_coding) const
{
  if (err_descr) {
    return BER_encode_TLV_negtest(err_descr, p_td, p_coding);
  }
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error
      (TTCN_EncDec::ET_UNBOUND, "Encoding an unbound value.");
  }
  BER_chk_descr(p_td);
  ASN_BER_TLV_t *new_tlv=ASN_BER_TLV_t::construct(NULL);
  TTCN_EncDec_ErrorContext ec_0("Component '");
  TTCN_EncDec_ErrorContext ec_1;
  int next_default_idx = 0;
  const default_struct* default_indexes = get_default_indexes();
  int field_cnt = get_count();
  for(int i=0; i<field_cnt; i++) {
    boolean is_default_field = default_indexes && (default_indexes[next_default_idx].index==i);
    if (!default_as_optional() && is_default_field) {
      if (!get_at(i)->is_equal(default_indexes[next_default_idx].value)) {
        ec_1.set_msg("%s': ", fld_name(i));
        new_tlv->add_TLV(get_at(i)->BER_encode_TLV(*fld_descr(i), p_coding));
      }
    } else { /* is not DEFAULT */
      ec_1.set_msg("%s': ", fld_name(i));
      new_tlv->add_TLV(get_at(i)->BER_encode_TLV(*fld_descr(i), p_coding));
    } /* !isDefault */
    if (is_default_field) next_default_idx++;
  } /* for i */
  if (is_set())
    if (p_coding==BER_ENCODE_DER) new_tlv->sort_tlvs_tag();
  new_tlv=ASN_BER_V2TLV(new_tlv, p_td, p_coding);
  return new_tlv;
}

ASN_BER_TLV_t* Record_Type::BER_encode_TLV_negtest(const Erroneous_descriptor_t* p_err_descr, const TTCN_Typedescriptor_t& p_td, unsigned p_coding) const
{
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error
      (TTCN_EncDec::ET_UNBOUND, "Encoding an unbound value.");
  }
  BER_chk_descr(p_td);
  ASN_BER_TLV_t *new_tlv=ASN_BER_TLV_t::construct(NULL);
  TTCN_EncDec_ErrorContext ec_0("Component '");
  TTCN_EncDec_ErrorContext ec_1;
  int next_default_idx = 0;
  const default_struct* default_indexes = get_default_indexes();
  int field_cnt = get_count();

  int values_idx = 0;
  int edescr_idx = 0;

  for (int i=0; i<field_cnt; i++) {
    boolean is_default_field = default_indexes && (default_indexes[next_default_idx].index==i);
     // the first condition is not needed, kept for ease of understanding
    if ( (p_err_descr->omit_before!=-1) && (i<p_err_descr->omit_before) ) {
      if (is_default_field) next_default_idx++;
      continue;
    }
    const Erroneous_values_t* err_vals = p_err_descr->next_field_err_values(i, values_idx);
    const Erroneous_descriptor_t* emb_descr = p_err_descr->next_field_emb_descr(i, edescr_idx);

    if (err_vals && err_vals->before) {
      if (err_vals->before->errval==NULL) TTCN_error(
        "internal error: erroneous before value missing");
      ec_1.set_msg("%s'(erroneous before): ", fld_name(i));
      if (err_vals->before->raw) {
        new_tlv->add_TLV(err_vals->before->errval->BER_encode_negtest_raw());
      } else {
        if (err_vals->before->type_descr==NULL) TTCN_error(
          "internal error: erroneous before typedescriptor missing");
        new_tlv->add_TLV(err_vals->before->errval->BER_encode_TLV(
          *err_vals->before->type_descr, p_coding));
      }
    }

    if (err_vals && err_vals->value) {
      if (err_vals->value->errval) { // replace
        ec_1.set_msg("%s'(erroneous value): ", fld_name(i));
        if (err_vals->value->raw) {
          new_tlv->add_TLV(err_vals->value->errval->BER_encode_negtest_raw());
        } else {
          if (err_vals->value->type_descr==NULL) TTCN_error(
            "internal error: erroneous value typedescriptor missing");
          new_tlv->add_TLV(err_vals->value->errval->BER_encode_TLV(
            *err_vals->value->type_descr, p_coding));
        }
      } // else -> omit
    } else {
      if (!default_as_optional() && is_default_field) {
        if (!get_at(i)->is_equal(default_indexes[next_default_idx].value)) {
          ec_1.set_msg("'%s': ", fld_name(i));
          if (emb_descr) {
            new_tlv->add_TLV(get_at(i)->BER_encode_TLV_negtest(emb_descr,
              *fld_descr(i), p_coding));
          } else {
            new_tlv->add_TLV(get_at(i)->BER_encode_TLV(*fld_descr(i), p_coding));
          }
        }
      } else { /* is not DEFAULT */
        ec_1.set_msg("'%s': ", fld_name(i));
        if (emb_descr) {
          new_tlv->add_TLV(get_at(i)->BER_encode_TLV_negtest(emb_descr,
            *fld_descr(i), p_coding));
        } else {
          new_tlv->add_TLV(get_at(i)->BER_encode_TLV(*fld_descr(i), p_coding));
        }
      } /* !isDefault */
    }

    if (err_vals && err_vals->after) {
      if (err_vals->after->errval==NULL) TTCN_error(
        "internal error: erroneous after value missing");
      ec_1.set_msg("%s'(erroneous after): ", fld_name(i));
      if (err_vals->after->raw) {
        new_tlv->add_TLV(err_vals->after->errval->BER_encode_negtest_raw());
      } else {
        if (err_vals->after->type_descr==NULL) TTCN_error(
          "internal error: erroneous after typedescriptor missing");
        new_tlv->add_TLV(err_vals->after->errval->BER_encode_TLV(
          *err_vals->after->type_descr, p_coding));
      }
    }

    if (is_default_field) next_default_idx++;
    if ( (p_err_descr->omit_after!=-1) && (i>=p_err_descr->omit_after) ) break;
  } /* for i */

  if (is_set())
    if (p_coding==BER_ENCODE_DER) new_tlv->sort_tlvs_tag();
  new_tlv=ASN_BER_V2TLV(new_tlv, p_td, p_coding);
  return new_tlv;
}

boolean Record_Type::BER_decode_TLV(const TTCN_Typedescriptor_t& p_td,
  const ASN_BER_TLV_t& p_tlv, unsigned L_form)
{
  BER_chk_descr(p_td);
  ASN_BER_TLV_t stripped_tlv;
  BER_decode_strip_tags(*p_td.ber, p_tlv, L_form, stripped_tlv);
  TTCN_EncDec_ErrorContext ec_0("While decoding '%s' type: ", get_descriptor()->name);
  stripped_tlv.chk_constructed_flag(TRUE);
  size_t V_pos=0;
  ASN_BER_TLV_t tmp_tlv;
  if (!is_set())
  { /* SEQUENCE decoding */
    boolean tlv_present=FALSE;
    {
      TTCN_EncDec_ErrorContext ec_1("Component '");
      TTCN_EncDec_ErrorContext ec_2;
      int next_default_idx = 0;
      int next_optional_idx = 0;
      const default_struct* default_indexes = get_default_indexes();
      const int* optional_indexes = get_optional_indexes();
      int field_cnt = get_count();
      for(int i=0; i<field_cnt; i++) {
        boolean is_default_field = default_indexes && (default_indexes[next_default_idx].index==i);
        boolean is_optional_field = optional_indexes && (optional_indexes[next_optional_idx]==i);
        ec_2.set_msg("%s': ", fld_descr(i)->name);
        if (!tlv_present) tlv_present=BER_decode_constdTLV_next(stripped_tlv, V_pos, L_form, tmp_tlv);
        if (is_default_field) { /* is DEFAULT */
          if (!tlv_present || !get_at(i)->BER_decode_isMyMsg(*fld_descr(i), tmp_tlv)) {
            get_at(i)->set_value(default_indexes[next_default_idx].value);
          } else {
            get_at(i)->BER_decode_TLV(*fld_descr(i), tmp_tlv, L_form);
            tlv_present=FALSE;
          }
        }
        else if (is_optional_field) { /* is OPTIONAL */
          if (!tlv_present) get_at(i)->set_to_omit();
          else {
            get_at(i)->BER_decode_TLV(*fld_descr(i), tmp_tlv, L_form);
            if (get_at(i)->ispresent()) tlv_present=FALSE;
          }
        }
        else { /* is not DEFAULT OPTIONAL */
          if(!tlv_present){
            ec_2.error(TTCN_EncDec::ET_INCOMPL_MSG,"Invalid or incomplete message was received.");
            return FALSE;
          }
          get_at(i)->BER_decode_TLV(*fld_descr(i), tmp_tlv, L_form);
          tlv_present=FALSE;
        } /* !isDefault */
        if (is_default_field) next_default_idx++;
        if (is_optional_field) next_optional_idx++;
      } /* for i */
    }
    BER_decode_constdTLV_end(stripped_tlv, V_pos, L_form, tmp_tlv, tlv_present);
  } /* SEQUENCE decoding */
  else
  { /* SET decoding */
    /* field indicator:
     *   0x01: value arrived
     *   0x02: is optional / not used :)
     *   0x04: has default / not used :)
     */
    int field_cnt = get_count();
    unsigned char* fld_indctr = new unsigned char[field_cnt];
    for (int i=0; i<field_cnt; i++) fld_indctr[i] = 0;
    int fld_curr = -1; /* ellipsis or error... */
    while (BER_decode_constdTLV_next(stripped_tlv, V_pos, L_form, tmp_tlv)) {
      for (int i=0; i<field_cnt; i++) {
        if (get_at(i)->BER_decode_isMyMsg(*fld_descr(i), tmp_tlv)) {
          fld_curr=i;
          TTCN_EncDec_ErrorContext ec_1("Component '%s': ", fld_name(i));
          get_at(i)->BER_decode_TLV(*fld_descr(i), tmp_tlv, L_form);
          break;
        }
      }
      if (fld_curr!=-1) {
        if (fld_indctr[fld_curr])
          ec_0.error(TTCN_EncDec::ET_DEC_DUPFLD, "Duplicated value for component '%s'.", fld_name(fld_curr));
        fld_indctr[fld_curr]=1;
      } /* if != -1 */
    } /* while */
    int next_default_idx = 0;
    int next_optional_idx = 0;
    const default_struct* default_indexes = get_default_indexes();
    const int* optional_indexes = get_optional_indexes();
    for (fld_curr=0; fld_curr<field_cnt; fld_curr++) {
      boolean is_default_field = default_indexes && (default_indexes[next_default_idx].index==fld_curr);
      boolean is_optional_field = optional_indexes && (optional_indexes[next_optional_idx]==fld_curr);
      if (!fld_indctr[fld_curr]) {
        if (is_default_field) get_at(fld_curr)->set_value(default_indexes[next_default_idx].value);
        else if (is_optional_field) get_at(fld_curr)->set_to_omit();
        else ec_0.error(TTCN_EncDec::ET_DEC_MISSFLD, "Missing value for component '%s'.", fld_name(fld_curr));
      }
      if (is_default_field) next_default_idx++;
      if (is_optional_field) next_optional_idx++;
    }
    delete[] fld_indctr;
  } /* SET decoding */

  if (is_opentype_outermost()) {
    TTCN_EncDec_ErrorContext ec_1("While decoding opentypes: ");
    TTCN_Type_list p_typelist;
    BER_decode_opentypes(p_typelist, L_form);
  } /* if sdef->opentype_outermost */
  return TRUE;
}

void Record_Type::BER_decode_opentypes(TTCN_Type_list& p_typelist, unsigned L_form)
{
  p_typelist.push(this);
  TTCN_EncDec_ErrorContext ec_0("Component '");
  TTCN_EncDec_ErrorContext ec_1;
  int field_cnt = get_count();
  for(int i=0; i<field_cnt; i++) {
    ec_1.set_msg("%s': ", fld_name(i));
    get_at(i)->BER_decode_opentypes(p_typelist, L_form);
  } /* for i */
  p_typelist.pop();
}

int Record_Type::RAW_encode(const TTCN_Typedescriptor_t& p_td,
  RAW_enc_tree& myleaf) const
{
  if (err_descr) return RAW_encode_negtest(err_descr, p_td, myleaf);
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error
      (TTCN_EncDec::ET_UNBOUND, "Encoding an unbound value.");
  }
  int encoded_length = 0;
  int field_cnt = get_count();
  myleaf.isleaf = FALSE;
  myleaf.body.node.num_of_nodes = field_cnt;
  myleaf.body.node.nodes = init_nodes_of_enc_tree(field_cnt);
  /* init nodes */
  int next_optional_idx = 0;
  const int* optional_indexes = get_optional_indexes();
  for (int i = 0; i < field_cnt; i++) {
    boolean is_optional_field = optional_indexes
      && (optional_indexes[next_optional_idx] == i);
    if (!is_optional_field || get_at(i)->ispresent()) {
      myleaf.body.node.nodes[i] = new RAW_enc_tree(TRUE, &myleaf,
        &(myleaf.curr_pos), i, fld_descr(i)->raw);
    }
    else {
      myleaf.body.node.nodes[i] = NULL;
    }
    if (is_optional_field) next_optional_idx++;
  }
  next_optional_idx = 0;
  for (int i = 0; i < field_cnt; i++) { /*encoding fields*/
    boolean is_optional_field = optional_indexes
      && (optional_indexes[next_optional_idx] == i);
    /* encoding of normal fields*/
    const Base_Type *field = get_at(i);
    if (is_optional_field) {
      next_optional_idx++;
      if (!field->ispresent())
        continue; // do not encode
      else
        field = field->get_opt_value(); // "reach into" the optional
    }
    encoded_length += field->RAW_encode(*fld_descr(i),
      *myleaf.body.node.nodes[i]);
  }
  return myleaf.length = encoded_length;
}

// In some cases (e.g. LENGTHTO, POINTERTO, CROSSTAG) it is not generated.
int Record_Type::RAW_encode_negtest(const Erroneous_descriptor_t *p_err_descr,
  const TTCN_Typedescriptor_t& /*p_td*/, RAW_enc_tree& myleaf) const
{
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error
      (TTCN_EncDec::ET_UNBOUND, "Encoding an unbound value.");
  }
  int encoded_length = 0;
  int num_fields = get_count();
  myleaf.isleaf = FALSE;
  myleaf.body.node.num_of_nodes = 0;
  for (int field_idx = 0; field_idx < num_fields; ++field_idx) {
    if ((p_err_descr->omit_before != -1) &&
        (field_idx < p_err_descr->omit_before))
      continue;
    else ++myleaf.body.node.num_of_nodes;
    const Erroneous_values_t *err_vals = p_err_descr->get_field_err_values(field_idx);
    if (err_vals && err_vals->before)
      ++myleaf.body.node.num_of_nodes;
    if (err_vals && err_vals->value && !err_vals->value->errval)
      --myleaf.body.node.num_of_nodes;
    if (err_vals && err_vals->after)
      ++myleaf.body.node.num_of_nodes;
    if ((p_err_descr->omit_after != -1) &&
        (field_idx >= p_err_descr->omit_after))
      break;
  }
  myleaf.body.node.nodes = init_nodes_of_enc_tree(myleaf.body.node.num_of_nodes);
  TTCN_EncDec_ErrorContext ec;
  int next_optional_idx = 0;
  const int *my_optional_indexes = get_optional_indexes();
  // Counter for fields and additional before/after fields.
  int node_pos = 0;
  for (int field_idx = 0; field_idx < num_fields; ++field_idx) {
    boolean is_optional_field = my_optional_indexes &&
      (my_optional_indexes[next_optional_idx] == field_idx);
    if ((p_err_descr->omit_before != -1) &&
        (field_idx < p_err_descr->omit_before)) {
      if (is_optional_field) ++next_optional_idx;
      continue;
    }
    const Erroneous_values_t *err_vals = p_err_descr->get_field_err_values(field_idx);
    const Erroneous_descriptor_t *emb_descr = p_err_descr->get_field_emb_descr(field_idx);
    if (err_vals && err_vals->before) {
      if (err_vals->before->errval == NULL)
        TTCN_error("internal error: erroneous before value missing");
      if (err_vals->before->raw) {
        myleaf.body.node.nodes[node_pos] =
          new RAW_enc_tree(TRUE, &myleaf, &(myleaf.curr_pos), node_pos,
                           err_vals->before->errval->get_descriptor()->raw);
        encoded_length += err_vals->before->errval->
          RAW_encode_negtest_raw(*myleaf.body.node.nodes[node_pos++]);
      } else {
        if (err_vals->before->type_descr == NULL)
          TTCN_error("internal error: erroneous before typedescriptor missing");
        myleaf.body.node.nodes[node_pos] =
          new RAW_enc_tree(TRUE, &myleaf, &(myleaf.curr_pos), node_pos,
                           err_vals->before->type_descr->raw);
        encoded_length += err_vals->before->errval->
          RAW_encode(*(err_vals->before->type_descr),
                     *myleaf.body.node.nodes[node_pos++]);
      }
    }
    if (err_vals && err_vals->value) {
      if (err_vals->value->errval) {
        ec.set_msg("'%s'(erroneous value): ", fld_name(field_idx));
        if (err_vals->value->raw) {
          myleaf.body.node.nodes[node_pos] =
            new RAW_enc_tree(TRUE, &myleaf, &(myleaf.curr_pos), node_pos,
                             err_vals->value->errval->get_descriptor()->raw);
          encoded_length += err_vals->value->errval->
            RAW_encode_negtest_raw(*myleaf.body.node.nodes[node_pos++]);
        } else {
          if (err_vals->value->type_descr == NULL)
            TTCN_error("internal error: erroneous value typedescriptor missing");
          myleaf.body.node.nodes[node_pos] =
            new RAW_enc_tree(TRUE, &myleaf, &(myleaf.curr_pos), node_pos,
                             err_vals->value->type_descr->raw);
          encoded_length += err_vals->value->errval->
            RAW_encode(*(err_vals->value->type_descr),
                       *myleaf.body.node.nodes[node_pos++]);
        }
      }
    } else {
      ec.set_msg("'%s': ", fld_name(field_idx));
      if (!is_optional_field || get_at(field_idx)->ispresent()) {
        const Base_Type *field =
          is_optional_field ? get_at(field_idx)->get_opt_value()
                            : get_at(field_idx);
        myleaf.body.node.nodes[node_pos] =
          new RAW_enc_tree(TRUE, &myleaf, &(myleaf.curr_pos), node_pos,
                           fld_descr(field_idx)->raw);
        if (emb_descr) {
          encoded_length +=
            field->RAW_encode_negtest(emb_descr, *fld_descr(field_idx),
                                      *myleaf.body.node.nodes[node_pos++]);
        } else {
          encoded_length +=
            field->RAW_encode(*fld_descr(field_idx),
                              *myleaf.body.node.nodes[node_pos++]);
        }
      } else {
        // `omitted' field.
        myleaf.body.node.nodes[node_pos++] = NULL;
      }
    }
    if (err_vals && err_vals->after) {
      if (err_vals->after->errval == NULL)
        TTCN_error("internal error: erroneous before value missing");
      if (err_vals->after->raw) {
        myleaf.body.node.nodes[node_pos] =
          new RAW_enc_tree(TRUE, &myleaf, &(myleaf.curr_pos), node_pos,
                           err_vals->after->errval->get_descriptor()->raw);
        encoded_length += err_vals->after->errval->
          RAW_encode_negtest_raw(*myleaf.body.node.nodes[node_pos++]);
      } else {
        if (err_vals->after->type_descr == NULL)
          TTCN_error("internal error: erroneous after typedescriptor missing");
        myleaf.body.node.nodes[node_pos] =
          new RAW_enc_tree(TRUE, &myleaf, &(myleaf.curr_pos), node_pos,
                           err_vals->after->type_descr->raw);
        encoded_length += err_vals->after->errval->
          RAW_encode(*(err_vals->after->type_descr),
                     *myleaf.body.node.nodes[node_pos++]);
      }
    }
    if (is_optional_field) ++next_optional_idx;
    if ((p_err_descr->omit_after != -1) &&
        (field_idx >= p_err_descr->omit_after))
      break;
  }
  return myleaf.length = encoded_length;
}

int Record_Type::RAW_decode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& buff,
  int limit, raw_order_t top_bit_ord, boolean no_err, int, boolean)
{
  int field_cnt = get_count();
  int opt_cnt = optional_count();
  int mand_num = field_cnt - opt_cnt; // expected mandatory fields

  raw_order_t local_top_order;
  if (p_td.raw->top_bit_order == TOP_BIT_INHERITED) local_top_order = top_bit_ord;
  else if (p_td.raw->top_bit_order == TOP_BIT_RIGHT) local_top_order = ORDER_MSB;
  else local_top_order = ORDER_LSB;

  if (is_set()) { /* set decoder start*/
    int prepaddlength = buff.increase_pos_padd(p_td.raw->prepadding);
    limit -= prepaddlength;
    int decoded_length = 0;
    int * const field_map = new int[field_cnt];
    memset(field_map, 0, field_cnt * sizeof(int));
    int nof_mand_fields = 0; // mandatory fields actually decoded
    if (opt_cnt>0) {
      const int* optional_indexes = get_optional_indexes();
      for (int i=0; i<opt_cnt; i++) get_at(optional_indexes[i])->set_to_omit();
    }
    while (limit > 0) {
      size_t fl_start_pos = buff.get_pos_bit();
      int next_optional_idx = 0;
      const int* optional_indexes = get_optional_indexes();
      for (int i=0; i<field_cnt; i++) { /* decoding fields without TAG */
        boolean is_optional_field = optional_indexes && (optional_indexes[next_optional_idx]==i);
        if (field_map[i] == 0) {
          Base_Type* field_ptr = get_at(i);
          if (is_optional_field) {
            field_ptr->set_to_present();
            field_ptr=field_ptr->get_opt_value();
          }
          int decoded_field_length = field_ptr->RAW_decode(*fld_descr(i), buff,
            limit, local_top_order, TRUE);
          if ( (is_optional_field && (decoded_field_length>0)) ||
            (!is_optional_field && (decoded_field_length>=0)) ) {
            decoded_length += decoded_field_length;
            limit -= decoded_field_length;
            if (!is_optional_field) nof_mand_fields++;
            field_map[i] = 1;
            goto continue_while;
          } else {
            buff.set_pos_bit(fl_start_pos);
            if (is_optional_field) get_at(i)->set_to_omit();
          }
        }
        if (is_optional_field) next_optional_idx++;
      }//for i
      break; // no field could be decoded successfully, quit
continue_while: ;
    }
    delete[] field_map;
    if (mand_num > 0 && nof_mand_fields != mand_num) {
      /* Not all required fields were decoded. If there are no bits left,
       * that means that the last field was decoded successfully but used up
       * the buffer. Signal "incomplete". If there were bits left, that means
       * no field could be decoded from them; signal an error. */
      return limit ? -1 : -TTCN_EncDec::ET_INCOMPL_MSG;
    }
    return decoded_length + prepaddlength + buff.increase_pos_padd(p_td.raw->padding);
  } else { /* record decoder start */
    int prepaddlength = buff.increase_pos_padd(p_td.raw->prepadding);
    limit -= prepaddlength;
    size_t last_decoded_pos = buff.get_pos_bit();
    size_t fl_start_pos;
    int decoded_length = 0;
    int decoded_field_length = 0;
    if (raw_has_ext_bit()) {
      const unsigned char* data=buff.get_read_data();
      int count=1;
      unsigned mask = 1 << (local_top_order==ORDER_LSB ? 0 : 7);
      if (p_td.raw->extension_bit==EXT_BIT_YES) {
        while((data[count-1] & mask) == 0 && count * 8 < (int)limit) count++;
      }
      else {
        while((data[count-1] & mask) != 0 && count * 8 < (int)limit) count++;
      }
      if(limit) limit=count*8;
    }

    int next_optional_idx = 0;
    const int* optional_indexes = get_optional_indexes();
    for (int i=0; i<field_cnt; i++) { /* decoding fields */
      boolean is_optional_field = optional_indexes && (optional_indexes[next_optional_idx]==i);
      /* check if enough bits to decode the field*/
      if (!is_optional_field || (limit>0)) {
        /* decoding of normal field */
        fl_start_pos = buff.get_pos_bit();
        Base_Type* field_ptr = get_at(i);
        if (is_optional_field) {
          field_ptr->set_to_present();
          field_ptr=field_ptr->get_opt_value();
        }
        decoded_field_length = field_ptr->RAW_decode(*fld_descr(i), buff, limit,
          local_top_order, is_optional_field ? TRUE : no_err);
        boolean field_present = TRUE;
        if (is_optional_field) {
          if (decoded_field_length < 1) { // swallow any error and become omit
            field_present = FALSE;
            get_at(i)->set_to_omit();
            buff.set_pos_bit(fl_start_pos);
          }
        } else {
          if (decoded_field_length < 0) return decoded_field_length;
        }
        if (field_present) {
          decoded_length+=decoded_field_length;
          limit-=decoded_field_length;
          last_decoded_pos=last_decoded_pos<buff.get_pos_bit()?buff.get_pos_bit():last_decoded_pos;
        }
      } else {
        get_at(i)->set_to_omit();
      }
      if (is_optional_field) next_optional_idx++;
    } /* decoding fields*/

    buff.set_pos_bit(last_decoded_pos);
    return decoded_length+prepaddlength+buff.increase_pos_padd(p_td.raw->padding);
  } /* record decoder end*/
}

int Record_Type::TEXT_encode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& buff) const
{
  if (err_descr) {
    return TEXT_encode_negtest(err_descr, p_td, buff);
  }
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error
      (TTCN_EncDec::ET_UNBOUND, "Encoding an unbound value.");
  }
  boolean need_separator=FALSE;
  int encoded_length=0;
  if (p_td.text->begin_encode) {
    buff.put_cs(*p_td.text->begin_encode);
    encoded_length+=p_td.text->begin_encode->lengthof();
  }
  int next_optional_idx = 0;
  const int* optional_indexes = get_optional_indexes();
  int field_cnt = get_count();
  for(int i=0;i<field_cnt;i++) {
    boolean is_optional_field = optional_indexes && (optional_indexes[next_optional_idx]==i);
    if (!is_optional_field || get_at(i)->ispresent()) {
      if (need_separator && p_td.text->separator_encode) {
        buff.put_cs(*p_td.text->separator_encode);
        encoded_length+=p_td.text->separator_encode->lengthof();
      }
      encoded_length += get_at(i)->TEXT_encode(*fld_descr(i),buff);
      need_separator=TRUE;
    }
    if (is_optional_field) next_optional_idx++;
  }
  if (p_td.text->end_encode) {
    buff.put_cs(*p_td.text->end_encode);
    encoded_length+=p_td.text->end_encode->lengthof();
  }
  return encoded_length;
}

/**
 * TEXT encode negative testing
 */
int Record_Type::TEXT_encode_negtest(const Erroneous_descriptor_t* p_err_descr, const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& buff) const
{
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error
      (TTCN_EncDec::ET_UNBOUND, "Encoding an unbound value.");
  }
  boolean need_separator=FALSE;
  int encoded_length=0;
  if (p_td.text->begin_encode) {
    buff.put_cs(*p_td.text->begin_encode);
    encoded_length+=p_td.text->begin_encode->lengthof();
  }
  int next_optional_idx = 0;
  const int* optional_indexes = get_optional_indexes();
  int field_cnt = get_count();

  int values_idx = 0;
  int edescr_idx = 0;

  for(int i=0;i<field_cnt;i++) {
    boolean is_optional_field = optional_indexes && (optional_indexes[next_optional_idx]==i);

    if ( (p_err_descr->omit_before!=-1) && (i<p_err_descr->omit_before) ) {
      if (is_optional_field) next_optional_idx++;
      continue;
    }

    const Erroneous_values_t* err_vals = p_err_descr->next_field_err_values(i, values_idx);
    const Erroneous_descriptor_t* emb_descr = p_err_descr->next_field_emb_descr(i, edescr_idx);

    if (err_vals && err_vals->before) {
      if (err_vals->before->errval==NULL) TTCN_error(
        "internal error: erroneous before value missing");

      if (need_separator && p_td.text->separator_encode) {
        buff.put_cs(*p_td.text->separator_encode);
        encoded_length+=p_td.text->separator_encode->lengthof();
      }
      if (err_vals->before->raw) {
        encoded_length += err_vals->before->errval->encode_raw(buff);
      } else {
        if (err_vals->before->type_descr==NULL) TTCN_error(
          "internal error: erroneous before typedescriptor missing");
        encoded_length += err_vals->before->errval->TEXT_encode(
          *(err_vals->before->type_descr),buff);
      }
      need_separator=TRUE;
    }

    if (err_vals && err_vals->value) {
      if (err_vals->value->errval) {
        if (need_separator && p_td.text->separator_encode) {
          buff.put_cs(*p_td.text->separator_encode);
          encoded_length+=p_td.text->separator_encode->lengthof();
        }
        if (err_vals->value->raw) {
          encoded_length += err_vals->value->errval->encode_raw(buff);
        } else {
          if (err_vals->value->type_descr==NULL) TTCN_error(
            "internal error: erroneous value typedescriptor missing");
          encoded_length += err_vals->value->errval->TEXT_encode(
            *(err_vals->value->type_descr),buff);
        }
        need_separator=TRUE;
      } // else -> omit
    } else {
      if (!is_optional_field || get_at(i)->ispresent()) {
        if (need_separator && p_td.text->separator_encode) {
          buff.put_cs(*p_td.text->separator_encode);
          encoded_length+=p_td.text->separator_encode->lengthof();
        }
        if (emb_descr) {
          encoded_length += get_at(i)->TEXT_encode_negtest(emb_descr, *fld_descr(i),buff);
        } else {
          encoded_length += get_at(i)->TEXT_encode(*fld_descr(i),buff);
        }
        need_separator=TRUE;
      }
    }

    if (err_vals && err_vals->after) {
      if (err_vals->after->errval==NULL) TTCN_error(
        "internal error: erroneous after value missing");
      if (need_separator && p_td.text->separator_encode) {
        buff.put_cs(*p_td.text->separator_encode);
        encoded_length+=p_td.text->separator_encode->lengthof();
      }
      if (err_vals->after->raw) {
        encoded_length += err_vals->after->errval->encode_raw(buff);
      } else {
        if (err_vals->after->type_descr==NULL) TTCN_error(
          "internal error: erroneous after typedescriptor missing");
        encoded_length += err_vals->after->errval->TEXT_encode(
          *(err_vals->after->type_descr),buff);
      }
      need_separator=TRUE;
    }

    if (is_optional_field) next_optional_idx++;

    if ( (p_err_descr->omit_after!=-1) && (i>=p_err_descr->omit_after) ) break;
  }
  if (p_td.text->end_encode) {
    buff.put_cs(*p_td.text->end_encode);
    encoded_length+=p_td.text->end_encode->lengthof();
  }
  return encoded_length;
}

int Record_Type::TEXT_decode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& buff,
  Limit_Token_List& limit, boolean no_err, boolean /*first_call*/)
{
  if (is_set()) {
    int decoded_length=0;
    int decoded_field_length=0;
    size_t pos=buff.get_pos();
    boolean sep_found=FALSE;
    int ml=0;
    int sep_length=0;
    int loop_detector=1;
    int last_field_num=-1;
    if (p_td.text->begin_decode) {
      int tl;
      if ((tl=p_td.text->begin_decode->match_begin(buff))<0) {
        if(no_err) return -1;
        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TOKEN_ERR,
          "The specified token '%s' not found for '%s': ",
          (const char*)*(p_td.text->begin_decode), p_td.name);
        return 0;
      }
      decoded_length+=tl;
      buff.increase_pos(tl);
    }
    if (p_td.text->end_decode) {
      limit.add_token(p_td.text->end_decode);
      ml++;
    }
    if(p_td.text->separator_decode){
      limit.add_token(p_td.text->separator_decode);
      ml++;
    }

    int field_cnt = get_count();
    int * const field_map = new int[field_cnt];
    memset(field_map, 0, field_cnt * sizeof(int));

    int mand_field_num = 0;
    int opt_field_num  = 0;
    int seof = 0;
    int has_repeatable=0;
    boolean repeatable = TRUE;

    int next_optional_idx = 0;
    const int* optional_indexes = get_optional_indexes();
    for (int i=0;i<field_cnt;i++) {
      boolean is_optional_field = optional_indexes && (optional_indexes[next_optional_idx]==i);
      if (is_optional_field) {
        get_at(i)->set_to_omit();
        opt_field_num++;
      } else {
        mand_field_num++;
      }
      if (get_at(i)->is_seof()) {
        seof++;
        repeatable = repeatable && fld_descr(i)->text->val.parameters->decoding_params.repeatable;
      }
      if (is_optional_field) next_optional_idx++;
    }
    boolean has_optinals = opt_field_num > 0;
    if ((seof>0) && repeatable) has_repeatable=1;

    while (mand_field_num+opt_field_num+has_repeatable) {
      loop_detector=1;
      /*while (TRUE)*/
      {
        next_optional_idx = 0;
        for (int i=0;i<field_cnt;i++) {
          boolean is_optional_field = optional_indexes && (optional_indexes[next_optional_idx]==i);
          if (get_at(i)->is_seof()) {
            if ( (fld_descr(i)->text->val.parameters->decoding_params.repeatable && field_map[i]<3)
                 || !field_map[i] ) {
              pos=buff.get_pos();
              decoded_field_length = get_at(i)->TEXT_decode(*fld_descr(i),buff, limit, TRUE,!field_map[i]);
              if (decoded_field_length<0) {
                buff.set_pos(pos);
                if (is_optional_field && !field_map[i]) get_at(i)->set_to_omit();
              } else {
                loop_detector=0;
                if (!field_map[i]) {
                  if (is_optional_field) opt_field_num--;
                  else mand_field_num--;
                  field_map[i]=1;
                } else field_map[i]=2;
                last_field_num=i;
                break;
              }
            }
          } else { // !...->is_seof
            if (!field_map[i]) {
              pos=buff.get_pos();
              decoded_field_length = get_at(i)->TEXT_decode(*fld_descr(i),buff,limit,TRUE);
              if (decoded_field_length<0) {
                buff.set_pos(pos);
                if (is_optional_field) get_at(i)->set_to_omit();
              } else {
                loop_detector=0;
                field_map[i]=1;
                if (is_optional_field) opt_field_num--;
                else mand_field_num--;
                last_field_num=i;
                break;
              }
            }
          } // !...->is_seof
          if (is_optional_field) next_optional_idx++;
        } // for i
        /* break*/
      }
      if (loop_detector) break;
      if (p_td.text->separator_decode) {
        int tl;
        if ((tl=p_td.text->separator_decode->match_begin(buff))<0) {
          if (p_td.text->end_decode) {
            int tl2;
            if ((tl2=p_td.text->end_decode->match_begin(buff))!=-1) {
              sep_found=FALSE;
              break;
            }
          } else if (limit.has_token(ml)) {
            int tl2;
            if ((tl2=limit.match(buff,ml))==0) {
              sep_found=FALSE;
              break;
            }
          } else break;
          buff.set_pos(pos);
          decoded_length-=decoded_field_length;
          field_map[last_field_num]+=2;

          if (has_optinals) {
            if (last_field_num>=0 && last_field_num<field_cnt) {
              if (get_at(last_field_num)->is_seof()) {
                if (get_at(last_field_num)->is_optional()) {
                  if (field_map[last_field_num]==3) {
                    get_at(last_field_num)->set_to_omit();
                    opt_field_num++;
                  }
                } else {
                  if (field_map[last_field_num]==3) {
                    mand_field_num++;
                  }
                }
              } else if (get_at(last_field_num)->is_optional()) {
                get_at(last_field_num)->set_to_omit();
                opt_field_num++;
              } else {
                mand_field_num++;
              }
            } else {
              mand_field_num++;
            }
          } // if (has_optinals)
        } else {
          sep_length=tl;
          decoded_length+=tl;
          buff.increase_pos(tl);
          for (int a=0;a<field_cnt;a++) if(field_map[a]>2) field_map[a]-=3;
          sep_found=TRUE;
        }
      } else if (p_td.text->end_decode) {
        int tl;
        if ((tl=p_td.text->end_decode->match_begin(buff))!=-1) {
          decoded_length+=tl;
          buff.increase_pos(tl);
          limit.remove_tokens(ml);
          if (mand_field_num)  decoded_length = -1;
          goto bail;
        }
      } else if(limit.has_token(ml)){
        int tl;
        if ((tl=limit.match(buff,ml))==0) {
          sep_found=FALSE;
          break;
        }
      }
    } // while ( + + )
    limit.remove_tokens(ml);
    if (sep_found) {
      if (mand_field_num) {
        if (no_err) decoded_length = -1;
        else TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TOKEN_ERR,
          "Error during decoding '%s': ", p_td.name);
        goto bail;
      } else {
        decoded_length-=sep_length;
        buff.set_pos(buff.get_pos()-sep_length);
      }
    }
    if (p_td.text->end_decode) {
      int tl;
      if ((tl=p_td.text->end_decode->match_begin(buff))<0) {
        if (no_err) decoded_length = -1;
        else TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TOKEN_ERR,
          "The specified token '%s' not found for '%s': ",
          (const char*)*(p_td.text->end_decode),p_td.name);
        goto bail;
      }
      decoded_length+=tl;
      buff.increase_pos(tl);
    }
    if (mand_field_num) decoded_length = -1;
bail:
    delete[] field_map;
    return decoded_length;
  } else { // record decoder
    int decoded_length=0;
    int decoded_field_length=0;
    size_t pos=buff.get_pos();
    boolean sep_found=FALSE;
    int sep_length=0;
    int ml=0;
    if (p_td.text->begin_decode) {
      int tl;
      if ((tl=p_td.text->begin_decode->match_begin(buff))<0) {
        if(no_err)return -1;
        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TOKEN_ERR,
          "The specified token '%s' not found for '%s': ",
          (const char*)*(p_td.text->begin_decode), p_td.name);
        return 0;
      }
      decoded_length+=tl;
      buff.increase_pos(tl);
    }
    if (p_td.text->end_decode) {
      limit.add_token(p_td.text->end_decode);
      ml++;
    }
    if (p_td.text->separator_decode) {
      limit.add_token(p_td.text->separator_decode);
      ml++;
    }

    int mand_field_num = 0;
    int opt_field_num  = 0;
    int last_man_index = 0;

    int field_cnt = get_count();
    int next_optional_idx = 0;
    const int* optional_indexes = get_optional_indexes();
    for (int i=0;i<field_cnt;i++) {
      boolean is_optional_field = optional_indexes && (optional_indexes[next_optional_idx]==i);
      if (is_optional_field) {
        get_at(i)->set_to_omit();
        opt_field_num++;
      } else {
        last_man_index=i+1;
        mand_field_num++;
      }
      if (is_optional_field) next_optional_idx++;
    }

    next_optional_idx = 0;
    for(int i=0;i<field_cnt;i++) {
      boolean is_optional_field = optional_indexes && (optional_indexes[next_optional_idx]==i);
      if (is_optional_field) {
        pos=buff.get_pos();
      }
      decoded_field_length = get_at(i)->TEXT_decode(*fld_descr(i),buff,limit,TRUE);
      if (decoded_field_length<0) {
        if (is_optional_field) {
          get_at(i)->set_to_omit();
          buff.set_pos(pos);
        } else {
          limit.remove_tokens(ml);
          if (no_err) return -1;
          TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TOKEN_ERR,
            "Error during decoding field '%s' for '%s': ",
            fld_descr(i)->name, p_td.name);
          return decoded_length;
        }
      } else {
        decoded_length+=decoded_field_length;
        if (last_man_index>(i+1)) {
          if (p_td.text->separator_decode) {
            int tl;
            if ((tl=p_td.text->separator_decode->match_begin(buff))<0) {
              if(is_optional_field) {
                get_at(i)->set_to_omit();
                buff.set_pos(pos);
                decoded_length-=decoded_field_length;
              } else {
                limit.remove_tokens(ml);
                if(no_err)return -1;
                TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TOKEN_ERR,
                  "The specified token '%s' not found for '%s': ",
                  (const char*)*(p_td.text->separator_decode),p_td.name);
                return decoded_length;
              }
            } else {
              decoded_length+=tl;
              buff.increase_pos(tl);
              sep_length=tl;
              sep_found=TRUE;
            }
          } else sep_found=FALSE;

        } else if (i==(field_cnt-1)) {
          sep_found=FALSE;
        } else {
          if (p_td.text->separator_decode) {
            int tl;
            if ((tl=p_td.text->separator_decode->match_begin(buff))<0) {
              if (is_optional_field) {
                if (p_td.text->end_decode) {
                  if ((tl=p_td.text->end_decode->match_begin(buff))!=-1) {
                    decoded_length+=tl;
                    buff.increase_pos(tl);
                    limit.remove_tokens(ml);
                    return decoded_length;
                  }
                } else if (limit.has_token(ml)) {
                  if ((tl=limit.match(buff,ml))==0) {
                    sep_found=FALSE;
                    break;
                  }
                } else break;
                get_at(i)->set_to_omit();
                buff.set_pos(pos);
                decoded_length-=decoded_field_length;
              } else {
                sep_found=FALSE;
                break;
              }
            } else {
              decoded_length+=tl;
              buff.increase_pos(tl);
              sep_length=tl;
              sep_found=TRUE;
            }
          } else {
            sep_found=FALSE;
            int tl;
            if (p_td.text->end_decode) {
              if ((tl=p_td.text->end_decode->match_begin(buff))!=-1) {
                decoded_length+=tl;
                buff.increase_pos(tl);
                limit.remove_tokens(ml);
                return decoded_length;
              }
            } else if (limit.has_token(ml)) {
              if ((tl=limit.match(buff,ml))==0) {
                sep_found=FALSE;
                break;
              }
            }
          }
        }
      }
      if (is_optional_field) next_optional_idx++;
    } // for i
    limit.remove_tokens(ml);
    if (sep_found) {
      buff.set_pos(buff.get_pos()-sep_length);
      decoded_length-=sep_length;
    }
    if (p_td.text->end_decode) {
      int tl;
      if ((tl=p_td.text->end_decode->match_begin(buff))<0) {
        if(no_err)return -1;
        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TOKEN_ERR,
          "The specified token '%s' not found for '%s': ",
          (const char*)*(p_td.text->end_decode),p_td.name);
        return decoded_length;
      }
      decoded_length+=tl;
      buff.increase_pos(tl);
    }
    return decoded_length;
  } // record decoder
}

const XERdescriptor_t* Record_Type::xer_descr(int /*field_index*/) const
{
  TTCN_error("Internal error: Record_Type::xer_descr() called.");
  return NULL;
}

char ** Record_Type::collect_ns(const XERdescriptor_t& p_td, size_t& num, bool& def_ns, unsigned int flavor) const
{
  const int field_cnt = get_count();
  // The USE-ORDER member is first, unless preempted by EMBED-VALUES
  const int uo_index = ((p_td.xer_bits & EMBED_VALUES) !=0);
  // Index of the first "normal" member (after E-V and U-O)
  const int start_at = uo_index + ((p_td.xer_bits & USE_ORDER) != 0);

  size_t num_collected = 0;
  // First, our own namespace. Sets num_collected to 0 or 1.
  // If it throws, nothing was allocated.
  char **collected_ns = Base_Type::collect_ns(p_td, num_collected, def_ns, flavor);

  try{
    // If the nil attribute will be written, add the control namespace
    boolean nil_attribute = (p_td.xer_bits & USE_NIL)
      && !get_at(field_cnt-1)->ispresent();

    if (nil_attribute) {
      collected_ns = (char**)Realloc(collected_ns, sizeof(char*) * ++num_collected);
      const namespace_t *c_ns = p_td.my_module->get_controlns();

      collected_ns[num_collected-1] = mprintf(" xmlns:%s='%s'", c_ns->px, c_ns->ns);
    }

    // Collect namespace declarations from all components (recursively).
    // This is extremely nasty, but we can't prosecute you for that.
    // (Monty Python - Crunchy Frog sketch). This whole thing is O(n^3). Yuck.
    for (int a = start_at; a < field_cnt; ++a) {
      size_t num_new = 0;
      boolean def_ns_1 = FALSE;
      char **new_namespaces = get_at(a)->collect_ns(*xer_descr(a), num_new, def_ns_1, flavor);
      merge_ns(collected_ns, num_collected, new_namespaces, num_new);
      def_ns = def_ns || def_ns_1;
      // merge_ns freed new_namespaces
    } // next field
  }
  catch (...) {
    // Probably a TC_Error thrown from the element's collect_ns(),
    // e.g. if encoding an unbound value.
    while (num_collected > 0) Free(collected_ns[--num_collected]);
    Free(collected_ns);
    throw;
  }

  num = num_collected;
  return collected_ns;
}

// FIXME some hashing should be implemented
int Record_Type::get_index_byname(const char *name, const char *uri) const {
  int num_fields = get_count();
  for (int i = 0; i < num_fields; ++i) {
    const XERdescriptor_t& xer = *xer_descr(i);
    if (check_name(name, xer, TRUE)
      && check_namespace(uri, xer)) return i;
  }
  return -1;
}

int Record_Type::XER_encode(const XERdescriptor_t& p_td, TTCN_Buffer& p_buf,
  unsigned int flavor, unsigned int flavor2, int indent, embed_values_enc_struct_t* emb_val_parent) const
{
  if (err_descr) {
    return XER_encode_negtest(err_descr, p_td, p_buf, flavor, flavor2, indent, 0);
  }
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error
      (TTCN_EncDec::ET_UNBOUND, "Encoding an unbound value.");
  }

  TTCN_EncDec_ErrorContext ec_0("Component '");
  TTCN_EncDec_ErrorContext ec_1;
  int encoded_length=(int)p_buf.get_len(); // how much is already in the buffer

  int exer = is_exer(flavor);
  if (exer && (p_td.xer_bits & EMBED_VALUES)) flavor |= XER_CANONICAL;
  const boolean indenting = !is_canonical(flavor);
  const int field_cnt = get_count();
  const int num_attributes = get_xer_num_attr();
  // The USE-ORDER member is first, unless preempted by EMBED-VALUES
  const int uo_index = ((p_td.xer_bits & EMBED_VALUES) !=0);
  // Index of the first "normal" member (after E-V and U-O)
  const int start_at = uo_index + ((p_td.xer_bits & USE_ORDER) != 0);
  const int first_nonattr = start_at + num_attributes;
  // start_tag_len is keeping track of how much was written at the end of the
  // start tag, i.e. the ">\n". This is used later to "back up" over it.
  int start_tag_len = 1 + indenting;
  // The EMBED-VALUES member, if applicable
  const Record_Of_Type* embed_values = 0;
  if (p_td.xer_bits & EMBED_VALUES) {
    embed_values = dynamic_cast<const Record_Of_Type*>(get_at(0));
    if (NULL == embed_values) {
      const OPTIONAL<Record_Of_Type>* const embed_opt = static_cast<const OPTIONAL<Record_Of_Type>*>(get_at(0));
      if(embed_opt->is_present()) {
        embed_values = &(*embed_opt)();
      }
    }
  }
  // The USE-ORDER member, if applicable
  const Record_Of_Type* const use_order = (p_td.xer_bits & USE_ORDER)
  ? static_cast<const Record_Of_Type*>(get_at(uo_index)) : 0;

  size_t num_collected = 0; // we use this to compute delay_close
  char **collected_ns = NULL;
  boolean def_ns = FALSE;
  if (exer) {
    if (indent == 0) { // top-level type
      collected_ns = collect_ns(p_td, num_collected, def_ns, flavor2);
    }
    else if ((flavor & DEF_NS_SQUASHED) && p_td.my_module && p_td.ns_index != -1) {
      const namespace_t * ns = p_td.my_module->get_ns((size_t)p_td.ns_index);
      // The default namespace has been squashed.
      // If we are in the default namespace, restore it.
      if (*ns->px == '\0') {
        collected_ns = Base_Type::collect_ns(p_td, num_collected, def_ns, flavor2);
      }
    }
  }

  // The type's own tag is omitted if we're doing E-XER,
  // and it's not the top-level type (XML must have a root element)
  // and it's either UNTAGGED or got USE_NIL or USE_TYPE or USE_UNION.
  boolean omit_tag = exer && (indent > 0)
    && ( (p_td.xer_bits & (UNTAGGED|XER_ATTRIBUTE))
      || (flavor & (USE_NIL|USE_TYPE_ATTR)));

  // If a default namespace is in effect (uri but no prefix) and the type
  // is unqualified, the default namespace must be canceled; otherwise
  // an XML tag without a ns prefix looks like it belongs to the def.namespace
  const boolean empty_ns_hack = exer && !omit_tag && (indent > 0)
    && (p_td.xer_bits & FORM_UNQUALIFIED)
    && (flavor & DEF_NS_PRESENT);

  // delay_close=true if there is stuff before the '>' of the start tag
  // (prevents writing the '>' which is built into the name).
  // This can only happen for EXER: if there are attributes or namespaces,
  // or either USE-NIL or USE-QNAME is set.
  boolean delay_close = exer && (num_attributes
    || empty_ns_hack // counts as having a namespace
    || (num_collected != 0)
    || (p_td.xer_bits & (USE_NIL|USE_QNAME))
    || (flavor & USE_NIL));

  size_t shorter = 0;

  if (!omit_tag) { /* write start tag */
    if (indenting) do_indent(p_buf, indent);
    /* name looks like this: "tagname>\n"
     * lose the \n if : not indenting or (single untagged(*) or attributes (*)) AND exer
     * lose the > if attributes are present (*) AND exer
     */
    p_buf.put_c('<');
    if (exer) write_ns_prefix(p_td, p_buf);
    p_buf.put_s((size_t)p_td.namelens[exer] - delay_close
      - (!indenting || delay_close || (exer && (p_td.xer_bits & HAS_1UNTAGGED))),
      (cbyte*)p_td.names[exer]);
  }
  else if (flavor & (USE_NIL|USE_TYPE_ATTR)) {
    // reopen the parent's start tag by overwriting the '>'
    size_t buf_len = p_buf.get_len();
    const unsigned char * const buf_data = p_buf.get_data();
    if (buf_data[buf_len - 1 - shorter] == '\n') ++shorter;
    if (buf_data[buf_len - 1 - shorter] == '>' ) ++shorter;

    if (shorter) {
      p_buf.increase_length(-shorter);
    }
    delay_close = TRUE;
  }

  int sub_len=0;
  // mask out extra flags we received, do not send them to the fields
  flavor &= XER_MASK;

  if (exer && (p_td.xer_bits & USE_QNAME)) { // QName trumps everything
    const Base_Type * const q_uri = get_at(0);
    if (q_uri->is_present()) {
      p_buf.put_s(11, (cbyte*)" xmlns:b0='");
      q_uri->XER_encode(*xer_descr(0), p_buf, flavor | XER_LIST, flavor2, indent+1, 0);
      p_buf.put_c('\'');
    }

    if (p_td.xer_bits & XER_ATTRIBUTE) begin_attribute(p_td, p_buf);
    else p_buf.put_c('>');

    if (q_uri->is_present()) {
      p_buf.put_s(3, (cbyte*)"b0:");
      sub_len += 3;
    }
    const Base_Type* const q_name = get_at(1);
    sub_len += q_name->XER_encode(*xer_descr(1), p_buf, flavor | XER_LIST, flavor2, indent+1, 0);
    if (p_td.xer_bits & XER_ATTRIBUTE) p_buf.put_c('\'');
  }
  else { // not USE-QNAME
    if (!exer && (p_td.xer_bits & EMBED_VALUES) && embed_values != NULL) {
      // The EMBED-VALUES member as an ordinary record of string
      sub_len += embed_values->XER_encode(*xer_descr(0), p_buf, flavor, flavor2, indent+1, 0);
    }

    if (!exer && (p_td.xer_bits & USE_ORDER)) {
      // The USE-ORDER member as an ordinary record of enumerated
      sub_len += use_order->XER_encode(*xer_descr(uo_index), p_buf, flavor, flavor2, indent+1, 0);
    }

    if (exer && (indent==0 || (flavor & DEF_NS_SQUASHED))) // write namespaces for toplevel only
    {
      for (size_t cur_coll = 0; cur_coll < num_collected; ++cur_coll) {
        p_buf.put_s(strlen(collected_ns[cur_coll]), (cbyte*)collected_ns[cur_coll]);
        Free(collected_ns[cur_coll]); // job done
      }
      Free(collected_ns);
    }

    if (def_ns) {
      flavor &= ~DEF_NS_SQUASHED;
      flavor |=  DEF_NS_PRESENT;
    }
    else if (empty_ns_hack) {
      p_buf.put_s(9, (cbyte*)" xmlns=''");
      flavor &= ~DEF_NS_PRESENT;
      flavor |=  DEF_NS_SQUASHED;
    }

    /* First all the attributes (not added to sub_len) */
    int i;
    for (i = start_at; i < first_nonattr; ++i) {
      boolean is_xer_attr_field = xer_descr(i)->xer_bits & XER_ATTRIBUTE;
      ec_1.set_msg("%s': ", fld_name(i)); // attr
      int tmp_len = get_at(i)->XER_encode(*xer_descr(i), p_buf, flavor, flavor2, indent+1, 0);
      if (is_xer_attr_field && !exer) sub_len += tmp_len; /* do not add if attribute and EXER */
    }

    // True if the "nil" attribute needs to be written.
    boolean nil_attribute = exer && (p_td.xer_bits & USE_NIL)
    && !get_at(field_cnt-1)->ispresent();

    // True if USE_ORDER is in effect and the "nil" attribute was written.
    // Then the record-of-enum for USE-ORDER will be empty.
    boolean early_to_bed = FALSE;

    if (nil_attribute) { // req. exer and USE_NIL
      const namespace_t *control_ns = p_td.my_module->get_controlns();
      p_buf.put_c(' ');
      p_buf.put_s(strlen(control_ns->px),
        (cbyte*)control_ns->px);
      p_buf.put_c(':');
      p_buf.put_s(10, (cbyte*)"nil='true'");
      if ((p_td.xer_bits & USE_ORDER)) early_to_bed = TRUE;
      // The whole content was omitted; nothing to do (and if we tried
      // to do it, we'd get an error for over-indexing a 0-length record-of).
    }

    if (delay_close && (!omit_tag || shorter)) {
      // Close the start tag left open. If indenting, also write a newline
      // unless USE-NIL in effect or there is a single untagged component.
      start_tag_len = 1 +
        ((p_td.xer_bits & (/*USE_NIL|*/HAS_1UNTAGGED)) ? 0 : indenting);
      p_buf.put_s(start_tag_len , (cbyte*)">\n");
    }

    if (exer && (p_td.xer_bits & EMBED_VALUES)) {
      /* write the first string */
      if (embed_values != NULL && embed_values->size_of() > 0) {
        sub_len += embed_values->get_at(0)->XER_encode(UNIVERSAL_CHARSTRING_xer_,
          p_buf, flavor | EMBED_VALUES, flavor2, indent+1, 0);
      }
    }

    const Record_Type *ordered = this; // the record affected by USE-ORDER

    // Index of the first non-attribute field of the record pointed to by
    // ordered, that is, the first field affected by USE-ORDER.
    size_t useorder_base = first_nonattr;

    int begin = i;
    int end   = field_cnt;
    if (exer && (p_td.xer_bits & USE_ORDER)) {
      const int to_send = use_order->size_of();
      // the length of the loop is determined by the length of use_order
      begin = 0;
      end = to_send;

      // Count the non-attribute optionals
      int n_optionals = 0;
      for (int B = optional_count() - 1; B >=+0; B--) {
        int oi = get_optional_indexes()[B];
        if (oi < first_nonattr) break;
        ++n_optionals;
      }

      int expected_max = field_cnt - first_nonattr;
      int expected_min = expected_max - n_optionals;

      if ((p_td.xer_bits & USE_NIL) && get_at(field_cnt-1)->ispresent()) {
        // The special case when USE_ORDER refers to the fields of a field,
        // not this record
        const Base_Type *last_optional = get_at(field_cnt-1);
        const Base_Type* inner = last_optional->get_opt_value();
        // it absolutely, positively has to be (derived from) Record_Type
        ordered = static_cast<const Record_Type*>(inner);
        useorder_base = ordered->get_xer_num_attr();
        begin = useorder_base;
        end   = ordered->get_count();

        expected_min = expected_max = ordered->get_count();
      }

      if (to_send > expected_max
        ||to_send < expected_min) {
        ec_1.set_msg("%s': ", fld_name(uo_index));
        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_CONSTRAINT,
          "Wrong number of USE-ORDER %d, must be %d..%d", to_send, expected_min, expected_max);
        begin = end = 0; // don't bother sending anything
      }
      else { // check no duplicates
        int *seen = new int [to_send];
        int num_seen = 0;
        for (int ei = 0; ei < to_send; ++ei) {
          const Base_Type *uoe = use_order->get_at(ei);
          const Enum_Type *enm = static_cast<const Enum_Type *>(uoe);
          int val = enm->as_int();
          for (int x = 0; x < num_seen; ++x) {
            if (val == seen[x]) { // complain
              ec_1.set_msg("%s': ", fld_name(uo_index));
              TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_CONSTRAINT,
                "Duplicate value for USE-ORDER");
              begin = end = 0; // don't bother sending anything
              goto trouble;
            }
          }
          seen[num_seen++] = val;
        }
        trouble:
        delete [] seen;
        // If the number is right and there are no duplicates, then carry on
      }
    }

    /* Then, all the non-attributes. Structuring the code like this depends on
     * all attributes appearing before all non-attributes (excluding
     * pseudo-members for USE-ORDER, etc.) */

    // early_to_bed can only be true if exer is true (transitive through nil_attribute)
    if (!early_to_bed) {
      embed_values_enc_struct_t* emb_val = 0;
      if (exer && (p_td.xer_bits & EMBED_VALUES) && 
          embed_values != NULL && embed_values->size_of() > 1) {
        emb_val = new embed_values_enc_struct_t;
        emb_val->embval_array = embed_values;
        emb_val->embval_index = 1;
        emb_val->embval_err = 0;
      }
      
      for ( i = begin; i < end; ++i ) {
        const Base_Type *uoe = 0; // "useOrder enum"
        const Enum_Type *enm = 0; // the enum value selecting the field
        if (exer && use_order) {
          uoe = use_order->get_at(i - begin);
          enm = static_cast<const Enum_Type *>(uoe);
        }
        if (p_td.xer_bits & UNTAGGED && i > 0 && exer && embed_values == NULL && 0 != emb_val_parent &&
          emb_val_parent->embval_index < emb_val_parent->embval_array->size_of()) {
          emb_val_parent->embval_array->get_at(emb_val_parent->embval_index)->XER_encode(UNIVERSAL_CHARSTRING_xer_
            , p_buf, flavor | EMBED_VALUES, flavor2, indent+1, 0);
          ++emb_val_parent->embval_index;
        }
        
        // "actual" index, may be perturbed by USE-ORDER
        int ai = !(exer && (p_td.xer_bits & USE_ORDER)) ? i :
        enm->as_int() + useorder_base;
        ec_1.set_msg("%s': ", ordered->fld_name(ai)); // non-attr

        const XERdescriptor_t& descr = *ordered->xer_descr(ai);
        sub_len += ordered->get_at(ai)->XER_encode(descr, p_buf,
          // Pass USE-NIL to the last field (except when USE-ORDER is also in effect,
          // because the tag-stripping effect of USE-NIL has been achieved
          // by encoding the sub-fields directly).
          flavor | ((exer && !use_order && (i == field_cnt-1)) ? (p_td.xer_bits & USE_NIL) : 0),
          flavor2, indent+!omit_tag, emb_val);

        // Now the next embed-values string (NOT affected by USE-ORDER!)
        if (exer && (p_td.xer_bits & EMBED_VALUES) && 0 != emb_val &&
            embed_values != NULL && emb_val->embval_index < embed_values->size_of() && ordered->get_at(ai)->is_present()) {
          embed_values->get_at(emb_val->embval_index)->XER_encode(UNIVERSAL_CHARSTRING_xer_
            , p_buf, flavor | EMBED_VALUES, flavor2, indent+1, 0);
          ++emb_val->embval_index;
        }
      } //for
      
      if (0 != emb_val) {
        if (embed_values != NULL && emb_val->embval_index < embed_values->size_of()) {
          ec_1.set_msg("%s': ", fld_name(0));
          TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_CONSTRAINT,
            "Too many EMBED-VALUEs specified: %d (expected %d or less)",
            embed_values->size_of(), emb_val->embval_index);
        }
        delete emb_val;
      }
    } // if (!early_to_bed)
  } // if (QNAME)

  if (!omit_tag) {
    if (sub_len) { // something was written, now an end tag
      if (indenting && !(exer && (p_td.xer_bits & (HAS_1UNTAGGED | USE_QNAME)))) {
        // The tags of the last optional member involved with USE_NIL
        // have been removed. If it was a simple type, the content was probably
        // written on a single line without anything resembling a close tag.
        // Do not indent our end tag in this case.
        switch ((int)(exer && (p_td.xer_bits & USE_NIL))) {
        case 1: {
          const unsigned char *buf_end = p_buf.get_data() + (p_buf.get_len()-1);
          if (buf_end[-1] != '>' || *buf_end != '\n') break;
          // If it does not look like an end tag, skip the indenting,
          // else fall through.
        }
        case 0:
          do_indent(p_buf, indent);
          break;
        }
      }
      p_buf.put_c('<');
      p_buf.put_c('/');
      if (exer) write_ns_prefix(p_td, p_buf);
      p_buf.put_s((size_t)p_td.namelens[exer]-!indenting, (cbyte*)p_td.names[exer]);
    }
    else { // need to generate an empty element tag
      p_buf.increase_length(-start_tag_len); // decrease length
      p_buf.put_s((size_t)2+indenting, (cbyte*)"/>\n");
    }
  }

  return (int)p_buf.get_len() - encoded_length;
}

// XERSTUFF Record_Type::encode_field
/** Helper for Record_Type::XER_encode_negtest
 *
 * Factored out because Record_Type::XER_encode (on which XER_encode_negtest
 * is based) calls the XER_encode method of the field in two places:
 * one for attributes, the other for elements.
 *
 * @param i index of the field
 * @param err_vals erroneous descriptor for the field
 * @param emb_descr deeper erroneous values
 * @param p_buf buffer containing the encoded value
 * @param sub_flavor flags
 * @param indent indentation level
 * @return the number of bytes generated
 */
int Record_Type::encode_field(int i,
  const Erroneous_values_t* err_vals, const Erroneous_descriptor_t* emb_descr,
  TTCN_Buffer& p_buf, unsigned int sub_flavor, unsigned int flavor2, int indent, embed_values_enc_struct_t* emb_val) const
{
  int enc_len = 0;
  TTCN_EncDec_ErrorContext ec;
  if (err_vals && err_vals->before) {
    if (err_vals->before->errval==NULL) TTCN_error(
      "internal error: erroneous before value missing");
    ec.set_msg("Erroneous value before component %s: ", fld_name(i));
    if (err_vals->before->raw) {
      enc_len += err_vals->before->errval->encode_raw(p_buf);
    } else {
      if (err_vals->before->type_descr==NULL) TTCN_error(
        "internal error: erroneous before typedescriptor missing");
      enc_len += err_vals->before->errval->XER_encode(
        *err_vals->before->type_descr->xer, p_buf, sub_flavor, flavor2, indent, 0);
    }
  }

  if (err_vals && err_vals->value) {
    if (err_vals->value->errval) { // replace
      ec.set_msg("Erroneous value for component %s: ", fld_name(i));
      if (err_vals->value->raw) {
        enc_len += err_vals->value->errval->encode_raw(p_buf);
      } else {
        if (err_vals->value->type_descr==NULL) TTCN_error(
          "internal error: erroneous value typedescriptor missing");
        enc_len += err_vals->value->errval->XER_encode(
          *err_vals->value->type_descr->xer, p_buf, sub_flavor, flavor2, indent, 0);
      }
    } // else -> omit
  } else {
    ec.set_msg("Component %s: ", fld_name(i));
    if (emb_descr) {
      enc_len += get_at(i)->XER_encode_negtest(emb_descr, *xer_descr(i), p_buf,
        sub_flavor, flavor2, indent, emb_val);
    } else {
      // the "real" encoder
      enc_len += get_at(i)->XER_encode(*xer_descr(i), p_buf,
        sub_flavor, flavor2, indent, emb_val);
    }
  }

  if (err_vals && err_vals->after) {
    if (err_vals->after->errval==NULL) TTCN_error(
      "internal error: erroneous after value missing");
    ec.set_msg("Erroneous value after component %s: ", fld_name(i));
    if (err_vals->after->raw) {
      enc_len += err_vals->after->errval->encode_raw(p_buf);
    } else {
      if (err_vals->after->type_descr==NULL) TTCN_error(
        "internal error: erroneous after typedescriptor missing");
      enc_len += err_vals->after->errval->XER_encode(
        *err_vals->after->type_descr->xer, p_buf, sub_flavor, flavor2, indent, 0);
    }
  }

  return enc_len;
}

// XERSTUFF Record_Type::XER_encode_negtest
int Record_Type::XER_encode_negtest(const Erroneous_descriptor_t* p_err_descr,
  const XERdescriptor_t& p_td, TTCN_Buffer& p_buf, unsigned int flavor, unsigned int flavor2, int indent,
  embed_values_enc_struct_t*) const
{
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error
      (TTCN_EncDec::ET_UNBOUND, "Encoding an unbound value.");
  }
  TTCN_EncDec_ErrorContext ec_0("Component '");
  TTCN_EncDec_ErrorContext ec_1;
  int encoded_length=(int)p_buf.get_len(); // how much is already in the buffer

  int exer = is_exer(flavor);
  if (exer && (p_td.xer_bits & EMBED_VALUES)) flavor |= XER_CANONICAL;
  const boolean indenting = !is_canonical(flavor);
  const int field_cnt = get_count();
  const int num_attributes = get_xer_num_attr();
  // The USE-ORDER member is first, unless preempted by EMBED-VALUES
  const int uo_index = ((p_td.xer_bits & EMBED_VALUES) !=0);
  // Index of the first "normal" member (after E-V and U-O)
  const int start_at = uo_index + ((p_td.xer_bits & USE_ORDER) != 0);
  const int first_nonattr = start_at + num_attributes;
  // start_tag_len is keeping track of how much was written at the end of the
  // start tag, i.e. the ">\n". This is used later to "back up" over it.
  int start_tag_len = 1 + indenting;
  // The EMBED-VALUES member, if applicable (always first)
  const Record_Of_Type* const embed_values = (p_td.xer_bits & EMBED_VALUES)
  ? static_cast<const Record_Of_Type*>(get_at(0)) : 0;
  // The USE-ORDER member, if applicable (first unless preempted by embed_vals)
  const Record_Of_Type* const use_order = (p_td.xer_bits & USE_ORDER)
  ? static_cast<const Record_Of_Type*>(get_at(uo_index)) : 0;

  int values_idx = 0;
  int edescr_idx = 0;

  size_t num_collected = 0; // we use this to compute delay_close
  char **collected_ns = NULL;
  boolean def_ns = FALSE;
  if (exer) {
    if (indent == 0) { // top-level type
      collected_ns = collect_ns(p_td, num_collected, def_ns, flavor2);
    }
    else if ((flavor & DEF_NS_SQUASHED) && p_td.my_module && p_td.ns_index != -1) {
      const namespace_t * ns = p_td.my_module->get_ns((size_t)p_td.ns_index);
      // The default namespace has been squashed.
      // If we are in the default namespace, restore it.
      if (*ns->px == '\0') {
        collected_ns = Base_Type::collect_ns(p_td, num_collected, def_ns, flavor2);
      }
    }
  }

  // The type's own tag is omitted if we're doing E-XER,
  // and it's not the top-level type (XML must have a root element)
  // and it's either UNTAGGED or got USE_NIL.
  boolean omit_tag = exer && indent
    && ( (p_td.xer_bits & (UNTAGGED|XER_ATTRIBUTE))
      || (flavor & (USE_NIL|USE_TYPE_ATTR)));

  // If a default namespace is in effect (uri but no prefix) and the type
  // is unqualified, the default namespace must be canceled; otherwise
  // an XML tag without a ns prefix looks like it belongs to the def.namespace
  const boolean empty_ns_hack = exer && !omit_tag && (indent > 0)
    && (p_td.xer_bits & FORM_UNQUALIFIED)
    && (flavor & DEF_NS_PRESENT);

  // delay_close=true if there is stuff before the '>' of the start tag
  // (prevents writing the '>' which is built into the name).
  // This can only happen for EXER: if there are attributes or namespaces,
  // or either USE-NIL or USE-QNAME is set.
  boolean delay_close = exer && (num_attributes
    || empty_ns_hack // counts as having a namespace
    || (num_collected != 0)
    || (p_td.xer_bits & (USE_NIL|USE_QNAME))
    || (flavor & USE_NIL));

  size_t shorter = 0;
  if (!omit_tag) { /* write start tag */
    if (indenting) do_indent(p_buf, indent);
    /* name looks like this: "tagname>\n"
     * lose the \n if : not indenting or (single untagged(*) or attributes (*)) AND exer
     * lose the > if attributes are present (*) AND exer
     */
    p_buf.put_c('<');
    if (exer) write_ns_prefix(p_td, p_buf);
    p_buf.put_s((size_t)p_td.namelens[exer] - delay_close
      - (!indenting || delay_close || (exer && (p_td.xer_bits & HAS_1UNTAGGED))),
      (cbyte*)p_td.names[exer]);
  }
  else if (flavor & USE_TYPE_ATTR) {
    // reopen the parent's tag
    size_t buf_len = p_buf.get_len();
    const unsigned char * const buf_data = p_buf.get_data();
    if (buf_data[buf_len - 1 - shorter] == '\n') ++shorter;
    if (buf_data[buf_len - 1 - shorter] == '>' ) ++shorter;

    if (shorter) {
      p_buf.increase_length(-shorter);
    }
    delay_close = TRUE;
  }

  int sub_len=0, tmp_len;
  // mask out extra flags we received, do not send them to the fields
  flavor &= XER_MASK;

  if (exer && (p_td.xer_bits & USE_QNAME)) {
    const Erroneous_values_t    * ev =
      p_err_descr->next_field_err_values(0, values_idx);
    const Erroneous_descriptor_t* ed =
      p_err_descr->next_field_emb_descr (0, edescr_idx);
    // At first, erroneous info for the first component (uri)

    TTCN_EncDec_ErrorContext ec;
    const Base_Type * const q_uri = get_at(0);

    if (ev && ev->before) {
      if (ev->before->errval==NULL) TTCN_error(
        "internal error: erroneous before value missing");
      ec.set_msg("Erroneous value before component #0: ");
      if (ev->before->raw) {
        sub_len += ev->before->errval->encode_raw(p_buf);
      } else {
        if (ev->before->type_descr==NULL) TTCN_error(
          "internal error: erroneous before typedescriptor missing");
        sub_len += ev->before->errval->XER_encode(
          *ev->before->type_descr->xer, p_buf, flavor, flavor2, indent, 0);
      }
    }

    if (ev && ev->value) {
      if (ev->value->errval) { // replace
        ec.set_msg("Erroneous value for component #0: ");
        if (ev->value->raw) {
          sub_len += ev->value->errval->encode_raw(p_buf);
        } else {
          if (ev->value->type_descr==NULL) TTCN_error(
            "internal error: erroneous value typedescriptor missing");
          sub_len += ev->value->errval->XER_encode(
            *ev->value->type_descr->xer, p_buf, flavor, flavor2, indent, 0);
        }
      } // else -> omit
    } else {
      ec.set_msg("Component #0: ");
      if (ed) {
        // universal charstring does not have components.
        // TTCN code which could have generated embedded erroneous descriptor
        // should have failed semantic analysis.
        TTCN_error("internal error: embedded descriptor unexpected");
      } else {
        // the "real" encoder
        if (q_uri->is_present()) {
          p_buf.put_s(11, (cbyte*)" xmlns:b0='");
          sub_len += q_uri->XER_encode(*xer_descr(0), p_buf, flavor | XER_LIST, flavor2, indent+1, 0);
          p_buf.put_c('\'');
        }
      }
    }

    if (ev && ev->after) {
      if (ev->after->errval==NULL) TTCN_error(
        "internal error: erroneous after value missing");
      ec.set_msg("Erroneous value after component #0: ");
      if (ev->after->raw) {
        sub_len += ev->after->errval->encode_raw(p_buf);
      } else {
        if (ev->after->type_descr==NULL) TTCN_error(
          "internal error: erroneous after typedescriptor missing");
        sub_len += ev->after->errval->XER_encode(
          *ev->after->type_descr->xer, p_buf, flavor, flavor2, indent, 0);
      }
    }

    if (p_td.xer_bits & XER_ATTRIBUTE) begin_attribute(p_td, p_buf);
    else p_buf.put_c('>');

    // Now switch to the second field (name)
    ev = p_err_descr->next_field_err_values(1, values_idx);
    ed = p_err_descr->next_field_emb_descr (1, edescr_idx);

    if (ev && ev->before) {
      if (ev->before->errval==NULL) TTCN_error(
        "internal error: erroneous before value missing");
      ec.set_msg("Erroneous value before component #1: ");
      if (ev->before->raw) {
        sub_len += ev->before->errval->encode_raw(p_buf);
      } else {
        if (ev->before->type_descr==NULL) TTCN_error(
          "internal error: erroneous before typedescriptor missing");
        sub_len += ev->before->errval->XER_encode(
          *ev->before->type_descr->xer, p_buf, flavor, flavor2, indent, 0);
      }
    }

    if (ev && ev->value) {
      if (ev->value->errval) { // replace
        ec.set_msg("Erroneous value for component #1: ");
        if (ev->value->raw) {
          sub_len += ev->value->errval->encode_raw(p_buf);
        } else {
          if (ev->value->type_descr==NULL) TTCN_error(
            "internal error: erroneous value typedescriptor missing");
          sub_len += ev->value->errval->XER_encode(
            *ev->value->type_descr->xer, p_buf, flavor, flavor2, indent, 0);
        }
      } // else -> omit
    } else {
      ec.set_msg("Component #1: ");
      if (ed) {
        // universal charstring does not have components
        TTCN_error("internal error: embedded descriptor unexpected");
      } else {
        // the "real" encoder
        if (q_uri->is_present()) {
          p_buf.put_s(3, (cbyte*)"b0:");
          sub_len += 3;
        }

        sub_len += get_at(1)->XER_encode(*xer_descr(1), p_buf, flavor | XER_LIST, flavor2, indent+1, 0);
      }
    }

    if (ev && ev->after) {
      if (ev->after->errval==NULL) TTCN_error(
        "internal error: erroneous after value missing");
      ec.set_msg("Erroneous value after component #1: ");
      if (ev->after->raw) {
        sub_len += ev->after->errval->encode_raw(p_buf);
      } else {
        if (ev->after->type_descr==NULL) TTCN_error(
          "internal error: erroneous after typedescriptor missing");
        sub_len += ev->after->errval->XER_encode(
          *ev->after->type_descr->xer, p_buf, flavor, flavor2, indent, 0);
      }
    }

    if (p_td.xer_bits & XER_ATTRIBUTE) p_buf.put_c('\'');
  }
  else { // not USE-QNAME
    if (!exer && (p_td.xer_bits & EMBED_VALUES)) {
      // The EMBED-VALUES member as an ordinary record of string
      sub_len += embed_values->XER_encode(*xer_descr(0), p_buf, flavor, flavor2, indent+1, 0);
    }

    if (!exer && (p_td.xer_bits & USE_ORDER)) {
      // The USE-ORDER member as an ordinary record of enumerated
      sub_len += use_order->XER_encode(*xer_descr(uo_index), p_buf, flavor, flavor2, indent+1, 0);
    }

    if (exer && (indent==0 || (flavor & DEF_NS_SQUASHED))) // write namespaces for toplevel only
    {
      for (size_t cur_coll = 0; cur_coll < num_collected; ++cur_coll) {
        p_buf.put_s(strlen(collected_ns[cur_coll]), (cbyte*)collected_ns[cur_coll]);
        Free(collected_ns[cur_coll]); // job done
      }
      Free(collected_ns);
    }

    if (def_ns) {
      flavor &= ~DEF_NS_SQUASHED;
      flavor |=  DEF_NS_PRESENT;
    }
    else if (empty_ns_hack) {
      p_buf.put_s(9, (cbyte*)" xmlns=''");
      flavor &= ~DEF_NS_PRESENT;
      flavor |=  DEF_NS_SQUASHED;
    }

    // True if the non-attribute fields need to be omitted;
    // e.g. if USE_ORDER is in effect and the "nil" attribute was written
    // (then the record-of-enum for USE-ORDER will be empty),
    // or "omit all after" was hit while processing attributes.
    boolean early_to_bed = FALSE;

    // First all the attributes (not added to sub_len)
    int i;
    for (i = start_at; i < first_nonattr; ++i) {
      const Erroneous_values_t    * ev =
        p_err_descr->next_field_err_values(i, values_idx);
      const Erroneous_descriptor_t* ed =
        p_err_descr->next_field_emb_descr(i, edescr_idx);

      if (i < p_err_descr->omit_before) continue;

      boolean is_xer_attr_field = xer_descr(i)->xer_bits & XER_ATTRIBUTE;
      ec_1.set_msg("%s': ", fld_name(i)); // attr

      tmp_len = encode_field(i, ev, ed, p_buf, flavor, flavor2, indent + !omit_tag, 0);

      if (is_xer_attr_field && !exer) sub_len += tmp_len; // do not add if attribute and EXER

      // omit_after value -1 becomes "very big"
      if ((unsigned int)i >= (unsigned int)p_err_descr->omit_after) {
        early_to_bed = TRUE; // no more fields to write
        break;
      }
    }

    // True if the "nil" attribute needs to be written.
    boolean nil_attribute = FALSE;
    // nil attribute unaffected by erroneous
    boolean nil_attribute_simple = FALSE;
    if (exer && (p_td.xer_bits & USE_NIL)) {
      nil_attribute = nil_attribute_simple = !get_at(field_cnt-1)->ispresent();

      if (p_err_descr->values_size > 0) // there is an erroneous "value := ..."
      {
        const Erroneous_values_t *ev_nil =
          p_err_descr->get_field_err_values(field_cnt-1);
        if (ev_nil && ev_nil->value) // value override for the last field
        {
          nil_attribute = (ev_nil->value->errval == NULL);
        }
      }
    }

    if (nil_attribute) { // req. exer and USE_NIL
      const namespace_t *control_ns = p_td.my_module->get_controlns();

      if (!nil_attribute_simple) {
        // It is likely that the declaration for namespace "xsi"
        // was not written. Do it now.
        p_buf.put_s(7, (cbyte*)" xmlns:");
        p_buf.put_s(strlen(control_ns->px), (cbyte*)control_ns->px);
        p_buf.put_s(2, (cbyte*)"='");
        p_buf.put_s(strlen(control_ns->ns), (cbyte*)control_ns->ns);
        p_buf.put_c('\'');
      }

      p_buf.put_c(' ');
      p_buf.put_s(strlen(control_ns->px), (cbyte*)control_ns->px);
      p_buf.put_c(':');
      p_buf.put_s(10, (cbyte*)"nil='true'");
      if ((p_td.xer_bits & USE_ORDER)) early_to_bed = TRUE;
      // The whole content was omitted; nothing to do (and if we tried
      // to do it, we'd get an error for over-indexing a 0-length record-of).
    }

    if (delay_close && (!omit_tag || shorter)) {
      // Close the start tag left open. If indenting, also write a newline
      // unless USE-NIL in effect or there is a single untagged component.
      start_tag_len = 1 +
        ((p_td.xer_bits & (/*USE_NIL|*/HAS_1UNTAGGED)) ? 0 : indenting);
      p_buf.put_s(start_tag_len , (cbyte*)">\n");
    }

    // Erroneous values for the embed_values member (if any).
    // Collected once but referenced multiple times.
    const Erroneous_descriptor_t* ed0 = NULL;
    int embed_values_val_idx = 0;
    int embed_values_descr_idx = 0;

    if (exer && (p_td.xer_bits & EMBED_VALUES)) {
      ed0 = p_err_descr->next_field_emb_descr(0, edescr_idx);

      // write the first string
      if (embed_values->size_of() > 0) {
        const Erroneous_values_t    * ev0_0 = NULL;
        const Erroneous_descriptor_t* ed0_0 = NULL;
        if (ed0) {
          ev0_0 = ed0->next_field_err_values(0, embed_values_val_idx);
          ed0_0 = ed0->next_field_emb_descr (0, embed_values_descr_idx);
        }
        sub_len += embed_values->encode_element(0, UNIVERSAL_CHARSTRING_xer_,
          ev0_0, ed0_0, p_buf, flavor | EMBED_VALUES, flavor2, indent+!omit_tag, 0);
      }
    }

    const Record_Type *ordered = this; // the record affected by USE-ORDER
    // By default it's this record, unless USE_NIL is _also_ in effect,
    // in which case it's the last member of this.

    // Index of the first non-attribute field of the record pointed to by
    // ordered, that is, the first field affected by USE-ORDER.
    size_t useorder_base = first_nonattr;

    int begin = i;
    int end   = field_cnt; // "one past", do not touch
    // by default, continue from the current field until the end, indexing this

    if (exer && (p_td.xer_bits & USE_ORDER)) {
      // the length of the loop is determined by the length of use_order
      const int to_send = use_order->size_of();

      // i will index all elements of the use_order member
      begin = 0;
      end = to_send;

      // Count the non-attribute optionals
      int n_optionals = 0;
      for (int B = optional_count() - 1; B >=+0; B--) {
        int oi = get_optional_indexes()[B];
        if (oi < first_nonattr) break;
        ++n_optionals;
      }

      int expected_min = field_cnt - first_nonattr - n_optionals;
      int expected_max = field_cnt - first_nonattr;

      if ((p_td.xer_bits & USE_NIL) && get_at(field_cnt-1)->ispresent()) {
        // The special case when USE_ORDER refers to the fields of a field,
        // not this record
        const Base_Type *last_optional = get_at(field_cnt-1);
        const Base_Type* inner = last_optional->get_opt_value();
        // it absolutely, positively has to be (derived from) Record_Type
        ordered = static_cast<const Record_Type*>(inner);
        useorder_base = ordered->get_xer_num_attr();
        begin = useorder_base;
        end   = ordered->get_count();

        expected_min = expected_max = ordered->get_count();
      }

      if (to_send > expected_max
        ||to_send < expected_min) {
        ec_1.set_msg("%s': ", fld_name(uo_index));
        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_CONSTRAINT,
          "Wrong number of USE-ORDER %d, must be %d..%d", to_send, expected_min, expected_max);
        early_to_bed = TRUE; // don't bother sending anything
      }
      else { // check no duplicates
        int *seen = new int [to_send];
        int num_seen = 0;
        for (int ei = 0; ei < to_send; ++ei) {
          const Base_Type *uoe = use_order->get_at(ei);
          const Enum_Type *enm = static_cast<const Enum_Type *>(uoe);
          int val = enm->as_int();
          for (int x = 0; x < num_seen; ++x) {
            if (val == seen[x]) { // complain
              ec_1.set_msg("%s': ", fld_name(uo_index));
              TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_CONSTRAINT,
                "Duplicate value for USE-ORDER");
              early_to_bed = TRUE; // don't bother sending anything
              goto trouble;
            }
          }
          seen[num_seen++] = val;
        }
        trouble:
        delete [] seen;
        // If the number is right and there are no duplicates, then carry on
      }
    } // endif(USE_ORDER)

    // Then, all the non-attributes. Structuring the code like this depends on
    // all attributes appearing before all non-attributes (excluding
    // pseudo-members for USE-ORDER, etc.)

    // This loop handles both the normal case (no USE_ORDER) when i indexes
    // fields of this record; and the USE_ORDER case (with or without USE-NIL).
    //
    // early_to_bed can only be true if exer is true (transitive through nil_attribute)
    if (!early_to_bed) {
      embed_values_enc_struct_t* emb_val = 0;
      if (exer && (p_td.xer_bits & EMBED_VALUES) && embed_values->size_of() > 1) {
        emb_val = new embed_values_enc_struct_t;
        emb_val->embval_array = embed_values;
        emb_val->embval_index = 1;
        emb_val->embval_err = ed0;
        emb_val->embval_err_val_idx = embed_values_val_idx;
        emb_val->embval_err_descr_idx = embed_values_descr_idx;
      }
      
      for ( i = begin; i < end; ++i ) {

        const Base_Type *uoe = 0; // "useOrder enum"
        const Enum_Type *enm = 0; // the enum value selecting the field

        // "actual" index, may be perturbed by USE-ORDER.
        // We use this value to index the appropriate record.
        int ai = i;

        const Erroneous_values_t    * ev = NULL;
        const Erroneous_descriptor_t* ed = NULL;
        if (exer && use_order) {
          // If USE-ORDER is in effect, it introduces a level of indirection
          // into the indexing of fields: "i" is used to select an element
          // of the use_order member (an enum), whose value is used to select
          // the field being encoded.
          uoe = use_order->get_at(i - begin);
          enm = static_cast<const Enum_Type *>(uoe);
          ai = enm->as_int() + useorder_base;

          // Because it is not guaranteed that ai will increase monotonically,
          // we can't use next_field_...().
          ev = p_err_descr->get_field_err_values(ai);
          ed = p_err_descr->get_field_emb_descr (ai);
        }
        else { // not USE-ORDER, sequential access
          ev = p_err_descr->next_field_err_values(ai, values_idx);
          ed = p_err_descr->next_field_emb_descr (ai, edescr_idx);
        }
        ec_1.set_msg("%s': ", ordered->fld_name(ai)); // non-attr

        if (ai < p_err_descr->omit_before) continue;

        // omit_after value -1 becomes "very big".
        if ((unsigned int)ai > (unsigned int)p_err_descr->omit_after) continue;
        // We can't skip all fields with break, because the next ai may be lower
        // than omit_after.

        sub_len += ordered->encode_field(ai, ev, ed, p_buf,
          // Pass USE-NIL to the last field (except when USE-ORDER is also in effect,
          // because the tag-stripping effect of USE-NIL has been achieved
          // by encoding the sub-fields directly).
          flavor | ((exer && !use_order && (i == field_cnt-1)) ? (p_td.xer_bits & USE_NIL) : 0),
          flavor2, indent + !omit_tag, emb_val);

        // Now the next embed-values string (NOT affected by USE-ORDER!)
        if (exer && (p_td.xer_bits & EMBED_VALUES) && 0 != emb_val &&
            emb_val->embval_index < embed_values->size_of()) {
          const Erroneous_values_t    * ev0_i = NULL;
          const Erroneous_descriptor_t* ed0_i = NULL;
          if (ed0) {
            ev0_i = ed0->next_field_err_values(emb_val->embval_index, emb_val->embval_err_val_idx);
            ed0_i = ed0->next_field_emb_descr (emb_val->embval_index, emb_val->embval_err_descr_idx);
          }
          embed_values->encode_element(emb_val->embval_index, UNIVERSAL_CHARSTRING_xer_,
            ev0_i, ed0_i, p_buf, flavor | EMBED_VALUES, flavor2, indent + !omit_tag, 0);
          ++emb_val->embval_index;
        }
      } //for
      if (0 != emb_val) {
        if (emb_val->embval_index < embed_values->size_of()) {
          ec_1.set_msg("%s': ", fld_name(0));
          TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_CONSTRAINT,
            "Too many EMBED-VALUEs specified: %d (expected %d or less)",
            embed_values->size_of(), emb_val->embval_index);
        }
        delete emb_val;
      }
    } // if (!early_to_bed)
  } // if (QNAME)


  if (!omit_tag) {
    if (sub_len) { // something was written, now an end tag
      if (indenting && !(exer && (p_td.xer_bits & (HAS_1UNTAGGED | USE_QNAME))))
        // The tags of the last optional member involved with USE_NIL
        // have been removed. If it was a simple type, the content was probably
        // written on a single line without anything resembling a close tag.
        // Do not indent our end tag in this case.
        switch ((int)(exer && (p_td.xer_bits & USE_NIL))) {
        case 1: {
          const unsigned char *buf_end = p_buf.get_data() + (p_buf.get_len()-1);
          if (buf_end[-1] != '>' || *buf_end != '\n') break;
          // If it does not look like an end tag, skip the indenting,
          // else fall through.
        }
        case 0:
          do_indent(p_buf, indent);
          break;
        }
      p_buf.put_c('<');
      p_buf.put_c('/');
      if (exer) write_ns_prefix(p_td, p_buf);
      p_buf.put_s((size_t)p_td.namelens[exer]-!indenting, (cbyte*)p_td.names[exer]);
    }
    else { // need to generate an empty element tag
      p_buf.increase_length(-start_tag_len); // decrease length
      p_buf.put_s((size_t)2+indenting, (cbyte*)"/>\n");
    }
  }

  return (int)p_buf.get_len() - encoded_length;
}

int Record_Type::XER_decode(const XERdescriptor_t& p_td, XmlReaderWrap& reader,
                            unsigned int flavor, unsigned int flavor2, embed_values_dec_struct_t* emb_val_parent)
{
  boolean exer = is_exer(flavor);
  int success, type;
  int depth=-1; // depth of the start tag
  unsigned long xerbits = p_td.xer_bits;
  if (flavor & XER_TOPLEVEL) xerbits &= ~UNTAGGED;
  const boolean own_tag = !(exer
    && ( (xerbits & (ANY_ELEMENT | UNTAGGED | XER_ATTRIBUTE))
      || (flavor & (USE_NIL | USE_TYPE_ATTR))));
  boolean tag_closed = (flavor & PARENT_CLOSED) != 0;
  // If the parent has USE-TYPE, our ATTRIBUTE members can be found
  // in the parent's tag (the reader is sitting on it).
  const boolean parent_tag = exer && ((flavor & USE_TYPE_ATTR) || (flavor2 & USE_NIL_PARENT_TAG));

  // Filter out flags passed by our parent. These are not for the fields.
  flavor &= XER_MASK; // also removes XER_TOPLEVEL
  flavor2 = XER_NONE; // Remove only bit: USE_NIL_PARENT_TAG (for now)

  const int field_cnt = get_count();
  const int num_attributes = get_xer_num_attr();

  // The index of potential "order" field, regardless of whether USE_ORDER
  // is in use or not.
  const int uo_index = ((p_td.xer_bits & EMBED_VALUES) !=0);

  // The first "non-special" field (skipping the USE-ORDER and EMBED-VALUES
  // fields); normal processing start at this field.
  const int start_at = uo_index + ((p_td.xer_bits & USE_ORDER) != 0);
  const int first_nonattr = start_at + num_attributes;
  
  // The index of the ANY-ATTRIBUTES member, if any
  int aa_index = -1;
  for (int k = 0; k < first_nonattr; ++k) {
    if (xer_descr(k)->xer_bits & ANY_ATTRIBUTES) {
      aa_index = k;
      if (!get_at(aa_index)->is_optional()) {
        static_cast<Record_Of_Type*>(get_at(aa_index))->set_size(0);
      }
      break; // there can be only one, 18.2.2
    }
  }

  if (own_tag) for (success=reader.Ok(); success==1; success=reader.Read()) {
    type = reader.NodeType();
    if (type==XML_READER_TYPE_ELEMENT) {
      verify_name(reader, p_td, exer);
      depth = reader.Depth();
      tag_closed = reader.IsEmptyElement();
      break;
    }
  }//for

  int i = 0;

  if (exer && (p_td.xer_bits & USE_QNAME)) { // QName trumps everything !
    // If element, it looks like this:
    // <name xmlns:b0="http://www.furniture.com">b0:table</name>
    // If attribute, it looks like this:
    // name='b0:table'

    if (p_td.xer_bits & XER_ATTRIBUTE) success = 1; // do nothing
    else for (success = reader.Read(); success == 1; success = reader.Read()) {
      type = reader.NodeType();
      if (type == XML_READER_TYPE_TEXT) break;
    }

    if (success == 1) {
      xmlChar *val = reader.NewValue();
      xmlChar *npfx = (xmlChar*)strchr((char*)val, ':');
      xmlChar *pfx;
      if (npfx != NULL) {
        *npfx++ = '\0'; // cut the string into two
        pfx = val;
      }
      else {
        npfx = val;
        pfx = NULL;
      }

      xmlChar *nsu = reader.LookupNamespace(pfx);

      OPTIONAL<UNIVERSAL_CHARSTRING> *q_prefix2 =
        static_cast<OPTIONAL<UNIVERSAL_CHARSTRING>*>(get_at(0));
      if (nsu) *q_prefix2 = (const char*)nsu;
      else      q_prefix2->set_to_omit(); // public in RT2 only

      UNIVERSAL_CHARSTRING *q_name2 = static_cast<UNIVERSAL_CHARSTRING*>(get_at(1));
      *q_name2 = (const char*)npfx;

      xmlFree(nsu);
      xmlFree(val);
    }
  }
  else { // not use-qname
    TTCN_EncDec_ErrorContext ec_0("Component '");
    TTCN_EncDec_ErrorContext ec_1;
    boolean usenil_attribute = FALSE; // true if found and said yes
    // If nillable and the nillable field is a record type, that has attributes
    // then it will become true, and skips the processing of the fields after
    boolean already_processed = FALSE;
    if (!exer) {
      if (!reader.IsEmptyElement()) reader.Read();
      // First, the (would-be) attributes (unaffected by USE-ORDER)
      for (i = 0; i < first_nonattr; i++) {
        ec_1.set_msg("%s': ", fld_name(i));
        get_at(i)->XER_decode(*xer_descr(i), reader, flavor, flavor2, 0);
      } // next field
    }
    else if (own_tag || parent_tag) { // EXER and not UNTAGGED: do attributes
      // Prepare for lack of attributes.
      // Fields with defaultForEmpty get the D-F-E value, optional get omit.
      for (i = start_at; i < first_nonattr; i++) {
        Base_Type &fld = *get_at(i);
        const XERdescriptor_t& xd = *xer_descr(i);
        if (xd.dfeValue) {
          if (fld.is_optional()) {
            fld.set_to_present();
            fld.get_opt_value()->set_value(xd.dfeValue);
          }
          else fld.set_value(xd.dfeValue);
        }
        else if (fld.is_optional()) fld.set_to_omit();
      }

      int num_aa = 0; // index into the ANY-ATTRIBUTE member

      const namespace_t *control_ns = 0;
      if (parent_tag || (p_td.xer_bits & USE_NIL)) {
        // xsi:type  or                  xsi:nil
        control_ns = p_td.my_module->get_controlns();
      }

      /* * * * * * * * * Attributes * * * * * * * * * * * * * */
      if(parent_tag && reader.NodeType() == XML_READER_TYPE_ATTRIBUTE) {
        success = reader.Ok();
      } else {
        success = reader.MoveToFirstAttribute();
      }
      for (;
        success == 1 && reader.NodeType() == XML_READER_TYPE_ATTRIBUTE;
        success = reader.AdvanceAttribute())
      {
        if (reader.IsNamespaceDecl()) {
          continue; // namespace declarations are handled for us by libxml2
        }

        const char *attr_name = (const char*)reader.LocalName();
        const char *ns_uri    = (const char*)reader.NamespaceUri();
        int field_index = get_index_byname(attr_name, ns_uri);
        if (field_index != -1) {
          // There is a field. Let it decode the attribute.
          ec_1.set_msg("%s': ", fld_name(field_index));
          get_at(field_index)->XER_decode(*xer_descr(field_index), reader, flavor, flavor2, 0);
          continue;
        }

        // Attribute not found. It could be the "nil" attribute
        if (p_td.xer_bits & USE_NIL) {
          const char *prefix = (const char*)reader.Prefix();
          // prefix may be NULL, control_ns->px is never NULL or empty
          if (prefix && !strcmp(prefix, control_ns->px)
            && !strcmp((const char*)reader.LocalName(), "nil"))
          { // It is the "nil" attribute
            const char *value = (const char*)reader.Value();
            if (value) {
              if (!strcmp(value, "1") || !strcmp(value, "true")) {
                // The field affected by USE-NIL is always the last one
                get_at(field_cnt-1)->set_to_omit();
                usenil_attribute = TRUE;
              } // true
            } // if value

            continue;
          } // it is the "nil" attribute
          // else, let the nillable field decode the next attributes, it is possible
          // that it belongs to him
          get_at(field_cnt-1)->XER_decode(*xer_descr(field_cnt-1), reader, flavor | USE_NIL, flavor2 | USE_NIL_PARENT_TAG, 0);
          already_processed = TRUE;
          break;
        } // type has USE-NIL
        
          const char *prefix = (const char*)reader.Prefix();
          // prefix may be NULL
          if (prefix && !strcmp((const char*)reader.LocalName(), "type")) {
            continue; // xsi:type has been processed by the parent
          }
        
        if (aa_index >= 0) {
          ec_1.set_msg("%s': ", fld_name(aa_index));
          TTCN_EncDec_ErrorContext ec_2("Attribute %d: ", num_aa);
          // We have a component with ANY-ATTRIBUTE. It must be a record of
          // UNIVERSAL_CHARSTRING. Add the attribute to it.
          Record_Of_Type *aa = 0;
          if (get_at(aa_index)->is_optional()) {
            if (num_aa == 0) {
              get_at(aa_index)->set_to_present();
            }
            aa = static_cast<Record_Of_Type*>(get_at(aa_index)->get_opt_value());
          }
          else {
            aa = static_cast<Record_Of_Type*>(get_at(aa_index));
          }
          UNIVERSAL_CHARSTRING *new_elem = static_cast<UNIVERSAL_CHARSTRING *>
          (aa->get_at(num_aa++));

          // Construct the AnyAttributeFormat (X.693amd1, 18.2.6)
          TTCN_Buffer aabuf;
          const xmlChar *name = reader.LocalName();
          const xmlChar *val  = reader.Value();
          const xmlChar *uri  = reader.NamespaceUri();
          
          if (xer_descr(aa_index)->xer_bits & (ANY_FROM | ANY_EXCEPT)) {
            check_namespace_restrictions(*xer_descr(aa_index), (const char*)uri);
          }
          // We don't care about reader.Prefix()
          // Using strlen to count UTF8 bytes, not characters
          aabuf.put_s(uri ? strlen((const char*)uri) : 0, uri);
          if (uri && *uri) aabuf.put_c(' ');
          aabuf.put_s(name ? strlen((const char*)name) : 0, name);
          aabuf.put_c('=');
          aabuf.put_c('"');
          aabuf.put_s(val ? strlen((const char*)val) : 0, val);
          aabuf.put_c('"');
          new_elem->decode_utf8(aabuf.get_len(), aabuf.get_data());

          continue;
        }
        
        // Lastly check for the xsi:schemaLocation attribute, this does not
        // affect TTCN-3, but it shouldn't cause a DTE
        if (reader.LocalName() && !strcmp((const char*)reader.LocalName(), "schemaLocation")) {
          if (!control_ns) {
            control_ns = p_td.my_module->get_controlns();
          }
          if (reader.Prefix() && !strcmp((const char*)reader.Prefix(), control_ns->px)) {
            continue;
          }
        }
        
        // Nobody wanted the attribute. That is an error.
        ec_0.set_msg(" "); ec_1.set_msg(" ");
        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG,
          "Unexpected attribute '%s', ns  '%s'", attr_name,
          ns_uri ? ns_uri : "");
      } // next attribute

      // Now check that all mandatory attributes have been set
      for (i = start_at; i < first_nonattr; ++i) {
        Base_Type * fld = get_at(i);
        if (fld->is_optional()) continue; // field is allowed to be unset
        if (!fld->is_bound()) {
          TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG,
            "Missing attribute '%s'", this->fld_name(i));
        }
      }

      i = first_nonattr; // finished with attributes
      // AdvanceAttribute did MoveToElement. Move into the content (if any),
      // except when the reader is already moved in(already_processed).
      if (!reader.IsEmptyElement() && !already_processed) reader.Read();
    } // end if (own_tag)
    
    /* * * * * * * * Non-attributes (elements) * * * * * * * * * * * */
    embed_values_dec_struct_t* emb_val = 0;
    boolean emb_val_optional = FALSE;
    if (exer && (p_td.xer_bits & EMBED_VALUES)) {
      emb_val = new embed_values_dec_struct_t;
      emb_val->embval_array = dynamic_cast<Record_Of_Type*>(get_at(0));
      if (NULL == emb_val->embval_array) {
        OPTIONAL<PreGenRecordOf::PREGEN__RECORD__OF__UNIVERSAL__CHARSTRING>* embed_value = static_cast<OPTIONAL<PreGenRecordOf::PREGEN__RECORD__OF__UNIVERSAL__CHARSTRING>*>(get_at(0));
        embed_value->set_to_present();
        emb_val->embval_array = static_cast<Record_Of_Type*>((*embed_value).get_opt_value());
        emb_val_optional = TRUE;
      }
      emb_val->embval_array->set_size(0);
      emb_val->embval_index = 0;
    }

    if (exer && (p_td.xer_bits & USE_ORDER)) {
      // Set all optional fields to omit because their respective XER_decode
      // will not be run (and will stay unbound) if the value is missing.
      int n_optionals = 0;
      for (int B = optional_count() - 1; B >=+0; B--) {
        int oi = get_optional_indexes()[B];
        if (oi < first_nonattr) break;
        get_at(oi)->set_to_omit();
        ++n_optionals;
      }

      Record_Of_Type *use_order = static_cast<Record_Of_Type*>(get_at(uo_index));
      // Initialize the use_order field to empty. Let it grow on demand.
      // (setting it to the minimum acceptable size may leave unbound elements
      // if the XML was incomplete).
      use_order->set_size(0);

      // Nothing to order if there are no child elements
      if (!tag_closed) {
        Record_Type *jumbled = this; // the record affected by USE_ORDER
        int begin = first_nonattr;
        int end   = field_cnt; // "one past"
        if (p_td.xer_bits & USE_NIL) {
          Base_Type *last_optional = get_at(field_cnt-1);
          if (!usenil_attribute) { // exer known true
            last_optional->set_to_present();
            jumbled = static_cast<Record_Type*>(last_optional->get_opt_value());
            // We will operate on the members of last_optional,
            // effectively bypassing last_optional->XER_decode() itself.
            begin = 0;
            end   = jumbled->get_count();
            ec_1.set_msg("%s': ", fld_name(field_cnt-1));
          }
        }
        if (num_attributes > 0
          && first_nonattr != field_cnt
          && i == first_nonattr - 1) { // exer known true
          // If there were attributes and their processing just finished,
          // the reader is positioned on the start tag of the record.
          // Move ahead, unless there are no non-attribute fields.
          reader.Read();
        }
        // Then, the non-attributes

        // The index runs over the members affected by USE-ORDER.
        // This is [first_nonattr,field_cnt) unless USE-NIL is involved,
        // in which case it's [0,optional_sequence::field_cnt)
        int *seen = new int[end-begin];
        int num_seen = 0;
        int last_any_elem = begin - 1;
        // The index of the latest embedded value can change outside of this function
        // (if the field is an untagged record of), in this case the next value should
        // be ignored, as it's already been handled by the record of
        int last_embval_index = 0;
        boolean early_exit = FALSE;
        for (i = begin; i < end; i++) {
          for (success = reader.Ok(); success == 1; success = reader.Read()) {
            type = reader.NodeType();
            if (0 != emb_val && reader.NodeType()==XML_READER_TYPE_TEXT) {
              UNIVERSAL_CHARSTRING emb_ustr((const char*)reader.Value());
              emb_val->embval_array->get_at(emb_val->embval_index)->set_value(&emb_ustr);
            }
            // The non-attribute components must not be UNTAGGED
            if (type == XML_READER_TYPE_ELEMENT) break;
            if (type == XML_READER_TYPE_END_ELEMENT) {
              early_exit = TRUE;
              break;
            }
          }
          if (0 != emb_val) {
            if (last_embval_index == emb_val->embval_index) {
              ++emb_val->embval_index;
            }
            last_embval_index = emb_val->embval_index;
          }
          if (success != 1 || early_exit) break;
          const char *name = (const char *)reader.LocalName();
          boolean field_name_found = FALSE;
          // Find out which member it is.
          // FIXME some hashing should be implemented
          for (int k = begin; k < end; k++) {
            if (!(jumbled->xer_descr(k)->xer_bits & ANY_ELEMENT) &&
                check_name(name, *jumbled->xer_descr(k), 1)) {
              ec_1.set_msg("%s': ", jumbled->fld_name(k));

              // Check for the same field being decoded twice.
              // We can't use the field's is_bound()/is_present(),
              // because the field may be bound on input, e.g. for
              // prototype(fast) or prototype(backtrack).
              int in_dex = k - begin;
              for (int o = 0; o < num_seen ;++o) {
                if (in_dex == seen[o]) TTCN_EncDec_ErrorContext::error(
                  TTCN_EncDec::ET_INVAL_MSG, "Duplicate element");
              }
              seen[num_seen++] = in_dex;
              // Set the next use-order member.
              // Non-const get_at creates the object in the record-of.
              static_cast<Enum_Type*>(use_order->get_at(i - begin))
              ->from_int(in_dex);
              Base_Type *b = jumbled->get_at(k);
              b->XER_decode(*jumbled->xer_descr(k), reader, flavor, flavor2, emb_val);
              field_name_found = TRUE;
              break;
            }
          }
          if (!field_name_found) {
            // Check the anyElement fields
            for (int k = last_any_elem + 1; k < end; k++) {
              if (jumbled->xer_descr(k)->xer_bits & ANY_ELEMENT) {
                ec_1.set_msg("%s': ", jumbled->fld_name(k));

                // Check for the same field being decoded twice.
                // We can't use the field's is_bound()/is_present(),
                // because the field may be bound on input, e.g. for
                // prototype(fast) or prototype(backtrack).
                int in_dex = k - begin;
                for (int o = 0; o < num_seen ;++o) {
                  if (in_dex == seen[o]) TTCN_EncDec_ErrorContext::error(
                    TTCN_EncDec::ET_INVAL_MSG, "Duplicate element");
                }
                seen[num_seen++] = in_dex;
                // Set the next use-order member.
                // Non-const get_at creates the object in the record-of.
                static_cast<Enum_Type*>(use_order->get_at(i - begin))
                ->from_int(in_dex);
                Base_Type *b = jumbled->get_at(k);
                b->XER_decode(*jumbled->xer_descr(k), reader, flavor, flavor2, emb_val);
                last_any_elem = k;
                field_name_found = TRUE;
                break;
              }
            }
          }
          if (!field_name_found) {
            TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG,
              "Bad XML tag '%s' instead of a valid field", name);
            break;
          }
        } // next field
        if (0 != emb_val) {
          if (reader.NodeType()==XML_READER_TYPE_TEXT) {
            UNIVERSAL_CHARSTRING emb_ustr((const char*)reader.Value());
            emb_val->embval_array->get_at(emb_val->embval_index)->set_value(&emb_ustr);
          }
          if (last_embval_index == emb_val->embval_index) {
            ++emb_val->embval_index;
          }
        }
        delete [] seen;
        ec_1.set_msg(" "); // no active component
        ec_0.set_msg(" ");

        // Check that we collected the required number of children
        int num_collected = use_order->size_of();
        if (p_td.xer_bits & USE_NIL) {
          int expected = usenil_attribute ? 0 : jumbled->get_count();
          if (num_collected != expected) {
            TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INCOMPL_MSG,
              "Incorrect number of fields %d, expected %d",
              num_collected, expected);
          }
        }
        else {
          if (num_collected < field_cnt - first_nonattr - n_optionals
            ||num_collected > field_cnt - first_nonattr) {
            TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INCOMPL_MSG,
              "Wrong number of fields! size = %d, expected %d..%d",
              use_order->size_of(), field_cnt - first_nonattr - n_optionals,
              field_cnt - first_nonattr);
          }
        }
      } // not empty element
    }
    else { // not USE-ORDER, simpler code   
      if (usenil_attribute) {
        reader.MoveToElement(); // value absent, nothing more to do
      } else {
        // The index of the latest embedded value can change outside of this function
        // (if the field is a untagged record of), in this case the next value should
        // be ignored, as it's already been handled by the record of
        // Omitted fields can also reset this value
        int last_embval_index = 0;
        for (; i<field_cnt; i++) {
          if (0 != emb_val) {
            if (reader.NodeType()==XML_READER_TYPE_TEXT) {
              UNIVERSAL_CHARSTRING emb_ustr((const char*)reader.Value());
              emb_val->embval_array->get_at(emb_val->embval_index)->set_value(&emb_ustr);
            }
            if (last_embval_index == emb_val->embval_index) {
              ++emb_val->embval_index;
            }
            last_embval_index = emb_val->embval_index;
          } else if (p_td.xer_bits & UNTAGGED && 0 != emb_val_parent) {
            if (reader.NodeType()==XML_READER_TYPE_TEXT) {
              UNIVERSAL_CHARSTRING emb_ustr((const char*)reader.Value());
              emb_val_parent->embval_array->get_at(emb_val_parent->embval_index)->set_value(&emb_ustr);
            }
            if (last_embval_index == emb_val_parent->embval_index) {
              ++emb_val_parent->embval_index;
            }
            last_embval_index = emb_val_parent->embval_index;
          }
          ec_1.set_msg("%s': ", fld_name(i));
          if (exer && i==field_cnt-1 && p_td.dfeValue && reader.IsEmptyElement()) {
            get_at(i)->set_value(p_td.dfeValue);
          }
          else {
            // In case the field is an optional anyElement -> check if it should be omitted
            boolean optional_any_elem_check = TRUE;
            if (get_at(i)->is_optional() && (xer_descr(i)->xer_bits & ANY_ELEMENT)) {
              // The "anyElement" coding instruction can only be applied to a universal charstring field
              OPTIONAL<UNIVERSAL_CHARSTRING>* opt_field = dynamic_cast<OPTIONAL<UNIVERSAL_CHARSTRING>*>(get_at(i));
              if (opt_field) {
                const char* next_field_name = NULL;
                if (i < field_cnt - 1) {
                  next_field_name = fld_name(i + 1);
                }
                optional_any_elem_check = opt_field->XER_check_any_elem(reader, next_field_name, tag_closed);
              }
            }
            if (optional_any_elem_check && !already_processed) {
              int new_flavor = flavor ;
              if (i == field_cnt-1) new_flavor |= (p_td.xer_bits & USE_NIL);
              if (tag_closed)       new_flavor |= PARENT_CLOSED;

              get_at(i)->XER_decode(*xer_descr(i), reader, new_flavor, flavor2, emb_val);
              if (!get_at(i)->is_optional() && get_at(i)->is_bound()) {
                // Remove XER_OPTIONAL when we found a non optional field which is bound
                flavor &= ~XER_OPTIONAL;
              }
            }
          }
          if (!get_at(i)->is_present()) {
            // there was no new element, the last embedded value is for the next field
            // (or the end of the record if this is the last field) 
            last_embval_index = -1;
          }
        } // next field
        if (0 != emb_val) {
          if (reader.NodeType()==XML_READER_TYPE_TEXT) {
            UNIVERSAL_CHARSTRING emb_ustr((const char*)reader.Value());
            emb_val->embval_array->get_at(emb_val->embval_index)->set_value(&emb_ustr);
          }
          if (last_embval_index == emb_val->embval_index) {
            ++emb_val->embval_index;
          }
        }
      }
    } // if use-order

    if (0 != emb_val) {
      boolean all_unbound = TRUE;
      static const UNIVERSAL_CHARSTRING emptystring(0, (const char*)NULL);
      for (int j = 0; j < emb_val->embval_index; ++j) {
        if (!emb_val->embval_array->get_at(j)->is_bound()) {
          emb_val->embval_array->get_at(j)->set_value(&emptystring);
        }else if((static_cast<const UNIVERSAL_CHARSTRING*>(emb_val->embval_array->get_at(j)))->lengthof() !=0) {
          all_unbound = FALSE;
        }
      }
      if(emb_val_optional && all_unbound){
        static_cast<OPTIONAL<PreGenRecordOf::PREGEN__RECORD__OF__UNIVERSAL__CHARSTRING>*>(get_at(0))->set_to_omit();
      }
      delete emb_val;
    } // if embed-values

  } // if use-qname
  
  // Check if every non-optional field has been set
  for (i = 0; i < field_cnt; ++i) {
    if (!get_at(i)->is_optional() && !get_at(i)->is_bound()) {
      if (flavor & XER_OPTIONAL) {
        // If there is a non optional field which is unbound and we are optional
        // then set to omit. Test: RecordOmit
        clean_up();
        return -1;
      }
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INCOMPL_MSG,
        "No data found for non-optional field '%s'", fld_name(i));
    }
  }
  
  if (own_tag) {
    // We had our start tag. Then our fields did their thing.
    // Now we expect the end tag. And it better be our end tag!
    int current_depth;
    for (success = reader.Ok(); success == 1; success = reader.Read()) {
      type = reader.NodeType();
      current_depth = reader.Depth();
      if (current_depth > depth) {
        if (XML_READER_TYPE_ELEMENT == type) {
          // We found a deeper start tag; it was not processed at all.
          // That is an error (maybe we should report error for all node types
          // except TEXT and WHITESPACE, not just ELEMENT).
          TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TAG,
            "Unprocessed XML tag `%s'", (const char *)reader.Name());
        }

        continue; // go past hoping that our end tag will arrive eventually
      }
      else if (current_depth == depth) { // at our level
        if (XML_READER_TYPE_ELEMENT == type) {
          verify_name(reader, p_td, exer);
          if (reader.IsEmptyElement()) {
            // FIXME this shouldn't really be possible;
            // only an empty record should be encoded as an empty element,
            // but those are implemented by Empty_Record_Type, not Record_Type.
            reader.Read(); // one last time
            break;
          }
        }
        // If we find an end tag at the right depth, it must be ours
        else if (XML_READER_TYPE_END_ELEMENT == type) {
          verify_end(reader, p_td, depth, exer);
          reader.Read();
          break;
        }
      }
      else {   //current_depth < depth; something has gone horribly wrong
        break; // better quit before we do further damage
        // Don't report an error; every enclosing type would do so,
        // spewing the same message over and over.
      }
    } // next
  }
  return 1; // decode successful
}

int Record_Type::JSON_encode(const TTCN_Typedescriptor_t& p_td, JSON_Tokenizer& p_tok) const
{
  if (err_descr) {		
    return JSON_encode_negtest(err_descr, p_td, p_tok);		
  }

  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound %s value.", is_set() ? "set" : "record");
    return -1;
  }
  
  if (NULL != p_td.json && p_td.json->as_value) {
    // if 'as value' is set, then the record/set has only one field,
    // encode that without any brackets or field names
    return get_at(0)->JSON_encode(*fld_descr(0), p_tok);
  }
  
  int enc_len = p_tok.put_next_token(JSON_TOKEN_OBJECT_START, NULL);
  
  int field_count = get_count();
  for (int i = 0; i < field_count; ++i) {
    boolean metainfo_unbound = NULL != fld_descr(i)->json && fld_descr(i)->json->metainfo_unbound;
    if ((NULL != fld_descr(i)->json && fld_descr(i)->json->omit_as_null) || 
        get_at(i)->is_present() || metainfo_unbound) {
      const char* field_name = (NULL != fld_descr(i)->json && NULL != fld_descr(i)->json->alias) ?
        fld_descr(i)->json->alias : fld_name(i);
      enc_len += p_tok.put_next_token(JSON_TOKEN_NAME, field_name);
      if (metainfo_unbound && !get_at(i)->is_bound()) {
        enc_len += p_tok.put_next_token(JSON_TOKEN_LITERAL_NULL);
        char* metainfo_str = mprintf("metainfo %s", field_name);
        enc_len += p_tok.put_next_token(JSON_TOKEN_NAME, metainfo_str);
        Free(metainfo_str);
        enc_len += p_tok.put_next_token(JSON_TOKEN_STRING, "\"unbound\"");
      }
      else {
        enc_len += get_at(i)->JSON_encode(*fld_descr(i), p_tok);
      }
    }
  }
  
  enc_len += p_tok.put_next_token(JSON_TOKEN_OBJECT_END, NULL);
  return enc_len;
}

int Record_Type::JSON_encode_negtest(const Erroneous_descriptor_t* p_err_descr,
                                     const TTCN_Typedescriptor_t& p_td,
                                     JSON_Tokenizer& p_tok) const 
{
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound %s value.", is_set() ? "set" : "record");
    return -1;
  }
  
  boolean as_value = NULL != p_td.json && p_td.json->as_value;
  
  int enc_len = as_value ? 0 : p_tok.put_next_token(JSON_TOKEN_OBJECT_START, NULL);
  
  int values_idx = 0;
  int edescr_idx = 0;
  
  int field_count = get_count();
  for (int i = 0; i < field_count; ++i) {
    if (-1 != p_err_descr->omit_before && p_err_descr->omit_before > i) {
      continue;
    }
    
    const Erroneous_values_t* err_vals = p_err_descr->next_field_err_values(i, values_idx);
    const Erroneous_descriptor_t* emb_descr = p_err_descr->next_field_emb_descr(i, edescr_idx);
    
    if (!as_value && NULL != err_vals && NULL != err_vals->before) {
      if (NULL == err_vals->before->errval) {
        TTCN_error("internal error: erroneous before value missing");
      }
      if (err_vals->before->raw) {
        enc_len += err_vals->before->errval->JSON_encode_negtest_raw(p_tok);
      } else {
        if (NULL == err_vals->before->type_descr) {
          TTCN_error("internal error: erroneous before typedescriptor missing");
        }
        // it's an extra field, so use the erroneous type's name as the field name
        enc_len += p_tok.put_next_token(JSON_TOKEN_NAME, err_vals->before->type_descr->name);
        enc_len += err_vals->before->errval->JSON_encode(*(err_vals->before->type_descr), p_tok);
      }
    }
    
    const char* field_name = (NULL != fld_descr(i)->json && NULL != fld_descr(i)->json->alias) ?
      fld_descr(i)->json->alias : fld_name(i);
    if (NULL != err_vals && NULL != err_vals->value) {
      if (NULL != err_vals->value->errval) {
        if (err_vals->value->raw) {
          enc_len += err_vals->value->errval->JSON_encode_negtest_raw(p_tok);
        } else {
          if (NULL == err_vals->value->type_descr) {
            TTCN_error("internal error: erroneous before typedescriptor missing");
          }
          // only replace the field's value, keep the field name
          if (!as_value) {
            enc_len += p_tok.put_next_token(JSON_TOKEN_NAME, field_name);
          }
          enc_len += err_vals->value->errval->JSON_encode(*(err_vals->value->type_descr), p_tok);
        }
      }
    } else {
      boolean metainfo_unbound = NULL != fld_descr(i)->json && fld_descr(i)->json->metainfo_unbound;
      if ((NULL != fld_descr(i)->json && fld_descr(i)->json->omit_as_null) || 
          get_at(i)->is_present() || metainfo_unbound || as_value) {
        if (!as_value) {
          enc_len += p_tok.put_next_token(JSON_TOKEN_NAME, field_name);
        }
        if (!as_value && metainfo_unbound && !get_at(i)->is_bound()) {
          enc_len += p_tok.put_next_token(JSON_TOKEN_LITERAL_NULL);
          char* metainfo_str = mprintf("metainfo %s", field_name);
          enc_len += p_tok.put_next_token(JSON_TOKEN_NAME, metainfo_str);
          Free(metainfo_str);
          enc_len += p_tok.put_next_token(JSON_TOKEN_STRING, "\"unbound\"");
        }
        else if (NULL != emb_descr) {
          enc_len += get_at(i)->JSON_encode_negtest(emb_descr, *fld_descr(i), p_tok);
        } else {
          enc_len += get_at(i)->JSON_encode(*fld_descr(i), p_tok);
        }
      }
    }
    
    if (!as_value && NULL != err_vals && NULL != err_vals->after) {
      if (NULL == err_vals->after->errval) {
        TTCN_error("internal error: erroneous after value missing");
      }
      if (err_vals->after->raw) {
        enc_len += err_vals->after->errval->JSON_encode_negtest_raw(p_tok);
      } else {
        if (NULL == err_vals->after->type_descr) {
          TTCN_error("internal error: erroneous before typedescriptor missing");
        }
        // it's an extra field, so use the erroneous type's name as the field name
        enc_len += p_tok.put_next_token(JSON_TOKEN_NAME, err_vals->after->type_descr->name);
        enc_len += err_vals->after->errval->JSON_encode(*(err_vals->after->type_descr), p_tok);
      }
    }
    
    if (-1 != p_err_descr->omit_after && p_err_descr->omit_after <= i) {
      break;
    }
  }
  
  if (!as_value) {
    enc_len += p_tok.put_next_token(JSON_TOKEN_OBJECT_END, NULL);
  }
  return enc_len;
}

int Record_Type::JSON_decode(const TTCN_Typedescriptor_t& p_td, JSON_Tokenizer& p_tok, boolean p_silent)
{
  if (NULL != p_td.json && p_td.json->as_value) {
    // if 'as value' is set, then the record/set has only one field,
    // decode that without the need of any brackets or field names 
    return get_at(0)->JSON_decode(*fld_descr(0), p_tok, p_silent);
  }
  json_token_t token = JSON_TOKEN_NONE;
  size_t dec_len = p_tok.get_next_token(&token, NULL, NULL);
  if (JSON_TOKEN_ERROR == token) {
    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_BAD_TOKEN_ERROR, "");
    return JSON_ERROR_FATAL;
  }
  else if (JSON_TOKEN_OBJECT_START != token) {
    return JSON_ERROR_INVALID_TOKEN;
  } 
  
  const int field_count = get_count();
  
  // initialize meta info states
  Vector<int> metainfo(field_count);
  Vector<boolean> field_found(field_count);
  for (int i = 0; i < field_count; ++i) {
    field_found.push_back(FALSE);
    metainfo.push_back(
      (NULL != fld_descr(i)->json && fld_descr(i)->json->metainfo_unbound) ?
      JSON_METAINFO_NONE : JSON_METAINFO_NOT_APPLICABLE);
  }
  
  while (TRUE) {
    // Read name - value token pairs until we reach some other token
    char* name = 0;
    size_t name_len = 0;
    size_t buf_pos = p_tok.get_buf_pos();
    dec_len += p_tok.get_next_token(&token, &name, &name_len);
    if (JSON_TOKEN_ERROR == token) {
      JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_BAD_TOKEN_ERROR, "");
      return JSON_ERROR_FATAL;
    }
    else if (JSON_TOKEN_NAME != token) {
      // undo the last action on the buffer
      p_tok.set_buf_pos(buf_pos);
      break;
    }
    else {
      // check for meta info
      boolean is_metainfo = FALSE;
      if (name_len > 9 && 0 == strncmp(name, "metainfo ", 9)) {
        name += 9;
        name_len -= 9;
        is_metainfo = TRUE;
      }
      
      // check field name
      int field_idx;
      for (field_idx = 0; field_idx < field_count; ++field_idx) {
        const char* expected_name = 0;
        if (NULL != fld_descr(field_idx)->json && NULL != fld_descr(field_idx)->json->alias) {
          expected_name = fld_descr(field_idx)->json->alias;
        } else {
          expected_name = fld_name(field_idx);
        }
        if (strlen(expected_name) == name_len &&
            0 == strncmp(expected_name, name, name_len)) {
          field_found[field_idx] = TRUE;
          break;
        }
      }
      if (field_count == field_idx) {
        // invalid field name
        JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, is_metainfo ?
          JSON_DEC_METAINFO_NAME_ERROR : JSON_DEC_INVALID_NAME_ERROR, (int)name_len, name);
        // if this is set to a warning, skip the value of the field
        dec_len += p_tok.get_next_token(&token, NULL, NULL);
        if (JSON_TOKEN_NUMBER != token && JSON_TOKEN_STRING != token &&
            JSON_TOKEN_LITERAL_TRUE != token && JSON_TOKEN_LITERAL_FALSE != token &&
            JSON_TOKEN_LITERAL_NULL != token) {
          TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_FIELD_TOKEN_ERROR, (int)name_len, name);
          return JSON_ERROR_FATAL;
        }
        continue;
      }
      
      if (is_metainfo) {
        if (JSON_METAINFO_NOT_APPLICABLE != metainfo[field_idx]) {
          // check meta info
          char* info_value = 0;
          size_t info_len = 0;
          dec_len += p_tok.get_next_token(&token, &info_value, &info_len);
          if (JSON_TOKEN_STRING == token && 9 == info_len &&
              0 == strncmp(info_value, "\"unbound\"", 9)) {
            metainfo[field_idx] = JSON_METAINFO_UNBOUND;
          }
          else {
            JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_METAINFO_VALUE_ERROR,
              fld_name(field_idx));
            return JSON_ERROR_FATAL;
          }
        }
        else {
          JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_METAINFO_NOT_APPLICABLE,
            fld_name(field_idx));
          return JSON_ERROR_FATAL;
        }
      }
      else {
        buf_pos = p_tok.get_buf_pos();
        int ret_val2 = get_at(field_idx)->JSON_decode(*fld_descr(field_idx), p_tok, p_silent);
        if (0 > ret_val2) {
          if (JSON_ERROR_INVALID_TOKEN == ret_val2) {
            // undo the last action on the buffer, check if the invalid token was a null token 
            p_tok.set_buf_pos(buf_pos);
            p_tok.get_next_token(&token, NULL, NULL);
            if (JSON_TOKEN_LITERAL_NULL == token) {
              if (JSON_METAINFO_NONE == metainfo[field_idx]) {
                // delay reporting an error for now, there might be meta info later
                metainfo[field_idx] = JSON_METAINFO_NEEDED;
                continue;
              }
              else if (JSON_METAINFO_UNBOUND == metainfo[field_idx]) {
                // meta info already found
                continue;
              }
            }
            JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_FIELD_TOKEN_ERROR, 
              (int)strlen(fld_name(field_idx)), fld_name(field_idx));
          }
          return JSON_ERROR_FATAL;
        }
        dec_len += (size_t)ret_val2;
      }
    }
  }
  
  dec_len += p_tok.get_next_token(&token, NULL, NULL);
  if (JSON_TOKEN_OBJECT_END != token) {
    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_OBJECT_END_TOKEN_ERROR, "");
    return JSON_ERROR_FATAL;
  }
  
  // Check if every field has been set and handle meta info
  for (int field_idx = 0; field_idx < field_count; ++field_idx) {
    Base_Type* field = get_at(field_idx);
    if (JSON_METAINFO_UNBOUND == metainfo[field_idx]) {
      field->clean_up();
    }
    else if (JSON_METAINFO_NEEDED == metainfo[field_idx]) {
      // no meta info was found for this field, report the delayed error
      JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_FIELD_TOKEN_ERROR,
        (int)strlen(fld_name(field_idx)), fld_name(field_idx));
    }
    else if (!field_found[field_idx]) {
      if (NULL != fld_descr(field_idx)->json && NULL != fld_descr(field_idx)->json->default_value) {
        get_at(field_idx)->JSON_decode(*fld_descr(field_idx), DUMMY_BUFFER, p_silent);
      }
      else if (field->is_optional()) {
        field->set_to_omit();
      } else {
        JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_MISSING_FIELD_ERROR, fld_name(field_idx));
        return JSON_ERROR_FATAL;
      }
    }
  }
  
  return (int)dec_len;
}

////////////////////////////////////////////////////////////////////////////////

Empty_Record_Type::Empty_Record_Type(): bound_flag(FALSE)
{
}

Empty_Record_Type::Empty_Record_Type(const Empty_Record_Type& other_value)
: Base_Type(other_value), bound_flag(other_value.bound_flag)
{
  if (!other_value.bound_flag)
    TTCN_error("Copying an unbound value of type %s.",
               other_value.get_descriptor()->name);
}

boolean Empty_Record_Type::operator==(null_type) const
{
  if (!bound_flag)
    TTCN_error("Comparison of an unbound value of type %s.",
               get_descriptor()->name);
  return TRUE;
}

void Empty_Record_Type::log() const
{
  if (bound_flag) TTCN_Logger::log_event_str("{ }");
  else TTCN_Logger::log_event_unbound();
}

void Empty_Record_Type::set_param(Module_Param& param) {
  param.basic_check(Module_Param::BC_VALUE, "empty record/set value (i.e. { })");
  Module_Param_Ptr mp = &param;
  if (param.get_type() == Module_Param::MP_Reference) {
    mp = param.get_referenced_param();
  }
  if (mp->get_type()!=Module_Param::MP_Value_List || mp->get_size()>0) {
    param.type_error("empty record/set value (i.e. { })", get_descriptor()->name);
  }
  bound_flag = TRUE;
}

Module_Param* Empty_Record_Type::get_param(Module_Param_Name& /* param_name */) const
{
  if (!is_bound()) {
    return new Module_Param_Unbound();
  }
  return new Module_Param_Value_List();
}

void Empty_Record_Type::encode_text(Text_Buf& /*text_buf*/) const
{
  if (!bound_flag)
    TTCN_error("Text encoder: Encoding an unbound value of type %s.",
               get_descriptor()->name);
}

void Empty_Record_Type::decode_text(Text_Buf& /*text_buf*/)
{
  bound_flag = TRUE;
}

boolean Empty_Record_Type::is_equal(const Base_Type* other_value) const
{
  const Empty_Record_Type* r2 = static_cast<const Empty_Record_Type*>(other_value);
  if ((bound_flag && r2->bound_flag) || (!bound_flag && !r2->bound_flag))
    return TRUE;
  if (!bound_flag || !r2->bound_flag)
    TTCN_error("Comparison of an unbound value of type %s.", get_descriptor()->name);
  return FALSE;
}

void Empty_Record_Type::set_value(const Base_Type* other_value)
{
  if (!static_cast<const Empty_Record_Type*>(other_value)->is_bound())
    TTCN_error("Assignment of an unbound value of type %s.",
               other_value->get_descriptor()->name);
  bound_flag = TRUE;
}

void Empty_Record_Type::encode(const TTCN_Typedescriptor_t& p_td,
  TTCN_Buffer& p_buf, TTCN_EncDec::coding_t p_coding, ...) const
{
  va_list pvar;
  va_start(pvar, p_coding);
  switch(p_coding) {
  case TTCN_EncDec::CT_BER: {
    TTCN_EncDec_ErrorContext ec("While BER-encoding type '%s': ", p_td.name);
    unsigned BER_coding=va_arg(pvar, unsigned);
    BER_encode_chk_coding(BER_coding);
    ASN_BER_TLV_t *tlv=BER_encode_TLV(p_td, BER_coding);
    tlv->put_in_buffer(p_buf);
    ASN_BER_TLV_t::destruct(tlv);
    break;}
  case TTCN_EncDec::CT_RAW: {
    TTCN_EncDec_ErrorContext ec("While RAW-encoding type '%s': ", p_td.name);
    if(!p_td.raw) TTCN_EncDec_ErrorContext::error_internal
                    ("No RAW descriptor available for type '%s'.", p_td.name);
    RAW_enc_tr_pos rp;
    rp.level=0;
    rp.pos=NULL;
    RAW_enc_tree root(FALSE, NULL, &rp, 1, p_td.raw);
    RAW_encode(p_td, root);
    root.put_to_buf(p_buf);
    break;}
  case TTCN_EncDec::CT_TEXT: {
    TTCN_EncDec_ErrorContext ec("While TEXT-encoding type '%s': ", p_td.name);
    if(!p_td.text) TTCN_EncDec_ErrorContext::error_internal
                     ("No TEXT descriptor available for type '%s'.", p_td.name);
    TEXT_encode(p_td,p_buf);
    break;}
  case TTCN_EncDec::CT_XER: {
    TTCN_EncDec_ErrorContext ec("While XER-encoding type '%s': ", p_td.name);
    unsigned XER_coding=va_arg(pvar, unsigned);
    XER_encode(*(p_td.xer),p_buf, XER_coding, 0, 0, 0);
    p_buf.put_c('\n');
    break;}
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

void Empty_Record_Type::decode(const TTCN_Typedescriptor_t& p_td,
  TTCN_Buffer& p_buf, TTCN_EncDec::coding_t p_coding, ...)
{
  va_list pvar;
  va_start(pvar, p_coding);
  switch(p_coding) {
  case TTCN_EncDec::CT_BER: {
    TTCN_EncDec_ErrorContext ec("While BER-decoding type '%s': ", p_td.name);
    unsigned L_form=va_arg(pvar, unsigned);
    ASN_BER_TLV_t tlv;
    BER_decode_str2TLV(p_buf, tlv, L_form);
    BER_decode_TLV(p_td, tlv, L_form);
    if(tlv.isComplete) p_buf.increase_pos(tlv.get_len());
    break;}
  case TTCN_EncDec::CT_RAW: {
    TTCN_EncDec_ErrorContext ec("While RAW-decoding type '%s': ", p_td.name);
    if(!p_td.raw)
      TTCN_EncDec_ErrorContext::error_internal
        ("No RAW descriptor available for type '%s'.", p_td.name);
    raw_order_t order;
    switch(p_td.raw->top_bit_order) {
    case TOP_BIT_LEFT:
      order=ORDER_LSB;
      break;
    case TOP_BIT_RIGHT:
    default:
      order=ORDER_MSB;
    }
    if(RAW_decode(p_td, p_buf, p_buf.get_len()*8, order)<0)
      ec.error(TTCN_EncDec::ET_INCOMPL_MSG,
               "Can not decode type '%s', because invalid or incomplete"
               " message was received", p_td.name);
    break;}
  case TTCN_EncDec::CT_TEXT: {
    Limit_Token_List limit;
    TTCN_EncDec_ErrorContext ec("While TEXT-decoding type '%s': ", p_td.name);
    if(!p_td.text) TTCN_EncDec_ErrorContext::error_internal
                     ("No TEXT descriptor available for type '%s'.", p_td.name);
    const unsigned char *b=p_buf.get_data();
    if(b[p_buf.get_len()-1]!='\0'){
      p_buf.set_pos(p_buf.get_len());
      p_buf.put_zero(8,ORDER_LSB);
      p_buf.rewind();
    }
    if(TEXT_decode(p_td,p_buf,limit)<0)
      ec.error(TTCN_EncDec::ET_INCOMPL_MSG,
        "Can not decode type '%s', because invalid or incomplete"
        " message was received", p_td.name);
    break;}
  case TTCN_EncDec::CT_XER: {
    TTCN_EncDec_ErrorContext ec("While XER-decoding type '%s': ", p_td.name);
    unsigned XER_coding=va_arg(pvar, unsigned);
    XmlReaderWrap reader(p_buf);
    for (int success=reader.Read(); success==1; success=reader.Read()) {
      if (reader.NodeType() == XML_READER_TYPE_ELEMENT) break;
    }
    XER_decode(*(p_td.xer), reader, XER_coding | XER_TOPLEVEL, XER_NONE, 0);
    size_t bytes = reader.ByteConsumed();
    p_buf.set_pos(bytes);
    break;}
  case TTCN_EncDec::CT_JSON: {
    TTCN_EncDec_ErrorContext ec("While JSON-decoding type '%s': ", p_td.name);
    if(!p_td.json) TTCN_EncDec_ErrorContext::error_internal
                     ("No JSON descriptor available for type '%s'.", p_td.name);
    JSON_Tokenizer tok((const char*)p_buf.get_data(), p_buf.get_len());
    if(JSON_decode(p_td, tok, FALSE)<0)
      ec.error(TTCN_EncDec::ET_INCOMPL_MSG,
        "Can not decode type '%s', because invalid or incomplete"
        " message was received", p_td.name);
    p_buf.set_pos(tok.get_buf_pos());
    break;}
  default:
    TTCN_error("Unknown coding method requested to decode type '%s'", p_td.name);
  }
  va_end(pvar);
}

ASN_BER_TLV_t* Empty_Record_Type::BER_encode_TLV(const TTCN_Typedescriptor_t& p_td,
  unsigned p_coding) const
{
  BER_chk_descr(p_td);
  ASN_BER_TLV_t *new_tlv=ASN_BER_TLV_t::construct(NULL);
  new_tlv=ASN_BER_V2TLV(new_tlv, p_td, p_coding);
  return new_tlv;
}

boolean Empty_Record_Type::BER_decode_TLV(const TTCN_Typedescriptor_t& p_td,
  const ASN_BER_TLV_t& p_tlv, unsigned L_form)
{
  BER_chk_descr(p_td);
  ASN_BER_TLV_t stripped_tlv;
  BER_decode_strip_tags(*p_td.ber, p_tlv, L_form, stripped_tlv);
  TTCN_EncDec_ErrorContext ec_0("While decoding '%s' type: ", get_descriptor()->name);
  stripped_tlv.chk_constructed_flag(TRUE);
  bound_flag=TRUE;
  return TRUE;
}

int Empty_Record_Type::RAW_encode(const TTCN_Typedescriptor_t& p_td,
  RAW_enc_tree& /*myleaf*/) const
{
  if (!bound_flag) TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
    "Encoding an unbound value of type %s.", p_td.name);
  return 0;
}

int Empty_Record_Type::RAW_decode(const TTCN_Typedescriptor_t& p_td,
  TTCN_Buffer& buff, int /*limit*/, raw_order_t /*top_bit_ord*/,
  boolean /*no_err*/, int /*sel_field*/, boolean /*first_call*/)
{
  bound_flag = TRUE;
  return buff.increase_pos_padd(p_td.raw->prepadding)
    +    buff.increase_pos_padd(p_td.raw->padding);
}

int Empty_Record_Type::TEXT_encode(const TTCN_Typedescriptor_t& p_td, TTCN_Buffer& buff) const
{
  int encoded_length=0;
  if(p_td.text->begin_encode) {
    buff.put_cs(*p_td.text->begin_encode);
    encoded_length+=p_td.text->begin_encode->lengthof();
  }
  if (!bound_flag) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND, "Encoding an unbound value.");
  }
  if(p_td.text->end_encode) {
    buff.put_cs(*p_td.text->end_encode);
    encoded_length+=p_td.text->end_encode->lengthof();
  }
  return encoded_length;
}

int Empty_Record_Type::TEXT_decode(const TTCN_Typedescriptor_t& p_td,
  TTCN_Buffer& buff, Limit_Token_List& /*limit*/, boolean no_err, boolean /*first_call*/)
{
  int decoded_length=0;
  if(p_td.text->begin_decode) {
    int tl;
    if((tl=p_td.text->begin_decode->match_begin(buff))<0) {
      if(no_err)return -1;
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TOKEN_ERR,
        "The specified token '%s' not found for '%s': ",
        (const char*)*(p_td.text->begin_decode), p_td.name);
      return 0;
    }
    decoded_length+=tl;
    buff.increase_pos(tl);
  }
  if(p_td.text->end_decode) {
    int tl;
    if((tl=p_td.text->end_decode->match_begin(buff))<0) {
      if(no_err)return -1;
      TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_TOKEN_ERR,
        "The specified token '%s' not found for '%s': ",
        (const char*)*(p_td.text->end_decode), p_td.name);
      return 0;
    }
    decoded_length+=tl;
    buff.increase_pos(tl);
  }
  bound_flag = TRUE;
  return decoded_length;
}

int Empty_Record_Type::XER_encode(const XERdescriptor_t& p_td,
  TTCN_Buffer& p_buf, unsigned int flavor, unsigned int /*flavor2*/, int indent, embed_values_enc_struct_t*) const
{
  int encoded_length=(int)p_buf.get_len();
  int indenting = !is_canonical(flavor);
  int exer = is_exer(flavor);
  if (indenting) do_indent(p_buf, indent);
  p_buf.put_c('<');
  if (exer) write_ns_prefix(p_td, p_buf);
  p_buf.put_s((size_t)p_td.namelens[exer]-2, (cbyte*)p_td.names[exer]);
  p_buf.put_s(2 + indenting, (cbyte*)"/>\n");
  return (int)p_buf.get_len() - encoded_length;
}

int Empty_Record_Type::XER_decode(const XERdescriptor_t& p_td,
  XmlReaderWrap& reader, unsigned int flavor, unsigned int /*flavor2*/, embed_values_dec_struct_t*)
{
  int exer = is_exer(flavor);
  bound_flag = TRUE;
  int success, depth = -1;
  for (success=reader.Ok(); success==1; success=reader.Read()) {
    int type = reader.NodeType();
    if (type==XML_READER_TYPE_ELEMENT) {
      verify_name(reader, p_td, exer);
      depth = reader.Depth();

      if (reader.IsEmptyElement()) {
        reader.Read(); break;
      }
      else if ((flavor & XER_MASK) == XER_CANONICAL) {
        TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_INVAL_MSG,
          "Expected an empty element tag");
        // Stay in the loop and look for the end element, in case the error
        // was ignored or reduced to warning.
      } // if(empty)
    }
    else if (type == XML_READER_TYPE_END_ELEMENT && depth != -1) {
      verify_end(reader, p_td, depth, exer);
      reader.Read();
      break;
    }
  }
  return 1; // decode successful
}

int Empty_Record_Type::JSON_encode(const TTCN_Typedescriptor_t&, JSON_Tokenizer& p_tok) const
{
  if (!is_bound()) {
    TTCN_EncDec_ErrorContext::error(TTCN_EncDec::ET_UNBOUND,
      "Encoding an unbound empty %s value.", is_set() ? "set" : "record");
    return -1;
  }
  
  return p_tok.put_next_token(JSON_TOKEN_OBJECT_START, NULL) +
    p_tok.put_next_token(JSON_TOKEN_OBJECT_END, NULL);
}

int Empty_Record_Type::JSON_decode(const TTCN_Typedescriptor_t&, JSON_Tokenizer& p_tok, boolean p_silent)
{
  json_token_t token = JSON_TOKEN_NONE;
  size_t dec_len = p_tok.get_next_token(&token, NULL, NULL);
  if (JSON_TOKEN_ERROR == token) {
    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_BAD_TOKEN_ERROR, "");
    return JSON_ERROR_FATAL;
  }
  else if (JSON_TOKEN_OBJECT_START != token) {
    return JSON_ERROR_INVALID_TOKEN;
  }
  
  dec_len += p_tok.get_next_token(&token, NULL, NULL);
  if (JSON_TOKEN_OBJECT_END != token) {
    JSON_ERROR(TTCN_EncDec::ET_INVAL_MSG, JSON_DEC_STATIC_OBJECT_END_TOKEN_ERROR, "");
    return JSON_ERROR_FATAL;
  }
  
  bound_flag = TRUE;
  
  return (int)dec_len;
}

boolean operator==(null_type /*null_value*/, const Empty_Record_Type& other_value)
{
  if (!other_value.is_bound())
    TTCN_error("Comparison of an unbound value of type %s.",
               other_value.get_descriptor()->name);
  return TRUE;
}

boolean operator!=(null_type /*null_value*/, const Empty_Record_Type& other_value)
{
  if (!other_value.is_bound())
    TTCN_error("Comparison of an unbound value of type %s.",
               other_value.get_descriptor()->name);
  return FALSE;
}
#endif
