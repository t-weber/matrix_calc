/**
 * symbol table
 * @author Tobias Weber
 * @date 13-apr-20
 * @license: see 'LICENSE.GPL' file
 */

#ifndef __SYMTAB_H__
#define __SYMTAB_H__

#include <memory>
#include <string>
#include <unordered_map>
#include <array>
#include <iostream>


struct Symbol;
using SymbolPtr = std::shared_ptr<Symbol>;


enum class SymbolType
{
	SCALAR,
	VECTOR,
	MATRIX,
	STRING,
	INT,
	VOID,

	COMP,	// compound
	FUNC,	// function pointer

	UNKNOWN,
};


struct Symbol
{
	std::string name;
	SymbolType ty = SymbolType::VOID;
	std::array<std::size_t, 2> dims{{0,0}};

	// for functions
	std::vector<SymbolType> argty{{}};
	SymbolType retty = SymbolType::VOID;
	std::array<std::size_t, 2> retdims{{0,0}};

	// for compound type
	//const Symbol *memblock = nullptr;
	std::vector<SymbolPtr> elems{};

	bool tmp = false;		// temporary or declared variable?
	bool on_heap = false;	// heap or stack variable?


	/**
	 * get the corresponding data type name
	 */
	static std::string get_type_name(SymbolType ty)
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
};


class SymTab
{
public:
	const Symbol* AddSymbol(const std::string& name_with_scope,
		const std::string& name, SymbolType ty,
		const std::array<std::size_t, 2>& dims,
		bool is_temp = false, bool on_heap = false)
	{
		Symbol sym{.name = name, .ty = ty, .dims = dims,
			/*.memblock = nullptr,*/ .elems = {}, 
			.tmp = is_temp, .on_heap = on_heap};
		auto pair = m_syms.insert_or_assign(name_with_scope, sym);
		return &pair.first->second;
	}


	const Symbol* AddFunc(const std::string& name_with_scope,
		const std::string& name, SymbolType retty,
		const std::vector<SymbolType>& argtypes,
		const std::array<std::size_t, 2>* retdims = nullptr,
		const std::vector<SymbolType>* multirettypes = nullptr)
	{
		Symbol sym{.name = name, .ty = SymbolType::FUNC, .argty = argtypes, .retty = retty};
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
		auto pair = m_syms.insert_or_assign(name_with_scope, sym);
		return &pair.first->second;
	}


	const Symbol* FindSymbol(const std::string& name) const
	{
		auto iter = m_syms.find(name);
		if(iter == m_syms.end())
			return nullptr;
		return &iter->second;
	}


	friend std::ostream& operator<<(std::ostream& ostr, const SymTab& tab)
	{
		for(const auto& pair : tab.m_syms)
			ostr << pair.first << " -> " << pair.second.name << "\n";

		return ostr;
	}


private:
	std::unordered_map<std::string, Symbol> m_syms;
};


#endif
