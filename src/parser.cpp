/**
 * parser
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 20-dec-19
 * @license: see 'LICENSE.GPL' file
 */

#include "ast.h"
#include "parser.h"
#include "printast.h"
#include "semantics.h"


/**
 * Lexer error output
 */
void yy::Lexer::LexerError(const char* err)
{
	std::cerr << "Lexer error in line " << GetCurLine()
		<< ": " << err << "." << std::endl;
}


/**
 * Lexer message
 */
void yy::Lexer::LexerOutput(const char* str, int /*len*/)
{
	std::cerr << "Lexer output (line " << GetCurLine()
		<< "): " << str << "." << std::endl;
}


/**
 * Parser error output
 */
void yy::Parser::error(const t_str& err)
{
	std::cerr << "Parser error in line " << context.GetCurLine()
		<< ": " << err << "." << std::endl;
}


/**
 * call lexer from parser
 */
extern yy::Parser::symbol_type yylex(yy::ParserContext &context)
{
	return context.GetLexer().lex();
}
