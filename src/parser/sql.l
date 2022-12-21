/* Inspired by https://raw.githubusercontent.com/thinkpad20/sql/master/src/lex/sql.l */

%{
#include "sql.tab.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
%}

ID_TEMPLATE      [a-zA-Z][a-zA-Z0-9_]*
INT_TEMPLATE     [0-9]+
FLOAT_TEMPLATE   [0-9]+|([0-9]*\.[0-9]+)([eE][-+]?[0-9]+)?
STRING_TEMPLATE  '(\\\\|\\n|\\r|\\\"|\\0|\\'|[^'\\])*'
DATE_TEMPLATE    '[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]'

%%

use|USE          { return USE; }
create|CREATE    { return CREATE; }
drop|DROP        { return DROP; }
select|SELECT    { return SELECT; }
insert|INSERT    { return INSERT; }
update|UPDATE    { return UPDATE; }
delete|DELETE    { return DELETE; }
show|SHOW        { return SHOW; }
set|SET          { return SET; }
output|OUTPUT    { return OUTPUT; }

database|DATABASE   { return DATABASE; }
table|TABLE         { return TABLE; }
index|INDEX         { return INDEX; }

default|DEFAULT         { return DEFAULT; }
unique|UNIQUE           { return UNIQUE; }
primary|PRIMARY         { return PRIMARY; }
foreign|FOREIGN         { return FOREIGN; }
references|REFERENCES   { return REFERENCES; }
check|CHECK             { return CHECK; }
key|KEY                 { return KEY; }

into|INTO          { return INTO; }
from|FROM          { return FROM; }
where|WHERE        { return WHERE; }
values|VALUES      { return VALUES; }
join|JOIN          { return JOIN; }
inner|INNER        { return INNER; }
outer|OUTER        { return OUTER; }
left|LEFT          { return LEFT; }
right|RIGHT        { return RIGHT; }
full|FULL          { return FULL; }
asc|ASC            { return ASC; }
desc|DESC          { return DESC; }
order|ORDER        { return ORDER; }
by|BY              { return BY; }
in|IN              { return IN; }
on|ON              { return ON; }
as|AS              { return AS; }
distinct|DISTINCT  { return DISTINCT; }
group|GROUP        { return GROUP; }
using|USING        { return USING; }

like|LIKE    { return LIKE; }
is|IS        { return IS; }
or|OR        { return OR; }
and|AND      { return AND; }
not|NOT      { return NOT; }
"!="         { return NEQ; }
"<>"         { return NEQ; }
">="         { return GEQ; }
"<="         { return LEQ; }

min|MIN      { return MIN; }
max|MAX      { return MAX; }
sum|SUM      { return SUM; }
avg|AVG      { return AVG; }
count|COUNT  { return COUNT; }

true|TRUE    { return TRUE; }
false|FALSE  { return FALSE; }
null|NULL    { return NULL_TOKEN; }

int|INT|integer|INTEGER   { return INTEGER; }
double|DOUBLE             { return DOUBLE; }
float|FLOAT               { return FLOAT; }
char|CHAR                 { return CHAR; }
date|DATE                 { return DATE; }
varchar|VARCHAR           { return VARCHAR; }

exit|EXIT                 { return EXIT; }

{ID_TEMPLATE}             { yylval.val_s = strdup(yytext); return IDENTIFIER; }
{INT_TEMPLATE}            { yylval.val_i = atoi(yytext);   return INT_LITERAL; }
{FLOAT_TEMPLATE}          { yylval.val_f = atof(yytext);   return FLOAT_LITERAL; }
{DATE_TEMPLATE}           { yylval.val_s = strndup(yytext + 1, strlen(yytext) - 2);
                            return DATE_LITERAL; }
{STRING_TEMPLATE}         { yylval.val_s = strndup(yytext + 1, strlen(yytext) - 2);
                            return STRING_LITERAL; }

[ \t\r\n]+                { /* empty */ }
.                         { return yytext[0]; }

%%
