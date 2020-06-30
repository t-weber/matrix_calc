/**
 * outputs the syntax tree
 * @author Tobias Weber
 * @date jun-20
 * @license: see 'LICENSE.GPL' file
 */

#include "printast.h"


ASTPrinter::ASTPrinter(std::ostream* ostr)
	: m_ostr{ostr}
{
}


t_astret ASTPrinter::visit(const ASTUMinus* ast)
{
	(*m_ostr) << "<UMinus>\n";
	ast->GetTerm()->accept(this);
	(*m_ostr) << "</UMinus>\n";

	return nullptr;
}


t_astret ASTPrinter::visit(const ASTPlus* ast)
{
	(*m_ostr) << "<Plus>\n";
	ast->GetTerm1()->accept(this);
	ast->GetTerm2()->accept(this);
	(*m_ostr) << "</Plus>\n";

	return nullptr;
}


t_astret ASTPrinter::visit(const ASTMult* ast)
{
	(*m_ostr) << "<Mult>\n";
	ast->GetTerm1()->accept(this);
	ast->GetTerm2()->accept(this);
	(*m_ostr) << "</Mult>\n";

	return nullptr;
}


t_astret ASTPrinter::visit(const ASTMod* ast)
{
	(*m_ostr) << "<Mod>\n";
	ast->GetTerm1()->accept(this);
	ast->GetTerm2()->accept(this);
	(*m_ostr) << "</Mod>\n";

	return nullptr;
}


t_astret ASTPrinter::visit(const ASTPow* ast)
{
	(*m_ostr) << "<Pow>\n";
	ast->GetTerm1()->accept(this);
	ast->GetTerm2()->accept(this);
	(*m_ostr) << "</Pow>\n";

	return nullptr;
}


t_astret ASTPrinter::visit(const ASTTransp* ast)
{
	(*m_ostr) << "<Transp>\n";
	ast->GetTerm()->accept(this);
	(*m_ostr) << "</Transp>\n";

	return nullptr;
}


t_astret ASTPrinter::visit(const ASTNorm* ast)
{
	(*m_ostr) << "<Norm>\n";
	ast->GetTerm()->accept(this);
	(*m_ostr) << "</Norm>\n";

	return nullptr;
}


t_astret ASTPrinter::visit(const ASTVar* ast)
{
	(*m_ostr) << "<Var ident=\"" << ast->GetIdent() << "\" />\n";

	return nullptr;
}


t_astret ASTPrinter::visit(const ASTCall* ast)
{
	(*m_ostr) << "<Call ident=\"" << ast->GetIdent() << "\">\n";

	std::size_t argidx = 0;
	for(const auto& arg : ast->GetArgumentList())
	{
		(*m_ostr) << "<arg_" << argidx << ">\n";
		arg->accept(this);
		(*m_ostr) << "</arg_" << argidx << ">\n";
		++argidx;
	}

	(*m_ostr) << "</Call>\n";

	return nullptr;
}


t_astret ASTPrinter::visit(const ASTStmts* ast)
{
	(*m_ostr) << "<Stmts>\n";

	std::size_t stmtidx = 0;
	for(const auto& stmt : ast->GetStatementList())
	{
		(*m_ostr) << "<stmt_" << stmtidx << ">\n";
		stmt->accept(this);
		(*m_ostr) << "</stmt_" << stmtidx << ">\n";
		++stmtidx;
	}

	(*m_ostr) << "</Stmts>\n";

	return nullptr;
}


t_astret ASTPrinter::visit(const ASTVarDecl* ast)
{
	(*m_ostr) << "<VarDecl>\n";

	std::size_t varidx = 0;
	for(const auto& var : ast->GetVariables())
	{
		(*m_ostr) << "<var_" << varidx << " ident=\"" << var << "\" />\n";
		++varidx;
	}

	if(ast->GetAssignment())
		ast->GetAssignment()->accept(this);

	(*m_ostr) << "</VarDecl>\n";

	return nullptr;
}


t_astret ASTPrinter::visit(const ASTFunc* ast)
{
	(*m_ostr) << "<Func ident=\"" << ast->GetIdent() << "\">\n";

	// TODO: args, ret vars

	ast->GetStatements()->accept(this);
	(*m_ostr) << "</Func>\n";

	return nullptr;
}


t_astret ASTPrinter::visit(const ASTReturn* ast)
{
	(*m_ostr) << "<Return>\n";
	ast->GetTerm()->accept(this);
	(*m_ostr) << "</Return>\n";

	return nullptr;
}


t_astret ASTPrinter::visit(const ASTAssign* ast)
{
	(*m_ostr) << "<Assign ident=\"" << ast->GetIdent() << "\">\n";
	ast->GetExpr()->accept(this);
	(*m_ostr) << "</Assign>\n";

	return nullptr;
}


t_astret ASTPrinter::visit(const ASTArrayAccess* ast)
{
	(*m_ostr) << "<ArrayAccess>\n";

	(*m_ostr) << "<idx1>\n";
	ast->GetNum1()->accept(this);
	(*m_ostr) << "</idx1>\n";

	if(ast->GetNum2())
	{
		(*m_ostr) << "<idx2>\n";
		ast->GetNum2()->accept(this);
		(*m_ostr) << "</idx2>\n";
	}

	(*m_ostr) << "<term>\n";
	ast->GetTerm()->accept(this);
	(*m_ostr) << "</term>\n";

	(*m_ostr) << "</ArrayAccess>\n";

	return nullptr;
}


t_astret ASTPrinter::visit(const ASTArrayAssign* ast)
{
	(*m_ostr) << "<ArrayAssign ident=\"" << ast->GetIdent() << "\">\n";

	(*m_ostr) << "<idx1>\n";
	ast->GetNum1()->accept(this);
	(*m_ostr) << "</idx1>\n";

	if(ast->GetNum2())
	{
		(*m_ostr) << "<idx2>\n";
		ast->GetNum2()->accept(this);
		(*m_ostr) << "</idx2>\n";
	}

	(*m_ostr) << "<expr>\n";
	ast->GetExpr()->accept(this);
	(*m_ostr) << "</expr>\n";

	(*m_ostr) << "</ArrayAssign>\n";

	return nullptr;
}


t_astret ASTPrinter::visit(const ASTComp* ast)
{
	(*m_ostr) << "<Comp op=\"";
	switch(ast->GetOp())
	{
		case ASTComp::EQU: (*m_ostr) << "equ"; break;
		case ASTComp::NEQ: (*m_ostr) << "neq"; break;
		case ASTComp::GT: (*m_ostr) << "gt"; break;
		case ASTComp::LT: (*m_ostr) << "lt"; break;
		case ASTComp::GEQ: (*m_ostr) << "geq"; break;
		case ASTComp::LEQ: (*m_ostr) << "leq"; break;
		default: (*m_ostr) << "unknown"; break;
	}
	(*m_ostr) << "\">\n";

	ast->GetTerm1()->accept(this);
	ast->GetTerm2()->accept(this);
	(*m_ostr) << "</Comp>\n";

	return nullptr;
}


t_astret ASTPrinter::visit(const ASTCond* ast)
{
	(*m_ostr) << "<Cond>\n";

	(*m_ostr) << "<cond>\n";
	ast->GetCond()->accept(this);
	(*m_ostr) << "</cond>\n";

	(*m_ostr) << "<if>\n";
	ast->GetIf()->accept(this);
	(*m_ostr) << "</if>\n";

	if(ast->GetElse())
	{
		(*m_ostr) << "<else>\n";
		ast->GetElse()->accept(this);
		(*m_ostr) << "</else>\n";
	}

	(*m_ostr) << "</Cond>\n";

	return nullptr;
}


t_astret ASTPrinter::visit(const ASTBool* ast)
{
	(*m_ostr) << "<Bool op=\"";
	switch(ast->GetOp())
	{
		case ASTBool::NOT: (*m_ostr) << "not"; break;
		case ASTBool::AND: (*m_ostr) << "and"; break;
		case ASTBool::OR: (*m_ostr) << "or"; break;
		case ASTBool::XOR: (*m_ostr) << "xor"; break;
		default: (*m_ostr) << "unknown"; break;
	}
	(*m_ostr) << "\">\n";

	ast->GetTerm1()->accept(this);
	if(ast->GetTerm2())
		ast->GetTerm2()->accept(this);

	(*m_ostr) << "</Bool>\n";

	return nullptr;
}


t_astret ASTPrinter::visit(const ASTLoop* ast)
{
	(*m_ostr) << "<Loop>\n";

	(*m_ostr) << "<cond>\n";
	ast->GetCond()->accept(this);
	(*m_ostr) << "</cond>\n";

	(*m_ostr) << "<stmt>\n";
	ast->GetLoopStmt()->accept(this);
	(*m_ostr) << "</stmt>\n";

	(*m_ostr) << "</Loop>\n";

	return nullptr;
}


t_astret ASTPrinter::visit(const ASTStrConst* ast)
{
	(*m_ostr) << "<Const type=\"str\" val=\"" << ast->GetVal() << "\" />\n";

	return nullptr;
}


t_astret ASTPrinter::visit(const ASTExprList* ast)
{
	(*m_ostr) << "<ExprList>\n";

	std::size_t expridx = 0;
	for(const auto& expr : ast->GetList())
	{
		(*m_ostr) << "<expr_" << expridx << ">\n";
		expr->accept(this);
		(*m_ostr) << "</expr_" << expridx << ">\n";
		++expridx;
	}

	(*m_ostr) << "</ExprList>\n";

	return nullptr;
}


t_astret ASTPrinter::visit(const ASTNumConst<double>* ast)
{
	(*m_ostr) << "<Const type=\"d\" val=\"" << ast->GetVal() << "\" />\n";

	return nullptr;
}


t_astret ASTPrinter::visit(const ASTNumConst<std::int64_t>* ast)
{
	(*m_ostr) << "<Const type=\"i64\" val=\"" << ast->GetVal() << "\" />\n";

	return nullptr;
}
