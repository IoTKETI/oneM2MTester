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
 *   Delic, Adam
 *   Forstner, Matyas
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Janos Zoltan – initial implementation
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#include "Struct_of.hh"

#include "../common/memory.h"
#include "Error.hh"
#include "Logger.hh"
#include "Template.hh"

#include "../common/dbgnew.hh"
#include <string.h>

void **allocate_pointers(int n_elements)
{
  void **ret_val = (void**)Malloc(n_elements * sizeof(void*));
  for (int elem_count = 0; elem_count < n_elements; elem_count++)
    ret_val[elem_count] = NULL;
  return ret_val;
}

void **reallocate_pointers(void **old_pointer, int old_n_elements,
  int n_elements)
{
  void **ret_val = (void**)Realloc(old_pointer, n_elements * sizeof(void*));
  for (int elem_count = old_n_elements; elem_count < n_elements; elem_count++)
    ret_val[elem_count] = NULL;
  return ret_val;
}

void free_pointers(void **old_pointer)
{
  Free(old_pointer);
}

/*
  Generic comparison function for 'set of' values. The fifth argument
  is a pointer to a type-specific callback function: boolean
  compare_function(Base_Type *left_ptr, int left_index,
    Base_Type *right_ptr, int right_index);

  - Arguments left_ptr and right_ptr shall point to the operands of comparison.
    They are handled transparently by the comparison function.

  - The function returns whether the element of left operand at position
    left_index equals to the element of right operand at position right_index.

  - In case of invalid value pointers, index overflow or negative indices the
    behaviour of the function is undefined.
*/
#ifdef TITAN_RUNTIME_2
boolean compare_set_of(const Record_Of_Type *left_ptr, int left_size,
  const Record_Of_Type *right_ptr, int right_size,
  compare_function_t compare_function)
#else
boolean compare_set_of(const Base_Type *left_ptr, int left_size,
  const Base_Type *right_ptr, int right_size,
  compare_function_t compare_function)
#endif
{
  if (left_size < 0 || right_size < 0 ||
     left_ptr == NULL || right_ptr == NULL)
  {
     TTCN_error("Internal error: compare_set_of: invalid argument.");
  }

  // if the sizes are different the values cannot be equal
  if (left_size != right_size) return FALSE;
  // if both values are empty they must be equal
  if (left_size == 0) return TRUE;

  //stores which have already been matched
  boolean *covered = (boolean*)Malloc(left_size * sizeof(boolean));
  //initially none of them
  memset(covered, 0, left_size * sizeof(boolean));

  boolean pair_found;
  int left_index, right_index;//the actual indices
  int first_on_right = 0;//index of the first element to check on the right side
  //index of the last element to check on the right side
  int last_on_right = left_size-1;

  for(left_index = 0; left_index < left_size; left_index++)
  {
    pair_found = FALSE;
    for(right_index=first_on_right;right_index<=last_on_right;right_index++)
    {
      if(!covered[right_index]
        && compare_function(left_ptr, left_index, right_ptr, right_index))
      {
        //a new match was found
        covered[right_index] = TRUE;
        //if it is the first then check if we can increase the index more,
        // reducing the elements that need to be checked
        if(right_index == first_on_right)
          while(++first_on_right<last_on_right && covered[first_on_right]){}
        //if it is the last then check if we can decrease the index more,
        // reducing the elements that need to be checked
        if(right_index == last_on_right)
          while(--last_on_right>first_on_right && covered[last_on_right]){}
        pair_found = TRUE;
        break;
      }
    }
    //if we can't find a pair to any of the elements, the sets can not
    // match any longer
    if(!pair_found){
      Free(covered);
      return FALSE;
    }
  }

  //if we found a pair to all the elements then they are the same
  Free(covered);
  return TRUE;
}

/*
  Simplified matching function for 'record of' types. It is used when the
  template does not contain permutation matching constructs.
  The fifth argument is a pointer to a type-specific callback function:
  boolean match_function(const Base_Type *value_ptr, int value_index,
    const Restricted_Length_Template *template_ptr, int template_index);

  - Arguments value_ptr and template_ptr shall point to the corresponding
    value and template object. They are handled transparently by the matching
    function.

  - If both index arguments are non-negative the function returns whether
    the value at position value_index matches the template_index-th
    element in the template.

  - If value_index is negative the function returns whether the
    element in the template at index template_index is a '*'
    (ANY_OR_NONE) wildcard.

  - Otherwise (in case of invalid pointers, index overflow or negative
    template_index) the behaviour of the function is undefined.

  The very same abstract algorithm is used in the matching of types of
  bitstring, octetstring, hexstring. The only differences are how we
  match 2 elements, how we find out if a template element is
  an ANY_OR_NONE / ANY element
 */

boolean match_array(const Base_Type *value_ptr, int value_size,
  const Restricted_Length_Template *template_ptr, int template_size,
  match_function_t match_function, boolean legacy)
{
  if (value_ptr == NULL || value_size < 0 || template_ptr == NULL ||
    template_size < 0)
    TTCN_error("Internal error: match_array: invalid argument.");

  // the empty template matches the empty value only
  if (template_size == 0) return value_size == 0;

  int template_index = 0;//the index of the template we are examining

  if (value_size == 0){
    //We matched if the remaining templates are
    // asterisks
    while(template_index < template_size &&
          match_function(value_ptr, -1, template_ptr, template_index, legacy))
      template_index++;

    return template_index == template_size;
  }

  int value_index = 0;//the index of the value we are examining at the point
  //the index of the last asterisk found in the template at the moment
  // -1 if no asterisks were found yet
  int last_asterisk = -1;
  //this is the index of the last value that is matched by
  // the last asterisk in the template
  int last_value_to_asterisk = -1;

  //must finish as we always increase one of the 4 indices or we return
  // and there are limited number of templates and values
  for(;;)
  {
    if(match_function(value_ptr, -1, template_ptr, template_index, legacy))
    {
      //if we found an asterisk we administer it, and step in the template
      last_asterisk = template_index++;
      last_value_to_asterisk = value_index;
    }
    else if(match_function(value_ptr, value_index, template_ptr, template_index,
      legacy))
    {
      //if we found a matching pair we step in both
      value_index++;
      template_index++;
    } else {
      //if we didn't match and we found no asterisk the match failed
      if(last_asterisk == -1) return FALSE;
      //if we found an asterisk than fall back to it
      //and step the value index
      template_index = last_asterisk +1;
      value_index = ++last_value_to_asterisk;
    }

    if(value_index == value_size && template_index == template_size)
    {
      //we finished clean
      return TRUE;
    } else if(template_index == template_size)
    {
      //value_index != value_size at this point so it is pointless
      // to check it in the if statement
      //At the end of the template
      if(match_function(value_ptr, -1, template_ptr, template_index-1, legacy)) {
        //if the templates last element is an asterisk it eats up the values
        return TRUE;
      } else if (last_asterisk == -1){
        //if there were no asterisk the match failed
        return FALSE;
      } else{
        //fall back to the asterisk, and step the value's indices
        template_index = last_asterisk+1;
        value_index = ++last_value_to_asterisk;
      }
    } else if(value_index == value_size)
    {
      //template_index != template_size at this point so it is pointless
      // to check it in the if statement
      //At the end of the value we matched if the remaining templates are
      // asterisks
      while(template_index < template_size &&
            match_function(value_ptr, -1, template_ptr, template_index, legacy))
        template_index++;

      return template_index == template_size;
    }
  }
}

/* Ancillary classes for 'set of' matching */

namespace {

enum edge_status { UNKNOWN, NO_EDGE, EDGE, PAIRS };

/* Class Matching_Table:
 * Responsibilities
 * - index transformation to skip asterisks in the template
 *   the table is initialized in constructor
 * - maintain a matrix that stores the status of edges
 *   (template <-> value relations)
 *   table is initialized explicitly
 * - a flag for each value element to indicate whether it is covered
 * Note: All dynamically allocated memory is collected into this class
 * to avoid memory leaks in case of errors (exceptions).
 */

class Matching_Table {
  match_function_t match_function;
  int value_size;
  int value_start;
  int template_size;
  int template_start;
  int n_asterisks;
  int *template_index_table;
  edge_status **edge_matrix;
  boolean *covered_vector; //tells if a value is covered
  boolean legacy;

  //if the value is covered, then tells by whom it is covered
  int *covered_index_vector;
  int nof_covered;
  int *paired_templates;

  //the matching function requires these pointers
  const Base_Type *value_ptr;

  //they are allocated and freed outside
  const Restricted_Length_Template *template_ptr;

public:
  //the match_set_of will be called from the permutation matcher
  // where the beginning of the examined set might not be at 0 position
  Matching_Table(const Base_Type *par_value_ptr, int par_value_start,
    int par_value_size,const Restricted_Length_Template *par_template_ptr,
    int par_template_start, int par_template_size,
    match_function_t par_match_function, boolean par_legacy)
  {
    match_function = par_match_function;
    value_size = par_value_size;
    value_start = par_value_start;
    template_start = par_template_start;
    value_ptr = par_value_ptr;
    template_ptr = par_template_ptr;
    legacy = par_legacy;
    n_asterisks = 0;
    nof_covered = 0;//to get rid of the linear summing

    // we won't use all elements if there are asterisks in template
    // it is cheaper to allocate it once instead of realloc'ing
    template_index_table = new int[par_template_size];
    // locating the asterisks in the template
    for (int i = 0; i < par_template_size; i++)
    {
      if(match_function(value_ptr, -1,template_ptr, par_template_start+i, legacy))
        n_asterisks++;
      else template_index_table[i - n_asterisks] = i;
    }
    // don't count the asterisks
    template_size = par_template_size - n_asterisks;

    edge_matrix = NULL;
    covered_vector = NULL;
    covered_index_vector = NULL;
    paired_templates = NULL;
  }

  ~Matching_Table()
  {
    delete [] template_index_table;
    if (edge_matrix != NULL)
    {
      for (int i = 0; i < template_size; i++) delete [] edge_matrix[i];
      delete [] edge_matrix;
    }
    delete [] covered_vector;
    delete [] covered_index_vector;
    delete [] paired_templates;
  }

  int get_template_size() const { return template_size; }

  boolean has_asterisk() const { return n_asterisks > 0; }

  void create_matrix()
  {
    edge_matrix = new edge_status*[template_size];
    for (int i = 0; i < template_size; i++)
    {
      edge_matrix[i] = new edge_status[value_size];
      for (int j = 0; j < value_size; j++) edge_matrix[i][j] = UNKNOWN;
    }
    covered_vector = new boolean[value_size];
    for (int j = 0; j < value_size; j++) covered_vector[j] = FALSE;

    paired_templates = new int[template_size];
    for(int j = 0; j < template_size; j++) paired_templates[j] = -1;

    covered_index_vector = new int[value_size];
  }

  edge_status get_edge(int template_index, int value_index)
  {
    if (edge_matrix[template_index][value_index] == UNKNOWN)
    {
      if (match_function(value_ptr, value_start + value_index,
        template_ptr,
        template_start + template_index_table[template_index], legacy))
      {
        edge_matrix[template_index][value_index] = EDGE;
      }else{
        edge_matrix[template_index][value_index] = NO_EDGE;
      }
    }
    return edge_matrix[template_index][value_index];
  }

  void set_edge(int template_index, int value_index, edge_status new_status)
  {
    edge_matrix[template_index][value_index] = new_status;
  }

  boolean is_covered(int value_index) const
    {
    return covered_vector[value_index];
    }

  int covered_by(int value_index) const
    {
    return covered_index_vector[value_index];
    }

  int get_nof_covered() const
    {
    return nof_covered;
    }

  void set_covered(int value_index, int template_index)
  {
    if(!covered_vector[value_index])
      nof_covered++;

    covered_vector[value_index] = TRUE;
    covered_index_vector[value_index] = template_index;
  }

  boolean is_paired(int j)
  {
    return paired_templates[j] != -1;
  }

  void set_paired(int j, int i)
  {
    paired_templates[j] = i;
  }

  int get_paired(int j)
  {
    return paired_templates[j];
  }
};

}

namespace {

/* Tree-list. It is used for storing a tree in BFS order. That is: the
 * first element is the root of the tree, it is followed by its
 * neighbours then the neighbours of the neighbours follow, and so
 * on. The elements can be reached sequentially by the use of the next
 * pointer. Also the parent of an element can be reached via a pointer
 * also. Elements can be inserted in the tree, and with the help of
 * the functions one can move or search in the tree.
 */

class Tree_list {
  /* Elements of the tree-list. */
  struct List_elem {
    int data;
    List_elem *next, *parent;
  } head, *current;

public:
  Tree_list(int head_data)
  {
    head.data = head_data;
    head.next = NULL;
    head.parent = NULL;
    current = &head;
  }

  ~Tree_list()
  {
    for (List_elem *ptr = head.next; ptr != NULL; )
    {
      List_elem *next = ptr->next;
      delete ptr;
      ptr = next;
    }
  }

  void insert_data(int new_data)
  {
    List_elem* newptr = new List_elem;
    newptr->data = new_data;
    newptr->next = current->next;
    newptr->parent = current;
    current->next = newptr;
  }

  void back_step()
  {
    if (current->parent != NULL) current = current->parent;
  }

  void step_forward()
  {
    if (current->next != NULL) current = current->next;
  }

  int head_value() const { return head.data; }
  int actual_data()const { return current->data; }
  boolean is_head() const { return current->parent == NULL; }
  boolean end_of_list() const { return current->next == NULL; }

  boolean do_exists(int find_data) const
    {
    for (const List_elem *ptr = &head; ptr != NULL; ptr = ptr->next)
      if (ptr->data == find_data) return TRUE;
    return FALSE;
    }
};

}

/* Generic matching function for 'set of' values. The interpretation
 * of third argument is the same as for 'record of'.
 */

/*
find_pairs implements an algorithm of matching sets using a graph
algorithm. The correctness of the algorithm is _not_ proven here just
the steps of the algorithm are described. Let denote the two sets by A
and B. The algorithm is for deciding whether for all elements of A
there can be found a matching pair in B. This means that the function
returns true when A matches a subset of B. If the two sets' sizes
equal then the function returns true if the two sets match each other.

The algorithm. For all elements in A the following:

Initialize some variables. After that start a cycle. In the cycle we
try to find a pair for the actual element (at the start actual element
is the element from set A - later it may change). The result can be:

case 1: If there is one pair the cycle is finished we administer the
                pairs (it may need a recursive modification in the
                other pairs) and after that the algorithm continues
                with the next element in A.

case 2: If there is no pair the algorithm returns false because if
                there is no pair for one element, that means that the
                two sets cannot be equal/matching.  case 3: If there
                is a "pair" but is already assigned as a pair to an
                other element of A then this "pair" element is put in
                the alternating tree list (if it's not in the list
                already). The alternating tree is built to modify and
                administer existing pairs in a way that the elements
                which already have pairs will have pairs (but maybe
                other ones) and we will be able to assign a pair for
                the actual element also.

If we haven't found a pair which is not assigned already nor we have
found only non-matching elements then (in other words in case 3) we
built an alternating tree with some "pairs" and continue the cycle by
going to the next element of the alternating tree list.  If it is from
set A, we do as described above, if not - the actual element is from
set B - we do the following. As this element is a pair of another
element it should have a pair, so if its pair is not already in the
alternating tree, we add it to the alternating tree list and continue
the cycle with the next element of the list.

If the algorithm doesn't terminate within the cycles by finding no
pairs (case 2) then it will find pairs (simply or with the help of
alternating trees) for each element of set A, meaning the two sets are
matching. If it terminates that means that the matching process
already failed somewhere resulting in non-matching.
*/

/*
match_set_of implements a set matching algorithm using graphs.

The algorithm implements subset, exact and superset matching.
The description will be general, to ease understanding, giving up on the
specific details. Later, when the base algorithm is already understood
I will mention a part that ain't really needed for this algorithm, but it
is still in the code.

The correctness of the algorithm is _not_ proven here just
the steps of the algorithm are described. Let denote the two sets by A
and B.
The algorithm returns true if the relationship between A and B is right
(A is subset of B and thats what we were checking for).

The algorithm:

In the beginning it makes some checks to see if we can quickly deny matching.

Than it goes through the elements of set A and tries to find a matching element
in set B which still has no pair. If it finds a matching element, then
it is administered.
If no new matching elements can be found than:
    -if the matching requirements (subset) allows this, we step to the next
      element of set A.
    -if the requirements (exact/superset) don't allow this, then we insert
      B's element in the tree, trying to find a new element of B
      for his pair in A.

The tree is walked recursively searching a new pair for elements of set A
(the elements of set B are only inserted so we have links between A's elements,
 when in case of success we try to administer the changes).

The algorithm can end on two ways:
    -not even with the recursive search can we find a pair for an element
      and the matching requirement doesn't allow this, so we return false.
    -we went through all of A's elements, in this case we decide based
      on the matching requirement and the number of pairs found
      (for example in subset matching the number of A's elements and the
       number of found pairs must be the same).

The strange piece of code is when we work with the pair_list.
It stores which element of set A is paired with an element of set B
(if there is no element than it stores -1).
This is not needed in this algorithm, but if we would call this algorithm
in a loop with increasing sets than it would make this algorithm incremental,
by not making the same matching test ever and ever again.
*/

boolean match_set_of_internal(const Base_Type *value_ptr,
  int value_start, int value_size,
  const Restricted_Length_Template *template_ptr,
  int template_start, int template_size,
  match_function_t match_function,
  type_of_matching match_type,
  int* number_of_uncovered, int* pair_list,
  unsigned int number_of_checked, boolean legacy)
{
  Matching_Table table(value_ptr, value_start, value_size,
    template_ptr, template_start, template_size,
    match_function, legacy);

  // we have to use the reduced length of the template
  // (not counting the asterisks)
  int real_template_size = table.get_template_size();

  // handling trivial cases:
  //to be compatible with the previous version
  // is a mistake if the user can't enter an asterisk as a set member
  if(match_type == EXACT && real_template_size < template_size)
    match_type = SUPERSET;

  if(match_type == SUBSET && real_template_size < template_size)
  {
    return TRUE;
  }

  //if the element count does not match the matching mode then we are ready
  if(match_type == SUBSET && value_size > real_template_size) return FALSE;
  if(match_type == EXACT && value_size != real_template_size) return FALSE;
  if(match_type == SUPERSET && value_size < real_template_size) return FALSE;

  // if the template has no non-asterisk elements
  if (real_template_size == 0) {
    // if the template has only asterisks -> matches everything
    if (template_size > 0) return TRUE;
    // the empty template matches the empty value only
    else return (value_size == 0 || match_type == SUPERSET);
  }

  // let's start the real work

  // allocate some memory
  table.create_matrix();

  //if we need increamentality

  if(pair_list != NULL)
    for(int i = 0; i < template_size; i++)
    {
      //if we have values from a previous matching than use them
      // instead of counting them out again
      if(pair_list[i] >= 0)
      {
        table.set_paired(i, pair_list[i]);
        table.set_covered(pair_list[i], i);
        table.set_edge(i, pair_list[i], PAIRS);
      }
    }

  for(int template_index = 0;
    template_index < real_template_size;
    template_index++)
  {
    if(table.is_paired(template_index))
      continue;

    boolean found_route = FALSE;
    Tree_list tree(template_index);
    for (int i = template_index; ; )
    {
      int j;
      if(table.is_paired(i))
        j = table.get_paired(i)+1;
      else
        j = number_of_checked;

      for(; j < value_size; j++)
      {
        //if it is not covered
        if(!table.is_covered(j))
        {
          //if it is not covered then it might be good
          if (table.get_edge(i, j) == EDGE)
          {
            //update the values in the tree
            // and in the other structures
            int new_value_index = j;
            int temp_value_index;
            int actual_node;
            boolean at_end = FALSE;
            for( ; !at_end; )
            {
              at_end = tree.is_head();
              actual_node = tree.actual_data();

              temp_value_index = table.get_paired(actual_node);
              if(temp_value_index != -1)
                table.set_edge(temp_value_index,actual_node,EDGE);

              table.set_paired(actual_node,new_value_index);

              if(pair_list != NULL)
                pair_list[actual_node] = new_value_index;

              table.set_edge(actual_node, new_value_index, PAIRS);
              table.set_covered(new_value_index,actual_node);

              new_value_index = temp_value_index;
              if(!at_end)
                tree.back_step();
            }

            //if we need subset matching
            // and we already matched every value
            // then we have finished
            if(match_type == SUBSET
              && table.get_nof_covered() == value_size)
            {
              return TRUE;
            }

            found_route = TRUE;
            break;
          }
        }
      }
      if (found_route) break;

      //we get here if we couldn't find a new value for the template
      // so we check if there is a covered value.
      //  if we find one then we try to find a new value for his
      // pair template.
      for(j = 0 ; j < value_size; j++)
      {
        if(table.is_covered(j) &&
          table.get_edge(i,j) == EDGE &&
          !tree.do_exists(j + real_template_size))
        {
          int temp_index = table.covered_by(j);
          if(!tree.do_exists(temp_index))
          {
            tree.insert_data(temp_index);
          }
        }
      }

      if (!tree.end_of_list()) {
        // continue with the next element
        tree.step_forward();
        i = tree.actual_data();
      } else {
        //couldn't find a matching value for a template
        // this can only be allowed in SUBSET matching,
        // otherwise there is no match
        if(match_type == EXACT)
          return FALSE;

        //every template has to match in SUPERSET matching
        if(match_type == SUPERSET)
        {
          //if we are not in permutation matching or don't need to count
          // the number of unmatched templates than exit
          if(number_of_uncovered == NULL)
            return FALSE;
        }

        //if we are SUBSET matching
        // then we have either returned true when we found
        // a new value for the last template (so we can't get to here)
        // or we just have to simply ignore this template
        break;
      }
    }
  }
  //we only reach here if we have found pairs to every template or we
  // are making a SUBSET match
  // (or in SUPERSET we need the number of uncovered)
  //the result depends on the number of pairs found

  int number_of_pairs = table.get_nof_covered();

  if(match_type == SUBSET)
  {
    if(number_of_pairs == value_size)
      return TRUE;
    else
      return FALSE;
  }

  //here EXACT can only be true or we would have return false earlier
  if(match_type == EXACT) return TRUE;

  if(match_type == SUPERSET)
  {
    //we only return FALSE if we need the number of uncovered templates and
    // there really were uncovered templates
    if(number_of_uncovered != NULL && number_of_pairs != real_template_size)
    {
      *number_of_uncovered = real_template_size - number_of_pairs;
      return FALSE;
    }else{
      return TRUE;
    }
  }

  return FALSE;
}


//the real matching function, we don't wan't it to be seen from outside so
// it is not declared in the header, but called by permutation_match

/*
recursive_permutation_match implements a recursive algorithm to match
two arrays of values say A and B, where A can contain permutations and
asterisks.
A permutation is matched if there is a matching part of values in array B
with any order of the values in the permutation.

The algorithm:

The algorithm is recursive so we will give the general working for one
level, which is what all the levels do.

At each level the algorithm is called with some left over of both arrays
and tries to match them.

There are three different ways to go on each level:
    -if A's leftover's first element is the beginning of a permutation
      then a set_of like matching will take place. It is not exactly a set_of
      matching because if there is a asterisk among the permutated elements
      than we can'tknow how long part of B's array will match it.

      So we try to find the smallest possible part of B's array which will be
      a superset of the permutated elements. This is achieved by making set_of
      matchings. When the superset is found, we call the algorithm for
      the leftovers of the two arrays.

      If the leftovers don't match, than we try matching a bigger part of B's
      array with the permutated elements, till we find a size where the
      leftovers match and we can return true, or we reach the maximal size
      the permutation allows as to match with elements of B, in this case
      we must return false.

    -if A's leftover start with an asterisk which does not belong to a
      permutation than it treated like a permutation, whose minimal size of
      matching is 0 elements, and maximal size of matching is the whole
      leftover array of B.

    -else we have to make element-by-element matches.
      if we match till the next asterisk or permutation start then we
      make a recursive call with the elements from there
      else we return false.

There are some speedups:
    -in permutation matching:
        -we estimate how small or large the matching set can be in
          advance.
        -to find the first matching set of B's elements we use the incremental
          version of set matching.
        -after finding a matching set we don't make any more set matches as
          the increased set must still be a superset.
    -if we fail in the element-by-element matching part than we don't return
      with false, but try to find the first element of B which will match with
      the last "unmatched" element of A. We give back this shift size to the
      calling level so it can make a bigger jump forward (skipping the calls
      that have no chance to match).
    -if we in any part of the algorithm find that match can't possibly happen,
      than we return to the calling level with NO_CHANCE. This way we can
      end the algorithm without making those unnecessary checks.
*/
static answer recursive_permutation_match(const Base_Type *value_ptr,
  unsigned int value_start_index,
  unsigned int value_size,
  const Record_Of_Template *template_ptr,
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
          match_function, SUPERSET, &x, pair_list,old_temp_size, legacy);

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

/*
outer function calling the real recursive_permutation_match

if we know that there is no need for the permutation matching
(because there are no permutations in the array, or the whole array
 is just one permutation), than we call appropriate matching function
instead of slower recursive_permutation_match.
*/
boolean match_record_of(const Base_Type *value_ptr, int value_size,
  const Record_Of_Template *template_ptr,
  int template_size, match_function_t match_function, boolean legacy)
{
  if (value_ptr == NULL || value_size < 0 ||
    template_ptr == NULL || template_size < 0 ||
    template_ptr->get_selection() != SPECIFIC_VALUE)
    TTCN_error("Internal error: match_record_of: invalid argument.");

  unsigned int nof_permutations = template_ptr->get_number_of_permutations();
  // use the simplified algorithm if the template does not contain permutation
  if (nof_permutations == 0)
    return match_array(value_ptr, value_size,
      template_ptr, template_size, match_function, legacy);
  // use 'set of' matching if all template elements are grouped into one
  // permutation
  if (nof_permutations == 1 && template_ptr->get_permutation_start(0) == 0 &&
    template_ptr->get_permutation_end(0) ==
      static_cast<unsigned int>(template_size - 1) )
    return match_set_of(value_ptr, value_size, template_ptr, template_size,
      match_function, legacy);

  unsigned int shift_size = 0;
  return recursive_permutation_match(value_ptr, 0, value_size, template_ptr,
    0, template_size, 0, match_function, shift_size, legacy) == SUCCESS;
}

boolean match_set_of(const Base_Type *value_ptr, int value_size,
  const Restricted_Length_Template *template_ptr,
  int template_size, match_function_t match_function, boolean legacy)
{
  if (value_ptr == NULL || value_size < 0 ||
    template_ptr == NULL || template_size < 0)
    TTCN_error("Internal error: match_set_of: invalid argument.");
  type_of_matching match_type = EXACT;
  switch (template_ptr->get_selection()) {
  case SPECIFIC_VALUE:
    match_type = EXACT;
    break;
  case SUPERSET_MATCH:
    match_type = SUPERSET;
    break;
  case SUBSET_MATCH:
    match_type = SUBSET;
    break;
  default:
    TTCN_error("Internal error: match_set_of: invalid matching type.");
  }
  return match_set_of_internal(value_ptr, 0, value_size, template_ptr, 0,
    template_size, match_function, match_type, NULL, NULL, 0, legacy);
}

void log_match_heuristics(const Base_Type *value_ptr, int value_size,
  const Restricted_Length_Template *template_ptr,
  int template_size,
  match_function_t match_function,
  log_function_t log_function, boolean legacy)
{
  if (value_ptr == NULL || value_size < 0 ||
    template_ptr == NULL || template_size < 0 ||
    template_ptr->get_selection() != SPECIFIC_VALUE)
    TTCN_error("Internal error: log_match_heuristics: invalid argument.");

  if (value_size == 0 && template_size == 0) return;

  if (!template_ptr->match_length(value_size)) {
    TTCN_Logger::log_event("Length restriction cannot be satisfied. ");
    return;
  }

  int asterisks_found = 0;
  for (int i = 0; i < template_size; i++)
  {
    // If j == -1, check whether the template element is an asterisk.
    // There is no problem if an asterisk has no matching pair.
    if (match_function(value_ptr, -1, template_ptr, i, legacy))
    {
      asterisks_found++;
    }
  }

  if(value_size < template_size - asterisks_found){
    TTCN_Logger::print_logmatch_buffer();
    if(asterisks_found == 0){
      TTCN_Logger::log_event(" Too few elements in value are present: %d was expected instead of %d", template_size, value_size);
    }else{
      TTCN_Logger::log_event(" Too few value elements are present in value: at least %d was expected instead of %d", template_size-asterisks_found, value_size);
    }
    return;
  }else if(asterisks_found == 0 && value_size > template_size){
    TTCN_Logger::print_logmatch_buffer();
    TTCN_Logger::log_event(" Too many elements are present in value: %d was expected instead of %d", template_size, value_size);
    return;
  }

  if (value_size == 0 || template_size == 0) return;

  if(TTCN_Logger::VERBOSITY_COMPACT != TTCN_Logger::get_matching_verbosity()){
    TTCN_Logger::log_event_str(" Some hints to find the reason of mismatch: ");
    TTCN_Logger::log_event_str("{ value elements that have no pairs in the template: ");
  }

  boolean value_found = FALSE;
  int nof_unmatched_values = 0;
  boolean *unmatched_values = new bool[value_size];
  for (int i = 0; i < value_size; i++)
  {
    boolean pair_found = FALSE;
    for (int j = 0; j < template_size; j++)
    {
      if (match_function(value_ptr, i, template_ptr, j, legacy))
      {
        pair_found = TRUE;
        break;
      }
    }

    unmatched_values[i] = !pair_found;
    if (!pair_found)
    {
      if(TTCN_Logger::VERBOSITY_COMPACT != TTCN_Logger::get_matching_verbosity()){
        if (value_found)
          TTCN_Logger::log_event_str(", ");
        else
          value_found = TRUE;

        log_function(value_ptr, NULL, i, 0, legacy);
        TTCN_Logger::log_event(" at index %d", i);
      }
      nof_unmatched_values++;
    }
  }

  if(TTCN_Logger::VERBOSITY_COMPACT != TTCN_Logger::get_matching_verbosity()){
    if (!value_found) TTCN_Logger::log_event_str("none");
    TTCN_Logger::log_event_str(", template elements that have no pairs in "
      "the value: ");
  }

  boolean template_found = FALSE;
  int nof_unmatched_templates = 0;
  boolean *unmatched_templates = new bool[template_size];
  for (int i = 0; i < template_size; i++)
  {
    boolean pair_found = FALSE;
    // if j == -1 it is checked whether the template element is an
    // asterisk there is no problem if an asterisk has no matching
    // pair
    for (int j = -1; j < value_size; j++)
    {
      if (match_function(value_ptr, j, template_ptr, i, legacy))
      {
        pair_found = TRUE;
        break;
      }
    }
    unmatched_templates[i] = !pair_found;
    if (!pair_found)
    {
      if(TTCN_Logger::VERBOSITY_COMPACT != TTCN_Logger::get_matching_verbosity()){
        if (template_found)
          TTCN_Logger::log_event_str(", ");
        else
          template_found = TRUE;

        log_function(NULL, template_ptr, 0, i, legacy);
        TTCN_Logger::log_event(" at index %d", i);
      }
      nof_unmatched_templates++;
    }
  }

  if(TTCN_Logger::VERBOSITY_COMPACT != TTCN_Logger::get_matching_verbosity()){
    if (!template_found) TTCN_Logger::log_event_str("none");

    TTCN_Logger::log_event_str(", matching value <-> template index pairs: ");
    boolean pair_found = FALSE;
    for (int i = 0; i < value_size; i++)
    {
      for (int j = 0; j < template_size; j++)
      {
        if (match_function(value_ptr, i, template_ptr, j, legacy))
        {
          if (pair_found)
            TTCN_Logger::log_char(',');
          else {
            TTCN_Logger::log_char('{');
            pair_found = TRUE;
          }
          TTCN_Logger::log_event(" %d <-> %d", i, j);
        }
      }
    }
    if (pair_found) TTCN_Logger::log_event_str(" }");
    else TTCN_Logger::log_event_str("none");
  }

  if(nof_unmatched_templates > 0 && nof_unmatched_values > 0){
    if(TTCN_Logger::VERBOSITY_COMPACT == TTCN_Logger::get_matching_verbosity()){
      int previous_size = TTCN_Logger::get_logmatch_buffer_len();
      for (int i = 0; i < value_size; i++)
      {
        if(unmatched_values[i]){
          for (int j = 0; j < template_size; j++)
          {
            if(unmatched_templates[j]){
              TTCN_Logger::log_logmatch_info("[%d <-> %d]", i, j);
              log_function(value_ptr, template_ptr, i, j, legacy);

              TTCN_Logger::set_logmatch_buffer_len(previous_size);
            }
          }
        }
      }
    }else{
      TTCN_Logger::log_event_str(", matching unmatched value <-> template index pairs: ");
      char sep = '{';
      for (int i = 0; i < value_size; i++)
      {
        if(unmatched_values[i]){
          for (int j = 0; j < template_size; j++)
          {
            if(unmatched_templates[j]){
              TTCN_Logger::log_event("%c %d <-> %d:{ ", sep, i, j);
              if('{' == sep){
                sep = ',';
              }
              log_function(value_ptr, template_ptr, i, j, legacy);
              TTCN_Logger::log_event_str(" }");
            }
          }
        }
      }

      TTCN_Logger::log_event_str(" }");
    }
  }
  delete [] unmatched_values;
  delete [] unmatched_templates;
  if(TTCN_Logger::VERBOSITY_COMPACT != TTCN_Logger::get_matching_verbosity())
    TTCN_Logger::log_event_str(" }");
}
