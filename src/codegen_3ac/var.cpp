/**
 * llvm three-address code generator -- variables
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date apr/may-2020
 * @license: see 'LICENSE.GPL' file
 *
 * References:
 *	* https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl03.html
 *	* https://llvm.org/docs/GettingStarted.html
 *	* https://llvm.org/docs/LangRef.html
 */

#include "asm.h"
#include <sstream>


t_astret LLAsm::visit(const ASTVar* ast)
{
	t_astret sym = get_sym(ast->GetIdent());
	if(sym == nullptr)
		throw std::runtime_error("ASTVar: Symbol \"" + ast->GetIdent() + "\" not in symbol table.");

	t_str var = t_str{"%"} + sym->name;

	if(sym->ty == SymbolType::SCALAR || sym->ty == SymbolType::INT)
	{
		t_astret retvar = get_tmp_var(sym->ty, &sym->dims);
		t_str ty = LLAsm::get_type_name(sym->ty);
		(*m_ostr) << "%" << retvar->name << " = load " 
			<< ty  << ", " << ty << "* " << var << "\n";
		return retvar;
	}
	else if(sym->ty == SymbolType::VECTOR || sym->ty == SymbolType::MATRIX)
	{
		return sym;
	}
	else if(sym->ty == SymbolType::STRING)
	{
		return sym;
	}
	else
	{
		throw std::runtime_error("ASTVar: Invalid type for visited variable: \"" + sym->name + "\".");
	}

	return nullptr;
}


t_astret LLAsm::visit(const ASTVarDecl* ast)
{
	for(const auto& varname : ast->GetVariables())
	{
		t_astret sym = get_sym(varname);
		if(!sym)
			throw std::runtime_error("ASTVarDecl: Variable \"" + varname + "\" not in symbol table.");

		t_str ty = LLAsm::get_type_name(sym->ty);

		if(sym->ty == SymbolType::SCALAR || sym->ty == SymbolType::INT)
		{
			(*m_ostr) << "%" << sym->name << " = alloca " << ty << "\n";
		}
		else if(sym->ty == SymbolType::VECTOR || sym->ty == SymbolType::MATRIX)
		{
			std::size_t dim = get_arraydim(sym);

			// allocate the array's memory
			(*m_ostr) << "%" << sym->name << " = alloca [" << dim << " x " << m_real << "]\n";
		}
		else if(sym->ty == SymbolType::STRING)
		{
			std::size_t dim = std::get<0>(sym->dims);

			// allocate the string's memory
			(*m_ostr) << "%" << sym->name << " = alloca [" << dim << " x i8]\n";

			// get a pointer to the string
			t_astret strptr = get_tmp_var(sym->ty);
			(*m_ostr) << "%" << strptr->name << " = getelementptr [" << dim << " x i8], ["
				<< dim << " x i8]* %" << sym->name << ", " << m_int << " 0, " << m_int << " 0\n";

			// set first element to zero
			(*m_ostr) << "store i8 0, i8* %"  << strptr->name << "\n";
		}
		else
		{
			throw std::runtime_error("ASTVarDecl: Invalid type in declaration: \"" + sym->name + "\".");
		}


		// optional assignment
		if(ast->GetAssignment())
			ast->GetAssignment()->accept(this);
	}

	return nullptr;
}


t_astret LLAsm::visit(const ASTAssign* ast)
{
	t_astret expr = ast->GetExpr()->accept(this);

	// multiple assignments
	if(ast->IsMultiAssign())
	{
		if(expr->ty != SymbolType::COMP)
			throw std::runtime_error("ASTAssign: Need a compound symbol for multi-assignment.");

		const auto& vars = ast->GetIdents();

		if(expr->elems.size() != vars.size())
		{
			std::ostringstream ostrerr;
			ostrerr << "ASTAssign: Mismatch in multi-assign size, "
				<< "expected " << vars.size() << " symbols, received "
				<< expr->elems.size() << " symbols.";
			throw std::runtime_error(ostrerr.str());
		}

		std::size_t elemidx = 0;
		for(std::size_t idx=0; idx<vars.size(); ++idx)
		{
			const SymbolPtr retsym = expr->elems[idx];

			const t_str& var = vars[idx];
			t_astret sym = get_sym(var);

			// get current memory block pointer
			t_astret varmemptr = get_tmp_var(SymbolType::STRING);
			(*m_ostr) << "%" << varmemptr->name << " = getelementptr i8, i8* %"
				<< expr->name << ", " << m_int << " " << elemidx << "\n";

			// directly read scalar value from memory block
			if(sym->ty == SymbolType::SCALAR || sym->ty == SymbolType::INT)
			{
				t_str symty = LLAsm::get_type_name(sym->ty);
				t_str retty = LLAsm::get_type_name(retsym->ty);

				t_astret varptr = get_tmp_var(sym->ty);
				(*m_ostr) << "%" << varptr->name << " = bitcast i8* %" << varmemptr->name 
					<< " to " << retty << "*\n";

				t_astret _varval = get_tmp_var(sym->ty);
				(*m_ostr) << "%" << _varval->name << " = load " <<
					retty << ", " << retty << "* %" << varptr->name << "\n";

				if(!check_sym_compat(
					sym->ty, std::get<0>(sym->dims), std::get<1>(sym->dims),
					retsym->ty, std::get<0>(retsym->dims), std::get<1>(retsym->dims)))
				{
					std::ostringstream ostrErr;
					ostrErr << "ASTAssign: Multi-assignment type or dimension mismatch: ";
					ostrErr << Symbol::get_type_name(sym->ty) << "["
						<< std::get<0>(sym->dims) << ", " << std::get<1>(sym->dims)
						<< "] != " << Symbol::get_type_name(retsym->ty) << "["
						<< std::get<0>(retsym->dims) << ", " << std::get<1>(retsym->dims)
						<< "].";
					throw std::runtime_error(ostrErr.str());
				}

				// cast if needed
				_varval = convert_sym(_varval, sym->ty);

				(*m_ostr) << "store " << symty
					<< " %" << _varval->name << ", "<< symty << "* %" << var << "\n";
			}

			// read real array from memory block
			else if(sym->ty == SymbolType::VECTOR || sym->ty == SymbolType::MATRIX)
			{
				cp_mem_vec(varmemptr, sym, false);
			}

			// read char array from memory block
			else if(sym->ty == SymbolType::STRING)
			{
				cp_mem_str(varmemptr, sym, false);
			}

			// nested compound symbols
			else if(sym->ty == SymbolType::COMP)
			{
				cp_mem_comp(varmemptr, sym);
			}

			elemidx += get_bytesize(sym);
		}

		// free heap return value (TODO: check if it really is on the heap)
		(*m_ostr) << "call void @ext_heap_free(i8* %" << expr->name << ")\n";
	}

	// single assignment
	else
	{
		t_str var = ast->GetIdent();
		t_astret sym = get_sym(var);

		if(!check_sym_compat(
			sym->ty, std::get<0>(sym->dims), std::get<1>(sym->dims),
			expr->ty, std::get<0>(expr->dims), std::get<1>(expr->dims)))
		{
			std::ostringstream ostrErr;
			ostrErr << "ASTAssign: Assignment type or dimension mismatch: ";
			ostrErr << Symbol::get_type_name(sym->ty) << "["
				<< std::get<0>(sym->dims) << ", " << std::get<1>(sym->dims)
				<< "] != " << Symbol::get_type_name(expr->ty) << "["
				<< std::get<0>(expr->dims) << ", " << std::get<1>(expr->dims)
				<< "].";
			throw std::runtime_error(ostrErr.str());
		}

		// cast if needed
		expr = convert_sym(expr, sym->ty);

		if(expr->ty == SymbolType::SCALAR || expr->ty == SymbolType::INT)
		{
			t_str ty = LLAsm::get_type_name(expr->ty);
			(*m_ostr) << "store " << ty << " %" << expr->name << ", "<< ty << "* %" << var << "\n";
		}

		else if(sym->ty == SymbolType::VECTOR || sym->ty == SymbolType::MATRIX)
		{
			std::size_t dimDst = get_arraydim(sym);
			std::size_t dimSrc = get_arraydim(expr);

			// copy elements in a loop
			generate_loop(0, dimDst, [this, expr, sym, dimDst, dimSrc](t_astret ctrval)
			{
				// loop statements
				t_astret elemptr_src = get_tmp_var(SymbolType::SCALAR);
				(*m_ostr) << "%" << elemptr_src->name << " = getelementptr ["
					<< dimSrc << " x " << m_real << "], ["
					<< dimSrc << " x " << m_real << "]* %"
					<< expr->name << ", " << m_int << " 0, " << m_int
					<< " %" << ctrval->name << "\n";
				t_astret elemptr_dst = get_tmp_var(SymbolType::SCALAR);
				(*m_ostr) << "%" << elemptr_dst->name << " = getelementptr ["
					<< dimDst << " x " << m_real << "], ["
					<< dimDst << " x " << m_real << "]* %"
					<< sym->name << ", " << m_int << " 0, " << m_int
					<< " %" << ctrval->name << "\n";
				t_astret elem_src = get_tmp_var(SymbolType::SCALAR);
				(*m_ostr) << "%" << elem_src->name << " = load " << m_real
					<< ", " << m_realptr << " %" << elemptr_src->name << "\n";

				(*m_ostr) << "store " << m_real << " %" << elem_src->name
					<< ", " << m_realptr << " %" << elemptr_dst->name << "\n";
			});
		}

		else if(sym->ty == SymbolType::STRING)
		{
			std::size_t src_dim = std::get<0>(expr->dims);
			std::size_t dst_dim = std::get<0>(sym->dims);
			//if(src_dim > dst_dim)	// TODO
			//	throw std::runtime_error("ASTAssign: Buffer of string \"" + sym->name + "\" is not large enough.");
			std::size_t dim = std::min(src_dim, dst_dim);


			// copy elements in a loop
			generate_loop(0, dim, [this, expr, sym, src_dim, dst_dim](t_astret ctrval)
			{
				// loop statements
				t_astret elemptr_src = get_tmp_var();
				(*m_ostr) << "%" << elemptr_src->name << " = getelementptr [" << src_dim << " x i8], ["
					<< src_dim << " x i8]* %"
					<< expr->name << ", " << m_int << " 0, " << m_int
					<< " %" << ctrval->name << "\n";
				t_astret elemptr_dst = get_tmp_var();
				(*m_ostr) << "%" << elemptr_dst->name << " = getelementptr [" << dst_dim << " x i8], ["
					<< dst_dim << " x i8]* %"
					<< sym->name << ", " << m_int << " 0, " << m_int
					<< " %" << ctrval->name << "\n";
				t_astret elem_src = get_tmp_var();
				(*m_ostr) << "%" << elem_src->name << " = load i8, i8* %" << elemptr_src->name << "\n";

				(*m_ostr) << "store i8 %" << elem_src->name << ", i8* %" << elemptr_dst->name << "\n";
			});
		}

		return expr;
	}

	return nullptr;
}


template<class t_real>
std::string real_to_hex(t_real val)
{
	std::ostringstream ostr;
	ostr << "0x";

	const std::uint8_t* p = reinterpret_cast<const std::uint8_t*>(&val);
	for(std::ptrdiff_t i=sizeof(t_real)-1; i>=0; --i)
	{
		ostr << std::setw(2) << std::right << std::setfill('0')
			<< std::hex << static_cast<std::uint16_t>(p[i]);
	}

	return ostr.str();
}


t_astret LLAsm::visit(const ASTNumConst<t_real>* ast)
{
	t_real val = ast->GetVal();
	std::string hexval;
	// need to use double representation for floats,
	// see: https://llvm.org/docs/LangRef.html#simple-constants
	if constexpr(std::is_same_v<std::decay_t<t_real>, float>)
		hexval = real_to_hex<double>(val);
	else
		hexval = real_to_hex<t_real>(val);

	t_astret retvar = get_tmp_var(SymbolType::SCALAR);
	t_astret retval = get_tmp_var(SymbolType::SCALAR);
	(*m_ostr) << "%" << retvar->name << " = alloca " << m_real << "\n";
	(*m_ostr) << "store " << m_real << " " << std::scientific
		<< hexval << ", " << m_realptr << " %" << retvar->name << "\n";
	(*m_ostr) << "%" << retval->name << " = load " << m_real << ", "
		<< m_realptr << " %" << retvar->name << "\n";

	return retval;
}


t_astret LLAsm::visit(const ASTNumConst<t_int>* ast)
{
	t_int val = ast->GetVal();

	t_astret retvar = get_tmp_var(SymbolType::INT);
	t_astret retval = get_tmp_var(SymbolType::INT);
	(*m_ostr) << "%" << retvar->name << " = alloca " << m_int << "\n";
	(*m_ostr) << "store " << m_int << " " << val << ", " << m_intptr
		<< " %" << retvar->name << "\n";
	(*m_ostr) << "%" << retval->name << " = load " << m_int << ", " << m_intptr
		<< " %" << retvar->name << "\n";

	return retval;
}


t_astret LLAsm::visit(const ASTStrConst* ast)
{
	const t_str& str = ast->GetVal();
	std::size_t dim = str.length()+1;

	std::array<std::size_t, 2> dims{{dim, 1}};
	t_astret str_mem = get_tmp_var(SymbolType::STRING, &dims);

	// allocate the string's memory
	(*m_ostr) << "%" << str_mem->name << " = alloca [" << dim << " x i8]\n";

	// set the individual chars
	for(std::size_t idx=0; idx<dim; ++idx)
	{
		t_astret ptr = get_tmp_var();
		(*m_ostr) << "%" << ptr->name << " = getelementptr [" << dim << " x i8], ["
			<< dim << " x i8]* %" << str_mem->name << ", "
			<< m_int << " 0, " << m_int << " " << idx << "\n";

		int val = (idx<dim-1) ? str[idx] : 0;
		(*m_ostr) << "store i8 " << val << ", i8* %"  << ptr->name << "\n";
	}

	return str_mem;
}


t_astret LLAsm::visit(const ASTExprList* ast)
{
	// only real arrays are handled here
	if(!ast->IsScalarArray())
	{
		throw std::runtime_error("ASTExprList: General expression list should not be directly evaluated.");
	}

	// array values and size
	const auto& lst = ast->GetList();
	std::size_t len = lst.size();
	std::array<std::size_t, 2> dims{{len, 1}};

	// allocate real array
	t_astret vec_mem = get_tmp_var(SymbolType::VECTOR, &dims);
	(*m_ostr) << "%" << vec_mem->name << " = alloca [" << len << " x " << m_real << "]\n";

	// set the individual array elements
	auto iter = lst.begin();
	for(std::size_t idx=0; idx<len; ++idx)
	{
		t_astret ptr = get_tmp_var();
		(*m_ostr) << "%" << ptr->name << " = getelementptr [" << len << " x " << m_real << "], ["
			<< len << " x " << m_real << "]* %"
			<< vec_mem->name << ", " << m_int << " 0, " << m_int << " " << idx << "\n";

		t_astret val = (*iter)->accept(this);
		val = convert_sym(val, SymbolType::SCALAR);

		(*m_ostr) << "store " << m_real << " %" << val->name << ", "
			<< m_realptr << " %"  << ptr->name << "\n";
		++iter;
	}

	return vec_mem;
}
