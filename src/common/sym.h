/**
 * symbol table
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 13-apr-20
 * @license: see 'LICENSE.GPL' file
 */

#ifndef __SYMTAB_H__
#define __SYMTAB_H__

#include <memory>
#include <string>
#include <unordered_map>
#include <array>
#include <optional>
#include <iostream>
#include <iomanip>

#include "types.h"



struct Symbol;
using SymbolPtr = std::shared_ptr<Symbol>;



enum class SymbolType
{
	VOID,

	SCALAR,
	INT,
	STRING,

	VECTOR,
	MATRIX,

	COMP,     // compound
	FUNC,     // function pointer

	UNKNOWN,
};



struct Symbol
{
	t_str name{};                     // local symbol identifier
	t_str scoped_name{};              // full identifier with scope prefixes
	t_str scope_name{};               // scope prefixes
	std::optional<t_str> ext_name{};  // name of external symbol (if different from internal name)

	SymbolType ty { SymbolType::VOID };
	std::array<std::size_t, 2> dims{{ 1, 1 }};

	// for functions
	std::vector<SymbolType> argty{{}};
	SymbolType retty = SymbolType::VOID;
	std::array<std::size_t, 2> retdims{{1,1}};

	// for compound type
	std::vector<SymbolPtr> elems{};

	bool is_tmp = false;              // temporary or declared variable?
	bool is_external = false;         // link to external variable or function?
	bool is_arg = false;              // symbol is a function argument
	std::optional<t_int> addr{};      // optional address of variable
	std::size_t argidx = 0;           // optional argument index

	mutable std::size_t refcnt = 0;   // number of reference to this symbol


	/**
	 * get the corresponding data type name
	 */
	static t_str get_type_name(SymbolType ty)
	{
		switch(ty)
		{
			case SymbolType::SCALAR: return "scalar";
			case SymbolType::VECTOR: return "vec";
			case SymbolType::MATRIX: return "mat";
			case SymbolType::STRING: return "str";
			case SymbolType::INT: return "int";
			case SymbolType::VOID: return "void";
			case SymbolType::COMP: return "comp";
			case SymbolType::FUNC: return "func";

			case SymbolType::UNKNOWN: return "unknown";
		}

		return "invalid";
	}


	static const t_str& get_scopenameseparator()
	{
		static const t_str sep{"::"};
		return sep;
	}
};



class SymTab
{
public:
	Symbol* AddSymbol(const t_str& scope,
		const t_str& name, SymbolType ty,
		const std::array<std::size_t, 2>& dims,
		bool is_temp = false)
	{
		Symbol sym{.name = name,
			.scoped_name = scope + name,
			.scope_name = scope,
			.ty = ty, .dims = dims, .elems = {},
			.is_tmp = is_temp, .refcnt = 0};

		auto pair = m_syms.insert_or_assign(sym.scoped_name, sym);
		return &pair.first->second;
	}


	Symbol* AddFunc(const t_str& scope,
		const t_str& name, SymbolType retty,
		const std::vector<SymbolType>& argtypes,
		const std::array<std::size_t, 2>* retdims = nullptr,
		const std::vector<SymbolType>* multirettypes = nullptr,
		bool is_external = false)
	{
		Symbol sym{.name = name,
			.scoped_name = scope + name,
			.scope_name = scope,
			.ty = SymbolType::FUNC,
			.argty = argtypes, .retty = retty,
			.is_external = is_external,
			.refcnt = 0};

		if(retdims)
			sym.retdims = *retdims;

		if(multirettypes)
		{
			for(SymbolType ty : *multirettypes)
			{
				auto retsym = std::make_shared<Symbol>();
				retsym->ty = ty;
				sym.elems.emplace_back(retsym);
			}
		}

		auto pair = m_syms.insert_or_assign(sym.scoped_name, sym);
		return &pair.first->second;
	}


	Symbol* AddExtFunc(const t_str& scope,
		const t_str& name, const t_str& extfunc_name,
		SymbolType retty,
		const std::vector<SymbolType>& argtypes,
		const std::array<std::size_t, 2>* retdims = nullptr,
		const std::vector<SymbolType>* multirettypes = nullptr)
	{
		Symbol *sym = AddFunc(scope, name, retty, argtypes, retdims, multirettypes, true);
		sym->ext_name = extfunc_name;
		return sym;
	}


	Symbol* FindSymbol(const t_str& name)
	{
		auto iter = m_syms.find(name);
		if(iter == m_syms.end())
			return nullptr;
		return &iter->second;
	}


	const Symbol* FindSymbol(const t_str& name) const
	{
		return const_cast<SymTab*>(this)->FindSymbol(name);
	}


	std::vector<const Symbol*> FindSymbolsWithSameScope(
		const t_str& scope, bool no_args = true) const
	{
		std::vector<const Symbol*> syms;

		for(const auto& [name, sym] : m_syms)
		{
			//if(sym.addr)
			//	std::cout << name << ": " << *sym.addr << std::endl;

			if(no_args && sym.is_arg)
				continue;

			if(sym.scope_name == scope)
				syms.push_back(&sym);
		}

		return syms;
	}


	const std::unordered_map<t_str, Symbol>& GetSymbols() const
	{
		return m_syms;
	}


	friend std::ostream& operator<<(std::ostream& ostr, const SymTab& tab)
	{
		const int name_len = 32;
		const int type_len = 18;
		const int refs_len = 8;
		const int dims_len = 8;
		//const int addr_len = 8;

		ostr << std::left << std::setw(name_len) << "full name"
			<< std::left << std::setw(type_len) << "type"
			<< std::left << std::setw(refs_len) << "refs"
			<< std::left << std::setw(dims_len) << "dim1"
			<< std::left << std::setw(dims_len) << "dim2"
			//<< std::left << std::setw(addr_len) << "addr"
			<< "\n";
		ostr << "--------------------------------------------------------------------------------\n";

		for(const auto& pair : tab.m_syms)
		{
			const Symbol& sym = pair.second;

			std::string ty = Symbol::get_type_name(sym.ty);
			if(sym.is_external)
				ty += " (ext)";
			if(sym.is_arg)
				ty += " (arg " + std::to_string(sym.argidx) + ")";
			if(sym.is_tmp)
				ty += " (tmp)";

			/*std::string addr = "-";
			if(sym.addr)
				addr = std::to_string(*sym.addr);*/

			ostr << std::left << std::setw(name_len) << pair.first
				<< std::left << std::setw(type_len) << ty
				<< std::left << std::setw(refs_len) << sym.refcnt
				<< std::left << std::setw(dims_len) << std::get<0>(sym.dims)
				<< std::left << std::setw(dims_len) << std::get<1>(sym.dims)
				//<< std::left << std::setw(addr_len) << addr
				<< "\n";
		}

		return ostr;
	}


private:
	std::unordered_map<t_str, Symbol> m_syms{};
};



// ----------------------------------------------------------------------------
// helper functions
// ----------------------------------------------------------------------------
/**
 * multiplies the elements of a container
 */
template<class t_cont, std::size_t ...seq>
constexpr typename t_cont::value_type multiply_elements(
	const t_cont& cont, const std::index_sequence<seq...>&)
{
	return (std::get<seq>(cont) * ...);
}



/**
 * multiplies all dimensions of an array type
 */
template<std::size_t NUM_DIMS = 2>
std::size_t get_arraydim(const std::array<std::size_t, NUM_DIMS>& dims)
{
	return multiply_elements(dims, std::make_index_sequence<NUM_DIMS>());
}
// ----------------------------------------------------------------------------


#endif