#pragma once

#include "noise_chunk_generator.hpp"
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <optional>

namespace mc
{

    template <typename K, typename V, typename Hash = std::hash<K>>
    class LruCache
    {
    public:
        explicit LruCache(int maxSize) : maxSize_(maxSize) {}

        V *get(const K &key)
        {
            auto it = map_.find(key);
            if (it == map_.end())
                return nullptr;
            return &it->second;
        }

        void put(const K &key, V value)
        {
            map_[key] = std::move(value);
            if ((int)map_.size() > maxSize_)
                map_.erase(map_.begin());
        }

        void clear() { map_.clear(); }

    private:
        int maxSize_;
        std::unordered_map<K, V, Hash> map_;
    };

    // ------------------------- PUBLIC API -------------------------

    class ChunkSampler
    {
    public:
        explicit ChunkSampler(uint64_t seed = 0, int cacheSize = 256)
            : seed_(seed), cacheSize_(cacheSize),
              generator_(std::make_unique<NoiseChunkGenerator>(seed)),
              chunkCache_(cacheSize),
              skipCavesChunkCache_(cacheSize),
              heightCache_(cacheSize)
        {
        }

        void setSeed(uint64_t seed)
        {
            if (seed_ == seed)
                return;
            seed_ = seed;
            generator_ = std::make_unique<NoiseChunkGenerator>(seed);
            chunkCache_.clear();
            skipCavesChunkCache_.clear();
            heightCache_.clear();
        }

        uint64_t getSeed() const { return seed_; }

        // -----------------------------------------------------------------------

        Chunk &getChunk(int chunkX, int chunkZ, bool skipCaves = false)
        {
            ChunkPos pos(chunkX, chunkZ);
            if (!skipCaves)
            {
                if (Chunk *c = chunkCache_.get(pos))
                    return *c;
            }
            else
            {
                if (Chunk *c = skipCavesChunkCache_.get(pos))
                    return *c;
            }

            Chunk chunk(pos);
            generator_->populateNoise(chunk, skipCaves);
            if (!skipCaves)
            {
                chunkCache_.put(pos, std::move(chunk));
                return *chunkCache_.get(pos);
            }
            else
            {
                skipCavesChunkCache_.put(pos, std::move(chunk));
                return *skipCavesChunkCache_.get(pos);
            }
        }

        Chunk &getChunkAt(int x, int z, bool skipCaves = false)
        {
            return getChunk(x >> 4, z >> 4, skipCaves);
        }

        // -----------------------------------------------------------------------

        std::vector<int> getHeightmap(int chunkX, int chunkZ, bool skipCaves = false)
        {
            ChunkPos pos(chunkX, chunkZ);
            if (!skipCaves)
            {
                if (Chunk *c = chunkCache_.get(pos))
                {
                    auto &hm = c->getHeightmap(Heightmap::Type::WORLD_SURFACE_WG);
                    std::vector<int> out(256);
                    for (int z = 0; z < 16; ++z)
                        for (int x = 0; x < 16; ++x)
                            out[x + z * 16] = hm.get(x, z);
                    return out;
                }
            }

            uint64_t key = heightKey(chunkX, chunkZ, skipCaves);
            if (std::vector<int> *h = heightCache_.get(key))
                return *h;

            auto heights = generator_->computeHeightmapOnly(chunkX, chunkZ, skipCaves);
            heightCache_.put(key, heights);
            return heights;
        }

        int getHeight(int x, int z, bool skipCaves = false)
        {
            int cx = x >> 4, cz = z >> 4;
            ChunkPos pos(cx, cz);

            if (!skipCaves)
            {
                if (Chunk *c = chunkCache_.get(pos))
                    return c->getHeightmap(Heightmap::Type::WORLD_SURFACE_WG).get(x & 15, z & 15);
            }

            uint64_t key = heightKey(cx, cz, skipCaves);
            if (std::vector<int> *h = heightCache_.get(key))
                return (*h)[(x & 15) + (z & 15) * 16];

            auto heights = generator_->computeHeightmapOnly(cx, cz, skipCaves);
            int result = heights[(x & 15) + (z & 15) * 16];
            heightCache_.put(key, std::move(heights));
            return result;
        }

        int getHeight(int x, int z, Heightmap::Type type, bool skipCaves = false)
        {
            if (type == Heightmap::Type::WORLD_SURFACE_WG)
                return getHeight(x, z, skipCaves);
            return getChunkAt(x, z).getHeightmap(type).get(x & 15, z & 15);
        }

        int getSurfaceY(int x, int z, bool skipCaves = false)
        {
            return getHeight(x, z, skipCaves) - 1;
        }

        int getTopSolidBlockY(int x, int z, bool skipCaves = false)
        {
            return getSurfaceY(x, z, skipCaves);
        }

        // -----------------------------------------------------------------------

        BlockState getBlock(int x, int y, int z, bool skipCaves = false)
        {
            // Quick check to avoid unnecessary chunk generation for out-of-bounds Y
            if (y < -60 || y > 254)
                return Blocks::AIR();

            Chunk &chunk = getChunkAt(x, z, skipCaves);
            return chunk.getBlock(x & 15, y, z & 15);
        }

        bool isAir(int x, int y, int z, bool skipCaves = false)
        {
            return getBlock(x, y, z, skipCaves).isAir();
        }

        bool isSolid(int x, int y, int z, bool skipCaves = false)
        {
            return getBlock(x, y, z, skipCaves).isOf(BlockType::STONE);
        }

        bool isWater(int x, int y, int z, bool skipCaves = false)
        {
            return getBlock(x, y, z, skipCaves).isOf(BlockType::WATER);
        }

        bool isLava(int x, int y, int z, bool skipCaves = false)
        {
            return getBlock(x, y, z, skipCaves).isOf(BlockType::LAVA);
        }

        // -----------------------------------------------------------------------

        void clearCache()
        {
            chunkCache_.clear();
            skipCavesChunkCache_.clear();
            heightCache_.clear();
        }

    private:
        uint64_t seed_;
        int cacheSize_;
        std::unique_ptr<NoiseChunkGenerator> generator_;
        LruCache<ChunkPos, Chunk, ChunkPos::Hash> chunkCache_;
        LruCache<ChunkPos, Chunk, ChunkPos::Hash> skipCavesChunkCache_;
        LruCache<uint64_t, std::vector<int>> heightCache_;

        static uint64_t heightKey(int cx, int cz, bool skipCaves = false)
        {
            uint64_t base = ((uint64_t)(uint32_t)cx << 32) | (uint32_t)cz;
            return skipCaves ? (base ^ 0x8000000000000000ULL) : base;
        }
    };

} // namespace mc