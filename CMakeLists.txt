cmake_minimum_required (VERSION 3.10)

include(GenerateExportHeader)

find_package(LLVM REQUIRED CONFIG)

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})

# Ekstazi CMake variables
set(EKSTAZI_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(EKSTAZI_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(EKSTAZI_LIB_SOURCE_DIR ${EKSTAZI_SOURCE_DIR}/ekstazi-lib)

# Include directory
include_directories(${EKSTAZI_INCLUDE_DIR})

# We need the following flags to link to a locally built LLVM
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-rtti -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fPIC")

# Ekstazi LLVM Pass
add_library(ekstazi-pass
  MODULE

  src/ekstazi.cc
  src/sha512.cc
)

# Ekstazi Library
set(EKSTAZI_LIB_SOURCES

  ${EKSTAZI_LIB_SOURCE_DIR}/depgraph/depgraph.cc
  ${EKSTAZI_LIB_SOURCE_DIR}/depgraph/file-parser.cc
  ${EKSTAZI_LIB_SOURCE_DIR}/depgraph/function.cc
  ${EKSTAZI_LIB_SOURCE_DIR}/type-hierarchy/type-hierarchy.cc

  ${EKSTAZI_LIB_SOURCE_DIR}/test-frameworks/gtest/gtest-adapter.cc

  ${EKSTAZI_LIB_SOURCE_DIR}/test-frameworks/gtest/test-types/test.cc
  ${EKSTAZI_LIB_SOURCE_DIR}/test-frameworks/gtest/test-types/value-parameterized-test.cc
  ${EKSTAZI_LIB_SOURCE_DIR}/test-frameworks/gtest/test-types/typed-test.cc
  ${EKSTAZI_LIB_SOURCE_DIR}/test-frameworks/gtest/test-types/type-parameterized-test.cc
  
  ${EKSTAZI_LIB_SOURCE_DIR}/llvm/function-comparator.cc

  ${EKSTAZI_LIB_SOURCE_DIR}/vtable/vtable.cc

  ${EKSTAZI_LIB_SOURCE_DIR}/utils/mangle.cc
  ${EKSTAZI_LIB_SOURCE_DIR}/utils/graph.cc
)

add_library(ekstazi-lib ${EKSTAZI_LIB_SOURCES})
target_compile_options(ekstazi-lib
  PUBLIC
  -std=c++17
)

add_executable(depgraph-analyzer
  ${EKSTAZI_LIB_SOURCE_DIR}/depgraph/analyzer/depgraph-analyzer.cc
)
target_link_libraries(depgraph-analyzer ekstazi-lib)

add_executable(type-hierarchy-analyzer
  ${EKSTAZI_SOURCE_DIR}/tools/type-hierarchy-analyzer.cc
)

target_link_libraries(type-hierarchy-analyzer ekstazi-lib)

add_executable(results-analyzer
  ${EKSTAZI_SOURCE_DIR}/tools/results-analyzer.cc
)

target_link_libraries(results-analyzer ekstazi-lib)

# add_subdirectory(src/depgraph)
# add_subdirectory(src/test-frameworks)

target_link_libraries(ekstazi-pass ekstazi-lib)

# Export module
generate_export_header(ekstazi-lib)
install(TARGETS ekstazi-lib EXPORT EkstaziTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/ekstazi/EkstaziConfigVersion.cmake"
  VERSION 1.0.0
  COMPATIBILITY AnyNewerVersion
)

export(EXPORT EkstaziTargets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/ekstazi/EkstaziTargets.cmake"
)

configure_file(cmake/EkstaziConfig.cmake
  "${CMAKE_CURRENT_BINARY_DIR}/ekstazi/EkstaziConfig.cmake"
  COPYONLY
)

set(ConfigPackageLocation lib/cmake/ekstazi)

install(EXPORT EkstaziTargets
  FILE EkstaziTargets.cmake
  DESTINATION ${ConfigPackageLocation}
)

install(
  FILES
    cmake/EkstaziConfig.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/ekstazi/EkstaziConfigVersion.cmake"
  DESTINATION
    ${ConfigPackageLocation}
)

# install(EXPORT EkstaziTargets
#   FILE EkstaziTargets.cmake
#   DESTINATION ${ConfigPackageLocation}
# )

# install(
#   FILES cmake/EkstaziConfig.cmake
#   "${}"
# )


# install(TARGETS ekstazi-pass ekstazi)

# Use the following to integrate into LLVM source code

# add_llvm_loadable_module(
#   Ekstazi

#   filename.cc
#   sha512.cc

#   PLUGIN_TOOL
#   opt
# )
