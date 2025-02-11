/**
 * syntax tree
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 20-dec-19
 * @license: see 'LICENSE.GPL' file
 */

#ifndef __AST_H__
#define __AST_H__

#include <memory>
#include <string>
#include <list>
#include <vector>
#include <algorithm>
#include <iostream>
#include <cstdint>

#include "common/sym.h"

#ifdef USE_DIRECT_PARSER
	#include "lalr1/ast.h"
#endif



class AST;
class ASTUMinus;
class ASTPlus;
class ASTMult;
class ASTMod;
class ASTPow;
class ASTTransp;
class ASTNorm;
class ASTStrConst;
class ASTVar;
class ASTStmts;
class ASTVarDecl;
class ASTArgNames;
class ASTTypeDecl;
class ASTFunc;
class ASTReturn;
class ASTCall;
class ASTAssign;
class ASTArrayAssign;
class ASTArrayAccess;
class ASTComp;
class ASTBool;
class ASTCond;
class ASTLoop;
class ASTLoopBreak;
class ASTLoopNext;
class ASTExprList;
template<class> class ASTNumConst;


enum class ASTType
{
	UMinus,
	Plus,
	Mult,
	Mod,
	Pow,
	Transp,
	Norm,
	StrConst,
	Var,
	Stmts,
	VarDecl,
	ArgNames,
	TypeDecl,
	Func,
	Return,
	Call,
	Assign,
	ArrayAssign,
	ArrayAccess,
	Comp,
	Bool,
	Cond,
	Loop,
	LoopBreak,
	LoopNext,
	ExprList,
	NumConst,
};


using ASTPtr = std::shared_ptr<AST>;
using t_astret = Symbol*;


/**
 * ast visitor
 */
class ASTVisitor
{
public:
	virtual ~ASTVisitor() {}

	virtual t_astret visit(const ASTUMinus* ast) = 0;
	virtual t_astret visit(const ASTPlus* ast) = 0;
	virtual t_astret visit(const ASTMult* ast) = 0;
	virtual t_astret visit(const ASTMod* ast) = 0;
	virtual t_astret visit(const ASTPow* ast) = 0;
	virtual t_astret visit(const ASTTransp* ast) = 0;
	virtual t_astret visit(const ASTNorm* ast) = 0;

	virtual t_astret visit(const ASTVarDecl* ast) = 0;
	virtual t_astret visit(const ASTVar* ast) = 0;
	virtual t_astret visit(const ASTAssign* ast) = 0;

	virtual t_astret visit(const ASTArrayAssign* ast) = 0;
	virtual t_astret visit(const ASTArrayAccess* ast) = 0;

	virtual t_astret visit(const ASTNumConst<t_real>* ast) = 0;
	virtual t_astret visit(const ASTNumConst<t_int>* ast) = 0;
	virtual t_astret visit(const ASTStrConst* ast) = 0;

	virtual t_astret visit(const ASTFunc* ast) = 0;
	virtual t_astret visit(const ASTCall* ast) = 0;
	virtual t_astret visit(const ASTReturn* ast) = 0;
	virtual t_astret visit(const ASTStmts* ast) = 0;

	virtual t_astret visit(const ASTCond* ast) = 0;
	virtual t_astret visit(const ASTLoop* ast) = 0;
	virtual t_astret visit(const ASTLoopBreak* ast) = 0;
	virtual t_astret visit(const ASTLoopNext* ast) = 0;

	virtual t_astret visit(const ASTComp* ast) = 0;
	virtual t_astret visit(const ASTBool* ast) = 0;
	virtual t_astret visit(const ASTExprList* ast) = 0;

	virtual t_astret visit(const ASTArgNames* ast) = 0;
	virtual t_astret visit(const ASTTypeDecl* ast) = 0;
};


/**
 * ast node base
 */
class AST
#ifdef USE_DIRECT_PARSER
	: public lalr1::ASTBase
#endif
{
public:
	virtual ~AST() = default;

	virtual t_astret accept(ASTVisitor* visitor) const = 0;
	virtual ASTType type() = 0;

	// TODO: either add this to all derived ast classes or use terminal override value in lexer
	//virtual bool IsTerminal() const override { return false; }
};


/**
 * ast visitor acceptor
 */
template<class t_ast_sub>
class ASTAcceptor : public AST
{
public:
	virtual t_astret accept(ASTVisitor* visitor) const override
	{
		const t_ast_sub *sub = static_cast<const t_ast_sub*>(this);
		return visitor->visit(sub);
	}
};


class ASTUMinus : public ASTAcceptor<ASTUMinus>
{
public:
	ASTUMinus(ASTPtr term) : term{term}
	{}

	const ASTPtr GetTerm() const { return term; }

	virtual ASTType type() override { return ASTType::UMinus; }

private:
	ASTPtr term{};
};


class ASTPlus : public ASTAcceptor<ASTPlus>
{
public:
	ASTPlus(ASTPtr term1, ASTPtr term2, bool invert = false)
		: term1{term1}, term2{term2}, inverted{invert}
	{}

	const ASTPtr GetTerm1() const { return term1; }
	const ASTPtr GetTerm2() const { return term2; }
	bool IsInverted() const { return inverted; }

	virtual ASTType type() override { return ASTType::Plus; }

private:
	ASTPtr term1{}, term2{};
	bool inverted = false;
};


class ASTMult : public ASTAcceptor<ASTMult>
{
public:
	ASTMult(ASTPtr term1, ASTPtr term2, bool invert = false)
		: term1{term1}, term2{term2}, inverted{invert}
	{}

	const ASTPtr GetTerm1() const { return term1; }
	const ASTPtr GetTerm2() const { return term2; }
	bool IsInverted() const { return inverted; }

	virtual ASTType type() override { return ASTType::Mult; }

private:
	ASTPtr term1{}, term2{};
	bool inverted = false;
};


class ASTMod : public ASTAcceptor<ASTMod>
{
public:
	ASTMod(ASTPtr term1, ASTPtr term2)
		: term1{term1}, term2{term2}
	{}

	const ASTPtr GetTerm1() const { return term1; }
	const ASTPtr GetTerm2() const { return term2; }

	virtual ASTType type() override { return ASTType::Mod; }

private:
	ASTPtr term1{}, term2{};
};


class ASTPow : public ASTAcceptor<ASTPow>
{
public:
	ASTPow(ASTPtr term1, ASTPtr term2)
		: term1{term1}, term2{term2}
	{}

	const ASTPtr GetTerm1() const { return term1; }
	const ASTPtr GetTerm2() const { return term2; }

	virtual ASTType type() override { return ASTType::Pow; }

private:
	ASTPtr term1{}, term2{};
};


class ASTTransp : public ASTAcceptor<ASTTransp>
{
public:
	ASTTransp(ASTPtr term) : term{term}
	{}

	const ASTPtr GetTerm() const { return term; }

	virtual ASTType type() override { return ASTType::Transp; }

private:
	ASTPtr term{};
};


class ASTNorm : public ASTAcceptor<ASTNorm>
{
public:
	ASTNorm(ASTPtr term) : term{term}
	{}

	const ASTPtr GetTerm() const { return term; }

	virtual ASTType type() override { return ASTType::Norm; }

private:
	ASTPtr term{};
};


class ASTVar : public ASTAcceptor<ASTVar>
{
public:
	ASTVar(const t_str& ident)
		: ident{ident}
	{}

	const t_str& GetIdent() const { return ident; }

	virtual ASTType type() override { return ASTType::Var; }

private:
	t_str ident{};
};


class ASTStmts : public ASTAcceptor<ASTStmts>
{
public:
	ASTStmts() : stmts{}
	{}

	void AddStatement(ASTPtr stmt)
	{
		stmts.push_front(stmt);
	}

	const std::list<ASTPtr>& GetStatementList() const
	{
		return stmts;
	}

	virtual ASTType type() override { return ASTType::Stmts; }

private:
	std::list<ASTPtr> stmts{};
};


class ASTVarDecl : public ASTAcceptor<ASTVarDecl>
{
public:
	ASTVarDecl()
		: vars{}
	{}

	ASTVarDecl(std::shared_ptr<ASTAssign> optAssign)
		: vars{}, optAssign{optAssign}
	{}

	void AddVariable(const t_str& var) { vars.push_front(var); }
	const std::list<t_str>& GetVariables() const { return vars; }

	const std::shared_ptr<ASTAssign> GetAssignment() const { return optAssign; }

	virtual ASTType type() override { return ASTType::VarDecl; }

private:
	std::list<t_str> vars{};

	// optional assignment
	std::shared_ptr<ASTAssign> optAssign{};
};


class ASTArgNames : public ASTAcceptor<ASTArgNames>
{
public:
	ASTArgNames() : argnames{}
	{}

	void AddArg(const t_str& argname,
		SymbolType ty=SymbolType::UNKNOWN,
		std::size_t dim1=1, std::size_t dim2=1)
	{
		argnames.push_front(std::make_tuple(argname, ty, dim1, dim2));
	}

	const std::list<std::tuple<t_str, SymbolType, std::size_t, std::size_t>>& GetArgs() const
	{
		return argnames;
	}

	std::vector<t_str> GetArgIdents() const
	{
		std::vector<t_str> idents;
		for(const auto& arg : argnames)
			idents.push_back(std::get<0>(arg));
		return idents;
	}

	std::vector<SymbolType> GetArgTypes() const
	{
		std::vector<SymbolType> ty;
		for(const auto& arg : argnames)
			ty.push_back(std::get<1>(arg));
		return ty;
	}

	virtual ASTType type() override { return ASTType::ArgNames; }

private:
	std::list<std::tuple<t_str, SymbolType, std::size_t, std::size_t>> argnames{};
};


class ASTTypeDecl : public ASTAcceptor<ASTTypeDecl>
{
public:
	ASTTypeDecl(SymbolType ty, std::size_t dim1=1, std::size_t dim2=1)
		: ty{ty}, dim1{dim1}, dim2{dim2}
	{}

	SymbolType GetType() const { return ty; }

	std::size_t GetDim(int i=0) const
	{
		if(i==0) return dim1;
		else if(i==1) return dim2;
		return 0;
	}

	std::tuple<SymbolType, std::size_t, std::size_t> GetRet() const
	{
		return std::make_tuple(ty, dim1, dim2);
	}

	virtual ASTType type() override { return ASTType::TypeDecl; }

private:
	SymbolType ty{SymbolType::UNKNOWN};
	std::size_t dim1=1, dim2=1;
};


class ASTFunc : public ASTAcceptor<ASTFunc>
{
public:
	ASTFunc(const t_str& ident, std::shared_ptr<ASTTypeDecl>& rettype,
		std::shared_ptr<ASTArgNames> args, std::shared_ptr<ASTStmts> stmts,
		std::shared_ptr<ASTArgNames> rets = nullptr)
		: ident{ident}, rettype{rettype->GetRet()}, args{args->GetArgs()},
			stmts{stmts}, rets{}
	{
		if(rets)
			this->rets = rets->GetArgs();
	}

	const t_str& GetIdent() const { return ident; }
	std::tuple<SymbolType, std::size_t, std::size_t> GetRetType() const { return rettype; }

	const std::list<std::tuple<t_str, SymbolType, std::size_t, std::size_t>>&
	GetArgs() const { return args; }

	const std::list<std::tuple<t_str, SymbolType, std::size_t, std::size_t>>&
	GetRets() const { return rets; }

	const std::shared_ptr<ASTStmts> GetStatements() const { return stmts; }

	virtual ASTType type() override { return ASTType::Func; }

private:
	t_str ident{};
	std::tuple<SymbolType, std::size_t, std::size_t> rettype{};
	std::list<std::tuple<t_str, SymbolType, std::size_t, std::size_t>> args{};
	std::shared_ptr<ASTStmts> stmts{};
	std::list<std::tuple<t_str, SymbolType, std::size_t, std::size_t>> rets{};
};


class ASTReturn : public ASTAcceptor<ASTReturn>
{
public:
	ASTReturn(const std::shared_ptr<ASTExprList>& rets = nullptr,
		bool multi_return = false)
		: rets{rets}, multi_return{multi_return}
	{}

	const std::shared_ptr<ASTExprList> GetRets() const { return rets; }

	bool IsMultiReturn() const { return multi_return; }

	virtual ASTType type() override { return ASTType::Return; }

private:
	std::shared_ptr<ASTExprList> rets{};
	bool multi_return = false;
};


class ASTExprList : public ASTAcceptor<ASTExprList>
{
public:
	ASTExprList()
	{}

	void AddExpr(ASTPtr expr)
	{
		exprs.push_front(expr);
	}

	const std::list<ASTPtr>& GetList() const
	{
		return exprs;
	}

	/**
	 * specialised use as an array of scalars
	 */
	void SetScalarArray(bool b)
	{
		m_scalararray = b;
	}

	bool IsScalarArray() const
	{
		return m_scalararray;
	}

	virtual ASTType type() override { return ASTType::ExprList; }

private:
	std::list<ASTPtr> exprs{};
	bool m_scalararray = false;
};


class ASTCall : public ASTAcceptor<ASTCall>
{
public:
	ASTCall(const t_str& ident)
		: ident{ident}, args{std::make_shared<ASTExprList>()}
	{}

	ASTCall(const t_str& ident, std::shared_ptr<ASTExprList> args)
		: ident{ident}, args{args}
	{}

	const t_str& GetIdent() const { return ident; }
	const std::list<ASTPtr>& GetArgumentList() const { return args->GetList(); }

	virtual ASTType type() override { return ASTType::Call; }

private:
	t_str ident{};
	std::shared_ptr<ASTExprList> args{};
};


class ASTAssign : public ASTAcceptor<ASTAssign>
{
public:
	ASTAssign()
	{}

	ASTAssign(const t_str& ident, ASTPtr _expr)
		: idents{{ident}}, expr{_expr}
	{}

	ASTAssign(const std::vector<t_str>& idents, ASTPtr expr)
		: idents{idents}, expr{expr}
	{}

	const std::vector<t_str>& GetIdents() const { return idents; }
	const t_str& GetIdent() const { return GetIdents()[0]; }
	const ASTPtr GetExpr() const { return expr; }

	bool IsMultiAssign() const { return idents.size() > 1; }
	bool IsNullAssign() const { return idents.size() == 0; }

	virtual ASTType type() override { return ASTType::Assign; }

private:
	std::vector<t_str> idents{};
	ASTPtr expr{};
};


class ASTComp : public ASTAcceptor<ASTComp>
{
public:
	enum CompOp
	{
		EQU, NEQ,
		GT, LT, GEQ, LEQ
	};

public:
	ASTComp(ASTPtr term1, ASTPtr term2, CompOp op)
		: term1{term1}, term2{term2}, op{op}
	{}

	ASTComp(ASTPtr term1, CompOp op)
		: term1{term1}, term2{nullptr}, op{op}
	{}

	const ASTPtr GetTerm1() const { return term1; }
	const ASTPtr GetTerm2() const { return term2; }
	CompOp GetOp() const { return op; }

	virtual ASTType type() override { return ASTType::Comp; }

private:
	ASTPtr term1{}, term2{};
	CompOp op{};
};


class ASTBool : public ASTAcceptor<ASTBool>
{
public:
	enum BoolOp
	{
		NOT,
		AND, OR, XOR
	};

public:
	ASTBool(ASTPtr term1, ASTPtr term2, BoolOp op)
		: term1{term1}, term2{term2}, op{op}
	{}

	ASTBool(ASTPtr term1, BoolOp op)
		: term1{term1}, term2{nullptr}, op{op}
	{}

	const ASTPtr GetTerm1() const { return term1; }
	const ASTPtr GetTerm2() const { return term2; }
	BoolOp GetOp() const { return op; }

	virtual ASTType type() override { return ASTType::Bool; }

private:
	ASTPtr term1{}, term2{};
	BoolOp op{};
};


class ASTCond : public ASTAcceptor<ASTCond>
{
public:
	ASTCond(const ASTPtr cond, ASTPtr if_stmt)
		: cond{cond}, if_stmt{if_stmt}
	{}
	ASTCond(const ASTPtr cond, ASTPtr if_stmt, ASTPtr else_stmt)
		: cond{cond}, if_stmt{if_stmt}, else_stmt{else_stmt}
	{}

	const ASTPtr GetCond() const { return cond; }
	const ASTPtr GetIf() const { return if_stmt; }
	const ASTPtr GetElse() const { return else_stmt; }
	bool HasElse() const { return else_stmt != nullptr; }

	virtual ASTType type() override { return ASTType::Cond; }

private:
	ASTPtr cond{};
	ASTPtr if_stmt{}, else_stmt{};
};


class ASTLoop : public ASTAcceptor<ASTLoop>
{
public:
	ASTLoop(const ASTPtr cond, ASTPtr stmt)
		: cond{cond}, stmt{stmt}
	{}

	const ASTPtr GetCond() const { return cond; }
	const ASTPtr GetLoopStmt() const { return stmt; }

	virtual ASTType type() override { return ASTType::Loop; }

private:
	ASTPtr cond{}, stmt{};
};


class ASTLoopBreak : public ASTAcceptor<ASTLoopBreak>
{
public:
	ASTLoopBreak(t_int num_loops = 0) : num_loops{num_loops}
	{}

	virtual ASTType type() override { return ASTType::LoopBreak; }

	t_int GetNumLoops() const { return num_loops; }

private:
	t_int num_loops{0};
};


class ASTLoopNext : public ASTAcceptor<ASTLoopNext>
{
public:
	ASTLoopNext(t_int num_loops = 0) : num_loops{num_loops}
	{}

	virtual ASTType type() override { return ASTType::LoopNext; }

	t_int GetNumLoops() const { return num_loops; }

private:
	t_int num_loops{0};
};


class ASTArrayAccess : public ASTAcceptor<ASTArrayAccess>
{
public:
	ASTArrayAccess(ASTPtr term,
		ASTPtr num1, ASTPtr num2 = nullptr,
		ASTPtr num3 = nullptr, ASTPtr num4 = nullptr,
		bool ranged12 = false, bool ranged34 = false)
		: term{term},
			num1{num1}, num2{num2}, num3{num3}, num4{num4},
			ranged12{ranged12}, ranged34{ranged34}
	{}

	const ASTPtr GetTerm() const { return term; }

	const ASTPtr GetNum1() const { return num1; }
	const ASTPtr GetNum2() const { return num2; }
	const ASTPtr GetNum3() const { return num3; }
	const ASTPtr GetNum4() const { return num4; }

	bool IsRanged12() const { return ranged12; }
	bool IsRanged34() const { return ranged34; }

	virtual ASTType type() override { return ASTType::ArrayAccess; }

private:
	ASTPtr term;

	ASTPtr num1{}, num2{};
	ASTPtr num3{}, num4{};
	bool ranged12 = false;
	bool ranged34 = false;
};


class ASTArrayAssign : public ASTAcceptor<ASTArrayAssign>
{
public:
	ASTArrayAssign(const t_str& ident, ASTPtr expr,
		ASTPtr num1, ASTPtr num2 = nullptr,
		ASTPtr num3 = nullptr, ASTPtr num4 = nullptr,
		bool ranged12 = false, bool ranged34 = false)
		: ident{ident}, expr{expr},
			num1{num1}, num2{num2},
			num3{num3}, num4{num4},
			ranged12{ranged12}, ranged34{ranged34}
	{}

	const t_str& GetIdent() const { return ident; }
	const ASTPtr GetExpr() const { return expr; }

	const ASTPtr GetNum1() const { return num1; }
	const ASTPtr GetNum2() const { return num2; }
	const ASTPtr GetNum3() const { return num3; }
	const ASTPtr GetNum4() const { return num4; }

	bool IsRanged12() const { return ranged12; }
	bool IsRanged34() const { return ranged34; }

	virtual ASTType type() override { return ASTType::ArrayAssign; }

private:
	t_str ident{};
	ASTPtr expr{};

	ASTPtr num1{}, num2{};
	ASTPtr num3{}, num4{};
	bool ranged12 = false;
	bool ranged34 = false;
};


template<class t_num>
class ASTNumConst : public ASTAcceptor<ASTNumConst<t_num>>
{
public:
	ASTNumConst(t_num val) : val{val}
	{}

	t_num GetVal() const { return val; }

	virtual ASTType type() override { return ASTType::NumConst; }

private:
	t_num val{};
};


class ASTStrConst : public ASTAcceptor<ASTStrConst>
{
public:
	ASTStrConst(const t_str& str) : val{str}
	{}

	const t_str& GetVal() const { return val; }

	virtual ASTType type() override { return ASTType::StrConst; }

private:
	t_str val{};
};


#endif
