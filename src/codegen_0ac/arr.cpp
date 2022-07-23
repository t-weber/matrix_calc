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
