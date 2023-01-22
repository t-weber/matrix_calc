/**
 * constants table
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 22-jan-2022
 * @license: see 'LICENSE.GPL' file
 */

#ifndef __CONSTTAB_H__
#define __CONSTTAB_H__

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <iostream>
#include <sstream>

#include "vm_0ac/types.h"


/**
 * constants table
 */
class ConstTab
{
public:
	// possible constant types
	using t_constval = std::variant<std::monostate, t_real, t_int, t_str>;


public:
	ConstTab() = default;
	~ConstTab() = default;

	// write a constant to the stream and get its position
	std::streampos AddConst(const t_constval& constval);

	// get the stream's bytes
	std::pair<std::streampos, std::shared_ptr<std::uint8_t[]>> GetBytes();


private:
	std::unordered_map<t_constval, std::streampos> m_consts{};
	std::stringstream m_ostr{};
};


#endif
