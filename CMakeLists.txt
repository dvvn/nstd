cmake_minimum_required(VERSION 3.12)

project(nstd)

set(NSTD_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/impl)
set(NSTD_WORK_DIR ${NSTD_DIR}/nstd)

include_directories(impl ${NSTD_DIR})

file(GLOB_RECURSE sources
    ${NSTD_WORK_DIR}/*.cpp ${NSTD_WORK_DIR}/*.ixx)

add_library(${PROJECT_NAME} ${sources})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
