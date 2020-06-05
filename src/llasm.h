/**
 * llvm three-address code generator
 * @author Tobias Weber
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
#include "sym.h"


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
	virtual t_astret visit(const ASTLoop* ast) override;
	virtual t_astret visit(const ASTStrConst* ast) override;
	virtual t_astret visit(const ASTExprList* ast) override;
	virtual t_astret visit(const ASTNumConst<double>* ast) override;
	virtual t_astret visit(const ASTNumConst<std::int64_t>* ast) override;


	// ------------------------------------------------------------------------
	// internally handled dummy nodes
	// ------------------------------------------------------------------------
	virtual t_astret visit(const ASTArgNames*) override { return nullptr; }
	virtual t_astret visit(const ASTTypeDecl*) override { return nullptr; }
	// ------------------------------------------------------------------------


protected:
	t_astret get_tmp_var(SymbolType ty = SymbolType::SCALAR,
		const std::array<std::size_t, 2>* dims = nullptr,
		const std::string* name = nullptr, bool on_heap = false);

	/**
	 * create a label for branch instructions
	 */
	std::string get_label();

	/**
	 * find the symbol with a specific name in the symbol table
	 */
	t_astret get_sym(const std::string& name) const;

	/**
	 * convert symbol to another type
	 */
	t_astret convert_sym(t_astret sym, SymbolType ty_to);

	/**
	 * get the corresponding data type name
	 */
	std::string get_type_name(SymbolType ty);


	/**
	 * generates if-then-else code
	 */
	template<class t_funcCond, class t_funcBody, class t_funcElseBody>
	void generate_cond(t_funcCond funcCond, t_funcBody funcBody, t_funcElseBody funcElseBody, bool hasElse=0);


private:
	std::size_t m_varCount = 0;	// # of tmp vars
	std::size_t m_labelCount = 0;	// # of labels

	std::vector<std::string> m_curscope;

	SymTab* m_syms = nullptr;

	std::ostream* m_ostr = &std::cout;

	// helper functions to reduce code redundancy
	t_astret scalar_matrix_prod(t_astret scalar, t_astret matrix, bool mul_or_div=1);
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

	std::string labelIf = get_label();
	std::string labelElse = hasElse ? get_label() : "";
	std::string labelEnd = get_label();

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


#endif
