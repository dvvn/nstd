cmake_minimum_required(VERSION 3.14)

project(nstd CXX)

include(CheckSymbolExists)

#unset(HAVE_STD_FORMAT CACHE)
check_symbol_exists(__cpp_lib_format "version" HAVE_STD_FORMAT)
add_compile_definitions()
#unset(HAVE_STD_RANGES CACHE)
check_symbol_exists(__cpp_lib_ranges "version" HAVE_STD_RANGES)

set(NSTD_DIR ${PROJECT_SOURCE_DIR}/impl)
set(NSTD_WORK_DIR ${NSTD_DIR}/nstd)

file(GLOB_RECURSE NSTD_FILES ${NSTD_WORK_DIR}/*.cpp ${NSTD_WORK_DIR}/*.ixx ${NSTD_WORK_DIR}/*.h)

add_library(nstd ${NSTD_FILES})
add_library(nstd::nstd ALIAS nstd)
target_include_directories(nstd PUBLIC ${NSTD_DIR})

include(FetchContent)

FetchContent_Declare(utf8_conv GIT_REPOSITORY https://github.com/ww898/utf-cpp.git)
FetchContent_Populate(utf8_conv) #no cmakelist inside
target_include_directories(nstd PUBLIC ${utf8_conv_SOURCE_DIR}/include)

FetchContent_Declare(veque GIT_REPOSITORY https://github.com/Shmoopty/veque.git)
FetchContent_Populate(veque) #no cmakelist inside
target_include_directories(nstd PUBLIC ${veque_SOURCE_DIR}/include)
target_compile_definitions(nstd PUBLIC NSTD_CUSTOM_DEQUE)

FetchContent_Declare(robin_hood GIT_REPOSITORY https://github.com/martinus/robin-hood-hashing.git)
FetchContent_MakeAvailable(robin_hood)
target_include_directories(nstd PUBLIC ${robin_hood_SOURCE_DIR}/src/include)
target_compile_definitions(nstd PUBLIC NSTD_CUSTOM_UNORDERED_MAP NSTD_CUSTOM_UNORDERED_SET)
target_link_libraries(nstd PUBLIC robin_hood::robin_hood)

if(NOT ${HAVE_STD_FORMAT})
  set(FMT_MODULE OFF)
  FetchContent_Declare(fmtlib GIT_REPOSITORY https://github.com/fmtlib/fmt.git)
  FetchContent_MakeAvailable(fmtlib)
  target_link_libraries(nstd fmt::fmt)
  target_include_directories(nstd PUBLIC ${fmtlib_SOURCE_DIR}/include)
  target_compile_definitions(nstd PUBLIC NSTD_CUSTOM_FORMAT)
endif()

if(NOT ${HAVE_STD_RANGES})
  set(RANGES_DEEP_STL_INTEGRATION ON)
  FetchContent_Declare(rangev3 GIT_REPOSITORY https://github.com/ericniebler/range-v3.git)
  FetchContent_MakeAvailable(rangev3)
  target_link_libraries(nstd range-v3)
  target_include_directories(nstd PUBLIC ${rangev3_SOURCE_DIR}/include)
  target_compile_definitions(nstd PUBLIC NSTD_CUSTOM_RANGES)
endif()
