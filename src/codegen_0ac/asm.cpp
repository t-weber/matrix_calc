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
		if(!sym->addr)
			throw std::runtime_error("Function address for \"" + func_name + "\" not known.");

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
		t_vm_addr to_skip = static_cast<t_vm_addr>(*sym->addr - pos);
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
	if(!m_curscope.size())
	{
		throw std::runtime_error("ASTVarDecl: Global variables are not supported.");
	}

	const t_str& cur_func = *m_curscope.rbegin();

	for(const auto& varname : ast->GetVariables())
	{
		// get variable from symbol table and assign an address
		t_astret sym = get_sym(varname);
		if(!sym)
			throw std::runtime_error("ASTVarDecl: Variable \"" + varname + "\" not in symbol table.");
		if(sym->addr)
			throw std::runtime_error("ASTVarDecl: Variable \"" + varname + "\" already declared.");

		// start of local stack frame?
		if(m_local_stack.find(cur_func) == m_local_stack.end())
		{
			// padding of maximum data type size, to avoid overwriting top stack value
			m_local_stack[cur_func] = g_vm_longest_size + 1;
		}

		sym->addr = -m_local_stack[cur_func];

		if(sym->ty == SymbolType::SCALAR)
			m_local_stack[cur_func] += get_vm_type_size(VMType::REAL, true);
		else if(sym->ty == SymbolType::INT)
			m_local_stack[cur_func] += get_vm_type_size(VMType::INT, true);
		else
			throw std::runtime_error("ASTVarDecl: Invalid type in declaration: \"" + sym->name + "\".");

		// optional assignment
		if(ast->GetAssignment())
		{
			ast->GetAssignment()->accept(this);
		}
	}

	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTVar* ast)
{
	const t_str& varname = ast->GetIdent();

	// get variable from symbol table
	t_astret sym = get_sym(varname);
	if(!sym)
		throw std::runtime_error("ASTVar: Variable \"" + varname + "\" not in symbol table.");
	if(!sym->addr)
		throw std::runtime_error("ASTVar: Variable \"" + varname + "\" has not been declared.");

	// push variable address
	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
	m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_BP));
	t_vm_addr addr = static_cast<t_vm_addr>(*sym->addr);
	m_ostr->write(reinterpret_cast<const char*>(&addr), sizeof(t_vm_addr));

	// dereference the variable
	if(sym->ty != SymbolType::FUNC)
		m_ostr->put(static_cast<t_vm_byte>(OpCode::DEREF));

	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTAssign* ast)
{
	t_str varname = ast->GetIdent();
	t_astret sym = get_sym(varname);
	if(!sym)
		throw std::runtime_error("ASTAssign: Variable \"" + varname + "\" not in symbol table.");
	if(!sym->addr)
		throw std::runtime_error("ASTAssign: Variable \"" + varname + "\" has not been declared.");

	ast->GetExpr()->accept(this);

	// push variable address
	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
	m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_BP));
	t_vm_addr addr = static_cast<t_vm_addr>(*sym->addr);
	m_ostr->write(reinterpret_cast<const char*>(&addr), sizeof(t_vm_addr));

	// assign variable
	m_ostr->put(static_cast<t_vm_byte>(OpCode::WRMEM));
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

	// TODO: args
	/*std::size_t argidx=0;
	for(const auto& [argname, argtype, dim1, dim2] : argnames)
	{
		std::string varname = *m_curscope.rbegin() + "/" + argname;

		VMType argty = VMType::UNKNOWN;
		if(argtype == SymbolType::SCALAR)
			argty = VMType::REAL;
		else if(argtype == SymbolType::INT)
			argty = VMType::INT;
		else if(argtype == SymbolType::STRING)
			argty = VMType::STR;

		++argidx;
	}*/

	// get function from symbol table and set address
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
	const t_str* funcname = &ast->GetIdent();
	t_astret func = get_sym(*funcname);
	if(!func)
		throw std::runtime_error("ASTCall: Function \"" + (*funcname) + "\" not in symbol table.");

	t_vm_int num_args = static_cast<t_vm_int>(func->argty.size());
	if(static_cast<t_vm_int>(ast->GetArgumentList().size()) != num_args)
		throw std::runtime_error("ASTCall: Invalid number of function parameters for \"" + (*funcname) + "\".");

	for(const auto& curarg : ast->GetArgumentList())
		curarg->accept(this);

	if(func->is_external)
	{
		// if the function has an alternate external name assigned, use it
		if(func->ext_name)
			funcname = &(*func->ext_name);

		// call external function
		// push external function name
		m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
		// write type descriptor byte
		m_ostr->put(static_cast<t_vm_byte>(VMType::STR));
		// write function name
		t_vm_addr len = static_cast<t_vm_addr>(funcname->length());
		m_ostr->write(reinterpret_cast<const char*>(&len),
			vm_type_size<VMType::ADDR_MEM, false>);
		// write string data
		m_ostr->write(funcname->data(), len);
		m_ostr->put(static_cast<t_vm_byte>(OpCode::EXTCALL));
	}
	else
	{
		// call internal function
		t_vm_addr func_addr = 0;  // to be filled in later
		// push function address relative to instruction pointer
		m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
		m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_IP));
		// already skipped over address and jmp instruction
		std::streampos addr_pos = m_ostr->tellp();
		t_vm_addr to_skip = static_cast<t_vm_addr>(func_addr - addr_pos);
		to_skip -= sizeof(t_vm_byte) + sizeof(t_vm_addr);
		m_ostr->write(reinterpret_cast<const char*>(&to_skip), sizeof(t_vm_addr));
		m_ostr->put(static_cast<t_vm_byte>(OpCode::CALL));

		// function address not yet known
		m_func_comefroms.emplace_back(
			std::make_tuple(*funcname, addr_pos, num_args, ast));
	}

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
