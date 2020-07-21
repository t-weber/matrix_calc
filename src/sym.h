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
#include <iomanip>


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
	std::string name{};
	std::string scoped_name{};

	SymbolType ty = SymbolType::VOID;
	std::array<std::size_t, 2> dims{{1,1}};

	// for functions
	std::vector<SymbolType> argty{{}};
	SymbolType retty = SymbolType::VOID;
	std::array<std::size_t, 2> retdims{{1,1}};

	// for compound type
	//const Symbol *memblock = nullptr;
	std::vector<SymbolPtr> elems{};

	bool tmp = false;		// temporary or declared variable?
	bool on_heap = false;	// heap or stack variable?
	bool is_external = false;	// link to external variable?

	mutable std::size_t refcnt = 0;	// number of reference to this symbol


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


	static const std::string& get_scopenameseparator()
	{
		static const std::string sep{"::"};
		return sep;
	}
};


class SymTab
{
public:
	Symbol* AddSymbol(const std::string& scope,
		const std::string& name, SymbolType ty,
		const std::array<std::size_t, 2>& dims,
		bool is_temp = false, bool on_heap = false)
	{
		Symbol sym{.name = name, .scoped_name = scope+name, 
			.ty = ty, .dims = dims,
			/*.memblock = nullptr,*/ .elems = {}, 
			.tmp = is_temp, .on_heap = on_heap, .refcnt = 0};
		auto pair = m_syms.insert_or_assign(scope+name, sym);
		return &pair.first->second;
	}


	Symbol* AddFunc(const std::string& scope,
		const std::string& name, SymbolType retty,
		const std::vector<SymbolType>& argtypes,
		const std::array<std::size_t, 2>* retdims = nullptr,
		const std::vector<SymbolType>* multirettypes = nullptr,
		bool is_external = false)
	{
		Symbol sym{.name = name, .scoped_name = scope+name,
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
		auto pair = m_syms.insert_or_assign(scope+name, sym);
		return &pair.first->second;
	}


	Symbol* AddExtFunc(const std::string& scope,
		const std::string& name, SymbolType retty,
		const std::vector<SymbolType>& argtypes,
		const std::array<std::size_t, 2>* retdims = nullptr,
		const std::vector<SymbolType>* multirettypes = nullptr)
	{
		return AddFunc(scope, name, retty, argtypes, retdims, multirettypes, true);
	}


	const Symbol* FindSymbol(const std::string& name) const
	{
		auto iter = m_syms.find(name);
		if(iter == m_syms.end())
			return nullptr;
		return &iter->second;
	}


	const std::unordered_map<std::string, Symbol>& GetSymbols() const
	{
		return m_syms;
	}


	friend std::ostream& operator<<(std::ostream& ostr, const SymTab& tab)
	{
		ostr << std::left << std::setw(32) << "full name" 
			<< std::left << std::setw(16) << "type" 
			<< std::left << std::setw(8) << "refs" 
			<< std::left << std::setw(8) << "dim1"
			<< std::left << std::setw(8) << "dim2"
			<< "\n";
		ostr << "--------------------------------------------------------------------------------\n";
		for(const auto& pair : tab.m_syms)
			ostr << std::left << std::setw(32) << pair.first 
				<< std::left << std::setw(16) << Symbol::get_type_name(pair.second.ty) 
				<< std::left << std::setw(8) << pair.second.refcnt 
				<< std::left << std::setw(8) << std::get<0>(pair.second.dims) 
				<< std::left << std::setw(8) << std::get<1>(pair.second.dims)
				<< "\n";

		return ostr;
	}


private:
	std::unordered_map<std::string, Symbol> m_syms;
};


#endif
