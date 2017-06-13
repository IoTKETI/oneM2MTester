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
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *
 ******************************************************************************/
#include "ArrayDimensions.hh"
#include "../string.hh"
#include "../CompilerError.hh"
#include "../Type.hh"
#include "../Value.hh"
#include "AST_ttcn3.hh"

#include <limits.h>

namespace Ttcn {

  using namespace Common;

  // =================================
  // ===== ArrayDimension
  // =================================

  ArrayDimension::ArrayDimension(const ArrayDimension& p)
  : Node(p), Location(p), checked(false), is_range(p.is_range),
    has_error(false), size(0), offset(0)
  {
    if (is_range) {
      u.range.lower = p.u.range.lower->clone();
      u.range.upper = p.u.range.upper->clone();
    } else {
      u.single = p.u.single->clone();
    }
  }

  ArrayDimension::ArrayDimension(Value *p_single)
    : Node(), checked(false), is_range(false), has_error(false),
      size(0), offset(0)
  {
    if (!p_single) FATAL_ERROR("ArrayDimension::ArrayDimension()");
    u.single = p_single;
  }

  ArrayDimension::ArrayDimension(Value *p_lower, Value *p_upper)
    : Node(), checked(false), is_range(true), has_error(false),
      size(0), offset(0)
  {
    if (!p_lower || !p_upper) FATAL_ERROR("ArrayDimension::ArrayDimension()");
    u.range.lower = p_lower;
    u.range.upper = p_upper;
  }

  ArrayDimension::~ArrayDimension()
  {
    if (is_range) {
      delete u.range.lower;
      delete u.range.upper;
    } else {
      delete u.single;
    }
  }

  ArrayDimension *ArrayDimension::clone() const
  {
    return new ArrayDimension(*this);
  }

  void ArrayDimension::set_my_scope(Scope *p_scope)
  {
    if (!p_scope) FATAL_ERROR("ArrayDimension::set_my_scope()");
    my_scope = p_scope;
    if (is_range) {
      u.range.lower->set_my_scope(p_scope);
      u.range.upper->set_my_scope(p_scope);
    } else {
      u.single->set_my_scope(p_scope);
    }
  }

  void ArrayDimension::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    if (is_range) {
      u.range.lower->set_fullname(p_fullname + ".<lower>");
      u.range.upper->set_fullname(p_fullname + ".<upper>");
    } else {
      u.single->set_fullname(p_fullname);
    }
  }

  void ArrayDimension::chk()
  {
    if (checked) return;
    checked = true;
    Int int_size = 0;
    has_error = false;
    if (is_range) {
      {
        Error_Context cntxt(u.range.lower, "In lower bound of array indices");
	u.range.lower->chk_expr_int(Type::EXPECTED_CONSTANT);
      }
      {
        Error_Context cntxt(u.range.upper, "In upper bound of array indices");
	u.range.upper->chk_expr_int(Type::EXPECTED_CONSTANT);
      }
      Value *v_lower = u.range.lower->get_value_refd_last();
      if (v_lower->get_valuetype() != Value::V_INT) has_error = true;
      Value *v_upper = u.range.upper->get_value_refd_last();
      if (v_upper->get_valuetype() != Value::V_INT) has_error = true;
      if (!has_error) {
        const int_val_t *lower_int = v_lower->get_val_Int();
        const int_val_t *upper_int = v_upper->get_val_Int();
        if (*lower_int > INT_MAX) {
          u.range.lower->error("The lower bound of an array index should be "
            "less than `%d' instead of `%s'", INT_MAX,
            (lower_int->t_str()).c_str());
          has_error = true;
        }
        if (*upper_int > INT_MAX) {
          u.range.upper->error("The upper bound of an array index should be "
            "less than `%d' instead of `%s'", INT_MAX,
            (upper_int->t_str()).c_str());
          has_error = true;
        }
        if (!has_error) {
          Int lower = lower_int->get_val();
          Int upper = upper_int->get_val();
          if (lower > upper) {
            error("The lower bound of array index (%s) is "
              "greater than the upper bound (%s)", Int2string(lower).c_str(),
              Int2string(upper).c_str());
            has_error = true;
          } else {
            int_size = upper - lower + 1;
            offset = lower;
          }
        }
      }
    } else {
      {
	Error_Context cntxt(u.single, "In array size");
	u.single->chk_expr_int(Type::EXPECTED_CONSTANT);
      }
      Value *v = u.single->get_value_refd_last();
      if (v->get_valuetype() != Value::V_INT) has_error = true;
      if (!has_error) {
        const int_val_t *int_size_int = v->get_val_Int();
        if (*int_size_int > INT_MAX) {
          u.single->error("The array size should be less than `%d' instead "
            "of `%s'", INT_MAX, (int_size_int->t_str()).c_str());
          has_error = true;
        }
        if (!has_error) {
          int_size = int_size_int->get_val();
          if (int_size <= 0) {
            u.single->error("A positive integer value was expected as array "
              "size instead of `%s'", Int2string(int_size).c_str());
            has_error = true;
          } else {
            size = int_size;
            offset = 0;
          }
        }
      }
    }
    if (!has_error) {
      size = static_cast<size_t>(int_size);
      if (static_cast<Int>(size) != int_size) {
        error("Array size `%s' is too large for being represented in memory",
	  Int2string(int_size).c_str());
	has_error = true;
      }
    }
  }

  void ArrayDimension::chk_index(Value *index, Type::expected_value_t exp_val)
  {
    if (!checked) chk();
    index->chk_expr_int(exp_val);
    if (has_error || index->is_unfoldable()) return;
    const int_val_t *v_index_int = index->get_value_refd_last()
      ->get_val_Int();
    if (*v_index_int < offset) {
      index->error("Array index underflow: the index value must be at least "
	"`%s' instead of `%s'", Int2string(offset).c_str(),
	(v_index_int->t_str()).c_str());
      index->set_valuetype(Value::V_ERROR);
    } else if (*v_index_int >= offset + static_cast<Int>(size)) {
      index->error("Array index overflow: the index value must be at most "
	"`%s' instead of `%s'", Int2string(offset + size - 1).c_str(),
	(v_index_int->t_str()).c_str());
      index->set_valuetype(Value::V_ERROR);
    }
  }

  size_t ArrayDimension::get_size()
  {
    if (!checked) chk();
    if (has_error) return 0;
    else return size;
  }

  Int ArrayDimension::get_offset()
  {
    if (!checked) chk();
    if (has_error) return 0;
    else return offset;
  }

  string ArrayDimension::get_stringRepr()
  {
    if (!checked) chk();
    string ret_val("[");
    if (has_error) ret_val += "<erroneous>";
    else if (is_range) {
      ret_val += Int2string(offset);
      ret_val += "..";
      ret_val += Int2string(offset + size - 1);
    } else ret_val += Int2string(size);
    ret_val += "]";
    return ret_val;
  }

  bool ArrayDimension::is_identical(ArrayDimension *p_dim)
  {
    if (!p_dim) FATAL_ERROR("ArrayDimension::is_identical()");
    if (!checked) chk();
    if (!p_dim->checked) p_dim->chk();
    if (has_error || p_dim->has_error) return true;
    else return size == p_dim->size && offset == p_dim->offset;
  }

  string ArrayDimension::get_value_type(Type *p_element_type, Scope *p_scope)
  {
    if (!checked) chk();
    if (has_error) FATAL_ERROR("ArrayDimension::get_value_type()");
    string ret_val("VALUE_ARRAY<");
    ret_val += p_element_type->get_genname_value(p_scope);
    ret_val += ", ";
    ret_val += Int2string(size);
    ret_val += ", ";
    ret_val += Int2string(offset);
    ret_val += '>';
    return ret_val;
  }

  string ArrayDimension::get_template_type(Type *p_element_type, Scope *p_scope)
  {
    if (!checked) chk();
    if (has_error) FATAL_ERROR("ArrayDimension::get_template_type()");
    string ret_val("TEMPLATE_ARRAY<");
    ret_val += p_element_type->get_genname_value(p_scope);
    ret_val += ", ";
    ret_val += p_element_type->get_genname_template(p_scope);
    ret_val += ", ";
    ret_val += Int2string(size);
    ret_val += ", ";
    ret_val += Int2string(offset);
    ret_val += '>';
    return ret_val;
  }

  void ArrayDimension::dump(unsigned level) const
  {
    DEBUG(level, "Array dimension:");
    if (is_range) {
      u.range.lower->dump(level + 1);
      DEBUG(level, "..");
      u.range.upper->dump(level + 1);
    } else {
      u.single->dump(level + 1);
    }
  }

  // =================================
  // ===== ArrayDimensions
  // =================================

  ArrayDimensions::~ArrayDimensions()
  {
    for (size_t i = 0; i < dims.size(); i++) delete dims[i];
    dims.clear();
  }

  ArrayDimensions *ArrayDimensions::clone() const
  {
    FATAL_ERROR("ArrayDimensions::clone");
  }

  void ArrayDimensions::set_my_scope(Scope *p_scope)
  {
    for (size_t i = 0; i < dims.size(); i++) dims[i]->set_my_scope(p_scope);
  }

  void ArrayDimensions::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    for (size_t i = 0; i < dims.size(); i++)
      dims[i]->set_fullname(p_fullname + "." + Int2string(i + 1));
  }

  void ArrayDimensions::add(ArrayDimension *dim)
  {
    if (!dim) FATAL_ERROR("ArrayDimensions::add()");
    dims.add(dim);
  }

  void ArrayDimensions::chk()
  {
    for (size_t i = 0; i < dims.size(); i++) {
      ArrayDimension *dim = dims[i];
      Error_Context cntxt(dim, "In array dimension #%lu", static_cast<unsigned long>(i+1));
      dim->chk();
    }
  }

  void ArrayDimensions::chk_indices(Common::Reference *ref, const char *def_name,
    bool allow_slicing, Type::expected_value_t exp_val, bool any_from)
  {
    FieldOrArrayRefs *subrefs = ref->get_subrefs();
    if (!subrefs) {
      if (!allow_slicing && !any_from)
	ref->error("Reference to a %s array without array index", def_name);
      return;
    }
    size_t nof_refs = subrefs->get_nof_refs(), nof_dims = dims.size();
    size_t upper_limit = nof_refs > nof_dims ? nof_dims : nof_refs;
    for (size_t i = 0; i < upper_limit; i++) {
      FieldOrArrayRef *subref = subrefs->get_ref(i);
      if (subref->get_type() != FieldOrArrayRef::ARRAY_REF) {
        subref->error("Invalid field reference `%s' in a %s array",
	  subref->get_id()->get_dispname().c_str(), def_name);
	return;
      }
      Error_Context cntxt(subref, "In array index #%lu", static_cast<unsigned long>(i+1));
      dims[i]->chk_index(subref->get_val(), exp_val);
    }
    if (nof_refs < nof_dims) {
      if (!allow_slicing && !any_from) {
        ref->error("Too few indices in a reference to a %s array without the "
          "'any from' clause: the array has %lu dimensions, but the reference "
          "has only %lu array %s", def_name, static_cast<unsigned long>(nof_dims),
          static_cast<unsigned long>(nof_refs), nof_refs > 1 ? "indices" : "index");
      }
    } else if (nof_refs > nof_dims) {
      ref->error("Too many indices in a reference to a %s array: the reference "
        "has %lu array indices, but the array has only %lu dimension%s",
        def_name, static_cast<unsigned long>( nof_refs ), static_cast<unsigned long>( nof_dims ),
        nof_dims > 1 ? "s" : "");
    }
    else if (any_from) { // nof_refs == nof_dims
      ref->error("Too many indices in a reference to a %s array with the "
        "'any from' clause : the reference has as many array indices as the "
        "array has dimensions (%lu)", def_name, static_cast<unsigned long>( nof_refs ));
    }
  }

  size_t ArrayDimensions::get_array_size()
  {
    size_t ret_val = 1;
    for (size_t i = 0; i < dims.size(); i++)
      ret_val *= dims[i]->get_size();
    return ret_val;
  }

  char *ArrayDimensions::generate_element_names(char *str,
    const string& p_name, size_t start_dim)
  {
    ArrayDimension *dim = dims[start_dim];
    size_t dim_size = dim->get_size();
    Int dim_offset = dim->get_offset();
    if (start_dim + 1 < dims.size()) {
      // there are more dimensions to generate
      for (size_t i = 0; i < dim_size; i++) {
	if (i > 0) str = mputstr(str, ", ");
	str = generate_element_names(str,
	  p_name + "[" + Int2string(dim_offset + i) + "]", start_dim + 1);
      }
    } else {
      // we are in the last dimension
      for (size_t i = 0; i < dim_size; i++) {
	if (i > 0) str = mputstr(str, ", ");
	str = mputprintf(str, "\"%s[%s]\"", p_name.c_str(),
	  Int2string(dim_offset + i).c_str());
      }
    }
    return str;
  }

  string ArrayDimensions::get_timer_type(size_t start_dim)
  {
    string ret_val("TIMER");
    // the wrapping is started with the rightmost array dimension
    for (size_t i = dims.size(); i > start_dim; i--) {
      ArrayDimension *dim = dims[i - 1];
      ret_val = "TIMER_ARRAY<" + ret_val + ", " + Int2string(dim->get_size()) +
        ", " + Int2string(dim->get_offset()) + ">";
    }
    return ret_val;
  }

  string ArrayDimensions::get_port_type(const string& p_genname)
  {
    string ret_val(p_genname);
    // the wrapping is started with the rightmost array dimension
    for (size_t i = dims.size(); i > 0; i--) {
      ArrayDimension *dim = dims[i - 1];
      ret_val = "PORT_ARRAY<" + ret_val + ", " + Int2string(dim->get_size()) +
        ", " + Int2string(dim->get_offset()) + ">";
    }
    return ret_val;
  }

  void ArrayDimensions::dump(unsigned level) const
  {
    DEBUG(level, "Array dimensions: (%lu pcs.)", static_cast<unsigned long>( dims.size() ));
    for (size_t i = 0; i < dims.size(); i++) dims[i]->dump(level + 1);
  }

  bool ArrayDimensions::is_identical(ArrayDimensions *other)
  {
    if (!other) FATAL_ERROR("ArrayDimensions::is_identical()");
    if (dims.size()!=other->dims.size()) return false;
    for (size_t i = 0; i < dims.size(); i++)
      if (!dims[i]->is_identical(other->dims[i])) return false;
    return true;
  }

}
