#include "SemTreeControl.h"

void SemTreeControl::checkDuplicate(const std::string& id) const {
    if (FindUpOneLevel(id) != nullptr)
        sc->semanticError("повторное объявление '" + id + "' в одной области");
}

Node* SemTreeControl::declareClass(const std::string& id) {
    checkDuplicate(id);
    Node* node = SetLeft(id, ObjectType::ClassType, {DataType::Class});
    node->type.classType = node;
    return node;
}

Node* SemTreeControl::declareVariable(const std::string& id, ValueType type) {
    checkDuplicate(id);
    if (type.kind == DataType::Class && !type.classType->complete)
        sc->semanticError("нельзя создать объект незавершённого класса '" + type.classType->id + "'");
    const ObjectType kind = type.kind == DataType::Class
        ? ObjectType::ClassData : ObjectType::Variable;
    Node* node = SetLeft(id, kind, type);
    if (kind == ObjectType::ClassData) copyMembers(type.classType, node);
    return node;
}

Node* SemTreeControl::declareFunction(const std::string& id, ValueType resultType) {
    checkDuplicate(id);
    return SetLeft(id, ObjectType::Function, resultType);
}

Node* SemTreeControl::requireName(const std::string& id) const {
    Node* node = FindUp(id);
    if (node == nullptr) sc->semanticError("имя '" + id + "' не объявлено или недоступно");
    return node;
}

ValueType SemTreeControl::namedType(const std::string& id) const {
    Node* node = requireName(id);
    if (node->objectType != ObjectType::ClassType)
        sc->semanticError("'" + id + "' не является именем класса");
    return node->type;
}

Node* SemTreeControl::requireMember(Node* object, const std::string& id) const {
    if (object->objectType != ObjectType::ClassData)
        sc->semanticError("слева от точки требуется объект класса: '" + object->id + "'");
    Node* member = FindRightDown(object, id);
    if (member == nullptr)
        sc->semanticError("в классе '" + object->type.classType->id + "' нет члена '" + id + "'");
    return member;
}

ValueType SemTreeControl::variableType(Node* node) const {
    if (node->objectType != ObjectType::Variable && node->objectType != ObjectType::ClassData)
        sc->semanticError("'" + node->id + "' не является переменной или полем");
    return node->type;
}

ValueType SemTreeControl::callType(Node* node) const {
    if (node->objectType != ObjectType::Function)
        sc->semanticError("'" + node->id + "' не является методом");
    return node->type;
}

bool SemTreeControl::isScalar(ValueType type) {
    return type.kind == DataType::Int || type.kind == DataType::Bool ||
           type.kind == DataType::Double;
}

ValueType SemTreeControl::unaryType(int operation, ValueType operand) const {
    if (operand.kind == DataType::Unknown) return {};
    if (!isScalar(operand)) sc->semanticError("унарная операция недопустима для класса");
    if (operation == TNot) return {DataType::Bool};
    // В C++ унарные + и - преобразуют bool в int.
    return {operand.kind == DataType::Double ? DataType::Double : DataType::Int};
}

ValueType SemTreeControl::binaryType(int operation, ValueType left, ValueType right) const {
    if (left.kind == DataType::Unknown || right.kind == DataType::Unknown) return {};
    if (!isScalar(left) || !isScalar(right))
        sc->semanticError("операция " + std::string(tokenTypeName(operation)) +
                          " недопустима для " + typeName(left) + " и " + typeName(right));

    // логика/сравнение -> bool; арифметика -> double при наличии double,
    // иначе -> int (в том числе bool + bool). % допускает только целые типы.
    if (operation == TAnd || operation == TOr || operation == TEq || operation == TNeq ||
        operation == TLt || operation == TLe || operation == TGt || operation == TGe)
        return {DataType::Bool};
    if (operation == TMod && (left.kind == DataType::Double || right.kind == DataType::Double))
        sc->semanticError("операция % не допускает double");
    if (left.kind == DataType::Double || right.kind == DataType::Double)
        return {DataType::Double};
    return {DataType::Int};
}

void SemTreeControl::checkAssignment(ValueType target, ValueType source) const {
    if (target.kind == DataType::Unknown || source.kind == DataType::Unknown) return;
    // Обычное присваивание C++ допускает преобразования между всеми тремя
    // простыми типами, включая double -> int и числовой тип -> bool.
    if (isScalar(target) && isScalar(source)) return;
    if (target.kind == DataType::Class && source.kind == DataType::Class &&
        target.classType == source.classType) return;
    sc->semanticError("нельзя присвоить " + typeName(source) + " значению типа " + typeName(target));
}

void SemTreeControl::checkCondition(ValueType type) const {
    if (type.kind != DataType::Unknown && !isScalar(type))
        sc->semanticError("условие for должно приводиться к bool, получено " + typeName(type));
}

void SemTreeControl::checkReturn(Node* function, ValueType type) const {
    if (function == nullptr) sc->semanticError("return допустим только внутри метода или main");
    checkAssignment(function->type, type);
}

void SemTreeControl::checkFunctionEnd(Node* function, bool returnsValue) const {
    // main в C++ может закончиться без return: подразумевается return 0.
    if (function->id != "main" && !returnsValue)
        sc->semanticError("метод '" + function->id + "' должен возвращать значение через return");
}
