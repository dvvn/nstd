cmake_minimum_required(VERSION 3.14)

project(nstd CXX)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(utf8_conv
                     GIT_REPOSITORY https://github.com/ww898/utf-cpp.git)
FetchContent_Populate(utf8_conv)
include_directories(${utf8_conv_SOURCE_DIR}/include)

FetchContent_Declare(veque GIT_REPOSITORY https://github.com/Shmoopty/veque.git)
FetchContent_Populate(veque)
include_directories(${veque_SOURCE_DIR}/include)

set(FMT_MODULE ON)
FetchContent_Declare(fmtlib GIT_REPOSITORY https://github.com/fmtlib/fmt.git)
FetchContent_MakeAvailable(fmtlib)

set(RANGES_DEEP_STL_INTEGRATION ON)
FetchContent_Declare(rangev3
                     GIT_REPOSITORY https://github.com/ericniebler/range-v3.git)
FetchContent_MakeAvailable(rangev3)

# ------------------

set(NSTD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/impl)
set(NSTD_WORK_DIR ${NSTD_DIR}/nstd)

include_directories(impl ${NSTD_DIR})

file(GLOB_RECURSE nstd_sources ${NSTD_WORK_DIR}/*.cpp ${NSTD_WORK_DIR}/*.ixx)

add_library(${PROJECT_NAME} ${nstd_sources})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_${CMAKE_CXX_STANDARD})
