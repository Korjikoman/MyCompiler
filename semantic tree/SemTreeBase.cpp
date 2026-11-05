#include "SemTreeBase.h"

#include <ostream>

SemTreeBase::SemTreeBase() : root(new Node), current(root) {}

SemTreeBase::~SemTreeBase() {
    deleteTree(root);
}

void SemTreeBase::deleteTree(Node* node) {
    if (node == nullptr) return;
    deleteTree(node->left);
    deleteTree(node->right);
    // up и type.classType -- ссылки, а не отдельные принадлежащие узлу объекты.
    delete node;
}

Node* SemTreeBase::SetLeft(const std::string& id, ObjectType objectType,
                           ValueType type) {
    Node* node = new Node;
    node->id = id;
    node->objectType = objectType;
    node->type = type;
    node->up = current;
    current->left = node;
    current = node;
    return node;
}

Node* SemTreeBase::SetBlackLeft() {
    return SetLeft("", ObjectType::Empty, {});
}

Node* SemTreeBase::SetBlackRight() {
    Node* node = new Node;
    node->up = current;
    current->right = node;
    current = node;
    return node;
}

void SemTreeBase::enterScope(Node* owner) {
    current = owner;
    SetBlackRight();
}

Node* SemTreeBase::enterBlock() {
    Node* boundary = SetBlackLeft();
    SetBlackRight();
    return boundary;
}

Node* SemTreeBase::FindUpOneLevel(const std::string& id) const {
    for (Node* node = current; node != nullptr; node = node->up) {
        if (node->objectType != ObjectType::Empty && node->id == id) return node;
        // Правый переход означает границу области. Выше искать повторы нельзя.
        if (node->up != nullptr && node->up->right == node) break;
    }
    return nullptr;
}

Node* SemTreeBase::FindRightDown(Node* owner, const std::string& id) const {
    // Идём только по цепочке членов. В локальные данные методов не заходим.
    for (Node* node = owner->right; node != nullptr; node = node->left) {
        if (node->objectType != ObjectType::Empty && node->id == id) return node;
    }
    return nullptr;
}

Node* SemTreeBase::FindUp(const std::string& id) const {
    for (Node* node = current; node != nullptr; node = node->up) {
        // Тела методов разбираются после всех заголовков класса.
        // Поэтому внутри метода видны и поля/методы, записанные ниже него.
        if (node->objectType == ObjectType::Empty && node->up != nullptr &&
            node->up->objectType == ObjectType::ClassType && node->up->complete) {
            Node* member = FindRightDown(node->up, id);
            if (member != nullptr) return member;
        }
        if (node->objectType != ObjectType::Empty && node->id == id) return node;
    }
    return nullptr;
}

void SemTreeBase::copyMembers(const Node* source, Node* object) {
    object->right = new Node;
    object->right->up = object;
    Node* tail = object->right;
    for (const Node* member = source->right; member != nullptr; member = member->left) {
        if (member->objectType == ObjectType::Empty) continue;
        Node* copy = new Node;
        copy->id = member->id;
        copy->objectType = member->objectType;
        copy->type = member->type;
        copy->up = tail;
        tail->left = copy;
        tail = copy;
        if (member->objectType == ObjectType::ClassData) copyMembers(member, copy);
        // Тело метода хранится у объявления класса, копируется только заголовок.
    }
}

std::string SemTreeBase::typeName(ValueType type) {
    switch (type.kind) {
        case DataType::Int: return "int";
        case DataType::Bool: return "bool";
        case DataType::Double: return "double";
        case DataType::Class: return type.classType == nullptr ? "class<?>" : type.classType->id;
        default: return "unknown";
    }
}

void SemTreeBase::printLevel(std::ostream& out, const Node* node, int depth) {
    for (; node != nullptr; node = node->left) {
        out << std::string(depth * 2, ' ');
        switch (node->objectType) {
            case ObjectType::Empty:
                if (node->up == nullptr) {
                    out << "[глобальная область]";
                } else if (node->up->right == node) {
                    if (node->up->objectType == ObjectType::ClassType) {
                        out << "[члены класса " << node->up->id << ']';
                    } else if (node->up->objectType == ObjectType::ClassData) {
                        out << "[члены объекта " << node->up->id << ']';
                    } else if (node->up->objectType == ObjectType::Function) {
                        out << "[тело " << node->up->id << "()]";
                    } else {
                        out << "[локальная область]";
                    }
                } else if (node->right != nullptr) {
                    out << "[вложенный блок]";
                } else {
                    out << "[граница области]";
                }
                break;
            case ObjectType::ClassType: out << "class " << node->id; break;
            case ObjectType::Function:
                out << "метод/функция " << node->id << "() : " << typeName(node->type);
                break;
            default: out << node->id << " : " << typeName(node->type); break;
        }
        out << '\n';
        if (node->right != nullptr) printLevel(out, node->right, depth + 1);
    }
}

void SemTreeBase::printTree(std::ostream& out) const {
    printLevel(out, root, 0);
}
