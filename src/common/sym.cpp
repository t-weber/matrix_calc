/**
 * symbol table
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 13-apr-20
 * @license: see 'LICENSE.GPL' file
 */

#include "sym.h"

#include <iomanip>



/**
 * get the corresponding data type name
 */
t_str Symbol::get_type_name(SymbolType ty)
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


const t_str& Symbol::get_scopenameseparator()
{
	static const t_str sep{"::"};
	return sep;
}


Symbol* SymTab::AddSymbol(const t_str& scope,
	const t_str& name, SymbolType ty,
	const std::array<std::size_t, 2>& dims,
	bool is_temp)
{
	Symbol sym{.name = name,
		.scoped_name = scope + name,
		.scope_name = scope,
		.ty = ty, .dims = dims, .elems = {},
		.is_tmp = is_temp, .refcnt = 0};

	auto pair = m_syms.insert_or_assign(sym.scoped_name, sym);
	return &pair.first->second;
}


Symbol* SymTab::AddFunc(const t_str& scope,
	const t_str& name, SymbolType retty,
	const std::vector<SymbolType>& argtypes,
	const std::array<std::size_t, 2>* retdims,
	const std::vector<SymbolType>* multirettypes,
	bool is_external)
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


Symbol* SymTab::AddExtFunc(const t_str& scope,
	const t_str& name, const t_str& extfunc_name,
	SymbolType retty,
	const std::vector<SymbolType>& argtypes,
	const std::array<std::size_t, 2>* retdims,
	const std::vector<SymbolType>* multirettypes)
{
	Symbol *sym = AddFunc(scope, name, retty, argtypes, retdims, multirettypes, true);
	sym->ext_name = extfunc_name;
	return sym;
}


Symbol* SymTab::FindSymbol(const t_str& name)
{
	auto iter = m_syms.find(name);
	if(iter == m_syms.end())
		return nullptr;
	return &iter->second;
}


const Symbol* SymTab::FindSymbol(const t_str& name) const
{
	return const_cast<SymTab*>(this)->FindSymbol(name);
}


std::vector<const Symbol*> SymTab::FindSymbolsWithSameScope(
	const t_str& scope, bool no_args) const
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


const std::unordered_map<t_str, Symbol>& SymTab::GetSymbols() const
{
	return m_syms;
}


std::ostream& operator<<(std::ostream& ostr, const SymTab& tab)
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
