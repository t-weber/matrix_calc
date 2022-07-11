/**
 * basic data types
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 10-july-2022
 * @license: see 'LICENSE.GPL' file
 */

#ifndef __LVAL_TYPES_H__
#define __LVAL_TYPES_H__


#ifdef __cplusplus

#include <cstdint>
#include <string>


using t_real = double;
using t_int = std::int64_t;
using t_str = std::string;


#else  // c compilation, e.g. for runtime


#include <stdint.h>

typedef double t_real;
typedef int64_t t_int;


#endif
#endif
