/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Godar, Marton
 *   Raduly, Csaba
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
#ifndef LIST_HH_
#define LIST_HH_

#include <cstdio>
#include <cstdlib>
#include <cstring>

template <class T>
class Item {
  Item(const Item<T> & other); // not implemented
  Item<T> & operator=(const Item<T> & rhs); // not implemented
  // Default destructor is used
public:
  Item();

  T Data;
  Item * Next; // pointer to the next element
  Item * Prev; // pointer to the previous element
};

template <class T>
Item<T>::Item()
: Data(T())
, Next(NULL)
, Prev(NULL) {
}

template <class T>
class List {
private:

  /**
   * Number of nodes in the list
   */
  size_t Size;

  /**
   * The initial node in the list
   */
  Item<T> * First;

  /**
   * The final node in the list
   */
  Item<T> * Last;

  /**
   * Memory free up
   */
  void freeMemory();

public:
  List();
  List(const List<T> & other);
  List<T> & operator=(const List<T> & other);
  ~List();

  typedef Item<T> * iterator;

  /**
   * Returns the size of the list
   */
  size_t length() const {
    return Size;
  }

  size_t size() const {
    return Size;
  }

  /**
   * True if List is empty, false otherwise
   */
  bool empty() const {
    return Size == 0;
  }

  /**
   * Clear list
   */
  void clear();

  /**
   * Sort the list using the < operator.
   * To be used when T is a type with operator<
   */
  void sort();

  /**
   * Sort the list using the compare function.
   * To be used when T is a pointer but we don't want to compare the pointers.
   */
  void sort(bool (*comparison_function)(const T lhs, const T rhs));

  /**
   * Returns with a pointer to the begin of the list
   */
  Item<T> * begin() const {
    return First;
  }

  /**
   * Returns with a pointer to the end of the list
   */
  Item<T> * end() const {
    return Last;
  }

  /**
   * Returns with reference to the first element
   */
  T & front() {
    return First->Data;
  }

  const T & front() const {
    return First->Data;
  }

  /**
   * Returns with reference to the last element
   */
  T & back() {
    return Last->Data;
  }

  const T & back() const {
    return Last->Data;
  }

  /**
   * Pushes element at the end of the list
   */
  void push_back(const T & element);

  /**
   * Pushes element at the front of the list
   */
  void push_front(const T & element);

  /**
   * Removes the final element in the list
   */
  void pop_back();

  /**
   * Removes the node from the list
   */
  void remove(iterator& node);

  /**
   * Removes duplicated elements from the list.
   */
  void remove_dups();
};

template <class T>
List<T>::List()
: Size(0)
, First(NULL)
, Last(NULL) {
}

template <class T>
List<T>::List(const List<T> & other)
: Size(0)
, First(NULL)
, Last(NULL) {
  if (other.empty()) return;

  Item<T> * CurrentNode = other.First;
  Item<T> * MyFinalNode = NULL;

  while (CurrentNode != NULL) {
    if (MyFinalNode == NULL) // first element
    {
      MyFinalNode = new Item<T>;
      MyFinalNode->Data = CurrentNode->Data;
      MyFinalNode->Next = NULL;
      MyFinalNode->Prev = NULL;
      First = MyFinalNode;
      Last = MyFinalNode;
    } else {
      MyFinalNode->Next = new Item<T>;
      MyFinalNode->Next->Data = CurrentNode->Data;
      MyFinalNode->Next->Next = NULL;
      MyFinalNode->Next->Prev = MyFinalNode;
      MyFinalNode = MyFinalNode->Next;
      Last = MyFinalNode;
    }

    CurrentNode = CurrentNode->Next;
    ++Size;
  }
}

template <class T>
List<T>::~List() {
  freeMemory();
}

template <class T>
List<T> & List<T>::operator=(const List<T> & other) {
  freeMemory();

  if (other.empty()) return *this;

  Item<T> * CurrentNode = other.First;
  Item<T> * MyFinalNode = NULL;

  while (CurrentNode != NULL) {
    if (MyFinalNode == NULL) {
      MyFinalNode = new Item<T>;
      MyFinalNode->Data = CurrentNode->Data;
      MyFinalNode->Next = NULL;
      MyFinalNode->Prev = NULL;
      First = MyFinalNode;
      Last = MyFinalNode;
    } else {
      MyFinalNode->Next = new Item<T>;
      MyFinalNode->Next->Data = CurrentNode->Data;
      MyFinalNode->Next->Next = CurrentNode->Next;
      MyFinalNode->Next->Prev = MyFinalNode;
      MyFinalNode = MyFinalNode->Next;
      Last = MyFinalNode;
    }

    CurrentNode = CurrentNode->Next;
    ++Size;
  }

  return *this;
}

template <class T>
void List<T>::freeMemory() {
  if (Size > 0) // if list is not empty
  {
    Item<T> * CurrentNode = First;
    Item<T> * NodeForDelete = NULL;

    for (size_t i = 0; i != Size; ++i) {
      NodeForDelete = CurrentNode;
      CurrentNode = CurrentNode->Next;
      delete NodeForDelete;
    }

    Size = 0;
  }

  First = NULL;
  Last = NULL;
}

template <class T>
void List<T>::remove(iterator& node) {
  if (Size == 0) return;

  if (node == NULL) return;

  if (node->Prev == NULL) // if this node was the first element in the list
  {
    First = node->Next;
  } else {
    node->Prev->Next = node->Next;
  }

  if (node->Next == NULL) // if this node was the last element in the list
  {
    Last = node->Prev;
  } else {
    node->Next->Prev = node->Prev;
  }

  delete(node);
  node = 0;

  --Size;
}

template <class T>
void List<T>::clear() {
  freeMemory();
}

template <class T>
void List<T>::sort() {
  if (Size <= 1) return;

  // Selection sort
  for (Item<T>* left = First; left; left = left->Next) {
    Item<T>* mini = left;
    for (Item<T>* curr = left->Next; curr; curr = curr->Next) {
      if (curr->Data < mini->Data) mini = curr;
    }

    if (mini) { // swap!
      T temp(mini->Data);
      mini->Data = left->Data;
      left->Data = temp;
    }
  }
}

template <class T>
void List<T>::sort(bool (*comparison_function)(const T lhs, const T rhs)) {
  if (Size <= 1) return;

  // Selection sort
  for (Item<T>* left = First; left; left = left->Next) {
    Item<T>* mini = left;
    for (Item<T>* curr = left->Next; curr; curr = curr->Next) {
      if (comparison_function(curr->Data, mini->Data)) mini = curr;
    }

    if (mini) { // swap!
      T temp(mini->Data);
      mini->Data = left->Data;
      left->Data = temp;
    }
  }
}

template <class T>
void List<T>::push_back(const T & element) {
  Item<T> * NewNode = new Item<T>;
  NewNode->Data = element;
  NewNode->Next = NULL;

  if (Size == 0) // if list is empty
  {
    NewNode->Prev = NULL;
    First = NewNode;
    Last = NewNode;
  } else {
    NewNode->Prev = Last;
    Last->Next = NewNode;
    Last = NewNode;
  }

  ++Size;
}

template <class T>
void List<T>::push_front(const T & element) {
  Item<T> * NewNode = new Item<T>;
  NewNode->Data = element;
  NewNode->Prev = NULL;

  if (Size == 0) // if list is empty
  {
    NewNode->Next = NULL;
    First = NewNode;
    Last = NewNode;
  } else {
    NewNode->Next = First;
    First->Prev = NewNode;
    First = NewNode;
  }

  ++Size;
}

template <class T>
void List<T>::pop_back() {
  Item<T> * LastNode = Last;

  if (Size == 1) {
    First = NULL;
    Last = NULL;
    delete(LastNode);
    --Size;
  } else if (Size > 1) {
    Last->Prev->Next = NULL;
    Last = Last->Prev;
    delete(LastNode);
    --Size;
  }
}

template <class T>
void List<T>::remove_dups() {
  Item<T>* ptr1 = First, *ptr2, *dup;

  while (ptr1 != NULL && ptr1->Next != NULL) {
    ptr2 = ptr1;
    while (ptr2->Next != NULL) {
      if (ptr1->Data == ptr2->Next->Data) {
        dup = ptr2->Next;
        ptr2->Next = ptr2->Next->Next;
        //If the last element is a duplicate
        if (ptr2->Next == NULL) {
          Last = ptr2;
        } else {
          ptr2->Next->Prev = ptr2;
        }
        delete(dup);
        Size--;
      } else {
        ptr2 = ptr2->Next;
      }
    }
    ptr1 = ptr1->Next;
  }

}

#endif /* LIST_HH_ */
