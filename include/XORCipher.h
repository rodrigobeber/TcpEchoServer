#pragma once

#include <cstdint>
#include <vector>
#include <string>

#define USE_XOR_CIPHER // Comment this line to disable decryption

class XORCipher {
private:
    uint32_t partialInitialKey{0};
    uint32_t calculateChecksum(const std::string& input);
    uint32_t nextKey(uint32_t key);
public:
    void calculatePartialInitialKey(const std::string& username, const std::string& password);
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& ciphertext, unsigned messageSequence);
    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext, unsigned messageSequence);
};