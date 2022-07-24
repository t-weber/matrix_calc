/**
 * asm generator and vm opcodes
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 14-jun-2022
 * @license see 'LICENSE.GPL' file
 */

#ifndef __0ACVM_OPCODES_H__
#define __0ACVM_OPCODES_H__

#include "types.h"



enum class OpCode : t_vm_byte
{
	HALT     = 0x00,  // stop program
	NOP      = 0x01,  // no operation
	INVALID  = 0x02,  // invalid opcode

	// memory operations
	PUSH     = 0x10,  // push direct data
	WRMEM    = 0x11,  // write memory
	RDMEM    = 0x12,  // read memory

	// arithmetic operations
	USUB     = 0x20,  // unary -
	ADD      = 0x21,  // +
	SUB      = 0x22,  // -
	MUL      = 0x23,  // *
	DIV      = 0x24,  // /
	MOD      = 0x25,  // %
	POW      = 0x26,  // ^

	// conversions
	TOI      = 0x30,  // cast to int
	TOF      = 0x31,  // cast to real
	TOS      = 0x32,  // cast to string
	TOV      = 0x33,  // cast to vector
	TOM      = 0x34,  // cast to matrix

	// jumps
	JMP      = 0x40,  // unconditional jump
	JMPCND   = 0x41,  // conditional jump

	// logical operations
	AND      = 0x50,  // &&
	OR       = 0x51,  // ||
	XOR      = 0x52,  // ^
	NOT      = 0x53,  // !

	// comparisons
	GT       = 0x60,  // >
	LT       = 0x61,  // <
	GEQU     = 0x62,  // >=
	LEQU     = 0x63,  // <=
	EQU      = 0x64,  // ==
	NEQU     = 0x65,  // !=

	// function calls
	CALL     = 0x70,  // call function
	RET      = 0x71,  // return from function
	EXTCALL  = 0x72,  // call system function

	// binary operations
	BINAND   = 0x80,  // &
	BINOR    = 0x81,  // |
	BINXOR   = 0x82,  // ^
	BINNOT   = 0x83,  // ~
	SHL      = 0x84,  // <<
	SHR      = 0x85,  // >>
	ROTL     = 0x86,  // rotate left
	ROTR     = 0x87,  // rotate right

	// array operations
	MAKEVEC  = 0x90,  // create a vector
	MAKEMAT  = 0x91,  // create a matrix

	// array memory operations
	RDARR1D  = 0xa0,  // read element from a 1d array type
	RDARR1DR = 0xa1,  // read range from a 1d array type
	RDARR2D  = 0xa2,  // read element from a 2d array type
	RDARR2DR = 0xa3,  // read range from a 2d array type

	WRARR1D  = 0xa4,  // write element to a 1d array type
	WRARR1DR = 0xa5,  // write range to a 1d array type
	WRARR2D  = 0xa6,  // write element to a 2d array type
	WRARR2DR = 0xa7,  // write range to a 2d array type
};



/**
 * get a string representation of an opcode
 */
template<class t_str = const char*>
constexpr t_str get_vm_opcode_name(OpCode op)
{
	switch(op)
	{
		case OpCode::HALT:      return "halt";
		case OpCode::NOP:       return "nop";
		case OpCode::INVALID:   return "invalid";
		case OpCode::PUSH:      return "push";
		case OpCode::WRMEM:     return "wrmem";
		case OpCode::RDMEM:     return "rdmem";
		case OpCode::USUB:      return "usub";
		case OpCode::ADD:       return "add";
		case OpCode::SUB:       return "sub";
		case OpCode::MUL:       return "mul";
		case OpCode::DIV:       return "div";
		case OpCode::MOD:       return "mod";
		case OpCode::POW:       return "pow";
		case OpCode::TOI:       return "toi";
		case OpCode::TOF:       return "tof";
		case OpCode::TOS:       return "tos";
		case OpCode::TOV:       return "tov";
		case OpCode::TOM:       return "tom";
		case OpCode::JMP:       return "jmp";
		case OpCode::JMPCND:    return "jmpcnd";
		case OpCode::AND:       return "and";
		case OpCode::OR:        return "or";
		case OpCode::XOR:       return "xor";
		case OpCode::NOT:       return "not";
		case OpCode::GT:        return "gt";
		case OpCode::LT:        return "lt";
		case OpCode::GEQU:      return "gequ";
		case OpCode::LEQU:      return "lequ";
		case OpCode::EQU:       return "equ";
		case OpCode::NEQU:      return "nequ";
		case OpCode::CALL:      return "call";
		case OpCode::RET:       return "ret";
		case OpCode::EXTCALL:   return "extcall";
		case OpCode::BINAND:    return "binand";
		case OpCode::BINOR:     return "binor";
		case OpCode::BINXOR:    return "binxor";
		case OpCode::BINNOT:    return "binnot";
		case OpCode::SHL:       return "shl";
		case OpCode::SHR:       return "shr";
		case OpCode::ROTL:      return "rotl";
		case OpCode::ROTR:      return "rotr";
		case OpCode::MAKEVEC:   return "makevec";
		case OpCode::MAKEMAT:   return "makemat";
		case OpCode::RDARR1D:   return "rdarr1d";
		case OpCode::RDARR1DR:  return "rdarr1dr";
		case OpCode::RDARR2D:   return "rdarr2d";
		case OpCode::RDARR2DR:  return "rdarr2dr";
		case OpCode::WRARR1D:   return "wrarr1d";
		case OpCode::WRARR1DR:  return "wrarr1dr";
		case OpCode::WRARR2D:   return "wrarr2d";
		case OpCode::WRARR2DR:  return "wrarr2dr";
		default:                return "<unknown>";
	}
}


#endif
