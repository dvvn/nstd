cmake_minimum_required(VERSION 3.14)

project(nstd CXX)

set(NSTD_DIR ${PROJECT_SOURCE_DIR}/impl)
set(NSTD_WORK_DIR ${NSTD_DIR}/nstd)

file(GLOB_RECURSE NSTD_FILES ${NSTD_WORK_DIR}/*.cpp ${NSTD_WORK_DIR}/*.ixx ${NSTD_WORK_DIR}/*.h)

add_library(nstd ${NSTD_FILES})
add_library(nstd::nstd ALIAS nstd)
target_include_directories(nstd PUBLIC ${NSTD_DIR})

include(FetchContent)

FetchContent_Declare(utf8_conv GIT_REPOSITORY https://github.com/ww898/utf-cpp.git)
FetchContent_Populate(utf8_conv)
target_include_directories(nstd PUBLIC ${utf8_conv_SOURCE_DIR}/include)

FetchContent_Declare(veque GIT_REPOSITORY https://github.com/Shmoopty/veque.git)
FetchContent_Populate(veque)
target_include_directories(nstd PUBLIC ${veque_SOURCE_DIR}/include)

set(FMT_MODULE OFF)
FetchContent_Declare(fmtlib GIT_REPOSITORY https://github.com/fmtlib/fmt.git)
FetchContent_MakeAvailable(fmtlib)
target_link_libraries(nstd fmt::fmt)
target_include_directories(nstd PUBLIC ${fmtlib_SOURCE_DIR}/include)

set(RANGES_DEEP_STL_INTEGRATION ON)
FetchContent_Declare(rangev3 GIT_REPOSITORY https://github.com/ericniebler/range-v3.git)
FetchContent_MakeAvailable(rangev3)
target_link_libraries(nstd range-v3)
target_include_directories(nstd PUBLIC ${rangev3_SOURCE_DIR}/include)
