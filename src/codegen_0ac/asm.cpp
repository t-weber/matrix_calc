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
		throw std::runtime_error("Start function is not in symbol table.");

	// push stack frame size
	t_vm_int framesize = static_cast<t_vm_int>(get_stackframe_size(func));
	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
	m_ostr->put(static_cast<t_vm_byte>(VMType::INT));
	m_ostr->write(reinterpret_cast<const char*>(&framesize), vm_type_size<VMType::INT, false>);

	// push relative function address
	t_vm_addr func_addr = 0;  // to be filled in later
	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
	m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_IP));
	// already skipped over address and jmp instruction
	std::streampos addr_pos = m_ostr->tellp();
	t_vm_addr to_skip = static_cast<t_vm_addr>(func_addr - addr_pos);
	to_skip -= vm_type_size<VMType::ADDR_IP, true>;
	m_ostr->write(reinterpret_cast<const char*>(&to_skip), vm_type_size<VMType::ADDR_IP, false>);

	// call the start function
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
		to_skip -= vm_type_size<VMType::ADDR_IP, true>;
		m_ostr->write(reinterpret_cast<const char*>(&to_skip), vm_type_size<VMType::ADDR_IP, false>);
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

	if(!sym)
	{
		throw std::runtime_error("get_sym: \"" + scoped_name +
			"\" does not have an associated symbol.");
	}

	return sym;
}


/**
 * finds the size of the symbol for the stack frame
 */
std::size_t ZeroACAsm::get_sym_size(const Symbol* sym) const
{
	if(sym->ty == SymbolType::SCALAR)
	{
		return vm_type_size<VMType::REAL, true>;
	}
	else if(sym->ty == SymbolType::INT)
	{
		return vm_type_size<VMType::INT, true>;
	}
	else if(sym->ty == SymbolType::STRING)
	{
		return get_vm_str_size(std::get<0>(sym->dims), true);
	}
	else
	{
		throw std::runtime_error("Invalid symbol type for \"" + sym->name + "\".");
	}

	return 0;
}


/**
 * finds the size of the local function variables for the stack frame
 */
std::size_t ZeroACAsm::get_stackframe_size(const Symbol* func) const
{
	auto syms = m_syms->FindSymbolsWithSameScope(
		func->scoped_name + Symbol::get_scopenameseparator());
	std::size_t needed_size = 0;

	//for(const auto symty : func->argty)
	for(const Symbol* sym : syms)
		needed_size += get_sym_size(sym);

	needed_size += g_vm_longest_size + 1;  // TODO: remove padding
	return needed_size;
}



// ----------------------------------------------------------------------------
// conditions and loops
// ----------------------------------------------------------------------------
t_astret ZeroACAsm::visit(const ASTCond* ast)
{
	// condition
	ast->GetCond()->accept(this);

	t_vm_addr skipEndCond = 0;         // how many bytes to skip to jump to end of the if block?
	t_vm_addr skipEndIf = 0;           // how many bytes to skip to jump to end of the entire if statement?
	std::streampos skip_addr = 0;      // stream position with the condition jump label
	std::streampos skip_else_addr = 0; // stream position with the if block jump label

	// if the condition is not fulfilled...
	m_ostr->put(static_cast<t_vm_byte>(OpCode::NOT));

	// ...skip to the end of the if block
	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));      // push jump address
	m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_IP));
	skip_addr = m_ostr->tellp();
	m_ostr->write(reinterpret_cast<const char*>(&skipEndCond),
		vm_type_size<VMType::ADDR_IP, false>);
	m_ostr->put(static_cast<t_vm_byte>(OpCode::JMPCND));

	// if block
	std::streampos before_if_block = m_ostr->tellp();
	ast->GetIf()->accept(this);
	if(ast->HasElse())
	{
		// skip to end of if statement if there's an else block
		m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));  // push jump address
		m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_IP));
		skip_else_addr = m_ostr->tellp();
		m_ostr->write(reinterpret_cast<const char*>(&skipEndIf),
			vm_type_size<VMType::ADDR_IP, false>);
		m_ostr->put(static_cast<t_vm_byte>(OpCode::JMP));
	}

	std::streampos after_if_block = m_ostr->tellp();

	// go back and fill in missing number of bytes to skip
	skipEndCond = after_if_block - before_if_block;
	m_ostr->seekp(skip_addr);
	m_ostr->write(reinterpret_cast<const char*>(&skipEndCond),
		vm_type_size<VMType::ADDR_IP, false>);
	m_ostr->seekp(0, std::ios_base::end);

	// else block
	if(ast->HasElse())
	{
		std::streampos before_else_block = m_ostr->tellp();
		ast->GetElse()->accept(this);
		std::streampos after_else_block = m_ostr->tellp();

		// go back and fill in missing number of bytes to skip
		skipEndIf = after_else_block - before_else_block;
		m_ostr->seekp(skip_else_addr);
		m_ostr->write(reinterpret_cast<const char*>(&skipEndIf),
			vm_type_size<VMType::ADDR_IP, false>);
	}

	// go to end of stream
	m_ostr->seekp(0, std::ios_base::end);
	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTLoop* ast)
{
	static std::size_t loop_ident = 0;
	++loop_ident;
	m_cur_loop.push_back(loop_ident);

	std::streampos loop_begin = m_ostr->tellp();

	// loop condition
	ast->GetCond()->accept(this);

	// how many bytes to skip to jump to end of the block?
	t_vm_addr skip = 0;
	std::streampos skip_addr = 0;

	m_ostr->put(static_cast<t_vm_byte>(OpCode::NOT));

	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));      // push jump address
	m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_IP));
	skip_addr = m_ostr->tellp();
	m_ostr->write(reinterpret_cast<const char*>(&skip),
		vm_type_size<VMType::ADDR_IP, false>);
	m_ostr->put(static_cast<t_vm_byte>(OpCode::JMPCND));

	std::streampos before_block = m_ostr->tellp();
	// loop statements
	ast->GetLoopStmt()->accept(this);

	// loop back
	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));      // push jump address
	m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_IP));
	std::streampos after_block = m_ostr->tellp();
	skip = after_block - before_block;
	t_vm_addr skip_back = loop_begin - after_block;
	skip_back -= vm_type_size<VMType::ADDR_IP, true>;
	m_ostr->write(reinterpret_cast<const char*>(&skip_back),
		vm_type_size<VMType::ADDR_IP, false>);
	m_ostr->put(static_cast<t_vm_byte>(OpCode::JMP));

	// go back and fill in missing number of bytes to skip
	after_block = m_ostr->tellp();
	skip = after_block - before_block;
	m_ostr->seekp(skip_addr);
	m_ostr->write(reinterpret_cast<const char*>(&skip),
		vm_type_size<VMType::ADDR_IP, false>);

	// fill in any saved, unset start-of-loop jump addresses (continues)
	while(true)
	{
		auto iter = m_loop_begin_comefroms.find(loop_ident);
		if(iter == m_loop_begin_comefroms.end())
			break;

		std::streampos pos = iter->second;
		m_loop_begin_comefroms.erase(iter);

		t_vm_addr to_skip = loop_begin - pos;
		// already skipped over address and jmp instruction
		to_skip -= vm_type_size<VMType::ADDR_IP, true>;
		m_ostr->seekp(pos);
		m_ostr->write(reinterpret_cast<const char*>(&to_skip),
			vm_type_size<VMType::ADDR_IP, false>);
	}

	// fill in any saved, unset end-of-loop jump addresses (breaks)
	while(true)
	{
		auto iter = m_loop_end_comefroms.find(loop_ident);
		if(iter == m_loop_end_comefroms.end())
			break;

		std::streampos pos = iter->second;
		m_loop_end_comefroms.erase(iter);

		t_vm_addr to_skip = after_block - pos;
		// already skipped over address and jmp instruction
		to_skip -= vm_type_size<VMType::ADDR_IP, true>;
		m_ostr->seekp(pos);
		m_ostr->write(reinterpret_cast<const char*>(&to_skip),
			vm_type_size<VMType::ADDR_IP, false>);
	}

	// go to end of stream
	m_ostr->seekp(0, std::ios_base::end);
	m_cur_loop.pop_back();
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


t_astret ZeroACAsm::visit(const ASTComp* ast)
{
	t_astret term1 = ast->GetTerm1()->accept(this);
	t_astret term2 = ast->GetTerm2()->accept(this);

	switch(ast->GetOp())
	{
		case ASTComp::EQU:
		{
			m_ostr->put(static_cast<t_vm_byte>(OpCode::EQU));
			break;
		}
		case ASTComp::NEQ:
		{
			m_ostr->put(static_cast<t_vm_byte>(OpCode::NEQU));
			break;
		}
		case ASTComp::GT:
		{
			m_ostr->put(static_cast<t_vm_byte>(OpCode::GT));
			break;
		}
		case ASTComp::LT:
		{
			m_ostr->put(static_cast<t_vm_byte>(OpCode::LT));
			break;
		}
		case ASTComp::GEQ:
		{
			m_ostr->put(static_cast<t_vm_byte>(OpCode::GEQU));
			break;
		}
		case ASTComp::LEQ:
		{
			m_ostr->put(static_cast<t_vm_byte>(OpCode::LEQU));
			break;
		}
		default:
		{
			throw std::runtime_error("ASTComp: Invalid operation.");
			break;
		}
	}

	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTBool* ast)
{
	t_astret term1 = ast->GetTerm1()->accept(this);
	t_astret term2 = nullptr;
	if(ast->GetTerm2())
		term2 = ast->GetTerm2()->accept(this);

	switch(ast->GetOp())
	{
		case ASTBool::XOR:
		{
			m_ostr->put(static_cast<t_vm_byte>(OpCode::XOR));
			break;
		}
		case ASTBool::OR:
		{
			m_ostr->put(static_cast<t_vm_byte>(OpCode::OR));
			break;
		}
		case ASTBool::AND:
		{
			m_ostr->put(static_cast<t_vm_byte>(OpCode::AND));
			break;
		}
		case ASTBool::NOT:
		{
			m_ostr->put(static_cast<t_vm_byte>(OpCode::NOT));
			break;
		}
		default:
		{
			throw std::runtime_error("ASTBool: Invalid operation.");
			break;
		}
	}

	return nullptr;
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// variables
// ----------------------------------------------------------------------------
t_astret ZeroACAsm::visit(const ASTVarDecl* ast)
{
	if(!m_curscope.size())
		throw std::runtime_error("ASTVarDecl: Global variables are not supported.");
	const t_str& cur_func = *m_curscope.rbegin();

	for(const auto& varname : ast->GetVariables())
	{
		// get variable from symbol table and assign an address
		t_astret sym = get_sym(varname);
		if(!sym)
			throw std::runtime_error("ASTVarDecl: Variable \"" + varname + "\" is not in symbol table.");
		if(sym->addr)
			throw std::runtime_error("ASTVarDecl: Variable \"" + varname + "\" already declared.");

		// start of local stack frame?
		if(m_local_stack.find(cur_func) == m_local_stack.end())
		{
			// padding of maximum data type size, to avoid overwriting top stack value
			m_local_stack[cur_func] = g_vm_longest_size + 1;  // TODO: remove padding
		}

		sym->addr = -m_local_stack[cur_func];
		m_local_stack[cur_func] += get_sym_size(sym);

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
		throw std::runtime_error("ASTVar: Variable \"" + varname + "\" is not in symbol table.");
	if(!sym->addr)
		throw std::runtime_error("ASTVar: Variable \"" + varname + "\" has not been declared.");

	// push variable address
	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
	m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_BP));
	t_vm_addr addr = static_cast<t_vm_addr>(*sym->addr);
	m_ostr->write(reinterpret_cast<const char*>(&addr), vm_type_size<VMType::ADDR_BP, false>);

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
		throw std::runtime_error("ASTAssign: Variable \"" + varname + "\" is not in symbol table.");
	if(!sym->addr)
		throw std::runtime_error("ASTAssign: Variable \"" + varname + "\" has not been declared.");

	ast->GetExpr()->accept(this);

	// push variable address
	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
	m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_BP));
	t_vm_addr addr = static_cast<t_vm_addr>(*sym->addr);
	m_ostr->write(reinterpret_cast<const char*>(&addr), vm_type_size<VMType::ADDR_BP, false>);

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
	m_ostr->write(reinterpret_cast<const char*>(&val), vm_type_size<VMType::INT, false>);

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
	m_ostr->write(reinterpret_cast<const char*>(&len), vm_type_size<VMType::ADDR_MEM, false>);
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

	// function arguments
	std::size_t argidx = 0;
	t_vm_addr frame_addr = 2 * (vm_type_size<VMType::ADDR_IP, true>); // skip old bp and ip on frame
	for(const auto& [argname, argtype, dim1, dim2] : argnames)
	{
		// get variable from symbol table and assign an address
		t_astret sym = get_sym(argname);
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
		frame_addr += get_sym_size(sym);

		++argidx;
	}

	// get function from symbol table and set address
	t_astret func = get_sym(funcname);
	if(!func)
		throw std::runtime_error("ASTFunc: Function \"" + funcname + "\" is not in symbol table.");
	func->addr = m_ostr->tellp();

	// function statement block
	ast->GetStatements()->accept(this);

	std::streampos ret_streampos = m_ostr->tellp();

	// push stack frame size
	t_vm_int framesize = static_cast<t_vm_int>(get_stackframe_size(func));
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


t_astret ZeroACAsm::visit(const ASTCall* ast)
{
	const t_str* funcname = &ast->GetIdent();
	t_astret func = get_sym(*funcname);
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
		if(func->ext_name)
			funcname = &(*func->ext_name);

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

	// call internal function
	else
	{
		// push stack frame size
		t_vm_int framesize = static_cast<t_vm_int>(get_stackframe_size(func));
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

	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTReturn* ast)
{
	if(!m_curscope.size())
		throw std::runtime_error("ASTReturn: Not in a function.");

	/*const t_str& cur_func = *m_curscope.rbegin();
	t_astret func = get_sym(cur_func);
	if(!func)
		throw std::runtime_error("ASTReturn: Function \"" + cur_func + "\" is not in symbol table.");*/


	// return value(s)
	const auto& retvals = ast->GetRets()->GetList();
	std::size_t numRets = retvals.size();

	for(const auto& retast : retvals)
		retast->accept(this);

	// write jump address to the end of the function
	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH)); // push jump address
	m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_IP));
	m_endfunc_comefroms.push_back(m_ostr->tellp());
	t_vm_addr dummy_addr = 0;
	m_ostr->write(reinterpret_cast<const char*>(&dummy_addr), vm_type_size<VMType::ADDR_IP, false>);

	// jump to end of function
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


t_astret ZeroACAsm::visit(const ASTExprList* ast)
{
	// TODO

	return nullptr;
}
// ----------------------------------------------------------------------------
