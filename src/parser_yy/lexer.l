/**
 * lexer
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 09-dec-19
 * @license see 'LICENSE.GPL' file
 *
 * References:
 *	https://github.com/westes/flex/tree/master/examples/manual
 *	http://www.gnu.org/software/bison/manual/html_node/index.html
 *	http://git.savannah.gnu.org/cgit/bison.git/tree/examples
 */

%option c++
%option noyywrap
%option yyclass = "yy::Lexer"


%{
	#include <string>
	#include <sstream>
	#include <iomanip>

	#include "parser_yy/parser.h"
%}


white	[ \t\r]

num_bin	[01]
num_oct	[0-7]
num_dec	[0-9]
num_hex	[0-9a-fA-F]

int_bin "0b"{num_bin}+
int_dec {num_dec}+
int_hex "0x"{num_hex}+

real	{num_dec}+(\.{num_dec}*)?
realeng	{real}([Ee][+-]?{num_dec}+)?

ident	[A-Za-z_]+[A-Za-z0-9_]*


%%


"\n"		{ IncCurLine(); }
{white}+	/* whitespace */
"#".*		/* comment */

";"			{ return yytext[0]; }
"+"|"-"		{ return yytext[0]; }
"*"|"/"|"%"	{ return yytext[0]; }
"^"|"'"		{ return yytext[0]; }
"("|")"		{ return yytext[0]; }
"{"|"}"		{ return yytext[0]; }
"["|"]"		{ return yytext[0]; }
"|"|"|"		{ return yytext[0]; }
","			{ return yytext[0]; }

"~"			{ return yy::Parser::make_RANGE(); }
"=="		{ return yy::Parser::make_EQU(); }
"<>"		{ return yy::Parser::make_NEQ(); }
">="		{ return yy::Parser::make_GEQ(); }
"<="		{ return yy::Parser::make_LEQ(); }
">"			{ return yy::Parser::make_GT(); }
"<"			{ return yy::Parser::make_LT(); }
"and"		{ return yy::Parser::make_AND(); }
"xor"		{ return yy::Parser::make_XOR(); }
"or"		{ return yy::Parser::make_OR(); }
"not"		{ return yy::Parser::make_NOT(); }

"assign"	{ return yy::Parser::make_ASSIGN(); }
"="			{ return yytext[0]; }

"scalar"|"var"	{ return yy::Parser::make_SCALARDECL(); }
"vec"		{ return yy::Parser::make_VECTORDECL(); }
"mat"		{ return yy::Parser::make_MATRIXDECL(); }
"str"		{ return yy::Parser::make_STRINGDECL(); }
"int"		{ return yy::Parser::make_INTDECL(); }

"func"		{ return yy::Parser::make_FUNC(); }
"ret"		{ return yy::Parser::make_RET(); }

"if"		{ return yy::Parser::make_IF(); }
"then"		{ return yy::Parser::make_THEN(); }
"else"		{ return yy::Parser::make_ELSE(); }

"loop"		{ return yy::Parser::make_LOOP(); }
"break"		{ return yy::Parser::make_BREAK(); }
"next"		{ return yy::Parser::make_NEXT(); }
"do"		{ return yy::Parser::make_DO(); }


{int_bin}	{
	t_int dec = 0;

	std::size_t pos = 0;
	for(const char* pc=yytext+yyleng-1; pc>=yytext+2; --pc)
	{
		if(*pc != '0')
			dec += (1<<pos);
		++pos;
	}

	return yy::Parser::make_INT(static_cast<t_int>(dec));
}

{int_dec}	{
	return yy::Parser::make_INT(static_cast<t_int>(std::stol(yytext)));
}

{int_hex}	{
	t_int l = 0;
	std::istringstream{yytext} >> std::hex >> l;
	return yy::Parser::make_INT(static_cast<t_int>(l));
}

{realeng}	{
	return yy::Parser::make_REAL(static_cast<t_real>(std::stod(yytext)));
}


{ident}		{
	return yy::Parser::make_IDENT(yytext);
}


"\""[^\"]*"\""	{
	// string
	t_str str{yytext+1, yytext+yyleng-1};
	return yy::Parser::make_STRING(str); }


. {
	t_str err = "Unknown token: \"";
	err += yytext;
	err += "\".";

	LexerError(err.c_str());
}


%%
