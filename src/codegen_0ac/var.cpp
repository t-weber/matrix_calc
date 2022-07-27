/**
 * zero-address code generator
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 10-july-2022
 * @license: see 'LICENSE.GPL' file
 */

#include "asm.h"


/**
 * find the symbol with a specific name in the symbol table
 */
t_astret ZeroACAsm::GetSym(const t_str& name) const
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
		throw std::runtime_error("GetSym: \"" + scoped_name +
			"\" does not have an associated symbol.");
	}

	return sym;
}


/**
 * finds the size of the symbol for the stack frame
 */
std::size_t ZeroACAsm::GetSymSize(const Symbol* sym) const
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
		t_astret sym = GetSym(varname);
		if(!sym)
			throw std::runtime_error("ASTVarDecl: Variable \"" + varname + "\" is not in symbol table.");
		if(sym->addr)
			throw std::runtime_error("ASTVarDecl: Variable \"" + varname + "\" already declared.");

		// start of local stack frame?
		if(m_local_stack.find(cur_func) == m_local_stack.end())
			m_local_stack[cur_func] = 0;

		m_local_stack[cur_func] += GetSymSize(sym);
		sym->addr = -m_local_stack[cur_func];

		if(ast->GetAssignment())
		{
			// initialise variable using given assignment
			ast->GetAssignment()->accept(this);
		}
		else
		{
			// initialise variable to 0 if no assignment is given
			if(sym->ty == SymbolType::INT)
			{
				PushIntConst(t_vm_int(0));
				AssignVar(sym);
			}
			else if(sym->ty == SymbolType::SCALAR)
			{
				PushRealConst(t_vm_real(0));
				AssignVar(sym);
			}
			else if(sym->ty == SymbolType::STRING)
			{
				PushStrConst(t_vm_str(""));
				AssignVar(sym);
			}
			else if(sym->ty == SymbolType::VECTOR)
			{
				std::vector<t_vm_real> vec(std::get<0>(sym->dims));
				PushVecConst(vec);
				AssignVar(sym);
			}
			else if(sym->ty == SymbolType::MATRIX)
			{
				t_vm_addr rows = static_cast<t_vm_addr>(
					std::get<0>(sym->dims));
				t_vm_addr cols = static_cast<t_vm_addr>(
					std::get<1>(sym->dims));

				std::vector<t_vm_real> mat(rows * cols);
				PushMatConst(rows, cols, mat);
				AssignVar(sym);
			}
		}

		if(!sym_ret)
			sym_ret = sym;
	}

	return sym_ret;
}


t_astret ZeroACAsm::visit(const ASTVar* ast)
{
	const t_str& varname = ast->GetIdent();

	// get variable from symbol table
	t_astret sym = GetSym(varname);
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


/**
 * assign symbol variable to current value on the stack
 */
void ZeroACAsm::AssignVar(t_astret sym)
{
	// push variable address
	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
	m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_BP));
	t_vm_addr addr = static_cast<t_vm_addr>(*sym->addr);
	m_ostr->write(reinterpret_cast<const char*>(&addr),
		vm_type_size<VMType::ADDR_BP, false>);

	// assign variable
	m_ostr->put(static_cast<t_vm_byte>(OpCode::WRMEM));
}


t_astret ZeroACAsm::visit(const ASTAssign* ast)
{
	ast->GetExpr()->accept(this);
	t_astret sym_ret = nullptr;

	for(const t_str& varname : ast->GetIdents())
	{
		t_astret sym = GetSym(varname);
		if(!sym)
			throw std::runtime_error("ASTAssign: Variable \"" + varname + "\" is not in symbol table.");
		if(!sym->addr)
			throw std::runtime_error("ASTAssign: Variable \"" + varname + "\" has not been declared.");

		CastTo(sym, std::nullopt, true);
		AssignVar(sym);

		if(!sym_ret)
			sym_ret = sym;
	}

	return sym_ret;
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

void ZeroACAsm::PushRealConst(t_vm_real val)
{
	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
	// write type descriptor byte
	m_ostr->put(static_cast<t_vm_byte>(VMType::REAL));
	// write value
	m_ostr->write(reinterpret_cast<const char*>(&val),
		vm_type_size<VMType::REAL, false>);
}


void ZeroACAsm::PushIntConst(t_vm_int val)
{
	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
	// write type descriptor byte
	m_ostr->put(static_cast<t_vm_byte>(VMType::INT));
	// write data
	m_ostr->write(reinterpret_cast<const char*>(&val),
		vm_type_size<VMType::INT, false>);
}


void ZeroACAsm::PushStrConst(const t_vm_str& val)
{
	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
	// write type descriptor byte
	m_ostr->put(static_cast<t_vm_byte>(VMType::STR));
	// write string length
	t_vm_addr len = static_cast<t_vm_addr>(val.length());
	m_ostr->write(reinterpret_cast<const char*>(&len),
		vm_type_size<VMType::ADDR_MEM, false>);
	// write string data
	m_ostr->write(val.data(), len);
}


void ZeroACAsm::PushVecConst(const std::vector<t_vm_real>& vec)
{
	// push elements
	for(t_vm_real val : vec)
		PushRealConst(val);

	// push number of elements
	t_vm_addr num_elems = static_cast<t_vm_addr>(vec.size());
	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
	m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_MEM));
	m_ostr->write(reinterpret_cast<const char*>(&num_elems),
		vm_type_size<VMType::ADDR_MEM, false>);

	m_ostr->put(static_cast<t_vm_byte>(OpCode::MAKEVEC));
}


void ZeroACAsm::PushMatConst(t_vm_addr rows, t_vm_addr cols, const std::vector<t_vm_real>& mat)
{
	// push elements
	for(t_vm_addr i=0; i<rows; ++i)
		for(t_vm_addr j=0; j<cols; ++j)
			PushRealConst(mat[i*cols + j]);

	// push number of columns
	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
	m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_MEM));
	m_ostr->write(reinterpret_cast<const char*>(&cols),
		vm_type_size<VMType::ADDR_MEM, false>);

	// push number of rows
	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
	m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_MEM));
	m_ostr->write(reinterpret_cast<const char*>(&rows),
		vm_type_size<VMType::ADDR_MEM, false>);

	m_ostr->put(static_cast<t_vm_byte>(OpCode::MAKEMAT));
}


t_astret ZeroACAsm::visit(const ASTNumConst<t_real>* ast)
{
	t_vm_real val = static_cast<t_vm_real>(ast->GetVal());
	PushRealConst(val);
	return m_scalar_const;
}


t_astret ZeroACAsm::visit(const ASTNumConst<t_int>* ast)
{
	t_vm_int val = static_cast<t_vm_int>(ast->GetVal());
	PushIntConst(val);
	return m_int_const;
}


t_astret ZeroACAsm::visit(const ASTStrConst* ast)
{
	const t_str& val = ast->GetVal();
	PushStrConst(val);
	return m_str_const;
}
// ----------------------------------------------------------------------------
