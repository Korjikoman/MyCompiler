//*****************************************************************************
// main.cpp -- отладочный запуск лексического, синтаксического и семантического анализа.
// Сначала печатаем таблицу лексем, затем разбираем файл и выводим семантическое дерево.
//*****************************************************************************
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

#include "LexicalAnalyzer/Scaner.h"
#include "LexicalAnalyzer/defs.h"
#include "SyntaxAnalyzer/TDiagram.h"

#ifndef DEFAULT_INPUT_PATH
#define DEFAULT_INPUT_PATH "input.txt"
#endif

int main(int argc, char* argv[]) {
    const char* fileName = (argc > 1) ? argv[1] : DEFAULT_INPUT_PATH;

    try {
        TScaner scanner(fileName);
        LEX lexeme{};
        int type = TErr;
        int errorCount = 0;

        std::cout << "Файл: " << fileName << "\n\n"
                  // setw считает байты UTF-8, а не русские буквы, поэтому шапка задана готовой строкой.
                  << "Позиция    Тип             Код     Лексема\n"
                  << std::string(52, '-') << '\n';

        do {
            type = scanner.Scaner(lexeme);
            if (type == TErr) {
                ++errorCount;
            }

            const std::string position = std::to_string(scanner.getTokenLine()) + ':' +
                                         std::to_string(scanner.getTokenColumn());
            std::cout << std::left
                      << std::setw(12) << position
                      << std::setw(16) << tokenTypeName(type)
                      << std::setw(8) << type
                      << lexeme << '\n';
        } while (type != TEnd);

        std::cout << "\nЛексический анализ завершён. Ошибок: " << errorCount << '\n';
        if (errorCount != 0) {
            std::cout << "Синтаксический анализ пропущен: сначала нужно исправить лексические ошибки.\n";
            return 1;
        }
        std::cout << "\nСинтаксический анализ...\n" << std::flush;
        TScaner syntaxScanner(fileName);
        TDiagram diagram(&syntaxScanner);
        diagram.S_progr();
        std::cout << "Синтаксический и семантический анализ завершён: ошибок нет.\n\n"
                  << "Семантическое дерево:\n";
        diagram.printSemanticTree(std::cout);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Ошибка анализа: " << error.what() << '\n';
        return 2;
    }
}
