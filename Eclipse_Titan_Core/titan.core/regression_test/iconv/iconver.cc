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
#include <TTCN3.hh>
#include <iconv.h>
#include <errno.h>
#include <vector>

#if defined(WIN32)
#include <cygwin/version.h>
#endif

#if defined(LINUX) || defined(INTERIX) || (CYGWIN_VERSION_DLL_MAJOR >= 1007)
#define ICONV_INPUT_TYPE char*
#else
#define ICONV_INPUT_TYPE const char*
#endif

static const universal_char ucs4_bom = { 0,0,0xFE,0xFF };

namespace converter
{
  // Convert from octetstring in UTF-8 to universal charstring
  INTEGER o2u(const CHARSTRING& encoding, const OCTETSTRING& input, UNIVERSAL_CHARSTRING& output)
  {
    errno = 0;
    iconv_t t = iconv_open("UCS-4BE", (const char*)encoding); // to, from
    if ((iconv_t)-1 == t) {
      perror(encoding);
      return errno;
    }
    size_t inbytesleft = input.lengthof();
    ICONV_INPUT_TYPE inptr = (ICONV_INPUT_TYPE)(const unsigned char *)input;

    // Scott Meyers, Effective STL, Item 16.
    // Don't even think of using Titan's vector like this!
    std::vector<universal_char> outbuf(inbytesleft + 1);
    // +1 for the BOM that Solaris iconv likes to put in at the front
    char* outptr = (char*)&outbuf.front();
    size_t const outbytes =  outbuf.size() * 4; // size is in universal_char
    size_t outbytesleft = outbytes; // will be modified by iconv

    size_t rez = iconv(t, &inptr, &inbytesleft, &outptr, &outbytesleft);
    if (rez == (size_t)-1) {
      perror("*** iconv ***"); // oh dear
    }
    else {
      size_t is_bom = ( ucs4_bom == outbuf.front() );
      output = UNIVERSAL_CHARSTRING((outbytes - outbytesleft - is_bom) / 4, &outbuf[is_bom]);
      errno = 0; // Solaris iconv may set errno to nonzero even after successful conversion :-O
    }
    iconv_close(t);

    return errno;
  }

  // Convert from universal charstring to octetstring in UTF-8
  INTEGER u2o(const CHARSTRING& encoding, const UNIVERSAL_CHARSTRING& input, OCTETSTRING& output)
  {
    errno = 0;
    iconv_t t = iconv_open((const char*)encoding, "UCS-4BE"); // to, from
    if ((iconv_t)-1 == t) {
      perror(encoding);
      return errno;
    }

    size_t inbytesleft = input.lengthof(); // in characters
    size_t to_be = inbytesleft * 6; // max 6 bytes in UTF-8 per character
    inbytesleft *= 4; // count in bytes

    char* const inbuf = (char*)input.operator const universal_char*();
    ICONV_INPUT_TYPE inptr = inbuf; // will be modified by iconv
    std::vector<unsigned char> outbuf(to_be | !to_be);
    // to_be, or not to_be, that is the question.
    // This makes sure that the vector's size is at least 1.
    char* outptr = (char*)&outbuf.front();

    size_t rez = iconv(t, &inptr, &inbytesleft, &outptr, &to_be);
    if (rez == (size_t)-1) {
      perror("*** iconv ***"); // oh dear
    }
    else {
      output = OCTETSTRING(outbuf.size() - (to_be | !to_be), &outbuf.front());
      errno = 0; // Solaris iconv may set errno to nonzero even after successful conversion :-O
    }
    iconv_close(t);

    return errno;
  }

} // namespace

// Drag o2u and u2o out to the global namespace for compatibility with old names.
// Ifdef-ing the namespace away will probably fail when this file is used
// from projects where old naming is permanently disabled.

INTEGER o2u(const CHARSTRING& encoding, const OCTETSTRING& input, UNIVERSAL_CHARSTRING& output) {
  return converter::o2u(encoding, input, output);
}

INTEGER u2o(const CHARSTRING& encoding, const UNIVERSAL_CHARSTRING& input, OCTETSTRING& output) {
  return converter::u2o(encoding, input, output);
}

