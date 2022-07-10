/**
 * 3-ac/llvm parser entry point
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 20-dec-19
 * @license: see 'LICENSE.GPL' file
 */

#include "ast.h"
#include "parser.h"
#include "llasm.h"
#include "printast.h"
#include "semantics.h"

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


int main(int argc, char** argv)
{
	try
	{
		std::ios_base::sync_with_stdio(0);
		std::locale loc{};
		std::locale::global(loc);
		//std::cout << "Locale: " << loc.name() << "." << std::endl;

		std::cout << "Matrix expression compiler version 0.2"
			<< " by Tobias Weber <tobias.weber@tum.de>, 2020." << std::endl;

		// llvm toolchain
		std::string tool_opt = "opt";
		std::string tool_bc = "llvm-as";
		std::string tool_bclink = "llvm-link";
		std::string tool_interp = "lli";
		std::string tool_s = "llc";
		std::string tool_o = "clang";
		std::string tool_exec = "clang";
		std::string tool_strip = "llvm-strip";


		// --------------------------------------------------------------------
		// get program arguments
		// --------------------------------------------------------------------
		std::vector<std::string> vecProgs;
		bool interpret = false;
		bool optimise = false;
		bool show_symbols = false;
		bool show_ast = false;
		bool verbose = false;
		std::string outprog;

		args::options_description arg_descr("Compiler arguments");
		arg_descr.add_options()
			("out,o", args::value(&outprog), "compiled program output")
			("optimise,O", args::bool_switch(&optimise), "optimise program")
			("interpret,i", args::bool_switch(&interpret), "directly run program in interpreter")
			("symbols,s", args::bool_switch(&show_symbols), "output symbol table")
			("ast,a", args::bool_switch(&show_ast), "output syntax tree")
			("verbose,v", args::bool_switch(&verbose), "verbose output")
			("program", args::value<decltype(vecProgs)>(&vecProgs), "input program to compile");

		args::positional_options_description posarg_descr;
		posarg_descr.add("program", -1);

		args::options_description arg_descr_toolchain("Toolchain programs");
		arg_descr_toolchain.add_options()
			("tool_opt", args::value(&tool_opt), "llvm optimiser")
			("tool_bc", args::value(&tool_bc), "llvm bitcode assembler")
			("tool_bclink", args::value(&tool_bclink), "llvm bitcode linker")
			("tool_interp", args::value(&tool_interp), "llvm bitcode interpreter")
			("tool_bccomp", args::value(&tool_s), "llvm bitcode compiler")
			("tool_asm", args::value(&tool_o), "native assembler")
			("tool_link", args::value(&tool_exec), "native linker")
			("tool_strip", args::value(&tool_strip), "strip tool");
		arg_descr.add(arg_descr_toolchain);

		auto argparser = args::command_line_parser{argc, argv};
		argparser.style(args::command_line_style::default_style);
		argparser.options(arg_descr);
		argparser.positional(posarg_descr);

		args::variables_map mapArgs;
		auto parsedArgs = argparser.run();
		args::store(parsedArgs, mapArgs);
		args::notify(mapArgs);

		if(vecProgs.size() == 0)
		{
			std::cerr << "Please specify an input program.\n" << std::endl;
			std::cout << arg_descr << std::endl;
			return 0;
		}

		// input file
		const std::string& inprog = vecProgs[0];

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

		std::string outprog_3ac = outprog + ".asm";
		std::string outprog_3ac_opt = outprog + "_opt.asm";
		std::string outprog_bc = outprog + ".bc";
		std::string outprog_linkedbc = outprog + "_linked.bc";
		std::string outprog_s = outprog + ".s";
		std::string outprog_o = outprog + ".o";

		std::string runtime_3ac = optimise ? "runtime_opt.asm" : "runtime.asm";
		std::string runtime_bc = "runtime.bc";

		std::string opt_verbose = verbose ? " -v " : "";
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

		yy::ParserContext ctx{ifstr};

		// register external runtime functions which should be available to the compiler
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "pow", SymbolType::SCALAR, {SymbolType::SCALAR, SymbolType::SCALAR});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "exp", SymbolType::SCALAR, {SymbolType::SCALAR});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "sin", SymbolType::SCALAR, {SymbolType::SCALAR});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "cos", SymbolType::SCALAR, {SymbolType::SCALAR});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "sqrt", SymbolType::SCALAR, {SymbolType::SCALAR});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "fabs", SymbolType::SCALAR, {SymbolType::SCALAR});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "labs", SymbolType::INT, {SymbolType::INT});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "strlen", SymbolType::INT, {SymbolType::STRING});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "set_eps", SymbolType::VOID, {SymbolType::SCALAR});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "get_eps", SymbolType::SCALAR, {});
		ctx.GetSymbols().AddExtFunc(ctx.GetScopeName(), "set_debug", SymbolType::VOID, {SymbolType::INT});

		// register internal runtime functions which should be available to the compiler
		ctx.GetSymbols().AddFunc(ctx.GetScopeName(), "putstr", SymbolType::VOID, {SymbolType::STRING});
		ctx.GetSymbols().AddFunc(ctx.GetScopeName(), "putflt", SymbolType::VOID, {SymbolType::SCALAR});
		ctx.GetSymbols().AddFunc(ctx.GetScopeName(), "putint", SymbolType::VOID, {SymbolType::INT});
		ctx.GetSymbols().AddFunc(ctx.GetScopeName(), "getflt", SymbolType::SCALAR, {SymbolType::STRING});
		ctx.GetSymbols().AddFunc(ctx.GetScopeName(), "getint", SymbolType::INT, {SymbolType::STRING});
		ctx.GetSymbols().AddFunc(ctx.GetScopeName(), "flt_to_str", SymbolType::VOID, {SymbolType::SCALAR, SymbolType::STRING, SymbolType::INT});
		ctx.GetSymbols().AddFunc(ctx.GetScopeName(), "int_to_str", SymbolType::VOID, {SymbolType::INT, SymbolType::STRING, SymbolType::INT});


		yy::Parser parser(ctx);
		int res = parser.parse();
		if(res != 0)
		{
			std::cerr << "Parser reports failure." << std::endl;
			return res;
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
		// TODO: semantic analysis
		// --------------------------------------------------------------------
		//std::cout << "Analysing semantics..." << std::endl;
		//Semantics sema;
		//
		//for(const auto& stmt : ctx.GetStatements()->GetStatementList())
		//	stmt->accept(&sema);
		// --------------------------------------------------------------------



		// --------------------------------------------------------------------
		// 3AC generation
		// --------------------------------------------------------------------
		std::cout << "Generating intermediate code: \""
			<< inprog << "\" -> \"" << outprog_3ac << "\"..." << std::endl;

		std::ofstream ofstr{outprog_3ac};
		std::ostream* ostr = &ofstr /*&std::cout*/;
		LLAsm llasm{&ctx.GetSymbols(), ostr};
		auto stmts = ctx.GetStatements()->GetStatementList();
		for(auto iter=stmts.rbegin(); iter!=stmts.rend(); ++iter)
		{
			(*iter)->accept(&llasm);
			(*ostr) << std::endl;
		}

		(*ostr) << "; -----------------------------------------------------------------------------\n";
		(*ostr) << "; external functions which are available to the compiler\n";
		(*ostr) << LLAsm::get_function_declarations(ctx.GetSymbols()) << std::endl;
		(*ostr) << "; -----------------------------------------------------------------------------\n";

		// additional runtime/startup code
		(*ostr) << "\n" << R"START(
; -----------------------------------------------------------------------------
; external functions which are not exposed to the compiler
declare i8* @strncpy(i8*, i8*, i64)
declare i8* @strncat(i8*, i8*, i64)
declare i32 @strncmp(i8*, i8*, i64)
declare i32 @puts(i8*)
declare i32 @snprintf(i8*, i64, i8*, ...)
declare i32 @printf(i8*, ...)
declare i32 @scanf(i8*, ...)
declare i8* @memcpy(i8*, i8*, i64)
declare i8* @ext_heap_alloc(i64, i64)
declare void @ext_heap_free(i8*)
declare void @ext_init()
declare void @ext_deinit()
; -----------------------------------------------------------------------------


; -----------------------------------------------------------------------------
; external functions from runtime.c which are not exposed to the compiler
declare double @ext_determinant(double*, i64)
declare i64 @ext_power(double*, double*, i64, i64)
declare i64 @ext_transpose(double*, double*, i64, i64)
; -----------------------------------------------------------------------------


; -----------------------------------------------------------------------------
; constants
@__strfmt_s = constant [3 x i8] c"%s\00"
@__strfmt_lg = constant [4 x i8] c"%lg\00"
@__strfmt_ld = constant [4 x i8] c"%ld\00"
@__str_vecbegin = constant [3 x i8] c"[ \00"
@__str_vecend = constant [3 x i8] c" ]\00"
@__str_vecsep = constant [3 x i8] c", \00"
@__str_matsep = constant [3 x i8] c"; \00"
; -----------------------------------------------------------------------------


; -----------------------------------------------------------------------------
; internal runtime functions

; returns 0 if flt <= eps
define double @zero_eps(double %flt)
{
	%eps = call double @get_eps()
	%fltabs = call double (double) @fabs(double %flt)

	%cond = fcmp ole double %fltabs, %eps
	br i1 %cond, label %labelIf, label %labelEnd
labelIf:
	ret double 0.
labelEnd:
	ret double %flt
}

; double -> string
define void @flt_to_str(double %flt, i8* %strptr, i64 %len)
{
	%fmtptr = bitcast [4 x i8]* @__strfmt_lg to i8*
	%theflt = call double (double) @zero_eps(double %flt)
	call i32 (i8*, i64, i8*, ...) @snprintf(i8* %strptr, i64 %len, i8* %fmtptr, double %theflt)
	ret void
}

; int -> string
define void @int_to_str(i64 %i, i8* %strptr, i64 %len)
{
	%fmtptr = bitcast [4 x i8]* @__strfmt_ld to i8*
	call i32 (i8*, i64, i8*, ...) @snprintf(i8* %strptr, i64 %len, i8* %fmtptr, i64 %i)
	ret void
}

; output a string
define void @putstr(i8* %val)
{
	call i32 (i8*) @puts(i8* %val)
	ret void
}

; output a float
define void @putflt(double %val)
{
	; convert to string
	%strval = alloca [64 x i8]
	%strvalptr = bitcast [64 x i8]* %strval to i8*
	call void @flt_to_str(double %val, i8* %strvalptr, i64 64)

	; output string
	call void (i8*) @putstr(i8* %strvalptr)
	ret void
}

; output an int
define void @putint(i64 %val)
{
	; convert to string
	%strval = alloca [64 x i8]
	%strvalptr = bitcast [64 x i8]* %strval to i8*
	call void @int_to_str(i64 %val, i8* %strvalptr, i64 64)

	; output string
	call void (i8*) @putstr(i8* %strvalptr)
	ret void
}

; input a float
define double @getflt(i8* %str)
{
	; output given string
	%fmtptr_s = bitcast [3 x i8]* @__strfmt_s to i8*
	call i32 (i8*, ...) @printf(i8* %fmtptr_s, i8* %str)

	; alloc double
	%d_ptr = alloca double

	; read double from stdin
	%fmtptr_g = bitcast [4 x i8]* @__strfmt_lg to i8*
	call i32 (i8*, ...) @scanf(i8* %fmtptr_g, double* %d_ptr)

	%d = load double, double* %d_ptr
	ret double %d
}

; input an int
define i64 @getint(i8* %str)
{
	; output given string
	%fmtptr_s = bitcast [3 x i8]* @__strfmt_s to i8*
	call i32 (i8*, ...) @printf(i8* %fmtptr_s, i8* %str)

	; alloc int
	%i_ptr = alloca i64

	; read int from stdin
	%fmtptr_ld = bitcast [4 x i8]* @__strfmt_ld to i8*
	call i32 (i8*, ...) @scanf(i8* %fmtptr_ld, i64* %i_ptr)

	%i = load i64, i64* %i_ptr
	ret i64 %i
}

; -----------------------------------------------------------------------------


; -----------------------------------------------------------------------------
; main entry point for llvm
define i32 @main()
{
	call void @ext_init()

	; call entry function
	call void @start()

	call void @ext_deinit()

	ret i32 0
}
; -----------------------------------------------------------------------------
)START";

		(*ostr) << std::endl;
		// --------------------------------------------------------------------



		// --------------------------------------------------------------------
		// 3AC optimisation
		// --------------------------------------------------------------------
		if(optimise)
		{
			std::cout << "Optimising intermediate code: \""
				<< outprog_3ac << "\" -> \"" << outprog_3ac_opt << "\"..." << std::endl;

			std::string cmd_opt = tool_opt + " -stats -S --strip-debug -o "
				+ outprog_3ac_opt + " " + outprog_3ac;
			if(std::system(cmd_opt.c_str()) != 0)
			{
				std::cerr << "Failed." << std::endl;
				return -1;
			}

			outprog_3ac = outprog_3ac_opt;
		}
		// --------------------------------------------------------------------



		// --------------------------------------------------------------------
		// Bitcode generation
		// --------------------------------------------------------------------
		std::cout << "Assembling bitcode: \""
			<< outprog_3ac << "\" -> \"" << outprog_bc << "\"..." << std::endl;

		std::string cmd_bc = tool_bc + " -o " + outprog_bc + " " + outprog_3ac;
		if(std::system(cmd_bc.c_str()) != 0)
		{
			std::cerr << "Failed." << std::endl;
			return -1;
		}


		std::cout << "Assembling runtime bitcode: \""
			<< runtime_3ac << "\" -> \"" << runtime_bc << "\"..." << std::endl;

		cmd_bc = tool_bc + " -o " + runtime_bc + " " + runtime_3ac;
		if(std::system(cmd_bc.c_str()) != 0)
		{
			std::cerr << "Failed." << std::endl;
			return -1;
		}
		// --------------------------------------------------------------------



		// --------------------------------------------------------------------
		// Bitcode linking
		// --------------------------------------------------------------------
		std::cout << "Linking bitcode to runtime: \""
			<< outprog_bc << "\" + \"" << runtime_bc  << "\" -> \""
			<< outprog_linkedbc << "\"..." << std::endl;

		std::string cmd_bclink = tool_bclink + opt_verbose + " -o " +
			outprog_linkedbc + " " + outprog_bc + " " + runtime_bc;
		if(std::system(cmd_bclink.c_str()) != 0)
		{
			std::cerr << "Failed." << std::endl;
			return -1;
		}
		// --------------------------------------------------------------------


		// interpret bitcode
		if(interpret)
		{
			std::cout << "Interpreting bitcode \"" << outprog_linkedbc << "\"..." << std::endl;

			std::string cmd_interp = tool_interp + " " + outprog_linkedbc;
			if(std::system(cmd_interp.c_str()) != 0)
			{
				std::cerr << "Failed." << std::endl;
				return -1;
			}
		}

		// compile bitcode
		else
		{
			std::cout << "Generating native assembly \""
				<< outprog_linkedbc << "\" -> \"" << outprog_s << "\"..." << std::endl;

			std::string opt_flag_s = optimise ? "-O2" : "";
			std::string cmd_s = tool_s + " " +
				opt_flag_s + " -o " + outprog_s + " " + outprog_linkedbc;
			if(std::system(cmd_s.c_str()) != 0)
			{
				std::cerr << "Failed." << std::endl;
				return -1;
			}


			std::cout << "Assembling native code \""
				<< outprog_s << "\" -> \"" << outprog_o << "\"..." << std::endl;

			std::string opt_flag_o = optimise ? "-O2" : "";
			std::string cmd_o = tool_o + opt_verbose + " " +
				opt_flag_o + " -c -o " + outprog_o + " " + outprog_s;
			if(std::system(cmd_o.c_str()) != 0)
			{
				std::cerr << "Failed." << std::endl;
				return -1;
			}


			std::cout << "Generating native executable \""
				<< outprog_o << "\" -> \"" << outprog << "\"..." << std::endl;

			std::string opt_flag_exec = optimise ? "-O2" : "";
			std::string cmd_exec = tool_exec + opt_verbose + " " +
				opt_flag_exec + " -o " + outprog + " " + outprog_o + " -lm -lc";
			if(std::system(cmd_exec.c_str()) != 0)
			{
				std::cerr << "Failed." << std::endl;
				return -1;
			}


			if(optimise)
			{
				std::cout << "Stripping debug symbols from \""
					<< outprog << "\"..." << std::endl;

				std::string cmd_strip = tool_strip + " " + outprog;
				if(std::system(cmd_strip.c_str()) != 0)
				{
					std::cerr << "Failed." << std::endl;
					return -1;
				}
			}
		}
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return -1;
	}

	return 0;
}
