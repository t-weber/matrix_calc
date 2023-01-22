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
#include <vector>
#include <optional>
#include <iostream>

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
	std::vector<SymbolType> argty{};
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

	// get the corresponding data type name
	static t_str get_type_name(SymbolType ty);

	static const t_str& get_scopenameseparator();
};



/**
 * symbol table
 */
class SymTab
{
public:
	Symbol* AddSymbol(const t_str& scope,
		const t_str& name, SymbolType ty,
		const std::array<std::size_t, 2>& dims,
		bool is_temp = false);

	Symbol* AddFunc(const t_str& scope,
		const t_str& name, SymbolType retty,
		const std::vector<SymbolType>& argtypes,
		const std::array<std::size_t, 2>* retdims = nullptr,
		const std::vector<SymbolType>* multirettypes = nullptr,
		bool is_external = false);

	Symbol* AddExtFunc(const t_str& scope,
		const t_str& name, const t_str& extfunc_name,
		SymbolType retty,
		const std::vector<SymbolType>& argtypes,
		const std::array<std::size_t, 2>* retdims = nullptr,
		const std::vector<SymbolType>* multirettypes = nullptr);

	Symbol* FindSymbol(const t_str& name);

	const Symbol* FindSymbol(const t_str& name) const;

	std::vector<const Symbol*> FindSymbolsWithSameScope(
		const t_str& scope, bool no_args = true) const;

	const std::unordered_map<t_str, Symbol>& GetSymbols() const;

	friend std::ostream& operator<<(std::ostream& ostr, const SymTab& tab);

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
