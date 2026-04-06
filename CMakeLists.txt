cmake_minimum_required(VERSION 3.14)

project(CitadelTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#сторонние библиотеки

add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

#библиотека с нашим кодом

add_library(ProjectCore STATIC
    source/Aggregator/Aggregator.cpp
    source/Orchestrator/Orchestrator.cpp
    source/Parser/Parser.cpp
    source/Worker/Worker.cpp
)

target_include_directories(ProjectCore PUBLIC 
    source/Aggregator
    source/Logger
    source/Orchestrator
    source/Parser
    source/Worker
    include
)

target_link_libraries(ProjectCore PUBLIC nlohmann_json::nlohmann_json)

#исполняемые файлы

add_executable(main_app main.cpp)
target_link_libraries(main_app PRIVATE ProjectCore)

enable_testing()
include(GoogleTest)

add_executable(unit_tests 
    module_GTests/aggregatorTest.cpp
    module_GTests/parserTest.cpp
    module_GTests/workerTest.cpp
)

target_link_libraries(unit_tests PRIVATE 
    ProjectCore 
    GTest::gtest_main
)

gtest_discover_tests(unit_tests)

#копирование примера

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/example/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/example)