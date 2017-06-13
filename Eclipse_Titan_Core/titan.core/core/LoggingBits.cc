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
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
#include "LoggingBits.hh"
#include <stdio.h>
#include <string.h>
#include <assert.h>


void Logging_Bits::clear()
{
  memset( bits, 0, sizeof(bits) );
}


void Logging_Bits::merge( const Logging_Bits & other )
{
  for( size_t i = 0; i < TTCN_Logger::NUMBER_OF_LOGSEVERITIES; ++i )
  {
    bits[i] = bits[i] || other.bits[i];
  }
}

void Logging_Bits::add_sev( TTCN_Logger::Severity sev )
{
  assert(sev >= 0);
  assert(sev < TTCN_Logger::NUMBER_OF_LOGSEVERITIES);
  // Note that the assert and the comparison below are different.
  // 0==LOG_NOTHING which is valid but it means nothing to do.
  if (sev > 0 && sev < TTCN_Logger::NUMBER_OF_LOGSEVERITIES) {
    bits[sev] = TRUE;
  }
}


boolean Logging_Bits::operator==( const Logging_Bits& other ) const
{
  return memcmp( bits, other.bits, sizeof(bits) ) == 0;
}



expstring_t Logging_Bits::describe() const
{
  expstring_t result = memptystr(); // not NULL
  size_t categ = 1; // skip LOG_NOTHING

  // First check whether the bits that make up LOG_ALL are all set
  // (by comparing with log_all, which has those bits set).
  // Remember to skip +1 for LOG_NOTHING
  if( memcmp(bits+1, log_all.bits+1, TTCN_Logger::WARNING_UNQUALIFIED) == 0 )
  {
    result = mputstr(result, "LOG_ALL");
    categ = TTCN_Logger::number_of_categories - 2; // only MATCHING and DEBUG left
  }
  
  for( ; categ < TTCN_Logger::number_of_categories; ++categ ) {
    // sev_categories[categ-1] is the non-inclusive lower end
    // sev_categories[categ  ] is the inclusive upper end
    // these two form a half-closed range (the opposite of what the STL uses)
    
    size_t low_inc = TTCN_Logger::sev_categories[categ-1] + 1;
    size_t high_inc= TTCN_Logger::sev_categories[categ];
    
    const boolean * first = bits + low_inc;
    size_t length= high_inc - low_inc + 1;
    assert(length < TTCN_Logger::NUMBER_OF_LOGSEVERITIES-1);
    // Comparing a segment of our bits with the "all 1s" of log_everything
    if( memcmp(first, log_everything.bits+1, length) == 0 ) {
      // all bits for this main severity are on
      if( result[0] != '\0' ) {
        // string not empty, append separator
        result = mputstr(result, " | ");
      }
      // append main severity name
      result = mputstr(result, TTCN_Logger::severity_category_names[categ]);
    }
    else {
      // not all bits are on, have to append them one by one
      for( size_t subcat = low_inc; subcat <= high_inc; ++subcat ) {
        if( bits[subcat] ) {
          if( result[0] != '\0' ) {
            // string not empty, append separator
            result = mputstr(result, " | ");
          }
          result = mputstr(result, TTCN_Logger::severity_category_names[categ]);
          result = mputc  (result, '_');
          result = mputstr(result, TTCN_Logger::severity_subcategory_names[subcat]);
        }
      } // next subcat
    }
  } // next categ
  
  if( result[0] == '\0' ) {
    result = mputstr(result, "LOG_NOTHING");
  }
  return result;
}

/********************** "Well-known" values **********************/

const Logging_Bits Logging_Bits::log_nothing = { {0} };

const Logging_Bits Logging_Bits::log_all = {
  { 0, // NOTHING
    1, // ACTION
    1,1,1,1, // DEFAULTOP
    1, // ERROR
    1,1,1,1,1,1, // EXECUTOR
    1,1, // FUNCTION
    1,1,1,1, // PARALLEL
    1,1,1, // TESTCASE
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // PORTEVENT
    1,1, // STATISTICS
    1,1,1,1,1,1, // TIMEROP
    1, // USER
    1,1,1,1, // VERDICTOP
    1, // WARNING
    0,0,0,0,0,0,0,0,0,0,0,0, // MATCHING
    0,0,0 // DEBUG
  }
};

const Logging_Bits Logging_Bits::log_everything = {
  { 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 }
};

// TTCN_ERROR | TTCN_WARNING | TTCN_ACTION | TTCN_TESTCASE | TTCN_STATISTICS
const Logging_Bits Logging_Bits::default_console_mask = {
  { 0, // NOTHING
    1, // ACTION
    0,0,0,0, // DEFAULTOP
    1, // ERROR
    0,0,0,0,0,0, // EXECUTOR
    0,0, // FUNCTION
    0,0,0,0, // PARALLEL
    1,1,1, // TESTCASE
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // PORTEVENT
    1,1, // STATISTICS
    0,0,0,0,0,0, // TIMEROP
    0, // USER
    0,0,0,0, // VERDICTOP
    1, // WARNING
    0,0,0,0,0,0,0,0,0,0,0,0, // MATCHING
    0,0,0 // DEBUG
  }
};
