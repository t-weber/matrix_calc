/**
 * llvm three-address code generator -- variables, arrays, assignments
 * @author Tobias Weber
 * @date apr/may-2020
 * @license: see 'LICENSE.GPL' file
 *
 * References:
 *	* https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl03.html
 *	* https://llvm.org/docs/GettingStarted.html
 *	* https://llvm.org/docs/LangRef.html
 */

#include "llasm.h"
#include <sstream>


t_astret LLAsm::visit(const ASTVar* ast)
{
	t_astret sym = get_sym(ast->GetIdent());
	if(sym == nullptr)
		throw std::runtime_error("ASTVar: Symbol \"" + ast->GetIdent() + "\" not in symbol table.");

	std::string var = std::string{"%"} + sym->name;

	if(sym->ty == SymbolType::SCALAR || sym->ty == SymbolType::INT)
	{
		t_astret retvar = get_tmp_var(sym->ty, &sym->dims);
		std::string ty = LLAsm::get_type_name(sym->ty);
		(*m_ostr) << "%" << retvar->name << " = load " << ty  << ", " << ty << "* " << var << "\n";
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
	for(const auto& _var : ast->GetVariables())
	{
		t_astret sym = get_sym(_var);
		std::string ty = LLAsm::get_type_name(sym->ty);

		if(sym->ty == SymbolType::SCALAR || sym->ty == SymbolType::INT)
		{
			(*m_ostr) << "%" << sym->name << " = alloca " << ty << "\n";
		}
		else if(sym->ty == SymbolType::VECTOR || sym->ty == SymbolType::MATRIX)
		{
			std::size_t dim = std::get<0>(sym->dims);
			if(sym->ty == SymbolType::MATRIX)
				dim *= std::get<1>(sym->dims);

			// allocate the array's memory
			(*m_ostr) << "%" << sym->name << " = alloca [" << dim << " x double]\n";
		}
		else if(sym->ty == SymbolType::STRING)
		{
			std::size_t dim = std::get<0>(sym->dims);

			// allocate the string's memory
			(*m_ostr) << "%" << sym->name << " = alloca [" << dim << " x i8]\n";

			// get a pointer to the string
			t_astret strptr = get_tmp_var();
			(*m_ostr) << "%" << strptr->name << " = getelementptr [" << dim << " x i8], ["
				<< dim << " x i8]* %" << sym->name << ", i64 0, i64 0\n";

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
		{
			throw std::runtime_error("ASTAssign: Need a compound symbol for multi-assignment.");
		}

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

			const std::string& var = vars[idx];
			t_astret sym = get_sym(var);

			// get current memory block pointer
			t_astret varmemptr = get_tmp_var(SymbolType::STRING);
			(*m_ostr) << "%" << varmemptr->name << " = getelementptr i8, i8* %"
				<< expr->name << ", i64 " << elemidx << "\n";

			// directly read scalar value from memory block
			if(sym->ty == SymbolType::SCALAR || sym->ty == SymbolType::INT)
			{
				std::string symty = LLAsm::get_type_name(sym->ty);
				std::string retty = LLAsm::get_type_name(retsym->ty);

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

				(*m_ostr) << "store " << symty << " %" << _varval->name << ", "<< symty << "* %" << var << "\n";
			}

			// read double array from memory block
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
		(*m_ostr) << "call void @free(i8* %" << expr->name << ")\n";
	}

	// single assignment
	else
	{
		std::string var = ast->GetIdent();
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
			std::string ty = LLAsm::get_type_name(expr->ty);
			(*m_ostr) << "store " << ty << " %" << expr->name << ", "<< ty << "* %" << var << "\n";
		}

		else if(sym->ty == SymbolType::VECTOR || sym->ty == SymbolType::MATRIX)
		{
			std::size_t dimDst = std::get<0>(sym->dims);
			std::size_t dimSrc = std::get<0>(expr->dims);

			if(expr->ty == SymbolType::MATRIX)
				dimSrc *= std::get<1>(expr->dims);
			if(sym->ty == SymbolType::MATRIX)
				dimDst *= std::get<1>(sym->dims);

			// copy elements in a loop
			generate_loop(0, dimDst, [this, expr, sym, dimDst, dimSrc](t_astret ctrval)
			{
				// loop statements
				t_astret elemptr_src = get_tmp_var(SymbolType::SCALAR);
				(*m_ostr) << "%" << elemptr_src->name << " = getelementptr [" << dimSrc << " x double], ["
					<< dimSrc << " x double]* %" << expr->name << ", i64 0, i64 %" << ctrval->name << "\n";
				t_astret elemptr_dst = get_tmp_var(SymbolType::SCALAR);
				(*m_ostr) << "%" << elemptr_dst->name << " = getelementptr [" << dimDst << " x double], ["
					<< dimDst << " x double]* %" << sym->name << ", i64 0, i64 %" << ctrval->name << "\n";
				t_astret elem_src = get_tmp_var(SymbolType::SCALAR);
				(*m_ostr) << "%" << elem_src->name << " = load double, double* %" << elemptr_src->name << "\n";

				(*m_ostr) << "store double %" << elem_src->name << ", double* %" << elemptr_dst->name << "\n";
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
					<< src_dim << " x i8]* %" << expr->name << ", i64 0, i64 %" << ctrval->name << "\n";
				t_astret elemptr_dst = get_tmp_var();
				(*m_ostr) << "%" << elemptr_dst->name << " = getelementptr [" << dst_dim << " x i8], ["
					<< dst_dim << " x i8]* %" << sym->name << ", i64 0, i64 %" << ctrval->name << "\n";
				t_astret elem_src = get_tmp_var();
				(*m_ostr) << "%" << elem_src->name << " = load i8, i8* %" << elemptr_src->name << "\n";

				(*m_ostr) << "store i8 %" << elem_src->name << ", i8* %" << elemptr_dst->name << "\n";
			});
		}

		return expr;
	}

	return nullptr;
}


t_astret LLAsm::visit(const ASTArrayAccess* ast)
{
	// get array indices
	t_astret num1 = ast->GetNum1()->accept(this);
	num1 = convert_sym(num1, SymbolType::INT);

	t_astret num2 = nullptr;
	t_astret num3 = nullptr;
	t_astret num4 = nullptr;

	if(ast->GetNum2())
	{
		num2 = ast->GetNum2()->accept(this);
		num2 = convert_sym(num2, SymbolType::INT);
	}

	if(ast->GetNum3())
	{
		num3 = ast->GetNum3()->accept(this);
		num3 = convert_sym(num3, SymbolType::INT);
	}

	if(ast->GetNum4())
	{
		num4 = ast->GetNum4()->accept(this);
		num4 = convert_sym(num4, SymbolType::INT);
	}

	t_astret term = ast->GetTerm()->accept(this);

	// single-element vector access
	if(term->ty == SymbolType::VECTOR && !ast->IsRanged12())
	{
		if(num2 || num3 || num4)	// no further arguments needed
			throw std::runtime_error("ASTArrayAccess: Invalid access operator for vector \"" + term->name + "\".");

		std::size_t dim = std::get<0>(term->dims);
		num1 = safe_array_index(num1, dim);

		// vector element pointer
		t_astret elemptr = get_tmp_var();
		(*m_ostr) << "%" << elemptr->name << " = getelementptr [" << dim << " x double], ["
			<< dim << " x double]* %" << term->name << ", i64 0, i64 %" << num1->name << "\n";

		// vector element
		t_astret elem = get_tmp_var(SymbolType::SCALAR);
		(*m_ostr) << "%" << elem->name << " = load double, double* %" << elemptr->name << "\n";
		return elem;
	}

	// single-element string access
	else if(term->ty == SymbolType::STRING && !ast->IsRanged12())
	{
		if(num2 || num3 || num4)	// no further arguments needed
			throw std::runtime_error("ASTArrayAccess: Invalid access operator for string \"" + term->name + "\".");

		std::size_t dim = std::get<0>(term->dims);
		num1 = safe_array_index(num1, dim);

		// string element pointer
		t_astret elemptr = get_tmp_var();
		(*m_ostr) << "%" << elemptr->name << " = getelementptr [" << dim << " x i8], ["
			<< dim << " x i8]* %" << term->name << ", i64 0, i64 %" << num1->name << "\n";

		// char at that pointer position
		t_astret elem = get_tmp_var();
		(*m_ostr) << "%" << elem->name << " = load i8, i8* %" << elemptr->name << "\n";

		// return string
		std::array<std::size_t, 2> retdims{{2, 0}};	// one char and a 0
		t_astret str_mem = get_tmp_var(SymbolType::STRING, &retdims);

		// allocate the return string's memory
		(*m_ostr) << "%" << str_mem->name << " = alloca [" << std::get<0>(retdims) << " x i8]\n";

		// store the char
		t_astret ptr0 = get_tmp_var();
		(*m_ostr) << "%" << ptr0->name << " = getelementptr [" << std::get<0>(retdims) << " x i8], ["
			<< std::get<0>(retdims) << " x i8]* %" << str_mem->name << ", i64 0, i64 0\n";

		(*m_ostr) << "store i8 %" << elem->name << ", i8* %" << ptr0->name << "\n";

		// add zero termination
		t_astret ptr1 = get_tmp_var();
		(*m_ostr) << "%" << ptr1->name << " = getelementptr [" << std::get<0>(retdims) << " x i8], ["
			<< std::get<0>(retdims) << " x i8]* %" << str_mem->name << ", i64 0, i64 1\n";

		(*m_ostr) << "store i8 0, i8* %" << ptr1->name << "\n";

		return str_mem;
	}

	// ranged vector and string access
	else if((term->ty == SymbolType::VECTOR || term->ty == SymbolType::STRING) && ast->IsRanged12())
	{
		if(num3 || num4)	// no further arguments needed
			throw std::runtime_error("ASTArrayAccess: Invalid access operator for \"" + term->name + "\".");

		std::string strty, strptrty;
		if(term->ty == SymbolType::VECTOR)
			strty = "double";
		else if(term->ty == SymbolType::STRING)
			strty = "i8";
		strptrty = strty + "*";

		std::size_t dim = std::get<0>(term->dims);
		num1 = safe_array_index(num1, dim);
		num2 = safe_array_index(num2, dim);
		
		// if num1 > num2, the loop has to be in the reversed order
		t_astret num2larger_ptr = get_tmp_var(SymbolType::INT);
		(*m_ostr) << "%" << num2larger_ptr->name << " = alloca i1\n";

		generate_cond([this, num1, num2]() -> t_astret
		{
			t_astret cond = get_tmp_var(SymbolType::INT);
			(*m_ostr) << "%" << cond->name << " = icmp sle i64 %" << num1->name 
				<< ", %" << num2->name << "\n";
			return cond;
		}, [this, num2larger_ptr]
		{
			(*m_ostr) << "store i1 1, i1* %" << num2larger_ptr->name << "\n";
		}, [this, num2larger_ptr]
		{
			(*m_ostr) << "store i1 0, i1* %" << num2larger_ptr->name << "\n";
		}, true);

		t_astret num2larger = get_tmp_var(SymbolType::INT);
		(*m_ostr) << "%" << num2larger->name << " = load i1, i1* %" 
			<< num2larger_ptr->name << "\n";

		// create a new vector for the results 
		// for the moment with full static size, which will be truncated on assignment
		// TODO
		t_astret vec_mem = get_tmp_var(term->ty, &term->dims);
		(*m_ostr) << "%" << vec_mem->name << " = alloca [" << dim << " x " << strty << "]\n";
		
		// assign elements in a loop
		// source vector index counter
		t_astret ctrsrc = get_tmp_var(SymbolType::INT);
		t_astret ctrsrcval = get_tmp_var(SymbolType::INT);
		(*m_ostr) << "%" << ctrsrc->name << " = alloca i64\n";
		(*m_ostr) << "store i64 %" << num1->name << ", i64* %" << ctrsrc->name << "\n";

		// destination vector index counter
		t_astret ctrdst = get_tmp_var(SymbolType::INT);
		t_astret ctrdstval = get_tmp_var(SymbolType::INT);
		(*m_ostr) << "%" << ctrdst->name << " = alloca i64\n";
		(*m_ostr) << "store i64 0, i64* %" << ctrdst->name << "\n";

		generate_loop([this, ctrsrc, ctrsrcval, ctrdst, ctrdstval, num2, num2larger]() -> t_astret
		{
			(*m_ostr) << "%" << ctrsrcval->name << " = load i64, i64* %" 
				<< ctrsrc->name << "\n";

			(*m_ostr) << "%" << ctrdstval->name << " = load i64, i64* %" 
				<< ctrdst->name << "\n";

			t_astret condptr = get_tmp_var();
			(*m_ostr) << "%" << condptr->name << " = alloca i1\n";

			// if num1 > num2, the loop has to be in the reversed order
			generate_cond([num2larger]() -> t_astret
			{
				return num2larger;
			}, [this, condptr, num2, ctrsrcval]
			{
				// ctrsrcval <= num2
				t_astret _cond = get_tmp_var(SymbolType::INT);
				(*m_ostr) << "%" << _cond->name << " = icmp sle i64 %" 
					<< ctrsrcval->name <<  ", %" << num2->name << "\n";
				(*m_ostr) << "store i1 %" << _cond->name 
					<< ", i1* %" << condptr->name << "\n";
			}, [this, condptr, num2, ctrsrcval]
			{
				// ctrsrcval >= num2
				t_astret _cond = get_tmp_var(SymbolType::INT);
				(*m_ostr) << "%" << _cond->name << " = icmp sge i64 %" 
					<< ctrsrcval->name <<  ", %" << num2->name << "\n";
				(*m_ostr) << "store i1 %" << _cond->name 
					<< ", i1* %" << condptr->name << "\n";
			}, true);
		
			t_astret condval = get_tmp_var(SymbolType::INT);
			(*m_ostr) << "%" << condval->name << " = load i1, i1* %" << condptr->name << "\n";
			return condval;
		}, [this, &strty, &strptrty, ctrsrc, ctrsrcval, ctrdst, ctrdstval, 
			term, dim, vec_mem, num2larger]
		{
			// source vector element pointer
			t_astret srcelemptr = get_tmp_var();
			(*m_ostr) << "%" << srcelemptr->name << " = getelementptr [" 
				<< dim << " x "<< strty << "], [" << dim << " x " << strty << "]* %" 
				<< term->name << ", i64 0, i64 %" << ctrsrcval->name << "\n";

			// destination vector element pointer
			t_astret dstelemptr = get_tmp_var();
			(*m_ostr) << "%" << dstelemptr->name << " = getelementptr [" 
				<< dim << " x " << strty << "], [" << dim << " x " << strty << "]* %" 
				<< vec_mem->name << ", i64 0, i64 %" << ctrdstval->name << "\n";

			// source vector element
			t_astret srcelem = get_tmp_var();
			(*m_ostr) << "%" << srcelem->name << " = load " << strty 
				<< ", " << strptrty << " %" << srcelemptr->name << "\n";

			// store to destination vector element pointer
			(*m_ostr) << "store " << strty << " %" << srcelem->name 
				<< ", " << strptrty << " %" << dstelemptr->name << "\n";

			// increment counters
			// if num1 > num2, the loop has to be in the reversed order
			generate_cond([num2larger]() -> t_astret
			{
				return num2larger;
			}, [this, ctrsrcval, ctrsrc]
			{
				// ++ctrsrcval
				t_astret newctrsrcval = get_tmp_var(SymbolType::INT);
				(*m_ostr) << "%" << newctrsrcval->name << " = add i64 %" 
					<< ctrsrcval->name << ", 1\n";
				(*m_ostr) << "store i64 %" << newctrsrcval->name << ", i64* %" 
					<< ctrsrc->name << "\n";
			}, [this, ctrsrcval, ctrsrc]
			{
				// --ctrsrcval
				t_astret newctrsrcval = get_tmp_var(SymbolType::INT);
				(*m_ostr) << "%" << newctrsrcval->name << " = sub i64 %" 
					<< ctrsrcval->name << ", 1\n";
				(*m_ostr) << "store i64 %" << newctrsrcval->name << ", i64* %" 
					<< ctrsrc->name << "\n";
			}, true);

			t_astret newctrdstval = get_tmp_var(SymbolType::INT);
			(*m_ostr) << "%" << newctrdstval->name << " = add i64 %" 
				<< ctrdstval->name << ", 1\n";
			(*m_ostr) << "store i64 %" << newctrdstval->name << ", i64* %" 
				<< ctrdst->name << "\n";
		});

		if(term->ty == SymbolType::STRING)
		{
			// add zero termination
			t_astret _ctrdstval = get_tmp_var(SymbolType::INT);
			(*m_ostr) << "%" << _ctrdstval->name << " = load i64, i64* %" 
				<< ctrdst->name << "\n";
			t_astret dstelemptr = get_tmp_var();
			(*m_ostr) << "%" << dstelemptr->name << " = getelementptr [" 
				<< dim << " x " << strty << "], [" << dim << " x " << strty << "]* %" 
				<< vec_mem->name << ", i64 0, i64 %" << _ctrdstval->name << "\n";

			(*m_ostr) << "store i8 0, i8* %" << dstelemptr->name << "\n";
		}

		return vec_mem;
	}

	// single-element matrix access
	else if(term->ty == SymbolType::MATRIX && !ast->IsRanged12() && !ast->IsRanged34())
	{
		if(!num2 || num3 || num4)	// second argument needed
			throw std::runtime_error("ASTArrayAccess: Invalid access operator for matrix \"" + term->name + "\".");

		std::size_t dim1 = std::get<0>(term->dims);
		std::size_t dim2 = std::get<1>(term->dims);
		num1 = safe_array_index(num1, dim1);
		num2 = safe_array_index(num2, dim2);

		// idx = num1*dim2 + num2
		t_astret idx1 = get_tmp_var();
		t_astret idx = get_tmp_var();
		(*m_ostr) << "%" << idx1->name << " = mul i64 %" << num1->name << ", " << dim2 << "\n";
		(*m_ostr) << "%" << idx->name << " = add i64 %" << idx1->name << ", %" << num2->name << "\n";

		// matrix element pointer
		t_astret elemptr = get_tmp_var();
		(*m_ostr) << "%" << elemptr->name << " = getelementptr [" << dim1*dim2 << " x double], ["
			<< dim1*dim2 << " x double]* %" << term->name << ", i64 0, i64 %" << idx->name << "\n";

		// matrix element
		t_astret elem = get_tmp_var(SymbolType::SCALAR);
		(*m_ostr) << "%" << elem->name << " = load double, double* %" << elemptr->name << "\n";
		return elem;
	}

	// ranged matrix access
	// TODO: allow mixed forms of the type mat[1, 2 .. 3]
	else if(term->ty == SymbolType::MATRIX && (ast->IsRanged12() || ast->IsRanged34()))
	{
		std::size_t dim1 = std::get<0>(term->dims);
		std::size_t dim2 = std::get<1>(term->dims);

		num1 = safe_array_index(num1, dim1);
		num2 = safe_array_index(num2, dim1);
		num3 = safe_array_index(num3, dim2);
		num4 = safe_array_index(num4, dim2);
		
		// if num1 > num2, the loop has to be in the reversed order
		t_astret num2larger_ptr = get_tmp_var(SymbolType::INT);
		(*m_ostr) << "%" << num2larger_ptr->name << " = alloca i1\n";

		generate_cond([this, num1, num2]() -> t_astret
		{
			t_astret cond = get_tmp_var(SymbolType::INT);
			(*m_ostr) << "%" << cond->name << " = icmp sle i64 %" << num1->name 
				<< ", %" << num2->name << "\n";
			return cond;
		}, [this, num2larger_ptr]
		{
			(*m_ostr) << "store i1 1, i1* %" << num2larger_ptr->name << "\n";
		}, [this, num2larger_ptr]
		{
			(*m_ostr) << "store i1 0, i1* %" << num2larger_ptr->name << "\n";
		}, true);

		t_astret num2larger = get_tmp_var(SymbolType::INT);
		(*m_ostr) << "%" << num2larger->name << " = load i1, i1* %" 
			<< num2larger_ptr->name << "\n";

		// if num3 > num4, the loop has to be in the reversed order
		t_astret num4larger_ptr = get_tmp_var(SymbolType::INT);
		(*m_ostr) << "%" << num4larger_ptr->name << " = alloca i1\n";

		generate_cond([this, num3, num4]() -> t_astret
		{
			t_astret cond = get_tmp_var(SymbolType::INT);
			(*m_ostr) << "%" << cond->name << " = icmp sle i64 %" << num3->name 
				<< ", %" << num4->name << "\n";
			return cond;
		}, [this, num4larger_ptr]
		{
			(*m_ostr) << "store i1 1, i1* %" << num4larger_ptr->name << "\n";
		}, [this, num4larger_ptr]
		{
			(*m_ostr) << "store i1 0, i1* %" << num4larger_ptr->name << "\n";
		}, true);

		t_astret num4larger = get_tmp_var(SymbolType::INT);
		(*m_ostr) << "%" << num4larger->name << " = load i1, i1* %" 
			<< num4larger_ptr->name << "\n";

		// create a new vector for the results 
		// for the moment with full static size, which will be truncated on assignment
		// TODO
		std::array<std::size_t, 2> vecdims{{dim1*dim2, 0}};
		t_astret vec_mem = get_tmp_var(SymbolType::VECTOR, &vecdims);
		(*m_ostr) << "%" << vec_mem->name << " = alloca [" << dim1*dim2 << " x double]\n";
		
		// assign elements in a loop
		// source vector index counter
		t_astret ctrsrc1 = get_tmp_var(SymbolType::INT);
		t_astret ctrsrc1val = get_tmp_var(SymbolType::INT);
		(*m_ostr) << "%" << ctrsrc1->name << " = alloca i64\n";
		(*m_ostr) << "store i64 %" << num1->name << ", i64* %" << ctrsrc1->name << "\n";

		t_astret ctrsrc3 = get_tmp_var(SymbolType::INT);
		t_astret ctrsrc3val = get_tmp_var(SymbolType::INT);
		(*m_ostr) << "%" << ctrsrc3->name << " = alloca i64\n";

		// destination vector index counter
		t_astret ctrdst = get_tmp_var(SymbolType::INT);
		t_astret ctrdstval = get_tmp_var(SymbolType::INT);
		(*m_ostr) << "%" << ctrdst->name << " = alloca i64\n";
		(*m_ostr) << "store i64 0, i64* %" << ctrdst->name << "\n";

		generate_loop([this, ctrsrc1, ctrsrc1val, num2, num2larger]() -> t_astret
		{
			(*m_ostr) << "%" << ctrsrc1val->name << " = load i64, i64* %" 
				<< ctrsrc1->name << "\n";

			t_astret cond1ptr = get_tmp_var();
			(*m_ostr) << "%" << cond1ptr->name << " = alloca i1\n";

			// if num1 > num2, the loop has to be in the reversed order
			generate_cond([num2larger]() -> t_astret
			{
				return num2larger;
			}, [this, cond1ptr, num2, ctrsrc1val]
			{
				// ctrsrc1val <= num2
				t_astret _cond = get_tmp_var(SymbolType::INT);
				(*m_ostr) << "%" << _cond->name << " = icmp sle i64 %" 
					<< ctrsrc1val->name <<  ", %" << num2->name << "\n";
				(*m_ostr) << "store i1 %" << _cond->name 
					<< ", i1* %" << cond1ptr->name << "\n";
			}, [this, cond1ptr, num2, ctrsrc1val]
			{
				// ctrsrc1val >= num2
				t_astret _cond = get_tmp_var(SymbolType::INT);
				(*m_ostr) << "%" << _cond->name << " = icmp sge i64 %" 
					<< ctrsrc1val->name <<  ", %" << num2->name << "\n";
				(*m_ostr) << "store i1 %" << _cond->name 
					<< ", i1* %" << cond1ptr->name << "\n";
			}, true);
		
			t_astret cond1val = get_tmp_var(SymbolType::INT);
			(*m_ostr) << "%" << cond1val->name << " = load i1, i1* %" 
				<< cond1ptr->name << "\n";
			return cond1val;
		}, [this, ctrsrc1, ctrsrc1val, ctrdst, ctrdstval, term, 
			dim1, dim2, vec_mem, num2larger, num3, ctrsrc3, ctrsrc3val, 
			num4larger, num4]
		{
			// -----------------------------------------------------------------
			// inner loop
			// -----------------------------------------------------------------
			(*m_ostr) << "store i64 %" << num3->name << ", i64* %" << ctrsrc3->name << "\n";

			generate_loop([this, ctrsrc3, ctrsrc3val, ctrdst, ctrdstval, 
				num4, num4larger]() -> t_astret
			{
				(*m_ostr) << "%" << ctrsrc3val->name << " = load i64, i64* %" 
					<< ctrsrc3->name << "\n";

				(*m_ostr) << "%" << ctrdstval->name << " = load i64, i64* %" 
					<< ctrdst->name << "\n";

				t_astret cond3ptr = get_tmp_var();
				(*m_ostr) << "%" << cond3ptr->name << " = alloca i1\n";

				// if num3 > num4, the loop has to be in the reversed order
				generate_cond([num4larger]() -> t_astret
				{
					return num4larger;
				}, [this, cond3ptr, num4, ctrsrc3val]
				{
					// ctrsrc3val <= num4
					t_astret _cond = get_tmp_var(SymbolType::INT);
					(*m_ostr) << "%" << _cond->name << " = icmp sle i64 %" 
						<< ctrsrc3val->name <<  ", %" << num4->name << "\n";
					(*m_ostr) << "store i1 %" << _cond->name 
						<< ", i1* %" << cond3ptr->name << "\n";
				}, [this, cond3ptr, num4, ctrsrc3val]
				{
					// ctrsrc3val >= num4
					t_astret _cond = get_tmp_var(SymbolType::INT);
					(*m_ostr) << "%" << _cond->name << " = icmp sge i64 %" 
						<< ctrsrc3val->name <<  ", %" << num4->name << "\n";
					(*m_ostr) << "store i1 %" << _cond->name 
						<< ", i1* %" << cond3ptr->name << "\n";
				}, true);
			
				t_astret cond3val = get_tmp_var(SymbolType::INT);
				(*m_ostr) << "%" << cond3val->name << " = load i1, i1* %" 
					<< cond3ptr->name << "\n";
				return cond3val;
			}, [this, ctrsrc1val, ctrsrc3, ctrsrc3val, ctrdst, ctrdstval, 
				term, dim1, dim2, vec_mem, num4larger]
			{
				// idx = num1*dim2 + num2
				t_astret idx1 = get_tmp_var();
				t_astret idx = get_tmp_var();
				(*m_ostr) << "%" << idx1->name << " = mul i64 %" 
					<< ctrsrc1val->name << ", " << dim2 << "\n";
				(*m_ostr) << "%" << idx->name << " = add i64 %" 
					<< idx1->name << ", %" << ctrsrc3val->name << "\n";

				// source matrix element pointer
				t_astret srcelemptr = get_tmp_var();
				(*m_ostr) << "%" << srcelemptr->name << " = getelementptr [" 
					<< dim1*dim2 << " x double], [" << dim1*dim2 << " x double]* %" 
					<< term->name << ", i64 0, i64 %" << idx->name << "\n";

				// destination matrix element pointer
				t_astret dstelemptr = get_tmp_var();
				(*m_ostr) << "%" << dstelemptr->name << " = getelementptr [" 
					<< dim1*dim2 << " x double], [" << dim1*dim2 << " x double]* %" 
					<< vec_mem->name << ", i64 0, i64 %" << ctrdstval->name << "\n";

				// source matrix element
				t_astret srcelem = get_tmp_var(SymbolType::SCALAR);
				(*m_ostr) << "%" << srcelem->name << " = load double, double* %" 
					<< srcelemptr->name << "\n";

				// store to destination matrix element pointer
				(*m_ostr) << "store double %" << srcelem->name << ", double* %" 
					<< dstelemptr->name << "\n";

				// increment counters
				// if num3 > num4, the loop has to be in the reversed order
				generate_cond([num4larger]() -> t_astret
				{
					return num4larger;
				}, [this, ctrsrc3val, ctrsrc3]
				{
					// ++ctrsrcval
					t_astret newctrsrc3val = get_tmp_var(SymbolType::INT);
					(*m_ostr) << "%" << newctrsrc3val->name << " = add i64 %" 
						<< ctrsrc3val->name << ", 1\n";
					(*m_ostr) << "store i64 %" << newctrsrc3val->name << ", i64* %" 
						<< ctrsrc3->name << "\n";
				}, [this, ctrsrc3val, ctrsrc3]
				{
					// --ctrsrcval
					t_astret newctrsrc3val = get_tmp_var(SymbolType::INT);
					(*m_ostr) << "%" << newctrsrc3val->name << " = sub i64 %" 
						<< ctrsrc3val->name << ", 1\n";
					(*m_ostr) << "store i64 %" << newctrsrc3val->name << ", i64* %" 
						<< ctrsrc3->name << "\n";
				}, true);

				t_astret newctrdstval = get_tmp_var(SymbolType::INT);
				(*m_ostr) << "%" << newctrdstval->name << " = add i64 %" 
					<< ctrdstval->name << ", 1\n";
				(*m_ostr) << "store i64 %" << newctrdstval->name << ", i64* %" 
					<< ctrdst->name << "\n";
			});
			// -----------------------------------------------------------------
	
			// increment counters
			// if num1 > num2, the loop has to be in the reversed order
			generate_cond([num2larger]() -> t_astret
			{
				return num2larger;
			}, [this, ctrsrc1val, ctrsrc1]
			{
				// ++ctrsrcval
				t_astret newctrsrc1val = get_tmp_var(SymbolType::INT);
				(*m_ostr) << "%" << newctrsrc1val->name << " = add i64 %" 
					<< ctrsrc1val->name << ", 1\n";
				(*m_ostr) << "store i64 %" << newctrsrc1val->name << ", i64* %" 
					<< ctrsrc1->name << "\n";
			}, [this, ctrsrc1val, ctrsrc1]
			{
				// --ctrsrcval
				t_astret newctrsrc1val = get_tmp_var(SymbolType::INT);
				(*m_ostr) << "%" << newctrsrc1val->name << " = sub i64 %" 
					<< ctrsrc1val->name << ", 1\n";
				(*m_ostr) << "store i64 %" << newctrsrc1val->name << ", i64* %" 
					<< ctrsrc1->name << "\n";
			}, true);
		});

		return vec_mem;
	}

	throw std::runtime_error("ASTArrayAccess: Invalid array access to \"" + term->name + "\".");
	//return nullptr;
}


t_astret LLAsm::visit(const ASTArrayAssign* ast)
{
	std::string var = ast->GetIdent();
	t_astret sym = get_sym(var);

	t_astret expr = ast->GetExpr()->accept(this);

	t_astret num1 = ast->GetNum1()->accept(this);
	num1 = convert_sym(num1, SymbolType::INT);

	t_astret num2 = nullptr;
	if(ast->GetNum2())
	{
		num2 = ast->GetNum2()->accept(this);
		num2 = convert_sym(num2, SymbolType::INT);
	}


	if(sym->ty == SymbolType::VECTOR)
	{
		// cast if needed
		if(expr->ty != SymbolType::SCALAR)
			expr = convert_sym(expr, SymbolType::SCALAR);

		if(num2)	// no second argument needed
			throw std::runtime_error("ASTArrayAssign: Invalid element assignment for vector \"" + sym->name + "\".");

		std::size_t dim = std::get<0>(sym->dims);
		num1 = safe_array_index(num1, dim);

		// vector element pointer
		t_astret elemptr = get_tmp_var();
		(*m_ostr) << "%" << elemptr->name << " = getelementptr [" << dim << " x double], ["
			<< dim << " x double]* %" << sym->name << ", i64 0, i64 %" << num1->name << "\n";

		// assign vector element
		(*m_ostr) << "store double %" << expr->name << ", double* %" << elemptr->name << "\n";
	}

	else if(sym->ty == SymbolType::MATRIX)
	{
		// cast if needed
		expr = convert_sym(expr, SymbolType::SCALAR);

		if(!num2)	// second argument needed
			throw std::runtime_error("ASTArrayAssign: Invalid element assignment for matrix \"" + sym->name + "\".");

		std::size_t dim1 = std::get<0>(sym->dims);
		std::size_t dim2 = std::get<1>(sym->dims);
		num1 = safe_array_index(num1, dim1);
		num2 = safe_array_index(num2, dim2);

		// idx = num1*dim2 + num2
		t_astret idx1 = get_tmp_var();
		t_astret idx = get_tmp_var();
		(*m_ostr) << "%" << idx1->name << " = mul i64 %" << num1->name << ", " << dim2 << "\n";
		(*m_ostr) << "%" << idx->name << " = add i64 %" << idx1->name << ", %" << num2->name << "\n";

		// matrix element pointer
		t_astret elemptr = get_tmp_var();
		(*m_ostr) << "%" << elemptr->name << " = getelementptr [" << dim1*dim2 << " x double], ["
			<< dim1*dim2 << " x double]* %" << sym->name << ", i64 0, i64 %" << idx->name << "\n";

		// assign matrix element
		(*m_ostr) << "store double %" << expr->name << ", double* %" << elemptr->name << "\n";
	}

	else if(sym->ty == SymbolType::STRING)
	{
		// cast if needed
		expr = convert_sym(expr, SymbolType::STRING);

		if(num2)	// no second argument needed
			throw std::runtime_error("ASTArrayAssign: Invalid element assignment for string \"" + sym->name + "\".");

		// target string dimensions
		std::size_t dim = std::get<0>(sym->dims);
		// source string dimensions
		std::size_t dim_src = std::get<0>(expr->dims);

		num1 = safe_array_index(num1, dim);

		// source string element pointer
		t_astret elemptr_src = get_tmp_var();
		(*m_ostr) << "%" << elemptr_src->name << " = getelementptr [" << dim_src << " x i8], ["
			<< dim_src << " x i8]* %" << expr->name << ", i64 0, i64 0\n";

		// char at that pointer position
		t_astret elem_src = get_tmp_var();
		(*m_ostr) << "%" << elem_src->name << " = load i8, i8* %" << elemptr_src->name << "\n";

		// target string element pointer
		t_astret elemptr = get_tmp_var();
		(*m_ostr) << "%" << elemptr->name << " = getelementptr [" << dim << " x i8], ["
			<< dim << " x i8]* %" << sym->name << ", i64 0, i64 %" << num1->name << "\n";

		// assign string element
		(*m_ostr) << "store i8 %" << elem_src->name << ", i8* %" << elemptr->name << "\n";
	}

	return expr;
}


t_astret LLAsm::visit(const ASTNumConst<double>* ast)
{
	double val = ast->GetVal();

	t_astret retvar = get_tmp_var(SymbolType::SCALAR);
	t_astret retvar2 = get_tmp_var(SymbolType::SCALAR);
	(*m_ostr) << "%" << retvar->name << " = alloca double\n";
	(*m_ostr) << "store double " << std::scientific << val << ", double* %" << retvar->name << "\n";
	(*m_ostr) << "%" << retvar2->name << " = load double, double* %" << retvar->name << "\n";

	return retvar2;
}


t_astret LLAsm::visit(const ASTNumConst<std::int64_t>* ast)
{
	std::int64_t val = ast->GetVal();

	t_astret retvar = get_tmp_var(SymbolType::INT);
	t_astret retvar2 = get_tmp_var(SymbolType::INT);
	(*m_ostr) << "%" << retvar->name << " = alloca i64\n";
	(*m_ostr) << "store i64 " << val << ", i64* %" << retvar->name << "\n";
	(*m_ostr) << "%" << retvar2->name << " = load i64, i64* %" << retvar->name << "\n";

	return retvar2;
}


t_astret LLAsm::visit(const ASTStrConst* ast)
{
	const std::string& str = ast->GetVal();
	std::size_t dim = str.length()+1;

	std::array<std::size_t, 2> dims{{dim, 0}};
	t_astret str_mem = get_tmp_var(SymbolType::STRING, &dims);

	// allocate the string's memory
	(*m_ostr) << "%" << str_mem->name << " = alloca [" << dim << " x i8]\n";

	// set the individual chars
	for(std::size_t idx=0; idx<dim; ++idx)
	{
		t_astret ptr = get_tmp_var();
		(*m_ostr) << "%" << ptr->name << " = getelementptr [" << dim << " x i8], ["
			<< dim << " x i8]* %" << str_mem->name << ", i64 0, i64 " << idx << "\n";

		int val = (idx<dim-1) ? str[idx] : 0;
		(*m_ostr) << "store i8 " << val << ", i8* %"  << ptr->name << "\n";
	}

	return str_mem;
}
