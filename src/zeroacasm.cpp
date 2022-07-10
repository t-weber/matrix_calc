/**
 * zero-address code generator
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 10-july-2022
 * @license: see 'LICENSE.GPL' file
 */

#include "zeroacasm.h"
#include <sstream>


ZeroACAsm::ZeroACAsm(SymTab* syms, std::ostream* ostr)
	: m_syms{syms}, m_ostr{ostr}
{}


t_astret ZeroACAsm::visit(const ASTStmts* ast)
{
	t_astret lastres = nullptr;

	for(const auto& stmt : ast->GetStatementList())
		lastres = stmt->accept(this);

	return lastres;
}


t_astret ZeroACAsm::visit(const ASTCond* ast)
{
	//return ast->GetCond()->accept(this);
	//ast->GetIf()->accept(this);
	//ast->GetElse()->accept(this);
	//ast->HasElse());

	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTLoop* ast)
{
	//t_astret cond = ast->GetCond()->accept(this);
	//ast->GetLoopStmt()->accept(this);
	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTUMinus* ast)
{
	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTPlus* ast)
{
	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTMult* ast)
{
	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTMod* ast)
{
	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTPow* ast)
{
	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTTransp* ast)
{
	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTNorm* ast)
{
	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTVar* ast)
{
	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTCall* ast)
{
	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTVarDecl* ast)
{
	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTFunc* ast)
{
	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTReturn* ast)
{
	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTAssign* ast)
{
	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTArrayAccess* ast)
{
	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTArrayAssign* ast)
{
	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTComp* ast)
{
	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTBool* ast)
{
	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTStrConst* ast)
{
	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTExprList* ast)
{
	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTNumConst<double>* ast)
{
	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTNumConst<std::int64_t>* ast)
{
	return nullptr;
}
