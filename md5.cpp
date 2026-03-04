#include "md5.hpp"
#include <stdexcept>

const std::array<uint32_t, 64> MD5::S = {
    7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
    5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};

const std::array<uint32_t, 64> MD5::K = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
    0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
    0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
    0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
    0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
    0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
    0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
    0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
    0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};

const std::array<uint8_t, 64> MD5::PADDING = {
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

MD5::MD5()
{
    buffer_ = {A, B, C, D};
}

void MD5::update(const std::string &input)
{
    update(reinterpret_cast<const uint8_t *>(input.data()), input.size());
}

void MD5::update(const uint8_t *input_buffer, size_t input_len)
{
    if (finalized_)
    {
        throw std::logic_error("MD5: cannot update after finalize() has been called");
    }

    uint32_t block[16];
    unsigned int offset = size_ % 64;
    size_ += input_len;

    for (size_t i = 0; i < input_len; ++i)
    {
        input_[offset++] = input_buffer[i];

        if (offset % 64 == 0)
        {
            for (unsigned int j = 0; j < 16; ++j)
            {
                block[j] = static_cast<uint32_t>(input_[(j * 4) + 3]) << 24 |
                           static_cast<uint32_t>(input_[(j * 4) + 2]) << 16 |
                           static_cast<uint32_t>(input_[(j * 4) + 1]) << 8 |
                           static_cast<uint32_t>(input_[(j * 4)]);
            }
            step(block);
            offset = 0;
        }
    }
}

void MD5::finalize()
{
    if (finalized_)
        return;

    uint32_t block[16];
    unsigned int offset = size_ % 64;
    unsigned int padding_length = (offset < 56) ? (56 - offset) : (56 + 64 - offset);

    update(PADDING.data(), padding_length);
    size_ -= padding_length;

    for (unsigned int j = 0; j < 14; ++j)
    {
        block[j] = static_cast<uint32_t>(input_[(j * 4) + 3]) << 24 |
                   static_cast<uint32_t>(input_[(j * 4) + 2]) << 16 |
                   static_cast<uint32_t>(input_[(j * 4) + 1]) << 8 |
                   static_cast<uint32_t>(input_[(j * 4)]);
    }
    block[14] = static_cast<uint32_t>(size_ * 8);
    block[15] = static_cast<uint32_t>((size_ * 8) >> 32);

    step(block);

    for (unsigned int i = 0; i < 4; ++i)
    {
        digest_[(i * 4) + 0] = static_cast<uint8_t>(buffer_[i] & 0xFF);
        digest_[(i * 4) + 1] = static_cast<uint8_t>((buffer_[i] >> 8) & 0xFF);
        digest_[(i * 4) + 2] = static_cast<uint8_t>((buffer_[i] >> 16) & 0xFF);
        digest_[(i * 4) + 3] = static_cast<uint8_t>((buffer_[i] >> 24) & 0xFF);
    }

    finalized_ = true;
}

void MD5::step(const uint32_t *input)
{
    uint32_t AA = buffer_[0];
    uint32_t BB = buffer_[1];
    uint32_t CC = buffer_[2];
    uint32_t DD = buffer_[3];

    for (unsigned int i = 0; i < 64; ++i)
    {
        uint32_t E;
        unsigned int j;

        switch (i / 16)
        {
        case 0:
            E = F(BB, CC, DD);
            j = i;
            break;
        case 1:
            E = G(BB, CC, DD);
            j = (i * 5 + 1) % 16;
            break;
        case 2:
            E = H(BB, CC, DD);
            j = (i * 3 + 5) % 16;
            break;
        default:
            E = I(BB, CC, DD);
            j = (i * 7) % 16;
            break;
        }

        uint32_t temp = DD;
        DD = CC;
        CC = BB;
        BB = BB + rotateLeft(AA + E + K[i] + input[j], S[i]);
        AA = temp;
    }

    buffer_[0] += AA;
    buffer_[1] += BB;
    buffer_[2] += CC;
    buffer_[3] += DD;
}

std::string MD5::hexdigest() const
{
    if (!finalized_)
    {
        throw std::logic_error("MD5: finalize() must be called before hexdigest()");
    }
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint8_t byte : digest_)
    {
        oss << std::setw(2) << static_cast<unsigned int>(byte);
    }
    return oss.str();
}

std::array<uint8_t, 16> MD5::hash(const std::string &input)
{
    MD5 ctx;
    ctx.update(input);
    ctx.finalize();
    return ctx.digest();
}

std::string MD5::hexhash(const std::string &input)
{
    MD5 ctx;
    ctx.update(input);
    ctx.finalize();
    return ctx.hexdigest();
}