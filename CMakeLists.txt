cmake_minimum_required(VERSION 3.14)
project(LogParserProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 1. Подключение GTest ---
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# --- 2. Создание общей библиотеки логики ---
# Добавляем и парсер, и воркер в одну библиотеку для удобства линковки
add_library(core_lib 
    source/parser/parser.cpp 
    source/worker/worker.cpp
    source/aggregator/aggregator.cpp
    source/orchestrator/orchestrator.cpp
)

# Указываем пути к хедерам. 
# Интерфейсные пути (PUBLIC) позволят тестам автоматически "видеть" эти папки.
target_include_directories(core_lib PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/source/parser
    ${CMAKE_CURRENT_SOURCE_DIR}/source/worker
    ${CMAKE_CURRENT_SOURCE_DIR}/source/aggregator
    ${CMAKE_CURRENT_SOURCE_DIR}/source/orchestrator
)

# Потоки нужны для std::async и std::future в Worker
find_package(Threads REQUIRED)
target_link_libraries(core_lib PUBLIC Threads::Threads)

# --- 3. Тесты ---

# Тесты для парсера
add_executable(parser_tests module_GTests/parserTest.cpp)
target_link_libraries(parser_tests PRIVATE core_lib gtest_main)

# Тесты для воркера
add_executable(worker_tests module_GTests/workerTest.cpp)
target_link_libraries(worker_tests PRIVATE core_lib gtest_main)

# Тесты для агрегатора
add_executable(aggregator_tests module_GTests/aggregatorTest.cpp)
target_link_libraries(aggregator_tests PRIVATE core_lib gtest_main)

# Интеграция с GoogleTest (позволяет запускать тесты через интерфейс IDE)
include(GoogleTest)
gtest_discover_tests(parser_tests)
gtest_discover_tests(worker_tests)
gtest_discover_tests(aggregator_tests)