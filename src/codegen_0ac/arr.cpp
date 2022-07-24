/**
 * zero-address code generator
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 10-july-2022
 * @license: see 'LICENSE.GPL' file
 */

#include "asm.h"


// ----------------------------------------------------------------------------
// arrays
// ----------------------------------------------------------------------------
t_astret ZeroACAsm::visit(const ASTArrayAccess* ast)
{
	t_astret term = ast->GetTerm()->accept(this);

	bool ranged12 = ast->IsRanged12();
	bool ranged34 = ast->IsRanged34();

	const ASTPtr num1 = ast->GetNum1();
	const ASTPtr num2 = ast->GetNum2();
	const ASTPtr num3 = ast->GetNum3();
	const ASTPtr num4 = ast->GetNum4();

	// single-element 1d array access
	if(!ranged12 && !ranged34 && num1 && !num2 && !num3 && !num4)
	{
		t_astret num1sym = num1->accept(this);
		if(num1sym->ty != SymbolType::INT)
			cast_to(m_int_const);

		m_ostr->put(static_cast<t_vm_byte>(OpCode::RDARR1D));

		if(term->ty == SymbolType::STRING)
			return m_str_const;
		else if(term->ty == SymbolType::MATRIX)
			return m_vec_const;
		else
			return m_scalar_const;
	}

	// ranged 1d array access
	else if(ranged12 && !ranged34 && num1 && num2 && !num3 && !num4)
	{
		t_astret num1sym = num1->accept(this);
		if(num1sym->ty != SymbolType::INT)
			cast_to(m_int_const);
		t_astret num2sym = num2->accept(this);
		if(num2sym->ty != SymbolType::INT)
			cast_to(m_int_const);

		m_ostr->put(static_cast<t_vm_byte>(OpCode::RDARR1DR));

		if(term->ty == SymbolType::STRING)
			return m_str_const;
		else if(term->ty == SymbolType::MATRIX)
			return m_mat_const;
		else
			return m_vec_const;
	}

	// single-element 2d array access
	else if(!ranged12 && !ranged34 && num1 && num2 && !num3 && !num4)
	{
		t_astret num1sym = num1->accept(this);
		if(num1sym->ty != SymbolType::INT)
			cast_to(m_int_const);
		t_astret num2sym = num2->accept(this);
		if(num2sym->ty != SymbolType::INT)
			cast_to(m_int_const);

		m_ostr->put(static_cast<t_vm_byte>(OpCode::RDARR2D));

		if(term->ty == SymbolType::MATRIX)
			return m_scalar_const;
	}

	// ranged 2d array access
	else if(ranged12 && ranged34 && num1 && num2 && num3 && num4)
	{
		t_astret num1sym = num1->accept(this);
		if(num1sym->ty != SymbolType::INT)
			cast_to(m_int_const);
		t_astret num2sym = num2->accept(this);
		if(num2sym->ty != SymbolType::INT)
			cast_to(m_int_const);
		t_astret num3sym = num3->accept(this);
		if(num3sym->ty != SymbolType::INT)
			cast_to(m_int_const);
		t_astret num4sym = num4->accept(this);
		if(num4sym->ty != SymbolType::INT)
			cast_to(m_int_const);

		m_ostr->put(static_cast<t_vm_byte>(OpCode::RDARR2DR));

		if(term->ty == SymbolType::MATRIX)
			return m_scalar_const;
	}

	throw std::runtime_error("ASTArrayAccess: Invalid array access to \"" + term->name + "\".");
}


t_astret ZeroACAsm::visit(const ASTArrayAssign* ast)
{
	// get variable from symbol table
	const t_str& varname = ast->GetIdent();
	t_astret sym = get_sym(varname);
	if(!sym)
		throw std::runtime_error("ASTArrayAssign: Variable \"" + varname + "\" is not in symbol table.");
	if(!sym->addr)
		throw std::runtime_error("ASTArrayAssign: Variable \"" + varname + "\" has not been declared.");

	// push variable address
	m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
	m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_BP));
	t_vm_addr addr = static_cast<t_vm_addr>(*sym->addr);
	m_ostr->write(reinterpret_cast<const char*>(&addr), vm_type_size<VMType::ADDR_BP, false>);

	// evaluate the rhs expression
	t_astret expr = ast->GetExpr()->accept(this);

	bool ranged12 = ast->IsRanged12();
	bool ranged34 = ast->IsRanged34();

	const ASTPtr num1 = ast->GetNum1();
	const ASTPtr num2 = ast->GetNum2();
	const ASTPtr num3 = ast->GetNum3();
	const ASTPtr num4 = ast->GetNum4();

	// single-element 1d array assignment
	if(!ranged12 && !ranged34 && num1 && !num2 && !num3 && !num4)
	{
		if(expr->ty != SymbolType::SCALAR)
			cast_to(m_scalar_const);

		t_astret num1sym = num1->accept(this);
		if(num1sym->ty != SymbolType::INT)
			cast_to(m_int_const);

		m_ostr->put(static_cast<t_vm_byte>(OpCode::WRARR1D));
	}

	// single-element 2d array access
	else if(!ranged12 && !ranged34 && num1 && num2 && !num3 && !num4)
	{
		if(expr->ty != SymbolType::SCALAR)
			cast_to(m_scalar_const);

		t_astret num1sym = num1->accept(this);
		if(num1sym->ty != SymbolType::INT)
			cast_to(m_int_const);
		t_astret num2sym = num2->accept(this);
		if(num2sym->ty != SymbolType::INT)
			cast_to(m_int_const);

		m_ostr->put(static_cast<t_vm_byte>(OpCode::WRARR2D));
	}

	return expr;
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
			cast_to(m_scalar_const);

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
		m_ostr->write(reinterpret_cast<const char*>(&num_elems),
			vm_type_size<VMType::ADDR_MEM, false>);

		m_ostr->put(static_cast<t_vm_byte>(OpCode::MAKEVEC));
		sym_ret = m_vec_const;
	}

	return sym_ret;
}
// ----------------------------------------------------------------------------
