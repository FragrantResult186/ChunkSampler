#pragma once

#include "chunk.hpp"
#include "noise_column_sampler.hpp"
#include "aquifer_sampler.hpp"
#include <vector>

namespace mc
{

    class NoiseChunkGenerator
    {
    public:
        static constexpr int LAVA_LEVEL = -54;
        static constexpr int WATER_LEVEL = 63;

        explicit NoiseChunkGenerator(uint64_t seed)
            : noiseColumnSampler_(seed)
        {
            lavaLevel_ = FluidLevel(LAVA_LEVEL, Blocks::LAVA());
            waterLevel_ = FluidLevel(WATER_LEVEL, Blocks::WATER());
            fluidSampler_ = [this](int, int y, int) -> FluidLevel
            {
                return (y < LAVA_LEVEL) ? lavaLevel_ : waterLevel_;
            };
        }

        void populateNoise(Chunk &chunk, bool skipCaves = false)
        {
            auto &surfaceMap = chunk.getHeightmap(Heightmap::Type::WORLD_SURFACE_WG);
            const ChunkPos &pos = chunk.getPos();
            int startX = pos.getStartX();
            int startZ = pos.getStartZ();

            ChunkNoiseSampler sampler = ChunkNoiseSampler::create(
                pos, noiseColumnSampler_,
                [](int, int, int)
                { return 0.0; },
                fluidSampler_,
                skipCaves);

            int topSection = chunk.countVerticalSections() - 1;
            sampler.sampleStartNoise();

            for (int q = 0; q < 4; ++q)
            {
                sampler.sampleEndNoise(q);
                for (int r = 0; r < 4; ++r)
                {
                    auto *section = &chunk.getSection(topSection);
                    for (int s = 48 - 1; s >= 0; --s)
                    {
                        sampler.sampleNoiseCorners(s, r);
                        for (int t = 8 - 1; t >= 0; --t)
                        {
                            int blockY = (-8 + s) * 8 + t;
                            int secIdx = chunk.getSectionIndex(blockY);
                            if (chunk.getSectionIndex(section->yOffset) != secIdx)
                                section = &chunk.getSection(secIdx);

                            sampler.sampleNoiseY(t / 8.0);
                            for (int x = 0; x < 4; ++x)
                            {
                                int worldX = startX + q * 4 + x;
                                int localX = worldX & 15;
                                sampler.sampleNoiseX(x / 4.0);
                                for (int aa = 0; aa < 4; ++aa)
                                {
                                    int worldZ = startZ + r * 4 + aa;
                                    int localZ = worldZ & 15;
                                    sampler.sampleNoise(aa / 4.0);
                                    BlockState bs = sampler.sampleInitialNoiseBlockState(worldX, blockY, worldZ);
                                    if (bs.isOf(BlockType::NONE))
                                        bs = Blocks::STONE();
                                    chunk.setBlock(localX, blockY, localZ, bs);
                                    if (!bs.isAir())
                                        surfaceMap.trackUpdate(localX, blockY, localZ);
                                }
                            }
                        }
                    }
                }
                sampler.swapBuffers();
            }
        }

        std::vector<int> computeHeightmapOnly(int chunkX, int chunkZ, bool skipCaves = false)
        {
            ChunkPos pos(chunkX, chunkZ);

            ChunkNoiseSampler sampler = ChunkNoiseSampler::create(
                pos, noiseColumnSampler_,
                [](int, int, int)
                { return 0.0; },
                fluidSampler_,
                skipCaves);

            std::vector<int> heights(256, 0);
            std::vector<bool> done(256, false);
            int remaining = 256;
            int startX = pos.getStartX();
            int startZ = pos.getStartZ();
            bool finished = false;

            sampler.sampleStartNoise();

            for (int q = 0; q < 4 && !finished; ++q)
            {
                sampler.sampleEndNoise(q);
                for (int r = 0; r < 4 && !finished; ++r)
                {
                    for (int s = 48 - 1; s >= 0 && !finished; --s)
                    {
                        sampler.sampleNoiseCorners(s, r);
                        for (int t = 8 - 1; t >= 0 && !finished; --t)
                        {
                            int blockY = (-8 + s) * 8 + t;
                            sampler.sampleNoiseY(t / 8.0);
                            for (int x = 0; x < 4; ++x)
                            {
                                int localX = (startX + q * 4 + x) & 15;
                                sampler.sampleNoiseX(x / 4.0);
                                for (int aa = 0; aa < 4; ++aa)
                                {
                                    int localZ = (startZ + r * 4 + aa) & 15;
                                    int idx = localX + localZ * 16;
                                    if (done[idx])
                                        continue;
                                    sampler.sampleNoise(aa / 4.0);
                                    int worldX = startX + q * 4 + x;
                                    int worldZ = startZ + r * 4 + aa;
                                    BlockState bs = sampler.sampleInitialNoiseBlockState(worldX, blockY, worldZ);
                                    if (bs.isOf(BlockType::NONE))
                                        bs = Blocks::STONE();
                                    if (!bs.isAir())
                                    {
                                        heights[idx] = blockY + 1;
                                        done[idx] = true;
                                        if (--remaining == 0)
                                        {
                                            finished = true;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                sampler.swapBuffers();
            }
            return heights;
        }

        int getHeightPreliminary(int x, int z) const
        {
            const int biomeX = x >> 2;
            const int biomeZ = z >> 2;
            auto mnp = noiseColumnSampler_.sampleTerrainNoisePoint(biomeX, biomeZ);
            int prelim = noiseColumnSampler_.findSurfaceYFromNoise(x, z, mnp.terrainNoisePoint);
            return prelim == std::numeric_limits<int>::max() ? -64 : prelim + 1;
        }

    private:
        NoiseColumnSampler noiseColumnSampler_;
        FluidLevel lavaLevel_;
        FluidLevel waterLevel_;
        FluidLevelSamplerFn fluidSampler_;
    };

} // namespace mc