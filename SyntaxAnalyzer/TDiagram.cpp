#include "TDiagram.h"

#include <sstream>
#include <stdexcept>

namespace {

bool isTypeStart(const int token) {
    return token == TInt || token == TBool || token == TDouble || token == TIdent;
}


[[noreturn]] void fail(TScaner* sc, const char* expected,
                       const int actual, const char* lexeme) {
    std::ostringstream message;
    message << "синтаксическая ошибка (строка " << sc->getTokenLine()
            << ", столбец " << sc->getTokenColumn() << "): ожидалось "
            << expected << ", получено " << tokenTypeName(actual)
            << " [" << lexeme << ']';
    throw std::runtime_error(message.str());
}


[[noreturn]] void unexpected(TScaner* sc, const char* expected) {
    LEX lexeme{};
    const int actual = sc->Scaner(lexeme);
    if (actual == TErr) {
        throw std::runtime_error("синтаксический разбор остановлен из-за лексической ошибки");
    }
    fail(sc, expected, actual, lexeme);
}

}

// Заглядывает на offset лексем вперёд, не меняя положение сканера.
int TDiagram::lookForward(const int offset) {
    if (offset < 0) {
        throw std::invalid_argument("смещение при просмотре вперёд не может быть отрицательным");
    }

    const TScaner::Position saved = sc->getPosition();
    LEX lexeme{};
    int token = TEnd;
    for (int i = 0; i <= offset; ++i) {
        token = sc->Scaner(lexeme);
        if (token == TEnd || token == TErr) {
            break;
        }
    }
    sc->setPosition(saved);
    if (token == TErr) {
        throw std::runtime_error("синтаксический разбор остановлен из-за лексической ошибки");
    }
    return token;
}


void TDiagram::expect(const int expectedType) {
    consume(expectedType);
}


std::string TDiagram::consume(const int expectedType) {
    LEX lexeme{};
    const int actual = sc->Scaner(lexeme);
    if (actual == TErr) {
        throw std::runtime_error("синтаксический разбор остановлен из-за лексической ошибки");
    }
    if (actual != expectedType) {
        fail(sc, tokenTypeName(expectedType), actual, lexeme);
    }
    return lexeme;
}


void TDiagram::S_progr() {
    externalDescriptions();

    if (lookForward() == TInt && lookForward(1) == TMain)
    {
        mainProgram();
    } else {
        unexpected(sc, "обязательная главная программа int main()");
    }
    expect(TEnd);
}


void TDiagram::externalDescriptions() {
    while (true) {
        const int first = lookForward();
        if (first == TEnd || (first == TInt && lookForward(1) == TMain)) {
            return;
        }
        externalDescription();
    }
}


void TDiagram::externalDescription() {
    if (lookForward() == TClass) {
        classDescription();
    } else if (isTypeStart(lookForward())) {
        dataDescription();
    } else {
        unexpected(sc, "описание класса, данных или int main()");
    }
}


void TDiagram::mainProgram() {
    expect(TInt);
    expect(TMain);
    expect(TLParen);
    expect(TRParen);

    Node* mainNode = semanticTree.declareFunction("main", {DataType::Int});
    semanticTree.enterScope(mainNode);
    currentFunction = mainNode;
    currentFunctionHasReturn = false;
    block(true);
    semanticTree.checkFunctionEnd(mainNode, currentFunctionHasReturn);
    currentFunction = nullptr;
    semanticTree.SetCurrentTree(mainNode);
}

// После class ident { многократно разбирает членов до закрывающей }.
// Конец файла внутри класса -- ошибка; после } обязательна точка с запятой.
void TDiagram::classDescription() {
    expect(TClass);
    const std::string className = consume(TIdent);
    Node* classNode = semanticTree.declareClass(className);
    semanticTree.enterScope(classNode);
    expect(TLBrace);

    const TScaner::Position bodyStart = sc->getPosition();
    Node* enclosingClass = currentClass;
    currentClass = classNode;
    predeclareClassMembers();
    classNode->complete = true;
    sc->setPosition(bodyStart);

    while (lookForward() != TRBrace) {
        if (lookForward() == TEnd) {
            unexpected(sc, "член класса или }");
        }
        classMember();
    }
    expect(TRBrace);
    expect(TSemicolon);
    currentClass = enclosingClass;
    semanticTree.SetCurrentTree(classNode);
}


void TDiagram::predeclareClassMembers() {
    while (lookForward() != TRBrace) {
        if (lookForward() == TEnd) unexpected(sc, "член класса или }");

        const ValueType memberType = type();
        std::string memberName = consume(TIdent);
        if (lookForward() == TLParen) {
            semanticTree.declareFunction(memberName, memberType);
            expect(TLParen);
            expect(TRParen);
            skipClassMethodBody();
            continue;
        }

        while (true) {
            semanticTree.declareVariable(memberName, memberType);
            if (lookForward() == TAssign) {
                expect(TAssign);
                skipFieldInitializer();
            }
            if (lookForward() != TComma) break;
            expect(TComma);
            memberName = consume(TIdent);
        }
        expect(TSemicolon);
    }
}


void TDiagram::skipClassMethodBody() {
    expect(TLBrace);
    int depth = 1;
    while (depth != 0) {
        LEX lexeme{};
        const int token = sc->Scaner(lexeme);
        if (token == TEnd || token == TErr) {
            fail(sc, "закрывающая } тела метода", token, lexeme);
        }
        if (token == TLBrace) ++depth;
        if (token == TRBrace) --depth;
    }
}


void TDiagram::skipFieldInitializer() {
    int parentheses = 0;
    while (true) {
        const int token = lookForward();
        if (parentheses == 0 && (token == TComma || token == TSemicolon)) return;
        if (token == TEnd || token == TRBrace) {
            unexpected(sc, "продолжение выражения, ',' или ';'");
        }
        LEX lexeme{};
        const int consumed = sc->Scaner(lexeme);
        if (consumed == TLParen) ++parentheses;
        if (consumed == TRParen) --parentheses;
        if (parentheses < 0) fail(sc, "выражение без лишней )", consumed, lexeme);
    }
}

// Поле и метод имеют общий префикс «тип ident». После него '(' означает
// метод без параметров; иначе fieldTail() разбирает остаток объявления поля.
void TDiagram::classMember() {
    const ValueType memberType = type();
    const std::string memberName = consume(TIdent);
    Node* member = semanticTree.FindRightDown(currentClass, memberName);
    if (member == nullptr) {
        sc->semanticError("внутренняя ошибка: заголовок члена '" + memberName + "' не зарегистрирован");
    }
    semanticTree.SetCurrentTree(member);

    // Общий префикс поля и метода уже прочитан: <Тип> ident.
    // Скобка выбирает метод без параметров; иначе идёт описание поля.
    if (lookForward() == TLParen) {
        Node* method = member;
        expect(TLParen);
        expect(TRParen);
        semanticTree.enterScope(method);
        Node* enclosingFunction = currentFunction;
        const bool enclosingHasReturn = currentFunctionHasReturn;
        currentFunction = method;
        currentFunctionHasReturn = false;
        block(true);
        semanticTree.checkFunctionEnd(method, currentFunctionHasReturn);
        currentFunction = enclosingFunction;
        currentFunctionHasReturn = enclosingHasReturn;
        semanticTree.SetCurrentTree(method);
    } else {
        fieldTail(memberType, memberName);
    }
}


ValueType TDiagram::type() {
    const int token = lookForward();
    switch (token) {
        case TInt: expect(TInt); return {DataType::Int};
        case TBool: expect(TBool); return {DataType::Bool};
        case TDouble: expect(TDouble); return {DataType::Double};
        case TIdent: return semanticTree.namedType(consume(TIdent));
        default: unexpected(sc, "тип int, bool, double или имя класса");
    }
}

// Описание данных: тип, хотя бы один объявитель, затем ноль или более
// объявителей через запятую и обязательная ';'.
void TDiagram::dataDescription() {
    const ValueType declaredType = type();
    declarator(declaredType);
    while (lookForward() == TComma) {
        expect(TComma);
        declarator(declaredType);
    }
    expect(TSemicolon);
}

// Один объявитель: имя переменной и необязательная инициализация '= выражение'.
// Запятую и ';' оставляет вызывающей функции.
void TDiagram::declarator(const ValueType declaredType, const bool addToTree) {
    const std::string name = consume(TIdent);
    if (addToTree) {
        semanticTree.declareVariable(name, declaredType);
    } else {
        Node* member = semanticTree.FindRightDown(currentClass, name);
        if (member == nullptr) {
            sc->semanticError("внутренняя ошибка: поле '" + name + "' не зарегистрировано");
        }
        semanticTree.SetCurrentTree(member);
    }
    if (lookForward() == TAssign) {
        expect(TAssign);
        semanticTree.checkAssignment(declaredType, expression());
    }
}

// было int x = 5, y, z = 10;
// разбираем остаток = 5, y, z = 10;
void TDiagram::fieldTail(const ValueType declaredType, const std::string& firstName) {
    (void)firstName;
    if (lookForward() == TAssign) {
        expect(TAssign);
        semanticTree.checkAssignment(declaredType, expression());
    }
    while (lookForward() == TComma) {
        expect(TComma);
        declarator(declaredType, false);
    }
    expect(TSemicolon);
}

void TDiagram::block(const bool reuseCurrentScope) {
    expect(TLBrace);
    Node* boundary = reuseCurrentScope ? nullptr : semanticTree.enterBlock();
    while (lookForward() != TRBrace) {
        if (lookForward() == TEnd) {
            unexpected(sc, "описание, оператор или }");
        }
        blockElement();
    }
    expect(TRBrace);
    if (boundary != nullptr) semanticTree.SetCurrentTree(boundary);
}


void TDiagram::blockElement() {
    const int first = lookForward();
    if (first == TInt || first == TBool || first == TDouble ||
        (first == TIdent && lookForward(1) == TIdent)) {
        dataDescription();
    } else {
        statement();
    }
}


void TDiagram::statement() {
    switch (lookForward()) {
        case TSemicolon: expect(TSemicolon); break; // пустой оператор
        case TLBrace: block(); break;
        case TFor: forStatement(); break;
        case TReturn: returnStatement(); break;
        case TIdent: identifierStatement(); break;
        default: unexpected(sc, "оператор ;, блок, for, return или ident");
    }
}

// присваивание
void TDiagram::assignment() {
    const ValueType targetType = leftValue();
    expect(TAssign);
    semanticTree.checkAssignment(targetType, expression());
}


void TDiagram::forStatement() {
    expect(TFor);
    expect(TLParen);
    if (lookForward() != TSemicolon) {
        assignment();
    }
    expect(TSemicolon);
    if (lookForward() != TSemicolon) {
        semanticTree.checkCondition(expression());
    }
    expect(TSemicolon);
    if (lookForward() != TRParen) {
        assignment();
    }
    expect(TRParen);
    statement();
}

// После return обязательно идёт выражение, затем ';'.
void TDiagram::returnStatement() {
    expect(TReturn);
    semanticTree.checkReturn(currentFunction, expression());
    currentFunctionHasReturn = true;
    expect(TSemicolon);
}


// car.speed = 60;
// обрабатываем car.speed
ValueType TDiagram::leftValue() {
    return semanticTree.variableType(identifierPath());
}

// car.getSpeed()
// car.engine.getPower()
void TDiagram::methodCall() {
    Node* method = identifierPath();
    expect(TLParen);
    expect(TRParen);
    semanticTree.callType(method);
}

// x = 5;
// car.speed = 100;
// car.getSpeed();
// car.engine.getPower();
void TDiagram::identifierStatement() {
    Node* node = identifierPath();
    if (lookForward() == TAssign) {
        const ValueType targetType = semanticTree.variableType(node);
        expect(TAssign);
        semanticTree.checkAssignment(targetType, expression());
        expect(TSemicolon);
    } else if (lookForward() == TLParen) {
        semanticTree.callType(node);
        expect(TLParen);
        expect(TRParen);
        expect(TSemicolon);
    } else {
        unexpected(sc, "= или () после имени");
    }
}

Node* TDiagram::identifierPath() {
    Node* node = semanticTree.requireName(consume(TIdent));
    while (lookForward() == TDot) {
        expect(TDot);
        node = semanticTree.requireMember(node, consume(TIdent));
    }
    return node;
}

// Вход в иерархию приоритетов. Самый внешний уровень -- логическое ИЛИ.
ValueType TDiagram::expression() {
    return logicalOr();
}

// Сначала разбирает левое логическое И, затем повторяет пары «|| И».
// Цикл заменяет левую рекурсию исходной КС-грамматики.
ValueType TDiagram::logicalOr() {
    ValueType result = logicalAnd();
    while (lookForward() == TOr) {
        const int operation = lookForward();
        expect(operation);
        result = semanticTree.binaryType(operation, result, logicalAnd());
    }
    return result;
}

// Аналогично соединяет выражения равенства оператором &&.
ValueType TDiagram::logicalAnd() {
    ValueType result = equality();
    while (lookForward() == TAnd) {
        const int operation = lookForward();
        expect(operation);
        result = semanticTree.binaryType(operation, result, equality());
    }
    return result;
}

// Разбирает последовательность сравнений на равенство == и !=.
// Операнды передаются уровню relation(), у которого приоритет выше.
ValueType TDiagram::equality() {
    ValueType result = relation();
    while (lookForward() == TEq || lookForward() == TNeq) {
        const int operation = lookForward();
        expect(operation);
        result = semanticTree.binaryType(operation, result, relation());
    }
    return result;
}

// Повторяет операторы <, <=, >, >= между арифметическими суммами.
ValueType TDiagram::relation() {
    ValueType result = addition();
    while (lookForward() == TLt || lookForward() == TLe ||
           lookForward() == TGt || lookForward() == TGe) {
        const int operation = lookForward();
        expect(operation);
        result = semanticTree.binaryType(operation, result, addition());
    }
    return result;
}

// Разбирает + и - после каждого произведения, соблюдая приоритет умножения.
ValueType TDiagram::addition() {
    ValueType result = multiplication();
    while (lookForward() == TPlus || lookForward() == TMinus) {
        const int operation = lookForward();
        expect(operation);
        result = semanticTree.binaryType(operation, result, multiplication());
    }
    return result;
}

// Разбирает *, /, % между унарными выражениями.
ValueType TDiagram::multiplication() {
    ValueType result = unary();
    while (lookForward() == TMult || lookForward() == TDiv ||
           lookForward() == TMod) {
        const int operation = lookForward();
        expect(operation);
        result = semanticTree.binaryType(operation, result, unary());
    }
    return result;
}

// Поглощает все префиксы !, +, - перед первичным выражением.
// Например, в !-x цикл прочитает ! и -, а primary() -- x.
ValueType TDiagram::unary() {
    if (lookForward() == TNot || lookForward() == TPlus ||
        lookForward() == TMinus) {
        const int operation = lookForward();
        expect(operation);
        return semanticTree.unaryType(operation, unary());
    }
    return primary();
}

// Развилка для константы, выражения в скобках и операнда-идентификатора.
// Скобки вызывают expression() рекурсивно и завершаются обязательной ')'.
ValueType TDiagram::primary() {
    const int first = lookForward();
    if (first == TConsInt) {
        expect(TConsInt);
        return {DataType::Int};
    } else if (first == TConsBool) {
        expect(TConsBool);
        return {DataType::Bool};
    } else if (first == TLParen) {
        expect(TLParen);
        const ValueType result = expression();
        expect(TRParen);
        return result;
    } else if (first == TIdent) {
        return operand();
    } else {
        unexpected(sc, "целая/логическая константа, ident или (выражение)");
    }
}


ValueType TDiagram::operand() {
    Node* node = identifierPath();
    if (lookForward() == TLParen) {
        expect(TLParen);
        expect(TRParen);
        return semanticTree.callType(node);
    }
    return semanticTree.variableType(node);
}


void TDiagram::printSemanticTree(std::ostream& out) const {
    semanticTree.printTree(out);
}
