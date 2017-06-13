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
 *
 ******************************************************************************/

#ifndef REFDINDEX_HH
#define REFDINDEX_HH

#ifdef TITAN_RUNTIME_2

/** This class contains the functions needed for adding and removing referenced
  * indexes to record of/set of types and the optional type (only in RT2).
  * By default the functions are empty.*/
class RefdIndexInterface
{
public:
  virtual ~RefdIndexInterface() {}
  virtual void add_refd_index(int) {}
  virtual void remove_refd_index(int) {}
};

/** References to record of/set of elements through 'out' and 'inout' function
  * parameters are handled by this class.
  * Usage: create instances of this class before the function call (one instance
  * for each referenced index), and place the instances and the function call in
  * a block (so the destructor is called immediately after the function call).
  * This way the referenced indexes are cleaned up even if the function call ends
  * with an exception (DTE) */
class RefdIndexHandler
{
public:
  RefdIndexHandler(RefdIndexInterface* p_container, int p_index)
  {
    container = p_container;
    index = p_index;
    container->add_refd_index(index);
  }
  
  ~RefdIndexHandler()
  {
    container->remove_refd_index(index);
  }
  
private:
  RefdIndexInterface* container;
  int index;
};

#endif /* TITAN_RUNTIME_2 */

#endif /* REFDINDEX_HH */

