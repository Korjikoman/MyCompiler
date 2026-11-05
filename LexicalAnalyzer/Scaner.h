#ifndef LAB4_SCANER_H
#define LAB4_SCANER_H

#include <cstddef>
#include <string>
#include "defs.h"

class TScaner {
private:
    TypeMod text{};             // исходный текст
    std::size_t uk = 0;         // индекс текущего символа
    int line = 1;               // текущая строка
    int column = 1;             // текущий столбец
    int tokenLine = 1;          // строка начала последней лексемы
    int tokenColumn = 1;        // столбец начала последней лексемы

    void getData(const char* fileName);
    char current() const;
    char next() const;
    void advance();

    // Автомат игнорируемых символов. false -- незакрытый /* ...
    bool skipIgnored(LEX lexeme);

public:
    // Снимок позиции для предварительного просмотра лексемы.
    // Одного uk недостаточно: Scaner меняет также строку и столбец.
    struct Position {
        std::size_t uk;
        int line;
        int column;
        int tokenLine;
        int tokenColumn;
    };

    explicit TScaner(const char* fileName);

    // Выделяет ровно одну лексему: тип возвращает, а текст записывает в lexeme.
    int Scaner(LEX lexeme);

    // Диагностика не завершает программу: main сможет показать все ошибки.
    void printError(const char* message, const char* lexeme) const;

    // Семантическая ошибка относится к последней прочитанной лексеме.
    [[noreturn]] void semanticError(const std::string& message) const;

    std::size_t getUK() const { return uk; }
    Position getPosition() const {
        return {uk, line, column, tokenLine, tokenColumn};
    }
    void setPosition(const Position& position) {
        uk = position.uk;
        line = position.line;
        column = position.column;
        tokenLine = position.tokenLine;
        tokenColumn = position.tokenColumn;
    }
    int getLine() const { return line; }
    int getColumn() const { return column; }
    int getTokenLine() const { return tokenLine; }
    int getTokenColumn() const { return tokenColumn; }
};

#endif
