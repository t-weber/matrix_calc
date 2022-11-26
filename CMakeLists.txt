#
# @author Tobias Weber (orcid: 0000-0002-7230-1932)
# @date 11-apr-2020
# @license: see 'LICENSE.GPL' file
#

project(mcalc)
cmake_minimum_required(VERSION 3.0)

option(USE_BOOST_GIL "use boost.gil" FALSE)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_VERBOSE_MAKEFILE ON)

add_definitions(-Wall -Wextra)


find_package(FLEX 2 REQUIRED)
find_package(BISON 3 REQUIRED)
find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options)

add_definitions(${Boost_CXX_FLAGS})

if(USE_BOOST_GIL)
	find_package(PNG REQUIRED)
	add_definitions(${PNG_DEFINITIONS} -DUSE_BOOST_GIL)
	include_directories(${PNG_INCLUDE_DIRS})
endif()


include_directories(
	"${PROJECT_SOURCE_DIR}"
	"${PROJECT_SOURCE_DIR}/src"
	"${FLEX_INCLUDE_DIRS}" "${Boost_INCLUDE_DIRS}" "${Boost_INCLUDE_DIRS}/.."
	"${CMAKE_CURRENT_BINARY_DIR}"
)



BISON_TARGET(parser_impl
	src/parser_yy/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser_impl.cpp
	DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser_defs.h
	VERBOSE REPORT_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser_report.txt
)


FLEX_TARGET(lexer_impl
	src/parser_yy/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer_impl.cpp
	DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/lexer_impl.h
)

ADD_FLEX_BISON_DEPENDENCY(lexer_impl parser_impl)



message("Flex outputs: ${FLEX_lexer_impl_OUTPUTS}")
message("Bison output source: ${BISON_parser_impl_OUTPUT_SOURCE}")
message("Bison output header: ${BISON_parser_impl_OUTPUT_HEADER}")



# -----------------------------------------------------------------------------
# 3-ac code generator
# -----------------------------------------------------------------------------
add_executable(mcalc
	src/parser_yy/parser.cpp src/parser_yy/parser.h
	src/common/types.h src/common/sym.h
	src/common/ext_funcs.h src/common/ext_funcs.h
	src/ast/ast.h
	src/ast/printast.cpp src/ast/printast.h
	src/ast/semantics.cpp src/ast/semantics.h
	src/codegen_3ac/main.cpp
	src/codegen_3ac/asm.cpp src/codegen_3ac/ops.cpp src/codegen_3ac/var.cpp
	src/codegen_3ac/arr.cpp src/codegen_3ac/func.cpp src/codegen_3ac/asm.h
	${FLEX_lexer_impl_OUTPUTS}
	${BISON_parser_impl_OUTPUT_SOURCE} ${BISON_parser_impl_OUTPUT_HEADER}
)

#add_dependencies(mcalc parser_impl lexer_impl)
target_link_libraries(mcalc ${Boost_LIBRARIES})



# build runtime for 3-ac code generator
execute_process(COMMAND clang -O2 -S -emit-llvm -I"${PROJECT_SOURCE_DIR}/src" -o runtime_opt.asm "${PROJECT_SOURCE_DIR}/src/libs/runtime.c")
execute_process(COMMAND clang -S -emit-llvm -o runtime.asm "${PROJECT_SOURCE_DIR}/src/libs/runtime.c")
# -----------------------------------------------------------------------------



# -----------------------------------------------------------------------------
# 0-ac code generator
# -----------------------------------------------------------------------------
add_executable(mcalc_0ac
	src/ast/ast.h
	src/ast/printast.cpp src/ast/printast.h
	src/ast/semantics.cpp src/ast/semantics.h
	src/common/types.h src/common/sym.h
	src/common/ext_funcs.h src/common/context.h
	src/parser_yy/parser.cpp src/parser_yy/parser.h
	src/codegen_0ac/main.cpp
	src/codegen_0ac/asm.cpp src/codegen_0ac/asm.h
	src/codegen_0ac/func.cpp src/codegen_0ac/var.cpp
	src/codegen_0ac/ops.cpp src/codegen_0ac/arr.cpp
	${FLEX_lexer_impl_OUTPUTS}
	${BISON_parser_impl_OUTPUT_SOURCE} ${BISON_parser_impl_OUTPUT_HEADER}
)

#add_dependencies(mcalc_0ac parser_impl lexer_impl)
target_link_libraries(mcalc_0ac ${Boost_LIBRARIES})
# -----------------------------------------------------------------------------



# -----------------------------------------------------------------------------
# 0-ac vm
# -----------------------------------------------------------------------------
add_executable(vm_0ac
	src/vm_0ac/main.cpp src/vm_0ac/types.h
	src/common/types.h
	src/vm_0ac/opcodes.h src/vm_0ac/vm.h
	src/vm_0ac/vm.cpp src/vm_0ac/run.cpp
	src/vm_0ac/extfuncs.cpp
	src/vm_0ac/memdump.cpp
)

target_link_libraries(vm_0ac ${Boost_LIBRARIES}
	$<$<TARGET_EXISTS:Threads::Threads>:Threads::Threads>
	$<$<TARGET_EXISTS:PNG::PNG>:PNG::PNG>
)
# -----------------------------------------------------------------------------
