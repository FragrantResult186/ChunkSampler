#pragma once

#include "math_helper.hpp"
#include <cstdint>
#include <string>
#include <cstdio>
#include <algorithm>

namespace mc
{

    struct Vec3i
    {
        int x, y, z;

        Vec3i() : x(0), y(0), z(0) {}
        Vec3i(int x, int y, int z) : x(x), y(y), z(z) {}
        Vec3i(double dx, double dy, double dz)
            : x(MathHelper::floor(dx)), y(MathHelper::floor(dy)), z(MathHelper::floor(dz)) {}

        bool operator==(const Vec3i &o) const { return x == o.x && y == o.y && z == o.z; }
        bool operator!=(const Vec3i &o) const { return !(*this == o); }

        bool operator<(const Vec3i &o) const
        {
            if (y != o.y)
                return y < o.y;
            if (z != o.z)
                return z < o.z;
            return x < o.x;
        }

        Vec3i add(int dx, int dy, int dz) const
        {
            if (dx == 0 && dy == 0 && dz == 0)
                return *this;
            return {x + dx, y + dy, z + dz};
        }
        Vec3i add(const Vec3i &v) const { return add(v.x, v.y, v.z); }
    };

    struct ChunkSectionPos
    {
        static int getSectionCoord(int blockCoord) { return blockCoord >> 4; }
        static int getBlockCoord(int sectionCoord) { return sectionCoord << 4; }
        static int getOffsetPos(int sectionCoord, int offset) { return getBlockCoord(sectionCoord) + offset; }
    };

    struct ChunkPos
    {
        int x, z;

        ChunkPos() : x(0), z(0) {}
        ChunkPos(int x, int z) : x(x), z(z) {}

        bool operator==(const ChunkPos &o) const { return x == o.x && z == o.z; }
        bool operator!=(const ChunkPos &o) const { return !(*this == o); }

        struct Hash
        {
            size_t operator()(const ChunkPos &p) const
            {
                uint32_t a = (uint32_t)(1664525 * p.x + 1013904223);
                uint32_t b = (uint32_t)(1664525 * (p.z ^ (int)0xDEAD10CC) + 1013904223);
                return (size_t)(a ^ b);
            }
        };

        uint64_t toLong() const { return toLong(x, z); }

        static uint64_t toLong(int x, int z)
        {
            return ((uint64_t)(uint32_t)x) | (((uint64_t)(uint32_t)z) << 32);
        }

        static int getPackedX(uint64_t l) { return (int)(l & 0xFFFFFFFFULL); }
        static int getPackedZ(uint64_t l) { return (int)((l >> 32) & 0xFFFFFFFFULL); }

        int getStartX() const { return ChunkSectionPos::getBlockCoord(x); }
        int getStartZ() const { return ChunkSectionPos::getBlockCoord(z); }
        int getEndX() const { return getOffsetX(15); }
        int getEndZ() const { return getOffsetZ(15); }
        int getCenterX() const { return getOffsetX(8); }
        int getCenterZ() const { return getOffsetZ(8); }

        int getOffsetX(int offset) const { return ChunkSectionPos::getOffsetPos(x, offset); }
        int getOffsetZ(int offset) const { return ChunkSectionPos::getOffsetPos(z, offset); }

        int getChebyshevDistance(const ChunkPos &o) const
        {
            return std::max(std::abs(x - o.x), std::abs(z - o.z));
        }

        std::string toString() const
        {
            char buf[64];
            snprintf(buf, sizeof(buf), "[%d, %d]", x, z);
            return buf;
        }
    };

    struct BlockPos : Vec3i
    {
        static const BlockPos ORIGIN;

        static constexpr int SIZE_BITS_X = 26;
        static constexpr int SIZE_BITS_Z = 26;
        static constexpr int SIZE_BITS_Y = 12;
        static constexpr uint64_t BITS_X = (1ULL << SIZE_BITS_X) - 1ULL;
        static constexpr uint64_t BITS_Y = (1ULL << SIZE_BITS_Y) - 1ULL;
        static constexpr uint64_t BITS_Z = (1ULL << SIZE_BITS_Z) - 1ULL;
        static constexpr int BIT_SHIFT_Z = SIZE_BITS_Y;
        static constexpr int BIT_SHIFT_X = SIZE_BITS_Y + SIZE_BITS_Z;

        BlockPos() : Vec3i(0, 0, 0) {}
        BlockPos(int x, int y, int z) : Vec3i(x, y, z) {}
        BlockPos(double dx, double dy, double dz) : Vec3i(dx, dy, dz) {}
        explicit BlockPos(const Vec3i &v) : Vec3i(v.x, v.y, v.z) {}

        BlockPos add(int dx, int dy, int dz) const
        {
            if (dx == 0 && dy == 0 && dz == 0)
                return *this;
            return {x + dx, y + dy, z + dz};
        }
        BlockPos add(const Vec3i &v) const { return add(v.x, v.y, v.z); }

        ChunkPos toChunkPos() const { return ChunkPos(x >> 4, z >> 4); }

        std::string toTP() const
        {
            char buf[64];
            snprintf(buf, sizeof(buf), "/tp %d %d %d", x, y, z);
            return buf;
        }

        static int64_t asLong(int x, int y, int z)
        {
            int64_t l = 0;
            l |= ((int64_t)(x & (int)BITS_X)) << BIT_SHIFT_X;
            l |= ((int64_t)(y & (int)BITS_Y)) << 0;
            l |= ((int64_t)(z & (int)BITS_Z)) << BIT_SHIFT_Z;
            return l;
        }

        static int unpackLongX(int64_t l) { return (int)(l << 64 - BIT_SHIFT_X - SIZE_BITS_X >> 64 - SIZE_BITS_X); }
        static int unpackLongY(int64_t l) { return (int)(l << 64 - SIZE_BITS_Y >> 64 - SIZE_BITS_Y); }
        static int unpackLongZ(int64_t l) { return (int)(l << 64 - BIT_SHIFT_Z - SIZE_BITS_Z >> 64 - SIZE_BITS_Z); }

        static int64_t add(int64_t packed, int dx, int dy, int dz)
        {
            return asLong(
                unpackLongX(packed) + dx,
                unpackLongY(packed) + dy,
                unpackLongZ(packed) + dz);
        }
    };

    inline const BlockPos BlockPos::ORIGIN{0, 0, 0};

} // namespace mc