cmake_minimum_required(VERSION 3.14)

include(FetchContent)

set(NSTD_DIR ${CMAKE_CURRENT_LIST_DIR}/impl)
set(NSTD_WORK_DIR ${NSTD_DIR}/nstd)

project(nstd_format) # fmt::format or std::format if exists

add_library(nstd_format ${NSTD_WORK_DIR}/_empty.cpp)
add_library(nstd::format ALIAS nstd_format)
target_compile_definitions(nstd_format INTERFACE NSTD_LIB_FORMAT)

set(FMT_MODULE OFF)
set(FMT_DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
FetchContent_Declare(fmtlib GIT_REPOSITORY https://github.com/fmtlib/fmt.git)
FetchContent_MakeAvailable(fmtlib)
target_link_libraries(nstd_format PUBLIC fmt::fmt)
target_include_directories(nstd_format PRIVATE fmt::fmt)

project(nstd_ranges CXX) # std::ranges or ::ranges if exists

add_library(nstd_ranges INTERFACE)
add_library(nstd::ranges ALIAS nstd_ranges)
target_compile_definitions(nstd_ranges INTERFACE NSTD_LIB_RANGES)
target_include_directories(nstd_ranges INTERFACE ${NSTD_DIR}) #todo: include only the ranges file

set(RANGES_DEEP_STL_INTEGRATION ON)
FetchContent_Declare(ranges GIT_REPOSITORY https://github.com/ericniebler/range-v3.git)
FetchContent_MakeAvailable(ranges)
target_link_libraries(nstd_ranges INTERFACE range-v3)
target_include_directories(nstd_ranges INTERFACE range-v3)

project(nstd_core) #------------

add_library(nstd_core ${NSTD_WORK_DIR}/lazy_invoke.ixx ${NSTD_WORK_DIR}/one_instance.ixx ${NSTD_WORK_DIR}/runtime_assert.cpp ${NSTD_WORK_DIR}/type_name.ixx)
add_library(nstd::core ALIAS nstd_core)
target_include_directories(nstd_core PUBLIC ${NSTD_DIR})

project(nstd_mem) # memory leated tools

file(GLOB NSTD_MEM_FILES ${NSTD_WORK_DIR}/mem/*)

add_library(nstd_mem ${NSTD_MEM_FILES})
add_library(nstd::mem ALIAS nstd_mem)
target_include_directories(nstd_mem PRIVATE ${NSTD_DIR}) #only core_utils file used
target_link_libraries(nstd_mem PRIVATE nstd_format PRIVATE nstd_ranges)
#target_include_directories(nstd_mem PRIVATE nstd_format)

project(nstd_text) #--------------------

file(GLOB_RECURSE NSTD_TEXT_FILES ${NSTD_WORK_DIR}/text/*)

add_library(nstd_text ${NSTD_TEXT_FILES})
add_library(nstd::text ALIAS nstd_text)
target_link_libraries(nstd_text PUBLIC nstd::core)
target_include_directories(nstd_text PUBLIC nstd::core)

FetchContent_Declare(utf8_conv GIT_REPOSITORY https://github.com/ww898/utf-cpp.git)
FetchContent_Populate(utf8_conv) #no cmakelist inside
target_include_directories(nstd_text PRIVATE ${utf8_conv_SOURCE_DIR}/include)

project(nstd_winapi) #-----------

file(GLOB_RECURSE NSTD_WIN_FILES ${NSTD_WORK_DIR}/winapi/*)

add_library(nstd_winapi ${NSTD_WIN_FILES})
add_library(nstd::winapi ALIAS nstd_winapi)
target_link_libraries(nstd_winapi PUBLIC nstd::mem PUBLIC nstd::text)
target_include_directories(nstd_mem PUBLIC nstd::text)
