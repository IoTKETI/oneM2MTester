/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Raduly, Csaba
 *
 ******************************************************************************/
#ifndef LOGOPTIONS_HH
#define LOGOPTIONS_HH

#include "Logger.hh"
#include "memory.h"

/** @brief Collection of logging severities to log.

Each log severity which had a corresponding bit in an integer,
now has a boolean in an array.

@note This class must be a POD type. Because it will be a member of a union,
it must not have a constructor or a copy assignment operator.
The compiler-generated copy constructor and assignment
are fine for this class.

\b Warning! This class has no default constructor. An expression like @code
Logging_Bits x; @endcode will result in \c x being \b uninitialised.
To avoid surprises, initialise Logging_Bits objects as an aggregate: @code
Logging_Bits d = { 0,0,........,0 }; @endcode or copy-initialise from
one of the predefined constants e.g. @code
Logging_Bits d = Logging_Bits::log_nothing; @endcode or call Logging_Bits::clear()
immediately after constructing.

*/
struct Logging_Bits
{
  boolean bits[TTCN_Logger::NUMBER_OF_LOGSEVERITIES];

  static const Logging_Bits log_nothing;
  static const Logging_Bits log_all;
  static const Logging_Bits log_everything;
  static const Logging_Bits default_console_mask;

  boolean operator ==( const Logging_Bits& other ) const;
  boolean operator !=( const Logging_Bits& other ) const
  {
    return ! operator==( other );
  }

  /** @brief Sets all bits to false.

  @post *this == Logging_Bits::log_nothing
  */
  void clear();

  /** @brief Sets one bit corresponding to a TTCN_Logger::Severity.


  @pre \p sev >= 0
  @pre \p sev < TTCN_Logger::NUMBER_OF_LOGSEVERITIES
  @param sev log severity
  @post  bits[sev] is true

  @note All other bits remain unchanged. To have set \b only the specified bit,
  call clear() first.
  */
  void add_sev ( TTCN_Logger::Severity sev );

  /** @brief Merge two Logging_Bits objects

  Bits which are set in \p sev will become set in \p *this. \n
  Bits which were already set in \p *this remain unchanged. \n
  Bits which weren't set in either \p *this or \p other remain unset.

  @param other Logging_Bits
  */
  void merge   ( const Logging_Bits & other );

  /** @brief Return a string corresponding to the bits set in this object.

  @return an \c expstring_t containing a textual description.
  The caller is responsible for calling Free().
  The returned string is suitable for use in the Titan config file.

  */
  expstring_t describe() const;

private:
};

#endif
