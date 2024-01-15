#include <iostream>
#include <iomanip>
#include "XORCipher.h"

void XORCipher::calculatePartialInitialKey(const std::string& username, const std::string& password) {
    partialInitialKey = (calculateChecksum(username) << 8) | calculateChecksum(password);
}

uint32_t XORCipher::calculateChecksum(const std::string& input) {
    uint32_t sum = 0;
    for (uint8_t ch : input) {
        sum += static_cast<uint32_t>(ch);
    }
    return sum & 0xFF;
}

uint32_t XORCipher::nextKey(uint32_t key) {
    return (key * 1103515245 + 12345) % 0x7FFFFFFF;
}

std::vector<uint8_t> XORCipher::decrypt(const std::vector<uint8_t>& ciphertext, unsigned messageSequence) {
    std::vector<uint8_t> plaintext(ciphertext.size());
    uint32_t initialKey = (messageSequence << 16) | partialInitialKey;

    uint32_t currentKey = nextKey(initialKey);
    uint8_t cipherKey = currentKey & 0xFF; // cipher_key[0]

    // Print the cipher key even if ciphertext is empty
    for (size_t i = 0; i < ciphertext.size(); ++i) {
        char decryptedChar = ciphertext[i] ^ cipherKey;
        plaintext[i] = decryptedChar;
        currentKey = nextKey(currentKey);
        cipherKey = currentKey & 0xFF; // cipher_key[i+1]
    }

    return plaintext;
}

// I used thos one for a client test program
std::vector<uint8_t> XORCipher::encrypt(const std::vector<uint8_t>& plaintext, unsigned messageSequence) {
    std::vector<uint8_t> ciphertext;
    uint32_t initialKey = (messageSequence << 16) | partialInitialKey;

    uint32_t currentKey = nextKey(initialKey);
    uint8_t cipherKey = currentKey & 0xFF; // cipher_key[0]

    // Print the cipher key even if plaintext is empty
    for (size_t i = 0; i < plaintext.size(); ++i) {
        uint8_t encryptedChar = plaintext[i] ^ cipherKey;
        ciphertext.push_back(encryptedChar);
        currentKey = nextKey(currentKey);
        cipherKey = currentKey & 0xFF; // cipher_key[i+1]
    }

    return ciphertext;
}