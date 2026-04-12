#include "agency/loginhashtable.h"
#include "agency/agencytypes.h"

namespace agency {

LoginHashTable::LoginHashTable() = default;

LoginHashTable::~LoginHashTable()
{
    clear();
}

void LoginHashTable::clear()
{
    for (int i = 0; i < kBucketCount; ++i) {
        Node* n = buckets_[i];
        while (n) {
            Node* nx = n->next;
            delete n;
            n = nx;
        }
        buckets_[i] = nullptr;
    }
}

int LoginHashTable::hashFunc(const QString& s) const
{
    unsigned h = 5381;
    const QChar* p = s.constData();
    const int len = s.size();
    for (int i = 0; i < len; ++i) {
        h = ((h << 5) + h) + static_cast<unsigned>(p[i].unicode());
    }
    return static_cast<int>(h % kBucketCount);
}

void LoginHashTable::insert(const QString& login)
{
    const int b = hashFunc(login);
    Node* n = new Node{login, buckets_[b]};
    buckets_[b] = n;
}

void LoginHashTable::remove(const QString& login)
{
    const int b = hashFunc(login);
    Node** pp = &buckets_[b];
    while (*pp) {
        if ((*pp)->login == login) {
            Node* dead = *pp;
            *pp = dead->next;
            delete dead;
            return;
        }
        pp = &(*pp)->next;
    }
}

bool LoginHashTable::contains(const QString& login) const
{
    const int b = hashFunc(login);
    for (Node* n = buckets_[b]; n; n = n->next) {
        if (n->login == login) {
            return true;
        }
    }
    return false;
}

void LoginHashTable::rebuildFromUserList(const User* head)
{
    clear();
    for (const User* u = head; u; u = u->next) {
        insert(u->login);
    }
}

} // namespace agency
