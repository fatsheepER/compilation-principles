add_library(syntax_analysis_top_down_lib
    src/grammar.cpp
    src/parser.cpp
    src/token_reader.cpp
    src/result_writer.cpp
)

target_include_directories(syntax_analysis_top_down_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(syntax_analysis_top_down
    src/main.cpp
)

target_link_libraries(syntax_analysis_top_down
    PRIVATE
        syntax_analysis_top_down_lib
)

if(MSVC)
    target_compile_options(syntax_analysis_top_down_lib PRIVATE /W4)
    target_compile_options(syntax_analysis_top_down PRIVATE /W4)
else()
    target_compile_options(syntax_analysis_top_down_lib PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(syntax_analysis_top_down PRIVATE -Wall -Wextra -pedantic)
endif()

add_custom_target(run_syntax_analysis_top_down
    COMMAND syntax_analysis_top_down
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS syntax_analysis_top_down
    COMMENT "Run top-down syntax analysis"
    USES_TERMINAL
)
