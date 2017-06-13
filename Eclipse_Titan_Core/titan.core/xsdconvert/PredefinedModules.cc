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
 *   Szabo, Bence Janos
 *
 ******************************************************************************/
/**
 * Predefined and standardized TTCN-3 modules
 * containing mappings of XSD element from the http://www.w3.org/2001/XMLSchema namespace
 *
 * Modules are generated always when conversion is called
 *
 */
const char * moduleUsefulTtcn3Types = {
  "module UsefulTtcn3Types {\n\n\n"


  "    type integer byte (-128 .. 127) with { variant \"/* 8 bit */\" };\n\n"

  "    type integer unsignedbyte (0 .. 255) with { variant \"/*unsigned 8 bit*/\" };\n\n"

  "    type integer short (-32768 .. 32767) with { variant \"/*16 bit*/\" };\n\n"

  "    type integer unsignedshort (0 .. 65535) with { variant \"/*unsigned 16 bit*/\" };\n\n"

  "    type integer long (-2147483648 .. 2147483647) with { variant \"/*32 bit*/\" };\n\n"

  "    type integer unsignedlong (0 .. 4294967295) with { variant \"/*unsigned 32 bit*/\" };\n\n"

  "    type integer longlong ( -9223372036854775808 .. 9223372036854775807 ) with { variant \"/*64 bit*/\" };\n\n"

  "    type integer unsignedlonglong ( 0 .. 18446744073709551615 ) with { variant \"/*unsigned 64 bit*/\" };\n\n"

  "    type float IEEE754float with { variant \"/*IEEE754 float*/\" };\n\n"

  "    type float IEEE754double with { variant \"/*IEEE754 double*/\" };\n\n"

  "    type float IEEE754extfloat with { variant \"/*IEEE754 extended float*/\" };\n\n"

  "    type float IEEE754extdouble with { variant \"/*IEEE754 extended double*/\" };\n\n"

  "    type universal charstring utf8string with { variant \"/*UTF-8*/\" };\n\n"

  "    type universal charstring bmpstring ( char ( 0,0,0,0 ) .. char ( 0,0,255,255) ) with { variant \"/*UCS-2*/\" };\n\n"

  "    type universal charstring utf16string ( char ( 0,0,0,0 ) .. char ( 0,16,255,255) ) with { variant \"/*UTF-16*/\" };\n\n"

  "    type universal charstring iso8859string ( char ( 0,0,0,0 ) .. char ( 0,0,0,255) ) with { variant \"/*8 bit*/\" };\n\n"

  "    type record IDLfixed\n"
  "    {\n"
  "    	unsignedshort digits,\n"
  "    	short scale,\n"
  "    	charstring value_\n"
  "    }\n"
  "    with {\n"
  "    variant \"/*IDL:fixed FORMAL/01-12-01 v.2.6*/\";\n"
  "    };\n\n"

  "    /*\n"
  "    type charstring char length (1);\n\n"

  "    NOTE 1: The name of this useful type is the same as the TTCN-3 keyword used to denote universal\n"
  "    charstring values in the quadraple form. In general it is disallowed to use TTCN-3 keywords as\n"
  "    identifiers. The \"char\" useful type is a solitary exception and allowed only for backward compatibility\n"
  "    with previous versions of the TTCN-3 standard. (except Titan doesn't)\n\n"

  "    NOTE 2: The special string \"8 bit\" defined in clause 28.2.3 may be used with this type to specify a given encoding\n"
  "    for its values. Also, other properties of the base type can be changed by using attribute mechanisms.\n"
  "    */\n\n"

  "    type universal charstring uchar length (1);\n\n"

  "    /*\n"
  "    NOTE: Special strings defined in clause 28.2.3 except \"8 bit\" may be used with this type to specify a given\n"
  "    encoding for its values. Also, other properties of the base type can be changed by using attribute\n"
  "    mechanisms.\n"
  "    */\n\n"

  "    type bitstring bit length (1);\n\n"

  "    type hexstring hex length (1);\n\n"

  "    type octetstring octet length (1);\n\n"

  "}\n"
  "with {\n"
  "encode \"XML\";\n"
  "}\n"

};

const char * moduleXSD = {

  "module XSD {\n\n"

  "import from UsefulTtcn3Types all;\n\n"

  "//These constants are used in the XSD date/time type definitions\n"
  "const charstring\n"
  "  dash := \"-\",\n"
  "  cln  := \":\",\n"
  "  year := \"[0-9]#4\",\n"
  "  yearExpansion := \"(-([1-9][0-9]#(0,))#(,1))#(,1)\",\n"
  "  month := \"(0[1-9]|1[0-2])\",\n"
  "  dayOfMonth := \"(0[1-9]|[12][0-9]|3[01])\",\n"
  "  hour := \"([01][0-9]|2[0-3])\",\n"
  "  minute := \"([0-5][0-9])\",\n"
  "  second := \"([0-5][0-9])\",\n"
  "  sFraction := \"(.[0-9]#(1,))#(,1)\",\n"
  "  endOfDayExt := \"24:00:00(.0#(1,))#(,1)\",\n"
  "  nums := \"[0-9]#(1,)\",\n"
  "  ZorTimeZoneExt := \"(Z|[+-]((0[0-9]|1[0-3]):[0-5][0-9]|14:00))#(,1)\",\n"
  "  durTime := \"(T[0-9]#(1,)\"&\n"
  "             \"(H([0-9]#(1,)(M([0-9]#(1,)(S|.[0-9]#(1,)S))#(,1)|.[0-9]#(1,)S|S))#(,1)|\"&\n"
  "             \"M([0-9]#(1,)(S|.[0-9]#(1,)S)|.[0-9]#(1,)M)#(,1)|\"&\n"
  "             \"S|\"&\n"
  "             \".[0-9]#(1,)S))\"\n\n"

  "//anySimpleType\n\n"

  "type XMLCompatibleString AnySimpleType\n"
  "with {\n"
  "variant \"XSD:anySimpleType\";\n"
  "};\n\n"

  "//anyType;\n\n"

  "type record AnyType\n"
  "{\n"
  "	record of String embed_values optional,\n"
  "	record of String attr optional,\n"
  "	record of String elem_list\n"
  "}\n"
  "with {\n"
  "variant \"XSD:anyType\";\n"
  "variant \"embedValues\";\n"
  "variant (attr) \"anyAttributes\";\n"
  "variant (elem_list) \"anyElement\";\n"
  "};\n"

  "// String types\n\n"

  "type XMLCompatibleString String\n"
  "with {\n"
  "variant \"XSD:string\";\n"
  "};\n\n"

  "type XMLStringWithNoCRLFHT NormalizedString\n"
  "with {\n"
  "variant \"XSD:normalizedString\";\n"
  "};\n\n"

  "type NormalizedString Token\n"
  "with {\n"
  "variant \"XSD:token\";\n"
  "};\n\n"

  "type XMLStringWithNoWhitespace Name\n"
  "with {\n"
  "variant \"XSD:Name\";\n"
  "};\n\n"

  "type XMLStringWithNoWhitespace NMTOKEN\n"
  "with {\n"
  "variant \"XSD:NMTOKEN\";\n"
  "};\n\n"

  "type Name NCName\n"
  "with {\n"
  "variant \"XSD:NCName\";\n"
  "};\n\n"

  "type NCName ID\n"
  "with {\n"
  "variant \"XSD:ID\";\n"
  "};\n\n"

  "type NCName IDREF\n"
  "with {\n"
  "variant \"XSD:IDREF\";\n"
  "};\n\n"

  "type NCName ENTITY\n"
  "with {\n"
  "variant \"XSD:ENTITY\";\n"
  "};\n\n"

  "type octetstring HexBinary\n"
  "with {\n"
  "variant \"XSD:hexBinary\";\n"
  "};\n\n"

  "type octetstring Base64Binary\n"
  "with {\n"
  "variant \"XSD:base64Binary\";\n"
  "};\n\n"

  "type XMLStringWithNoCRLFHT AnyURI\n"
  "with {\n"
  "variant \"XSD:anyURI\";\n"
  "};\n\n"

  "type charstring Language (pattern \"[a-zA-Z]#(1,8)(-\\w#(1,8))#(0,)\")\n"
  "with {\n"
  "variant \"XSD:language\";\n"
  "};\n"

  "// Integer types\n\n"

  "type integer Integer\n"
  "with {\n"
  "variant \"XSD:integer\";\n"
  "};\n\n"

  "type integer PositiveInteger (1 .. infinity)\n"
  "with {\n"
  "variant \"XSD:positiveInteger\";\n"
  "};\n\n"

  "type integer NonPositiveInteger (-infinity .. 0)\n"
  "with {\n"
  "variant \"XSD:nonPositiveInteger\";\n"
  "};\n\n"

  "type integer NegativeInteger (-infinity .. -1)\n"
  "with {\n"
  "variant \"XSD:negativeInteger\";\n"
  "};\n\n"

  "type integer NonNegativeInteger (0 .. infinity)\n"
  "with {\n"
  "variant \"XSD:nonNegativeInteger\";\n"
  "};\n\n"

  "type longlong Long\n"
  "with {\n"
  "variant \"XSD:long\";\n"
  "};\n\n"

  "type unsignedlonglong UnsignedLong\n"
  "with {\n"
  "variant \"XSD:unsignedLong\";\n"
  "};\n\n"

  "type long Int\n"
  "with {\n"
  "variant \"XSD:int\";\n"
  "};\n\n"

  "type unsignedlong UnsignedInt\n"
  "with {\n"
  "variant \"XSD:unsignedInt\";\n"
  "};\n\n"

  "type short Short\n"
  "with {\n"
  "variant \"XSD:short\";\n"
  "};\n\n"

  "type unsignedshort UnsignedShort\n"
  "with {\n"
  "variant \"XSD:unsignedShort\";\n"
  "};\n\n"

  "type byte Byte\n"
  "with {\n"
  "variant \"XSD:byte\";\n"
  "};\n\n"

  "type unsignedbyte UnsignedByte\n"
  "with {\n"
  "variant \"XSD:unsignedByte\";\n"
  "};\n\n"

  "// Float types\n\n"

  "type float Decimal\n"
  "with {\n"
  "variant \"XSD:decimal\";\n"
  "};\n\n"

  "type IEEE754float Float\n"
  "with {\n"
  "variant \"XSD:float\";\n"
  "};\n\n"

  "type IEEE754double Double\n"
  "with {\n"
  "variant \"XSD:double\";\n"
  "};\n\n"

  "// Time types\n\n"


  "type charstring Duration (pattern\n"
  "  \"{dash}#(,1)P({nums}(Y({nums}(M({nums}D{durTime}#(,1)|{durTime}#(,1))|D{durTime}#(,1))|\" &\n"
  "  \"{durTime}#(,1))|M({nums}D{durTime}#(,1)|{durTime}#(,1))|D{durTime}#(,1))|{durTime})\")\n"
  "with {\n"
  "variant \"XSD:duration\";\n"
  "};\n\n"

  "type charstring DateTime (pattern\n"
  "  \"{yearExpansion}{year}{dash}{month}{dash}{dayOfMonth}T({hour}{cln}{minute}{cln}{second}\" &\n"
  " \"{sFraction}|{endOfDayExt}){ZorTimeZoneExt}\" )\n"
  "with {\n"
  "variant \"XSD:dateTime\";\n"
  "};\n\n"

  "type charstring Time (pattern\n"
  "  \"({hour}{cln}{minute}{cln}{second}{sFraction}|{endOfDayExt}){ZorTimeZoneExt}\" )\n"
  "with {\n"
  "variant \"XSD:time\";\n"
  "};\n\n"

  "type charstring Date (pattern\n"
  "  \"{yearExpansion}{year}{dash}{month}{dash}{dayOfMonth}{ZorTimeZoneExt}\" )\n"
  "with {\n"
  "variant \"XSD:date\";\n"
  "};\n\n"

  "type charstring GYearMonth (pattern\n"
  "  \"{yearExpansion}{year}{dash}{month}{ZorTimeZoneExt}\" )\n"
  "with {\n"
  "variant \"XSD:gYearMonth\";\n"
  "};\n\n"

  "type charstring GYear (pattern\n"
  "  \"{yearExpansion}{year}{ZorTimeZoneExt}\" )\n"
  "with {\n"
  "variant \"XSD:gYear\";\n"
  "};\n\n"

  "type charstring GMonthDay (pattern\n"
  " \"{dash}{dash}{month}{dash}{dayOfMonth}{ZorTimeZoneExt}\" )\n"
  "with {\n"
  "variant \"XSD:gMonthDay\";\n"
  "};\n\n"

  "type charstring GDay (pattern\n"
  "  \"{dash}{dash}{dash}{dayOfMonth}{ZorTimeZoneExt}\" )\n"
  "with {\n"
  "variant \"XSD:gDay\";\n"
  "};\n\n"

  "type charstring GMonth (pattern\n"
  "  \"{dash}{dash}{month}{ZorTimeZoneExt}\" )\n"
  "with {\n"
  "variant \"XSD:gMonth\";\n"
  "};\n\n"

  "// Sequence types\n\n"

  "type record of NMTOKEN NMTOKENS\n"
  "with {\n"
  "variant \"XSD:NMTOKENS\";\n"
  "};\n\n"

  "type record of IDREF IDREFS\n"
  "with {\n"
  "variant \"XSD:IDREFS\";\n"
  "};\n\n"

  "type record of ENTITY ENTITIES\n"
  "with {\n"
  "variant \"XSD:ENTITIES\";\n"
  "};\n\n"

  "type record QName\n"
  "{\n"
  "	AnyURI uri  optional,\n"
  "	NCName name\n"
  "}\n"
  "with {\n"
  "variant \"XSD:QName\";\n"
  "};\n\n"

  "// Boolean type\n\n"

  "type boolean Boolean\n"
  "with {\n"
  "variant \"XSD:boolean\";\n"
  "};\n\n"

  "//TTCN-3 type definitions supporting the mapping of W3C XML Schema built-in datatypes\n\n"

  "type utf8string XMLCompatibleString\n"
  "(\n"
  "	char(0,0,0,9)..char(0,0,0,9),\n"
  "	char(0,0,0,10)..char(0,0,0,10),\n"
  "	char(0,0,0,13)..char(0,0,0,13),\n"
  "  	char(0,0,0,32)..char(0,0,215,255),\n"
  "  	char(0,0,224,0)..char(0,0,255,253),\n"
  "  	char(0,1,0,0)..char(0,16,255,253)\n"
  ")\n\n"

  "type utf8string XMLStringWithNoWhitespace\n"
  "(\n"
  "	char(0,0,0,33)..char(0,0,215,255),\n"
  "  	char(0,0,224,0)..char(0,0,255,253),\n"
  "  	char(0,1,0,0)..char(0,16,255,253)\n"
  ")\n\n"

  "type utf8string XMLStringWithNoCRLFHT\n"
  "(\n"
  "	char(0,0,0,32)..char(0,0,215,255),\n"
  " 	char(0,0,224,0)..char(0,0,255,253),\n"
  "  	char(0,1,0,0)..char(0,16,255,253)\n"
  ")\n\n"

  "}\n"
  "with{\n"
  "encode \"XML\"\n"
  "}\n"

};
