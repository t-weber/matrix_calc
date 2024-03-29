/**
 * parser
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 20-dec-19
 * @license see 'LICENSE.GPL' file
 *
 * References:
 *		https://github.com/westes/flex/tree/master/examples/manual
 *		http://www.gnu.org/software/bison/manual/html_node/index.html
 *		http://git.savannah.gnu.org/cgit/bison.git/tree/examples
 *		https://de.wikipedia.org/wiki/LL(k)-Grammatik
 */

// parser options
%skeleton "lalr1.cc"
//%glr-parser
%require "3.2"

%define api.parser.class { Parser }
%define api.namespace { yy }
%define api.value.type variant  // instead of union
%define api.token.constructor   // symbol constructors


// code for parser_impl.cpp
// before inclusion of definitions header
%code requires
{
	#include "ast/ast.h"
	#include "common/sym.h"
}

// after inclusion of definitions header
%code
{
	#include "parser_yy/parser.h"
	#include <cmath>
	#include <cstdint>

	#define DEFAULT_STRING_SIZE 128
}


// (forward) declarations for parser_defs.h
%code requires
{
	namespace yy
	{
		class ParserContext;
	}
}


// parameter to use for parser and yylex
%param
{
	yy::ParserContext &context
}


// terminal symbols
%token<t_str> IDENT
%token<t_real> REAL
%token<t_int> INT
%token<t_str> STRING
%token FUNC RET ASSIGN
%token SCALARDECL VECTORDECL MATRIXDECL STRINGDECL INTDECL
%token IF THEN ELSE
%token LOOP DO BREAK NEXT
%token EQU NEQ GT LT GEQ LEQ
%token AND XOR OR NOT
%token RANGE


// nonterminals
%type<std::shared_ptr<AST>> expression
%type<std::shared_ptr<ASTExprList>> expressions
%type<std::shared_ptr<AST>> statement
%type<std::shared_ptr<ASTStmts>> statements
%type<std::shared_ptr<ASTVarDecl>> variables
%type<std::shared_ptr<ASTArgNames>> full_argumentlist
%type<std::shared_ptr<ASTArgNames>> argumentlist
%type<std::shared_ptr<ASTArgNames>> identlist
%type<std::shared_ptr<ASTArgNames>> typelist
%type<std::shared_ptr<ASTStmts>> block
%type<std::shared_ptr<ASTFunc>> function
%type<std::shared_ptr<ASTTypeDecl>> typedecl
%type<std::shared_ptr<AST>> opt_assign


// precedences and left/right-associativity
// see: https://en.wikipedia.org/wiki/Order_of_operations
%nonassoc RET
%left ','
%right '='
%left XOR
%left OR
%left AND
%left GT LT GEQ LEQ
%left EQU NEQ
%left '+' '-'
%left '*' '/' '%'
%right NOT
%right UNARY_OP
%right '^' '\''
%left '(' '[' '{' '|'

// for the if/else r/s conflict shift "else"
// see: https://www.gnu.org/software/bison/manual/html_node/Non-Operators.html
%precedence IF THEN
%precedence ELSE

%precedence IDENT


%%
// non-terminals / grammar

/**
 * program start symbol
 */
program
	: statements[stmts]                     { context.SetStatements($stmts); }
	;


/**
 * a list of statements
 */
statements[res]
	: statement[stmt] statements[lst]       { $lst->AddStatement($stmt); $res = $lst; }
	| %empty                                { $res = std::make_shared<ASTStmts>(); }
	;


/**
 * variables
 */
variables[res]
	// several variables
	: IDENT[name] ',' variables[lst] {
		t_str symName = context.AddScopedSymbol($name)->scoped_name;
		$lst->AddVariable(symName);
		$res = $lst;
	}

	// a variable
	| IDENT[name] {
		t_str symName = context.AddScopedSymbol($name)->scoped_name;
		$res = std::make_shared<ASTVarDecl>();
		$res->AddVariable(symName);
	}

	// a variable with an assignment
	| IDENT[name] '=' expression[term] {
		t_str symName = context.AddScopedSymbol($name)->scoped_name;
		$res = std::make_shared<ASTVarDecl>(std::make_shared<ASTAssign>($name, $term));
		$res->AddVariable(symName);
	}
	;


/**
 * statement
 */
statement[res]
	: expression[term] ';'       { $res = $term; }
	| block[blk]                 { $res = $blk; }

	// function
	| function[func]             { $res = $func;  }
	
	// (multiple) return(s)
	| RET expressions[terms] ';' {
		$res = std::make_shared<ASTReturn>($terms);
	}

	// variable declarations
	// scalar / t_real
	| SCALARDECL {
			context.SetSymType(SymbolType::SCALAR);
		}
		variables[vars] ';'  { $res = $vars; }

	// vector
	| VECTORDECL INT[dim] {
			context.SetSymType(SymbolType::VECTOR);
			context.SetSymDims(std::size_t($dim));
		}
		variables[vars] ';' {
			$res = $vars;
		}

	// matrix
	| MATRIXDECL INT[dim1] INT[dim2] {
			context.SetSymType(SymbolType::MATRIX);
			context.SetSymDims(std::size_t($dim1), std::size_t($dim2));
		}
		variables[vars] ';' {
			$res = $vars;
		}

	// string with default size
	| STRINGDECL {
			context.SetSymType(SymbolType::STRING);
			context.SetSymDims(std::size_t(DEFAULT_STRING_SIZE));
		}
		variables[vars] ';'  { $res = $vars; }

	// string with a given (static) size
	| STRINGDECL INT[dim] {
			context.SetSymType(SymbolType::STRING);
			context.SetSymDims(std::size_t(std::size_t($dim)));
		}
		variables[vars] ';'  { $res = $vars; }

	// int
	| INTDECL {
			context.SetSymType(SymbolType::INT);
		}
		variables[vars] ';'  { $res = $vars; }

	// conditional
	| IF expression[cond] THEN statement[if_stmt] {
		$res = std::make_shared<ASTCond>($cond, $if_stmt); }
	| IF expression[cond] THEN statement[if_stmt] ELSE statement[else_stmt] {
		$res = std::make_shared<ASTCond>($cond, $if_stmt, $else_stmt); }

	// loop
	| LOOP expression[cond] DO statement[stmt] {
		$res = std::make_shared<ASTLoop>($cond, $stmt); }

	// break multiple loops
	| BREAK INT[num] ';' {
		$res = std::make_shared<ASTLoopBreak>($num);
	}

	// break current loop
	| BREAK ';' {
		$res = std::make_shared<ASTLoopBreak>();
	}

	// continue multiple loops
	| NEXT INT[num] ';' {
		$res = std::make_shared<ASTLoopNext>($num);
	}

	// continue current loop
	| NEXT ';' {
		$res = std::make_shared<ASTLoopNext>();
	}
	;


/**
 * function
 */
function[res]
	// single return value
	: FUNC typedecl[rettype] IDENT[ident] {
			context.EnterScope($ident);
		}
		'(' full_argumentlist[args] ')' {
			// register argument variables
			std::size_t argidx = 0;
			for(const auto& arg : $args->GetArgs())
			{
				Symbol* sym = context.AddScopedSymbol(std::get<0>(arg));
				sym->ty = std::get<1>(arg);
				sym->is_arg = true;
				sym->argidx = argidx;
				std::get<0>(sym->dims) = std::get<2>(arg);
				std::get<1>(sym->dims) = std::get<3>(arg);
				++argidx;
			}

			// register the function in the symbol map
			std::array<std::size_t, 2> retdims{{$rettype->GetDim(0), $rettype->GetDim(1)}};
			context.GetSymbols().AddFunc(
				context.GetScopeName(1), $ident, 
				$rettype->GetType(), $args->GetArgTypes(), &retdims);
		}
		block[blk] {
			$res = std::make_shared<ASTFunc>($ident, $rettype, $args, $blk);

			context.LeaveScope($ident);
		}

	// no return value
	| FUNC IDENT[ident] {
			context.EnterScope($ident);
		}
		'(' full_argumentlist[args] ')' {
			// register argument variables
			std::size_t argidx = 0;
			for(const auto& arg : $args->GetArgs())
			{
				Symbol* sym = context.AddScopedSymbol(std::get<0>(arg));
				sym->ty = std::get<1>(arg);
				sym->is_arg = true;
				sym->argidx = argidx;
				std::get<0>(sym->dims) = std::get<2>(arg);
				std::get<1>(sym->dims) = std::get<3>(arg);
				++argidx;
			}

			// register the function in the symbol map
			context.GetSymbols().AddFunc(
				context.GetScopeName(1), $ident, 
				SymbolType::VOID, $args->GetArgTypes());
		}
		block[blk] {
			auto rettype = std::make_shared<ASTTypeDecl>(SymbolType::VOID);
			$res = std::make_shared<ASTFunc>($ident, rettype, $args, $blk);

			context.LeaveScope($ident);
		}

	// multiple return values
	| FUNC '(' typelist[retargs] ')' IDENT[ident] {
			context.EnterScope($ident);
		}
		'(' full_argumentlist[args] ')' {
			// register argument variables
			std::size_t argidx = 0;
			for(const auto& arg : $args->GetArgs())
			{
				Symbol* sym = context.AddScopedSymbol(std::get<0>(arg));
				sym->ty = std::get<1>(arg);
				sym->is_arg = true;
				sym->argidx = argidx;
				std::get<0>(sym->dims) = std::get<2>(arg);
				std::get<1>(sym->dims) = std::get<3>(arg);
				++argidx;
			}

			// register the function in the symbol map
			std::vector<SymbolType> multirettypes = $retargs->GetArgTypes();
			context.GetSymbols().AddFunc(
				context.GetScopeName(1), $ident, 
				SymbolType::COMP, $args->GetArgTypes(), 
				nullptr, &multirettypes);
		}
		block[blk] {
			auto rettype = std::make_shared<ASTTypeDecl>(SymbolType::COMP);
			$res = std::make_shared<ASTFunc>($ident, rettype, $args, $blk, $retargs);
			
			context.LeaveScope($ident);
		}
	;


/**
 * declaration of variables
 */
typedecl[res]
	// scalars
	: SCALARDECL {
		$res = std::make_shared<ASTTypeDecl>(SymbolType::SCALAR);
	}
	
	// vectors
	| VECTORDECL INT[dim] {
		$res = std::make_shared<ASTTypeDecl>(SymbolType::VECTOR, $dim);
	}
	
	// matrices
	| MATRIXDECL INT[dim1] INT[dim2] 
	{ 
		$res = std::make_shared<ASTTypeDecl>(SymbolType::MATRIX, $dim1, $dim2);
	}
	
	// strings with default size
	| STRINGDECL { 
		$res = std::make_shared<ASTTypeDecl>(SymbolType::STRING, DEFAULT_STRING_SIZE);
	}

	// strings with given size
	| STRINGDECL INT[dim] { 
		$res = std::make_shared<ASTTypeDecl>(SymbolType::STRING, $dim);
	}

	// ints
	| INTDECL { 
		$res = std::make_shared<ASTTypeDecl>(SymbolType::INT); 
	}
	;


full_argumentlist[res]
	: argumentlist[args]        { $res = $args; }
	| %empty                    { $res = std::make_shared<ASTArgNames>(); }
	;


/**
 * a comma-separated list of type names and variable identifiers
 */
argumentlist[res]
	: typedecl[ty] IDENT[argname] ',' argumentlist[lst] {
		$lst->AddArg($argname, $ty->GetType(), $ty->GetDim(0), $ty->GetDim(1));
		$res = $lst;
	}
	| typedecl[ty] IDENT[argname] {
		$res = std::make_shared<ASTArgNames>();
		$res->AddArg($argname, $ty->GetType(), $ty->GetDim(0), $ty->GetDim(1));
	}
	;


/**
 * a comma-separated list of variable identifiers
 */
identlist[res]
	: IDENT[argname] ',' identlist[lst] {
		$lst->AddArg($argname);
		$res = $lst;
	}
	| IDENT[argname] {
		$res = std::make_shared<ASTArgNames>();
		$res->AddArg($argname);
	}
	;


/**
 * a comma-separated list of type names
 */
typelist[res]
	: typedecl[ty] ',' typelist[lst] {
		$lst->AddArg("ret", $ty->GetType(), $ty->GetDim(0), $ty->GetDim(1));
		$res = $lst;
	}
	| typedecl[ty] {
		$res = std::make_shared<ASTArgNames>();
		$res->AddArg("ret", $ty->GetType(), $ty->GetDim(0), $ty->GetDim(1));
	}
	;


/**
 * a comma-separated list of expressions
 */
expressions[res]
	: expression[num] ',' expressions[lst] {
		$lst->AddExpr($num);
		$res = $lst; 
	}
	| expression[num] {
		$res = std::make_shared<ASTExprList>();
		$res->AddExpr($num);
	}
	;


/**
 * a block of statements
 */
block[res]
	: '{' statements[stmts] '}'      { $res = $stmts; }
	;


/**
 * expression
 */
expression[res]
	: '(' expression[term] ')'             { $res = $term; }

	// unary expressions
	| '+' expression[term] %prec UNARY_OP  { $res = $term; }
	| '-' expression[term] %prec UNARY_OP  { $res = std::make_shared<ASTUMinus>($term); }
	| '|' expression[term] '|'             { $res = std::make_shared<ASTNorm>($term); }
	| expression[term] '\''                { $res = std::make_shared<ASTTransp>($term); }

	// unary boolean expression
	| NOT expression[term]                 { $res = std::make_shared<ASTBool>($term, ASTBool::NOT); }

	// binary expressions
	| expression[term1] '+' expression[term2]    { $res = std::make_shared<ASTPlus>($term1, $term2, 0); }
	| expression[term1] '-' expression[term2]    { $res = std::make_shared<ASTPlus>($term1, $term2, 1); }
	| expression[term1] '*' expression[term2]    { $res = std::make_shared<ASTMult>($term1, $term2, 0); }
	| expression[term1] '/' expression[term2]    { $res = std::make_shared<ASTMult>($term1, $term2, 1); }
	| expression[term1] '%' expression[term2]    { $res = std::make_shared<ASTMod>($term1, $term2); }
	| expression[term1] '^' expression[term2]    { $res = std::make_shared<ASTPow>($term1, $term2); }

	// binary boolean expressions
	| expression[term1] AND expression[term2]    { $res = std::make_shared<ASTBool>($term1, $term2, ASTBool::AND); }
	| expression[term1] OR expression[term2]     { $res = std::make_shared<ASTBool>($term1, $term2, ASTBool::OR); }
	| expression[term1] XOR expression[term2]    { $res = std::make_shared<ASTBool>($term1, $term2, ASTBool::XOR); }

	// comparison expressions
	| expression[term1] EQU expression[term2]    { $res = std::make_shared<ASTComp>($term1, $term2, ASTComp::EQU); }
	| expression[term1] NEQ expression[term2]    { $res = std::make_shared<ASTComp>($term1, $term2, ASTComp::NEQ); }
	| expression[term1] GT expression[term2]     { $res = std::make_shared<ASTComp>($term1, $term2, ASTComp::GT); }
	| expression[term1] LT expression[term2]     { $res = std::make_shared<ASTComp>($term1, $term2, ASTComp::LT); }
	| expression[term1] GEQ expression[term2]    { $res = std::make_shared<ASTComp>($term1, $term2, ASTComp::GEQ); }
	| expression[term1] LEQ expression[term2]    { $res = std::make_shared<ASTComp>($term1, $term2, ASTComp::LEQ); }

	// constants
	| REAL[num]               { $res = std::make_shared<ASTNumConst<t_real>>($num); }
	| INT[num]                { $res = std::make_shared<ASTNumConst<t_int>>($num); }
	| STRING[str]             { $res = std::make_shared<ASTStrConst>($str); }
	| '[' expressions[arr] ']' {  // scalar array
		$arr->SetScalarArray(true); 
		$res = $arr; 
	}

	// variable
	| IDENT[ident] %prec IDENT {
		// does the identifier name a constant?
		auto pair = context.GetConst($ident);
		if(std::get<0>(pair))
		{
			auto variant = std::get<1>(pair);
			if(std::holds_alternative<t_real>(variant))
				$res = std::make_shared<ASTNumConst<t_real>>(std::get<t_real>(variant));
			else if(std::holds_alternative<t_int>(variant))
				$res = std::make_shared<ASTNumConst<t_int>>(std::get<t_int>(variant));
			else if(std::holds_alternative<t_str>(variant))
				$res = std::make_shared<ASTStrConst>(std::get<t_str>(variant));
		}

		// identifier names a variable
		else
		{
			const Symbol* sym = context.FindScopedSymbol($ident);
			if(sym)
				++sym->refcnt;
			else
				error("Cannot find symbol \"" + $ident + "\".");

			$res = std::make_shared<ASTVar>($ident);
		}
	}

	// vector access and assignment
	| expression[term] '[' expression[idx] ']' opt_assign[opt_term] {
		if(!$opt_term)
		{	// array access into any vector expression
			$res = std::make_shared<ASTArrayAccess>($term, $idx);
		}
		else
		{	// assignment of a vector element
			if($term->type() != ASTType::Var)
			{
				error("Can only assign to an l-value symbol.");
				$res = nullptr;
			}
			else
			{
				auto var = std::static_pointer_cast<ASTVar>($term);
				$res = std::make_shared<ASTArrayAssign>(
					var->GetIdent(), $opt_term, $idx);
			}
		}
	}

	// vector ranged access and assignment
	| expression[term] '[' expression[idx1] RANGE expression[idx2] ']' opt_assign[opt_term] {
		if(!$opt_term)
		{	// array access into any vector expression
			$res = std::make_shared<ASTArrayAccess>(
				$term, $idx1, $idx2, nullptr, nullptr, true);
		}
		else
		{	// assignment of a vector element
			if($term->type() != ASTType::Var)
			{
				error("Can only assign to an l-value symbol.");
				$res = nullptr;
			}
			else
			{
				auto var = std::static_pointer_cast<ASTVar>($term);
				$res = std::make_shared<ASTArrayAssign>(
					var->GetIdent(), $opt_term, $idx1, $idx2, nullptr, nullptr, true);
			}
		}
	}

	// matrix access and assignment
	| expression[term] '[' expression[idx1] ',' expression[idx2] ']' opt_assign[opt_term] {
		if(!$opt_term)
		{	// array access into any matrix expression
			$res = std::make_shared<ASTArrayAccess>($term, $idx1, $idx2);
		}
		else
		{	// assignment of a matrix element
			if($term->type() != ASTType::Var)
			{
				error("Can only assign to an l-value symbol.");
				$res = nullptr;
			}
			else
			{
				auto var = std::static_pointer_cast<ASTVar>($term);
				$res = std::make_shared<ASTArrayAssign>(
					var->GetIdent(), $opt_term, $idx1, $idx2);
			}
		}
	}

	// matrix ranged access and assignment
	| expression[term] '[' expression[idx1] RANGE expression[idx2] ',' expression[idx3] RANGE expression[idx4] ']' opt_assign[opt_term] {
		if(!$opt_term)
		{	// array access into any matrix expression
			$res = std::make_shared<ASTArrayAccess>(
				$term, $idx1, $idx2, $idx3, $idx4, true, true);
		}
		else
		{	// assignment of a matrix element
			if($term->type() != ASTType::Var)
			{
				error("Can only assign to an l-value symbol.");
				$res = nullptr;
			}
			else
			{
				auto var = std::static_pointer_cast<ASTVar>($term);
				$res = std::make_shared<ASTArrayAssign>(
					var->GetIdent(), $opt_term, $idx1, $idx2, $idx3, $idx4, true, true);
			}
		}
	}

	// function calls
	| IDENT[ident] '(' ')' {
		const Symbol* sym = context.GetSymbols().FindSymbol($ident);
		if(sym && sym->ty == SymbolType::FUNC)
		{
			++sym->refcnt;
		}
		else
		{
			// TODO: move this check into semantics.cpp, as only the functions
			// that have already been parsed are registered at this point
			error("Cannot find function \"" + $ident + "\".");
		}

		$res = std::make_shared<ASTCall>($ident);
	}
	| IDENT[ident] '(' expressions[args] ')' {
		const Symbol* sym = context.GetSymbols().FindSymbol($ident);
		if(sym && sym->ty == SymbolType::FUNC)
		{
			++sym->refcnt;
		}
		else
		{
			// TODO: move this check into semantics.cpp, as only the functions
			// that have already been parsed are registered at this point
			error("Cannot find function \"" + $ident + "\".");
		}

		$res = std::make_shared<ASTCall>($ident, $args);
	}

	// (multiple) assignments
	| IDENT[ident] '=' expression[term] %prec '=' {
		$res = std::make_shared<ASTAssign>($ident, $term);
	}
	| ASSIGN identlist[idents] '=' expression[term] %prec '=' {
		$res = std::make_shared<ASTAssign>($idents->GetArgIdents(), $term);
	}
	;


/**
 * optional assignment
 */
opt_assign[res]
	: '=' expression[term] { $res = $term; }
	| %empty               { $res = nullptr; }
	;

%%
