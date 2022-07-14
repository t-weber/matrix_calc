/**
 * zero-address code generator
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 10-july-2022
 * @license: see 'LICENSE.GPL' file
 */

#include "asm.h"

#include <sstream>



ZeroACAsm::ZeroACAsm(SymTab* syms, std::ostream* ostr)
	: m_syms{syms}, m_ostr{ostr}
{}



/**
 * insert start-up code
 */
void ZeroACAsm::Start()
{
	// call start function
	const t_str& funcname = "start";
	t_astret func = get_sym(funcname);
	if(!func)
		throw std::runtime_error("Start function not in symbol table.");

	t_vm_addr func_addr = 0;  // to be filled in later
	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
	// push relative function address
	m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_IP));
	// already skipped over address and jmp instruction
	std::streampos addr_pos = m_ostr->tellp();
	t_vm_addr to_skip = static_cast<t_vm_addr>(func_addr - addr_pos);
	to_skip -= sizeof(t_vm_byte) + sizeof(t_vm_addr);
	m_ostr->write(reinterpret_cast<const char*>(&to_skip), sizeof(t_vm_addr));
	m_ostr->put(static_cast<t_vm_byte>(OpCode::CALL));

	// function address not yet known
	m_func_comefroms.emplace_back(
		std::make_tuple(funcname, addr_pos, 0, nullptr));

	// add a halt instruction
	m_ostr->put(static_cast<t_vm_byte>(OpCode::HALT));
}


/**
 * insert missing addresses and finalising code
 */
void ZeroACAsm::Finish()
{
	// add a final halt instruction
	m_ostr->put(static_cast<t_vm_byte>(OpCode::HALT));


	// patch function addresses
	for(const auto& [func_name, pos, num_args, call_ast] : m_func_comefroms)
	{
		t_astret sym = get_sym(func_name);
		if(!sym)
			throw std::runtime_error("Tried to call unknown function \"" + func_name + "\".");

		t_vm_int func_num_args = static_cast<t_vm_int>(sym->argty.size());
		if(num_args != func_num_args)
		{
			std::ostringstream msg;
			msg << "Function \"" << func_name << "\" takes " << func_num_args
				<< " arguments, but " << num_args << " were given.";
			throw std::runtime_error(msg.str());
		}

		m_ostr->seekp(pos);

		// write relative function address
		t_vm_addr to_skip = static_cast<t_vm_addr>(sym->addr - pos);
		// already skipped over address and jmp instruction
		to_skip -= sizeof(t_vm_byte) + sizeof(t_vm_addr);
		m_ostr->write(reinterpret_cast<const char*>(&to_skip), sizeof(t_vm_addr));
	}

	// seek to end of stream
	m_ostr->seekp(0, std::ios_base::end);
}


/**
 * find the symbol with a specific name in the symbol table
 */
t_astret ZeroACAsm::get_sym(const t_str& name) const
{
	t_str scoped_name;
	for(const t_str& scope : m_curscope)
		scoped_name += scope + Symbol::get_scopenameseparator();
	scoped_name += name;

	Symbol* sym = nullptr;
	if(m_syms)
	{
		sym = m_syms->FindSymbol(scoped_name);

		// try global scope instead
		if(!sym)
			sym = m_syms->FindSymbol(name);
	}

	if(sym==nullptr)
		throw std::runtime_error("get_sym: \"" + scoped_name + "\" does not have an associated symbol.");

	return sym;
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
	const t_str& funcname = ast->GetIdent();
	m_curscope.push_back(funcname);

	auto argnames = ast->GetArgs();
	t_vm_int num_args = static_cast<t_vm_int>(argnames.size());

	if(num_args)
	{
		// TODO
	}

	// add function to symbol table
	t_astret func = get_sym(funcname);
	if(!func)
		throw std::runtime_error("ASTFunc: Function \"" + funcname + "\" not in symbol table.");
	func->addr = m_ostr->tellp();

	// function statement block
	ast->GetStatements()->accept(this);

	std::streampos ret_streampos = m_ostr->tellp();

	// push number of arguments and return
	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
	m_ostr->put(static_cast<t_vm_byte>(VMType::INT));
	m_ostr->write(reinterpret_cast<const char*>(&num_args), sizeof(t_vm_int));
	m_ostr->put(static_cast<t_vm_byte>(OpCode::RET));

	// end-of-function jump address
	std::streampos end_func_streampos = m_ostr->tellp();

	// fill in any saved, unset end-of-function jump addresses
	for(std::streampos pos : m_endfunc_comefroms)
	{
		t_vm_addr to_skip = ret_streampos - pos;
		// already skipped over address and jmp instruction
		to_skip -= sizeof(t_vm_byte) + sizeof(t_vm_addr);
		m_ostr->seekp(pos);
		m_ostr->write(reinterpret_cast<const char*>(&to_skip), sizeof(t_vm_addr));
	}
	m_endfunc_comefroms.clear();
	m_ostr->seekp(end_func_streampos);

	m_curscope.pop_back();
	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTCall* ast)
{
	const t_str& funcname = ast->GetIdent();
	t_astret func = get_sym(funcname);
	if(!func)
		throw std::runtime_error("ASTCall: Function \"" + funcname + "\" not in symbol table.");

	t_vm_int num_args = static_cast<t_vm_int>(func->argty.size());
	if(ast->GetArgumentList().size() != num_args)
		throw std::runtime_error("ASTCall: Invalid number of function parameters for \"" + funcname + "\".");

	for(const auto& curarg : ast->GetArgumentList())
	{
		// TODO
	}

	t_vm_addr func_addr = 0;  // to be filled in later
	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
	// push relative function address
	m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_IP));
	// already skipped over address and jmp instruction
	std::streampos addr_pos = m_ostr->tellp();
	t_vm_addr to_skip = static_cast<t_vm_addr>(func_addr - addr_pos);
	to_skip -= sizeof(t_vm_byte) + sizeof(t_vm_addr);
	m_ostr->write(reinterpret_cast<const char*>(&to_skip), sizeof(t_vm_addr));
	m_ostr->put(static_cast<t_vm_byte>(OpCode::CALL));

	// function address not yet known
	m_func_comefroms.emplace_back(
		std::make_tuple(funcname, addr_pos, num_args, ast));

	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTReturn* ast)
{
	// jump to the end of the function
	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH)); // push jump address
	m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_IP));
	m_endfunc_comefroms.push_back(m_ostr->tellp());
	t_vm_addr dummy_addr = 0;
	m_ostr->write(reinterpret_cast<const char*>(&dummy_addr), sizeof(t_vm_addr));
	m_ostr->put(static_cast<t_vm_byte>(OpCode::JMP));

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
