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
 *   Raduly, Csaba
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#ifndef _Ttcn_Quadruple_HH
#define _Ttcn_Quadruple_HH

class QuadInterval;

/** \class Quad
 *  \brief Represents a quadruple.
 *
 *  The stored value can be accessed as an integer or as fields of the
 *  quadruple. The aim is to help the generation of the posix regular expression
 *  from TTCN-3 pattern of type universal charstring. This class can create the
 *  string representation needed by pattern matching functions.
 *
 *  <b>The string representation:</b>
 *  Every field of the quadruple is converted to its hexadecimal representation
 *  but the regular 0..F interval is substituted with the continuous A..P
 *  interval. One quadruple is represented with 8 characters, each from A-P.
 */
class Quad {

  union {
    unsigned int value;
#if defined(__sparc__) || defined(__sparc)
    struct {
      unsigned char group; /* big endian, MSB */
      unsigned char plane;
      unsigned char row;
      unsigned char cell;
    } comp;
#else
    struct {
      unsigned char cell; /* little endian, LSB */
      unsigned char row;
      unsigned char plane;
      unsigned char group;
    } comp;
#endif
  } u;

public:

  /**
   *  Default constructor initializes the quadruple to \q{0,0,0,0}
   */
  Quad();
  Quad(unsigned int value);
  Quad(unsigned char group, unsigned char plane, unsigned char row,
    unsigned char cell);
  Quad(const Quad& rhs);

  unsigned int get_value() const;

  void set(unsigned char group, unsigned char plane, unsigned char row,
    unsigned char cell);

  /**
   * Sets the given field of the quadruple.
   * @param field 0 - group ... 3 - cell.
   * @param c Value to set.
   */
  void set(int field, unsigned char c);
  
  void set_hexrepr(const char* hex_repr);

  const Quad operator-(const Quad& rhs) const;
  const Quad& operator=(const Quad& rhs);
  bool operator==(unsigned int i) const;
  bool operator==(const Quad& rhs) const;
  bool operator<=(const Quad& rhs) const;
  bool operator>=(const Quad& rhs) const;

  bool operator<(const Quad& rhs) const;
  bool operator<(const QuadInterval& rhs) const;

  /**
   * Getter function for easy access to fields.
   * @param i Field selector: 0 - group ... 3 - cell
   * @return Value of the field.
   */
  unsigned char operator[](int i) const;

  /**
   * Returns the hex representation of this quadruple. [A-P]{8}
   * The string must be deallocated by the caller with Free()
   */
  char* get_hexrepr() const;

  /**
   * Returns the hex representation of \p value. [A-P]{8}
   * The string must be deallocated by the caller with Free()
   */
  static char* get_hexrepr(unsigned int value);

  /** Fills \p str with the hex representation.
   *  str[0] gets the upper half of the most significant byte (group)
   *  str[7] gets the lower half of the least significant byte (cell)
   */
  static void  get_hexrepr(const Quad& q, char* const str);

  /**
   * Returns the hex representation of one field of the quadruple. [A-P]{2}
   * The string must be deallocated by the caller with Free()
   */
  static char* char_hexrepr(unsigned char c);
};

/** \class QuadInterval
 *  \brief Represents an interval in a regular expression set.
 *
 *  The interval is given with its lower and upper boundary. Boundaries are
 *  given as pointers to quadruples: \a lower and \a upper.
 *
 *  To help creating more optimal regular expressions some basic operation is
 *  implemented: \a contains, \a has_intersection, \a join.
 */
class QuadInterval {
  Quad lower;
  Quad upper;

  friend class Quad;

public:
  QuadInterval(Quad p_lower, Quad p_upper);

  QuadInterval(const QuadInterval& rhs);

  const Quad& get_lower() const;
  const Quad& get_upper() const;

  bool contains(const Quad& rhs) const;
  bool contains(const QuadInterval& rhs) const;
  bool has_intersection(const QuadInterval& rhs) const;
  void join(const QuadInterval& rhs);

  /**
   * operator<()
   * @param rhs A quadruple.
   * @return true if value of \a rhs is larger than value of the upper boundary.
   */
  bool operator<(const Quad& rhs) const;
  /**
   * operator<()
   * @param rhs A QuadInterval.
   * @return true if \a rhs does not intersect with \a this and lower boundary
   * of \a rhs is larger than upper boundary of \a this.
   */
  bool operator<(const QuadInterval& rhs) const;

  unsigned int width() const;

  /**
   * Generate a posix regular expression for the interval.
   * @return pointer to the string which should be freed by the caller.
   */
  char* generate_posix();

private:
  char* generate_hex_interval(unsigned char source, unsigned char dest);
};

/** \class QuadSet
 *  \brief Represents a set in a regular expression.
 *
 *  Stores the quadruples and the intervals in the set and creates a posix
 *  regular expression.
 */
class QuadSet {

  enum elemtype_t {
    QSET_QUAD,
    QSET_INTERVAL
  };

  /**
   * Linked list storing the quadruples and intervals. This list is kept sorted
   * to ease the POSIX regular expression generation in case of negated sets.
   */
  typedef struct _quadset_node_t {
    union {
      Quad* p_quad;
      QuadInterval* p_interval;
    } u;
    _quadset_node_t* next;
    elemtype_t etype;
  } quadset_node_t;

  quadset_node_t* set;
  bool negate;

  /// Copy constructor disabled
  QuadSet(const QuadSet&);
  /// Assignment disabled
  QuadSet& operator=(const QuadSet&);
public:
  QuadSet();

  bool add(Quad* p_quad);
  void add(QuadInterval* interval);

  void join(QuadSet* rhs);

  void set_negate(bool neg);

  bool has_quad(const Quad& q) const;
  bool is_empty() const;

  /**
   * Generate a posix regular expression for the set.
   * @return pointer to the string which should be freed by the caller.
   */
  char* generate_posix();

  ~QuadSet();

private:
  void clean(quadset_node_t* start);

  /**
   * Generate the disjoint set of \a this.
   */
  void do_negate();

  void add_negate_interval(const Quad& q1, const Quad& q2);
  void join_if_possible(quadset_node_t* start);
};

#endif
