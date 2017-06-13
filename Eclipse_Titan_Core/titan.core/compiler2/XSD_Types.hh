/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
#ifndef XSD_TYPES_HH_
#define XSD_TYPES_HH_

/**
 * XSD type variants. For example: XSD:integer XSD:binary etc.
 * A field can only have one of these variants.
 * This enum should be in sync with the XSD_types enum in XER.hh
 */
typedef enum {
  XSD_NONE = 0, // XER_NONE should be zero
  XSD_ANYSIMPLETYPE, // Unused
  XSD_ANYTYPE, // Unused
  XSD_STRING,
  XSD_NORMALIZEDSTRING,
  XSD_TOKEN,
  XSD_NAME,
  XSD_NMTOKEN,
  XSD_NCName,
  XSD_ID,
  XSD_IDREF,
  XSD_ENTITY,
  XSD_HEXBINARY,
  XSD_BASE64BINARY,
  XSD_ANYURI,
  XSD_LANGUAGE,
  XSD_INTEGER,
  XSD_POSITIVEINTEGER,
  XSD_NONPOSITIVEINTEGER,
  XSD_NEGATIVEINTEGER,
  XSD_NONNEGATIVEINTEGER,
  XSD_LONG,
  XSD_UNSIGNEDLONG,
  XSD_INT,
  XSD_UNSIGNEDINT,
  XSD_SHORT,
  XSD_UNSIGNEDSHORT,
  XSD_BYTE,
  XSD_UNSIGNEDBYTE,
  XSD_DECIMAL,
  XSD_FLOAT,
  XSD_DOUBLE,
  XSD_DURATION,
  XSD_DATETIME,
  XSD_TIME,
  XSD_DATE,
  XSD_GYEARMONTH,
  XSD_GYEAR,
  XSD_GMONTHDAY,
  XSD_GDAY,
  XSD_GMONTH,
  XSD_NMTOKENS, // Unused
  XSD_IDREFS, // Unused
  XSD_ENTITIES, // Unused
  XSD_QNAME, // Unused
  XSD_BOOLEAN
} XSD_types;

#ifdef __cplusplus
extern "C" {
#endif
  
const char * XSD_type_to_string(const XSD_types xsd_type);

const char * XSD_type_to_xml_type(const XSD_types xsd_type);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*XSD_TYPES_HH*/
