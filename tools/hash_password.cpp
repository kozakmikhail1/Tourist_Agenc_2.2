// Сборка: g++ -std=c++17 -O2 hash_password.cpp -o hash_password
// Использование: hash_password.exe admin admin
#include <iostream>
#include <string>

// Должен совпадать с agency::hashPassword для ASCII (латиница/цифры/«_»).
static unsigned int hashPassword(const std::string& login, const std::string& password)
{
    const std::string combined = login + password;
    unsigned int h = 5381U;
    constexpr unsigned int kPrime = 131U;
    for (unsigned char ch : combined) {
        h = (h * kPrime) ^ static_cast<unsigned int>(ch);
    }
    return h;
}

int main(int argc, char* argv[])
{
    if (argc < 3) {
        std::cerr << "usage: hash_password <login> <password>\n";
        return 1;
    }
    std::cout << hashPassword(argv[1], argv[2]) << '\n';
    return 0;
}
