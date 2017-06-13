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
 *
 ******************************************************************************/
#include "Node.hh"
#include "error.h"
#include <stdio.h>

// =================================
// ===== Node
// =================================

int Node::counter=0;
#ifdef MEMORY_DEBUG
static Node *list_head = 0, *list_tail = 0;
#endif

Node::Node()
{
#ifdef MEMORY_DEBUG
  prev_node = list_tail;
  next_node = 0;
  if (list_tail) list_tail->next_node = this;
  else list_head = this;
  list_tail = this;
#endif
  counter++;
}

Node::Node(const Node& p)
{
#ifdef MEMORY_DEBUG
  prev_node = list_tail;
  next_node = 0;
  if (list_tail) list_tail->next_node = this;
  else list_head = this;
  list_tail = this;
#endif
  counter++;
}

Node::~Node()
{
  counter--;
#ifdef MEMORY_DEBUG
  if (prev_node) prev_node->next_node = next_node;
  else list_head = next_node;
  if (next_node) next_node->prev_node = prev_node;
  else list_tail = prev_node;
#endif
}

void Node::chk_counter()
{
  DEBUG(1, "Node::counter is %d", counter);
  if(counter)
    WARNING("%d nodes were not deleted."
            " Please send a bug report including"
            " the current input file(s).", counter);
#ifdef MEMORY_DEBUG
  for(Node *iter = list_head; iter; iter = iter->next_node) {
    fprintf(stderr, "Undeleted node address: %p.\n", iter);
  }
  list_head = 0;
  list_tail = 0;
#endif
}

