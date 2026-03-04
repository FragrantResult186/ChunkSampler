#include "abstract_random.hpp"

namespace mc
{

    namespace detail
    {
        inline void md5(const uint8_t *msg, size_t len, uint8_t out[16])
        {
            MD5 ctx;
            ctx.update(msg, len);
            ctx.finalize();
            auto d = ctx.digest();
            std::copy(d.begin(), d.end(), out);
        }

        inline uint64_t leU64(const uint8_t *b)
        {
            uint64_t v = 0;
            for (int i = 0; i < 8; ++i)
                v |= (uint64_t)b[i] << (i * 8);
            return v;
        }

        inline uint64_t beU64(const uint8_t *b)
        {
            uint64_t v = 0;
            for (int i = 0; i < 8; ++i)
                v = (v << 8) | b[i];
            return v;
        }
    } // namespace detail

    // ---- Xoroshiro128PlusPlusRandom -------------------------------------------------------

    Xoroshiro128PlusPlusRandom::Xoroshiro128PlusPlusRandom(uint64_t seed)
        : impl_(RandomSeed::createXoroshiroSeed(seed))
    {
    }

    Xoroshiro128PlusPlusRandom::Xoroshiro128PlusPlusRandom(uint64_t lo, uint64_t hi)
        : impl_(lo, hi)
    {
    }

    int Xoroshiro128PlusPlusRandom::nextInt()
    {
        return (int)(impl_.next() & 0xFFFFFFFFULL);
    }

    int Xoroshiro128PlusPlusRandom::nextInt(int bound)
    {
        if (bound <= 0)
            return 0;

        uint64_t l = (uint32_t)nextInt();
        uint64_t m = l * (uint64_t)bound;
        uint64_t n = m & 0xFFFFFFFFULL;

        if (n < (uint64_t)bound)
        {
            uint64_t threshold = ((uint64_t)(~(uint32_t)bound + 1)) % (uint32_t)bound;
            while (n < threshold)
            {
                l = (uint32_t)nextInt();
                m = l * (uint64_t)bound;
                n = m & 0xFFFFFFFFULL;
            }
        }
        return (int)(m >> 32);
    }

    double Xoroshiro128PlusPlusRandom::nextDouble()
    {
        return (double)(impl_.next() >> 11) * 1.1102230246251565e-16;
    }

    uint64_t Xoroshiro128PlusPlusRandom::nextRaw()
    {
        return impl_.next();
    }

    std::unique_ptr<RandomDeriver> Xoroshiro128PlusPlusRandom::createRandomDeriver()
    {
        uint64_t lo = nextRaw();
        uint64_t hi = nextRaw();
        return std::make_unique<XoroshiroRandomDeriver>(lo, hi);
    }

    // ---- XoroshiroRandomDeriver -----------------------------------------------------------

    XoroshiroRandomDeriver::XoroshiroRandomDeriver(uint64_t lo, uint64_t hi)
        : seedLo_(lo), seedHi_(hi)
    {
    }

    std::unique_ptr<AbstractRandom> XoroshiroRandomDeriver::createRandom(int x, int y, int z)
    {
        int64_t h = MathHelper::hashCode(x, y, z);
        return std::make_unique<Xoroshiro128PlusPlusRandom>((uint64_t)(h ^ (int64_t)seedLo_), seedHi_);
    }

    std::unique_ptr<AbstractRandom> XoroshiroRandomDeriver::createRandom(const std::string &key)
    {
        uint8_t digest[16];
        detail::md5((const uint8_t *)key.data(), key.size(), digest);
        uint64_t lo = detail::beU64(digest);
        uint64_t hi = detail::beU64(digest + 8);
        return std::make_unique<Xoroshiro128PlusPlusRandom>(lo ^ seedLo_, hi ^ seedHi_);
    }

} // namespace mc