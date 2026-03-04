#pragma once

#include <cstdint>

namespace mc
{

    struct XoroshiroSeed
    {
        uint64_t lo, hi;
    };

    namespace RandomSeed
    {

        inline uint64_t nextSplitMix64(uint64_t l)
        {
            l = (l ^ (l >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
            l = (l ^ (l >> 27)) * UINT64_C(0x94D049BB133111EB);
            return l ^ (l >> 31);
        }

        inline XoroshiroSeed createXoroshiroSeed(uint64_t seed)
        {
            uint64_t m = seed ^ UINT64_C(0x6A09E667F3BCC909);
            uint64_t n = m + UINT64_C(0x9E3779B97F4A7C15);
            return {nextSplitMix64(m), nextSplitMix64(n)};
        }

    } // namespace RandomSeed

    struct Xoroshiro128PlusPlusImpl
    {
        uint64_t lo, hi;

        Xoroshiro128PlusPlusImpl(uint64_t l, uint64_t h) : lo(l), hi(h) {}
        explicit Xoroshiro128PlusPlusImpl(XoroshiroSeed s) : lo(s.lo), hi(s.hi) {}

        uint64_t next()
        {
            uint64_t s0 = lo;
            uint64_t s1 = hi;
            uint64_t result = rotl(s0 + s1, 17) + s0;
            s1 ^= s0;
            lo = rotl(s0, 49) ^ s1 ^ (s1 << 21);
            hi = rotl(s1, 28);
            return result;
        }

    private:
        static uint64_t rotl(uint64_t x, int k)
        {
            return (x << k) | (x >> (64 - k));
        }
    };

} // namespace mc