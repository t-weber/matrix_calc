/**
 * llvm three-address code generator
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 11-apr-20
 * @license: see 'LICENSE.GPL' file
 *
 * References:
 *	* https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl03.html
 *	* https://llvm.org/docs/GettingStarted.html
 *	* https://llvm.org/docs/LangRef.html
 */

#ifndef __LLASM_H__
#define __LLASM_H__

#include "ast.h"
#include <stack>
#include <type_traits>


template<class t_type, class t_str = const char*>
t_str get_lltype_name()
{
	if constexpr(std::is_same_v<std::decay_t<t_type>, long double>)
		return "long double";
	else if constexpr(std::is_same_v<std::decay_t<t_type>, double>)
		return "double";
	else if constexpr(std::is_same_v<std::decay_t<t_type>, float>)
		return "float";
	else if constexpr(std::is_same_v<std::decay_t<t_type>, std::int64_t>)
		return "i64";
	else if constexpr(std::is_same_v<std::decay_t<t_type>, std::int32_t>)
		return "i32";
	else if constexpr(std::is_same_v<std::decay_t<t_type>, std::int16_t>)
		return "i16";
	else if constexpr(std::is_same_v<std::decay_t<t_type>, std::int8_t>)
		return "i8";
	else if constexpr(std::is_same_v<std::decay_t<t_type>, std::uint64_t>)
		return "u64";
	else if constexpr(std::is_same_v<std::decay_t<t_type>, std::uint32_t>)
		return "u32";
	else if constexpr(std::is_same_v<std::decay_t<t_type>, std::uint16_t>)
		return "u16";
	else if constexpr(std::is_same_v<std::decay_t<t_type>, std::uint8_t>)
		return "u8";
	else
		return "<unknown>";
}


/**
 * multiply the elements of a container
 */
template<class t_cont, std::size_t ...seq>
constexpr typename t_cont::value_type multiply_elements(
	const t_cont& cont, const std::index_sequence<seq...>&)
{
	return (std::get<seq>(cont) * ...);
}


/**
 * multiply all dimensions of an array type
 */
template<std::size_t NUM_DIMS=2>
std::size_t get_arraydim(const std::array<std::size_t, NUM_DIMS>& dims)
{
	return multiply_elements(dims, std::make_index_sequence<NUM_DIMS>());
}


class LLAsm : public ASTVisitor
{
public:
	LLAsm(SymTab* syms, std::ostream* ostr=&std::cout);
	virtual ~LLAsm() = default;

	virtual t_astret visit(const ASTUMinus* ast) override;
	virtual t_astret visit(const ASTPlus* ast) override;
	virtual t_astret visit(const ASTMult* ast) override;
	virtual t_astret visit(const ASTMod* ast) override;
	virtual t_astret visit(const ASTPow* ast) override;
	virtual t_astret visit(const ASTTransp* ast) override;
	virtual t_astret visit(const ASTNorm* ast) override;
	virtual t_astret visit(const ASTVar* ast) override;
	virtual t_astret visit(const ASTCall* ast) override;
	virtual t_astret visit(const ASTStmts* ast) override;
	virtual t_astret visit(const ASTVarDecl* ast) override;
	virtual t_astret visit(const ASTFunc* ast) override;
	virtual t_astret visit(const ASTReturn* ast) override;
	virtual t_astret visit(const ASTAssign* ast) override;
	virtual t_astret visit(const ASTArrayAccess* ast) override;
	virtual t_astret visit(const ASTArrayAssign* ast) override;
	virtual t_astret visit(const ASTComp* ast) override;
	virtual t_astret visit(const ASTCond* ast) override;
	virtual t_astret visit(const ASTBool* ast) override;
	virtual t_astret visit(const ASTLoop* ast) override;
	virtual t_astret visit(const ASTStrConst* ast) override;
	virtual t_astret visit(const ASTExprList* ast) override;
	virtual t_astret visit(const ASTNumConst<t_real>* ast) override;
	virtual t_astret visit(const ASTNumConst<t_int>* ast) override;

	// ------------------------------------------------------------------------
	// internally handled dummy nodes
	// ------------------------------------------------------------------------
	virtual t_astret visit(const ASTArgNames*) override { return nullptr; }
	virtual t_astret visit(const ASTTypeDecl*) override { return nullptr; }
	// ------------------------------------------------------------------------


	/**
	 * output declarations for registered functions
	 */
	static t_str get_function_declarations(const SymTab& symtab, bool only_externals=1);


protected:
	t_astret get_tmp_var(SymbolType ty = SymbolType::SCALAR,
		const std::array<std::size_t, 2>* dims = nullptr,
		const t_str* name = nullptr);

	/**
	 * create a label for branch instructions
	 */
	t_str get_label();

	/**
	 * create a label for a block of statements
	 */
	t_str get_block_label();

	/**
	 * get the type name for a symbol
	 */
	static t_str get_type_name(SymbolType ty);

	/**
	 * get the element type for an array type
	 */
	static SymbolType get_element_type(SymbolType ty);

	/**
	 * get the (static) byte size of a symbol
	 */
	static std::size_t get_bytesize(t_astret sym);

	/**
	 * get the dimensions of an array type
	 */
	static std::size_t get_arraydim(t_astret sym);

	/**
	 * find the symbol with a specific name in the symbol table
	 */
	t_astret get_sym(const t_str& name) const;

	/**
	 * convert symbol to another type
	 */
	t_astret convert_sym(t_astret sym, SymbolType ty_to);

	/**
	 * check if two symbols can be converted to one another
	 */
	static bool check_sym_compat(
		SymbolType ty1, std::size_t dim1_1, std::size_t dim1_2,
		SymbolType ty2, std::size_t dim2_1, std::size_t dim2_2);

	/**
	 * limits the array index to the [0, size[ range
	 */
	t_astret safe_array_index(t_astret idx, std::size_t size);

	/**
	 * generates if-then-else code
	 */
	template<class t_funcCond, class t_funcBody, class t_funcElseBody>
	void generate_cond(t_funcCond funcCond, t_funcBody funcBody, t_funcElseBody funcElseBody, bool hasElse=0);

	/**
	 * generates loop code
	 */
	template<class t_funcCond, class t_funcBody>
	void generate_loop(t_funcCond funcCond, t_funcBody funcBody);

	/**
	 * generates loop code with managed counter
	 */
	template<class t_funcBody>
	void generate_loop(t_int start, t_int end, t_funcBody funcBody);


	t_astret cp_comp_mem(t_astret sym, t_astret mem);
	t_astret cp_vec_mem(t_astret sym, t_astret mem=nullptr);
	t_astret cp_str_mem(t_astret sym, t_astret mem=nullptr);

	t_astret cp_mem_comp(t_astret mem, t_astret sym);
	t_astret cp_mem_vec(t_astret mem, t_astret sym, bool alloc_sym=true);
	t_astret cp_mem_str(t_astret mem, t_astret sym, bool alloc_sym=true);


private:
	std::size_t m_varCount = 0;         // # of tmp vars
	std::size_t m_labelCount = 0;       // # of labels
	std::size_t m_labelCountBlock = 0;  // # of block labels

	std::vector<t_str> m_curscope;

	SymTab* m_syms = nullptr;

	std::ostream* m_ostr = &std::cout;

	// helper functions to reduce code redundancy
	t_astret scalar_matrix_prod(t_astret scalar, t_astret matrix, bool mul_or_div=1);

	// stack only needed for (future) nested functions
	std::stack<const ASTFunc*> m_funcstack;

	// type names
	static const t_str m_real;
	static const t_str m_int;
	static const t_str m_realptr;
	static const t_str m_intptr;
};



/**
 * generates if-then-else code
 */
template<class t_funcCond, class t_funcBody, class t_funcElseBody>
void LLAsm::generate_cond(t_funcCond funcCond, t_funcBody funcBody, t_funcElseBody funcElseBody, bool hasElse)
{
	(*m_ostr) << "\n;-------------------------------------------------------------\n";
	(*m_ostr) << "; condition head\n";
	(*m_ostr) << ";-------------------------------------------------------------\n";
	t_astret cond = funcCond();
	(*m_ostr) << ";-------------------------------------------------------------\n";

	t_str labelIf = get_label();
	t_str labelElse = hasElse ? get_label() : "";
	t_str labelEnd = get_label();

	if(hasElse)
		(*m_ostr) << "br i1 %" << cond->name << ", label %" << labelIf << ", label %" << labelElse << "\n";
	else
		(*m_ostr) << "br i1 %" << cond->name << ", label %" << labelIf << ", label %" << labelEnd << "\n";

	(*m_ostr) << ";-------------------------------------------------------------\n";
	(*m_ostr) << "; condition body\n";
	(*m_ostr) << ";-------------------------------------------------------------\n";
	(*m_ostr) << labelIf << ":\n";
	funcBody();
	(*m_ostr) << ";-------------------------------------------------------------\n";

	(*m_ostr) << "br label %" << labelEnd << "\n";

	if(hasElse)
	{
		(*m_ostr) << ";-------------------------------------------------------------\n";
		(*m_ostr) << "; condition \"else\" body\n";
		(*m_ostr) << ";-------------------------------------------------------------\n";
		(*m_ostr) << labelElse << ":\n";
		funcElseBody();
		(*m_ostr) << ";-------------------------------------------------------------\n";
		(*m_ostr) << "br label %" << labelEnd << "\n";
	}

	(*m_ostr) << labelEnd << ":\n";
	(*m_ostr) << ";-------------------------------------------------------------\n\n";
}



/**
 * generates loop code
 */
template<class t_funcCond, class t_funcBody>
void LLAsm::generate_loop(t_funcCond funcCond, t_funcBody funcBody)
{
	t_str labelStart = get_label();
	t_str labelBegin = get_label();
	t_str labelEnd = get_label();
	t_str block = get_block_label();

	(*m_ostr) << "\n;-------------------------------------------------------------\n";
	(*m_ostr) << "; loop head\n";
	(*m_ostr) << ";-------------------------------------------------------------\n";
	(*m_ostr) << "br label %" << labelStart << "\n";
	(*m_ostr) << labelStart << ":\n";
	(*m_ostr) << "%" << block << " = call i8* @llvm.stacksave()\n";
	t_astret cond = funcCond();
	(*m_ostr) << "br i1 %" << cond->name << ", label %" << labelBegin << ", label %" << labelEnd << "\n";

	(*m_ostr) << ";-------------------------------------------------------------\n";
	(*m_ostr) << "; loop body\n";
	(*m_ostr) << ";-------------------------------------------------------------\n";
	(*m_ostr) << labelBegin << ":\n";
	funcBody();
	// remove stack variables created within the loop by alloca (would overflow otherwise)
	(*m_ostr) << "call void @llvm.stackrestore(i8* %" << block << ")\n";
	(*m_ostr) << ";-------------------------------------------------------------\n";

	(*m_ostr) << "br label %" << labelStart << "\n";
	(*m_ostr) << labelEnd << ":\n";
	(*m_ostr) << "call void @llvm.stackrestore(i8* %" << block << ")\n";
	(*m_ostr) << ";-------------------------------------------------------------\n\n";
}



/**
 * generates loop code with managed counter
 */
template<class t_funcBody>
void LLAsm::generate_loop(t_int start, t_int end, t_funcBody funcBody)
{
	(*m_ostr) << "\n;-------------------------------------------------------------\n";
	(*m_ostr) << "; loop counter\n";
	(*m_ostr) << ";-------------------------------------------------------------\n";
	t_astret ctr = get_tmp_var(SymbolType::INT);
	t_astret ctrval = get_tmp_var(SymbolType::INT);
	(*m_ostr) << "%" << ctr->name << " = alloca " << m_int << "\n";
	(*m_ostr) << "store " << m_int << " " << start << ", " << m_intptr
		<< " %" << ctr->name << "\n";

	generate_loop([this, ctr, ctrval, end]() -> t_astret
	{
		// loop condition: ctr < end
		(*m_ostr) << "%" << ctrval->name << " = load " << m_int << ", " << m_intptr
			<< " %" << ctr->name << "\n";

		t_astret _cond = get_tmp_var();
		(*m_ostr) << "%" << _cond->name << " = icmp slt " << m_int
			<< " %" << ctrval->name <<  ", " << end << "\n";

		return _cond;
	}, [this, &funcBody, ctr, ctrval]
	{
		funcBody(ctrval);

		(*m_ostr) << ";-------------------------------------------------------------\n";
		(*m_ostr) << "; increment loop counter\n";
		(*m_ostr) << ";-------------------------------------------------------------\n";
		t_astret newctrval = get_tmp_var(SymbolType::INT);
		(*m_ostr) << "%" << newctrval->name << " = add " << m_int
			<< " %" << ctrval->name << ", 1\n";
		(*m_ostr) << "store " << m_int << " %" << newctrval->name << ", " << m_intptr
			<< " %" << ctr->name << "\n";
	});
}

#endif
