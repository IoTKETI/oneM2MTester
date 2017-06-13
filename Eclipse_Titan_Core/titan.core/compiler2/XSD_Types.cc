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
#include "XSD_Types.hh"
#include <cstddef>
#include "error.h"

// Used in the code generation of the xer descriptor, and the union generation
const char * XSD_type_to_string(const XSD_types xsd_type) {
  switch (xsd_type) {
    case XSD_NONE:
      return "XSD_NONE";
    case XSD_ANYSIMPLETYPE:
      return "XSD_ANYSIMPLETYPE";
    case XSD_ANYTYPE:
      return "XSD_ANYTYPE";
    case XSD_STRING:
      return "XSD_STRING";
    case XSD_NORMALIZEDSTRING:
      return "XSD_NORMALIZEDSTRING";
    case XSD_TOKEN:
      return "XSD_TOKEN";
    case XSD_NAME:
      return "XSD_NAME";
    case XSD_NMTOKEN:
      return "XSD_NMTOKEN";
    case XSD_NCName:
      return "XSD_NCName";
    case XSD_ID:
      return "XSD_ID";
    case XSD_IDREF:
      return "XSD_IDREF";
    case XSD_ENTITY:
      return "XSD_ENTITY";
    case XSD_HEXBINARY:
      return "XSD_HEXBINARY";
    case XSD_BASE64BINARY:
      return "XSD_BASE64BINARY";
    case XSD_ANYURI:
      return "XSD_ANYURI";
    case XSD_LANGUAGE:
      return "XSD_LANGUAGE";
    case XSD_INTEGER:
      return "XSD_INTEGER";
    case XSD_POSITIVEINTEGER:
      return "XSD_POSITIVEINTEGER";
    case XSD_NONPOSITIVEINTEGER:
      return "XSD_NONPOSITIVEINTEGER";
    case XSD_NEGATIVEINTEGER:
      return "XSD_NEGATIVEINTEGER";
    case XSD_NONNEGATIVEINTEGER:
      return "XSD_NONNEGATIVEINTEGER";
    case XSD_LONG:
      return "XSD_LONG";
    case XSD_UNSIGNEDLONG:
      return "XSD_UNSIGNEDLONG";
    case XSD_INT:
      return "XSD_INT";
    case XSD_UNSIGNEDINT:
      return "XSD_UNSIGNEDINT";
    case XSD_SHORT:
      return "XSD_SHORT";
    case XSD_UNSIGNEDSHORT:
      return "XSD_UNSIGNEDSHORT";
    case XSD_BYTE:
      return "XSD_BYTE";
    case XSD_UNSIGNEDBYTE:
      return "XSD_UNSIGNEDBYTE";
    case XSD_DECIMAL:
      return "XSD_DECIMAL";
    case XSD_FLOAT:
      return "XSD_FLOAT";
    case XSD_DOUBLE:
      return "XSD_DOUBLE";
    case XSD_DURATION:
      return "XSD_DURATION";
    case XSD_DATETIME:
      return "XSD_DATETIME";
    case XSD_TIME:
      return "XSD_TIME";
    case XSD_DATE:
      return "XSD_DATE";
    case XSD_GYEARMONTH:
      return "XSD_GYEARMONTH";
    case XSD_GYEAR:
      return "XSD_GYEAR";
    case XSD_GMONTHDAY:
      return "XSD_GMONTHDAY";
    case XSD_GDAY:
      return "XSD_GDAY";
    case XSD_GMONTH:
      return "XSD_GMONTH";
    case XSD_NMTOKENS:
      return "XSD_NMTOKENS";
    case XSD_IDREFS:
      return "XSD_IDREFS";
    case XSD_ENTITIES:
      return "XSD_ENTITIES";
    case XSD_QNAME:
      return "XSD_QNAME";
    case XSD_BOOLEAN:
      return "XSD_BOOLEAN";
    default:
      FATAL_ERROR("XSD_Types::XSD_type_to_string - invalid XSD type");
      return NULL;
  }
}

// Used in the union XER encoder and decoder code generation
const char * XSD_type_to_xml_type(const XSD_types xsd_type) {
  switch (xsd_type) {
    case XSD_NONE:
      return "";
    case XSD_ANYSIMPLETYPE:
      return "anySimpleType";
    case XSD_ANYTYPE:
      return "anyType";
    case XSD_STRING:
      return "string";
    case XSD_NORMALIZEDSTRING:
      return "normalizedString";
    case XSD_TOKEN:
      return "token";
    case XSD_NAME:
      return "Name";
    case XSD_NMTOKEN:
      return "NMTOKEN";
    case XSD_NCName:
      return "NCName";
    case XSD_ID:
      return "ID";
    case XSD_IDREF:
      return "IDREF";
    case XSD_ENTITY:
      return "ENTITY";
    case XSD_HEXBINARY:
      return "hexBinary";
    case XSD_BASE64BINARY:
      return "base64Binary";
    case XSD_ANYURI:
      return "anyURI";
    case XSD_LANGUAGE:
      return "language";
    case XSD_INTEGER:
      return "integer";
    case XSD_POSITIVEINTEGER:
      return "positiveInteger";
    case XSD_NONPOSITIVEINTEGER:
      return "nonPositiveInteger";
    case XSD_NEGATIVEINTEGER:
      return "negativeInteger";
    case XSD_NONNEGATIVEINTEGER:
      return "nonNegativeInteger";
    case XSD_LONG:
      return "long";
    case XSD_UNSIGNEDLONG:
      return "unsignedLong";
    case XSD_INT:
      return "int";
    case XSD_UNSIGNEDINT:
      return "unsignedInt";
    case XSD_SHORT:
      return "short";
    case XSD_UNSIGNEDSHORT:
      return "unsignedShort";
    case XSD_BYTE:
      return "byte";
    case XSD_UNSIGNEDBYTE:
      return "unsignedByte";
    case XSD_DECIMAL:
      return "decimal";
    case XSD_FLOAT:
      return "float";
    case XSD_DOUBLE:
      return "double";
    case XSD_DURATION:
      return "duration";
    case XSD_DATETIME:
      return "dateTime";
    case XSD_TIME:
      return "time";
    case XSD_DATE:
      return "date";
    case XSD_GYEARMONTH:
      return "gYearMonth";
    case XSD_GYEAR:
      return "gYear";
    case XSD_GMONTHDAY:
      return "gMonthDay";
    case XSD_GDAY:
      return "gDay";
    case XSD_GMONTH:
      return "gMonth";
    case XSD_NMTOKENS:
      return "NMTOKENS";
    case XSD_IDREFS:
      return "IDREFS";
    case XSD_ENTITIES:
      return "ENTITIES";
    case XSD_QNAME:
      return "QName";
    case XSD_BOOLEAN:
      return "boolean";
    default:
      FATAL_ERROR("XSD_Types::XSD_type_to_xml_type - invalid XSD type");
      return NULL;
  }
}
