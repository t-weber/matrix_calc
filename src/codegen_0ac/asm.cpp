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
