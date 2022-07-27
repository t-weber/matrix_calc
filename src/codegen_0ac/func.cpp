/**
 * zero-address code generator
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 10-july-2022
 * @license: see 'LICENSE.GPL' file
 */

#include "asm.h"


/**
 * finds the size of the local function variables for the stack frame
 */
std::size_t ZeroACAsm::GetStackFrameSize(const Symbol* func) const
{
	auto syms = m_syms->FindSymbolsWithSameScope(
		func->scoped_name + Symbol::get_scopenameseparator());
	std::size_t needed_size = 0;

	//for(const auto symty : func->argty)
	for(const Symbol* sym : syms)
		needed_size += GetSymSize(sym);

	return needed_size;
}



// ----------------------------------------------------------------------------
// functions
// ----------------------------------------------------------------------------
t_astret ZeroACAsm::visit(const ASTFunc* ast)
{
	const t_str& funcname = ast->GetIdent();
	m_curscope.push_back(funcname);

	auto argnames = ast->GetArgs();
	t_vm_int num_args = static_cast<t_vm_int>(argnames.size());

	// function arguments
	std::size_t argidx = 0;
	t_vm_addr frame_addr = 2 * (vm_type_size<VMType::ADDR_IP, true>); // skip old bp and ip on frame
	for(const auto& [argname, argtype, dim1, dim2] : argnames)
	{
		// get variable from symbol table and assign an address
		t_astret sym = GetSym(argname);
		if(!sym)
			throw std::runtime_error("ASTFunc: Argument \"" + argname + "\" is not in symbol table.");
		if(sym->addr)
			throw std::runtime_error("ASTFunc: Argument \"" + argname + "\" already declared.");
		if(!sym->is_arg)
			throw std::runtime_error("ASTFunc: Variable \"" + argname + "\" is not an argument.");
		if(sym->ty != argtype)
			throw std::runtime_error("ASTFunc: Argument \"" + argname + "\" type mismatch.");
		if(sym->argidx != argidx)
			throw std::runtime_error("ASTFunc: Argument \"" + argname + "\" index mismatch.");

		sym->addr = frame_addr;
		frame_addr += GetSymSize(sym);

		++argidx;
	}

	// get function from symbol table and set address
	t_astret func = GetSym(funcname);
	if(!func)
		throw std::runtime_error("ASTFunc: Function \"" + funcname + "\" is not in symbol table.");
	func->addr = m_ostr->tellp();

	// function statement block
	ast->GetStatements()->accept(this);

	std::streampos ret_streampos = m_ostr->tellp();

	// push stack frame size
	t_vm_int framesize = static_cast<t_vm_int>(GetStackFrameSize(func));
	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
	m_ostr->put(static_cast<t_vm_byte>(VMType::INT));
	m_ostr->write(reinterpret_cast<const char*>(&framesize), vm_type_size<VMType::INT, false>);

	// push number of arguments
	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
	m_ostr->put(static_cast<t_vm_byte>(VMType::INT));
	m_ostr->write(reinterpret_cast<const char*>(&num_args), vm_type_size<VMType::INT, false>);

	// return
	m_ostr->put(static_cast<t_vm_byte>(OpCode::RET));

	// end-of-function jump address
	std::streampos end_func_streampos = m_ostr->tellp();

	// fill in any saved, unset end-of-function jump addresses
	for(std::streampos pos : m_endfunc_comefroms)
	{
		t_vm_addr to_skip = ret_streampos - pos;
		// already skipped over address and jmp instruction
		to_skip -= vm_type_size<VMType::ADDR_IP, true>;
		m_ostr->seekp(pos);
		m_ostr->write(reinterpret_cast<const char*>(&to_skip), vm_type_size<VMType::ADDR_IP, false>);
	}
	m_endfunc_comefroms.clear();
	m_ostr->seekp(end_func_streampos);

	m_cur_loop.clear();
	m_curscope.pop_back();

	return nullptr;
}


/**
 * calls an external function
 */
void ZeroACAsm::CallExternal(const t_str& funcname)
{
	// push external function name
	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
	// write type descriptor byte
	m_ostr->put(static_cast<t_vm_byte>(VMType::STR));
	// write function name
	t_vm_addr len = static_cast<t_vm_addr>(funcname.length());
	m_ostr->write(reinterpret_cast<const char*>(&len),
		vm_type_size<VMType::ADDR_MEM, false>);
	// write string data
	m_ostr->write(funcname.data(), len);
	m_ostr->put(static_cast<t_vm_byte>(OpCode::EXTCALL));
}


t_astret ZeroACAsm::visit(const ASTCall* ast)
{
	const t_str* funcname = &ast->GetIdent();
	t_astret func = GetSym(*funcname);
	if(!func)
		throw std::runtime_error("ASTCall: Function \"" + (*funcname) + "\" is not in symbol table.");

	t_vm_int num_args = static_cast<t_vm_int>(func->argty.size());
	if(static_cast<t_vm_int>(ast->GetArgumentList().size()) != num_args)
		throw std::runtime_error("ASTCall: Invalid number of function parameters for \"" + (*funcname) + "\".");

	for(auto iter = ast->GetArgumentList().rbegin(); iter != ast->GetArgumentList().rend(); ++iter)
		(*iter)->accept(this);

	// call external function
	if(func->is_external)
	{
		// if the function has an alternate external name assigned, use it
		//if(func->ext_name)
		//	funcname = &(*func->ext_name);
		CallExternal(*funcname);
	}

	// call internal function
	else
	{
		// push stack frame size
		t_vm_int framesize = static_cast<t_vm_int>(GetStackFrameSize(func));
		m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
		m_ostr->put(static_cast<t_vm_byte>(VMType::INT));
		m_ostr->write(reinterpret_cast<const char*>(&framesize), vm_type_size<VMType::INT, false>);

		// push function address relative to instruction pointer
		t_vm_addr func_addr = 0;  // to be filled in later
		m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
		m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_IP));
		// already skipped over address and jmp instruction
		std::streampos addr_pos = m_ostr->tellp();
		t_vm_addr to_skip = static_cast<t_vm_addr>(func_addr - addr_pos);
		to_skip -= vm_type_size<VMType::ADDR_IP, true>;
		m_ostr->write(reinterpret_cast<const char*>(&to_skip), vm_type_size<VMType::ADDR_IP, false>);

		// call the function
		m_ostr->put(static_cast<t_vm_byte>(OpCode::CALL));

		// function address not yet known
		m_func_comefroms.emplace_back(
			std::make_tuple(*funcname, addr_pos, num_args, ast));
	}

	return func;
}


t_astret ZeroACAsm::visit(const ASTReturn* ast)
{
	if(!m_curscope.size())
		throw std::runtime_error("ASTReturn: Not in a function.");

	/*const t_str& cur_func = *m_curscope.rbegin();
	t_astret func = GetSym(cur_func);
	if(!func)
		throw std::runtime_error("ASTReturn: Function \"" + cur_func + "\" is not in symbol table.");*/

	t_astret sym_ret = nullptr;

	// return value(s)
	for(const auto& retast : ast->GetRets()->GetList())
	{
		t_astret sym = retast->accept(this);
		if(!sym_ret)
			sym_ret = sym;
	}

	// write jump address to the end of the function
	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH)); // push jump address
	m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_IP));
	m_endfunc_comefroms.push_back(m_ostr->tellp());
	t_vm_addr dummy_addr = 0;
	m_ostr->write(reinterpret_cast<const char*>(&dummy_addr), vm_type_size<VMType::ADDR_IP, false>);

	// jump to end of function
	m_ostr->put(static_cast<t_vm_byte>(OpCode::JMP));

	return sym_ret;
}


t_astret ZeroACAsm::visit(const ASTStmts* ast)
{
	for(const auto& stmt : ast->GetStatementList())
		stmt->accept(this);

	return nullptr;
}
// ----------------------------------------------------------------------------
