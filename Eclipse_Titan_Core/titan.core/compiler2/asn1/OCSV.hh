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
 *
 ******************************************************************************/
#ifndef _Asn_OCSV_HH
#define _Asn_OCSV_HH

#include "Object.hh"
#include "Block.hh"

namespace Asn {

  using namespace Common;

  class OCSV_Builder;
  class OCSV_Parser;

  /**
   * OCS visitor to build the OCS. :) It's clear and simple, isn't it?
   */
  class OCSV_Builder : public OCS_Visitor {
  private:
    TokenBuf *tb;
    FieldSpecs *fss; // ref.

    OCSV_Builder(const OCSV_Builder& p);
    void visit0(OCS_Node& p);
  public:
    OCSV_Builder(TokenBuf *p_tb, FieldSpecs *p_fss);
    virtual ~OCSV_Builder();
    virtual void visit_root(OCS_root& p);
    virtual void visit_seq(OCS_seq& p);
    virtual void visit_literal(OCS_literal& p);
    virtual void visit_setting(OCS_setting& p);
  };

  /**
   * OCS visitor to parse an object definition.
   */
  class OCSV_Parser : public OCS_Visitor {
  private:
    TokenBuf *tb;
    Obj_defn *my_obj;
    /** Stores whether the parsing was successful. If it is false, the
     *  parsing cannot be continued. */
    bool success;
    /** Stores whether the previous parsing was successful. */
    bool prev_success;

    OCSV_Parser(const OCSV_Parser& p);
  public:
    OCSV_Parser(TokenBuf *p_tb, Obj_defn *p_my_obj);
    virtual ~OCSV_Parser();
    virtual void visit_root(OCS_root& p);
    virtual void visit_seq(OCS_seq& p);
    virtual void visit_literal(OCS_literal& p);
    virtual void visit_setting(OCS_setting& p);
  private:
    size_t is_ref(size_t pos);
    size_t is_tag(size_t pos);
    size_t is_constraint(size_t pos);
    size_t is_constraints(size_t pos);
    size_t is_nakedtype(size_t pos);
    size_t is_type(size_t pos);
    size_t is_value(size_t pos);
    size_t is_object(size_t pos);
    size_t is_objectset(size_t pos);
    Block* get_first_n(size_t n);
    Type* parse_type();
    Value* parse_value();
    Object* parse_object();
    ObjectSet* parse_objectset();
  };

} // namespace Asn

#endif // _Asn_OCSV_HH
