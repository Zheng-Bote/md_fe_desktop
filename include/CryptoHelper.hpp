#pragma once

#include <string>
#include <vector>

namespace md::crypto {

class CryptoHelper {
public:
    static std::vector<unsigned char> encryptAES256(const std::string& plaintext, const std::string& password);
    static std::string decryptAES256(const std::vector<unsigned char>& ciphertext, const std::string& password);
};

} // namespace md::crypto
