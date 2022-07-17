/**
 * llvm zero-address code generator
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 10-july-2022
 * @license: see 'LICENSE.GPL' file
 */

#ifndef __ZEROACASM_H__
#define __ZEROACASM_H__

#include "ast.h"
#include "../vm_0ac/opcodes.h"

#include <stack>
#include <unordered_map>


/**
 * zero-address code generation
 * (the return value is only used for type information for casting)
 */
class ZeroACAsm : public ASTVisitor
{
public:
	ZeroACAsm(SymTab* syms, std::ostream* ostr = &std::cout);
	virtual ~ZeroACAsm();

	ZeroACAsm(const ZeroACAsm&) = delete;
	const ZeroACAsm& operator=(const ZeroACAsm&) = delete;

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

	void Start();
	void Finish();


protected:
	/**
	 * find the symbol with a specific name in the symbol table
	 */
	t_astret get_sym(const t_str& name) const;

	/**
	 * finds the size of the symbol for the stack frame
	 */
	std::size_t get_sym_size(const Symbol* sym) const;

	/**
	 * finds the size of the local function variables for the stack frame
	 */
	std::size_t get_stackframe_size(const Symbol* func) const;

	/**
	 * emit code to cast to given type
	 */
	void cast_to(t_astret ty_to, std::streampos pos);


private:
	// symbol table
	SymTab* m_syms{nullptr};
	// code output
	std::ostream* m_ostr{&std::cout};

	// currently active function scope
	std::vector<t_str> m_curscope{};
	// current address on stack for local variables
	std::unordered_map<std::string, t_vm_addr> m_local_stack{};

	// stream positions where addresses need to be patched in
	std::vector<std::tuple<std::string, std::streampos, t_vm_addr, const AST*>>
		m_func_comefroms{};
	std::vector<std::streampos> m_endfunc_comefroms{};

	// currently active loops in function
	std::vector<std::size_t> m_cur_loop{};
	std::unordered_multimap<std::size_t, std::streampos>
		m_loop_begin_comefroms{}, m_loop_end_comefroms{};

	// dummy symbols for constants
	Symbol *m_scalar_const{}, *m_int_const{}, *m_str_const{};
};


#endif
