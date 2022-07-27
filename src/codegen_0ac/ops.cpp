/**
 * zero-address code generator
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 10-july-2022
 * @license: see 'LICENSE.GPL' file
 */

#include "asm.h"


// ----------------------------------------------------------------------------
// operations
// ----------------------------------------------------------------------------
Symbol* ZeroACAsm::GetTypeConst(SymbolType ty) const
{
	switch(ty)
	{
		case SymbolType::SCALAR:
			return m_scalar_const;
		case SymbolType::INT:
			return m_int_const;
		case SymbolType::STRING:
			return m_str_const;
		case SymbolType::MATRIX:
			return m_mat_const;
		case SymbolType::VECTOR:
			return m_vec_const;
		default:
			return nullptr;
	}
	return nullptr;
}


/**
 * returns common type of a binary operation
 * @returns [ needs_cast?, first type cast, second type cast, operation result type ]
 */
std::tuple<t_astret, t_astret, t_astret>
ZeroACAsm::GetCastSymType(t_astret term1, t_astret term2)
{
	if(!term1 || !term2)
		return std::make_tuple(nullptr, nullptr, nullptr);

	SymbolType ty1 = term1->ty;
	SymbolType ty2 = term2->ty;

	// use return type for function
	if(ty1 == SymbolType::FUNC)
		term1 = GetTypeConst(term1->retty);
	if(ty2 == SymbolType::FUNC)
		term2 = GetTypeConst(term2->retty);

	// already same type?
	if(ty1 == ty2)
		return std::make_tuple(nullptr, nullptr, term1);

	else if(ty1 == SymbolType::INT && ty2 == SymbolType::SCALAR)
		return std::make_tuple(term2, nullptr, term2);
	else if(ty1 == SymbolType::SCALAR && ty2 == SymbolType::INT)
		return std::make_tuple(nullptr, term1, term1);

	else if(ty1 == SymbolType::STRING && ty2 == SymbolType::SCALAR)
		return std::make_tuple(nullptr, term1, term1);
	else if(ty1 == SymbolType::STRING && ty2 == SymbolType::INT)
		return std::make_tuple(nullptr, term1, term1);
	else if(ty1 == SymbolType::SCALAR && ty2 == SymbolType::STRING)
		return std::make_tuple(term2, nullptr, term2);
	else if(ty1 == SymbolType::INT && ty2 == SymbolType::STRING)
		return std::make_tuple(term2, nullptr, term2);

	// no casts between matrix/scalar operations
	else if(ty1 == SymbolType::MATRIX && ty2 == SymbolType::SCALAR)
		return std::make_tuple(nullptr, nullptr, term1);
	else if(ty1 == SymbolType::MATRIX && ty2 == SymbolType::INT)
		return std::make_tuple(nullptr, m_scalar_const, term1);
	else if(ty1 == SymbolType::SCALAR && ty2 == SymbolType::MATRIX)
		return std::make_tuple(nullptr, nullptr, term2);
	else if(ty1 == SymbolType::INT && ty2 == SymbolType::MATRIX)
		return std::make_tuple(m_scalar_const, nullptr, term2);

	// no casts between vector/scalar operations
	else if(ty1 == SymbolType::VECTOR && ty2 == SymbolType::SCALAR)
		return std::make_tuple(nullptr, nullptr, term1);
	else if(ty1 == SymbolType::VECTOR && ty2 == SymbolType::INT)
		return std::make_tuple(nullptr, m_scalar_const, term1);
	else if(ty1 == SymbolType::SCALAR && ty2 == SymbolType::VECTOR)
		return std::make_tuple(nullptr, nullptr, term2);
	else if(ty1 == SymbolType::INT && ty2 == SymbolType::VECTOR)
		return std::make_tuple(m_scalar_const, nullptr, term2);

	return std::make_tuple(nullptr, term1, term1);
}


/**
 * emit code to cast to given type
 */
void ZeroACAsm::CastTo(t_astret ty_to,
	const std::optional<std::streampos>& pos,
	bool allow_array_cast)
{
	if(!ty_to)
		return;

	t_vm_byte op = static_cast<t_vm_byte>(OpCode::NOP);

	if(ty_to->ty == SymbolType::STRING)
	{
		op = static_cast<t_vm_byte>(OpCode::TOS);
	}
	else if(ty_to->ty == SymbolType::INT)
	{
		op = static_cast<t_vm_byte>(OpCode::TOI);
	}
	else if(ty_to->ty == SymbolType::SCALAR)
	{
		op = static_cast<t_vm_byte>(OpCode::TOF);
	}
	else if(ty_to->ty == SymbolType::VECTOR && allow_array_cast)
	{
		op = static_cast<t_vm_byte>(OpCode::TOV);

		// TODO: this doesn't work if "pos" is also given
		// push vector length
		t_vm_addr cols = static_cast<t_vm_addr>(std::get<0>(ty_to->dims));

		m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
		m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_MEM));
		m_ostr->write(reinterpret_cast<const char*>(&cols),
			vm_type_size<VMType::ADDR_MEM, false>);
	}
	else if(ty_to->ty == SymbolType::MATRIX && allow_array_cast)
	{
		op = static_cast<t_vm_byte>(OpCode::TOM);

		// TODO: this doesn't work if "pos" is also given
		// push number of columns
		t_vm_addr cols = static_cast<t_vm_addr>(std::get<0>(ty_to->dims));
		m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
		m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_MEM));
		m_ostr->write(reinterpret_cast<const char*>(&cols),
			vm_type_size<VMType::ADDR_MEM, false>);

		// push number of rows
		t_vm_addr rows = static_cast<t_vm_addr>(std::get<1>(ty_to->dims));
		m_ostr->put(static_cast<t_vm_byte>(OpCode::PUSH));
		m_ostr->put(static_cast<t_vm_byte>(VMType::ADDR_MEM));
		m_ostr->write(reinterpret_cast<const char*>(&rows),
			vm_type_size<VMType::ADDR_MEM, false>);
	}

	if(pos)
		m_ostr->seekp(*pos);
	m_ostr->write(reinterpret_cast<const char*>(&op), sizeof(t_vm_byte));
	if(pos)
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
	auto [first_ty, second_ty, res_ty]
		= GetCastSymType(term1, term2);
	if(first_ty)
		CastTo(first_ty, term1_pos);
	if(second_ty)
		CastTo(second_ty, term2_pos);
	common_type = res_ty;

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
	auto [first_ty, second_ty, res_ty]
		= GetCastSymType(term1, term2);
	if(first_ty)
		CastTo(first_ty, term1_pos);
	if(second_ty)
		CastTo(second_ty, term2_pos);
	common_type = res_ty;

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
	auto [first_ty, second_ty, res_ty]
		= GetCastSymType(term1, term2);
	if(first_ty)
		CastTo(first_ty, term1_pos);
	if(second_ty)
		CastTo(second_ty, term2_pos);
	common_type = res_ty;

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
	auto [first_ty, second_ty, res_ty]
		= GetCastSymType(term1, term2);
	if(first_ty)
		CastTo(first_ty, term1_pos);
	if(second_ty)
		CastTo(second_ty, term2_pos);
	common_type = res_ty;

	m_ostr->put(static_cast<t_vm_byte>(OpCode::POW));

	return common_type;
}


t_astret ZeroACAsm::visit(const ASTTransp* ast)
{
	t_astret term = ast->GetTerm()->accept(this);

	if(term->ty == SymbolType::MATRIX)
	{
		CallExternal("transpose");
	}
	else
	{
		throw std::runtime_error(
			"ASTTrans: Transposing is not possible for \""
				+ term->name + "\".");
	}

	return term;
}


t_astret ZeroACAsm::visit(const ASTNorm* ast)
{
	t_astret term = ast->GetTerm()->accept(this);
	CallExternal("norm");
	return term;
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
