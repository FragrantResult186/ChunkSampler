#pragma once

#include "chunk.hpp"
#include "block_pos.hpp"
#include "noise.hpp"
#include "abstract_random.hpp"
#include "math_helper.hpp"
#include <vector>
#include <cstdint>
#include <functional>

namespace mc
{

    class ChunkNoiseSampler;
    struct FluidLevel;
    using FluidLevelSamplerFn = std::function<FluidLevel(int, int, int)>;

    class AquiferSampler
    {
    public:
        virtual ~AquiferSampler() = default;
        virtual BlockState apply(int x, int y, int z, double density, double extra) = 0;
    };

    class AquiferSamplerImpl : public AquiferSampler
    {
    private:
        static constexpr int GRID_SIZE_X = 16;
        static constexpr int GRID_SIZE_Y = 12;
        static constexpr int GRID_SIZE_Z = 16;

        static constexpr int NEIGHBOUR_OFFSETS[13][2] = {
            {-2, -1}, {-1, -1}, {0, -1}, {1, -1}, {-3, 0}, {-2, 0}, {-1, 0}, {0, 0}, {1, 0}, {-2, 1}, {-1, 1}, {0, 1}, {1, 1}};

    public:
        AquiferSamplerImpl(ChunkNoiseSampler *chunkNoiseSampler,
                           const ChunkPos &chunkPos,
                           const DoublePerlinNoiseSampler &barrierNoise,
                           const DoublePerlinNoiseSampler &floodednessNoise,
                           const DoublePerlinNoiseSampler &spreadNoise,
                           const DoublePerlinNoiseSampler &lavaNoise,
                           RandomDeriver &randomDeriver,
                           int startY,
                           int height,
                           FluidLevelSamplerFn fluidLevelSampler);

        BlockState apply(int x, int y, int z, double density, double extra) override;

        FluidLevel getFluidLevel(int x, int y, int z);

    private:
        ChunkNoiseSampler *chunkNoiseSampler_;
        const DoublePerlinNoiseSampler &barrierNoise_;
        const DoublePerlinNoiseSampler &floodednessNoise_;
        const DoublePerlinNoiseSampler &spreadNoise_;
        const DoublePerlinNoiseSampler &lavaNoise_;
        RandomDeriver &randomDeriver_;
        FluidLevelSamplerFn fluidLevelSampler_;

        int startX_, startY_, startZ_;
        int sizeX_, sizeY_, sizeZ_;
        std::vector<FluidLevel> waterLevels_;
        std::vector<bool> waterLevelsInit_;
        std::vector<int64_t> blockPositions_;

        int getLocalX(int blockX) const { return MathHelper::floorDiv(blockX, 16); }
        int getLocalY(int blockY) const { return MathHelper::floorDiv(blockY, 12); }
        int getLocalZ(int blockZ) const { return MathHelper::floorDiv(blockZ, 16); }

        int index(int lx, int ly, int lz) const
        {
            int ix = lx - startX_;
            int iy = ly - startY_;
            int iz = lz - startZ_;
            return (iy * sizeZ_ + iz) * sizeX_ + ix;
        }

        FluidLevel getWaterLevel(int64_t packed);
        static double maxDistance(int d1, int d2);
        double calculateDensity(int x, int y, int z, double &barrierCache,
                                const FluidLevel &f1, const FluidLevel &f2);
        BlockState determineFluidType(int x, int y, int z,
                                      const FluidLevel &base, int targetY);
    };

} // namespace mc