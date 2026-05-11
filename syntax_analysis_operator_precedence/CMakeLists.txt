add_library(syntax_analysis_operator_precedence_lib
    src/grammar.h
    src/grammar.cpp
    src/first_last_vt.h
    src/first_last_vt.cpp
)

target_include_directories(syntax_analysis_operator_precedence_lib
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(syntax_analysis_operator_precedence_lib
    PUBLIC 
        syntax_analysis_common
)

add_executable(syntax_analysis_operator_precedence
    src/main.cpp
)

target_link_libraries(syntax_analysis_operator_precedence
    PRIVATE 
        syntax_analysis_operator_precedence_lib
)

if(MSVC)
    target_compile_options(syntax_analysis_operator_precedence_lib PRIVATE /W4)
    target_compile_options(syntax_analysis_operator_precedence PRIVATE /W4)
else()
    target_compile_options(syntax_analysis_operator_precedence_lib PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(syntax_analysis_operator_precedence PRIVATE -Wall -Wextra -pedantic)
endif()

add_custom_target(run_syntax_analysis_operator_precedence
    COMMAND syntax_analysis_operator_precedence
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS syntax_analysis_operator_precedence
    COMMENT "Run operator-precedence syntax analysis"
    USES_TERMINAL
)