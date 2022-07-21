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
{
	// dummy symbol for scalar constants
	m_scalar_const = new Symbol();
	m_scalar_const->ty = SymbolType::SCALAR;
	m_scalar_const->is_tmp = true;
	m_scalar_const->name = "<scalar>";

	// dummy symbol for int constants
	m_int_const = new Symbol();
	m_int_const->ty = SymbolType::INT;
	m_int_const->is_tmp = true;
	m_int_const->name = "<int>";

	// dummy symbol for string constants
	m_str_const = new Symbol();
	m_str_const->ty = SymbolType::STRING;
	m_str_const->is_tmp = true;
	m_str_const->name = "<str>";
}


ZeroACAsm::~ZeroACAsm()
{
	if(m_scalar_const)
	{
		delete m_scalar_const;
		m_scalar_const = nullptr;
	}

	if(m_int_const)
	{
		delete m_int_const;
		m_int_const = nullptr;
	}

	if(m_str_const)
	{
		delete m_str_const;
		m_str_const = nullptr;
	}
}


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
		return get_vm_str_size(std::get<0>(sym->dims), true, true);
	}
	else if(sym->ty == SymbolType::VECTOR)
	{
		return get_vm_vec_size(std::get<0>(sym->dims), true, true);
	}
	else if(sym->ty == SymbolType::MATRIX)
	{
		return get_vm_mat_size(std::get<0>(sym->dims), std::get<1>(sym->dims), true, true);
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


t_astret ZeroACAsm::visit(const ASTLoopBreak* ast)
{
	//if(!m_curscope.size())
	//	throw std::runtime_error("ASTLoopBreak: Not in a function.");
	if(!m_cur_loop.size())
		throw std::runtime_error("ASTLoopBreak: Not in a loop.");

	t_int loop_depth = ast->GetNumLoops();

	// reduce to maximum loop depth
	if(static_cast<std::size_t>(loop_depth) >= m_cur_loop.size() || loop_depth < 0)
		loop_depth = static_cast<t_int>(m_cur_loop.size()-1);

	// jump to the end of the loop
	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));  // push jump address
	m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_IP));
	m_loop_end_comefroms.insert(std::make_pair(
		m_cur_loop[m_cur_loop.size()-loop_depth-1], m_ostr->tellp()));
	t_vm_addr dummy_addr = 0;
	m_ostr->write(reinterpret_cast<const char*>(&dummy_addr),
		vm_type_size<VMType::ADDR_IP, false>);
	m_ostr->put(static_cast<t_vm_byte>(OpCode::JMP));

	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTLoopNext* ast)
{
	//if(!m_curscope.size())
	//	throw std::runtime_error("ASTLoopNext: Not in a function.");
	if(!m_cur_loop.size())
		throw std::runtime_error("ASTLoopNext: Not in a loop.");

	t_int loop_depth = ast->GetNumLoops();

	// reduce to maximum loop depth
	if(static_cast<std::size_t>(loop_depth) >= m_cur_loop.size() || loop_depth < 0)
		loop_depth = static_cast<t_int>(m_cur_loop.size()-1);

	// jump to the beginning of the loop
	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));  // push jump address
	m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_IP));
	m_loop_begin_comefroms.insert(std::make_pair(
		m_cur_loop[m_cur_loop.size()-loop_depth-1], m_ostr->tellp()));
	t_vm_addr dummy_addr = 0;
	m_ostr->write(reinterpret_cast<const char*>(&dummy_addr),
		vm_type_size<VMType::ADDR_IP, false>);
	m_ostr->put(static_cast<t_vm_byte>(OpCode::JMP));

	return nullptr;
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// operations
// ----------------------------------------------------------------------------
/**
 * return common type of a binary operation
 * @returns [ needs_cast?, first term is common type? ]
 */
static std::tuple<bool, bool>
get_cast_symtype(t_astret term1, t_astret term2)
{
	if(!term1 || !term2)
		return std::make_tuple(false, true);

	SymbolType ty1 = term1->ty;
	SymbolType ty2 = term2->ty;

	// use return type for function
	if(ty1 == SymbolType::FUNC)
		ty1 = term1->retty;
	if(ty2 == SymbolType::FUNC)
		ty2 = term2->retty;

	// already same type?
	if(ty1 == ty2)
		return std::make_tuple(false, true);

	if(ty1 == SymbolType::INT && ty2 == SymbolType::SCALAR)
		return std::make_tuple(true, false);
	if(ty1 == SymbolType::SCALAR && ty2 == SymbolType::INT)
		return std::make_tuple(true, true);

	if(ty1 == SymbolType::STRING && ty2 == SymbolType::SCALAR)
		return std::make_tuple(true, true);
	if(ty1 == SymbolType::STRING && ty2 == SymbolType::INT)
		return std::make_tuple(true, true);
	if(ty1 == SymbolType::SCALAR && ty2 == SymbolType::STRING)
		return std::make_tuple(true, false);
	if(ty1 == SymbolType::INT && ty2 == SymbolType::STRING)
		return std::make_tuple(true, false);

	return std::make_tuple(true, true);
}


/**
 * emit code to cast to given type
 */
void ZeroACAsm::cast_to(t_astret ty_to, std::streampos pos)
{
	if(!ty_to)
		return;

	t_vm_byte op = static_cast<t_vm_byte>(OpCode::NOP);

	if(ty_to->ty == SymbolType::STRING)
		op = static_cast<t_vm_byte>(OpCode::TOS);
	else if(ty_to->ty == SymbolType::INT)
		op = static_cast<t_vm_byte>(OpCode::TOI);
	else if(ty_to->ty == SymbolType::SCALAR)
		op = static_cast<t_vm_byte>(OpCode::TOF);

	m_ostr->seekp(pos);
	m_ostr->write(reinterpret_cast<const char*>(&op), sizeof(t_vm_byte));
	m_ostr->seekp(0, std::ios_base::end);
}


t_astret ZeroACAsm::visit(const ASTUMinus* ast)
{
	t_astret term = ast->GetTerm()->accept(this);
	m_ostr->put(static_cast<t_vm_byte>(OpCode::USUB));

	return term;
}


t_astret ZeroACAsm::visit(const ASTPlus* ast)
{
	t_astret term1 = ast->GetTerm1()->accept(this);
	std::streampos term1_pos = m_ostr->tellp();
	// placeholder for potential cast
	m_ostr->put(static_cast<t_vm_byte>(OpCode::NOP));

	t_astret term2 = ast->GetTerm2()->accept(this);
	std::streampos term2_pos = m_ostr->tellp();

	t_astret common_type = term1;

	// cast if needed
	if(auto [needs_cast, cast_to_first] = get_cast_symtype(term1, term2); needs_cast)
	{
		if(cast_to_first)
		{
			cast_to(term1, term2_pos);  // cast second term to first term type
			common_type = term1;
		}
		else
		{
			cast_to(term2, term1_pos);  // cast fist term to second term type
			common_type = term2;
		}
	}

	if(ast->IsInverted())
		m_ostr->put(static_cast<t_vm_byte>(OpCode::SUB));
	else
		m_ostr->put(static_cast<t_vm_byte>(OpCode::ADD));

	return common_type;
}


t_astret ZeroACAsm::visit(const ASTMult* ast)
{
	t_astret term1 = ast->GetTerm1()->accept(this);
	std::streampos term1_pos = m_ostr->tellp();
	// placeholder for potential cast
	m_ostr->put(static_cast<t_vm_byte>(OpCode::NOP));

	t_astret term2 = ast->GetTerm2()->accept(this);
	std::streampos term2_pos = m_ostr->tellp();

	t_astret common_type = term1;

	// cast if needed
	if(auto [needs_cast, cast_to_first] = get_cast_symtype(term1, term2); needs_cast)
	{
		if(cast_to_first)
		{
			cast_to(term1, term2_pos);  // cast second term to first term type
			common_type = term1;
		}
		else
		{
			cast_to(term2, term1_pos);  // cast fist term to second term type
			common_type = term2;
		}
	}

	if(ast->IsInverted())
		m_ostr->put(static_cast<t_vm_byte>(OpCode::DIV));
	else
		m_ostr->put(static_cast<t_vm_byte>(OpCode::MUL));

	return common_type;
}


t_astret ZeroACAsm::visit(const ASTMod* ast)
{
	t_astret term1 = ast->GetTerm1()->accept(this);
	std::streampos term1_pos = m_ostr->tellp();
	// placeholder for potential cast
	m_ostr->put(static_cast<t_vm_byte>(OpCode::NOP));

	t_astret term2 = ast->GetTerm2()->accept(this);
	std::streampos term2_pos = m_ostr->tellp();

	t_astret common_type = term1;

	// cast if needed
	if(auto [needs_cast, cast_to_first] = get_cast_symtype(term1, term2); needs_cast)
	{
		if(cast_to_first)
		{
			cast_to(term1, term2_pos);  // cast second term to first term type
			common_type = term1;
		}
		else
		{
			cast_to(term2, term1_pos);  // cast fist term to second term type
			common_type = term2;
		}
	}

	m_ostr->put(static_cast<t_vm_byte>(OpCode::MOD));

	return common_type;
}


t_astret ZeroACAsm::visit(const ASTPow* ast)
{
	t_astret term1 = ast->GetTerm1()->accept(this);
	std::streampos term1_pos = m_ostr->tellp();
	// placeholder for potential cast
	m_ostr->put(static_cast<t_vm_byte>(OpCode::NOP));

	t_astret term2 = ast->GetTerm2()->accept(this);
	std::streampos term2_pos = m_ostr->tellp();

	t_astret common_type = term1;

	// cast if needed
	if(auto [needs_cast, cast_to_first] = get_cast_symtype(term1, term2); needs_cast)
	{
		if(cast_to_first)
		{
			cast_to(term1, term2_pos);  // cast second term to first term type
			common_type = term1;
		}
		else
		{
			cast_to(term2, term1_pos);  // cast fist term to second term type
			common_type = term2;
		}
	}

	m_ostr->put(static_cast<t_vm_byte>(OpCode::POW));

	return common_type;
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
	ast->GetTerm1()->accept(this);
	ast->GetTerm2()->accept(this);

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
	ast->GetTerm1()->accept(this);
	if(ast->GetTerm2())
		ast->GetTerm2()->accept(this);

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

	t_astret sym_ret = nullptr;

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
			m_local_stack[cur_func] = 0;

		m_local_stack[cur_func] += get_sym_size(sym);
		sym->addr = -m_local_stack[cur_func];

		// optional assignment
		if(ast->GetAssignment())
			ast->GetAssignment()->accept(this);

		if(!sym_ret)
			sym_ret = sym;
	}

	return sym_ret;
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
		m_ostr->put(static_cast<t_vm_byte>(OpCode::RDMEM));

	return sym;
}


t_astret ZeroACAsm::visit(const ASTAssign* ast)
{
	ast->GetExpr()->accept(this);
	t_astret sym_ret = nullptr;

	for(const t_str& varname : ast->GetIdents())
	{
		t_astret sym = get_sym(varname);
		if(!sym)
			throw std::runtime_error("ASTAssign: Variable \"" + varname + "\" is not in symbol table.");
		if(!sym->addr)
			throw std::runtime_error("ASTAssign: Variable \"" + varname + "\" has not been declared.");

		// push variable address
		m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
		m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_BP));
		t_vm_addr addr = static_cast<t_vm_addr>(*sym->addr);
		m_ostr->write(reinterpret_cast<const char*>(&addr), vm_type_size<VMType::ADDR_BP, false>);

		// assign variable
		m_ostr->put(static_cast<t_vm_byte>(OpCode::WRMEM));

		if(!sym_ret)
			sym_ret = sym;
	}

	return sym_ret;
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

	return m_scalar_const;
}


t_astret ZeroACAsm::visit(const ASTNumConst<t_int>* ast)
{
	t_vm_int val = static_cast<t_vm_int>(ast->GetVal());

	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
	// write type descriptor byte
	m_ostr->put(static_cast<t_vm_byte>(VMType::INT));
	// write data
	m_ostr->write(reinterpret_cast<const char*>(&val), vm_type_size<VMType::INT, false>);

	return m_int_const;
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

	return m_str_const;
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

	return func;
}


t_astret ZeroACAsm::visit(const ASTReturn* ast)
{
	if(!m_curscope.size())
		throw std::runtime_error("ASTReturn: Not in a function.");

	/*const t_str& cur_func = *m_curscope.rbegin();
	t_astret func = get_sym(cur_func);
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



// ----------------------------------------------------------------------------
// arrays
// ----------------------------------------------------------------------------
t_astret ZeroACAsm::visit(const ASTArrayAccess* ast)
{
	std::cout << "TODO: ASTArrayAccess" << std::endl;
	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTArrayAssign* ast)
{
	std::cout << "TODO: ASTArrayAssign" << std::endl;
	return nullptr;
}


t_astret ZeroACAsm::visit(const ASTExprList* ast)
{
	t_astret sym_ret = nullptr;
	bool is_arr = ast->IsScalarArray();

	t_vm_addr num_elems = 0;
	for(const auto& elem : ast->GetList())
	{
		t_astret sym = elem->accept(this);

		// make sure all array elements are real
		if(is_arr)
			cast_to(m_scalar_const, m_ostr->tellp());

		if(!sym_ret)
			sym_ret = sym;

		++num_elems;
	}

	// create a vector out of the elements on the stack
	if(is_arr)
	{
		// push number of elements
		m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
		m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_MEM));
		m_ostr->write(reinterpret_cast<const char*>(&num_elems), vm_type_size<VMType::ADDR_MEM, false>);

		m_ostr->put(static_cast<t_vm_byte>(OpCode::MAKEVEC));
	}

	return sym_ret;
}
// ----------------------------------------------------------------------------
