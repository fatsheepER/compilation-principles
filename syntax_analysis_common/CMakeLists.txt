add_library(syntax_analysis_common
    src/terminal.h
    src/terminal.cpp
    src/input_token.h
    src/token_reader.h
    src/token_reader.cpp
)

target_include_directories(syntax_analysis_common
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(MSVC)
    target_compile_options(syntax_analysis_common PRIVATE /W4)
else()
    target_compile_options(syntax_analysis_common PRIVATE -Wall -Wextra -pedantic)
endif()