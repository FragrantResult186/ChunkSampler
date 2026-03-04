#pragma once

#include "xoroshiro.hpp"
#include "math_helper.hpp"
#include "md5.hpp"
#include <cstdint>
#include <string>
#include <memory>
#include <cstring>

namespace mc
{

    class AbstractRandom;
    class RandomDeriver;

    class AbstractRandom
    {
    public:
        virtual ~AbstractRandom() = default;
        virtual std::unique_ptr<RandomDeriver> createRandomDeriver() = 0;
        virtual int nextInt() = 0;
        virtual int nextInt(int bound) = 0;
        virtual double nextDouble() = 0;
    };

    class RandomDeriver
    {
    public:
        virtual ~RandomDeriver() = default;
        virtual std::unique_ptr<AbstractRandom> createRandom(const std::string &key) = 0;
        virtual std::unique_ptr<AbstractRandom> createRandom(int x, int y, int z) = 0;
    };

    class Xoroshiro128PlusPlusRandom : public AbstractRandom
    {
    public:
        explicit Xoroshiro128PlusPlusRandom(uint64_t seed);
        Xoroshiro128PlusPlusRandom(uint64_t lo, uint64_t hi);

        std::unique_ptr<RandomDeriver> createRandomDeriver() override;

        int nextInt() override;
        int nextInt(int bound) override;
        double nextDouble() override;
        uint64_t nextRaw();

    private:
        Xoroshiro128PlusPlusImpl impl_;
    };

    class XoroshiroRandomDeriver : public RandomDeriver
    {
    public:
        XoroshiroRandomDeriver(uint64_t lo, uint64_t hi);

        std::unique_ptr<AbstractRandom> createRandom(int x, int y, int z) override;
        std::unique_ptr<AbstractRandom> createRandom(const std::string &key) override;

    private:
        uint64_t seedLo_, seedHi_;
    };

} // namespace mc