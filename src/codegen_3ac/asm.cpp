/**
 * llvm three-address code generator
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


// type names
const t_str LLAsm::m_real = get_lltype_name<t_real>();
const t_str LLAsm::m_int = get_lltype_name<t_int>();
const t_str LLAsm::m_realptr = t_str(get_lltype_name<t_real>()) + "*";
const t_str LLAsm::m_intptr = t_str(get_lltype_name<t_int>()) + "*";



LLAsm::LLAsm(SymTab* syms, std::ostream* ostr) : m_syms{syms}, m_ostr{ostr}
{}


t_astret LLAsm::visit(const ASTStmts* ast)
{
	t_astret lastres = nullptr;
	t_str block = get_block_label();
	(*m_ostr) << "%" << block << " = call i8* @llvm.stacksave()\n";

	for(const auto& stmt : ast->GetStatementList())
		lastres = stmt->accept(this);

	// remove stack variables, limit scope to current block
	(*m_ostr) << "call void @llvm.stackrestore(i8* %" << block << ")\n";
	return lastres;
}


t_astret LLAsm::visit(const ASTCond* ast)
{
	generate_cond([this, ast]() -> t_astret
	{
		return ast->GetCond()->accept(this);
	}, [this, ast]
	{
		ast->GetIf()->accept(this);
	}, [this, ast]
	{
		ast->GetElse()->accept(this);
	}, ast->HasElse());

	return nullptr;
}


t_astret LLAsm::visit(const ASTLoop* ast)
{
	generate_loop([this, ast]() -> t_astret
	{
		t_astret cond = ast->GetCond()->accept(this);
		return cond;
	}, [this, ast]
	{
		ast->GetLoopStmt()->accept(this);
	});

	return nullptr;
}


// ----------------------------------------------------------------------------


t_astret LLAsm::get_tmp_var(SymbolType ty,
	const std::array<std::size_t, 2>* dims,
	const t_str* name)
{
	t_str var;
	if(name)
		var = *name;

	// create a unique temporary name
	if(!name || var == "")
	{
		var = "__tmp_";
		var += std::to_string(m_varCount);
		++m_varCount;
	}

	// if the symbol is already known (e.g. for parameters), update and use it
	if(name)
	{
		Symbol *sym = const_cast<Symbol*>(get_sym(*name));
		if(sym)
		{
			sym->ty = ty;
			if(dims)
				sym->dims = *dims;
			return sym;
		}
	}

	if(dims)
		return m_syms->AddSymbol("", var, ty, *dims, true);
	else
		return m_syms->AddSymbol("", var, ty, {1,1}, true);
}


t_str LLAsm::get_label()
{
	t_str lab{"__lab_"};
	lab += std::to_string(m_labelCount);
	++m_labelCount;
	return lab;
}


t_str LLAsm::get_block_label()
{
	t_str lab{"__block_"};
	lab += std::to_string(m_labelCountBlock);
	++m_labelCountBlock;
	return lab;
}


/**
 * output declarations for registered functions
 */
t_str LLAsm::get_function_declarations(const SymTab& symtab, bool only_externals)
{
	std::ostringstream ostr;

	for(const auto& _sym : symtab.GetSymbols())
	{
		const Symbol& sym = std::get<1>(_sym);

		if(sym.ty != SymbolType::FUNC)
			continue;

		// consider only external runtime functions
		if(only_externals && !sym.is_external)
			continue;

		ostr << "declare " << get_type_name(sym.retty);

		// if the function has an external name assigned, use it
		if(sym.ext_name)
			ostr << " @" << *sym.ext_name << "(";
		else
			ostr << " @" << sym.name << "(";

		for(std::size_t arg=0; arg<sym.argty.size(); ++arg)
		{
			SymbolType ty = sym.argty[arg];
			ostr << get_type_name(ty);

			// add size arguments for array types
			if(ty == SymbolType::VECTOR || ty == SymbolType::MATRIX)
				ostr << ", " << m_int;	// first dim
			if(ty == SymbolType::MATRIX)
				ostr << ", " << m_int;	// second dim

			if(arg < sym.argty.size()-1)
				ostr << ", ";
		}

		ostr << ")\n";
	}

	return ostr.str();
}


/**
 * get the corresponding data type name
 */
t_str LLAsm::get_type_name(SymbolType ty)
{
	switch(ty)
	{
		case SymbolType::SCALAR: return m_real;
		case SymbolType::VECTOR: return m_realptr;
		case SymbolType::MATRIX: return m_realptr;
		case SymbolType::STRING: return "i8*";
		case SymbolType::INT: return m_int;
		case SymbolType::VOID: return "void";
		case SymbolType::COMP: return "i8*";	// pointer to memory block
		//case SymbolType::FUNC: return "func";	// TODO: function pointers
		default: return "invalid";
	}

	return "invalid";
}


/**
 * get the element type for an array type
 */
SymbolType LLAsm::get_element_type(SymbolType ty)
{
	if(ty == SymbolType::VECTOR)
		return SymbolType::SCALAR;
	else if(ty == SymbolType::MATRIX)
		return SymbolType::SCALAR;
	else if(ty == SymbolType::STRING)
		return SymbolType::STRING;

	return SymbolType::UNKNOWN;
}


/**
 * get the (static) byte size of a symbol
 */
std::size_t LLAsm::get_bytesize(t_astret sym)
{
	SymbolType ty = sym->ty;

	switch(ty)
	{
		case SymbolType::SCALAR:
			return sizeof(t_real);
		case SymbolType::VECTOR:
			return sizeof(t_real)*std::get<0>(sym->dims);
		case SymbolType::MATRIX:
			return sizeof(t_real)*std::get<0>(sym->dims)*std::get<1>(sym->dims);
		case SymbolType::STRING:
			return sizeof(char)*std::get<0>(sym->dims);
		case SymbolType::INT:
			return sizeof(t_int);
		case SymbolType::VOID:
			return 0;
		case SymbolType::COMP:
		{
			std::size_t size = 0;
			for(const SymbolPtr& thesym : sym->elems)
				size += get_bytesize(thesym.get());
			return size;
		}
		case SymbolType::FUNC:
			return sizeof(void*);
		default:
			return 0;
	}

	return 0;
}


/**
 * get the dimensions of an array type
 */
std::size_t LLAsm::get_arraydim(t_astret sym)
{
	return ::get_arraydim<2>(sym->dims);
}


/**
 * find the symbol with a specific name in the symbol table
 */
t_astret LLAsm::get_sym(const t_str& name) const
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

	if(sym==nullptr)
		throw std::runtime_error("get_sym: \"" + scoped_name + "\" does not have an associated symbol.");

	//++sym->refcnt;	// increment symbol's reference counter
	return sym;
}


/**
 * convert symbol to another type
 */
t_astret LLAsm::convert_sym(t_astret sym, SymbolType ty_to)
{
	// already the correct type
	if(sym->ty == ty_to)
		return sym;

	// re-interpret vector as matrix
	else if(ty_to == SymbolType::MATRIX && sym->ty == SymbolType::VECTOR)
		return sym;


	// scalar conversions
	if(ty_to == SymbolType::SCALAR || ty_to == SymbolType::INT)
	{
		t_str op;
		if(sym->ty == SymbolType::INT && ty_to == SymbolType::SCALAR)
			op = "sitofp";
		else if(sym->ty == SymbolType::SCALAR && ty_to == SymbolType::INT)
			op = "fptosi";

		if(op == "")
			throw std::runtime_error("Invalid scalar type conversion.");

		t_str from = LLAsm::get_type_name(sym->ty);
		t_str to = LLAsm::get_type_name(ty_to);

		t_astret var = get_tmp_var(ty_to, &sym->dims);
		(*m_ostr) << "%" << var->name << " = " << op << " " << from << "%" << sym->name << " to " << to << "\n";
		return var;
	}

	// conversions to string
	else if(ty_to == SymbolType::STRING)
	{
		// ... from int
		if(sym->ty == SymbolType::INT)
		{
			std::size_t len = 32;
			std::array<std::size_t, 2> dims{{len, 1}};
			t_astret str_mem = get_tmp_var(SymbolType::STRING, &dims);
			t_astret strptr = get_tmp_var(SymbolType::STRING, &dims);

			(*m_ostr) << "%" << str_mem->name << " = alloca [" << len << " x i8]\n";
			(*m_ostr) << "%" << strptr->name << " = getelementptr ["
				<< len << " x i8], ["
				<< len << " x i8]* %"
				<< str_mem->name << ", " << m_int << " 0, " << m_int << " 0\n";

			(*m_ostr) << "call void @int_to_str(" << m_int
				<< " %"  << sym->name << ", i8* %" << strptr->name
				<< ", " << m_int << " " << len << ")\n";
			return str_mem;
		}

		// ... from (t_real) scalar
		else if(sym->ty == SymbolType::SCALAR)
		{
			std::size_t len = 32;
			std::array<std::size_t, 2> dims{{len, 1}};
			t_astret str_mem = get_tmp_var(SymbolType::STRING, &dims);
			t_astret strptr = get_tmp_var(SymbolType::STRING, &dims);

			(*m_ostr) << "%" << str_mem->name << " = alloca [" << len << " x i8]\n";
			(*m_ostr) << "%" << strptr->name << " = getelementptr ["
				<< len << " x i8], ["
				<< len << " x i8]* %"
				<< str_mem->name << ", " << m_int << " 0, " << m_int << " 0\n";

			(*m_ostr) << "call void @flt_to_str(" << m_real << " %"  << sym->name
				<< ", i8* %" << strptr->name << ", " << m_int << " " << len << ")\n";
			return str_mem;
		}

		// ... from vector or matrix
		else if(sym->ty == SymbolType::VECTOR || sym->ty == SymbolType::MATRIX)
		{
			std::size_t num_floats = get_arraydim(sym);
			std::size_t len = 32 * num_floats;
			std::array<std::size_t, 2> dims{{len, 1}};

			t_astret str_mem = get_tmp_var(SymbolType::STRING, &dims);
			t_astret strptr = get_tmp_var(SymbolType::STRING, &dims);

			(*m_ostr) << "%" << str_mem->name << " = alloca [" << len << " x i8]\n";
			(*m_ostr) << "%" << strptr->name << " = getelementptr ["
				<< len << " x i8], ["
				<< len << " x i8]* %"
				<< str_mem->name << ", " << m_int << " 0, " << m_int << " 0\n";


			// prepare "[ ", "] ", ", ", and "; " strings
			t_astret vecbegin = get_tmp_var(SymbolType::STRING);
			t_astret vecend = get_tmp_var(SymbolType::STRING);
			t_astret vecsep = get_tmp_var(SymbolType::STRING);
			t_astret matsep = nullptr;

			(*m_ostr) << "%" << vecbegin->name << " = bitcast [3 x i8]* @__str_vecbegin to i8*\n";
			(*m_ostr) << "%" << vecend->name << " = bitcast [3 x i8]* @__str_vecend to i8*\n";
			(*m_ostr) << "%" << vecsep->name << " = bitcast [3 x i8]* @__str_vecsep to i8*\n";

			if(sym->ty == SymbolType::MATRIX)
			{
				matsep = get_tmp_var(SymbolType::STRING);
				(*m_ostr) << "%" << matsep->name << " = bitcast [3 x i8]* @__str_matsep to i8*\n";
			}


			// vector start: "[ "
			(*m_ostr) << "call i8* @strncpy(i8* %" << strptr->name << ", i8* %"
				<< vecbegin->name << ", " << m_int << " 3)\n";


			// output array elements in a loop
			generate_loop(0, num_floats, [this, num_floats, sym, strptr, matsep, vecsep](t_astret ctrval)
			{	// loop statements
				// get vector/matrix element
				t_astret elemptr = get_tmp_var();
				t_astret elem = get_tmp_var();

				(*m_ostr) << "%" << elemptr->name << " = getelementptr ["
					<< num_floats << " x " << m_real << "], ["
					<< num_floats << " x " << m_real << "]* %"
					<< sym->name << ", " << m_int << " 0, " << m_int
					<< " %" << ctrval->name << "\n";
				(*m_ostr) << "%" << elem->name << " = load "
					<< m_real << ", " << m_realptr
					<< " %" << elemptr->name << "\n";


				// convert vector/matrix component to string
				std::size_t lenComp = 32;
				std::array<std::size_t, 2> dimsComp{{lenComp, 1}};
				t_astret strComp_mem = get_tmp_var(SymbolType::STRING, &dimsComp);
				t_astret strCompptr = get_tmp_var(SymbolType::STRING, &dimsComp);

				(*m_ostr) << "%" << strComp_mem->name << " = alloca [" << lenComp << " x i8]\n";
				(*m_ostr) << "%" << strCompptr->name << " = getelementptr ["
					<< lenComp << " x i8], ["
					<< lenComp << " x i8]* %"
					<< strComp_mem->name << ", " << m_int << " 0, " << m_int << " 0\n";

				(*m_ostr) << "call void @flt_to_str(" << m_real << " %"  << elem->name
					<< ", i8* %" << strCompptr->name << ", " << m_int << " " << lenComp << ")\n";

				(*m_ostr) << "call i8* @strncat(i8* %" << strptr->name << ", i8* %"
					<< strCompptr->name << ", " << m_int << " " << lenComp << ")\n";


				// separator ", " or "; "
				if(sym->ty == SymbolType::MATRIX)
				{
					generate_cond([this, ctrval, num_floats]() -> t_astret
					{
						// don't output last "; " or ", ": i < num_floats-1
						t_astret _cond = get_tmp_var(SymbolType::INT);
						(*m_ostr) << "%" << _cond->name << " = icmp slt " << m_int
							<< " %" << ctrval->name << ", " << num_floats-1 << "\n";
						return _cond;
					}, [this, sym, ctrval, strptr, matsep, vecsep]
					{
						t_astret ctr_1 = get_tmp_var(SymbolType::INT);
						t_astret ctr_mod_cols = get_tmp_var(SymbolType::INT);

						generate_cond([this, sym, ctrval, ctr_1, ctr_mod_cols]() -> t_astret
						{
							//output ';' or ','?: ((i+1) % std::get<1>(sym->dims)) == 0
							t_astret cond_M_or_v = get_tmp_var(SymbolType::INT);
							(*m_ostr) << "%" << ctr_1->name << " = add " << m_int
								<< " %" << ctrval->name << ", 1\n";
							(*m_ostr) << "%" << ctr_mod_cols->name << " = srem " << m_int
								<< " %" << ctr_1->name
								<< ", " << std::get<0>(sym->dims) << "\n";
							(*m_ostr) << "%" << cond_M_or_v->name << " = icmp eq " << m_int
								<< " %" << ctr_mod_cols->name << ", 0\n";
							return cond_M_or_v;
						}, [this, strptr, matsep]
						{
							// if: matrix separator case
							(*m_ostr) << "call i8* @strncat(i8* %" << strptr->name
								<< ", i8* %" << matsep->name << ", " << m_int << " 3)\n";
						}, [this, strptr, vecsep]
						{
							// else: vector separator case
							(*m_ostr) << "call i8* @strncat(i8* %" << strptr->name
								<< ", i8* %" << vecsep->name << ", " << m_int << " 3)\n";

						}, true);
					}, []{}, false);
				}

				else if(sym->ty == SymbolType::VECTOR)
				{
					generate_cond([this, ctrval, num_floats]() -> t_astret
					{
						// don't output last ", ": i < num_floats-1
						t_astret _cond = get_tmp_var(SymbolType::INT);
						(*m_ostr) << "%" << _cond->name << " = icmp slt " << m_int << " %"
							<< ctrval->name << ", " << num_floats-1 << "\n";
						return _cond;
					}, [this, strptr, vecsep]
					{
						(*m_ostr) << "call i8* @strncat(i8* %" << strptr->name
							<< ", i8* %" << vecsep->name << ", " << m_int << " 3)\n";
					}, []{}, false);
				}
			});

			// vector end: " ]"
			(*m_ostr) << "call i8* @strncat(i8* %" << strptr->name << ", i8* %"
				<< vecend->name << ", " << m_int << " 3)\n";

			return str_mem;
		}
	}

	// unknown conversion
	else
	{
		throw std::runtime_error("Invalid type conversion.");
	}

	return nullptr;
}


bool LLAsm::check_sym_compat(
	SymbolType ty1, std::size_t dim1_1, std::size_t dim1_2,
	SymbolType ty2, std::size_t dim2_1, std::size_t dim2_2)
{
	// allow truncating assignments of the form: vec 2 v = [1, 2, 3, 4];
	// needed for ranged array access
	// TODO: remove as soon as ranged access uses dynamic array size
	if(ty1==SymbolType::VECTOR && ty2==SymbolType::VECTOR)
		return dim1_1 <= dim2_1 && (dim2_2==0 || dim2_2==1);

	// allow assignments of the form: mat 2 2 A = [1, 2, 3, 4];
	// also allow truncated assignments ("<="), TODO: remove in the future
	if(ty1==SymbolType::MATRIX && ty2==SymbolType::VECTOR)
		return dim1_1*dim1_2 <= dim2_1 && (dim2_2==0 || dim2_2==1);

	if(ty1==SymbolType::MATRIX && ty2==SymbolType::MATRIX)
		return dim1_1==dim2_1 && dim1_2==dim2_2;
	if(ty1==SymbolType::VECTOR && ty2==SymbolType::VECTOR)
		return dim1_1==dim2_1;
	if(ty1==SymbolType::MATRIX && ty2==SymbolType::VECTOR)
		return false;
	if(ty1==SymbolType::VECTOR && ty2==SymbolType::MATRIX)
		return false;

	return true;
}


/**
 * product between a scalar and a matrix (or vector)
 */
t_astret LLAsm::scalar_matrix_prod(t_astret scalar, t_astret matrix, bool mul_or_div)
{
	scalar = convert_sym(scalar, SymbolType::SCALAR);

	std::size_t dim = std::get<0>(matrix->dims);
	if(matrix->ty == SymbolType::MATRIX)
		dim *= std::get<1>(matrix->dims);

	// allocate real array for result
	t_astret vec_mem = get_tmp_var(matrix->ty, &matrix->dims);
	(*m_ostr) << "%" << vec_mem->name << " = alloca [" << dim << " x " << m_real << "]\n";

	// copy matrix elements in a loop
	generate_loop(0, dim, [this, dim, matrix, scalar, vec_mem, mul_or_div](t_astret ctrval)
	{
		// loop statements
		t_astret elemptr_src = get_tmp_var();
		t_astret elem_src = get_tmp_var();
		(*m_ostr) << "%" << elemptr_src->name << " = getelementptr ["
			<< dim << " x " << m_real << "], ["
			<< dim << " x " << m_real << "]* %"
			<< matrix->name << ", " << m_int << " 0, " << m_int
			<< " %" << ctrval->name << "\n";
		(*m_ostr) << "%" << elem_src->name << " = load "
			<< m_real << ", " << m_realptr
			<< " %" << elemptr_src->name << "\n";

		t_astret elemptr_dst = get_tmp_var();
		(*m_ostr) << "%" << elemptr_dst->name << " = getelementptr ["
			<< dim << " x " << m_real << "], ["
			<< dim << " x " << m_real << "]* %"
			<< vec_mem->name << ", " << m_int << " 0, " << m_int
			<< " %" << ctrval->name << "\n";

		// multiply element value with scalar
		t_astret elem_dst = get_tmp_var(SymbolType::SCALAR);

		if(mul_or_div)
		{
			(*m_ostr) << "%" << elem_dst->name << " = fmul " << m_real << " %"
				<< elem_src->name << ", %" << scalar->name << "\n";
		}
		else
		{
			(*m_ostr) << "%" << elem_dst->name << " = fdiv " << m_real << " %"
				<< elem_src->name << ", %" << scalar->name << "\n";
		}

		// save result in array
		(*m_ostr) << "store " << m_real << " %" << elem_dst->name
			<< ", " << m_realptr << " %" << elemptr_dst->name << "\n";
	});

	return vec_mem;
}


/**
 * copy the memory of a compound symbol
 */
t_astret LLAsm::cp_comp_mem(t_astret sym, t_astret mem)
{
	std::size_t len = get_bytesize(sym);

	(*m_ostr) << "call i8* @memcpy(i8* %" << mem->name << ", i8* %" << sym->name
		<< ", " << m_int << " " << len << ")\n";

	return mem;
}


/**
 * copy a vector into a memory block
 */
t_astret LLAsm::cp_vec_mem(t_astret sym, t_astret mem)
{
	std::size_t dim = std::get<0>(sym->dims);
	if(sym->ty == SymbolType::MATRIX)
		dim *= std::get<1>(sym->dims);

	t_astret termptr = get_tmp_var(sym->ty);
	(*m_ostr) << "%" << termptr->name << " = bitcast [" << dim
		<< " x " << m_real << "]* %" << sym->name << " to i8*\n";

	// if no memory block is given, allocate one
	if(!mem)
	{
		mem = get_tmp_var(sym->ty, &sym->dims, nullptr);
		(*m_ostr) << "%" << mem->name << " = call i8* @ext_heap_alloc(" << m_int << " "
			<< dim << ", " << m_int << " " << sizeof(t_real) << ")\n";
	}

	(*m_ostr) << "call i8* @memcpy(i8* %" << mem->name << ", i8* %" << termptr->name
		<< ", " << m_int << " " << dim*sizeof(t_real) << ")\n";

	// cast to the actual t_real*
	t_astret mem_double = get_tmp_var(sym->ty, &sym->dims);
	(*m_ostr) << "%" << mem_double->name << " = bitcast i8* %"
		<< mem->name << " to " << m_realptr << "\n";

	return mem_double;
}


/**
 * copy a string into a memory block
 */
t_astret LLAsm::cp_str_mem(t_astret sym, t_astret mem)
{
	std::size_t dim = std::get<0>(sym->dims);

	t_astret termptr = get_tmp_var(SymbolType::STRING);
	(*m_ostr) << "%" << termptr->name << " = bitcast [" << dim << " x i8]* %" << sym->name << " to i8*\n";

	t_astret strretlen = get_tmp_var(SymbolType::INT);
	(*m_ostr) << "%" << strretlen->name << " = call " << m_int
		<< " @strlen(i8* %" << termptr->name << ")\n";

	t_astret strretlen_z = get_tmp_var(SymbolType::INT);
	(*m_ostr) << "%" << strretlen_z->name << " = add " << m_int
		<< " %" << strretlen->name << ", 1\n";

	// if no memory block is given, allocate one
	if(!mem)
	{
		mem = get_tmp_var(SymbolType::STRING, &sym->dims, nullptr);
		(*m_ostr) << "%" << mem->name << " = call i8* @ext_heap_alloc(" << m_int
			<< " %" << strretlen_z->name << ", " << m_int << " 1" << ")\n";
	}

	(*m_ostr) << "call i8* @strncpy(i8* %" << mem->name << ", i8* %" << termptr->name
		<< ", " << m_int << " " << dim << ")\n";

	return mem;
}


/**
 * copy a a memory block to a composite symbol
 */
t_astret LLAsm::cp_mem_comp(t_astret mem, t_astret sym)
{
	std::size_t len = get_bytesize(sym);

	(*m_ostr) << "call i8* @memcpy(i8* %" << sym->name << ", i8* %"
		<< mem->name << ", " << m_int << " " << len << ")\n";

	return sym;
}


/**
 * copy a a memory block to a vector
 */
t_astret LLAsm::cp_mem_vec(t_astret mem, t_astret sym, bool alloc_sym)
{
	std::size_t argdim = std::get<0>(sym->dims);
	if(sym->ty == SymbolType::MATRIX)
		argdim *= std::get<1>(sym->dims);

	// allocate memory for local array copy
	if(alloc_sym)
		(*m_ostr) << "%" << sym->name << " = alloca [" << argdim << " x " << m_real << "]\n";

	t_astret arrptr = get_tmp_var();
	(*m_ostr) << "%" << arrptr->name << " = getelementptr ["
		<< argdim << " x " << m_real << "], ["
		<< argdim << " x " << m_real << "]* %"
		<< sym->name << ", " << m_int << " 0, " << m_int << " 0\n";

	// copy array
	t_astret arrptr_cast = get_tmp_var();

	// cast to memcpy argument pointer type
	(*m_ostr) << "%" << arrptr_cast->name << " = bitcast " << m_realptr
		<< " %" << arrptr->name << " to i8*\n";

	(*m_ostr) << "call i8* @memcpy(i8* %" << arrptr_cast->name << ", i8* %" << mem->name
		<< ", " << m_int << " " << argdim*sizeof(t_real) << ")\n";

	return sym;
}


/**
 * copy a a memory block to a string
 */
t_astret LLAsm::cp_mem_str(t_astret mem, t_astret sym, bool alloc_sym)
{
	if(alloc_sym)
		(*m_ostr) << "%" << sym->name << " = alloca [" << std::get<0>(sym->dims) << " x i8]\n";

	t_astret strptr = get_tmp_var();
	(*m_ostr) << "%" << strptr->name << " = getelementptr ["
		<< std::get<0>(sym->dims) << " x i8], ["
		<< std::get<0>(sym->dims) << " x i8]* %"
		<< sym->name << ", " << m_int << " 0, " << m_int << " 0\n";

	// copy string
	(*m_ostr) << "call i8* @strncpy(i8* %" << strptr->name << ", i8* %" << mem->name
		<< ", " << m_int << " " << std::get<0>(sym->dims) << ")\n";

	return sym;
}


t_astret LLAsm::safe_array_index(t_astret idx, std::size_t size)
{
	// modidx = idx % size
	t_astret modidx = get_tmp_var(SymbolType::INT);
	(*m_ostr) << "%" << modidx->name << " = srem " << m_int
		<< " %" << idx->name << ", " << size << "\n";

	// if(modidx < 0) modidx2 = size - modidx
	t_astret modidx2 = get_tmp_var(SymbolType::INT);
	(*m_ostr) << "%" << modidx2->name << " = alloca " << m_int << "\n";

	generate_cond(
	[this, modidx]() -> t_astret
	{
		t_astret _cond = get_tmp_var(SymbolType::INT);
		(*m_ostr) << "%" << _cond->name << " = icmp slt " << m_int
			<< " %" << modidx->name << ", 0\n";
		return _cond;
	}, [this, modidx, modidx2, size]
	{
		t_astret diff = get_tmp_var(SymbolType::INT);
		(*m_ostr) << "%" << diff->name << " = sub " << m_int << " " << size
			<< ", %" << modidx->name << "\n";

		(*m_ostr) << "store " << m_int
			<< " %" << diff->name
			<< ", " << m_intptr
			<< " %" << modidx2->name << "\n";
	}, [this, modidx, modidx2]
	{
		(*m_ostr) << "store " << m_int
			<< " %" << modidx->name
			<< ", " << m_intptr
			<< " %" << modidx2->name << "\n";
	}, true);

	// write result back to an int variable
	t_astret modidx3 = get_tmp_var(SymbolType::INT);
	(*m_ostr) << "%" << modidx3->name 
		<< " = load " << m_int << ", " << m_intptr
		<< " %" << modidx2->name << "\n";
	return modidx3;
}
