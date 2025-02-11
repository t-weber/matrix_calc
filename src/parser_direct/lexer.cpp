/**
 * lexer
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 7-mar-2020, 26-nov-2022
 * @license see 'LICENSE' file
 */

#include "lexer.h"
#include "ast/ast.h"

#include <sstream>
#include <memory>
#include <regex>
#include <bitset>
#include <type_traits>
#include <boost/algorithm/string.hpp>

using namespace lalr1;


template<template<std::size_t, class...> class t_func, class t_params, std::size_t ...seq>
constexpr void constexpr_loop(const std::index_sequence<seq...>&, const t_params& params)
{
	( (std::apply(t_func<seq>{}, params)), ... );
}


Lexer::Lexer(std::istream* istr) : m_istr{istr}
{
}


/**
 * find all matching tokens for input string
 */
std::vector<t_lexer_match>
Lexer::GetMatchingTokens(const std::string& str, std::size_t line)
{
	std::vector<t_lexer_match> matches;

	{	// int
		static const std::regex regex_int{"(0[xb])?([0-9]+)"};
		std::smatch smatch;
		if(std::regex_match(str, smatch, regex_int))
		{
			t_int val{};

			if(smatch.str(1) == "0x")
			{
				// hexadecimal integers
				std::istringstream{smatch.str(0)} >> std::hex >> val;
			}
			else if(smatch.str(1) == "0b")
			{
				// binary integers
				using t_bits = std::bitset<sizeof(t_int)*8>;
				t_bits bits(smatch.str(2));

				using t_ulong = std::invoke_result_t<decltype(&t_bits::to_ulong), t_bits*>;
				if constexpr(sizeof(t_ulong) >= sizeof(t_int))
					val = static_cast<t_int>(bits.to_ulong());
				else
					val = static_cast<t_int>(bits.to_ullong());
			}
			else
			{
				// decimal integers
				std::istringstream{smatch.str(2)} >> std::dec >> val;
			}

			matches.emplace_back(std::make_tuple(
				static_cast<t_symbol_id>(Token::INT), val, line));
		}
		else if(str == "0x" || str == "0b")
		{
			// dummy matches to continue searching for longest int
			matches.emplace_back(std::make_tuple(
				static_cast<t_symbol_id>(Token::INT), 0, line));
		}
	}

	{	// real
		//static const std::regex regex_real{"[0-9]+(\\.[0-9]*)?"};
		static const std::regex regex_real{"[0-9]+(\\.[0-9]*)?([Ee][+-]?[0-9]*)?"};
		std::smatch smatch;
		if(std::regex_match(str, smatch, regex_real))
		{
			t_real val{};
			std::istringstream{str} >> val;
			matches.emplace_back(std::make_tuple(
				static_cast<t_symbol_id>(Token::REAL), val, line));
		}
	}

	{	// keywords and identifiers
		if(str == "if")
		{
			matches.emplace_back(std::make_tuple(
				static_cast<t_symbol_id>(Token::IF), str, line));
		}
		else if(str == "then")
		{
			matches.emplace_back(std::make_tuple(
				static_cast<t_symbol_id>(Token::THEN), str, line));
		}
		else if(str == "else")
		{
			matches.emplace_back(std::make_tuple(
				static_cast<t_symbol_id>(Token::ELSE), str, line));
		}
		else if(str == "loop")
		{
			matches.emplace_back(std::make_tuple(
				static_cast<t_symbol_id>(Token::LOOP), str, line));
		}
		else if(str == "break")
		{
			matches.emplace_back(std::make_tuple(
				static_cast<t_symbol_id>(Token::BREAK), str, line));
		}
		else if(str == "next")
		{
			matches.emplace_back(std::make_tuple(
				static_cast<t_symbol_id>(Token::NEXT), str, line));
		}
		else if(str == "do")
		{
			matches.emplace_back(std::make_tuple(
				static_cast<t_symbol_id>(Token::DO), str, line));
		}
		else if(str == "func")
		{
			matches.emplace_back(std::make_tuple(
				static_cast<t_symbol_id>(Token::FUNC), str, line));
		}
		else if(str == "ret")
		{
			matches.emplace_back(std::make_tuple(
				static_cast<t_symbol_id>(Token::RET), str, line));
		}
		else if(str == "assign")
		{
			matches.emplace_back(std::make_tuple(
				static_cast<t_symbol_id>(Token::ASSIGN), str, line));
		}
		else if(str == "scalar" || str == "var")
		{
			matches.emplace_back(std::make_tuple(
				static_cast<t_symbol_id>(Token::SCALARDECL), str, line));
		}
		else if(str == "vec")
		{
			matches.emplace_back(std::make_tuple(
				static_cast<t_symbol_id>(Token::VECTORDECL), str, line));
		}
		else if(str == "mat")
		{
			matches.emplace_back(std::make_tuple(
				static_cast<t_symbol_id>(Token::MATRIXDECL), str, line));
		}
		else if(str == "str")
		{
			matches.emplace_back(std::make_tuple(
				static_cast<t_symbol_id>(Token::STRINGDECL), str, line));
		}
		else if(str == "int")
		{
			matches.emplace_back(std::make_tuple(
				static_cast<t_symbol_id>(Token::INTDECL), str, line));
		}
		else
		{
			// identifier
			static const std::regex regex_ident{"[_A-Za-z]+[_A-Za-z0-9]*"};
			std::smatch smatch;
			if(std::regex_match(str, smatch, regex_ident))
				matches.emplace_back(std::make_tuple(
					static_cast<t_symbol_id>(Token::IDENT), str, line));
		}
	}

	{
		// operators
		if(str == "==")
		{
			matches.emplace_back(std::make_tuple(
				static_cast<t_symbol_id>(Token::EQU), str, line));
		}
		else if(str == "!=" || str == "<>")
		{
			matches.emplace_back(std::make_tuple(
				static_cast<t_symbol_id>(Token::NEQ), str, line));
		}
		else if(str == "||" || str == "or")
		{
			matches.emplace_back(std::make_tuple(
				static_cast<t_symbol_id>(Token::OR), str, line));
		}
		else if(str == "&&" || str == "and")
		{
			matches.emplace_back(std::make_tuple(
				static_cast<t_symbol_id>(Token::AND), str, line));
		}
		else if(str == "xor")
		{
			matches.emplace_back(std::make_tuple(
				static_cast<t_symbol_id>(Token::XOR), str, line));
		}
		else if(str == ">=")
		{
			matches.emplace_back(std::make_tuple(
				static_cast<t_symbol_id>(Token::GEQ), str, line));
		}
		else if(str == "<=")
		{
			matches.emplace_back(std::make_tuple(
				static_cast<t_symbol_id>(Token::LEQ), str, line));
		}
		else if(str == "!" || str == "not")
		{
			matches.emplace_back(std::make_tuple(
				static_cast<t_symbol_id>(Token::NOT), str, line));
		}
		else if(str == "~")
		{
			matches.emplace_back(std::make_tuple(
				static_cast<t_symbol_id>(Token::RANGE), str, line));
		}

		// tokens represented by themselves
		else if(str == "+" || str == "-" || str == "*" || str == "/" ||
			str == "%" || str == "^" || str == "(" || str == ")" ||
			str == "{" || str == "}" || str == "[" || str == "]" ||
			str == "," || str == ";" || str == "=" || str == "'" ||
			str == ">" || str == "<" ||
			str == "!" || str == "|" || str == "&")
			matches.emplace_back(std::make_tuple(
				static_cast<t_symbol_id>(str[0]), std::nullopt, line));
	}

	//std::cerr << "Input \"" << str << "\" has " << matches.size() << " matches." << std::endl;
	return matches;
}


/**
 * replace escape sequences
 */
static void replace_escapes(std::string& str)
{
	boost::replace_all(str, "\\n", "\n");
	boost::replace_all(str, "\\t", "\t");
	boost::replace_all(str, "\\r", "\r");
}


/**
 * get next token and attribute
 */
t_lexer_match Lexer::GetNextToken(std::size_t* _line)
{
	std::string input;
	std::vector<t_lexer_match> longest_lexer_matching;
	bool eof = false;
	bool in_line_comment = false;
	bool in_string = false;

	std::size_t dummy_line = 1;
	std::size_t *line = _line;
	if(!line) line = &dummy_line;

	// find longest matching token
	while(!(eof = m_istr->eof()))
	{
		int c = m_istr->get();
		if(c == std::char_traits<char>::eof())
		{
			eof = true;
			break;
		}
		//std::cout << "Input: " << c << " (0x" << std::hex << int(c) << ")." << std::endl;

		if(in_line_comment && c != '\n')
			continue;

		// if outside any other match...
		if(longest_lexer_matching.size() == 0)
		{
			if(c == '\"' && !in_line_comment)
			{
				if(!in_string)
				{
					in_string = true;
					continue;
				}
				else
				{
					replace_escapes(input);
					in_string = false;
					return std::make_tuple(
						static_cast<t_symbol_id>(Token::STR), input, *line);
				}
			}

			// ... ignore comments
			if(c == '#' && !in_string)
			{
				in_line_comment = true;
				continue;
			}

			// ... ignore white spaces
			if((c==' ' || c=='\t') && !in_string)
				continue;

			// ... new line
			else if(c=='\n')
			{
				in_line_comment = false;
				++(*line);
				continue;
			}
		}

		input += c;
		if(in_string)
			continue;

		auto matching = GetMatchingTokens(input, *line);
		if(matching.size())
		{
			longest_lexer_matching = matching;

			if(m_istr->peek() == std::char_traits<char>::eof())
			{
				eof = true;
				break;
			}
		}
		else
		{
			// no more matches
			m_istr->putback(c);
			break;
		}
	}

	if(longest_lexer_matching.size() == 0 && eof)
		return std::make_tuple((t_symbol_id)Token::END, std::nullopt, *line);

	if(longest_lexer_matching.size() == 0)
	{
		std::ostringstream ostrErr;
		ostrErr << "Line " << *line << ": Invalid input in lexer: \""
			<< input << "\"" << " (length: " << input.length() << ").";
		throw std::runtime_error(ostrErr.str());
	}

	return longest_lexer_matching[0];
}


template<std::size_t IDX> struct _Lval_LoopFunc
{
	void operator()(
		std::vector<t_toknode>* vec, t_symbol_id id, t_index tableidx,
		const t_lval& lval, std::size_t line) const
	{
		using t_val = std::variant_alternative_t<IDX, typename t_lval::value_type>;

		if(std::holds_alternative<t_val>(*lval))
		{
			t_toknode astnode = nullptr;

			if constexpr(std::is_same_v<t_val, std::string>)
				astnode = std::make_shared<ASTStrConst>(std::get<IDX>(*lval));
			else
				astnode = std::make_shared<ASTNumConst<t_val>>(std::get<IDX>(*lval));

			astnode->SetId(id);
			astnode->SetTableIndex(tableidx);
			astnode->SetLineRange(std::make_pair(line, line));
			astnode->SetTerminalOverride(true);
			vec->emplace_back(std::move(astnode));
		}
	};
};


/**
 * get all tokens and attributes
 */
std::vector<t_toknode> Lexer::GetAllTokens()
{
	std::vector<t_toknode> vec;
	std::size_t line = 1;

	while(true)
	{
		auto tup = GetNextToken(&line);
		t_symbol_id id = std::get<0>(tup);
		const t_lval& lval = std::get<1>(tup);
		std::size_t line = std::get<2>(tup);

		// get index into parse tables
		t_index tableidx = 0;
		if(m_mapTermIdx)
		{
			auto iter = m_mapTermIdx->find(id);
			if(iter != m_mapTermIdx->end())
				tableidx = iter->second;
		}

		// does this token have an attribute?
		if(lval)
		{
			// find the correct type in the variant
			auto seq = std::make_index_sequence<
				std::variant_size_v<typename t_lval::value_type>>();

			constexpr_loop<_Lval_LoopFunc>(
				seq, std::make_tuple(&vec, id, tableidx, lval, line));
		}
		else
		{
			t_toknode astnode = std::make_shared<ASTNumConst<t_int>>(-1);
			astnode->SetId(id);
			astnode->SetTableIndex(tableidx);
			astnode->SetLineRange(std::make_pair(line, line));
			astnode->SetTerminalOverride(true);
			vec.emplace_back(std::move(astnode));
		}

		if(id == (t_symbol_id)Token::END)
			break;
	}

	return vec;
}



// test
/*#include <fstream>

// g++ -DUSE_DIRECT_PARSER -I.. -std=c++20 -o 0 lexer.cpp
int main(int argc, char **argv)
{
	if(argc <= 1)
		return -1;

	std::ifstream istr(argv[1]);
	Lexer lexer(&istr);

	for(const t_toknode& tok : lexer.GetAllTokens())
	{
		auto linerange = tok->GetLineRange();
		std::cout << "id = " << tok->GetId()
			<< ", idx = " << tok->GetTableIndex();
		if(std::isprint(tok->GetId()))
			std::cout << ", ch = \"" << char(tok->GetId()) << "\"";
		if(linerange)
			std::cout << ", lines = " << linerange->first << ".." << linerange->second;
		std::cout << std::endl;
	}

	return 0;
}*/
