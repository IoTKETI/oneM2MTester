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
 *   Kovacs, Ferenc
 *   Raduly, Csaba
 *   Szabo, Janos Zoltan – initial implementation
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
#ifndef ADDFUNC_HH
#define ADDFUNC_HH

#include "Types.h"

class INTEGER;
class FLOAT;
class BITSTRING;
class BITSTRING_ELEMENT;
class HEXSTRING;
class HEXSTRING_ELEMENT;
class OCTETSTRING;
class OCTETSTRING_ELEMENT;
class CHARSTRING;
class CHARSTRING_ELEMENT;
class UNIVERSAL_CHARSTRING;
class UNIVERSAL_CHARSTRING_ELEMENT;
class BITSTRING_template;
class HEXSTRING_template;
class OCTETSTRING_template;
class CHARSTRING_template;
class UNIVERSAL_CHARSTRING_template;

// Helper functions

extern char hexdigit_to_char(unsigned char hexdigit);

// Additional predefined functions defined in Annex C of ES 101 873-1

// C.1 - int2char
extern CHARSTRING int2char(int value);
extern CHARSTRING int2char(const INTEGER& value);

// C.2 - int2unichar
extern UNIVERSAL_CHARSTRING int2unichar(int value);
extern UNIVERSAL_CHARSTRING int2unichar(const INTEGER& value);

// C.3 - int2bit
extern BITSTRING int2bit(int value, int length);
extern BITSTRING int2bit(int value, const INTEGER& length);
extern BITSTRING int2bit(const INTEGER& value, int length);
extern BITSTRING int2bit(const INTEGER& value, const INTEGER& length);

// C.4 - int2hex
extern HEXSTRING int2hex(int value, int length);
extern HEXSTRING int2hex(int value, const INTEGER& length);
extern HEXSTRING int2hex(const INTEGER& value, int length);
extern HEXSTRING int2hex(const INTEGER& value, const INTEGER& length);

// C.5 - int2oct
extern OCTETSTRING int2oct(int value, int length);
extern OCTETSTRING int2oct(int value, const INTEGER& length);
extern OCTETSTRING int2oct(const INTEGER& value, int length);
extern OCTETSTRING int2oct(const INTEGER& value, const INTEGER& length);

// C.6 - int2str
extern CHARSTRING int2str(int value);
extern CHARSTRING int2str(const INTEGER& value);

// C.7 - int2float
extern double int2float(int value);
extern double int2float(const INTEGER& value);

// C.8 - float2int
extern INTEGER float2int(double value);
extern INTEGER float2int(const FLOAT& value);

// C.9 - char2int
extern int char2int(char value);
extern int char2int(const char *value);
extern int char2int(const CHARSTRING& value);
extern int char2int(const CHARSTRING_ELEMENT& value);

// C.10 - char2oct
extern OCTETSTRING char2oct(const char *value);
extern OCTETSTRING char2oct(const CHARSTRING& value);
extern OCTETSTRING char2oct(const CHARSTRING_ELEMENT& value);

// C.11 - unichar2int
extern int unichar2int(const universal_char& value);
extern int unichar2int(const UNIVERSAL_CHARSTRING& value);
extern int unichar2int(const UNIVERSAL_CHARSTRING_ELEMENT& value);

// C.12 - bit2int
extern INTEGER bit2int(const BITSTRING& value);
extern INTEGER bit2int(const BITSTRING_ELEMENT& value);

// C.13 - bit2hex
extern HEXSTRING bit2hex(const BITSTRING& value);
extern HEXSTRING bit2hex(const BITSTRING_ELEMENT& value);

// C.14 - bit2oct
extern OCTETSTRING bit2oct(const BITSTRING& value);
extern OCTETSTRING bit2oct(const BITSTRING_ELEMENT& value);

// C.15 - bit2str
extern CHARSTRING bit2str(const BITSTRING& value);
extern CHARSTRING bit2str(const BITSTRING_ELEMENT& value);

// C.16 - hex2int
extern INTEGER hex2int(const HEXSTRING& value);
extern INTEGER hex2int(const HEXSTRING_ELEMENT& value);

// C.17 - hex2bit
extern BITSTRING hex2bit(const HEXSTRING& value);
extern BITSTRING hex2bit(const HEXSTRING_ELEMENT& value);

// C.18 - hex2oct
extern OCTETSTRING hex2oct(const HEXSTRING& value);
extern OCTETSTRING hex2oct(const HEXSTRING_ELEMENT& value);

// C.19 - hex2str
extern CHARSTRING hex2str(const HEXSTRING& value);
extern CHARSTRING hex2str(const HEXSTRING_ELEMENT& value);

// C.20 - oct2int
extern INTEGER oct2int(const OCTETSTRING& value);
extern INTEGER oct2int(const OCTETSTRING_ELEMENT& value);

// C.21 - oct2bit
extern BITSTRING oct2bit(const OCTETSTRING& value);
extern BITSTRING oct2bit(const OCTETSTRING_ELEMENT& value);

// C.22 - oct2hex
extern HEXSTRING oct2hex(const OCTETSTRING& value);
extern HEXSTRING oct2hex(const OCTETSTRING_ELEMENT& value);

// C.23 - oct2str
extern CHARSTRING oct2str(const OCTETSTRING& value);
extern CHARSTRING oct2str(const OCTETSTRING_ELEMENT& value);

// C.24 - oct2char
extern CHARSTRING oct2char(const OCTETSTRING& value);
extern CHARSTRING oct2char(const OCTETSTRING_ELEMENT& value);

// C.25 - str2int
extern INTEGER str2int(const char *value);
extern INTEGER str2int(const CHARSTRING& value);
extern INTEGER str2int(const CHARSTRING_ELEMENT& value);

// C.26 - str2oct
extern OCTETSTRING str2oct(const char *value);
extern OCTETSTRING str2oct(const CHARSTRING& value);

// C.27 - str2float
extern double str2float(const char *value);
extern double str2float(const CHARSTRING& value);

// C.28 - lengthof: built-in

// C.29 - sizeof: built-in

// C.31 - ispresent: built-in

// C.32 - ischosen: built-in

// C.33 - regexp
extern CHARSTRING regexp(const CHARSTRING& instr, const CHARSTRING& expression,
    int groupno, boolean nocase);
extern CHARSTRING regexp(const CHARSTRING& instr, const CHARSTRING& expression,
    const INTEGER& groupno, boolean nocase);
extern UNIVERSAL_CHARSTRING regexp(const UNIVERSAL_CHARSTRING& instr,
    const UNIVERSAL_CHARSTRING* expression_val,
    const UNIVERSAL_CHARSTRING_template* expression_tmpl,
    int groupno, boolean nocase);
extern UNIVERSAL_CHARSTRING regexp(const UNIVERSAL_CHARSTRING& instr,
    const UNIVERSAL_CHARSTRING& expression, int groupno, boolean nocase);
extern UNIVERSAL_CHARSTRING regexp(const UNIVERSAL_CHARSTRING& instr,
    const UNIVERSAL_CHARSTRING& expression, const INTEGER& groupno,
    boolean nocase);
// regexp on templates
extern CHARSTRING regexp(const CHARSTRING_template& instr,
    const CHARSTRING_template& expression, int groupno, boolean nocase);
extern CHARSTRING regexp(const CHARSTRING_template& instr,
    const CHARSTRING_template& expression, const INTEGER& groupno,
    boolean nocase);
extern UNIVERSAL_CHARSTRING regexp(const UNIVERSAL_CHARSTRING_template& instr,
    const UNIVERSAL_CHARSTRING_template& expression, int groupno,
    boolean nocase);
extern UNIVERSAL_CHARSTRING regexp(const UNIVERSAL_CHARSTRING_template& instr,
    const UNIVERSAL_CHARSTRING_template& expression, const INTEGER& groupno,
    boolean nocase);

// C.34 - substr
extern void check_substr_arguments(int value_length, int index,
    int returncount, const char *string_type, const char *element_name);
extern BITSTRING substr(const BITSTRING& value, int index, int returncount);
extern BITSTRING substr(const BITSTRING& value, int index,
    const INTEGER& returncount);
extern BITSTRING substr(const BITSTRING& value, const INTEGER& index,
    int returncount);
extern BITSTRING substr(const BITSTRING& value, const INTEGER& index,
    const INTEGER& returncount);
extern BITSTRING substr(const BITSTRING_ELEMENT& value, int index,
    int returncount);
extern BITSTRING substr(const BITSTRING_ELEMENT& value, int index,
    const INTEGER& returncount);
extern BITSTRING substr(const BITSTRING_ELEMENT& value, const INTEGER& index,
    int returncount);
extern BITSTRING substr(const BITSTRING_ELEMENT& value, const INTEGER& index,
    const INTEGER& returncount);

extern HEXSTRING substr(const HEXSTRING& value, int index, int returncount);
extern HEXSTRING substr(const HEXSTRING& value, int index,
    const INTEGER& returncount);
extern HEXSTRING substr(const HEXSTRING& value, const INTEGER& index,
    int returncount);
extern HEXSTRING substr(const HEXSTRING& value, const INTEGER& index,
    const INTEGER& returncount);
extern HEXSTRING substr(const HEXSTRING_ELEMENT& value, int index,
    int returncount);
extern HEXSTRING substr(const HEXSTRING_ELEMENT& value, int index,
    const INTEGER& returncount);
extern HEXSTRING substr(const HEXSTRING_ELEMENT& value, const INTEGER& index,
    int returncount);
extern HEXSTRING substr(const HEXSTRING_ELEMENT& value, const INTEGER& index,
    const INTEGER& returncount);

extern OCTETSTRING substr(const OCTETSTRING& value, int index,
    int returncount);
extern OCTETSTRING substr(const OCTETSTRING& value, int index,
    const INTEGER& returncount);
extern OCTETSTRING substr(const OCTETSTRING& value, const INTEGER& index,
    int returncount);
extern OCTETSTRING substr(const OCTETSTRING& value, const INTEGER& index,
    const INTEGER& returncount);
extern OCTETSTRING substr(const OCTETSTRING_ELEMENT& value, int index,
    int returncount);
extern OCTETSTRING substr(const OCTETSTRING_ELEMENT& value, int index,
    const INTEGER& returncount);
extern OCTETSTRING substr(const OCTETSTRING_ELEMENT& value,
    const INTEGER& index, int returncount);
extern OCTETSTRING substr(const OCTETSTRING_ELEMENT& value,
    const INTEGER& index, const INTEGER& returncount);

extern CHARSTRING substr(const CHARSTRING& value, int index,
    int returncount);
extern CHARSTRING substr(const CHARSTRING& value, int index,
    const INTEGER& returncount);
extern CHARSTRING substr(const CHARSTRING& value, const INTEGER& index,
    int returncount);
extern CHARSTRING substr(const CHARSTRING& value, const INTEGER& index,
    const INTEGER& returncount);
extern CHARSTRING substr(const CHARSTRING_ELEMENT& value, int index,
    int returncount);
extern CHARSTRING substr(const CHARSTRING_ELEMENT& value, int index,
    const INTEGER& returncount);
extern CHARSTRING substr(const CHARSTRING_ELEMENT& value,
    const INTEGER& index, int returncount);
extern CHARSTRING substr(const CHARSTRING_ELEMENT& value,
    const INTEGER& index, const INTEGER& returncount);

extern UNIVERSAL_CHARSTRING substr(const UNIVERSAL_CHARSTRING& value,
    int index, int returncount);
extern UNIVERSAL_CHARSTRING substr(const UNIVERSAL_CHARSTRING& value,
    int index, const INTEGER& returncount);
extern UNIVERSAL_CHARSTRING substr(const UNIVERSAL_CHARSTRING& value,
    const INTEGER& index, int returncount);
extern UNIVERSAL_CHARSTRING substr(const UNIVERSAL_CHARSTRING& value,
    const INTEGER& index, const INTEGER& returncount);
extern UNIVERSAL_CHARSTRING substr(const UNIVERSAL_CHARSTRING_ELEMENT& value,
    int index, int returncount);
extern UNIVERSAL_CHARSTRING substr(const UNIVERSAL_CHARSTRING_ELEMENT& value,
    int index, const INTEGER& returncount);
extern UNIVERSAL_CHARSTRING substr(const UNIVERSAL_CHARSTRING_ELEMENT& value,
    const INTEGER& index, int returncount);
extern UNIVERSAL_CHARSTRING substr(const UNIVERSAL_CHARSTRING_ELEMENT& value,
    const INTEGER& index, const INTEGER& returncount);

// substr for templates
extern BITSTRING substr(const BITSTRING_template& value, int index,
    int returncount);
extern BITSTRING substr(const BITSTRING_template& value, int index,
    const INTEGER& returncount);
extern BITSTRING substr(const BITSTRING_template& value, const INTEGER& index,
    int returncount);
extern BITSTRING substr(const BITSTRING_template& value, const INTEGER& index,
    const INTEGER& returncount);

extern HEXSTRING substr(const HEXSTRING_template& value, int index,
    int returncount);
extern HEXSTRING substr(const HEXSTRING_template& value, int index,
    const INTEGER& returncount);
extern HEXSTRING substr(const HEXSTRING_template& value, const INTEGER& index,
    int returncount);
extern HEXSTRING substr(const HEXSTRING_template& value, const INTEGER& index,
    const INTEGER& returncount);

extern OCTETSTRING substr(const OCTETSTRING_template& value, int index,
    int returncount);
extern OCTETSTRING substr(const OCTETSTRING_template& value, int index,
    const INTEGER& returncount);
extern OCTETSTRING substr(const OCTETSTRING_template& value, const INTEGER& index,
    int returncount);
extern OCTETSTRING substr(const OCTETSTRING_template& value, const INTEGER& index,
    const INTEGER& returncount);

extern CHARSTRING substr(const CHARSTRING_template& value, int index,
    int returncount);
extern CHARSTRING substr(const CHARSTRING_template& value, int index,
    const INTEGER& returncount);
extern CHARSTRING substr(const CHARSTRING_template& value, const INTEGER& index,
    int returncount);
extern CHARSTRING substr(const CHARSTRING_template& value, const INTEGER& index,
    const INTEGER& returncount);

extern UNIVERSAL_CHARSTRING substr(const UNIVERSAL_CHARSTRING_template& value,
    int index, int returncount);
extern UNIVERSAL_CHARSTRING substr(const UNIVERSAL_CHARSTRING_template& value,
    int index, const INTEGER& returncount);
extern UNIVERSAL_CHARSTRING substr(const UNIVERSAL_CHARSTRING_template& value,
    const INTEGER& index, int returncount);
extern UNIVERSAL_CHARSTRING substr(const UNIVERSAL_CHARSTRING_template& value,
    const INTEGER& index, const INTEGER& returncount);

// C.35 - replace
extern void check_replace_arguments(int value_length, int index, int len,
	const char *string_type, const char *element_name);
extern BITSTRING replace(const BITSTRING& value, int index, int len,
	const BITSTRING& repl);
extern BITSTRING replace(const BITSTRING& value, int index,
	const INTEGER& len, const BITSTRING& repl);
extern BITSTRING replace(const BITSTRING& value, const INTEGER& index,
	int len, const BITSTRING& repl);
extern BITSTRING replace(const BITSTRING& value, const INTEGER& index,
	const INTEGER& len, const BITSTRING& repl);

extern HEXSTRING replace(const HEXSTRING& value, int index, int len,
	const HEXSTRING& repl);
extern HEXSTRING replace(const HEXSTRING& value, int index,
	const INTEGER& len, const HEXSTRING& repl);
extern HEXSTRING replace(const HEXSTRING& value, const INTEGER& index,
	int len, const HEXSTRING& repl);
extern HEXSTRING replace(const HEXSTRING& value, const INTEGER& index,
	const INTEGER& len, const HEXSTRING& repl);

extern OCTETSTRING replace(const OCTETSTRING& value, int index, int len,
	const OCTETSTRING& repl);
extern OCTETSTRING replace(const OCTETSTRING& value, int index,
	const INTEGER& len, const OCTETSTRING& repl);
extern OCTETSTRING replace(const OCTETSTRING& value, const INTEGER& index,
	int len, const OCTETSTRING& repl);
extern OCTETSTRING replace(const OCTETSTRING& value, const INTEGER& index,
	const INTEGER& len, const OCTETSTRING& repl);

extern CHARSTRING replace(const CHARSTRING& value, int index, int len,
	const CHARSTRING& repl);
extern CHARSTRING replace(const CHARSTRING& value, int index,
	const INTEGER& len, const CHARSTRING& repl);
extern CHARSTRING replace(const CHARSTRING& value, const INTEGER& index,
	int len, const CHARSTRING& repl);
extern CHARSTRING replace(const CHARSTRING& value, const INTEGER& index,
	const INTEGER& len, const CHARSTRING& repl);

extern UNIVERSAL_CHARSTRING replace(const UNIVERSAL_CHARSTRING& value,
	int index, int len, const UNIVERSAL_CHARSTRING& repl);
extern UNIVERSAL_CHARSTRING replace(const UNIVERSAL_CHARSTRING& value,
	int index, const INTEGER& len, const UNIVERSAL_CHARSTRING& repl);
extern UNIVERSAL_CHARSTRING replace(const UNIVERSAL_CHARSTRING& value,
	const INTEGER& index, int len, const UNIVERSAL_CHARSTRING& repl);
extern UNIVERSAL_CHARSTRING replace(const UNIVERSAL_CHARSTRING& value,
	const INTEGER& index, const INTEGER& len,
	const UNIVERSAL_CHARSTRING& repl);
// replace - template parameters
extern BITSTRING replace(const BITSTRING_template& value, int index, int len,
	const BITSTRING_template& repl);
extern BITSTRING replace(const BITSTRING_template& value, int index,
	const INTEGER& len, const BITSTRING_template& repl);
extern BITSTRING replace(const BITSTRING_template& value, const INTEGER& index,
	int len, const BITSTRING_template& repl);
extern BITSTRING replace(const BITSTRING_template& value, const INTEGER& index,
	const INTEGER& len, const BITSTRING_template& repl);

extern HEXSTRING replace(const HEXSTRING_template& value, int index, int len,
	const HEXSTRING_template& repl);
extern HEXSTRING replace(const HEXSTRING_template& value, int index,
	const INTEGER& len, const HEXSTRING_template& repl);
extern HEXSTRING replace(const HEXSTRING_template& value, const INTEGER& index,
	int len, const HEXSTRING_template& repl);
extern HEXSTRING replace(const HEXSTRING_template& value, const INTEGER& index,
	const INTEGER& len, const HEXSTRING_template& repl);

extern OCTETSTRING replace(const OCTETSTRING_template& value, int index, int len,
	const OCTETSTRING& repl);
extern OCTETSTRING replace(const OCTETSTRING_template& value, int index,
	const INTEGER& len, const OCTETSTRING& repl);
extern OCTETSTRING replace(const OCTETSTRING_template& value, const INTEGER& index,
	int len, const OCTETSTRING_template& repl);
extern OCTETSTRING replace(const OCTETSTRING_template& value, const INTEGER& index,
	const INTEGER& len, const OCTETSTRING_template& repl);

extern CHARSTRING replace(const CHARSTRING_template& value, int index, int len,
	const CHARSTRING_template& repl);
extern CHARSTRING replace(const CHARSTRING_template& value, int index,
	const INTEGER& len, const CHARSTRING_template& repl);
extern CHARSTRING replace(const CHARSTRING_template& value, const INTEGER& index,
	int len, const CHARSTRING_template& repl);
extern CHARSTRING replace(const CHARSTRING_template& value, const INTEGER& index,
	const INTEGER& len, const CHARSTRING_template& repl);

extern UNIVERSAL_CHARSTRING replace(const UNIVERSAL_CHARSTRING_template& value,
	int index, int len, const UNIVERSAL_CHARSTRING_template& repl);
extern UNIVERSAL_CHARSTRING replace(const UNIVERSAL_CHARSTRING_template& value,
	int index, const INTEGER& len, const UNIVERSAL_CHARSTRING_template& repl);
extern UNIVERSAL_CHARSTRING replace(const UNIVERSAL_CHARSTRING_template& value,
	const INTEGER& index, int len, const UNIVERSAL_CHARSTRING_template& repl);
extern UNIVERSAL_CHARSTRING replace(const UNIVERSAL_CHARSTRING_template& value,
	const INTEGER& index, const INTEGER& len,
	const UNIVERSAL_CHARSTRING_template& repl);


// C.36 - rnd
extern double rnd();
extern double rnd(double seed);
extern double rnd(const FLOAT& seed);


// Additional predefined functions defined in Annex B of ES 101 873-7

// B.1 decomp - not implemented yet


// Non-standard functions

// str2bit
extern BITSTRING str2bit(const char *value);
extern BITSTRING str2bit(const CHARSTRING& value);
extern BITSTRING str2bit(const CHARSTRING_ELEMENT& value);

// str2hex
extern HEXSTRING str2hex(const char *value);
extern HEXSTRING str2hex(const CHARSTRING& value);
extern HEXSTRING str2hex(const CHARSTRING_ELEMENT& value);

// float2str
extern CHARSTRING float2str(double value);
extern CHARSTRING float2str(const FLOAT& value);

// unichar2char
extern CHARSTRING unichar2char(const UNIVERSAL_CHARSTRING& value);
extern CHARSTRING unichar2char(const UNIVERSAL_CHARSTRING_ELEMENT& value);

// only for internal purposes
extern CHARSTRING get_port_name(const char *port_name, int array_index);
extern CHARSTRING get_port_name(const char *port_name,
    const INTEGER& array_index);
extern CHARSTRING get_port_name(const CHARSTRING& port_name, int array_index);
extern CHARSTRING get_port_name(const CHARSTRING& port_name,
    const INTEGER& array_index);

unsigned char char_to_hexdigit(char c);

// unichar2oct
extern OCTETSTRING unichar2oct(const UNIVERSAL_CHARSTRING& invalue); // default string_encoding UTF-8
extern OCTETSTRING unichar2oct(const UNIVERSAL_CHARSTRING& invalue,
                               const CHARSTRING& string_encoding);
//oct2unichar
extern UNIVERSAL_CHARSTRING oct2unichar(const OCTETSTRING& invalue ); // default string_encoding UTF-8
extern UNIVERSAL_CHARSTRING oct2unichar(const OCTETSTRING& invalue,
                                        const CHARSTRING& string_encoding);

extern CHARSTRING get_stringencoding(const OCTETSTRING& encoded__value);
extern OCTETSTRING remove_bom(const OCTETSTRING& encoded__value);
extern CHARSTRING encode_base64(const OCTETSTRING& msg, boolean use_linebreaks);
extern CHARSTRING encode_base64(const OCTETSTRING& msg);
extern OCTETSTRING decode_base64(const CHARSTRING& b64);

#endif
