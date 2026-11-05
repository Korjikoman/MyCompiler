#include "Scaner.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

// Ключевые слова распознаются после выделения обычного идентификатора.
// true и false находятся в этой же таблице, но возвращают TConsBool.
const char* const KEYWORDS[MAX_KEYW] = {
    "class", "int", "bool", "double", "for", "return", "main", "true", "false"
};
const int KEYWORD_TYPES[MAX_KEYW] = {
    TClass, TInt, TBool, TDouble, TFor, TReturn, TMain, TConsBool, TConsBool
};

bool isLetter(const char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch == '_');
}

bool isDigit(const char ch) {
    return ch >= '0' && ch <= '9';
}

bool isIdentifierPart(const char ch) {
    return isLetter(ch) || isDigit(ch) || ch == '_';
}

} // namespace

TScaner::TScaner(const char* fileName) {
    getData(fileName);
}

char TScaner::current() const {
    return text[uk];
}

char TScaner::next() const {
    // Проверка нужна для файла ровно из MAX_TEXT - 1 символов:
    // его '\0' стоит в самом последнем элементе массива.
    if (uk + 1 >= MAX_TEXT) {
        return '\0';
    }
    return text[uk + 1];
}

void TScaner::advance() {
    if (current() == '\0') {
        return;
    }
    if (current() == '\n') {
        ++line;
        column = 1;
    } else {
        ++column;
    }
    ++uk;
}

void TScaner::printError(const char* message, const char* lexeme) const {
    // Диагностика печатается в тот же поток, что и таблица лексем:
    // так порядок строк не зависит от буферизации stdout/stderr.
    std::cout << "Лексическая ошибка (строка " << tokenLine
              << ", столбец " << tokenColumn << "): " << message;
    if (lexeme != nullptr && lexeme[0] != '\0') {
        std::cout << " [" << lexeme << ']';
    }
    std::cout << '\n';
}

void TScaner::semanticError(const std::string& message) const {
    std::ostringstream out;
    out << "семантическая ошибка (строка " << tokenLine
        << ", столбец " << tokenColumn << "): " << message;
    throw std::runtime_error(out.str());
}

bool TScaner::skipIgnored(LEX lexeme) {
    while (true) {
        // Состояние 1: пробельные символы зацикливают автомат в начальном состоянии.
        while (current() == ' ' || current() == '\t' ||
               current() == '\n' || current() == '\r') {
            advance();
        }

        // '/' может начинать комментарий или быть операцией деления.
        if (current() == '/' && next() == '/') {
            // Состояние 3: пропускаем все до конца строки.
            advance();
            advance();
            while (current() != '\n' && current() != '\r' && current() != '\0') {
                advance();
            }
            continue;
        }

        if (current() == '/' && next() == '*') {
            // Состояния 4 и 5: идем до пары "*/". Одиночная '*' комментарий не заканчивает.
            tokenLine = line;
            tokenColumn = column;
            advance();
            advance();
            while (current() != '\0' && !(current() == '*' && next() == '/')) {
                advance();
            }
            if (current() == '\0') {
                std::strcpy(lexeme, "/*");
                printError("незакрытый многострочный комментарий", lexeme);
                return false;
            }
            advance(); // '*'
            advance(); // '/'
            continue;
        }

        // Одиночный '/' остается для основного автомата и будет TDiv.
        return true;
    }
}

int TScaner::Scaner(LEX lexeme) {
    std::memset(lexeme, 0, MAX_LEX);
    if (!skipIgnored(lexeme)) {
        return TErr;
    }

    tokenLine = line;
    tokenColumn = column;

    if (current() == '\0') {
        lexeme[0] = '#'; // видимое отладочное изображение конца текста
        return TEnd;
    }

    std::size_t length = 0;
    bool tooLong = false;

    // Всегда поглощает символ. При переполнении буфера лишние символы не записываются,
    // но пропускаются, чтобы не сбить начало следующей лексемы.
    const auto take = [&]() {
        if (length < MAX_LEX - 1) {
            lexeme[length++] = current();
            lexeme[length] = '\0';
        } else {
            tooLong = true;
        }
        advance();
    };

    const auto finish = [&](const int type) {
        if (tooLong) {
            printError("лексема превышает MAX_LEX - 1 символов", lexeme);
            return TErr;
        }
        return type;
    };

    // Ветвь "буква": первый символ -- буква, остальные -- буквы, цифры или '_'.
    if (isLetter(current())) {
        do {
            take();
        } while (isIdentifierPart(current()));

        if (tooLong) {
            return finish(TIdent);
        }
        for (int i = 0; i < MAX_KEYW; ++i) {
            if (std::strcmp(lexeme, KEYWORDS[i]) == 0) {
                return KEYWORD_TYPES[i];
            }
        }
        return TIdent;
    }

    // Ветвь "цифра": только беззнаковые целые десятичные константы.
    // '+' и '-' будут отдельными лексемами; унарный смысл определит синтаксический анализатор.
    if (isDigit(current())) {
        do {
            take();
        } while (isDigit(current()));
        return finish(TConsInt);
    }

    // Односимвольные лексемы сразу ведут в тупиковое заключительное состояние.
    switch (current()) {
        case '.': take(); return TDot;
        case ',': take(); return TComma;
        case ';': take(); return TSemicolon;
        case '(': take(); return TLParen;
        case ')': take(); return TRParen;
        case '{': take(); return TLBrace;
        case '}': take(); return TRBrace;
        case '+': take(); return TPlus;
        case '-': take(); return TMinus;
        case '*': take(); return TMult;
        case '/': take(); return TDiv;
        case '%': take(); return TMod;

        // Заключительные состояния с переходом: при следующем '=' тип меняется.
        case '=':
            take();
            if (current() == '=') { take(); return TEq; }
            return TAssign;
        case '!':
            take();
            if (current() == '=') { take(); return TNeq; }
            return TNot;
        case '<':
            take();
            if (current() == '=') { take(); return TLe; }
            return TLt;
        case '>':
            take();
            if (current() == '=') { take(); return TGe; }
            return TGt;

        // Одиночные '&' и '|' в язык варианта не входят.
        case '&':
            take();
            if (current() == '&') { take(); return TAnd; }
            printError("ожидался второй символ '&'", lexeme);
            return TErr;
        case '|':
            take();
            if (current() == '|') { take(); return TOr; }
            printError("ожидался второй символ '|'", lexeme);
            return TErr;

        default:
            // Поглощаем ошибочный символ, иначе следующий вызов зациклится на нем.
            take();
            printError("символ не входит в алфавит языка", lexeme);
            return TErr;
    }
}

void TScaner::getData(const char* fileName) {
    std::ifstream input(fileName, std::ios::binary);
    if (!input) {
        throw std::runtime_error(std::string("не удалось открыть входной файл: ") + fileName);
    }

    input.read(text, MAX_TEXT - 1);
    const std::streamsize bytesRead = input.gcount();
    if (bytesRead == MAX_TEXT - 1 && input.peek() != std::char_traits<char>::eof()) {
        throw std::runtime_error("исходный файл длиннее MAX_TEXT - 1 символов");
    }
    text[static_cast<std::size_t>(bytesRead)] = '\0';
}
