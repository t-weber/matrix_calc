/**
 * llvm three-address code generator
 * @author Tobias Weber
 * @date apr/may-2020
 * @license: see 'LICENSE.GPL' file
 *
 * References:
 *	* https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl03.html
 *	* https://llvm.org/docs/GettingStarted.html
 *	* https://llvm.org/docs/LangRef.html
 */

#include "llasm.h"
#include <sstream>


LLAsm::LLAsm(SymTab* syms, std::ostream* ostr) : m_syms{syms}, m_ostr{ostr}
{}


t_astret LLAsm::visit(const ASTStmts* ast)
{
	t_astret lastres = nullptr;

	for(const auto& stmt : ast->GetStatementList())
		lastres = stmt->accept(this);

	return lastres;
}


t_astret LLAsm::visit(const ASTCond* ast)
{
	generate_cond([this, ast]() -> t_astret
	{
		return ast->GetCond()->accept(this);
	}, [this, ast]
	{
		ast->GetIf()->accept(this);
	}, [this, ast]
	{
		ast->GetElse()->accept(this);
	}, ast->HasElse());

	return nullptr;
}


t_astret LLAsm::visit(const ASTLoop* ast)
{
	generate_loop([this, ast]() -> t_astret
	{
		t_astret cond = ast->GetCond()->accept(this);
		return cond;
	}, [this, ast]
	{
		ast->GetLoopStmt()->accept(this);
	});

	return nullptr;
}


t_astret LLAsm::visit(const ASTExprList* ast)
{
	// only double arrays are handled here
	if(!ast->IsScalarArray())
	{
		throw std::runtime_error("ASTExprList: General expression list should not be directly evaluated.");
	}

	// array values and size
	const auto& lst = ast->GetList();
	std::size_t len = lst.size();
	std::array<std::size_t, 2> dims{{len, 1}};

	// allocate double array
	t_astret vec_mem = get_tmp_var(SymbolType::VECTOR, &dims);
	(*m_ostr) << "%" << vec_mem->name << " = alloca [" << len << " x double]\n";

	// set the individual array elements
	auto iter = lst.begin();
	for(std::size_t idx=0; idx<len; ++idx)
	{
		t_astret ptr = get_tmp_var();
		(*m_ostr) << "%" << ptr->name << " = getelementptr [" << len << " x double], ["
			<< len << " x double]* %" << vec_mem->name << ", i64 0, i64 " << idx << "\n";

		t_astret val = (*iter)->accept(this);
		val = convert_sym(val, SymbolType::SCALAR);

		(*m_ostr) << "store double %" << val->name << ", double* %"  << ptr->name << "\n";
		++iter;
	}

	return vec_mem;
}
