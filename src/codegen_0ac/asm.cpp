/**
 * zero-address code generator
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 10-july-2022
 * @license: see 'LICENSE.GPL' file
 */

#include "asm.h"
#include "../vm_0ac/opcodes.h"

#include <sstream>



ZeroACAsm::ZeroACAsm(SymTab* syms, std::ostream* ostr)
	: m_syms{syms}, m_ostr{ostr}
{}


void ZeroACAsm::Finish()
{
	// add a final halt instruction
	m_ostr->put(static_cast<t_vm_byte>(OpCode::HALT));
}


// ----------------------------------------------------------------------------
// conditions and loops
// ----------------------------------------------------------------------------
t_astret ZeroACAsm::visit(const ASTCond* ast)
{
	// TODO

	//return ast->GetCond()->accept(this);
	//ast->GetIf()->accept(this);
	//ast->GetElse()->accept(this);
	//ast->HasElse());

	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTLoop* ast)
{
	// TODO

	//t_astret cond = ast->GetCond()->accept(this);
	//ast->GetLoopStmt()->accept(this);
	return nullptr;
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// operations
// ----------------------------------------------------------------------------
t_astret ZeroACAsm::visit(const ASTUMinus* ast)
{
	ast->GetTerm()->accept(this);
	m_ostr->put(static_cast<t_vm_byte>(OpCode::USUB));
	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTPlus* ast)
{
	ast->GetTerm1()->accept(this);
	ast->GetTerm2()->accept(this);

	// TODO: cast if needed

	if(ast->IsInverted())
		m_ostr->put(static_cast<t_vm_byte>(OpCode::SUB));
	else
		m_ostr->put(static_cast<t_vm_byte>(OpCode::ADD));

	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTMult* ast)
{
	ast->GetTerm1()->accept(this);
	ast->GetTerm2()->accept(this);

	// TODO: cast if needed

	if(ast->IsInverted())
		m_ostr->put(static_cast<t_vm_byte>(OpCode::DIV));
	else
		m_ostr->put(static_cast<t_vm_byte>(OpCode::MUL));

	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTMod* ast)
{
	ast->GetTerm1()->accept(this);
	ast->GetTerm2()->accept(this);

	// TODO: cast if needed

	m_ostr->put(static_cast<t_vm_byte>(OpCode::MOD));

	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTPow* ast)
{
	ast->GetTerm1()->accept(this);
	ast->GetTerm2()->accept(this);

	// TODO: cast if needed

	m_ostr->put(static_cast<t_vm_byte>(OpCode::POW));

	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTTransp* ast)
{
	// TODO

	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTNorm* ast)
{
	// TODO

	return nullptr;
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// variables
// ----------------------------------------------------------------------------
t_astret ZeroACAsm::visit(const ASTVarDecl* ast)
{
	// TODO

	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTVar* ast)
{
	// TODO

	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTAssign* ast)
{
	// TODO

	return nullptr;
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------
t_astret ZeroACAsm::visit(const ASTNumConst<t_real>* ast)
{
	t_vm_real val = static_cast<t_vm_real>(ast->GetVal());

	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
	// write type descriptor byte
	m_ostr->put(static_cast<t_vm_byte>(VMType::REAL));
	// write value
	m_ostr->write(reinterpret_cast<const char*>(&val),
		vm_type_size<VMType::REAL, false>);

	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTNumConst<t_int>* ast)
{
	t_vm_int val = static_cast<t_vm_int>(ast->GetVal());

	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
	// write type descriptor byte
	m_ostr->put(static_cast<t_vm_byte>(VMType::INT));
	// write data
	m_ostr->write(reinterpret_cast<const char*>(&val),
		vm_type_size<VMType::INT, false>);

	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTStrConst* ast)
{
	const t_str& val = ast->GetVal();

	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
	// write type descriptor byte
	m_ostr->put(static_cast<t_vm_byte>(VMType::STR));
	// write string length
	t_vm_addr len = static_cast<t_vm_addr>(val.length());
	m_ostr->write(reinterpret_cast<const char*>(&len),
		vm_type_size<VMType::ADDR_MEM, false>);
	// write string data
	m_ostr->write(val.data(), len);

	return nullptr;
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// functions
// ----------------------------------------------------------------------------
t_astret ZeroACAsm::visit(const ASTFunc* ast)
{
	// TODO
	ast->GetStatements()->accept(this);

	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTCall* ast)
{
	// TODO

	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTReturn* ast)
{
	// TODO

	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTStmts* ast)
{
	for(const auto& stmt : ast->GetStatementList())
		stmt->accept(this);

	return nullptr;
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// arrays
// ----------------------------------------------------------------------------
t_astret ZeroACAsm::visit(const ASTArrayAccess* ast)
{
	// TODO

	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTArrayAssign* ast)
{
	// TODO

	return nullptr;
}
// ----------------------------------------------------------------------------



t_astret ZeroACAsm::visit(const ASTExprList* ast)
{
	// TODO

	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTComp* ast)
{
	// TODO

	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTBool* ast)
{
	// TODO

	return nullptr;
}
