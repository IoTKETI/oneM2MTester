/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Szabo, Janos Zoltan – initial implementation
 *
 ******************************************************************************/
 
%option noyywrap
%option yylineno
%option nounput
%{

/*

Tokenizer source code for TTCN-3 (input of flex)
Rev: PA10
Date: April 15 2005
Author: Janos Zoltan Szabo (ejnosza)

Revision history:

Rev.	Date		Author		Comments
PA1	Nov 10 2000	tmpjsz		Created from the prototype compiler.
PA2	Dec  5 2000	tmpjsz		Obsolete keywords removed.
					Updated according to BNF v1.0.10.
PA3	Dec 11 2000	tmpjsz		No changes.

PA5	Sep 20 2001	tmpjsz		Updated according to BNF v1.1.2.
PA6	Apr 16-17 2002	tmpjsz		Upgrade to BNF v2.2.0 (Rev. 12.5)
PA7	Nov 26-27 2002	tmpjsz		Upgrade to BNF v2.2.1 (Rev. 12.7)
PA8	May 10-13 2004	ejnosza		Upgrade to BNF v3.0.0Mockupv1
PA9	March 2005	ejnosza		Added support for multi-dimension
					sub-references in port/timer/component
					operations.
PA10	Apr 13-15 2005	ejnosza		Upgrade to final BNF v3.0.0

*/

#include "parser.tab.h"

int dot_flag = 0;

#define RETURN(ret_val) \
	if (dot_flag) { \
		backup_token = ret_val; \
		return '.'; \
	} else return ret_val

#define RETURN_DOT(ret_val) \
	if (dot_flag) { \
		dot_flag = 0; \
		return Dot##ret_val; \
	} else return ret_val

%}

BIN 0|1
BINORMATCH {BIN}|"\?"|"\*"
HEX [0-9A-Fa-f]
HEXORMATCH {HEX}|"\?"|"\*"
OCT {HEX}{HEX}
OCTORMATCH {OCT}|"\?"|"\*"
CHAR [^\"]

NUMBER 0|([1-9][0-9]*)

FLOAT ({NUMBER}\.[0-9]+)|({NUMBER}(\.[0-9]+)?E-?{NUMBER})

IDENTIFIER [A-Za-z][A-Za-z0-9_]*

COMMENT \/\*[^\*]*(\*+[^\/\*][^\*]*)*\*+\/
CPLUSPLUSCOMMENT \/\/[^\r\n]*

WHITESPACE [ \t\r\n]+

%%
	static int backup_token;
	if (dot_flag) {
		dot_flag = 0;
		return backup_token;
	}

{WHITESPACE}	{ /* Drop whitespaces */ }

{COMMENT}|{CPLUSPLUSCOMMENT}	{ /* Drop comments */ }

	/* Keywords */

action		RETURN(ActionKeyword);
activate	RETURN(ActivateKeyword);
address		RETURN(AddressKeyword);
alive		RETURN_DOT(AliveKeyword);
all		RETURN(AllKeyword);
alt		RETURN(AltKeyword);
altstep		RETURN(AltstepKeyword);
any		RETURN(AnyKeyword);
anytype		RETURN(AnyTypeKeyword);
bitstring	RETURN(BitStringKeyword);
boolean		RETURN(BooleanKeyword);
call		RETURN_DOT(CallOpKeyword);
case		RETURN(CaseKeyword);
catch		RETURN_DOT(CatchOpKeyword);
char		RETURN(CharKeyword);
charstring	RETURN(CharStringKeyword);
check		RETURN_DOT(CheckOpKeyword);
clear		RETURN_DOT(ClearOpKeyword);
complement	RETURN(ComplementKeyword);
component	RETURN(ComponentKeyword);
connect		RETURN(ConnectKeyword);
const		RETURN(ConstKeyword);
control		RETURN(ControlKeyword);
create		RETURN_DOT(CreateKeyword);
default		RETURN(DefaultKeyword);
deactivate	RETURN(DeactivateKeyword);
disconnect	RETURN(DisconnectKeyword);
display		RETURN(DisplayKeyword);
do		RETURN(DoKeyword);
done		RETURN_DOT(DoneKeyword);
else		RETURN(ElseKeyword);
encode		RETURN(EncodeKeyword);
enumerated	RETURN(EnumKeyword);
except		RETURN(ExceptKeyword);
exception	RETURN(ExceptionKeyword);
execute		RETURN(ExecuteKeyword);
extends		RETURN(ExtendsKeyword);
extension	RETURN(ExtensionKeyword);
external	RETURN(ExtKeyword);
float		RETURN(FloatKeyword);
for		RETURN(ForKeyword);
from		RETURN(FromKeyword);
function	RETURN(FunctionKeyword);
getcall		RETURN_DOT(GetCallOpKeyword);
getreply	RETURN_DOT(GetReplyOpKeyword);
getverdict	RETURN(GetVerdictKeyword);
goto		RETURN(GotoKeyword);
group		RETURN(GroupKeyword);
hexstring	RETURN(HexStringKeyword);
if		RETURN(IfKeyword);
ifpresent	RETURN(IfPresentKeyword);
import		RETURN(ImportKeyword);
in		RETURN(InParKeyword);
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
modifies	RETURN(ModifiesKeyword);
module		RETURN(TTCN3ModuleKeyword);
modulepar	RETURN(ModuleParKeyword);
mtc		RETURN(MTCKeyword);
noblock		RETURN(NoBlockKeyword);
nowait		RETURN(NowaitKeyword);
objid		RETURN(ObjectIdentifierKeyword);
octetstring	RETURN(OctetStringKeyword);
of		RETURN(OfKeyword);
omit		RETURN(OmitKeyword);
on		RETURN(OnKeyword);
optional	RETURN(OptionalKeyword);
out		RETURN(OutParKeyword);
override	RETURN(OverrideKeyword);
param		RETURN(ParamKeyword);
pattern		RETURN(PatternKeyword);
permutation	RETURN(PermutationKeyword);
port		RETURN(PortKeyword);
procedure	RETURN(ProcedureKeyword);
raise		RETURN_DOT(RaiseKeyword);
read		RETURN_DOT(ReadKeyword);
receive		RETURN_DOT(ReceiveOpKeyword);
record		RETURN(RecordKeyword);
recursive	RETURN(RecursiveKeyword);
reply		RETURN_DOT(ReplyKeyword);
repeat		RETURN(RepeatKeyword);
return		RETURN(ReturnKeyword);
running		RETURN_DOT(RunningKeyword);
runs		RETURN(RunsKeyword);
select		RETURN(SelectKeyword);
self		RETURN(SelfOp);
send		RETURN_DOT(SendOpKeyword);
sender		RETURN(SenderKeyword);
set		RETURN(SetKeyword);
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

	/* Values */

{NUMBER}	RETURN(Number);

{FLOAT}		RETURN(FloatValue);

true|false	RETURN(BooleanConst);

pass|fail|inconc|none|error	RETURN(VerdictConst);

\'{BIN}*\'B	 RETURN(Bstring);

\'{BINORMATCH}*\'B	RETURN(BitStringMatch);

\'{HEX}*\'H		RETURN(Hstring);

\'{HEXORMATCH}*\'H	RETURN(HexStringMatch);

\'{OCT}*\'O	RETURN(Ostring);

\'{OCTORMATCH}*\'O	RETURN(OctetStringMatch);

(\"{CHAR}*\"|"<\">")+	RETURN(Cstring);

null			RETURN(NullKeyword);

	/* Multi-character operators */

":="		RETURN(AssignmentChar);
".."		RETURN(DotDot);
"->"		RETURN(PortRedirectSymbol);
"=="		RETURN(EQ);
"!="		RETURN(NE);
">="		RETURN(GE);
"<="		RETURN(LE);

"<<"		RETURN(SL);
">>"		RETURN(SR);
"<@"		RETURN(RL);
"@>"		RETURN(RR);

mod		RETURN(Mod);
rem		RETURN(Rem);
or		RETURN(Or);
or4b		RETURN(Or4b);
xor		RETURN(Xor);
xor4b		RETURN(Xor4b);
and		RETURN(And);
and4b		RETURN(And4b);
not		RETURN(Not);
not4b		RETURN(Not4b);

	/* Identifiers */

{IDENTIFIER}	RETURN(Identifier);

	/* Single character literals */

\.		if (dot_flag) {
			backup_token = '.';
			return '.';
		} else dot_flag = 1;

.		return yytext[0];

<<EOF>>		if (dot_flag) {
			dot_flag = 0;
			return '.';
		} else return EOF;

%%

