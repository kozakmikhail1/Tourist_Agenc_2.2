#ifndef LOGINHASHTABLE_H
#define LOGINHASHTABLE_H

#include <QString>

namespace agency {

struct User;

/** Простая хеш-таблица (цепочки) для проверки уникальности логина при регистрации. */
class LoginHashTable {
public:
    static constexpr int kBucketCount = 257;

    LoginHashTable();
    ~LoginHashTable();

    LoginHashTable(const LoginHashTable&) = delete;
    LoginHashTable& operator=(const LoginHashTable&) = delete;

    void clear();
    void insert(const QString& login);
    void remove(const QString& login);
    bool contains(const QString& login) const;

    void rebuildFromUserList(const User* head);

private:
    struct Node {
        QString login;
        Node* next = nullptr;
    };

    int hashFunc(const QString& s) const;

    Node* buckets_[kBucketCount]{};
};

} // namespace agency

#endif
