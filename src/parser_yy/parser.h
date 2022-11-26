/**
 * parser
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 27-may-18
 * @license: see 'LICENSE.GPL' file
 */

#ifndef __PARSER_H__
#define __PARSER_H__

#undef yyFlexLexer

#include <iostream>
#include <FlexLexer.h>

#include "common/context.h"
#include "parser_defs.h"


namespace yy
{
	class YParserContext;

	/**
	 * lexer
	 */
	class Lexer : public ::LexerContext, public yyFlexLexer
	{
	protected:
		ParserContext* m_context = nullptr;

	public:
		Lexer() : yyFlexLexer{std::cin, std::cerr} {}
		Lexer(ParserContext* context, std::istream& istr)
			: yyFlexLexer{istr, std::cerr}, m_context(context) {}
		virtual ~Lexer() = default;

		Lexer(const Lexer&) = delete;
		const Lexer& operator=(const Lexer&) = delete;

		virtual yy::Parser::symbol_type lex();

		virtual void LexerOutput(const char* str, int len) override;
		virtual void LexerError(const char* err) override;

		void IncCurLine() { ++m_curline; }
		std::size_t GetCurLine() const { return m_curline; }

	private:
		virtual int yylex() final { return -1; }
	};


	/**
	 * holds parser state
	 */
	class ParserContext : public ::ParserContext
	{
	private:
		yy::Lexer m_lex{};

	public:
		ParserContext(std::istream& istr = std::cin)
			: m_lex{this, istr}
		{}

		yy::Lexer& GetLexer()
		{
			return m_lex;
		}

		virtual std::size_t GetCurLine() const override
		{
			return m_lex.GetCurLine();
		}
	};
}


// yylex definition for lexer
#undef YY_DECL
#define YY_DECL yy::Parser::symbol_type yy::Lexer::lex()

// yylex function which the parser calls
extern yy::Parser::symbol_type yylex(yy::ParserContext &context);


// stop parsing
#define yyterminate() { return yy::Parser::by_type::kind_type(0); }


#endif
