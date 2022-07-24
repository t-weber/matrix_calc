/**
 * runs the vm
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 19-jun-2022
 * @license see 'LICENSE.GPL' file
 */

#include "vm.h"
#include "../version.h"

#include <vector>
#include <iostream>
#include <fstream>

#if __has_include(<filesystem>)
	#include <filesystem>
	namespace fs = std::filesystem;
#elif __has_include(<boost/filesystem.hpp>)
	#include <boost/filesystem.hpp>
	namespace fs = boost::filesystem;
#endif

#include <boost/program_options.hpp>
namespace args = boost::program_options;



static bool run_vm(const fs::path& prog, t_vm_addr mem_size,
	bool enable_debug, bool enable_checks)
{
	using namespace m_ops;

	std::size_t filesize = fs::file_size(prog);
	std::ifstream ifstr(prog, std::ios_base::binary);
	if(!ifstr)
		return false;

	std::vector<VM::t_byte> bytes;
	bytes.resize(filesize);

	ifstr.read(reinterpret_cast<char*>(bytes.data()), filesize);
	if(ifstr.fail())
		return false;

	VM vm(mem_size);
	VM::t_addr sp_initial = vm.GetSP();

	vm.SetDebug(enable_debug);
	vm.SetChecks(enable_checks);
	vm.SetMem(0, bytes.data(), filesize, true);
	vm.Run();

	// print remaining stack
	std::size_t stack_idx = 0;
	while(vm.GetSP() < sp_initial)
	{
		VM::t_data dat = vm.PopData();
		const char* type_name = VM::GetDataTypeName(dat);

		std::cout << "Stack[" << stack_idx << "] = ";
		std::visit([type_name](auto&& val) -> void
		{
			using t_val = std::decay_t<decltype(val)>;
			// variant not empty?
			if constexpr(!std::is_same_v<t_val, std::monostate>)
				std::cout << val << " [" << type_name << "]";
		}, dat);
		std::cout << std::endl;

		++stack_idx;
	}

	return true;
}



int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
	try
	{
		std::ios_base::sync_with_stdio(false);

                // --------------------------------------------------------------------
                // get program arguments
                // --------------------------------------------------------------------
		std::vector<std::string> progs;
		bool enable_debug = false;
		bool enable_checks = true;
		t_vm_addr mem_size = 4096;

		args::options_description arg_descr("Virtual machine arguments");
		arg_descr.add_options()
			("debug,d", args::bool_switch(&enable_debug), "enable debug output")
			("checks,c", args::value<decltype(enable_checks)>(&enable_checks), "enable memory checks")
			("mem,m", args::value<decltype(mem_size)>(&mem_size), "set memory size")
			("prog", args::value<decltype(progs)>(&progs), "input program to run");

		args::positional_options_description posarg_descr;
		posarg_descr.add("prog", -1);

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
			std::cout << "0ac virtual machine version " << MCALC_VER
				<< " by Tobias Weber <tobias.weber@tum.de>, 2022."
				<< std::endl;
			std::cout << "Internal data type lengths:"
				<< " real: " << sizeof(t_vm_real)*8 << " bits,"
				<< " int: " << sizeof(t_vm_int)*8 << " bits."
				<< " address: " << sizeof(t_vm_addr)*8 << " bits."
				<< std::endl;

			std::cerr << "Please specify an input program.\n" << std::endl;
			std::cout << arg_descr << std::endl;
			return 0;
		}
                // --------------------------------------------------------------------

                // input file
		fs::path inprog = progs[0];

		if(!run_vm(inprog, mem_size, enable_debug, enable_checks))
		{
			std::cerr << "Could not run \"" << inprog.string() << "\"." << std::endl;
			return -1;
		}		
	}
	catch(const std::exception& err)
	{
		std::cerr << "Error: " << err.what() << std::endl;
		return -1;
	}

	return 0;
}
