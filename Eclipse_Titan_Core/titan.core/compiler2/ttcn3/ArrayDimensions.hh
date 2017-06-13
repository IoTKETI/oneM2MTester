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
 *   Raduly, Csaba
 *
 ******************************************************************************/
#ifndef _Ttcn_Array_HH
#define _Ttcn_Array_HH

#include "../Setting.hh"
#include "../Type.hh"
#include "../Int.hh"
#include "../vector.hh"

namespace Common {
  // not defined here
  class Value;
  class Scope;
}

namespace Ttcn {

  using namespace Common;

  class Ref_base;

  /**
   * Class to represent the index set of a TTCN-3 array dimension.
   */
  class ArrayDimension : public Node, public Location {
  private:
    bool checked;
    bool is_range;
    bool has_error;
    union {
      Value *single;
      struct {
        Value *lower, *upper;
      } range;
    } u;
    Scope *my_scope;
    size_t size;
    Int offset;
    ArrayDimension(const ArrayDimension& p);
    ArrayDimension& operator=(const ArrayDimension& p);
  public:
    ArrayDimension(Value *p_single);
    ArrayDimension(Value *p_lower, Value *p_upper);
    virtual ~ArrayDimension();
    virtual ArrayDimension *clone() const;
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_fullname(const string& p_fullname);
    void chk();
    void chk_index(Value *index, Type::expected_value_t exp_val);
    size_t get_size();
    Int get_offset();
    string get_stringRepr();
    /** Returns whether \a this and \a p_dim are identical (i.e. both has the
     * same size and offset). */
    bool is_identical(ArrayDimension *p_dim);
    /** Returns the C++ type expression that represents the equivalent value
     * class at runtime viewed from the module of scope \a p_scope.
     * Argument \a p_element_type points to the embedded type. */
    string get_value_type(Type *p_element_type, Scope *p_scope);
    /** Returns the C++ type expression that represents the equivalent template
     * class at runtime viewed from the module of scope \a p_scope.
     * Argument \a p_element_type points to the embedded type. */
    string get_template_type(Type *p_element_type, Scope *p_scope);
    bool get_has_error() const { return has_error; }
    virtual void dump(unsigned level) const;
  };

  /**
   * Class to represent the array dimensions of TTCN-3 timers and ports.
   * These arrays do not support slicing, that is, only a single element of
   * them can be accessed.
   */
  class ArrayDimensions : public Node {
  private:
    vector<ArrayDimension> dims;
    ArrayDimensions(const ArrayDimensions& p);
    ArrayDimensions& operator=(const ArrayDimensions& p);
  public:
    ArrayDimensions() : Node() { }
    virtual ~ArrayDimensions();
    virtual ArrayDimensions *clone() const;
    virtual void set_my_scope(Scope *p_scope);
    virtual void set_fullname(const string& p_fullname);
    void add(ArrayDimension *dim);
    size_t get_nof_dims() { return dims.size(); }
    ArrayDimension *get_dim_byIndex(size_t index) { return dims[index]; }
    void chk();
    /** Checks the field or array references of \a ref against the array
     * dimensions. If parameter \a allow_slicing is false slicing of timer or
     * port arrays are not allowed thus the number of array indices in \a ref
     * must be the same as the number of array dimensions. Otherwise (i.e. if
     * \a allow_slicing is true) \a ref may have fewer indices.
     * If parameter \a any_from is true, then the reference must point to a
     * timer or port array instead of a single timer or port.
     * Parameter \a def_name is needed for error messages, it shall be either
     * "timer" or "port". */
    void chk_indices(Common::Reference *ref, const char *def_type, bool allow_slicing,
      Type::expected_value_t exp_val, bool any_from = false);
    /** Returns the total number of elements in the array. */
    size_t get_array_size();
    /** Generates a C++ code fragment, which is a comma separated list of string
     * literals containing the array element indices in canonical order. Each
     * string literal is prefixed with name \a p_name. The generated code is
     * appended to \a str and the resulting string is returned. The function is
     * recursive, argument \a start_dim must be zero when calling it from
     * outside. */
    char *generate_element_names(char *str, const string& p_name,
      size_t start_dim = 0);
    /** Returns the C++ type expression that represents the type of the timer
     * array. It does not consider the leftmost \a start_dim dimensions. */
    string get_timer_type(size_t start_dim = 0);
    /** Returns the C++ type expression that represents the type of the port
     * array. Parameter \a p_genname is the name of port type class. */
    string get_port_type(const string& p_genname);
    virtual void dump(unsigned level) const;
    bool is_identical(ArrayDimensions *other);
  };

}

#endif // _Ttcn_Array_HH
