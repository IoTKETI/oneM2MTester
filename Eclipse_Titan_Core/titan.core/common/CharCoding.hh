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
#ifndef CHARCODING_HH
#define	CHARCODING_HH

/** Character Coding types.
  * Needed by both the compiler and runtime */

namespace CharCoding {
  
  enum CharCodingType { 
    UNKNOWN,
    ASCII,
    UTF_8,
    UTF16,
    UTF16BE,
    UTF16LE,
    UTF32,
    UTF32BE,
    UTF32LE
  };
  
}

#endif	/* CHARCODING_HH */

