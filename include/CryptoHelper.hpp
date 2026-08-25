/**
 * SPDX-FileComment: Medical Devices Desktop Frontend
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file CryptoHelper.hpp
 * @brief Header for CryptoHelper.hpp
 * @version 1.0.0
 * @date 2026-08-25
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

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
