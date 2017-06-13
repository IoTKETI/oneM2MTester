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
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Gecse, Roland
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
#include "Valuestuff.hh"
#include "Identifier.hh"
#include "Value.hh"
#include "asn1/Object0.hh"
#include "asn1/Block.hh"
#include "asn1/TokenBuf.hh"
#include <limits.h>

#include "ustring.hh"

namespace Common {

  // =================================
  // ===== Values
  // =================================

  Values::Values(bool p_indexed) : Node(), indexed(p_indexed)
  {
    if (!p_indexed) vs = new vector<Value>();
    else ivs = new vector<IndexedValue>();
  }
  
  Values::Values(const Values& p) : Node(p), indexed(p.indexed)
  {
    if (indexed) {
      ivs = new vector<IndexedValue>();
      for (size_t i = 0; i < p.ivs->size(); ++i) {
        ivs->add((*p.ivs)[i]->clone());
      }
    }
    else {
      vs = new vector<Value>();
      for (size_t i = 0; i < p.vs->size(); ++i) {
        vs->add((*p.vs)[i]->clone());
      }
    }
  }

  Values::~Values()
  {
    if (!indexed) {
      for (size_t i = 0; i < vs->size(); i++) delete (*vs)[i];
      vs->clear();
      delete vs;
    } else {
      for (size_t i = 0; i < ivs->size(); i++) delete (*ivs)[i];
      ivs->clear();
      delete ivs;
    }
  }

  Values *Values::clone() const
  {
    return new Values(*this);
  }

  void Values::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    if (!indexed) {
      for (size_t i = 0; i < vs->size(); i++) {
        Value *v = (*vs)[i];
        if (v) v->set_fullname(p_fullname + "[" + Int2string(i) + "]");
      }
    } else {
      for (size_t i = 0; i < ivs->size(); i++) {
        IndexedValue *iv = (*ivs)[i];
        // The order is defined by the user.  The index used, doesn't really
        // matter here.
        if (iv) iv->set_fullname(p_fullname + "[" + Int2string(i) + "]");
      }
    }
  }

  void Values::set_my_scope(Scope *p_scope)
  {
    if (!indexed) {
      for (size_t i = 0; i < vs->size(); i++) {
        Value *v = (*vs)[i];
        if (v) v->set_my_scope(p_scope);
      }
    } else {
      for (size_t i = 0; i < ivs->size(); i++) {
        IndexedValue *iv = (*ivs)[i];
        if (iv) iv->set_my_scope(p_scope);
      }
    }
  }

  size_t Values::get_nof_vs() const
  {
    if (indexed) FATAL_ERROR("Values::get_nof_vs()");
    return vs->size();
  }

  size_t Values::get_nof_ivs() const
  {
    if (!indexed) FATAL_ERROR("Values::get_nof_ivs()");
    return ivs->size();
  }

  Value *Values::get_v_byIndex(size_t p_i) const
  {
    if (indexed) FATAL_ERROR("Values::get_v_byIndex");
    return (*vs)[p_i];
  }

  IndexedValue *Values::get_iv_byIndex(size_t p_i) const
  {
    if (!indexed) FATAL_ERROR("Values::get_iv_byIndex()");
    return (*ivs)[p_i];
  }

  void Values::add_v(Value *p_v)
  {
    if (!p_v) FATAL_ERROR("Values::add_v(): NULL parameter");
    if (indexed) FATAL_ERROR("Values::add_v()");
    vs->add(p_v);
  }

  void Values::add_iv(IndexedValue *p_iv)
  {
    if (!p_iv) FATAL_ERROR("Values::add_iv(): NULL parameter");
    if (!indexed) FATAL_ERROR("Values::add_iv()");
    ivs->add(p_iv);
  }

  Value *Values::steal_v_byIndex(size_t p_i)
  {
    if (indexed) FATAL_ERROR("Values::steal_v_byIndex()");
    Value *ret_val = (*vs)[p_i];
    if (!ret_val) FATAL_ERROR("Values::steal_v_byIndex()");
    (*vs)[p_i] = NULL;
    return ret_val;
  }

  void Values::dump(unsigned level) const
  {
    if (!indexed) {
      for (size_t i = 0; i < vs->size(); i++) {
        Value *v = (*vs)[i];
        if (v) v->dump(level);
        else DEBUG(level, "Value: stolen");
      }
    } else {
      for (size_t i = 0; i < ivs->size(); i++) {
        IndexedValue *iv = (*ivs)[i];
        if (iv) iv->dump(level);
        else DEBUG(level, "Value: stolen");
      }
    }
  }

  // =================================
  // ===== IndexedValue
  // =================================

  IndexedValue::IndexedValue(Ttcn::FieldOrArrayRef *p_index, Value *p_value)
    : Node(), Location(), index(p_index), value(p_value)
  {
    if (!p_index || !p_value)
      FATAL_ERROR("NULL parameter: IndexedValue::IndexedValue()");
  }
  
  IndexedValue::IndexedValue(const IndexedValue& p) : Node(p), Location(p)
  {
    index = p.index->clone();
    value = p.value->clone();
  }

  IndexedValue::~IndexedValue()
  {
    delete index;
    delete value;
  }

  IndexedValue *IndexedValue::clone() const
  {
    return new IndexedValue(*this);
  }

  void IndexedValue::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    if (value) value->set_fullname(p_fullname);
    if (index) index->set_fullname(p_fullname);
  }

  void IndexedValue::set_my_scope(Scope *p_scope)
  {
    if (value) value->set_my_scope(p_scope);
    if (index) index->set_my_scope(p_scope);
  }

  void IndexedValue::set_code_section
    (GovernedSimple::code_section_t p_code_section)
  {
    // Follow the style and add ifs everywhere...
    if (index) index->get_val()->set_code_section(p_code_section);
    if (value) value->set_code_section(p_code_section);
  }

  Value *IndexedValue::steal_value()
  {
    if (!value) FATAL_ERROR("IndexedValue::steal_value()");
    Value *tmp = value;
    value = 0;
    return tmp;
  }

  void IndexedValue::dump(unsigned level) const
  {
    index->dump(level);
    if (value) value->dump(level + 1);
    else DEBUG(level + 1, "Value: stolen");
  }

  bool IndexedValue::is_unfoldable(ReferenceChain *refch,
  		Type::expected_value_t exp_val)
  {
	  return (value->is_unfoldable(refch, exp_val) ||
			  	  get_index()->is_unfoldable(refch, exp_val));
  }

  // =================================
  // ===== NamedValue
  // =================================

  NamedValue::NamedValue(Identifier *p_name, Value *p_value)
    : Node(), Location(), name(p_name), value(p_value)
  {
    if (!p_name || !p_value)
      FATAL_ERROR("NULL parameter: NamedValue::NamedValue()");
  }

  NamedValue::NamedValue(const NamedValue& p)
    : Node(p), Location(p)
  {
    if (!p.value) FATAL_ERROR("NamedValue::NamedValue(): value is stolen");
    name = p.name->clone();
    value = p.value->clone();
  }

  NamedValue::~NamedValue()
  {
    delete name;
    delete value;
  }

  NamedValue *NamedValue::clone() const
  {
    return new NamedValue(*this);
  }

  void NamedValue::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    if (value) value->set_fullname(p_fullname);
  }

  void NamedValue::set_my_scope(Scope *p_scope)
  {
    if (value) value->set_my_scope(p_scope);
  }

  Value *NamedValue::steal_value()
  {
    if (!value) FATAL_ERROR("NamedValue::steal_value()");
    Value *tmp = value;
    value = 0;
    return tmp;
  }

  void NamedValue::dump(unsigned level) const
  {
    name->dump(level);
    if (value) value->dump(level + 1);
    else DEBUG(level + 1, "Value: stolen");
  }

  // =================================
  // ===== NamedValues
  // =================================

  NamedValues::NamedValues(const NamedValues& p)
    : Node(p), checked(false)
  {
    for (size_t i = 0; i < p.nvs_v.size(); i++)
      nvs_v.add(p.nvs_v[i]->clone());
  }

  NamedValues::~NamedValues()
  {
    for(size_t i = 0; i < nvs_v.size(); i++) delete nvs_v[i];
    nvs_v.clear();
    nvs_m.clear();
  }

  NamedValues *NamedValues::clone() const
  {
    return new NamedValues(*this);
  }

  void NamedValues::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    for(size_t i = 0; i < nvs_v.size(); i++) {
      NamedValue *nv = nvs_v[i];
      nv->set_fullname(p_fullname + "." + nv->get_name().get_dispname());
    }
  }

  void NamedValues::set_my_scope(Scope *p_scope)
  {
    for (size_t i = 0; i < nvs_v.size(); i++)
      nvs_v[i]->set_my_scope(p_scope);
  }

  void NamedValues::add_nv(NamedValue *p_nv)
  {
    if (!p_nv) FATAL_ERROR("NamedValues::add_nv(): NULL parameter");
    nvs_v.add(p_nv);
    if (checked) {
      const string& name = p_nv->get_name().get_name();
      if (nvs_m.has_key(name)) FATAL_ERROR("NamedValues::add_nv()");
      nvs_m.add(name, p_nv);
    }
  }

  bool NamedValues::has_nv_withName(const Identifier& p_name)
  {
    if (!checked) chk_dupl_id(false);
    return nvs_m.has_key(p_name.get_name());
  }

  NamedValue* NamedValues::get_nv_byName(const Identifier& p_name)
  {
    if (!checked) chk_dupl_id(false);
    return nvs_m[p_name.get_name()];
  }

  void NamedValues::chk_dupl_id(bool report_errors)
  {
    if (checked) nvs_m.clear();
    for (size_t i = 0; i < nvs_v.size(); i++) {
      NamedValue *nv = nvs_v[i];
      const Identifier& id = nv->get_name();
      const string& name = id.get_name();
      if (!nvs_m.has_key(name)) nvs_m.add(name, nv);
      else if (report_errors) {
	const char *dispname_str = id.get_dispname().c_str();
	nv->error("Duplicate identifier: `%s'", dispname_str);
	nvs_m[name]->note("Previous definition of `%s' is here", dispname_str);
      }
    }
    checked = true;
  }

  void NamedValues::dump(unsigned level) const
  {
    for (size_t i = 0; i < nvs_v.size(); i++)
      nvs_v[i]->dump(level);
  }

  // =================================
  // ===== OID_comp
  // =================================

  const OID_comp::nameform_t OID_comp::names_root[] = {
    { "itu__t", 0 },
    { "ccitt", 0 },
    { "itu__r", 0 },
    { "iso", 1 },
    { "joint__iso__itu__t", 2 },
    { "joint__iso__ccitt", 2 },
    { NULL, 0 }
  };

  const OID_comp::nameform_t OID_comp::names_itu[] = {
    { "recommendation", 0 },
    { "question", 1 },
    { "administration", 2 },
    { "network__operator", 3 },
    { "identified__organization", 4 },
    { "r__recommendation", 5 },
    { NULL, 0 }
  };

  const OID_comp::nameform_t OID_comp::names_iso[] = {
    { "standard", 0 },
    { "registration__authority", 1 },
    { "member__body", 2 },
    { "identified__organization", 3 },
    { NULL, 0 }
  };

  // taken from OID repository: http://asn1.elibel.tm.fr/oid/
  const OID_comp::nameform_t OID_comp::names_joint[] = {
    { "presentation", 0 },
    { "asn1", 1 },
    { "association__control", 2 },
    { "reliable__transfer", 3 },
    { "remote__operations", 4 },
    { "ds", 5 },
    { "directory", 5 },
    { "mhs", 6 },
    { "mhs__motis", 6 },
    { "ccr", 7 },
    { "oda", 8 },
    { "ms", 9 },
    { "osi__management", 9 },
    { "transaction__processing", 10 },
    { "dor", 11 },
    { "distinguished__object__reference", 11 },
    { "reference__data__transfer", 12 },
    { "network__layer", 13 },
    { "network__layer__management", 13 },
    { "transport__layer", 14 },
    { "transport__layer__management", 14 },
    { "datalink__layer", 15 },
    { "datalink__layer__management", 15 },
    { "datalink__layer__management__information", 15 },
    { "country", 16 },
    { "registration__procedures", 17 },
    { "registration__procedure", 17 },
    { "physical__layer", 18 },
    { "physical__layer__management", 18 },
    { "mheg", 19 },
    { "genericULS", 20 },
    { "generic__upper__layers__security", 20 },
    { "guls", 20 },
    { "transport__layer__security__protocol", 21 },
    { "network__layer__security__protocol", 22 },
    { "international__organizations", 23 },
    { "internationalRA", 23 },
    { "sios", 24 },
    { "uuid", 25 },
    { "odp", 26 },
    { "upu", 40 },
    { NULL, 0 }
  };

  OID_comp::OID_comp(Identifier *p_name, Value *p_number)
    : Node(), Location(), defdval(0), name(p_name), number(p_number), var(0)
  {
    if (p_name) {
      if (p_number) formtype = NAMEANDNUMBERFORM;
      else formtype = NAMEFORM;
    } else {
      if (p_number) formtype = NUMBERFORM;
      else FATAL_ERROR("NULL parameter: Common::OID_comp::OID_comp()");
    }
  }

  OID_comp::OID_comp(Value *p_defdval)
    : Node(), Location(), formtype(DEFDVALUE), defdval(p_defdval), name(0),
      number(0), var(0)
  {
    if(!p_defdval)
      FATAL_ERROR("NULL parameter: Common::OID_comp::OID_comp()");
  }

  OID_comp::~OID_comp()
  {
    delete defdval;
    delete name;
    delete number;
    delete var;
  }

  OID_comp *OID_comp::clone() const
  {
    FATAL_ERROR("OID_comp::clone");
  }

  void OID_comp::chk_OID(ReferenceChain& refch, Value *parent, size_t index,
                     oidstate_t& state)
  {
    Error_Context ec(this, "In component #%lu of OBJECT IDENTIFIER value",
                     static_cast<unsigned long>(index + 1));
    switch (formtype) {
    case NAMEFORM: {
      Int v_Int = detect_nameform(state);
      if (v_Int >= 0) {
	// we have recognized the NameForm
	number = new Value(Value::V_INT, v_Int);
	number->set_fullname(get_fullname());
	number->set_my_scope(parent->get_my_scope());
	number->set_location(*this);
	// the component is finished, jump out from switch
	break;
      } else {
	// otherwise treat as a defined value and continue
	Common::Reference *ref;
	if (parent->is_asn1()) ref = new Asn::Ref_defd_simple(0, name);
	else ref = new Ttcn::Reference(0, name);
	name = 0;
	defdval = new Value(Value::V_REFD, ref);
	defdval->set_fullname(get_fullname());
	defdval->set_my_scope(parent->get_my_scope());
	defdval->set_location(*this);
	formtype = DEFDVALUE;
      }
      }
    // no break
    case DEFDVALUE:
      chk_defdvalue_OID(refch, state);
      break;
    case NUMBERFORM:
      chk_numberform_OID(state);
      break;
    case NAMEANDNUMBERFORM:
      chk_nameandnumberform(state);
      break;
    default:
      FATAL_ERROR("OID_comp::chk_OID()");
    } // switch
  }

  void OID_comp::chk_ROID(ReferenceChain& refch, size_t index)
  {
    Error_Context ec(this, "In component #%lu of RELATIVE-OID value",
      static_cast<unsigned long> (index + 1));
    switch (formtype) {
    case DEFDVALUE:
      chk_defdvalue_ROID(refch);
      break;
    case NUMBERFORM:
    case NAMEANDNUMBERFORM:
      chk_numberform_ROID();
      break;
    default:
      FATAL_ERROR("OID_comp::chk_ROID()");
    }
  }

  bool OID_comp::has_error()
  {
    if (formtype == DEFDVALUE) return defdval->has_oid_error();
    else return number->get_value_refd_last()->get_valuetype() != Value::V_INT;
  }

  void OID_comp::get_comps(vector<string>& comps)
  {
    if (formtype == DEFDVALUE) defdval->get_oid_comps(comps);
    else if (formtype == VARIABLE) {
      comps.add(new string("OBJID::from_INTEGER(" + var->get_stringRepr() + ")"));
    }
    else comps.add(new string(number->get_stringRepr() + "u"));
  }
  
  bool OID_comp::is_variable()
  {
    return (formtype == VARIABLE);
  }

  Int OID_comp::detect_nameform(oidstate_t& state)
  {
    const string& name_str = name->get_name();
    Int ret_val = -1;
    switch (state) {
    case START:
      if (name_str == "itu__t" || name_str == "ccitt") {
	state = ITU;
	ret_val = 0;
      } else if (name_str == "itu__r") {
	warning("Identifier `%s' should not be used as NameForm",
	  name->get_dispname().c_str());
        state = ITU;
	ret_val = 0;
      } else if (name_str == "iso") {
	state = ISO;
	ret_val = 1;
      } else if (name_str == "joint__iso__itu__t"
        || name_str == "joint__iso__ccitt") {
	state = JOINT;
	ret_val = 2;
      }
      break;
    case ITU:
      for (const nameform_t *nf = names_itu; nf->name != NULL; nf++) {
	if (name_str == nf->name) {
	  ret_val = nf->value;
	  switch (nf->value) {
	  case 0:
	    state = ITU_REC;
	    break;
	  case 5:
	    warning("Identifier `%s' should not be used as NameForm",
	      name->get_dispname().c_str());
	    // no break
	  default:
	    state = LATER;
	  }
	  break;
	}
      }
      break;
    case ISO:
      for (const nameform_t *nf = names_iso; nf->name != NULL; nf++) {
	if (name_str == nf->name) {
	  ret_val = nf->value;
	  state = LATER;
	  break;
	}
      }
      break;
    case JOINT:
      for (const nameform_t *nf = names_joint; nf->name != NULL; nf++) {
	if (name_str == nf->name) {
	  ret_val = nf->value;
	  state = LATER;
	  warning("Identifier `%s' should not be used as NameForm",
	    name->get_dispname().c_str());
	  break;
	}
      }
      break;
    case ITU_REC:
      if (name_str.size() == 1) {
	char c = name_str[0];
	if (c >= 'a' && c <= 'z') {
	  ret_val = c - 'a' + 1;
	  state = LATER;
	}
      }
      break;
    default:
      break;
    }
    return ret_val;
  }

  void OID_comp::chk_defdvalue_OID(ReferenceChain& refch, oidstate_t& state)
  {
    Value *v = defdval->get_value_refd_last();
    switch (v->get_valuetype()) {
    case Value::V_ERROR:
      state = LATER;
      break;
    case Value::V_INT:
      // if the component is a reference to an integer value
      // it shall be set to number form
      formtype = NUMBERFORM;
      number = defdval;
      defdval = 0;
      chk_numberform_OID(state);
      break;
    case Value::V_OID:
      if (state != START) defdval->error("INTEGER or RELATIVE-OID "
	"value was expected");
      state = LATER;
      v->chk_OID(refch);
      break;
    case Value::V_ROID:
      switch (state) {
      case ITU_REC:
	state = LATER;
	// no break
      case LATER:
	break;
      default:
	defdval->error("RELATIVE-OID value cannot be used as the "
	  "%s component of an OBJECT IDENTIFIER value",
	  state == START ? "first" : "second");
	state = LATER;
	break;
      }
      break;
    case Value::V_REFD: {
      Common::Reference* ref = v->get_reference();
      Common::Assignment* as = ref->get_refd_assignment();
      Type* t = as->get_Type()->get_type_refd_last();
      if (t->get_typetype() == Type::T_INT) {
        formtype = VARIABLE;
        var = defdval;
        defdval = 0;
      } else {
        defdval->error("INTEGER variable was expected");
      }
      state = LATER;
      break; }      
    default:
      if (state == START) defdval->error("INTEGER or OBJECT IDENTIFIER "
	"value was expected for the first component");
      else defdval->error("INTEGER or RELATIVE-OID value was expected");
      state = LATER;
      break;
    }
  }

  void OID_comp::chk_numberform_OID(oidstate_t& state)
  {
    Value *v = number->get_value_refd_last();
    switch (v->get_valuetype()) {
    case Value::V_INT:
      // everything is OK, continue the checking
      break;
    case Value::V_ERROR:
      state = LATER;
      return;
    default:
      number->error("INTEGER value was expected in the number form");
      state = LATER;
      return;
    }
    const int_val_t *v_Num = v->get_val_Int();
    if (v_Num->get_val() > static_cast<Int>(UINT_MAX)) {
      number->error("An integer value less than `%u' was expected in the "
        "number form instead of `%s'", UINT_MAX, (v_Num->t_str()).c_str());
      state = LATER;
      return;
    }
    Int v_Int = v_Num->get_val();
    switch (state) {
    case START:
      switch (v_Int) {
      case 0:
	state = ITU;
	break;
      case 1:
	state = ISO;
	break;
      case 2:
	state = JOINT;
	break;
      default:
        number->error("The value of first OBJECT IDENTIFIER component must "
          "be between 0 and 2 instead of %s", Int2string(v_Int).c_str());
	state = LATER;
        break;
      }
      break;
    case ITU:
    case ISO:
      if (v_Int < 0 || v_Int > 39) number->error("The value of "
	  "second OBJECT IDENTIFIER component must be between 0 and 39 "
	  "instead of %s", Int2string(v_Int).c_str());
      if (state == ITU && v_Int == 0) state = ITU_REC;
      else state = LATER;
      break;
    case JOINT:
    default:
      if (v_Int < 0) number->error("A non-negative integer value was "
        "expected instead of %s", Int2string(v_Int).c_str());
      state = LATER;
      break;
    }
  }

  void OID_comp::chk_nameandnumberform(oidstate_t& state)
  {
    // make a backup of state
    oidstate_t oldstate = state;
    chk_numberform_OID(state);
    Value *v = number->get_value_refd_last();
    if (v->get_valuetype() != Value::V_INT || !v->get_val_Int()->is_native())
      return;
    Int v_Int = v->get_val_Int()->get_val();
    const string& name_str = name->get_name();
    switch (oldstate) {
    case START:
      if (!is_valid_name_for_number(name_str, v_Int, names_root)) {
	number->warning("Identifier %s was expected instead of `%s' for "
	  "number %s in the NameAndNumberForm as the first OBJECT IDENTIFIER "
	  "component", get_expected_name_for_number(v_Int, number->is_asn1(),
	  names_root).c_str(), name->get_dispname().c_str(),
	  Int2string(v_Int).c_str());
      }
      break;
    case ITU:
      if (!is_valid_name_for_number(name_str, v_Int, names_itu)) {
	number->warning("Identifier %s was expected instead of `%s' for "
	  "number %s in the NameAndNumberForm as the second OBJECT IDENTIFIER "
	  "component", get_expected_name_for_number(v_Int, number->is_asn1(),
	  names_itu).c_str(), name->get_dispname().c_str(),
	  Int2string(v_Int).c_str());
      }
      break;
    case ISO:
      if (!is_valid_name_for_number(name_str, v_Int, names_iso)) {
	number->warning("Identifier %s was expected instead of `%s' for "
	  "number %s in the NameAndNumberForm as the second OBJECT IDENTIFIER "
	  "component", get_expected_name_for_number(v_Int, number->is_asn1(),
	  names_iso).c_str(), name->get_dispname().c_str(),
	  Int2string(v_Int).c_str());
      }
      break;
    case JOINT:
      if (!is_valid_name_for_number(name_str, v_Int, names_joint)) {
	number->warning("Identifier %s was expected instead of `%s' for "
	  "number %s in the NameAndNumberForm as the second OBJECT IDENTIFIER "
	  "component", get_expected_name_for_number(v_Int, number->is_asn1(),
	  names_joint).c_str(), name->get_dispname().c_str(),
	  Int2string(v_Int).c_str());
      }
      break;
    case ITU_REC:
      if (v_Int >= 1 && v_Int <= 26 &&
	(name_str.size() != 1 || name_str[0] != 'a' + v_Int - 1))
	number->warning("Identifier `%c' was expected instead of `%s' for "
	  "number %s in the NameAndNumberForm as the third OBJECT IDENTIFIER "
	  "component", static_cast<char>('a' + v_Int - 1), name->get_dispname().c_str(),
	  Int2string(v_Int).c_str());
      break;
    default:
      // no warning can be displayed in later states
      break;
    }
  }

  bool OID_comp::is_valid_name_for_number(const string& p_name,
    const Int& p_number, const nameform_t *p_names)
  {
    bool ret_val = true;
    for (const nameform_t *nf = p_names; nf->name != NULL; nf++) {
      if (static_cast<unsigned int>(p_number) == nf->value) {
	if (p_name == nf->name) return true;
	else ret_val = false;
      }
    }
    return ret_val;
  }

  string OID_comp::get_expected_name_for_number(const Int& p_number,
    bool p_asn1, const nameform_t *p_names)
  {
    size_t nof_expected_names = 0;
    for (const nameform_t *nf = p_names; nf->name != NULL; nf++) {
      if (static_cast<unsigned int>(p_number) == nf->value) nof_expected_names++;
    }
    if (nof_expected_names <= 0)
      FATAL_ERROR("OID_comp::get_expected_name_for_number()");
    string ret_val;
    size_t i = 0;
    for (const nameform_t *nf = p_names; nf->name != NULL; nf++) {
      if (static_cast<unsigned int>(p_number) == nf->value) {
	if (i > 0) {
	  if (i < nof_expected_names - 1) ret_val += ", ";
	  else ret_val += " or ";
	}
	ret_val += "`";
	Identifier id(Identifier::ID_NAME, string(nf->name));
	if (p_asn1) ret_val += id.get_asnname();
	else ret_val += id.get_ttcnname();
	ret_val += "'";
	i++;
      }
    }
    return ret_val;
  }

  void OID_comp::chk_defdvalue_ROID(ReferenceChain& refch)
  {
    Value *v = defdval->get_value_refd_last();
    switch (v->get_valuetype()) {
    case Value::V_ERROR:
      break;
    case Value::V_INT:
      // if the component is a reference to an integer value
      // it shall be set to number form
      formtype = NUMBERFORM;
      number = defdval;
      defdval = 0;
      chk_numberform_ROID();
      break;
    case Value::V_ROID:
      v->chk_ROID(refch);
      break;
    default:
      defdval->error("INTEGER or RELATIVE-OID value was expected");
      break;
    }
  }

  void OID_comp::chk_numberform_ROID()
  {
    Value *v = number->get_value_refd_last();
    switch (v->get_valuetype()) {
    case Value::V_INT: {
      const int_val_t *v_Num = v->get_val_Int();
      if (*v_Num > INT_MAX) {
        number->error("An integer value less than `%d' was expected instead "
          "of `%s'", INT_MAX, (v_Num->t_str()).c_str());
        return;
      }
      Int v_Int = v_Num->get_val();
      if (v_Int < 0)
        number->error("A non-negative integer value was expected instead "
          "of %s", Int2string(v_Int).c_str());
      break; }
    case Value::V_ERROR:
      break;
    default:
      number->error("INTEGER value was expected in the number form");
      break;
    }
  }

  void OID_comp::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    if (defdval) defdval->set_fullname(p_fullname);
    if (number) number->set_fullname(p_fullname);
    if (var) var->set_fullname(p_fullname);
  }

  void OID_comp::set_my_scope(Scope *p_scope)
  {
    if(defdval) defdval->set_my_scope(p_scope);
    if(number) number->set_my_scope(p_scope);
    if(var) var->set_my_scope(p_scope);
  }

  void OID_comp::append_stringRepr(string& str) const
  {
    switch (formtype) {
    case NAMEFORM:
      str += name->get_dispname();
      break;
    case NUMBERFORM:
    case NAMEANDNUMBERFORM:
      str += number->get_stringRepr();
      break;
    case DEFDVALUE:
      str += defdval->get_stringRepr();
      break;
    case VARIABLE:
      str += var->get_stringRepr();
      break;
    default:
      str += "<unknown OID component>";
      break;
    }
  }

  // =================================
  // ===== CharsDefn
  // =================================

  CharsDefn::CharsDefn(string *p_str)
    : Node(), Location(), selector(CD_CSTRING), checked(false)
  {
    if(!p_str) FATAL_ERROR("CharsDefn::CharsDefn()");
    u.str=p_str;
  }

  CharsDefn::CharsDefn(Value *p_val)
    : Node(), Location(), selector(CD_VALUE), checked(false)
  {
    if(!p_val) FATAL_ERROR("CharsDefn::CharsDefn()");
    u.val=p_val;
  }

  CharsDefn::CharsDefn(Int p_g, Int p_p, Int p_r, Int p_c)
    : Node(), Location(), selector(CD_QUADRUPLE), checked(false)
  {
    u.quadruple.g=p_g;
    u.quadruple.p=p_p;
    u.quadruple.r=p_r;
    u.quadruple.c=p_c;
  }

  CharsDefn::CharsDefn(Int p_c, Int p_r)
    : Node(), Location(), selector(CD_TUPLE), checked(false)
  {
    u.tuple.c=p_c;
    u.tuple.r=p_r;
  }

  CharsDefn::CharsDefn(Block *p_block)
    : Node(), Location(), selector(CD_BLOCK), checked(false)
  {
    if(!p_block) FATAL_ERROR("CharsDefn::CharsDefn()");
    u.block=p_block;
  }

  CharsDefn::~CharsDefn()
  {
    switch(selector) {
    case CD_CSTRING:
      delete u.str;
      break;
    case CD_VALUE:
      delete u.val;
      break;
    case CD_BLOCK:
      delete u.block;
      break;
    default:
      break;
    } // switch
  }

  CharsDefn *CharsDefn::clone() const
  {
    FATAL_ERROR("CharsDefn::clone");
  }

  void CharsDefn::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    switch (selector) {
    case CD_VALUE:
      u.val->set_fullname(p_fullname);
      break;
    case CD_BLOCK:
      u.block->set_fullname(p_fullname);
      break;
    default:
      break;
    }
  }

  void CharsDefn::set_my_scope(Scope *p_scope)
  {
    if(selector==CD_VALUE)
      u.val->set_my_scope(p_scope);
  }

  void CharsDefn::parse_block()
  {
    if(selector!=CD_BLOCK) return;
    Block *t_block=u.block;
    Node *node=t_block->parse(KW_Block_QuadrupleOrTuple);
    CharsDefn *cd=dynamic_cast<CharsDefn*>(node);
    if(!cd) cd=new CharsDefn(0, 0, 0, 0);
    delete t_block;
    if(cd->selector==CD_QUADRUPLE) {
      u.quadruple=cd->u.quadruple;
      selector=CD_QUADRUPLE;
    }
    else {
      u.tuple=cd->u.tuple;
      selector=CD_TUPLE;
    }
    delete cd;
  }

  void CharsDefn::chk()
  {
    if(checked) return;
    checked=true;
    if(selector==CD_BLOCK) parse_block();
    switch(selector) {
    case CD_QUADRUPLE:
      if(u.quadruple.g>127) {
        error("In quadruple: Group value must be in range 0..127");
        u.quadruple.g=0;
      }
      if(u.quadruple.p>255) {
        error("In quadruple: Plane value must be in range 0..255");
        u.quadruple.p=0;
      }
      if(u.quadruple.r>255) {
        error("In quadruple: Row value must be in range 0..255");
        u.quadruple.r=0;
      }
      if(u.quadruple.c>255) {
        error("In quadruple: Cell value must be in range 0..255");
        u.quadruple.c=0;
      }
      break;
    case CD_TUPLE:
      if(u.tuple.c>7) {
        error("In tuple: Column value must be in range 0..7");
        u.tuple.c=0;
      }
      if(u.tuple.r>15) {
        error("In tuple: Row value must be in range 0..15");
        u.tuple.r=0;
      }
      break;
    default:
      break;
    } // switch
  }

  string CharsDefn::get_string(ReferenceChain *refch)
  {
    chk();
    switch(selector) {
    case CD_CSTRING:
      return *u.str;
    case CD_QUADRUPLE:
      error("Quadruple form is not allowed here.");
      return string("\0");
    case CD_TUPLE:{
      error("Tuple form is not allowed here.");
      char c=u.tuple.c*16+u.tuple.r;
      return string(1, &c); }
    case CD_VALUE:
      switch(u.val->get_value_refd_last(refch)->get_valuetype()) {
      case Value::V_ERROR:
        return string();
      case Value::V_CHARSYMS:
      case Value::V_CSTR:
        return u.val->get_val_str();
      case Value::V_ISO2022STR:
        error("Reference to string value was expected"
              " (instead of ISO-2022 string).");
        return string();
      case Value::V_USTR:
        error("Reference to string value was expected"
              " (instead of ISO-10646 string).");
        return string();
      default:
        error("Reference to character string value was expected.");
        return string();
      } // switch valuetype
      break;
    default:
      FATAL_ERROR("CharsDefn::get_string()");
      // to eliminate warning
      return string();
    }
  }

  ustring CharsDefn::get_ustring(ReferenceChain *refch)
  {
    chk();
    switch(selector) {
    case CD_CSTRING:
      return ustring(*u.str);
      break;
    case CD_QUADRUPLE:
      return ustring(u.quadruple.g, u.quadruple.p,
                     u.quadruple.r, u.quadruple.c);
      break;
    case CD_TUPLE:
      error("Tuple form is not allowed here.");
      return ustring(0, 0, 0, u.tuple.c*16+u.tuple.r);
      break;
    case CD_VALUE:
      switch(u.val->get_value_refd_last(refch)->get_valuetype()) {
      case Value::V_ERROR:
        return ustring();
      case Value::V_CSTR:
      case Value::V_USTR:
      case Value::V_CHARSYMS:
        return u.val->get_val_ustr();
      case Value::V_ISO2022STR:
        error("Reference to ISO-10646 string value was expected"
              " (instead of ISO-2022 string).");
        return ustring();
      default:
        error("Reference to string value was expected.");
        return ustring();
      } // switch valuetype
      break;
    default:
      FATAL_ERROR("CharsDefn::get_ustring()");
      // to eliminate warning
      return ustring();
    }
  }

  string CharsDefn::get_iso2022string(ReferenceChain *refch)
  {
    chk();
    switch(selector) {
    case CD_CSTRING:
      return *u.str;
    case CD_QUADRUPLE:
      error("Quadruple form is not allowed here");
      return string("\0");
    case CD_TUPLE:{
      char c=u.tuple.c*16+u.tuple.r;
      return string(1, &c); }
    case CD_VALUE:
      switch(u.val->get_value_refd_last(refch)->get_valuetype()) {
      case Value::V_ERROR:
        return string();
      case Value::V_CSTR:
      case Value::V_ISO2022STR:
      case Value::V_CHARSYMS:
        return u.val->get_val_iso2022str();
      case Value::V_USTR:
        error("Reference to ISO-2022 string value was expected"
              " (instead of ISO-10646 string).");
        return string();
      default:
        error("Reference to string value was expected.");
        return string();
      } // switch valuetype
      break;
    default:
      FATAL_ERROR("CharsDefn::get_iso2022string()");
      // to eliminate warning
      return string();
    }
  }

  size_t CharsDefn::get_len(ReferenceChain *refch)
  {
    chk();
    switch(selector) {
    case CD_CSTRING:
      return u.str->size();
    case CD_QUADRUPLE:
    case CD_TUPLE:
      return 1;
    case CD_VALUE:
      switch(u.val->get_value_refd_last(refch)->get_valuetype()) {
      case Value::V_ERROR:
        return 0;
      case Value::V_CSTR:
      case Value::V_USTR:
      case Value::V_ISO2022STR:
      case Value::V_CHARSYMS:
        return u.val->get_val_strlen();
      default:
        error("Reference to string value was expected.");
        return 0;
      } // switch valuetype
      break;
    default:
      FATAL_ERROR("CharsDefn::get_LEN()");
      // to eliminate warning
      return 0;
    }
  }

  void CharsDefn::dump(unsigned level) const
  {
    switch (selector) {
    case CD_CSTRING:
      DEBUG(level, "\"%s\"", u.str->c_str());
      break;
    case CD_QUADRUPLE:
      DEBUG(level, "{%s, %s, %s, %s}",
	    Int2string(u.quadruple.g).c_str(),
	    Int2string(u.quadruple.p).c_str(),
	    Int2string(u.quadruple.r).c_str(),
	    Int2string(u.quadruple.c).c_str());
      break;
    case CD_TUPLE:
      DEBUG(level, "{%s, %s}", Int2string(u.tuple.c).c_str(),
	    Int2string(u.tuple.r).c_str());
      break;
    case CD_VALUE:
      u.val->dump(level);
      break;
    case CD_BLOCK:
      u.block->dump(level);
      break;
    default:
      break;
    }
  }

  // =================================
  // ===== CharSyms
  // =================================

  CharSyms::CharSyms() : Node(), Location(), selector(CS_UNDEF)
  {
  }

  CharSyms::~CharSyms()
  {
    for(size_t i=0; i<cds.size(); i++) delete cds[i];
    cds.clear();
    switch(selector) {
    case CS_UNDEF:
      break;
    case CS_CSTRING:
    case CS_ISO2022STRING:
      delete u.str;
      break;
    case CS_USTRING:
      delete u.ustr;
      break;
    default:
      FATAL_ERROR("CharSyms::~CharSyms()");
    } // switch
  }

  CharSyms *CharSyms::clone() const
  {
    FATAL_ERROR("CharSyms::clone");
  }

  void CharSyms::set_fullname(const string& p_fullname)
  {
    Node::set_fullname(p_fullname);
    for (size_t i = 0; i < cds.size(); i++)
      cds[i]->set_fullname(p_fullname + "." + Int2string(i));
  }

  void CharSyms::set_my_scope(Scope *p_scope)
  {
    for(size_t i=0; i<cds.size(); i++)
      cds[i]->set_my_scope(p_scope);
  }

  void CharSyms::add_cd(CharsDefn *p_cd)
  {
    if(!p_cd) return;
    cds.add(p_cd);
  }

  string CharSyms::get_string(ReferenceChain *refch)
  {
    switch(selector) {
    case CS_UNDEF:{
      bool destroy_refch=refch==0;
      if(refch) refch->mark_state();
      else refch=new ReferenceChain(this, "While checking"
                                    " charstring value");
      u.str=new string();
      selector=CS_CHECKING;
      if(refch->add(get_fullname()))
        for(size_t i=0; i<cds.size(); i++)
          *u.str+=cds[i]->get_string(refch);
      if(destroy_refch) delete refch;
      else refch->prev_state();
      selector=CS_CSTRING;
    }
    // no break
    case CS_CSTRING:
      return *u.str;
    case CS_USTRING:
      error("ISO-10646 charstring cannot be used in charstring context");
      return string();
    case CS_ISO2022STRING:
      error("ISO-2022 charstring cannot be used in charstring context");
      return string();
    case CS_CHECKING:
      error("Circular reference in `%s'", get_fullname().c_str());
      return string();
    default:
      FATAL_ERROR("CharSyms::get_string()");
      return string();
    } // switch
  }

  ustring CharSyms::get_ustring(ReferenceChain *refch)
  {
    switch(selector) {
    case CS_UNDEF:{
      bool destroy_refch=refch==0;
      if(refch) refch->mark_state();
      else refch=new ReferenceChain(this, "While checking"
                                    " charstring value");
      u.ustr=new ustring();
      selector=CS_CHECKING;
      if(refch->add(get_fullname()))
        for(size_t i=0; i<cds.size(); i++)
          *u.ustr+=cds[i]->get_ustring(refch);
      if(destroy_refch) delete refch;
      else refch->prev_state();
      selector=CS_USTRING;
    }
    // no break
    case CS_USTRING:
      return *u.ustr;
    case CS_CSTRING:
      return ustring(*u.str);
    case CS_ISO2022STRING:
      error("ISO-2022 charstring cannot be used in ISO-10646 context");
      return ustring();
    case CS_CHECKING:
      error("Circular reference in `%s'", get_fullname().c_str());
      return ustring();
    default:
      FATAL_ERROR("CharSyms::get_ustring()");
      return ustring();
    } // switch
  }

  string CharSyms::get_iso2022string(ReferenceChain *refch)
  {
    switch(selector) {
    case CS_UNDEF:{
      bool destroy_refch=refch==0;
      if(refch) refch->mark_state();
      else refch=new ReferenceChain(this, "While checking"
                                    " charstring value");
      u.str=new string();
      selector=CS_CHECKING;
      if(refch->add(get_fullname()))
        for(size_t i=0; i<cds.size(); i++)
          *u.str+=cds[i]->get_iso2022string(refch);
      if(destroy_refch) delete refch;
      else refch->prev_state();
      selector=CS_CSTRING;
    }
    // no break
    case CS_CSTRING:
    case CS_ISO2022STRING:
      return *u.str;
    case CS_USTRING:
      error("ISO-10646 charstring cannot be used in ISO-2022 context");
      return string();
    case CS_CHECKING:
      error("Circular reference in `%s'", get_fullname().c_str());
      return string();
    default:
      FATAL_ERROR("CharSyms::get_iso2022string()");
      return string();
    } // switch
  }

  size_t CharSyms::get_len(ReferenceChain *refch)
  {
    switch(selector) {
    case CS_UNDEF:{
      bool destroy_refch=refch==0;
      if(refch) refch->mark_state();
      else refch=new ReferenceChain(this, "While checking"
                                    " charstring value");
      size_t len=0;
      selector=CS_CHECKING;
      if(refch->add(get_fullname()))
        for(size_t i=0; i<cds.size(); i++)
          len+=cds[i]->get_len(refch);
      if(destroy_refch) delete refch;
      else refch->prev_state();
      selector=CS_UNDEF;
    }
    // no break
    case CS_CSTRING:
    case CS_ISO2022STRING:
      return u.str->size();
    case CS_USTRING:
      return u.ustr->size();
    case CS_CHECKING:
      error("Circular reference in `%s'", get_fullname().c_str());
      return 0;
    default:
      FATAL_ERROR("CharSyms::get_len()");
      return 0;
    } // switch
  }

  void CharSyms::dump(unsigned level) const
  {
    DEBUG(level, "CharSyms:");
    level++;
    for (size_t i = 0; i < cds.size(); i++)
      cds[i]->dump(level);
  }

} // namespace Common
