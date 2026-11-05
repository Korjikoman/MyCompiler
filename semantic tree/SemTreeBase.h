#ifndef LAB4_SEM_TREE_BASE_H
#define LAB4_SEM_TREE_BASE_H

#include <iosfwd>
#include <string>

struct Node;

enum class ObjectType { Empty, Variable, Function, ClassType, ClassData };
enum class DataType { Unknown, Int, Bool, Double, Class };

struct ValueType {
    DataType kind = DataType::Unknown;
    Node* classType = nullptr; // для Class: ссылка на объявление конкретного класса
};

struct Node {
    std::string id;
    ObjectType objectType = ObjectType::Empty;
    ValueType type;
    bool complete = false; // все поля и заголовки методов класса уже прочитаны

    Node* up = nullptr;    // родитель
    Node* left = nullptr;  // следующее объявление В ТОЙ ЖЕ области
    Node* right = nullptr; // начало ВЛОЖЕННОЙ области
};

// Здесь нет проверок присваивания, операций и других правил языка.
class SemTreeBase {
protected:
    Node* root;
    Node* current;

    static void deleteTree(Node* node);
    static void printLevel(std::ostream& out, const Node* node, int depth);

public:
    SemTreeBase();
    virtual ~SemTreeBase();
    SemTreeBase(const SemTreeBase&) = delete;
    SemTreeBase& operator=(const SemTreeBase&) = delete;

    Node* GetCurrentTree() const { return current; }
    void SetCurrentTree(Node* node) { current = node; }

    Node* SetLeft(const std::string& id, ObjectType objectType, ValueType type);
    Node* SetBlackLeft();  // пустой служебный узел -- граница блока
    Node* SetBlackRight(); // пустой служебный узел -- вход в область

    Node* FindUp(const std::string& id) const;
    Node* FindUpOneLevel(const std::string& id) const;
    Node* FindRightDown(Node* owner, const std::string& id) const;

    // Сохранить возвращённый узел; при выходе восстановить current на него.
    Node* enterBlock();
    void enterScope(Node* owner);

    void copyMembers(const Node* source, Node* object);
    void printTree(std::ostream& out) const;
    static std::string typeName(ValueType type);
};

#endif
