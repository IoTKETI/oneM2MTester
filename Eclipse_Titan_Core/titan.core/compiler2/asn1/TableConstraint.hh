/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Delic, Adam
 *   Raduly, Csaba
 *
 ******************************************************************************/
#ifndef _Asn_Constraint_HH
#define _Asn_Constraint_HH

#include "../Constraint.hh"

namespace Asn {

  /**
   * \addtogroup Constraint
   *
   * @{
   */

  using namespace Common;

  class AtNotation;
  class AtNotations;
  class TableConstraint;

  // not defined here
  //class Identifier;
  class Block;
  class ObjectSet;
  class FieldName;


  /**
   * This class represents an ASN AtNotation.
   */
  class AtNotation : public Node {
  private:
    int levels; /**< number of "."s in \@...compid.compid */
    FieldName *cids; /**< ComponentIds */

    Identifier *oc_fieldname; /**< ObjectClass fieldname */
    Type *firstcomp; /**< link to it */
    Type *lastcomp; /**< link to it */

    AtNotation(const AtNotation& p);
  public:
    AtNotation(int p_levels, FieldName *p_cids);
    virtual ~AtNotation();
    virtual AtNotation* clone() const {return new AtNotation(*this);}
    int get_levels() {return levels;}
    FieldName* get_cids() {return cids;}
    void set_oc_fieldname(const Identifier& p_oc_fieldname);
    Identifier *get_oc_fieldname() {return oc_fieldname;}
    void set_firstcomp(Type *p_t) {firstcomp=p_t;}
    void set_lastcomp(Type *p_t) {lastcomp=p_t;}
    Type* get_firstcomp() {return firstcomp;}
    Type* get_lastcomp() {return lastcomp;}
    string get_dispname() const;
  };

  class AtNotations : public Node {
  private:
    vector<AtNotation> ans; /**< AtNotations */

    AtNotations(const AtNotations& p);
  public:
    AtNotations() : Node() {}
    virtual ~AtNotations();
    virtual AtNotations* clone() const {return new AtNotations(*this);}
    void add_an(AtNotation *p_an);
    size_t get_nof_ans() const {return ans.size();}
    AtNotation* get_an_byIndex(size_t p_i) const {return ans[p_i];}
  };

  /**
   * This class represents TableConstraint (SimpleTableConstraint and
   * ComponentRelationConstraint).
   */
  class TableConstraint : public Constraint {
  private:
    Block *block_os;
    Block *block_ans;
    ObjectSet *os;
    AtNotations *ans;
    Type *consdtype; /**< link to the constrained type */
    const Identifier* oc_fieldname; /**< link to...  */

    TableConstraint(const TableConstraint& p);
  public:
    TableConstraint(Block *p_block_os, Block *p_block_ans);
    virtual ~TableConstraint();
    virtual TableConstraint* clone() const
    {return new TableConstraint(*this);}
    virtual void chk();
    const AtNotations* get_ans() const {return ans;}
    ObjectSet* get_os() const {return os;}
    const Identifier* get_oc_fieldname() const {return oc_fieldname;}
    const char* get_name() const { return "table constraint"; }
  private:
    void parse_blocks();
  };

  /** @} end of Constraint group */

} // namespace Asn

#endif // _Asn_Constraint_HH
