add_library(syntax_analysis_slr_lib
    src/grammar.h 
    src/grammar.cpp
    src/follow_set.h
    src/follow_set.cpp
    src/lr0_automaton.h
    src/lr0_automaton.cpp
    src/slr_table.h
    src/slr_table.cpp
)

target_include_directories(syntax_analysis_slr_lib
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/src 
)

target_link_libraries(syntax_analysis_slr_lib
    PUBLIC 
        syntax_analysis_common
)

add_executable(syntax_analysis_slr
    src/main.cpp
)

target_link_libraries(syntax_analysis_slr
    PRIVATE 
        syntax_analysis_slr_lib
)

if(MSVC)
    target_compile_options(syntax_analysis_slr_lib PRIVATE /W4)
    target_compile_options(syntax_analysis_slr PRIVATE /W4)
else()
    target_compile_options(syntax_analysis_slr_lib PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(syntax_analysis_slr PRIVATE -Wall -Wextra -pedantic)
endif()

add_custom_target(run_syntax_analysis_slr
    COMMAND syntax_analysis_slr
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS syntax_analysis_slr
    COMMENT "Run SLR(1) syntax analysis"
    USES_TERMINAL
)