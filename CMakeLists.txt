cmake_minimum_required(VERSION 3.14)
project(LogParserProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Подключаем GTest через интернет
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# 2. Описываем нашу библиотеку парсера
# Указываем пути относительно корня, где лежит CMakeLists.txt
add_library(parser_lib 
    source/parser/parser.cpp 
    source/parser/parser.h
)

# Указываем, где искать хедеры (включая nlohmann)
target_include_directories(parser_lib PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/source/parser
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 3. Создаем исполняемый файл для тестов
add_executable(run_tests test/parserTest.cpp)

# Линкуем всё вместе
target_link_libraries(run_tests PRIVATE 
    parser_lib 
    gtest_main
)

# Интеграция с тестами IDE
include(GoogleTest)
gtest_discover_tests(run_tests)