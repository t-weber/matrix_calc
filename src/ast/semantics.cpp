/**
 * performs semantic checks on the syntax tree
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date jul-20
 * @license: see 'LICENSE.GPL' file
 */

#include "semantics.h"


Semantics::Semantics()
{
}


t_astret Semantics::visit(const ASTUMinus* ast)
{
	ast->GetTerm()->accept(this);

	return nullptr;
}


t_astret Semantics::visit(const ASTPlus* ast)
{
	ast->GetTerm1()->accept(this);
	ast->GetTerm2()->accept(this);

	return nullptr;
}


t_astret Semantics::visit(const ASTMult* ast)
{
	ast->GetTerm1()->accept(this);
	ast->GetTerm2()->accept(this);

	return nullptr;
}


t_astret Semantics::visit(const ASTMod* ast)
{
	ast->GetTerm1()->accept(this);
	ast->GetTerm2()->accept(this);

	return nullptr;
}


t_astret Semantics::visit(const ASTPow* ast)
{
	ast->GetTerm1()->accept(this);
	ast->GetTerm2()->accept(this);

	return nullptr;
}


t_astret Semantics::visit(const ASTTransp* ast)
{
	ast->GetTerm()->accept(this);

	return nullptr;
}


t_astret Semantics::visit(const ASTNorm* ast)
{
	ast->GetTerm()->accept(this);

	return nullptr;
}


t_astret Semantics::visit([[maybe_unused]] const ASTVar* ast)
{
	return nullptr;
}


t_astret Semantics::visit(const ASTCall* ast)
{
	for(const auto& arg : ast->GetArgumentList())
		arg->accept(this);

	return nullptr;
}


t_astret Semantics::visit(const ASTStmts* ast)
{
	for(const auto& stmt : ast->GetStatementList())
		stmt->accept(this);

	return nullptr;
}


t_astret Semantics::visit(const ASTVarDecl* ast)
{
	if(ast->GetAssignment())
		ast->GetAssignment()->accept(this);

	return nullptr;
}


t_astret Semantics::visit(const ASTFunc* ast)
{
	ast->GetStatements()->accept(this);

	return nullptr;
}


t_astret Semantics::visit(const ASTReturn* ast)
{
	const auto& retvals = ast->GetRets()->GetList();
	std::size_t numRets = retvals.size();

	if(numRets == 0)
	{
	}
	else if(numRets == 1)
	{
		(*retvals.begin())->accept(this);
	}
	else if(numRets > 1)
	{
		for(const auto& elem : retvals)
			elem->accept(this);
	}

	return nullptr;
}


t_astret Semantics::visit(const ASTAssign* ast)
{
	// multiple assignments
	if(ast->IsMultiAssign())
		ast->GetExpr()->accept(this);

	// single assignment
	else
		ast->GetExpr()->accept(this);

	return nullptr;
}


t_astret Semantics::visit(const ASTArrayAccess* ast)
{
	ast->GetNum1()->accept(this);

	if(ast->GetNum2())
		ast->GetNum2()->accept(this);

	ast->GetTerm()->accept(this);

	return nullptr;
}


t_astret Semantics::visit(const ASTArrayAssign* ast)
{
	ast->GetNum1()->accept(this);

	if(ast->GetNum2())
		ast->GetNum2()->accept(this);

	ast->GetExpr()->accept(this);

	return nullptr;
}


t_astret Semantics::visit(const ASTComp* ast)
{
	ast->GetTerm1()->accept(this);
	ast->GetTerm2()->accept(this);

	return nullptr;
}


t_astret Semantics::visit(const ASTCond* ast)
{
	ast->GetCond()->accept(this);
	ast->GetIf()->accept(this);

	if(ast->GetElse())
		ast->GetElse()->accept(this);

	return nullptr;
}


t_astret Semantics::visit(const ASTBool* ast)
{
	ast->GetTerm1()->accept(this);
	if(ast->GetTerm2())
		ast->GetTerm2()->accept(this);

	return nullptr;
}


t_astret Semantics::visit(const ASTLoop* ast)
{
	ast->GetCond()->accept(this);
	ast->GetLoopStmt()->accept(this);

	return nullptr;
}


t_astret Semantics::visit([[maybe_unused]] const ASTStrConst* ast)
{
	return nullptr;
}


t_astret Semantics::visit(const ASTExprList* ast)
{
	for(const auto& expr : ast->GetList())
		expr->accept(this);

	return nullptr;
}


t_astret Semantics::visit([[maybe_unused]] const ASTNumConst<t_real>* ast)
{
	return nullptr;
}


t_astret Semantics::visit([[maybe_unused]] const ASTNumConst<t_int>* ast)
{
	return nullptr;
}
