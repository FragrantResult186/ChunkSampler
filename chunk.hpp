#pragma once

#include "math_helper.hpp"
#include "block_pos.hpp"
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace mc
{

    enum class BlockType
    {
        AIR,
        STONE,
        WATER,
        LAVA,
        // Internal sentinel used to represent Java's `null` from AquiferSampler.apply
        NONE
    };

    struct BlockState
    {
        BlockType type;
        explicit BlockState(BlockType t = BlockType::AIR) : type(t) {}
        bool isAir() const { return type == BlockType::AIR; }
        bool isOf(BlockType t) const { return type == t; }
    };

    namespace Blocks
    {
        inline BlockState AIR() { return BlockState{BlockType::AIR}; }
        inline BlockState STONE() { return BlockState{BlockType::STONE}; }
        inline BlockState WATER() { return BlockState{BlockType::WATER}; }
        inline BlockState LAVA() { return BlockState{BlockType::LAVA}; }
        // Internal sentinel used to represent Java's `null` from AquiferSampler.apply
        inline BlockState NONE() { return BlockState{BlockType::NONE}; }
    }

    struct HeightLimitView
    {
        virtual int getHeight() const = 0;
        virtual int getBottomY() const = 0;
        virtual ~HeightLimitView() = default;

        int getTopY() const { return getBottomY() + getHeight(); }
        int getBottomSectionCoord() const { return getBottomY() >> 4; }
        int getTopSectionCoord() const { return ((getTopY() - 1) >> 4) + 1; }
        int countVerticalSections() const { return getTopSectionCoord() - getBottomSectionCoord(); }

        int getSectionIndex(int blockY) const { return sectionCoordToIndex(blockY >> 4); }
        int sectionCoordToIndex(int sectionY) const { return sectionY - getBottomSectionCoord(); }
    };

    class PackedIntegerArray
    {
    public:
        PackedIntegerArray(int bitsPerEntry, int size)
            : bitsPerEntry_(bitsPerEntry),
              size_(size),
              maxValue_((1ULL << bitsPerEntry) - 1),
              elemsPerLong_(64 / bitsPerEntry)
        {
            int numLongs = (size + elemsPerLong_ - 1) / elemsPerLong_;
            data_.assign(numLongs, 0ULL);
        }

        void set(int idx, int val)
        {
            int longIdx = idx / elemsPerLong_;
            int bitOffset = (idx % elemsPerLong_) * bitsPerEntry_;
            data_[longIdx] = (data_[longIdx] & ~(maxValue_ << bitOffset)) | ((uint64_t)val << bitOffset);
        }

        int get(int idx) const
        {
            int longIdx = idx / elemsPerLong_;
            int bitOffset = (idx % elemsPerLong_) * bitsPerEntry_;
            return (int)((data_[longIdx] >> bitOffset) & maxValue_);
        }

    private:
        int bitsPerEntry_, size_, elemsPerLong_;
        uint64_t maxValue_;
        std::vector<uint64_t> data_;
    };

    struct Heightmap
    {
        enum class Type
        {
            WORLD_SURFACE_WG
        };

        explicit Heightmap(const HeightLimitView &chunk)
            : chunk_(&chunk),
              storage_(MathHelper::ceilLog2(chunk.getHeight() + 1), 256),
              bottomY_(chunk.getBottomY())
        {
        }

        void trackUpdate(int x, int blockY, int z)
        {
            int cur = get(x, z);
            if (blockY > cur - 2 && blockY >= cur)
                set(x, z, blockY + 1);
        }

        int get(int x, int z) const { return storage_.get(toIndex(x, z)) + bottomY_; }

    private:
        const HeightLimitView *chunk_;
        PackedIntegerArray storage_;
        int bottomY_;

        void set(int x, int z, int val) { storage_.set(toIndex(x, z), val - bottomY_); }
        static int toIndex(int x, int z) { return x + z * 16; }
    };

    struct ChunkSection
    {
        int yOffset;
        explicit ChunkSection(int sectionIndex) : yOffset(sectionIndex << 4) {}
    };

    class Chunk : public HeightLimitView
    {
    public:
        static constexpr int BOTTOM_Y = -64;
        static constexpr int HEIGHT = 384;
        static constexpr int NUM_SECTIONS = 24;
        static constexpr int BLOCK_COUNT = 16 * HEIGHT * 16;

        Chunk() : pos_(0, 0), blocks_(BLOCK_COUNT, BlockState(BlockType::AIR)) {}
        explicit Chunk(const ChunkPos &pos) : pos_(pos), blocks_(BLOCK_COUNT, BlockState(BlockType::AIR))
        {
            sections_.reserve(NUM_SECTIONS);
            for (int i = 0; i < NUM_SECTIONS; ++i)
                sections_.emplace_back(i - 4);
        }

        int getHeight() const override { return HEIGHT; }
        int getBottomY() const override { return BOTTOM_Y; }

        const ChunkPos &getPos() const { return pos_; }

        ChunkSection &getSection(int idx) { return sections_[idx]; }
        const ChunkSection &getSection(int idx) const { return sections_[idx]; }

        Heightmap &getHeightmap(Heightmap::Type type)
        {
            auto it = heightmaps_.find(type);
            if (it == heightmaps_.end())
            {
                auto [ins, ok] = heightmaps_.emplace(type, Heightmap(*this));
                return ins->second;
            }
            return it->second;
        }

        void setBlock(int localX, int blockY, int localZ, BlockState bs)
        {
            int idx = blockIndex(localX, blockY, localZ);
            if (idx >= 0) blocks_[idx] = bs;
        }

        BlockState getBlock(int localX, int blockY, int localZ) const
        {
            int idx = blockIndex(localX, blockY, localZ);
            if (idx < 0) return BlockState(BlockType::AIR);
            return blocks_[idx];
        }

    private:
        ChunkPos pos_;
        std::vector<ChunkSection> sections_;
        std::unordered_map<Heightmap::Type, Heightmap> heightmaps_;
        std::vector<BlockState> blocks_;

        int blockIndex(int lx, int blockY, int lz) const
        {
            int ly = blockY - BOTTOM_Y;
            if (lx < 0 || lx >= 16 || ly < 0 || ly >= HEIGHT || lz < 0 || lz >= 16)
                return -1;
            return lx + ly * 16 + lz * 16 * HEIGHT;
        }
    };

} // namespace mc