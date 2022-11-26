/**
 * common front-end functions
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 20-dec-19
 * @license: see 'LICENSE.GPL' file
 */

#ifndef __MCALC_MAIN_H__
#define __MCALC_MAIN_H__


#include "context.h"
#include <cstdint>


/**
 * registers external runtime functions which should be available to the compiler
 */
template<class t_real, class t_int>
void add_ext_funcs(ParserContext& ctx, bool skip_some = false)
{
	// real functions
	if constexpr(std::is_same_v<std::decay_t<t_real>, float>)
	{
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "pow", "powf",
			SymbolType::SCALAR, {SymbolType::SCALAR, SymbolType::SCALAR});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "exp", "expf",
			SymbolType::SCALAR, {SymbolType::SCALAR});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "sin", "sinf",
			SymbolType::SCALAR, {SymbolType::SCALAR});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "cos", "cosf",
			SymbolType::SCALAR, {SymbolType::SCALAR});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "sqrt", "sqrtf",
			SymbolType::SCALAR, {SymbolType::SCALAR});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "fabs", "fabsf",
			SymbolType::SCALAR, {SymbolType::SCALAR});
	}
	else if constexpr(std::is_same_v<std::decay_t<t_real>, double>)
	{
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "pow", "pow",
			SymbolType::SCALAR, {SymbolType::SCALAR, SymbolType::SCALAR});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "exp", "exp",
			SymbolType::SCALAR, {SymbolType::SCALAR});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "sin", "sin",
			SymbolType::SCALAR, {SymbolType::SCALAR});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "cos", "cos",
			SymbolType::SCALAR, {SymbolType::SCALAR});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "sqrt", "sqrt",
			SymbolType::SCALAR, {SymbolType::SCALAR});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "fabs", "fabs",
			SymbolType::SCALAR, {SymbolType::SCALAR});
	}
	else if constexpr(std::is_same_v<std::decay_t<t_real>, long double>)
	{
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "pow", "powl",
			SymbolType::SCALAR, {SymbolType::SCALAR, SymbolType::SCALAR});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "exp", "expl",
			SymbolType::SCALAR, {SymbolType::SCALAR});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "sin", "sinl",
			SymbolType::SCALAR, {SymbolType::SCALAR});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "cos", "cosl",
			SymbolType::SCALAR, {SymbolType::SCALAR});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "sqrt", "sqrtl",
			SymbolType::SCALAR, {SymbolType::SCALAR});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "fabs", "fabsl",
			SymbolType::SCALAR, {SymbolType::SCALAR});
	}

	// int functions
	if constexpr(std::is_same_v<std::decay_t<t_real>, std::int32_t>)
	{
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "abs", "abs",
			SymbolType::INT, {SymbolType::INT});
	}
	else if constexpr(std::is_same_v<std::decay_t<t_real>, std::int64_t>)
	{
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "abs", "labs",
			SymbolType::INT, {SymbolType::INT});
	}

	ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "strlen", "strlen",
		SymbolType::INT, {SymbolType::STRING});

	ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "set_debug", "set_debug",
		SymbolType::VOID, {SymbolType::INT});

	ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "set_eps", "set_eps",
		SymbolType::VOID, {SymbolType::SCALAR});
	ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "get_eps", "get_eps",
		SymbolType::SCALAR, {});

	// functions that could also be declared as internals (e.g. in 3ac module)
	if(!skip_some)
	{
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "putstr", "putstr",
			SymbolType::VOID, {SymbolType::STRING});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "putflt", "putflt",
			SymbolType::VOID, {SymbolType::SCALAR});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "putint", "putint",
			SymbolType::VOID, {SymbolType::INT});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "getflt", "getflt",
			SymbolType::SCALAR, {SymbolType::STRING});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "getint", "getint",
			SymbolType::INT, {SymbolType::STRING});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "flt_to_str", "flt_to_str",
			SymbolType::VOID, {SymbolType::SCALAR, SymbolType::STRING, SymbolType::INT});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "int_to_str", "int_to_str",
			SymbolType::VOID, {SymbolType::INT, SymbolType::STRING, SymbolType::INT});
	}
}


#endif
