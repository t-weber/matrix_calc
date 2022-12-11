/**
 * matrix expression grammar
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date dec-2022
 * @license see 'LICENSE' file
 */

#ifndef __MCALC_GRAMMAR_H__
#define __MCALC_GRAMMAR_H__

#include "lalr1/types.h"
#include "lalr1/symbol.h"
#include "lalr1/ast.h"
#include "ast/ast.h"
#include "common/context.h"


/**
 * non-terminals identifiers
 */
enum : std::size_t
{
	START,

	EXPRESSION,
	EXPRESSIONS,

	STATEMENT,
	STATEMENTS,

	VARIABLES,
	FULL_ARGUMENTLIST,
	ARGUMENTLIST,
	IDENTLIST,
	TYPELIST,

	BLOCK,
	FUNCTION,
	TYPEDECL,
	OPT_ASSIGN,
};


class MatrixCalcGrammar
{
public:
	void CreateGrammar();

#ifdef CREATE_PRODUCTION_RULES
	template<template<class...> class t_cont = std::vector>
	t_cont<lalr1::NonTerminalPtr> GetAllNonTerminals() const
	{
		return t_cont<lalr1::NonTerminalPtr>{{
			start,
			expression, expressions,
			statement, statements,
			variables,
			full_argumentlist, argumentlist,
			identlist, typelist,
			block, function,
			typedecl,
			opt_assign,
		}};
	}

	const lalr1::NonTerminalPtr& GetStartNonTerminal() const { return start; }
#endif

#ifdef CREATE_SEMANTIC_RULES
	const lalr1::t_semanticrules& GetSemanticRules() const { return rules; }
#endif

	const ParserContext& GetContext() const { return m_context; }
	ParserContext& GetContext() { return m_context; }


private:
#ifdef CREATE_PRODUCTION_RULES
	// non-terminals
	lalr1::NonTerminalPtr start{},
		expressions{}, expression{},
		statements{}, statement{},
		variables{},
		full_argumentlist{}, argumentlist{},
		identlist{}, typelist{},
		block{}, function{},
		typedecl{},
		opt_assign{};

	// terminals
	lalr1::TerminalPtr op_assign{}, op_plus{}, op_minus{},
		op_mult{}, op_div{}, op_mod{}, op_pow{},
		op_norm{}, op_trans{};
	lalr1::TerminalPtr op_and{}, op_or{}, op_not{}, op_xor{},
		op_equ{}, op_neq{},
		op_lt{}, op_gt{}, op_geq{}, op_leq{};
	lalr1::TerminalPtr bracket_open{}, bracket_close{};
	lalr1::TerminalPtr block_begin{}, block_end{};
	lalr1::TerminalPtr array_begin{}, array_end{}, range{};
	lalr1::TerminalPtr keyword_if{}, keyword_then{}, keyword_else{};
	lalr1::TerminalPtr keyword_loop{}, keyword_do{},
		keyword_break{}, keyword_next{};
	lalr1::TerminalPtr keyword_func{}, keyword_ret{};
	lalr1::TerminalPtr keyword_assign{};
	lalr1::TerminalPtr comma{}, stmt_end{};
	lalr1::TerminalPtr sym_real{}, sym_int{}, sym_str{}, ident{};
	lalr1::TerminalPtr real_decl{}, vec_decl{}, mat_decl{},
		int_decl{}, str_decl{};
#endif

	ParserContext m_context{};

#ifdef CREATE_SEMANTIC_RULES
	// semantic rules
	lalr1::t_semanticrules rules{};
#endif
};

#endif
