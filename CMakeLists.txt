cmake_minimum_required(VERSION 3.16)
project(lab4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(lab4 main.cpp LexicalAnalyzer/Scaner.cpp LexicalAnalyzer/Scaner.h LexicalAnalyzer/defs.h
        SyntaxAnalyzer/TDiagram.cpp
        SyntaxAnalyzer/TDiagram.h
        "semantic tree/SemTreeBase.cpp"
        "semantic tree/SemTreeBase.h"
        "semantic tree/SemTreeControl.cpp"
        "semantic tree/SemTreeControl.h")

# Абсолютный путь позволяет запускать отладочную сборку из папки CLion:
# там рабочая директория обычно не совпадает с каталогом input.txt.
target_compile_definitions(lab4 PRIVATE DEFAULT_INPUT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/input.txt")

if (MSVC)
    target_compile_options(lab4 PRIVATE /W4)
else()
    target_compile_options(lab4 PRIVATE -Wall -Wextra -Wpedantic)
endif()
