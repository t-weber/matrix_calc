/**
 * parser
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 27-may-18
 * @license: see 'LICENSE.GPL' file
 */

#ifndef __PARSER_CONTEXT_H__
#define __PARSER_CONTEXT_H__

#include <iostream>
#include <string>
#include <array>
#include <variant>
#include <unordered_map>
#include <cmath>

#include "ast/ast.h"


/**
 * holds the common lexer state
 */
class LexerContext
{
protected:
	std::size_t m_curline = 1;

public:
	LexerContext() = default;
	virtual ~LexerContext() = default;

	LexerContext(const LexerContext&) = delete;
	const LexerContext& operator=(const LexerContext&) = delete;

	void IncCurLine() { ++m_curline; }
	std::size_t GetCurLine() const { return m_curline; }
};


/**
 * holds the common parser state
 */
class ParserContext
{
protected:
	std::shared_ptr<ASTStmts> m_statements{};

	SymTab m_symbols{};
	std::unordered_map<t_str, std::variant<t_real, t_int, t_str>> m_consts
	{{
		{"pi", t_real(M_PI)},
	}};

	// information about currently parsed symbol
	std::vector<t_str> m_curscope{};
	SymbolType m_symtype = SymbolType::SCALAR;
	std::array<std::size_t, 2> m_symdims = {1, 1};


public:
	ParserContext() = default;
	virtual ~ParserContext() = default;

	virtual std::size_t GetCurLine() const = 0;


	// --------------------------------------------------------------------
	void SetStatements(std::shared_ptr<ASTStmts> stmts) { m_statements = stmts; }
	const std::shared_ptr<ASTStmts> GetStatements() const { return m_statements; }
	// --------------------------------------------------------------------


	// --------------------------------------------------------------------
	/**
	 * current function scope
	 */
	const std::vector<t_str>& GetScope() const
	{
		return m_curscope;
	}

	/**
	 * get the currently active scope name, ignoring the last "up" levels
	 */
	t_str GetScopeName(std::size_t up=0) const
	{
		t_str name;
		for(std::size_t level=0; level<m_curscope.size()-up; ++level)
			name += m_curscope[level] + Symbol::get_scopenameseparator();
		return name;
	}

	void EnterScope(const t_str& name)
	{
		m_curscope.push_back(name);
	}

	void LeaveScope(const t_str& name)
	{
		const t_str& curscope = *m_curscope.rbegin();

		if(curscope != name)
		{
			std::cerr << "Error in line " << GetCurLine()
				<< ": Trying to leave scope " << name
				<< ", but the top scope is " <<curscope << ".";
		}

		m_curscope.pop_back();
	}
	// --------------------------------------------------------------------


	// --------------------------------------------------------------------
	Symbol* AddScopedSymbol(const t_str& name)
	{
		const t_str& scope = GetScopeName();
		return m_symbols.AddSymbol(scope, name, m_symtype, m_symdims);
	}

	const Symbol* FindScopedSymbol(const t_str& name) const
	{
		const t_str& scope = GetScopeName();
		return m_symbols.FindSymbol(scope + name);
	}

	const SymTab& GetSymbols() const { return m_symbols; }
	SymTab& GetSymbols() { return m_symbols; }

	// type of current symbol
	void SetSymType(SymbolType ty) { m_symtype = ty; }

	// dimensions of vector and matrix symbols
	void SetSymDims(std::size_t dim1, std::size_t dim2=1)
	{
		m_symdims[0] = dim1;
		m_symdims[1] = dim2;
	}


	std::pair<bool, std::variant<t_real, t_int, t_str>>
	GetConst(const t_str& name) const
	{
		auto iter = m_consts.find(name);
		if(iter == m_consts.end())
			return std::make_pair(false, t_real(0.));

		return std::make_pair(true, iter->second);
	}
	// --------------------------------------------------------------------
};


#endif
