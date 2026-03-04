#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <array>
#include <vector>
#include <sstream>
#include <iomanip>

class MD5
{
public:
    MD5();

    void update(const std::string &input);
    void update(const uint8_t *input, size_t length);
    void finalize();

    std::array<uint8_t, 16> digest() const { return digest_; }
    std::string hexdigest() const;

    static std::array<uint8_t, 16> hash(const std::string &input);
    static std::string hexhash(const std::string &input);

private:
    static constexpr uint32_t A = 0x67452301;
    static constexpr uint32_t B = 0xefcdab89;
    static constexpr uint32_t C = 0x98badcfe;
    static constexpr uint32_t D = 0x10325476;

    static const std::array<uint32_t, 64> S;
    static const std::array<uint32_t, 64> K;
    static const std::array<uint8_t, 64> PADDING;

    static uint32_t F(uint32_t x, uint32_t y, uint32_t z) { return (x & y) | (~x & z); }
    static uint32_t G(uint32_t x, uint32_t y, uint32_t z) { return (x & z) | (y & ~z); }
    static uint32_t H(uint32_t x, uint32_t y, uint32_t z) { return x ^ y ^ z; }
    static uint32_t I(uint32_t x, uint32_t y, uint32_t z) { return y ^ (x | ~z); }

    static uint32_t rotateLeft(uint32_t x, uint32_t n) { return (x << n) | (x >> (32 - n)); }

    void step(const uint32_t *input);

    uint64_t size_ = 0;
    std::array<uint32_t, 4> buffer_ = {A, B, C, D};
    std::array<uint8_t, 64> input_ = {};
    std::array<uint8_t, 16> digest_ = {};
    bool finalized_ = false;
};