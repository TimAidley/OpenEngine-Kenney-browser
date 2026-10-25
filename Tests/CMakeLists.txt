# Standalone tests for the package's pure logic (pack-name parsing and the
# extraction rules). The editor builds the package module itself; this build
# exists only so the rules can be tested without an editor.
#
#   cmake -S Tests -B Tests/build -DCMAKE_PREFIX_PATH=<vcpkg_installed/x64-linux>
#   cmake --build Tests/build && Tests/build/KenneyAssetsTests
#
# KENNEY_BUNDLE_DIR (environment) points the bundle test at an unzipped
# All-in-1 bundle; without it that test is skipped.
cmake_minimum_required(VERSION 3.24)
project(KenneyAssetsTests CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OPENENGINE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../OpenEngine" CACHE PATH "OpenEngine source tree")
find_package(GTest CONFIG REQUIRED)

add_executable(KenneyAssetsTests
    KenneyPackNameTests.cpp
    KenneyExtractRulesTests.cpp
    ../kenney-assets/Editor/KenneyPackName.cpp
    ../kenney-assets/Editor/KenneyExtractRules.cpp)
target_include_directories(KenneyAssetsTests PRIVATE
    ../kenney-assets/Editor
    "${OPENENGINE_ROOT}/Engine/Modules/Types/Include")
target_compile_definitions(KenneyAssetsTests PRIVATE
    KENNEY_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Fixtures")
target_link_libraries(KenneyAssetsTests PRIVATE GTest::gtest_main)
