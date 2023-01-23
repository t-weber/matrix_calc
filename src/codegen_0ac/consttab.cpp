/**
 * constants table
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 22-jan-2023
 * @license: see 'LICENSE.GPL' file
 */

#include "consttab.h"


/**
 * write a constant to the stream and get its position
 */
std::streampos ConstTab::AddConst(const t_constval& constval)
{
	// look if the value is already in the map
	if(auto iter = m_consts.find(constval); iter != m_consts.end())
		return iter->second;

	std::streampos streampos = m_ostr.tellp();

	// write constant to stream
	if(std::holds_alternative<t_real>(constval))
	{
		const t_real realval = std::get<t_real>(constval);

		// write real type descriptor byte
		m_ostr.put(static_cast<t_vm_byte>(VMType::REAL));
		// write real data
		m_ostr.write(reinterpret_cast<const char*>(&realval),
			vm_type_size<VMType::REAL, false>);
	}
	else if(std::holds_alternative<t_int>(constval))
	{
		const t_int intval = std::get<t_int>(constval);

		// write int type descriptor byte
		m_ostr.put(static_cast<t_vm_byte>(VMType::INT));
		// write int data
		m_ostr.write(reinterpret_cast<const char*>(&intval),
			vm_type_size<VMType::INT, false>);
	}
	else if(std::holds_alternative<t_str>(constval))
	{
		const t_str& strval = std::get<t_str>(constval);

		// write string type descriptor byte
		m_ostr.put(static_cast<t_vm_byte>(VMType::STR));
		// write string length
		t_vm_addr len = static_cast<t_vm_addr>(strval.length());
		m_ostr.write(reinterpret_cast<const char*>(&len),
			vm_type_size<VMType::ADDR_MEM, false>);
		// write string data
		m_ostr.write(strval.data(), len);
	}
	else
	{
		throw std::runtime_error("Unknown constant type.");
	}

	// otherwise add a new constant to the map
	m_consts.insert(std::make_pair(constval, streampos));
	return streampos;
}



/**
 * get the stream's bytes
 */
std::pair<std::streampos, std::shared_ptr<std::uint8_t[]>> ConstTab::GetBytes()
{
	std::streampos size = m_ostr.tellp();
	if(!size)
		return std::make_pair(0, nullptr);

	auto buffer = std::shared_ptr<std::uint8_t[]>(new std::uint8_t[size]);
	m_ostr.seekg(0, std::ios_base::beg);

	m_ostr.read(reinterpret_cast<char*>(buffer.get()), size);
	return std::make_pair(size, buffer);
}
