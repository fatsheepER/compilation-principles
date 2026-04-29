add_library(lexical_analysis_lib
    src/lexer.cpp
    src/tables.cpp
)

target_include_directories(lexical_analysis_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(lexical_analysis
    src/main.cpp
)

target_link_libraries(lexical_analysis
    PRIVATE
        lexical_analysis_lib
)

if(MSVC)
    target_compile_options(lexical_analysis_lib PRIVATE /W4)
    target_compile_options(lexical_analysis PRIVATE /W4)
else()
    target_compile_options(lexical_analysis_lib PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(lexical_analysis PRIVATE -Wall -Wextra -pedantic)
endif()

add_custom_target(run_lexical_analysis
    COMMAND lexical_analysis
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS lexical_analysis
    COMMENT "Run lexical analysis with input/s.txt"
    USES_TERMINAL
)