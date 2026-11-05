#ifndef LAB4_TDIAGRAM_H
#define LAB4_TDIAGRAM_H

#include "../LexicalAnalyzer/Scaner.h"
#include "../semantic tree/SemTreeControl.h"

#include <iosfwd>
#include <string>

class TDiagram {
private:
    TScaner* sc; // сканер, из которого получаем очередные лексемы
    SemTreeControl semanticTree;
    Node* currentFunction = nullptr;
    Node* currentClass = nullptr;
    bool currentFunctionHasReturn = false;

    // Программа, описания классов и переменных, функция main.
    void externalDescriptions();
    void externalDescription();
    void mainProgram();
    void classDescription();
    void classMember();
    void predeclareClassMembers();
    void skipClassMethodBody();
    void skipFieldInitializer();
    ValueType type();
    void dataDescription();
    void declarator(ValueType declaredType, bool addToTree = true);
    void fieldTail(ValueType declaredType, const std::string& firstName);

    // Составной оператор, его элементы и отдельные операторы.
    void block(bool reuseCurrentScope = false);
    void blockElement();
    void statement();
    void assignment();
    void forStatement();
    void returnStatement();
    ValueType leftValue();
    void methodCall();
    void identifierStatement();
    Node* identifierPath();

    // Выражение с уровнями приоритета операций.
    ValueType expression();
    ValueType logicalOr();
    ValueType logicalAnd();
    ValueType equality();
    ValueType relation();
    ValueType addition();
    ValueType multiplication();
    ValueType unary();
    ValueType primary();
    ValueType operand();

    void expect(int expectedType);
    std::string consume(int expectedType);

    int lookForward(int offset = 0);

public:
    explicit TDiagram(TScaner* scanner) : sc(scanner), semanticTree(scanner) {}

    // Начальная диаграмма <Программа>. С неё начинается разбор текста.
    void S_progr();
    void printSemanticTree(std::ostream& out) const;
};

#endif // LAB4_TDIAGRAM_H
