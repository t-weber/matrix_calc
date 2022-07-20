/**
 * asm generator and vm opcodes
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 14-jun-2022
 * @license see 'LICENSE.GPL' file
 */

#ifndef __0ACVM_TYPES_H__
#define __0ACVM_TYPES_H__

#include <vector>

#include "../types.h"

#include "math_algos.h"
#include "math_conts.h"


using t_vm_int = ::t_int;
using t_vm_real = ::t_real;
using t_vm_addr = std::int32_t;
using t_vm_byte = std::uint8_t;
using t_vm_bool = t_vm_byte;

using t_vm_str = std::string;
using t_vm_vec = m::vec<::t_vm_real, std::vector>;
using t_vm_mat = m::mat<::t_vm_real, std::vector>;


enum class VMType : t_vm_byte
{
	UNKNOWN     = 0x00,

	REAL        = 0x01,
	INT         = 0x02,
	BOOLEAN     = 0x03,

	STR         = 0x10,
	VEC         = 0x11,
	MAT         = 0x12,

	ADDR_MEM    = 0x20,   // address refering to absolute memory locations
	ADDR_IP     = 0x21,   // address relative to the instruction pointer
	ADDR_SP     = 0x22,   // address relative to the stack pointer
	ADDR_BP     = 0x23,   // address relative to a local base pointer
};



/**
 * get a string representation of a base register name
 */
template<class t_str = const char*>
constexpr t_str get_vm_base_reg(VMType ty)
{
	switch(ty)
	{
		case VMType::UNKNOWN:     return "unknown";

		case VMType::ADDR_MEM:    return "absolute";
		case VMType::ADDR_IP:     return "ip";
		case VMType::ADDR_SP:     return "sp";
		case VMType::ADDR_BP:     return "bp";

		default:                  return "<unknown>";
	}
}



/**
 * get a string representation of a type name
 * (run-time version)
 */
template<class t_str = const char*>
constexpr t_str get_vm_type_name(VMType ty)
{
	switch(ty)
	{
		case VMType::UNKNOWN:     return "unknown";

		case VMType::REAL:        return "real";
		case VMType::INT:         return "integer";
		case VMType::BOOLEAN:     return "boolean";

		case VMType::STR:         return "string";
		case VMType::VEC:         return "vector";
		case VMType::MAT:         return "matrix";

		case VMType::ADDR_MEM:    return "absolute address";
		case VMType::ADDR_IP:     return "address relative to ip";
		case VMType::ADDR_SP:     return "address relative to sp";
		case VMType::ADDR_BP:     return "address relative to bp";

		default:                  return "<unknown>";
	}
}



// maximum size to reserve for static variables
constexpr const t_vm_addr g_vm_longest_size = 64;


/**
 * get (static) type sizes (including data type and, optionally, descriptor byte)
 */
template<VMType ty, bool with_descr = false> constexpr t_vm_addr vm_type_size
	= g_vm_longest_size + (with_descr ? sizeof(t_vm_byte) : 0);
template<bool with_descr> constexpr inline t_vm_addr vm_type_size<VMType::UNKNOWN, with_descr>
	= g_vm_longest_size + (with_descr ? sizeof(t_vm_byte) : 0);
template<bool with_descr> constexpr inline t_vm_addr vm_type_size<VMType::REAL, with_descr>
	= sizeof(t_vm_real) + (with_descr ? sizeof(t_vm_byte) : 0);
template<bool with_descr> constexpr inline t_vm_addr vm_type_size<VMType::INT, with_descr>
	= sizeof(t_vm_int) + (with_descr ? sizeof(t_vm_byte) : 0);
template<bool with_descr> constexpr inline t_vm_addr vm_type_size<VMType::BOOLEAN, with_descr>
	= sizeof(t_vm_bool) + (with_descr ? sizeof(t_vm_byte) : 0);
template<bool with_descr> constexpr inline t_vm_addr vm_type_size<VMType::ADDR_MEM, with_descr>
	= sizeof(t_vm_addr) + (with_descr ? sizeof(t_vm_byte) : 0);
template<bool with_descr> constexpr inline t_vm_addr vm_type_size<VMType::ADDR_IP, with_descr>
	= sizeof(t_vm_addr) + (with_descr ? sizeof(t_vm_byte) : 0);
template<bool with_descr> constexpr inline t_vm_addr vm_type_size<VMType::ADDR_SP, with_descr>
	= sizeof(t_vm_addr) + (with_descr ? sizeof(t_vm_byte) : 0);
template<bool with_descr> constexpr inline t_vm_addr vm_type_size<VMType::ADDR_BP, with_descr>
	= sizeof(t_vm_addr) + (with_descr ? sizeof(t_vm_byte) : 0);
//template<bool with_descr> constexpr inline t_vm_addr vm_type_size<VMType::STR, with_descr>
//	= g_vm_longest_size + (with_descr ? sizeof(t_vm_byte) : 0);


static inline t_vm_addr get_vm_str_size(t_vm_addr raw_len,
	bool with_descr = false, bool with_len = false)
{
	return raw_len*sizeof(t_vm_byte)
		+ (with_len ? sizeof(t_vm_addr) : 0)
		+ (with_descr ? sizeof(t_vm_byte) : 0);
}


static inline t_vm_addr get_vm_vec_size(t_vm_addr raw_len,
	bool with_descr = false, bool with_len = false)
{
	return raw_len*sizeof(t_vm_real)
		+ (with_len ? sizeof(t_vm_addr) : 0)
		+ (with_descr ? sizeof(t_vm_byte) : 0);
}


static inline t_vm_addr get_vm_mat_size(
	t_vm_addr raw_len_1, t_vm_addr raw_len_2,
	bool with_descr = false, bool with_len = false)
{
	return raw_len_1*raw_len_2*sizeof(t_vm_real)
		+ (with_len ? 2*sizeof(t_vm_addr) : 0)
		+ (with_descr ? sizeof(t_vm_byte) : 0);
}


#endif
