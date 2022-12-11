/**
 * script grammar
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date dec-2022
 * @license see 'LICENSE' file
 */

#include "grammar.h"
#include "lexer.h"

#define DEFAULT_STRING_SIZE 128


void MatrixCalcGrammar::CreateGrammar(bool add_rules, bool add_semantics)
{
	// non-terminals
	start = std::make_shared<lalr1::NonTerminal>(START, "start");
	expression = std::make_shared<lalr1::NonTerminal>(EXPRESSION, "expression");
	expressions = std::make_shared<lalr1::NonTerminal>(EXPRESSIONS, "expressions");
	statement = std::make_shared<lalr1::NonTerminal>(STATEMENT, "statement");
	statements = std::make_shared<lalr1::NonTerminal>(STATEMENTS, "statements");
	variables = std::make_shared<lalr1::NonTerminal>(VARIABLES, "variables");
	full_argumentlist = std::make_shared<lalr1::NonTerminal>(FULL_ARGUMENTLIST, "full_argumentlist");
	argumentlist = std::make_shared<lalr1::NonTerminal>(ARGUMENTLIST, "argumentlist");
	identlist = std::make_shared<lalr1::NonTerminal>(IDENTLIST, "identlist");
	typelist = std::make_shared<lalr1::NonTerminal>(TYPELIST, "typelist");
	block = std::make_shared<lalr1::NonTerminal>(BLOCK, "block");
	function = std::make_shared<lalr1::NonTerminal>(FUNCTION, "function");
	typedecl = std::make_shared<lalr1::NonTerminal>(TYPEDECL, "typedecl");
	opt_assign = std::make_shared<lalr1::NonTerminal>(OPT_ASSIGN, "opt_assign");

	// terminals
	op_assign = std::make_shared<lalr1::Terminal>('=', "=");
	op_plus = std::make_shared<lalr1::Terminal>('+', "+");
	op_minus = std::make_shared<lalr1::Terminal>('-', "-");
	op_mult = std::make_shared<lalr1::Terminal>('*', "*");
	op_div = std::make_shared<lalr1::Terminal>('/', "/");
	op_mod = std::make_shared<lalr1::Terminal>('%', "%");
	op_pow = std::make_shared<lalr1::Terminal>('^', "^");
	op_norm = std::make_shared<lalr1::Terminal>('|', "|");
	op_trans = std::make_shared<lalr1::Terminal>('\'', "'");

	op_equ = std::make_shared<lalr1::Terminal>(static_cast<std::size_t>(Token::EQU), "==");
	op_neq = std::make_shared<lalr1::Terminal>(static_cast<std::size_t>(Token::NEQ), "!=");
	op_geq = std::make_shared<lalr1::Terminal>(static_cast<std::size_t>(Token::GEQ), ">=");
	op_leq = std::make_shared<lalr1::Terminal>(static_cast<std::size_t>(Token::LEQ), "<=");
	op_and = std::make_shared<lalr1::Terminal>(static_cast<std::size_t>(Token::AND), "&&");
	op_or = std::make_shared<lalr1::Terminal>(static_cast<std::size_t>(Token::OR), "||");
	op_xor = std::make_shared<lalr1::Terminal>(static_cast<std::size_t>(Token::XOR), "xor");
	op_gt = std::make_shared<lalr1::Terminal>('>', ">");
	op_lt = std::make_shared<lalr1::Terminal>('<', "<");
	op_not = std::make_shared<lalr1::Terminal>('!', "!");

	bracket_open = std::make_shared<lalr1::Terminal>('(', "(");
	bracket_close = std::make_shared<lalr1::Terminal>(')', ")");
	block_begin = std::make_shared<lalr1::Terminal>('{', "{");
	block_end = std::make_shared<lalr1::Terminal>('}', "}");
	array_begin = std::make_shared<lalr1::Terminal>('[', "[");
	array_end = std::make_shared<lalr1::Terminal>(']', "]");
	range = std::make_shared<lalr1::Terminal>(static_cast<std::size_t>(Token::RANGE), "~");

	comma = std::make_shared<lalr1::Terminal>(',', ",");
	stmt_end = std::make_shared<lalr1::Terminal>(';', ";");

	sym_real = std::make_shared<lalr1::Terminal>(static_cast<std::size_t>(Token::REAL), "real");
	sym_int = std::make_shared<lalr1::Terminal>(static_cast<std::size_t>(Token::INT), "integer");
	sym_str = std::make_shared<lalr1::Terminal>(static_cast<std::size_t>(Token::STR), "string");
	ident = std::make_shared<lalr1::Terminal>(static_cast<std::size_t>(Token::IDENT), "ident");

	real_decl = std::make_shared<lalr1::Terminal>(static_cast<std::size_t>(Token::SCALARDECL), "real_decl");
	vec_decl = std::make_shared<lalr1::Terminal>(static_cast<std::size_t>(Token::VECTORDECL), "vector_decl");
	mat_decl = std::make_shared<lalr1::Terminal>(static_cast<std::size_t>(Token::MATRIXDECL), "matrix_decl");
	int_decl = std::make_shared<lalr1::Terminal>(static_cast<std::size_t>(Token::INTDECL), "integer_decl");
	str_decl = std::make_shared<lalr1::Terminal>(static_cast<std::size_t>(Token::STRINGDECL), "string_decl");

	keyword_if = std::make_shared<lalr1::Terminal>(static_cast<std::size_t>(Token::IF), "if");
	keyword_then = std::make_shared<lalr1::Terminal>(static_cast<std::size_t>(Token::THEN), "then");
	keyword_else = std::make_shared<lalr1::Terminal>(static_cast<std::size_t>(Token::ELSE), "else");
	keyword_loop = std::make_shared<lalr1::Terminal>(static_cast<std::size_t>(Token::LOOP), "loop");
	keyword_do = std::make_shared<lalr1::Terminal>(static_cast<std::size_t>(Token::DO), "do");
	keyword_func = std::make_shared<lalr1::Terminal>(static_cast<std::size_t>(Token::FUNC), "func");
	keyword_ret = std::make_shared<lalr1::Terminal>(static_cast<std::size_t>(Token::RET), "ret");
	keyword_next = std::make_shared<lalr1::Terminal>(static_cast<std::size_t>(Token::NEXT), "next");
	keyword_break = std::make_shared<lalr1::Terminal>(static_cast<std::size_t>(Token::BREAK), "break");
	keyword_assign = std::make_shared<lalr1::Terminal>(static_cast<std::size_t>(Token::ASSIGN), "assign");


	// operator precedences and associativities
	// see: https://en.wikipedia.org/wiki/Order_of_operations
	comma->SetPrecedence(5, 'l');
	op_assign->SetPrecedence(10, 'r');

	op_xor->SetPrecedence(20, 'l');
	op_or->SetPrecedence(21, 'l');
	op_and->SetPrecedence(22, 'l');

	op_lt->SetPrecedence(30, 'l');
	op_gt->SetPrecedence(30, 'l');

	op_geq->SetPrecedence(30, 'l');
	op_leq->SetPrecedence(30, 'l');

	op_equ->SetPrecedence(40, 'l');
	op_neq->SetPrecedence(40, 'l');

	op_plus->SetPrecedence(50, 'l');
	op_minus->SetPrecedence(50, 'l');

	op_mult->SetPrecedence(60, 'l');
	op_div->SetPrecedence(60, 'l');
	op_mod->SetPrecedence(60, 'l');

	op_not->SetPrecedence(70, 'l');
	// TODO: unary_ops->SetPrecedence(75, 'r');

	op_pow->SetPrecedence(80, 'r');

	bracket_open->SetPrecedence(90, 'l');
	block_begin->SetPrecedence(90, 'l');
	array_begin->SetPrecedence(90, 'l');
	op_norm->SetPrecedence(90, 'l');
	op_trans->SetPrecedence(90, 'r');

	// for the if/else r/s conflict shift "else"
	// see: https://www.gnu.org/software/bison/manual/html_node/Non-Operators.html
	keyword_if->SetPrecedence(100, 'l');
	keyword_then->SetPrecedence(100, 'l');
	keyword_else->SetPrecedence(110, 'l');
	ident->SetPrecedence(120, 'l');
	keyword_func->SetPrecedence(0, 'l');


	// rule semantic id number
	lalr1::t_semantic_id semanticindex = 0;

	// --------------------------------------------------------------------------------
	// start
	// --------------------------------------------------------------------------------
	// rule 0: start -> statements
	if(add_rules)
	{
		start->AddRule({ statements }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[this](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match) return nullptr;
			auto stmts = std::dynamic_pointer_cast<ASTStmts>(args[0]);
			m_context.SetStatements(stmts);
			return stmts;
		}));
	}
	++semanticindex;
	// --------------------------------------------------------------------------------

	// --------------------------------------------------------------------------------
	// statements
	// --------------------------------------------------------------------------------
	// rule 1, list of statements: statements -> statement statements
	if(add_rules)
	{
		statements->AddRule({ statement, statements }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match) return nullptr;
			auto stmt = std::dynamic_pointer_cast<AST>(args[0]);
			auto stmts = std::dynamic_pointer_cast<ASTStmts>(args[1]);
			stmts->AddStatement(stmt);
			return stmts;
		}));
	}
	++semanticindex;

	// rule 2: statements -> epsilon
	if(add_rules)
	{
		statements->AddRule({ lalr1::g_eps }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, [[maybe_unused]] const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;
			return std::make_shared<ASTStmts>();
		}));
	}
	++semanticindex;
	// --------------------------------------------------------------------------------

	// --------------------------------------------------------------------------------
	// variables
	// --------------------------------------------------------------------------------
	// rule 3: several variables
	if(add_rules)
	{
		variables->AddRule({ ident, comma, variables }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[this](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match) return nullptr;

			auto varident = std::dynamic_pointer_cast<ASTStrConst>(args[0]);
			const t_str& name = varident->GetVal();
			t_str symName = m_context.AddScopedSymbol(name)->scoped_name;

			auto lst = std::dynamic_pointer_cast<ASTVarDecl>(args[2]);
			lst->AddVariable(symName);
			return lst;
		}));
	}
	++semanticindex;

	// rule 4: a single variable
	if(add_rules)
	{
		variables->AddRule({ ident }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[this](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match) return nullptr;

			auto ident = std::dynamic_pointer_cast<ASTStrConst>(args[0]);
			const t_str& name = ident->GetVal();
			t_str symName = m_context.AddScopedSymbol(name)->scoped_name;

			auto lst = std::make_shared<ASTVarDecl>();
			lst->AddVariable(symName);
			return lst;
		}));
	}
	++semanticindex;

	// rule 5: a variable with an assignment
	if(add_rules)
	{
		variables->AddRule({ ident, op_assign, expression }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[this](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match) return nullptr;

			auto ident = std::dynamic_pointer_cast<ASTStrConst>(args[0]);
			const t_str& name = ident->GetVal();
			t_str symName = m_context.AddScopedSymbol(name)->scoped_name;

			auto term = std::dynamic_pointer_cast<AST>(args[2]);

			auto lst = std::make_shared<ASTVarDecl>(std::make_shared<ASTAssign>(name, term));
			lst->AddVariable(symName);
			return lst;
		}));
	}
	++semanticindex;
	// --------------------------------------------------------------------------------

	// --------------------------------------------------------------------------------
	// statement
	// --------------------------------------------------------------------------------
	// rule 6: statement -> expression ;
	if(add_rules)
	{
		statement->AddRule({ expression, stmt_end }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match) return nullptr;
			return args[0];
		}));
	}
	++semanticindex;

	// rule 7: statement -> block
	if(add_rules)
	{
		statement->AddRule({ block }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match) return nullptr;
			return args[0];
		}));
	}
	++semanticindex;

	// rule 8: statement -> function
	if(add_rules)
	{
		statement->AddRule({ function }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match) return nullptr;
			return args[0];
		}));
	}
	++semanticindex;

	// rule 9: (multi-)return
	if(add_rules)
	{
		statement->AddRule({ keyword_ret, expressions, stmt_end }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match) return nullptr;
			auto terms = std::dynamic_pointer_cast<ASTExprList>(args[1]);
			return std::make_shared<ASTReturn>(terms);
		}));
	}
	++semanticindex;

	// rule 10: scalar declaration
	if(add_rules)
	{
		statement->AddRule({ real_decl, variables, stmt_end }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[this](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(args.size() == 1)
				m_context.SetSymType(SymbolType::SCALAR);

			if(!full_match)
				return nullptr;
			return args[1];
		}));
	}
	++semanticindex;

	// rule 11: vector declaration
	if(add_rules)
	{
		statement->AddRule({ vec_decl, sym_int, variables, stmt_end }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[this](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(args.size() == 2)
			{
				auto dim_node = std::dynamic_pointer_cast<ASTNumConst<t_int>>(args[1]);
				const t_int dim = dim_node->GetVal();

				m_context.SetSymType(SymbolType::VECTOR);
				m_context.SetSymDims(std::size_t(dim));
			}

			if(!full_match)
				return nullptr;
			return args[2];
		}));
	}
	++semanticindex;

	// rule 12: matrix declaration
	if(add_rules)
	{
		statement->AddRule({ mat_decl, sym_int, sym_int, variables, stmt_end }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[this](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(args.size() == 3)
			{
				auto dim1_node = std::dynamic_pointer_cast<ASTNumConst<t_int>>(args[1]);
				auto dim2_node = std::dynamic_pointer_cast<ASTNumConst<t_int>>(args[2]);
				const t_int dim1 = dim1_node->GetVal();
				const t_int dim2 = dim2_node->GetVal();

				m_context.SetSymType(SymbolType::MATRIX);
				m_context.SetSymDims(std::size_t(dim1), std::size_t(dim2));

			}

			if(!full_match)
				return nullptr;

			return args[3];
		}));
	}
	++semanticindex;

	// rule 13: string declaration with default size
	if(add_rules)
	{
		statement->AddRule({ str_decl, variables, stmt_end }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[this](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(args.size() == 1)
			{
				m_context.SetSymType(SymbolType::STRING);
				m_context.SetSymDims(std::size_t(DEFAULT_STRING_SIZE));
			}

			if(!full_match)
				return nullptr;
			return args[1];
		}));
	}
	++semanticindex;

	// rule 14: string declaration with given static size
	if(add_rules)
	{
		statement->AddRule({ str_decl, sym_int, variables, stmt_end }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[this](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(args.size() == 2)
			{
				auto dim_node = std::dynamic_pointer_cast<ASTNumConst<t_int>>(args[1]);
				const t_int dim = dim_node->GetVal();

				m_context.SetSymType(SymbolType::STRING);
				m_context.SetSymDims(std::size_t(dim));
			}

			if(!full_match)
				return nullptr;
			return args[2];
		}));
	}
	++semanticindex;

	// rule 15: int declaration
	if(add_rules)
	{
		statement->AddRule({ int_decl, variables, stmt_end }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[this](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(args.size() == /*1*/ 0)  // check
				m_context.SetSymType(SymbolType::INT);

			if(!full_match)
				return nullptr;
			return args[1];
		}));
	}
	++semanticindex;

	// rule 16: conditional
	if(add_rules)
	{
		statement->AddRule({ keyword_if, expression, keyword_then, statement }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;
			auto cond = std::dynamic_pointer_cast<AST>(args[1]);
			auto if_stmt = std::dynamic_pointer_cast<AST>(args[3]);
			return std::make_shared<ASTCond>(cond, if_stmt);
		}));
	}
	++semanticindex;

	// rule 17: conditional
	if(add_rules)
	{
		statement->AddRule({ keyword_if, expression, keyword_then, statement, keyword_else, statement }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;
			auto cond = std::dynamic_pointer_cast<AST>(args[1]);
			auto if_stmt = std::dynamic_pointer_cast<AST>(args[3]);
			auto else_stmt = std::dynamic_pointer_cast<AST>(args[5]);
			return std::make_shared<ASTCond>(cond, if_stmt, else_stmt);
		}));
	}
	++semanticindex;

	// rule 18: loop
	if(add_rules)
	{
		statement->AddRule({ keyword_loop, expression, keyword_do, statement }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;
			auto cond = std::dynamic_pointer_cast<AST>(args[1]);
			auto stmt = std::dynamic_pointer_cast<AST>(args[3]);
			return std::make_shared<ASTLoop>(cond, stmt);
		}));
	}
	++semanticindex;

	// rule 19: break current loop
	if(add_rules)
	{
		statement->AddRule({ keyword_break, stmt_end }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, [[maybe_unused]] const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;
			return std::make_shared<ASTLoopBreak>();
		}));
	}
	++semanticindex;

	// rule 20: break multiple loops
	if(add_rules)
	{
		statement->AddRule({ keyword_break, sym_int, stmt_end }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;
			auto num_node = std::dynamic_pointer_cast<ASTNumConst<t_int>>(args[1]);
			const t_int num = num_node->GetVal();

			return std::make_shared<ASTLoopBreak>(num);
		}));
	}
	++semanticindex;

	// rule 21: continue current loop
	if(add_rules)
	{
		statement->AddRule({ keyword_next, stmt_end }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, [[maybe_unused]] const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;
			return std::make_shared<ASTLoopNext>();
		}));
	}
	++semanticindex;

	// rule 22: continue multiple loops
	if(add_rules)
	{
		statement->AddRule({ keyword_next, sym_int, stmt_end }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;
			auto num_node = std::dynamic_pointer_cast<ASTNumConst<t_int>>(args[1]);
			const t_int num = num_node->GetVal();

			return std::make_shared<ASTLoopNext>(num);
		}));
	}
	++semanticindex;
	// --------------------------------------------------------------------------------

	// --------------------------------------------------------------------------------
	// typedecl
	// --------------------------------------------------------------------------------
	// rule 23: scalar declaration
	if(add_rules)
	{
		typedecl->AddRule({ real_decl }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, [[maybe_unused]] const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;
			return std::make_shared<ASTTypeDecl>(SymbolType::SCALAR);
		}));
	}
	++semanticindex;

	// rule 24: vector declaration
	if(add_rules)
	{
		typedecl->AddRule({ vec_decl, sym_int }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto dim_node = std::dynamic_pointer_cast<ASTNumConst<t_int>>(args[1]);
			const t_int dim = dim_node->GetVal();
			return std::make_shared<ASTTypeDecl>(SymbolType::VECTOR, dim);
		}));
	}
	++semanticindex;

	// rule 25: matrix declaration
	if(add_rules)
	{
		typedecl->AddRule({ mat_decl, sym_int, sym_int }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto dim_node1 = std::dynamic_pointer_cast<ASTNumConst<t_int>>(args[1]);
			auto dim_node2 = std::dynamic_pointer_cast<ASTNumConst<t_int>>(args[2]);
			const t_int dim1 = dim_node1->GetVal();
			const t_int dim2 = dim_node2->GetVal();
			return std::make_shared<ASTTypeDecl>(SymbolType::MATRIX, dim1, dim2);
		}));
	}
	++semanticindex;

	// rule 26: string declaration with default size
	if(add_rules)
	{
		typedecl->AddRule({ str_decl }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, [[maybe_unused]] const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;
			return std::make_shared<ASTTypeDecl>(SymbolType::STRING, DEFAULT_STRING_SIZE);
		}));
	}
	++semanticindex;

	// rule 27: string declaration with given static size
	if(add_rules)
	{
		typedecl->AddRule({ str_decl, sym_int }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto dim_node = std::dynamic_pointer_cast<ASTNumConst<t_int>>(args[1]);
			const t_int dim = dim_node->GetVal();
			return std::make_shared<ASTTypeDecl>(SymbolType::STRING, dim);
		}));
	}
	++semanticindex;

	// rule 28: int declaration
	if(add_rules)
	{
		typedecl->AddRule({ int_decl }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, [[maybe_unused]] const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;
			return std::make_shared<ASTTypeDecl>(SymbolType::INT);
		}));
	}
	++semanticindex;
	// --------------------------------------------------------------------------------

	// --------------------------------------------------------------------------------
	// opt_assign
	// --------------------------------------------------------------------------------
	// rule 29
	if(add_rules)
	{
		opt_assign->AddRule({ op_assign, expression }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;
			return args[1];
		}));
	}
	++semanticindex;

	// rule 30: opt_assign -> eps
	if(add_rules)
	{
		opt_assign->AddRule({ lalr1::g_eps }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[]([[maybe_unused]] bool full_match, [[maybe_unused]] const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			return nullptr;
		}));
	}
	++semanticindex;
	// --------------------------------------------------------------------------------

	// --------------------------------------------------------------------------------
	// block
	// --------------------------------------------------------------------------------
	// rule 31
	if(add_rules)
	{
		block->AddRule({ block_begin, statements, block_end }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;
			return args[1];
		}));
	}
	++semanticindex;
	// --------------------------------------------------------------------------------

	// --------------------------------------------------------------------------------
	// full_argumentlist
	// --------------------------------------------------------------------------------
	// rule 32
	if(add_rules)
	{
		full_argumentlist->AddRule({ argumentlist }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;
			return args[0];
		}));
	}
	++semanticindex;

	// rule 33: full_argumentlist -> eps
	if(add_rules)
	{
		full_argumentlist->AddRule({ lalr1::g_eps }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, [[maybe_unused]] const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;
			return std::make_shared<ASTArgNames>();
		}));
	}
	++semanticindex;
	// --------------------------------------------------------------------------------

	// --------------------------------------------------------------------------------
	// argumentlist
	// --------------------------------------------------------------------------------
	// rule 34
	if(add_rules)
	{
		argumentlist->AddRule({ typedecl, ident, comma, argumentlist }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto ty = std::dynamic_pointer_cast<ASTTypeDecl>(args[0]);
			auto argname = std::dynamic_pointer_cast<ASTStrConst>(args[1]);
			auto arglist = std::dynamic_pointer_cast<ASTArgNames>(args[3]);

			arglist->AddArg(argname->GetVal(), ty->GetType(), ty->GetDim(0), ty->GetDim(1));
			return arglist;
		}));
	}
	++semanticindex;

	// rule 35
	if(add_rules)
	{
		argumentlist->AddRule({ typedecl, ident }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto ty = std::dynamic_pointer_cast<ASTTypeDecl>(args[0]);
			auto argname = std::dynamic_pointer_cast<ASTStrConst>(args[1]);

			auto arglist = std::make_shared<ASTArgNames>();
			arglist->AddArg(argname->GetVal(), ty->GetType(), ty->GetDim(0), ty->GetDim(1));
			return arglist;
		}));
	}
	++semanticindex;
	// --------------------------------------------------------------------------------

	// --------------------------------------------------------------------------------
	// identlist
	// --------------------------------------------------------------------------------
	// rule 36
	if(add_rules)
	{
		identlist->AddRule({ ident, comma, identlist }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto argname = std::dynamic_pointer_cast<ASTStrConst>(args[0]);
			auto idents = std::dynamic_pointer_cast<ASTArgNames>(args[2]);

			idents->AddArg(argname->GetVal());
			return idents;
		}));
	}
	++semanticindex;

	// rule 37
	if(add_rules)
	{
		identlist->AddRule({ ident }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto argname = std::dynamic_pointer_cast<ASTStrConst>(args[0]);
			auto idents = std::make_shared<ASTArgNames>();

			idents->AddArg(argname->GetVal());
			return idents;
		}));
	}
	++semanticindex;
	// --------------------------------------------------------------------------------

	// --------------------------------------------------------------------------------
	// typelist
	// --------------------------------------------------------------------------------
	// rule 38
	if(add_rules)
	{
		typelist->AddRule({ typedecl, comma, typelist }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto ty = std::dynamic_pointer_cast<ASTTypeDecl>(args[0]);
			auto arglist = std::dynamic_pointer_cast<ASTArgNames>(args[2]);

			arglist->AddArg("ret", ty->GetType(), ty->GetDim(0), ty->GetDim(1));
			return arglist;
		}));
	}
	++semanticindex;

	// rule 39
	if(add_rules)
	{
		typelist->AddRule({ typedecl }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto ty = std::dynamic_pointer_cast<ASTTypeDecl>(args[0]);

			auto arglist = std::make_shared<ASTArgNames>();
			arglist->AddArg("ret", ty->GetType(), ty->GetDim(0), ty->GetDim(1));
			return arglist;
		}));
	}
	++semanticindex;
	// --------------------------------------------------------------------------------

	// --------------------------------------------------------------------------------
	// expressions
	// --------------------------------------------------------------------------------
	// rule 40
	if(add_rules)
	{
		expressions->AddRule({ expression, comma, expressions }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto expr = std::dynamic_pointer_cast<AST>(args[0]);
			auto exprs = std::dynamic_pointer_cast<ASTExprList>(args[2]);
			exprs->AddExpr(expr);
			return exprs;
		}));
	}
	++semanticindex;

	// rule 41
	if(add_rules)
	{
		expressions->AddRule({ expression }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto expr = std::dynamic_pointer_cast<AST>(args[0]);
			auto exprs = std::make_shared<ASTExprList>();
			exprs->AddExpr(expr);
			return exprs;
		}));
	}
	++semanticindex;
	// --------------------------------------------------------------------------------

	// --------------------------------------------------------------------------------
	// expression
	// --------------------------------------------------------------------------------
	// rule 42: expression -> ( expression )
	if(add_rules)
	{
		expression->AddRule({ bracket_open, expression, bracket_close }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			return args[1];
		}));
	}
	++semanticindex;

	// rule 43: unary plus
	if(add_rules)
	{
		expression->AddRule({ op_plus, expression }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;
			return args[1];
		}));
	}
	++semanticindex;

	// rule 44: unary minus
	if(add_rules)
	{
		expression->AddRule({ op_minus, expression }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto expr = std::dynamic_pointer_cast<AST>(args[1]);
			return std::make_shared<ASTUMinus>(expr);
		}));
	}
	++semanticindex;

	// rule 45: norm
	if(add_rules)
	{
		expression->AddRule({ op_norm, expression, op_norm }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto expr = std::dynamic_pointer_cast<AST>(args[1]);
			return std::make_shared<ASTNorm>(expr);
		}));
	}
	++semanticindex;

	// rule 46: boolean not
	if(add_rules)
	{
		expression->AddRule({ op_not, expression }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto expr = std::dynamic_pointer_cast<AST>(args[1]);
			return std::make_shared<ASTBool>(expr, ASTBool::NOT);
		}));
	}
	++semanticindex;

	// rule 47: expression -> expression + expression
	if(add_rules)
	{
		expression->AddRule({ expression, op_plus, expression }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto expr1 = std::dynamic_pointer_cast<AST>(args[0]);
			auto expr2 = std::dynamic_pointer_cast<AST>(args[2]);
			return std::make_shared<ASTPlus>(expr1, expr2, 0);
		}));
	}
	++semanticindex;

	// rule 48: expression -> expression - expression
	if(add_rules)
	{
		expression->AddRule({ expression, op_minus, expression }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto expr1 = std::dynamic_pointer_cast<AST>(args[0]);
			auto expr2 = std::dynamic_pointer_cast<AST>(args[2]);
			return std::make_shared<ASTPlus>(expr1, expr2, 1);
		}));
	}
	++semanticindex;

	// rule 49: expression -> expression * expression
	if(add_rules)
	{
		expression->AddRule({ expression, op_mult, expression }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto expr1 = std::dynamic_pointer_cast<AST>(args[0]);
			auto expr2 = std::dynamic_pointer_cast<AST>(args[2]);
			return std::make_shared<ASTMult>(expr1, expr2, 0);
		}));
	}
	++semanticindex;

	// rule 50: expression -> expression / expression
	if(add_rules)
	{
		expression->AddRule({ expression, op_div, expression }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto expr1 = std::dynamic_pointer_cast<AST>(args[0]);
			auto expr2 = std::dynamic_pointer_cast<AST>(args[2]);
			return std::make_shared<ASTMult>(expr1, expr2, 1);
		}));
	}
	++semanticindex;

	// rule 51: expression -> expression % expression
	if(add_rules)
	{
		expression->AddRule({ expression, op_mod, expression }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto expr1 = std::dynamic_pointer_cast<AST>(args[0]);
			auto expr2 = std::dynamic_pointer_cast<AST>(args[2]);
			return std::make_shared<ASTMod>(expr1, expr2);
		}));
	}
	++semanticindex;

	// rule 52: expression -> expression ^ expression
	if(add_rules)
	{
		expression->AddRule({ expression, op_pow, expression }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto expr1 = std::dynamic_pointer_cast<AST>(args[0]);
			auto expr2 = std::dynamic_pointer_cast<AST>(args[2]);
			return std::make_shared<ASTPow>(expr1, expr2);
		}));
	}
	++semanticindex;

	// rule 53: expression -> expression AND expression
	if(add_rules)
	{
		expression->AddRule({ expression, op_and, expression }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto expr1 = std::dynamic_pointer_cast<AST>(args[0]);
			auto expr2 = std::dynamic_pointer_cast<AST>(args[2]);
			return std::make_shared<ASTBool>(expr1, expr2, ASTBool::AND);
		}));
	}
	++semanticindex;

	// rule 54: expression -> expression OR expression
	if(add_rules)
	{
		expression->AddRule({ expression, op_or, expression }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto expr1 = std::dynamic_pointer_cast<AST>(args[0]);
			auto expr2 = std::dynamic_pointer_cast<AST>(args[2]);
			return std::make_shared<ASTBool>(expr1, expr2, ASTBool::OR);
		}));
	}
	++semanticindex;

	// rule 55: expression -> expression XOR expression
	if(add_rules)
	{
		expression->AddRule({ expression, op_xor, expression }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto expr1 = std::dynamic_pointer_cast<AST>(args[0]);
			auto expr2 = std::dynamic_pointer_cast<AST>(args[2]);
			return std::make_shared<ASTBool>(expr1, expr2, ASTBool::XOR);
		}));
	}
	++semanticindex;

	// rule 56: expression -> expression == expression
	if(add_rules)
	{
		expression->AddRule({ expression, op_equ, expression }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto expr1 = std::dynamic_pointer_cast<AST>(args[0]);
			auto expr2 = std::dynamic_pointer_cast<AST>(args[2]);
			return std::make_shared<ASTComp>(expr1, expr2, ASTComp::EQU);
		}));
	}
	++semanticindex;

	// rule 57: expression -> expression != expression
	if(add_rules)
	{
		expression->AddRule({ expression, op_neq, expression }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto expr1 = std::dynamic_pointer_cast<AST>(args[0]);
			auto expr2 = std::dynamic_pointer_cast<AST>(args[2]);
			return std::make_shared<ASTComp>(expr1, expr2, ASTComp::NEQ);
		}));
	}
	++semanticindex;

	// rule 58: expression -> expression > expression
	if(add_rules)
	{
		expression->AddRule({ expression, op_gt, expression }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto expr1 = std::dynamic_pointer_cast<AST>(args[0]);
			auto expr2 = std::dynamic_pointer_cast<AST>(args[2]);
			return std::make_shared<ASTComp>(expr1, expr2, ASTComp::GT);
		}));
	}
	++semanticindex;

	// rule 59: expression -> expression < expression
	if(add_rules)
	{
		expression->AddRule({ expression, op_lt, expression }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto expr1 = std::dynamic_pointer_cast<AST>(args[0]);
			auto expr2 = std::dynamic_pointer_cast<AST>(args[2]);
			return std::make_shared<ASTComp>(expr1, expr2, ASTComp::LT);
		}));
	}
	++semanticindex;

	// rule 60: expression -> expression >= expression
	if(add_rules)
	{
		expression->AddRule({ expression, op_geq, expression }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto expr1 = std::dynamic_pointer_cast<AST>(args[0]);
			auto expr2 = std::dynamic_pointer_cast<AST>(args[2]);
			return std::make_shared<ASTComp>(expr1, expr2, ASTComp::GEQ);
		}));
	}
	++semanticindex;

	// rule 61: expression -> expression <= expression
	if(add_rules)
	{
		expression->AddRule({ expression, op_leq, expression }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto expr1 = std::dynamic_pointer_cast<AST>(args[0]);
			auto expr2 = std::dynamic_pointer_cast<AST>(args[2]);
			return std::make_shared<ASTComp>(expr1, expr2, ASTComp::LEQ);
		}));
	}
	++semanticindex;

	// rule 62: expression -> real
	if(add_rules)
	{
		expression->AddRule({ sym_real }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto num_node = std::dynamic_pointer_cast<ASTNumConst<t_real>>(args[0]);
			const t_real num = num_node->GetVal();
			return std::make_shared<ASTNumConst<t_real>>(num);
		}));
	}
	++semanticindex;

	// rule 63: expression -> int
	if(add_rules)
	{
		expression->AddRule({ sym_int }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto num_node = std::dynamic_pointer_cast<ASTNumConst<t_int>>(args[0]);
			const t_int num = num_node->GetVal();
			return std::make_shared<ASTNumConst<t_int>>(num);
		}));
	}
	++semanticindex;

	// rule 64: expression -> string
	if(add_rules)
	{
		expression->AddRule({ sym_str }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto str_node = std::dynamic_pointer_cast<ASTStrConst>(args[0]);
			const t_str& str = str_node->GetVal();
			return std::make_shared<ASTStrConst>(str);
		}));
	}
	++semanticindex;

	// rule 65: scalar array
	if(add_rules)
	{
		expression->AddRule({ array_begin, expressions, array_end }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto exprs = std::dynamic_pointer_cast<ASTExprList>(args[1]);
			exprs->SetScalarArray(true);
			return exprs;
		}));
	}
	++semanticindex;

	// rule 66: variable
	if(add_rules)
	{
		expression->AddRule({ ident }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[this](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto identnode = std::dynamic_pointer_cast<ASTStrConst>(args[0]);
			const t_str& identstr = identnode->GetVal();

			// does the identifier name a constant?
			auto pair = m_context.GetConst(identstr);
			if(std::get<0>(pair))
			{
				auto variant = std::get<1>(pair);
				if(std::holds_alternative<t_real>(variant))
					return std::make_shared<ASTNumConst<t_real>>(std::get<t_real>(variant));
				else if(std::holds_alternative<t_int>(variant))
					return std::make_shared<ASTNumConst<t_int>>(std::get<t_int>(variant));
				else if(std::holds_alternative<t_str>(variant))
					return std::make_shared<ASTStrConst>(std::get<t_str>(variant));
			}

			// identifier names a variable
			else
			{
				const Symbol* sym = m_context.FindScopedSymbol(identstr);
				if(sym)
					++sym->refcnt;
				else
					std::cerr << "Cannot find symbol \"" << identstr << "\"." << std::endl;
				return std::make_shared<ASTVar>(identstr);
			}

			return nullptr;
		}));
	}
	++semanticindex;

	// rule 67: vector access and assignment
	if(add_rules)
	{
		expression->AddRule({ expression /*0*/, array_begin,
			expression /*2*/, array_end,
			opt_assign /*4*/ }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[this](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto term = std::dynamic_pointer_cast<AST>(args[0]);
			auto idx = std::dynamic_pointer_cast<AST>(args[2]);

			if(!args[4])
			{
				// array access into any vector expression
				return std::make_shared<ASTArrayAccess>(term, idx);
			}
			else
			{
				// assignment of a vector element
				if(term->type() != ASTType::Var)
				{
					std::cerr << "Can only assign to an l-value symbol." << std::endl;
					return nullptr;
				}
				else
				{
					auto opt_term = std::dynamic_pointer_cast<AST>(args[4]);
					auto var = std::static_pointer_cast<ASTVar>(term);
					return std::make_shared<ASTArrayAssign>(
						var->GetIdent(), opt_term, idx);
				}
			}

			return nullptr;
		}));
	}
	++semanticindex;

	// rule 68: vector ranged access and assignment
	if(add_rules)
	{
		expression->AddRule({ expression /*0*/, array_begin,
			expression /*2*/, range, expression /*4*/, array_end,
			opt_assign /*6*/ }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[this](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto term = std::dynamic_pointer_cast<AST>(args[0]);
			auto idx1 = std::dynamic_pointer_cast<AST>(args[2]);
			auto idx2 = std::dynamic_pointer_cast<AST>(args[4]);

			if(!args[6])
			{
				// array access into any vector expression
				return std::make_shared<ASTArrayAccess>(
					term, idx1, idx2, nullptr, nullptr, true);
			}
			else
			{
				// assignment of a vector element
				if(term->type() != ASTType::Var)
				{
					std::cerr << "Can only assign to an l-value symbol." << std::endl;
					return nullptr;
				}
				else
				{
					auto opt_term = std::dynamic_pointer_cast<AST>(args[6]);
					auto var = std::static_pointer_cast<ASTVar>(term);
					return std::make_shared<ASTArrayAssign>(
						var->GetIdent(), opt_term,
						idx1, idx2, nullptr, nullptr, true);
				}
			}

			return nullptr;
		}));
	}
	++semanticindex;

	// rule 69: matrix access and assignment
	if(add_rules)
	{
		expression->AddRule({ expression /*0*/, array_begin,
			expression /*2*/, comma, expression /*4*/,
			array_end, opt_assign /*6*/ }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[this](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto term = std::dynamic_pointer_cast<AST>(args[0]);
			auto idx1 = std::dynamic_pointer_cast<AST>(args[2]);
			auto idx2 = std::dynamic_pointer_cast<AST>(args[4]);

			if(!args[6])
			{
				// array access into any matrix expression
				return std::make_shared<ASTArrayAccess>(term, idx1, idx2);
			}
			else
			{
				// assignment of a matrix element
				if(term->type() != ASTType::Var)
				{
					std::cerr << "Can only assign to an l-value symbol." << std::endl;
					return nullptr;
				}
				else
				{
					auto opt_term = std::dynamic_pointer_cast<AST>(args[6]);
					auto var = std::static_pointer_cast<ASTVar>(term);
					return std::make_shared<ASTArrayAssign>(
						var->GetIdent(), opt_term, idx1, idx2);
				}
			}

			return nullptr;
		}));
	}
	++semanticindex;

	// rule 70: matrix ranged access and assignment
	if(add_rules)
	{
		expression->AddRule({ expression /*0*/, array_begin,
			expression /*2*/, range, expression /*4*/, comma,
			expression /*6*/, range, expression /*8*/,
			array_end, opt_assign /*10*/ }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[this](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto term = std::dynamic_pointer_cast<AST>(args[0]);
			auto idx1 = std::dynamic_pointer_cast<AST>(args[2]);
			auto idx2 = std::dynamic_pointer_cast<AST>(args[4]);
			auto idx3 = std::dynamic_pointer_cast<AST>(args[6]);
			auto idx4 = std::dynamic_pointer_cast<AST>(args[8]);

			if(!args[10])
			{
				// array access into any matrix expression
				return std::make_shared<ASTArrayAccess>(
					term, idx1, idx2, idx3, idx4, true, true);
			}
			else
			{
				// assignment of a matrix element
				if(term->type() != ASTType::Var)
				{
					std::cerr << "Can only assign to an l-value symbol." << std::endl;
					return nullptr;
				}
				else
				{
					auto opt_term = std::dynamic_pointer_cast<AST>(args[10]);
					auto var = std::static_pointer_cast<ASTVar>(term);
					return std::make_shared<ASTArrayAssign>(
						var->GetIdent(), opt_term,
						idx1, idx2, idx3, idx4, true, true);
				}
			}

			return nullptr;
		}));
	}
	++semanticindex;

	// rule 71: function call without arguments
	if(add_rules)
	{
		expression->AddRule({ ident, bracket_open, bracket_close }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[this](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto identnode = std::dynamic_pointer_cast<ASTStrConst>(args[0]);
			const t_str& funcname = identnode->GetVal();
			const Symbol* sym = m_context.GetSymbols().FindSymbol(funcname);

			if(sym && sym->ty == SymbolType::FUNC)
			{
				++sym->refcnt;
			}
			else
			{
				// TODO: move this check into semantics.cpp, as only the functions
				// that have already been parsed are registered at this point
				std::cerr << "Cannot find function \"" << funcname << "\"." << std::endl;
			}

			return std::make_shared<ASTCall>(funcname);
		}));
	}
	++semanticindex;

	// rule 72: function call with arguments
	if(add_rules)
	{
		expression->AddRule({ ident, bracket_open, expressions, bracket_close }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[this](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto identnode = std::dynamic_pointer_cast<ASTStrConst>(args[0]);
			const t_str& funcname = identnode->GetVal();
			const Symbol* sym = m_context.GetSymbols().FindSymbol(funcname);

			if(sym && sym->ty == SymbolType::FUNC)
			{
				++sym->refcnt;
			}
			else
			{
				// TODO: move this check into semantics.cpp, as only the functions
				// that have already been parsed are registered at this point
				std::cerr << "Cannot find function \"" << funcname << "\"." << std::endl;
			}

			auto funcargs = std::dynamic_pointer_cast<ASTExprList>(args[2]);
			return std::make_shared<ASTCall>(funcname, funcargs);
		}));
	}
	++semanticindex;

	// rule 73: assignment
	if(add_rules)
	{
		expression->AddRule({ ident, op_assign, expression }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[this](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto identnode = std::dynamic_pointer_cast<ASTStrConst>(args[0]);
			const t_str& ident = identnode->GetVal();

			auto term = std::dynamic_pointer_cast<AST>(args[2]);
			return std::make_shared<ASTAssign>(ident, term);
		}));
	}
	++semanticindex;

	// rule 74: multi-assignment
	if(add_rules)
	{
		expression->AddRule({ keyword_assign, identlist, op_assign, expression }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[this](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto idents = std::dynamic_pointer_cast<ASTArgNames>(args[1]);
			auto term = std::dynamic_pointer_cast<AST>(args[3]);
			return std::make_shared<ASTAssign>(idents->GetArgIdents(), term);
		}));
	}
	++semanticindex;

	// rule 75: transpose
	if(add_rules)
	{
		expression->AddRule({ expression, op_trans }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(!full_match)
				return nullptr;

			auto expr = std::dynamic_pointer_cast<AST>(args[0]);
			return std::make_shared<ASTTransp>(expr);
		}));
	}
	++semanticindex;
	// --------------------------------------------------------------------------------

	// --------------------------------------------------------------------------------
	// function
	// --------------------------------------------------------------------------------
	// rule 76: function with a single return value
	if(add_rules)
	{
		function->AddRule({ keyword_func, typedecl /*1*/, ident /*2*/,
			bracket_open, full_argumentlist /*4*/, bracket_close,
			block /*6*/ }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[this](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(args.size() == 3)
			{
				auto funcname = std::dynamic_pointer_cast<ASTStrConst>(args[2]);
				m_context.EnterScope(funcname->GetVal());
			}
			else if(args.size() == /*6*/ 5) // check
			{
				auto rettype = std::dynamic_pointer_cast<ASTTypeDecl>(args[1]);
				auto funcname = std::dynamic_pointer_cast<ASTStrConst>(args[2]);
				auto funcargs = std::dynamic_pointer_cast<ASTArgNames>(args[4]);

				// register argument variables
				std::size_t argidx = 0;
				for(const auto& arg : funcargs->GetArgs())
				{
					Symbol* sym = m_context.AddScopedSymbol(std::get<0>(arg));
					sym->ty = std::get<1>(arg);
					sym->is_arg = true;
					sym->argidx = argidx;
					std::get<0>(sym->dims) = std::get<2>(arg);
					std::get<1>(sym->dims) = std::get<3>(arg);
					++argidx;
				}

				// register the function in the symbol map
				std::array<std::size_t, 2> retdims{{
					rettype->GetDim(0), rettype->GetDim(1)}};
				m_context.GetSymbols().AddFunc(
					m_context.GetScopeName(1), funcname->GetVal(),
					rettype->GetType(), funcargs->GetArgTypes(), &retdims);
			}

			if(!full_match)
				return nullptr;

			auto rettype = std::dynamic_pointer_cast<ASTTypeDecl>(args[1]);
			auto funcname = std::dynamic_pointer_cast<ASTStrConst>(args[2]);
			auto funcargs = std::dynamic_pointer_cast<ASTArgNames>(args[4]);
			auto funcblock = std::dynamic_pointer_cast<ASTStmts>(args[6]);

			auto res = std::make_shared<ASTFunc>(
				funcname->GetVal(), rettype, funcargs, funcblock);
			m_context.LeaveScope(funcname->GetVal());
			return res;
		}));
	}
	++semanticindex;

	// rule 77: function with no return value
	if(add_rules)
	{
		function->AddRule({ keyword_func, ident /*1*/,
			bracket_open, full_argumentlist /*3*/, bracket_close,
			block /*5*/ }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[this](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(args.size() == 2)
			{
				auto funcname = std::dynamic_pointer_cast<ASTStrConst>(args[1]);
				m_context.EnterScope(funcname->GetVal());
			}
			else if(args.size() == /*5*/ 4) // check
			{
				auto funcname = std::dynamic_pointer_cast<ASTStrConst>(args[1]);
				auto funcargs = std::dynamic_pointer_cast<ASTArgNames>(args[3]);

				// register argument variables
				std::size_t argidx = 0;
				for(const auto& arg : funcargs->GetArgs())
				{
					Symbol* sym = m_context.AddScopedSymbol(std::get<0>(arg));
					sym->ty = std::get<1>(arg);
					sym->is_arg = true;
					sym->argidx = argidx;
					std::get<0>(sym->dims) = std::get<2>(arg);
					std::get<1>(sym->dims) = std::get<3>(arg);
					++argidx;
				}

				// register the function in the symbol map
				m_context.GetSymbols().AddFunc(
					m_context.GetScopeName(1), funcname->GetVal(),
					SymbolType::VOID, funcargs->GetArgTypes());
			}

			if(!full_match)
				return nullptr;

			auto rettype = std::make_shared<ASTTypeDecl>(SymbolType::VOID);
			auto funcname = std::dynamic_pointer_cast<ASTStrConst>(args[1]);
			auto funcargs = std::dynamic_pointer_cast<ASTArgNames>(args[3]);
			auto funcblock = std::dynamic_pointer_cast<ASTStmts>(args[5]);

			auto res = std::make_shared<ASTFunc>(
				funcname->GetVal(), rettype, funcargs, funcblock);
			m_context.LeaveScope(funcname->GetVal());
			return res;
		}));
	}
	++semanticindex;

	// rule 78: function with multiple return values
	if(add_rules)
	{
		function->AddRule({ keyword_func,
			bracket_open, typelist /*2*/, bracket_close,
			ident /*4*/, bracket_open, full_argumentlist /*6*/, bracket_close,
			block /*8*/ }, semanticindex);
	}
	if(add_semantics)
	{
		rules.emplace(std::make_pair(semanticindex,
		[this](bool full_match, const lalr1::t_semanticargs& args, [[maybe_unused]] lalr1::t_astbaseptr retval) -> lalr1::t_astbaseptr
		{
			if(args.size() == 5)
			{
				auto funcname = std::dynamic_pointer_cast<ASTStrConst>(args[4]);
				m_context.EnterScope(funcname->GetVal());
			}
			else if(args.size() == /*8*/ 7) // check
			{
				auto retargs = std::dynamic_pointer_cast<ASTArgNames>(args[2]);
				auto funcname = std::dynamic_pointer_cast<ASTStrConst>(args[4]);
				auto funcargs = std::dynamic_pointer_cast<ASTArgNames>(args[6]);

				// register argument variables
				std::size_t argidx = 0;
				for(const auto& arg : funcargs->GetArgs())
				{
					Symbol* sym = m_context.AddScopedSymbol(std::get<0>(arg));
					sym->ty = std::get<1>(arg);
					sym->is_arg = true;
					sym->argidx = argidx;
					std::get<0>(sym->dims) = std::get<2>(arg);
					std::get<1>(sym->dims) = std::get<3>(arg);
					++argidx;
				}

				// register the function in the symbol map
				std::vector<SymbolType> multirettypes = retargs->GetArgTypes();
				m_context.GetSymbols().AddFunc(
					m_context.GetScopeName(1), funcname->GetVal(),
					SymbolType::COMP, funcargs->GetArgTypes(),
					nullptr, &multirettypes);
			}

			if(!full_match)
				return nullptr;

			auto rettype = std::make_shared<ASTTypeDecl>(SymbolType::COMP);
			auto retargs = std::dynamic_pointer_cast<ASTArgNames>(args[2]);
			auto funcname = std::dynamic_pointer_cast<ASTStrConst>(args[4]);
			auto funcargs = std::dynamic_pointer_cast<ASTArgNames>(args[6]);
			auto funcblock = std::dynamic_pointer_cast<ASTStmts>(args[8]);

			auto res = std::make_shared<ASTFunc>(
				funcname->GetVal(), rettype, funcargs, funcblock, retargs);
			m_context.LeaveScope(funcname->GetVal());
			return res;
		}));
	}
	++semanticindex;
	// --------------------------------------------------------------------------------
}
