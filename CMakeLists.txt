cmake_minimum_required(VERSION 3.20)

project(compilation_principles VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(lexical_analysis_lib
    lexical_analysis/src/lexer.cpp
    lexical_analysis/src/tables.cpp
)
target_include_directories(lexical_analysis_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/lexical_analysis/src
)

add_executable(lexical_analysis
    lexical_analysis/src/main.cpp
)
target_link_libraries(lexical_analysis
    PRIVATE
        lexical_analysis_lib
)

add_executable(syntax_analysis_top_down
    syntax_analysis_top_down/src/main.cpp
)

if(MSVC)
    target_compile_options(lexical_analysis_lib PRIVATE /W4)
    target_compile_options(lexical_analysis PRIVATE /W4)
    target_compile_options(syntax_analysis_top_down PRIVATE /W4)
else()
    target_compile_options(lexical_analysis_lib PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(lexical_analysis PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(syntax_analysis_top_down PRIVATE -Wall -Wextra -pedantic)
endif()

add_custom_target(run_lexical_analysis
    COMMAND lexical_analysis
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lexical_analysis
    DEPENDS lexical_analysis
    COMMENT "Run lexical analysis with lexical_analysis/input/s.txt"
    USES_TERMINAL
)

add_custom_target(run_syntax_analysis_top_down
    COMMAND syntax_analysis_top_down
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/syntax_analysis_top_down
    DEPENDS syntax_analysis_top_down
    COMMENT "Run top-down syntax analysis"
    USES_TERMINAL
)
