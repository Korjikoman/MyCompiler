#ifndef LAB4_SEM_TREE_CONTROL_H
#define LAB4_SEM_TREE_CONTROL_H

#include "SemTreeBase.h"
#include "../LexicalAnalyzer/Scaner.h"

class SemTreeControl : public SemTreeBase {
private:
    TScaner* sc;
    void checkDuplicate(const std::string& id) const;
    static bool isScalar(ValueType type);

public:
    explicit SemTreeControl(TScaner* scanner) : sc(scanner) {}

    Node* declareClass(const std::string& id);
    Node* declareVariable(const std::string& id, ValueType type);
    Node* declareFunction(const std::string& id, ValueType resultType);
    ValueType namedType(const std::string& id) const;
    Node* requireName(const std::string& id) const;
    Node* requireMember(Node* object, const std::string& id) const;
    ValueType variableType(Node* node) const;
    ValueType callType(Node* node) const;

    ValueType unaryType(int operation, ValueType operand) const;
    ValueType binaryType(int operation, ValueType left, ValueType right) const;
    void checkAssignment(ValueType target, ValueType source) const;
    void checkCondition(ValueType type) const;
    void checkReturn(Node* function, ValueType type) const;
    void checkFunctionEnd(Node* function, bool returnsValue) const;
};

#endif
