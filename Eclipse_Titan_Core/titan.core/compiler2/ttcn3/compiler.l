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
 *   Cserveni, Akos
 *   Delic, Adam
 *   Feher, Csaba
 *   Forstner, Matyas
 *   Kovacs, Ferenc
 *   Kremer, Peter
 *   Raduly, Csaba
 *   Szabados, Kristof
 *   Szabo, Bence Janos
 *   Szabo, Janos Zoltan – initial implementation
 *   Zalanyi, Balazs Andor
 *
 ******************************************************************************/
%option noyywrap
%option never-interactive
%option nounput
%{

/* Tokenizer for TTCN-3 Core Language */

#include "../../common/dbgnew.hh"
#include "compiler.h"
#include "../string.hh"
#include "../CompilerError.hh"
#include "../Real.hh"
#include "../Value.hh"
#include "AST_ttcn3.hh"
#include "Ttcnstuff.hh" // at least for PortTypeBody::PortOperationMode_t
#include "Statement.hh" // for Statement::statementtype_t
#include "Attributes.hh"

#include <string.h>
#include <ctype.h>
#include <openssl/md5.h>

namespace Common {
  class IndexedValue;
  class Location;
}

namespace Ttcn {
  class ParamRedirect;
  class Statement;
  class AltGuard;
  class IfClause;
  class IfClauses;
  class NamedTemplate;
  class NamedTemplates;
  class IndexedTemplate;
  class IndexedTemplates;
  class Templates;
  class CompTypeRefList;
}

using namespace Common;
using namespace Ttcn;

#include "compiler.tab.hh"

#define yylval ttcn3_lval
#define yylloc ttcn3_lloc

/* global variable indicating the location of the returned token to bison */
extern YYLTYPE yylloc;

extern bool is_erroneous_parsed;

/* always points to the first character of the regexp to be recognized */
static int current_line, current_column;

/* when reporting an error in linemarker or preprocessor
 * directive the real file name and line number is needed */
static const char* real_infile;
/* real_lineno = current_line + real_lineno_offset */
static int real_lineno_offset;

static bool dot_flag = false;
/* type of the backup token (that was found after a dot) */
static int backup_token;
/* semantic value of the backup token */
static YYSTYPE backup_lval;
/* location of the backup token */
static YYLTYPE backup_lloc;

static MD5_CTX md5_ctx;

static void fill_location()
{
  yylloc.first_line = current_line;
  yylloc.first_column = current_column;
  current_column += yyleng;
  yylloc.last_line = current_line;
  yylloc.last_column = current_column;
}

static void update_md5()
{
  MD5_Update(&md5_ctx, yytext, yyleng);
  MD5_Update(&md5_ctx, " ", 1);
}

#define RETURN_SAVED_DOT \
  do { \
    yytext[0] = '\0'; \
    yylloc.first_line = dot_line; \
    yylloc.first_column = dot_column; \
    yylloc.last_line = dot_line; \
    yylloc.last_column = dot_column + 1; \
    return '.'; \
  } while (0)

/* return macro for simple tokens without semantic value */
#define RETURN(ret_val) \
  do { \
    update_md5(); \
    fill_location(); \
    if (dot_flag) { \
      backup_token = ret_val; \
      backup_lloc = yylloc; \
      RETURN_SAVED_DOT; \
    } else return ret_val; \
  } while (0)

/* same as RETURN(ret_val) macro but without location update,
 * usually a return after an error */
#define RETURN_NOLOCUPD(ret_val) \
  do { \
    update_md5(); \
    if (dot_flag) { \
      backup_token = ret_val; \
      backup_lloc = yylloc; \
      RETURN_SAVED_DOT; \
    } else return ret_val; \
  } while (0)

/* return macro for simple tokens with semantic value */
#define RETURN_LVAL(ret_val) \
  do { \
    update_md5(); \
    fill_location(); \
    if (dot_flag) { \
      backup_token = ret_val; \
      backup_lval = yylval; \
      backup_lloc = yylloc; \
      RETURN_SAVED_DOT; \
    } else return ret_val; \
  } while (0)

/* return macro for special tokens that are glued together with previous dot */
#define RETURN_DOT(ret_val) \
  do { \
    update_md5(); \
    if (dot_flag) { \
      dot_flag = false; \
      yylloc.first_line = dot_line; \
      yylloc.first_column = dot_column; \
      current_column += yyleng; \
      yylloc.last_line = current_line; \
      yylloc.last_column = current_column; \
      return Dot##ret_val; \
    } else { \
      fill_location(); \
      return ret_val; \
    } \
  } while (0)

extern string *parse_charstring_value(const char *str, const Location& loc);

%}

NUMBER 0|([1-9][0-9]*)

FLOAT ({NUMBER}\.[0-9]+)|({NUMBER}(\.[0-9]+)?[Ee][+-]?{NUMBER})

IDENTIFIER [A-Za-z][A-Za-z0-9_]*

LINECOMMENT "//"[^\r\n]*

WHITESPACE [ \t\v\f]

NEWLINE \r|\n|\r\n

LINEMARKER {NUMBER}{WHITESPACE}+\"([^\\\"\r\n]|\\[^\r\n])*\"

UID [uU][+]?[0-9A-Fa-f]{1,8}

TITAN "$#&&&(#TITANERRONEOUS$#&&^#% "

%x SC_blockcomment SC_cstring
%x SC_binstring SC_binstring_bad
%s SC_charkeyword

%%
  /* local variables of yylex() */
  int start_line = 0, start_column = 0; /**< used by block comments and
					     string literals */
  int dot_line = 0, dot_column = 0; /**< location of the previous '.' token */
  /* variables used when processing binary strings */
  expstring_t binstr = NULL; /**< the string itself */
  bool valid_bit = false, /**< binstr is valid bitstring */
    valid_oct = false, /**< binstr is valid octetstring */
    half_oct = false, /**< binstr is not a valid octetstr but a valid hexstr */
    contains_match = false, /**< binstr contains matching symbol */
    contains_ws = false; /**< binstr contains whitespace characters */

  if (dot_flag) {
    if (backup_token == '.') {
      /* Two dots were found in the previous call: the first one was returned,
       * the second one is now in the backup. Let's assume that we have just
       * found the second one. */
      dot_line = backup_lloc.first_line;
      dot_column = backup_lloc.first_column;
    } else {
      /* Return the token and its semantic value that was backed up after the
       * last token (which was a dot). */
      dot_flag = false;
      yylval = backup_lval;
      yylloc = backup_lloc;
      return backup_token;
    }
  }


{TITAN} {
  // hack: to avoid the erroneous parsing reporting a syntax error where it
  //       also lists TTCN3ModuleKeyword as a possible nonterminal to use
  if (is_erroneous_parsed) {
    RETURN(TitanErroneousHackKeyword);
  } else {
    Location loc(infile, current_line, current_column, current_line,
                 current_column + yyleng);
    loc.error("Unexpected `%s'.", yytext);
  }
}

	/* Eat up comments and whitespaces */

"/*" {
  start_line = current_line;
  start_column = current_column;
  current_column += 2;
  BEGIN(SC_blockcomment);
}

<SC_blockcomment> /* -------- SC_blockcomment scope -------------- */
{

  "*/" {
    current_column += 2;
    BEGIN(INITIAL);
  }

  {NEWLINE} {
    current_line++;
    current_column = 0;
  }

  . current_column++;

} /* SC_blockcomment */

{LINECOMMENT}?{NEWLINE} {
  current_line++;
  current_column = 0;
}

{LINECOMMENT} {
  current_column += yyleng;
}

{WHITESPACE}+ current_column += yyleng;

	/* C preprocessor line markers */

^{WHITESPACE}*"#"({WHITESPACE}*"line")?{WHITESPACE}+{LINEMARKER}[^\r\n]*{NEWLINE} {
  bool error_flag = false;
  int real_lineno = current_line + real_lineno_offset;
  /* skipping the leading whitespaces */
  int marker_begin = 0;
  while (yytext[marker_begin] != '#') marker_begin++;
  /* skipping the trailing whitespaces and newline */
  int marker_end = yyleng - 1;
  while (yytext[marker_end] == '\r' || yytext[marker_end] == '\n' ||
	 yytext[marker_end] == ' ' || yytext[marker_end] == '\t') marker_end--;
  marker_end++;
  Location loc(real_infile, real_lineno, current_column + marker_begin,
    real_lineno, current_column + marker_end);
  Error_Context cntxt(&loc, "In preprocessor line marker");
  /* parsing out the line number */
  int lineno_begin = marker_begin + 1;
  while (!isdigit(static_cast<unsigned char>( yytext[lineno_begin] ))) lineno_begin++;
  int lineno_end = lineno_begin + 1;
  while (isdigit(static_cast<int>( yytext[lineno_end] ))) lineno_end++;
  errno = 0;
  int new_lineno = strtol(yytext + lineno_begin, NULL, 10);
  if (errno != 0) {
    Location lineno_loc(real_infile, real_lineno, current_column + lineno_begin,
      real_lineno, current_column + lineno_end);
    string lineno_str(lineno_end - lineno_begin, yytext + lineno_begin);
    lineno_loc.error("Line number `%s' is too large for being represented in "
      "memory: %s", lineno_str.c_str(), strerror(errno));
    error_flag = true;
  }
  /* parsing out the file name */
  int filename_begin = lineno_end + 1;
  while (yytext[filename_begin] != '"') filename_begin++;
  filename_begin++;
  int filename_end = filename_begin;
  while (yytext[filename_end] != '"') {
    if (yytext[filename_end] == '\\') filename_end += 2;
    else filename_end++;
  }
  Location filename_loc(real_infile, real_lineno, current_column +
    filename_begin - 1, real_lineno, current_column + filename_end + 1);
  string filename_str(filename_end - filename_begin, yytext + filename_begin);
  string *parsed_filename = parse_charstring_value(filename_str.c_str(),
    filename_loc);
  if (!parsed_filename) error_flag = true;
  /* updating the line/column counters */
  if (error_flag) {
    /* the line marker is erroneous, use the real line numbers */
    infile = real_infile;
    current_line = real_lineno + 1;
    real_lineno_offset = 0;
  } else {
    /* set the given line number */
    infile = Location::add_source_file_name(*parsed_filename);
    current_line = new_lineno;
    real_lineno_offset = real_lineno + 1 - new_lineno;
  }
  current_column = 0;
  delete parsed_filename;
}

^{WHITESPACE}*"#"[^\r\n]* {
  int real_lineno = current_line + real_lineno_offset;
  /* skip the leading and trailing whitespaces */
  int marker_begin = 0;
  while (yytext[marker_begin] != '#') marker_begin++;
  int marker_end = yyleng - 1;
  while (yytext[marker_end] == ' ' || yytext[marker_end] == '\t') marker_end--;
  marker_end++;
  Location loc(real_infile, real_lineno, current_column + marker_begin,
    real_lineno, current_column + marker_end);
  loc.error("Invalid/unsupported preprocessor directive or line marker: `%s'",
    string(marker_end - marker_begin, yytext + marker_begin).c_str());
  current_column += yyleng;
}

	/* Keywords */

action		RETURN(ActionKeyword);
activate	RETURN(ActivateKeyword);
address		RETURN(AddressKeyword);
alive		RETURN_DOT(AliveKeyword);
all		RETURN(AllKeyword);
alt		RETURN(AltKeyword);
altstep		RETURN(AltstepKeyword);
and		RETURN(AndKeyword);
and4b		RETURN(And4bKeyword);
any		RETURN(AnyKeyword);
anytype		RETURN(AnyTypeKeyword);
apply		RETURN_DOT(ApplyKeyword);
bitstring	RETURN(BitStringKeyword);
boolean		RETURN(BooleanKeyword);
break		RETURN(BreakKeyword);
call		RETURN_DOT(CallOpKeyword);
case		RETURN(CaseKeyword);
catch		RETURN_DOT(CatchOpKeyword);
char		{ BEGIN(SC_charkeyword); RETURN(CharKeyword); }
charstring	RETURN(CharStringKeyword);
checkstate      RETURN_DOT(CheckStateKeyword);
check		RETURN_DOT(CheckOpKeyword);
clear		RETURN_DOT(ClearOpKeyword);
complement	RETURN(ComplementKeyword);
component	RETURN(ComponentKeyword);
connect		RETURN(ConnectKeyword);
const		RETURN(ConstKeyword);
continue	RETURN(ContinueKeyword);
control		RETURN(ControlKeyword);
create		RETURN_DOT(CreateKeyword);
decmatch	RETURN(DecodedMatchKeyword);
deactivate	RETURN(DeactivateKeyword);
default		RETURN(DefaultKeyword);
derefers	RETURN(DerefersKeyword);
disconnect	RETURN(DisconnectKeyword);
display		RETURN(DisplayKeyword);
do		RETURN(DoKeyword);
done		RETURN_DOT(DoneKeyword);
else		RETURN(ElseKeyword);
encode		RETURN(EncodeKeyword);
enumerated	RETURN(EnumKeyword);
error		RETURN(ErrorKeyword);
except		RETURN(ExceptKeyword);
exception	RETURN(ExceptionKeyword);
execute		RETURN(ExecuteKeyword);
extends		RETURN(ExtendsKeyword);
extension	RETURN(ExtensionKeyword);
external	RETURN(ExtKeyword);
fail		RETURN(FailKeyword);
false		RETURN(FalseKeyword);
float		RETURN(FloatKeyword);
for		RETURN(ForKeyword);
friend		RETURN(FriendKeyword);
from		RETURN(FromKeyword);
function	RETURN(FunctionKeyword);
getcall		RETURN_DOT(GetCallOpKeyword);
getreply	RETURN_DOT(GetReplyOpKeyword);
getverdict	RETURN(GetVerdictKeyword);
goto		RETURN(GotoKeyword);
group		RETURN(GroupKeyword);
halt		RETURN_DOT(HaltKeyword);
hexstring	RETURN(HexStringKeyword);
if		RETURN(IfKeyword);
ifpresent	RETURN(IfPresentKeyword);
import		RETURN(ImportKeyword);
in		RETURN(InParKeyword);
inconc		RETURN(InconcKeyword);
infinity	RETURN(InfinityKeyword);
inout		RETURN(InOutParKeyword);
integer		RETURN(IntegerKeyword);
interleave	RETURN(InterleavedKeyword);
kill		RETURN_DOT(KillKeyword);
killed		RETURN_DOT(KilledKeyword);
label		RETURN(LabelKeyword);
language	RETURN(LanguageKeyword);
length		RETURN(LengthKeyword);
log		RETURN(LogKeyword);
map		RETURN(MapKeyword);
match		RETURN(MatchKeyword);
message		RETURN(MessageKeyword);
mixed		RETURN(MixedKeyword);
mod		RETURN(ModKeyword);
modifies	RETURN(ModifiesKeyword);
module		RETURN(TTCN3ModuleKeyword);
modulepar	RETURN(ModuleParKeyword);
mtc		RETURN(MTCKeyword);
noblock		RETURN(NoBlockKeyword);
none		RETURN(NoneKeyword);
not		RETURN(NotKeyword);
not_a_number  RETURN(NaNKeyword);
not4b		RETURN(Not4bKeyword);
nowait		RETURN(NowaitKeyword);
null		RETURN(NullKeyword);
objid		RETURN(ObjectIdentifierKeyword);
octetstring	RETURN(OctetStringKeyword);
of		RETURN(OfKeyword);
omit		RETURN(OmitKeyword);
on		RETURN(OnKeyword);
optional	RETURN(OptionalKeyword);
or		RETURN(OrKeyword);
or4b		RETURN(Or4bKeyword);
out		RETURN(OutParKeyword);
override	RETURN(OverrideKeyword);
param		RETURN(ParamKeyword);
pass		RETURN(PassKeyword);
pattern		RETURN(PatternKeyword);
permutation	RETURN(PermutationKeyword);
port		RETURN(PortKeyword);
present		RETURN(PresentKeyword);
private		RETURN(PrivateKeyword);
procedure	RETURN(ProcedureKeyword);
public		RETURN(PublicKeyword);
raise		RETURN_DOT(RaiseKeyword);
read		RETURN_DOT(ReadKeyword);
receive		RETURN_DOT(ReceiveOpKeyword);
record		RETURN(RecordKeyword);
recursive	RETURN(RecursiveKeyword);
refers		RETURN(RefersKeyword);
rem		RETURN(RemKeyword);
repeat		RETURN(RepeatKeyword);
reply		RETURN_DOT(ReplyKeyword);
return		RETURN(ReturnKeyword);
running		RETURN_DOT(RunningKeyword);
runs		RETURN(RunsKeyword);
select		RETURN(SelectKeyword);
self		RETURN(SelfKeyword);
send		RETURN_DOT(SendOpKeyword);
sender		RETURN(SenderKeyword);
set		RETURN(SetKeyword);
setstate	RETURN(SetstateKeyword);
setverdict	RETURN(SetVerdictKeyword);
signature	RETURN(SignatureKeyword);
start		RETURN_DOT(StartKeyword);
stop		RETURN_DOT(StopKeyword);
subset		RETURN(SubsetKeyword);
superset	RETURN(SupersetKeyword);
system		RETURN(SystemKeyword);
template	RETURN(TemplateKeyword);
testcase	RETURN(TestcaseKeyword);
timeout		RETURN_DOT(TimeoutKeyword);
timer		RETURN(TimerKeyword);
to		RETURN(ToKeyword);
trigger		RETURN_DOT(TriggerOpKeyword);
true 		RETURN(TrueKeyword);
type		RETURN(TypeDefKeyword);
union		RETURN(UnionKeyword);
universal	RETURN(UniversalKeyword);
unmap		RETURN(UnmapKeyword);
value		RETURN(ValueKeyword);
valueof		RETURN(ValueofKeyword);
var		RETURN(VarKeyword);
variant		RETURN(VariantKeyword);
verdicttype	RETURN(VerdictTypeKeyword);
while		RETURN(WhileKeyword);
with		RETURN(WithKeyword);
xor		RETURN(XorKeyword);
xor4b		RETURN(Xor4bKeyword);

  /* modifier keywords */

"@nocase"        RETURN(NocaseKeyword);
"@lazy"          RETURN(LazyKeyword);
"@decoded"       RETURN(DecodedKeyword);
"@deterministic" RETURN(DeterministicKeyword);
"@fuzzy"         RETURN(FuzzyKeyword);
"@index"         RETURN(IndexKeyword);

  /* special TITAN specific keywords */

"@try"    RETURN(TitanSpecificTryKeyword);
"@catch"  RETURN(TitanSpecificCatchKeyword);
"@profiler" RETURN(TitanSpecificProfilerKeyword);
"@update" RETURN(TitanSpecificUpdateKeyword);

	/* Predefined function identifiers */

bit2hex		RETURN(bit2hexKeyword);
bit2int		RETURN(bit2intKeyword);
bit2oct		RETURN(bit2octKeyword);
bit2str		RETURN(bit2strKeyword);
char2int	RETURN(char2intKeyword);
char2oct	RETURN(char2octKeyword);
decomp		RETURN(decompKeyword);
float2int	RETURN(float2intKeyword);
float2str	RETURN(float2strKeyword);
hex2bit		RETURN(hex2bitKeyword);
hex2int		RETURN(hex2intKeyword);
hex2oct		RETURN(hex2octKeyword);
hex2str		RETURN(hex2strKeyword);
int2bit		RETURN(int2bitKeyword);
int2char	RETURN(int2charKeyword);
int2enum  RETURN(int2enumKeyword);
int2float	RETURN(int2floatKeyword);
int2hex		RETURN(int2hexKeyword);
int2oct		RETURN(int2octKeyword);
int2str		RETURN(int2strKeyword);
int2unichar	RETURN(int2unicharKeyword);
isvalue		RETURN(isvalueKeyword);
isbound		RETURN(isboundKeyword);
ischosen	RETURN(ischosenKeyword);
ispresent	RETURN(ispresentKeyword);
istemplatekind  RETURN(istemplatekindKeyword);
lengthof	RETURN(lengthofKeyword);
oct2bit		RETURN(oct2bitKeyword);
oct2char	RETURN(oct2charKeyword);
oct2hex		RETURN(oct2hexKeyword);
oct2int		RETURN(oct2intKeyword);
oct2str		RETURN(oct2strKeyword);
regexp		RETURN(regexpKeyword);
replace		RETURN(replaceKeyword);
rnd			RETURN(rndKeyword);
sizeof		RETURN(sizeofKeyword);
str2bit		RETURN(str2bitKeyword);
str2float	RETURN(str2floatKeyword);
str2hex		RETURN(str2hexKeyword);
str2int		RETURN(str2intKeyword);
str2oct		RETURN(str2octKeyword);
substr		RETURN(substrKeyword);
unichar2int	RETURN(unichar2intKeyword);
unichar2char	RETURN(unichar2charKeyword);
log2str         RETURN(log2strKeyword);
enum2int        RETURN(enum2intKeyword);
encvalue	RETURN(encvalueKeyword);
decvalue	RETURN(decvalueKeyword);
testcasename    RETURN(testcasenameKeyword);
ttcn2string RETURN(ttcn2stringKeyword);
string2ttcn RETURN(string2ttcnKeyword);
unichar2oct	RETURN(unichar2octKeyword);
oct2unichar	RETURN(oct2unicharKeyword);
remove_bom	RETURN(remove_bomKeyWord);
get_stringencoding	RETURN(get_stringencodingKeyWord);
encode_base64	RETURN(encode_base64KeyWord);
decode_base64	RETURN(decode_base64KeyWord);
encvalue_unichar RETURN(encvalue_unicharKeyWord);
decvalue_unichar RETURN(decvalue_unicharKeyWord);
any2unistr      RETURN(any2unistrKeyWord);
hostid          RETURN(hostidKeyWord);

	/* Values */

{NUMBER} {
  Location loc(infile, current_line, current_column, current_line,
    current_column + yyleng);
  yylval.int_val = new int_val_t(yytext, loc);
  RETURN_LVAL(Number);
}

{FLOAT} {
  Location loc(infile, current_line, current_column, current_line,
    current_column + yyleng);
  yylval.float_val = string2Real(yytext, loc);
  RETURN_LVAL(FloatValue);
}

NULL RETURN(NullValue);

"'" {
  binstr=memptystr();
  valid_bit=true;
  valid_oct=true;
  half_oct=false;
  contains_match=false;
  contains_ws=false;
  start_line = current_line;
  start_column = current_column;
  current_column++;
  MD5_Update(&md5_ctx, yytext, yyleng);
  BEGIN(SC_binstring);
}

\" {
  yylval.str = memptystr();
  start_line = current_line;
  start_column = current_column;
  current_column++;
  MD5_Update(&md5_ctx, yytext, yyleng);
  BEGIN(SC_cstring);
}  

<SC_charkeyword>
{  
  {UID} {
    yylval.str = mcopystrn(yytext, yyleng);
    RETURN_LVAL(Cstring);
  }

  [,] { RETURN(*yytext); }

  [)] { BEGIN(INITIAL); RETURN(*yytext); }
}

<SC_binstring> /* -------- SC_binstring scope -------------- */
{

  {WHITESPACE}+ {
    contains_ws = true;
    current_column += yyleng;
  }

  {WHITESPACE}*{NEWLINE} {
    contains_ws = true;
    current_line++;
    current_column = 0;
  }

  [01] {
    binstr = mputc(binstr, yytext[0]);
    half_oct = !half_oct;
    current_column++;
  }

  [2-9A-F] {
    binstr = mputc(binstr, yytext[0]);
    valid_bit = false;
    half_oct = !half_oct;
    current_column++;
  }

  [a-f] {
    binstr = mputc(binstr, yytext[0] - 'a' + 'A');
    valid_bit = false;
    half_oct = !half_oct;
    current_column++;
  }

  "?"|"*" {
    binstr = mputc(binstr, yytext[0]);
    contains_match = true;
    if (half_oct) valid_oct = false;
    current_column++;
  }

  "'"[bBhHoO] {
    yylloc.first_line = start_line;
    yylloc.first_column = start_column;
    yylloc.last_line = current_line;
    yylloc.last_column = current_column + 2;
    Location loc(infile, yylloc);
    int ret_val = TOK_errval;
    switch (yytext[1]) {
    case 'b': {
      Location loc2(infile, current_line, current_column + 1, current_line,
	current_column + 2);
      loc2.warning("The last character of a bitstring literal should be "
	"`B' instead of `b'");
      /* no break */ }
    case 'B':
      if (valid_bit) {
	if (contains_ws) loc.warning("Bitstring %s contains whitespace and/or "
	  "newline character(s)", contains_match ? "match" : "value");
        ret_val = contains_match ? BitStringMatch : Bstring;
	yylval.string_val = new string(binstr);
      } else loc.error("Bitstring value contains invalid character");
      break;
    case 'h': {
      Location loc2(infile, current_line, current_column + 1, current_line,
	current_column + 2);
      loc2.warning("The last character of a hexstring literal should be "
	"`H' instead of `h'");
      /* no break */ }
    case 'H':
      if (contains_ws) loc.warning("Hexstring %s contains whitespace and/or "
	"newline character(s)", contains_match ? "match" : "value");
      ret_val = contains_match ? HexStringMatch : Hstring;
      yylval.string_val = new string(binstr);
      break;
    case 'o': {
      Location loc2(infile, current_line, current_column + 1, current_line,
	current_column + 2);
      loc2.warning("The last character of an octetstring literal should be "
	"`O' instead of `o'");
      /* no break */ }
    case 'O':
      if (valid_oct && !half_oct) {
	if (contains_ws) loc.warning("Octetstring %s contains whitespace "
	  "and/or newline character(s)", contains_match ? "match" : "value");
	ret_val = contains_match ? OctetStringMatch : Ostring;
	yylval.string_val = new string(binstr);
      } else if (contains_match) {
	loc.error("Octetstring match contains half octet(s)");
      } else {
	loc.error("Octetstring value contains odd number of hexadecimal "
	  "digits");
      }
    }
    MD5_Update(&md5_ctx, binstr, strlen(binstr));
    Free(binstr);
    update_md5();
    BEGIN(INITIAL);
    current_column += 2;
    if (dot_flag) {
      backup_token = ret_val;
      backup_lval = yylval;
      backup_lloc = yylloc;
      RETURN_SAVED_DOT;
    } else return ret_val;
  }

  "'" {
    yylloc.first_line = start_line;
    yylloc.first_column = start_column;
    current_column++;
    yylloc.last_line = current_line;
    yylloc.last_column = current_column;
    Location loc(infile, yylloc);
    loc.error("Invalid binary string literal. Expecting `B', `H' or `O' after "
      "the closing `''");
    MD5_Update(&md5_ctx, binstr, strlen(binstr));
    Free(binstr);
    BEGIN(INITIAL);
    RETURN_NOLOCUPD(TOK_errval);
  }

  . {
    Location loc(infile, current_line, current_column, current_line,
      current_column + 1);
    int c = static_cast<unsigned char>( yytext[0] );
    loc.error("Invalid character `%c' (0x%02X) in binary string",
      isprint(c) ? c : '?', c);
    MD5_Update(&md5_ctx, binstr, strlen(binstr));
    Free(binstr);
    MD5_Update(&md5_ctx, yytext, 1);
    current_column++;
    BEGIN(SC_binstring_bad);
  }

} /* SC_binstring scope */

<SC_binstring_bad> /* -------- SC_binstring_bad scope -------------- */
{

  {WHITESPACE}+ current_column += yyleng;

  {WHITESPACE}*{NEWLINE} {
    current_line++;
    current_column = 0;
  }

  "'"[bBhHoO]? {
    current_column += yyleng;
    yylloc.first_line = start_line;
    yylloc.first_column = start_column;
    yylloc.last_line = current_line;
    yylloc.last_column = current_column;
    BEGIN(INITIAL);
    RETURN_NOLOCUPD(TOK_errval);
  }

  . {
    MD5_Update(&md5_ctx, yytext, yyleng);
    current_column++;
  }

} /* SC_binstring_bad scope */

<SC_cstring> /* -------- SC_cstring scope -------------- */
{

  \\?{NEWLINE} { /* newline possibly preceded by backslash */
    yylval.str = mputstr(yylval.str, yytext);
    current_line++;
    current_column = 0;
  }

  \"\"|\\. { /* two doublequotes or any backslash-escaped char */
    yylval.str = mputstr(yylval.str, yytext);
    current_column += 2;
    /* Note that both get added ("external representation").
     * parse_charstring_value() in charstring_la.l is responsible
     * for transforming the string to "internal representation" */
  }

  \" {
    current_column++;
    yylloc.first_line = start_line;
    yylloc.first_column = start_column;
    yylloc.last_line = current_line;
    yylloc.last_column = current_column;
    MD5_Update(&md5_ctx, yylval.str, strlen(yylval.str));
    update_md5();
    BEGIN(INITIAL);
    if (dot_flag) {
      backup_token = Cstring;
      backup_lval = yylval;
      backup_lloc = yylloc;
      RETURN_SAVED_DOT;
    } else return Cstring;
  }

  . {
    yylval.str = mputc(yylval.str, yytext[0]);
    current_column++;
  }

} /* SC_cstring scope */

	/* Macros */

"%moduleId" {
  yylval.macrotype = Value::MACRO_MODULEID;
  RETURN_LVAL(MacroValue);
}
"%fileName" {
  yylval.macrotype = Value::MACRO_FILENAME;
  RETURN_LVAL(MacroValue);
}
"%lineNumber" {
  yylval.macrotype = Value::MACRO_LINENUMBER;
  RETURN_LVAL(MacroValue);
}
"%definitionId"	{
  yylval.macrotype = Value::MACRO_DEFINITIONID;
  RETURN_LVAL(MacroValue);
}
"%testcaseId" {
  yylval.macrotype = Value::MACRO_TESTCASEID;
  RETURN_LVAL(MacroValue);
}
"%"{IDENTIFIER} {
  fill_location();
  Location loc(infile, yylloc);
  loc.error("Invalid macro notation: `%s'", yytext);
  RETURN_NOLOCUPD(TOK_errval);
}

"__MODULE__" {
  yylval.macrotype = Value::MACRO_MODULEID;
  RETURN_LVAL(MacroValue);
}
"__FILE__" {
  yylval.macrotype = Value::MACRO_FILEPATH;
  RETURN_LVAL(MacroValue);
}
"__BFILE__" {
  yylval.macrotype = Value::MACRO_BFILENAME;
  RETURN_LVAL(MacroValue);
}
"__LINE__" {
  yylval.macrotype = Value::MACRO_LINENUMBER_C;
  RETURN_LVAL(MacroValue);
}
"__SCOPE__" {
  yylval.macrotype = Value::MACRO_SCOPE;
  RETURN_LVAL(MacroValue);
}
"__TESTCASE__" {
  yylval.macrotype = Value::MACRO_TESTCASEID;
  RETURN_LVAL(MacroValue);
}
"__"{IDENTIFIER}"__" {
  fill_location();
  Location loc(infile, yylloc);
  loc.error("Invalid macro notation: `%s'", yytext);
  RETURN_NOLOCUPD(TOK_errval);
}

	/* Multi-character operators */

":="		RETURN(AssignmentChar);
"\.\."		RETURN(DotDot);
"->"		RETURN(PortRedirectSymbol);
"=="		RETURN(EQ);
"!="		RETURN(NE);
">="		RETURN(GE);
"<="		RETURN(LE);
"<<"		RETURN(SL);
">>"		RETURN(SR);
"<@"		RETURN(RL);
"@>"		RETURN(_RR);

"++" |
"--" {
  fill_location();
  Location loc(infile, yylloc);
  loc.error("Operator `%s' is reserved for future use", yytext);
}

	/* Invalid operators */

"::=" {
  fill_location();
  Location loc(infile, yylloc);
  loc.error("`::=' is not a valid assignment operator in TTCN-3. Did you mean "
    "`:='?");
  RETURN_NOLOCUPD(AssignmentChar);
}

"=" {
  fill_location();
  Location loc(infile, yylloc);
  loc.error("A single `=' character cannot be used in TTCN-3. Did you mean "
    "the assignment sign `:=' or the equality operator `=='?");
  /* the former is more probable than the latter */
  RETURN_NOLOCUPD(AssignmentChar);
}

"<>" {
  fill_location();
  Location loc(infile, yylloc);
  loc.error("`<>' is not a valid comparison operator in TTCN-3. Did you mean "
    "`!='?");
  RETURN_NOLOCUPD(NE);
}

	/* Identifiers */

{IDENTIFIER} {
  yylval.id = new Identifier(Identifier::ID_TTCN, string(yyleng, yytext));
  RETURN_LVAL(IDentifier);
}

	/* Single character tokens (brackets, operators, etc.) */

\. {
  update_md5();
  if (dot_flag) {
    /* store this dot in the backup */
    backup_token = '.';
    backup_lloc.first_line = current_line;
    backup_lloc.first_column = current_column;
    current_column++;
    backup_lloc.last_line = current_line;
    backup_lloc.last_column = current_column;
    /* return the dot that was found previously */
    RETURN_SAVED_DOT;
  } else {
    dot_flag = true;
    dot_line = current_line;
    dot_column = current_column;
    current_column++;
  }
}

[()\[\]{}+\-\*/&:;,<>\?!]  RETURN(*yytext);

	/* Invalid characters */

. {
  fill_location();
  Location loc(infile, yylloc);
  int c = static_cast<unsigned char>( yytext[0] );
  loc.error("Character `%c' (0x%02X) is not used in TTCN-3",
    isprint(c) ? c : '?', c);
}

	/* EOF rule */

<*><<EOF>> {
  if (YY_START != INITIAL) {
    Location loc(infile, start_line, start_column, current_line,
      current_column);
    switch (YY_START) {
    case SC_blockcomment:
      loc.error("Unterminated block comment");
      break;
    case SC_binstring:
      Free(binstr);
      /* no break */
    case SC_binstring_bad:
      loc.error("Unterminated binary string literal");
      break;
    case SC_cstring:
      Free(yylval.str);
      loc.error("Unterminated character string literal");
    }
    BEGIN(INITIAL);
  }
  if (dot_flag) {
    dot_flag = false;
    RETURN_SAVED_DOT;
  } else {
    yylloc.first_line = current_line;
    yylloc.first_column = current_column;
    yylloc.last_line = current_line;
    yylloc.last_column = current_column + 1;
    return EOF;
  }
}

%%

void init_ttcn3_lex()
{
  dot_flag = false;
  current_line = 1;
  current_column = 0;
  real_infile = infile;
  real_lineno_offset = 0;
  MD5_Init(&md5_ctx);
}

void init_erroneous_lex(const char* p_infile, int p_line, int p_column)
{
  infile = p_infile;
  current_line = p_line;
  current_column = p_column;
  real_infile = infile;
  real_lineno_offset = 0;
  dot_flag = false;
}

void free_dot_flag_stuff()
{
  if (dot_flag) {
    dot_flag = false;
    /* clean up the semantic value of the token that was backed up */
    switch (backup_token) {
    case IDentifier:
      delete backup_lval.id;
      break;
    case Bstring:
    case Hstring:
    case Ostring:
    case BitStringMatch:
    case HexStringMatch:
    case OctetStringMatch:
      delete backup_lval.string_val;
      break;
    case Cstring:
      Free(backup_lval.str);
    default:
      break;
    }
  }
}

void free_ttcn3_lex()
{
  free_dot_flag_stuff();
  fclose(ttcn3_in);
  ttcn3_lex_destroy();
}

/* called from ttcn3_parse_file to finalize MD5 and add it to the module */
void set_md5_checksum(Ttcn::Module *m)
{
  unsigned char md5_sum[MD5_DIGEST_LENGTH];
  MD5_Final(md5_sum, &md5_ctx);
  m->set_checksum(sizeof(md5_sum), md5_sum);
}
