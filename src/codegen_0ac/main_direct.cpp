/**
 * 0-ac parser entry point
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date dec-2022
 * @license see 'LICENSE' file
 */

// g++ -DUSE_DIRECT_PARSER -I.. -std=c++20 -o parser ../parser_direct/parser.cpp ../parser_direct/grammar.cpp ../parser_direct/lexer.cpp main_direct.cpp asm.cpp arr.cpp func.cpp ops.cpp var.cpp ../ast/printast.cpp  -llalr1 -lboost_program_options

#include "ast/ast.h"
#include "ast/printast.h"
#include "ast/semantics.h"
#include "common/helpers.h"
#include "common/version.h"
#include "common/ext_funcs.h"
#include "parser_direct/lexer.h"
#include "parser_direct/grammar.h"
#include "parser.h"
#include "asm.h"

#include <fstream>
#include <locale>

#if __has_include(<filesystem>)
	#include <filesystem>
	namespace fs = std::filesystem;
#elif __has_include(<boost/filesystem.hpp>)
	#include <boost/filesystem.hpp>
	namespace fs = boost::filesystem;
#else
	#error No filesystem support found.
#endif

#include <boost/program_options.hpp>
namespace args = boost::program_options;

using namespace lalr1;


int main(int argc, char** argv)
{
	try
	{
		t_timepoint start_time  = t_clock::now();

		std::ios_base::sync_with_stdio(0);
		std::locale loc{};
		std::locale::global(loc);

		std::cout << "Matrix expression 0ac compiler version " << MCALC_VER
			<< " by Tobias Weber <tobias.weber@tum.de>, 2022."
			<< std::endl;
		std::cout << "Internal data type lengths:"
			<< " real: " << sizeof(t_real)*8 << " bits,"
			<< " int: " << sizeof(t_int)*8 << " bits."
			<< std::endl;


		// --------------------------------------------------------------------
		// get program arguments
		// --------------------------------------------------------------------
		std::vector<std::string> progs;
		bool show_symbols = false;
		bool show_ast = false;
		bool debug = false;
		std::string outprog;

		args::options_description arg_descr("Compiler arguments");
		arg_descr.add_options()
			("out,o", args::value(&outprog), "compiled program output")
			("symbols,s", args::bool_switch(&show_symbols), "output symbol table")
			("ast,a", args::bool_switch(&show_ast), "output syntax tree")
			("debug,d", args::bool_switch(&debug), "output debug infos")
			("program", args::value<decltype(progs)>(&progs), "input program to compile");

		args::positional_options_description posarg_descr;
		posarg_descr.add("program", -1);

		auto argparser = args::command_line_parser{argc, argv};
		argparser.style(args::command_line_style::default_style);
		argparser.options(arg_descr);
		argparser.positional(posarg_descr);

		args::variables_map mapArgs;
		auto parsedArgs = argparser.run();
		args::store(parsedArgs, mapArgs);
		args::notify(mapArgs);

		if(progs.size() == 0)
		{
			std::cerr << "Please specify an input program.\n" << std::endl;
			std::cout << arg_descr << std::endl;
			return 0;
		}

		// input file
		const std::string& inprog = progs[0];

		// output files
		if(outprog == "")
		{
			fs::path outfile(inprog);
			outfile = outfile.filename();
			outfile.replace_extension("");
			outprog = outfile.string();
		}

		std::string outprog_ast = outprog + "_ast.xml";
		std::string outprog_syms = outprog + "_syms.txt";
		std::string outprog_0ac = outprog + ".bin";
		// --------------------------------------------------------------------



		// --------------------------------------------------------------------
		// parse input
		// --------------------------------------------------------------------
		std::cout << "Parsing \"" << inprog << "\"..." << std::endl;

		std::ifstream ifstr{inprog};
		if(!ifstr)
		{
			std::cerr << "Cannot open \"" << inprog << "\"." << std::endl;
			return -1;
		}

		MatrixCalcGrammar grammar;
		ParserContext& ctx = grammar.GetContext();
		grammar.CreateGrammar(false, true);
		const auto& rules = grammar.GetSemanticRules();

		// register external runtime functions which should be available to the compiler
		add_ext_funcs<t_real, t_int>(ctx);

		Lexer lexer(&ifstr);
		const std::vector<t_toknode>& tokens = lexer.GetAllTokens();
		if(debug)
		{
			std::cout << "Input tokens:\n";
			for(const t_toknode& tok : tokens)
			{
				auto linerange = tok->GetLineRange();
				std::cout << "\tid = " << tok->GetId()
					<< ", idx = " << tok->GetTableIndex();
				if(std::isprint(tok->GetId()))
					std::cout << ", ch = \"" << char(tok->GetId()) << "\"";
				if(linerange)
					std::cout << ", lines = " << linerange->first << ".." << linerange->second;
				std::cout << std::endl;
			}
		}

		Parser parser;
		parser.SetSemanticRules(&rules);
		parser.SetDebug(debug);

		t_astbaseptr ast = parser.Parse(tokens);
		if(!ast || !ctx.GetStatements())
		{
			std::cerr << "Parser reports failure." << std::endl;
			return -1;
		}

		if(show_symbols)
		{
			std::cout << "Writing symbol table to \"" << outprog_syms << "\"..." << std::endl;

			std::ofstream ostrSyms{outprog_syms};
			//ostrSyms << "\nSymbol table:\n";
			ostrSyms << ctx.GetSymbols() << std::endl;
		}

		if(show_ast)
		{
			std::cout << "Writing AST to \"" << outprog_ast << "\"..." << std::endl;

			std::ofstream ostrAST{outprog_ast};
			ASTPrinter printer{&ostrAST};

			ostrAST << "<ast>\n";
			auto stmts = ctx.GetStatements()->GetStatementList();
			for(auto iter=stmts.rbegin(); iter!=stmts.rend(); ++iter)
			{
				(*iter)->accept(&printer);
				ostrAST << "\n";
			}
			ostrAST << "</ast>" << std::endl;
		}
		// --------------------------------------------------------------------



		// --------------------------------------------------------------------
		// 0AC generation
		// --------------------------------------------------------------------
		std::cout << "Generating code: \""
			<< inprog << "\" -> \"" << outprog_0ac << "\"..." << std::endl;

		std::ofstream ofstr{outprog_0ac};
		std::ostream* ostr = &ofstr;
		ostr->precision(std::numeric_limits<t_real>::digits10);

		ZeroACAsm zeroacasm{&ctx.GetSymbols(), ostr};
		zeroacasm.Start();
		auto stmts = ctx.GetStatements()->GetStatementList();
		for(auto iter=stmts.rbegin(); iter!=stmts.rend(); ++iter)
			(*iter)->accept(&zeroacasm);
		zeroacasm.Finish();
		// --------------------------------------------------------------------


		auto [comp_time, time_unit] = get_elapsed_time<
			t_real, t_timepoint>(start_time);
		std::cout << "Compilation time: "
			<< comp_time << " " << time_unit << "."
			<< std::endl;
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return -1;
	}

	return 0;
}
