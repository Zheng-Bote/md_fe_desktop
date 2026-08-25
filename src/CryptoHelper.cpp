/**
 * SPDX-FileComment: Medical Devices Desktop Frontend
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file CryptoHelper.cpp
 * @brief Implementation of CryptoHelper.cpp
 * @version 1.0.0
 * @date 2026-08-25
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#include "CryptoHelper.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdexcept>
#include <cstring>

namespace md::crypto {

static const int SALT_SIZE = 8;
static const int KEY_SIZE = 32; // AES-256
static const int IV_SIZE = 16;
static const int ITERATIONS = 10000;

std::vector<unsigned char> CryptoHelper::encryptAES256(const std::string& plaintext, const std::string& password) {
    unsigned char salt[SALT_SIZE];
    if (RAND_bytes(salt, sizeof(salt)) != 1) {
        throw std::runtime_error("Failed to generate salt");
    }

    unsigned char key[KEY_SIZE];
    unsigned char iv[IV_SIZE];

    if (!PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                           salt, sizeof(salt), ITERATIONS,
                           EVP_sha256(), sizeof(key), key)) {
        throw std::runtime_error("PBKDF2 failed");
    }
    
    // For CBC we need an IV, we can just use another PBKDF2 derivation or simpler: 
    // Just derive key + IV using EVP_BytesToKey
    if (!EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha256(), salt, 
                        (const unsigned char*)password.c_str(), password.length(), 
                        ITERATIONS, key, iv)) {
        throw std::runtime_error("EVP_BytesToKey failed");
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("EVP_CIPHER_CTX_new failed");

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EVP_EncryptInit_ex failed");
    }

    std::vector<unsigned char> ciphertext(plaintext.length() + EVP_MAX_BLOCK_LENGTH);
    int len = 0;
    int ciphertext_len = 0;

    if (1 != EVP_EncryptUpdate(ctx, ciphertext.data(), &len, (const unsigned char*)plaintext.c_str(), plaintext.length())) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EVP_EncryptUpdate failed");
    }
    ciphertext_len = len;

    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EVP_EncryptFinal_ex failed");
    }
    ciphertext_len += len;
    ciphertext.resize(ciphertext_len);

    EVP_CIPHER_CTX_free(ctx);

    // Format: "Salted__" + salt + ciphertext
    std::vector<unsigned char> result;
    const char magic[] = "Salted__";
    result.insert(result.end(), magic, magic + 8);
    result.insert(result.end(), salt, salt + SALT_SIZE);
    result.insert(result.end(), ciphertext.begin(), ciphertext.end());

    return result;
}

std::string CryptoHelper::decryptAES256(const std::vector<unsigned char>& input, const std::string& password) {
    if (input.size() < 16) {
        throw std::runtime_error("Input too short");
    }

    const char magic[] = "Salted__";
    if (std::memcmp(input.data(), magic, 8) != 0) {
        throw std::runtime_error("Invalid magic header");
    }

    unsigned char salt[SALT_SIZE];
    std::memcpy(salt, input.data() + 8, SALT_SIZE);

    unsigned char key[KEY_SIZE];
    unsigned char iv[IV_SIZE];

    if (!EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha256(), salt, 
                        (const unsigned char*)password.c_str(), password.length(), 
                        ITERATIONS, key, iv)) {
        throw std::runtime_error("EVP_BytesToKey failed");
    }

    const unsigned char* ciphertext = input.data() + 16;
    int ciphertext_len = input.size() - 16;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("EVP_CIPHER_CTX_new failed");

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EVP_DecryptInit_ex failed");
    }

    std::vector<unsigned char> plaintext(ciphertext_len + EVP_MAX_BLOCK_LENGTH);
    int len = 0;
    int plaintext_len = 0;

    if (1 != EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext, ciphertext_len)) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EVP_DecryptUpdate failed");
    }
    plaintext_len = len;

    if (1 != EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EVP_DecryptFinal_ex failed (wrong password?)");
    }
    plaintext_len += len;
    plaintext.resize(plaintext_len);

    EVP_CIPHER_CTX_free(ctx);

    return std::string(plaintext.begin(), plaintext.end());
}

} // namespace md::crypto
