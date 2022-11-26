/**
 * lexer
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 7-mar-2020, 26-nov-2022
 * @license see 'LICENSE' file
 */

#ifndef __MCALC_LEXER_H__
#define __MCALC_LEXER_H__


#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <optional>

#include "lval.h"
#include "lalr1/common.h"


using t_tok = t_symbol_id;

// [ token, lvalue, line number ]
using t_lexer_match = std::tuple<t_tok, t_lval, std::size_t>;


enum class Token : t_tok
{
	// tokens with an lvalue
	REAL_CONST  = 1000,
	INT_CONST   = 1001,
	STR_CONST   = 1002,
	IDENT       = 1003,

	// types
	SCALAR      = 2000,
	VEC         = 2001,
	MAT         = 2002,
	INT         = 2003,
	STR         = 2004,

	// logical operators
	EQU         = 3000,
	NEQU        = 3001,
	GEQU        = 3002,
	LEQU        = 3003,
	AND         = 3004,
	OR          = 3005,
	XOR         = 3006,
	NOT         = 3007,

	ASSIGN      = 4000,
	RANGE       = 4001,

	// conditionals
	IF          = 5000,
	THEN        = 5001,
	ELSE        = 5002,

	// loops
	LOOP        = 6000,
	DO          = 6001,
	BREAK       = 6002,
	NEXT        = 6003,

	// functions
	FUNC        = 7000,
	RET         = 7001,

	END         = END_IDENT,
};


class Lexer
{
public:
	Lexer(std::istream* = &std::cin);

	// get all tokens and attributes
	std::vector<t_toknode> GetAllTokens();

	void SetTermIdxMap(const t_mapIdIdx* map) { m_mapTermIdx = map; }


protected:
	// get next token and attribute
	t_lexer_match GetNextToken(std::size_t* line = nullptr);

	// find all matching tokens for input string
	std::vector<t_lexer_match> GetMatchingTokens(
		const std::string& str, std::size_t line);


private:
	std::istream* m_istr{nullptr};
	const t_mapIdIdx* m_mapTermIdx{nullptr};
};


#endif
