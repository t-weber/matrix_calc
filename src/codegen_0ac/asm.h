/**
 * llvm zero-address code generator
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 10-july-2022
 * @license: see 'LICENSE.GPL' file
 */

#ifndef __ZEROACASM_H__
#define __ZEROACASM_H__

#include "ast/ast.h"
#include "codegen_0ac/consttab.h"
#include "vm_0ac/opcodes.h"

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

	virtual t_astret visit(const ASTVarDecl* ast) override;
	virtual t_astret visit(const ASTVar* ast) override;
	virtual t_astret visit(const ASTAssign* ast) override;

	virtual t_astret visit(const ASTArrayAccess* ast) override;
	virtual t_astret visit(const ASTArrayAssign* ast) override;

	virtual t_astret visit(const ASTNumConst<t_real>* ast) override;
	virtual t_astret visit(const ASTNumConst<t_int>* ast) override;
	virtual t_astret visit(const ASTStrConst* ast) override;

	virtual t_astret visit(const ASTFunc* ast) override;
	virtual t_astret visit(const ASTCall* ast) override;
	virtual t_astret visit(const ASTReturn* ast) override;
	virtual t_astret visit(const ASTStmts* ast) override;

	virtual t_astret visit(const ASTCond* ast) override;
	virtual t_astret visit(const ASTLoop* ast) override;
	virtual t_astret visit(const ASTLoopBreak* ast) override;
	virtual t_astret visit(const ASTLoopNext* ast) override;

	virtual t_astret visit(const ASTComp* ast) override;
	virtual t_astret visit(const ASTBool* ast) override;
	virtual t_astret visit(const ASTExprList* ast) override;

	// ------------------------------------------------------------------------
	// internally handled dummy nodes
	// ------------------------------------------------------------------------
	virtual t_astret visit(const ASTArgNames*) override { return nullptr; }
	virtual t_astret visit(const ASTTypeDecl*) override { return nullptr; }
	// ------------------------------------------------------------------------

	void Start();
	void Finish();


protected:
	// finds the symbol with a specific name in the symbol table
	t_astret GetSym(const t_str& name) const;

	// finds the size of the symbol for the stack frame
	std::size_t GetSymSize(const Symbol* sym) const;

	// finds the size of the local function variables for the stack frame
	std::size_t GetStackFrameSize(const Symbol* func) const;

	// returns common type of a binary operation
	std::tuple<t_astret, t_astret, t_astret>
	GetCastSymType(t_astret term1, t_astret term2);

	// emits code to cast to given type
	void CastTo(t_astret ty_to,
		const std::optional<std::streampos>& pos = std::nullopt,
		bool allow_array_cast = false);

	// push constants
	void PushRealConst(t_vm_real);
	void PushIntConst(t_vm_int);
	void PushStrConst(const t_vm_str& str);
	void PushVecConst(const std::vector<t_vm_real>& vec);
	void PushMatConst(t_vm_addr rows, t_vm_addr cols, const std::vector<t_vm_real>& mat);

	void AssignVar(t_astret sym);
	void CallExternal(const t_str& funcname);

	Symbol* GetTypeConst(SymbolType ty) const;


private:
	// symbol table
	SymTab* m_syms{nullptr};

	// constants table
	ConstTab m_consttab{};

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
	std::vector<std::tuple<std::streampos, std::streampos>> m_const_addrs{};

	// currently active loops in function
	std::vector<std::size_t> m_cur_loop{};
	std::unordered_multimap<std::size_t, std::streampos>
		m_loop_begin_comefroms{}, m_loop_end_comefroms{};

	// dummy symbols for constants
	Symbol *m_scalar_const{}, *m_int_const{}, *m_str_const{};
	Symbol *m_vec_const{}, *m_mat_const{};
};


#endif
