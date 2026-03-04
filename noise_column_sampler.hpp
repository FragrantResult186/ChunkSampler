#pragma once

#include "math_helper.hpp"
#include "noise.hpp"
#include "terrain.hpp"
#include "chunk_noise_sampler.hpp"
#include "abstract_random.hpp"
#include "aquifer_sampler.hpp"
#include <cmath>
#include <string>
#include <memory>

namespace mc
{

    namespace NoiseParameters
    {
        using NP = DoublePerlinNoiseSampler::NoiseParameters;
        inline NP OFFSET{-3, {1, 1, 1, 0}};
        inline NP AQUIFER_BARRIER{-3, {1}};
        inline NP AQUIFER_FLUID_LEVEL_FLOODEDNESS{-7, {1}};
        inline NP AQUIFER_LAVA{-1, {1}};
        inline NP AQUIFER_FLUID_LEVEL_SPREAD{-5, {1}};
        inline NP PILLAR{-7, {1, 1}};
        inline NP PILLAR_RARENESS{-8, {1}};
        inline NP PILLAR_THICKNESS{-8, {1}};
        inline NP SPAGHETTI_2D{-7, {1}};
        inline NP SPAGHETTI_2D_ELEVATION{-8, {1}};
        inline NP SPAGHETTI_2D_MODULATOR{-11, {1}};
        inline NP SPAGHETTI_2D_THICKNESS{-11, {1}};
        inline NP SPAGHETTI_3D_1{-7, {1}};
        inline NP SPAGHETTI_3D_2{-7, {1}};
        inline NP SPAGHETTI_3D_RARITY{-11, {1}};
        inline NP SPAGHETTI_3D_THICKNESS{-8, {1}};
        inline NP SPAGHETTI_ROUGHNESS{-5, {1}};
        inline NP SPAGHETTI_ROUGHNESS_MODULATOR{-8, {1}};
        inline NP CAVE_ENTRANCE{-7, {0.4, 0.5, 1}};
        inline NP CAVE_LAYER{-8, {1}};
        inline NP CAVE_CHEESE{-8, {0.5, 1, 2, 1, 2, 1, 0, 2, 0}};
        inline NP CONTINENTALNESS{-9, {1, 1, 2, 2, 2, 1, 1, 1, 1}};
        inline NP EROSION{-9, {1, 1, 0, 1, 1}};
        inline NP RIDGE{-7, {1, 2, 1, 0, 0, 0}};
        inline NP NOODLE{-8, {1}};
        inline NP NOODLE_THICKNESS{-8, {1}};
        inline NP NOODLE_RIDGE_A{-7, {1}};
        inline NP NOODLE_RIDGE_B{-7, {1}};
    }

    struct SlideConfig
    {
        double target;
        int size, offset;

        double apply(double d, int i) const
        {
            if (size <= 0)
                return d;
            double e = (double)(i - offset) / (double)size;
            return MathHelper::clampedLerp(target, d, e);
        }
    };

    namespace NoiseHelper
    {
        inline double lerpFromProgress(const DoublePerlinNoiseSampler &sampler,
                                       double d, double e, double f, double g, double h)
        {
            double i = sampler.sample(d, e, f);
            return MathHelper::lerpFromProgress(i, -1.0, 1.0, g, h);
        }
    }

    namespace CaveScaler
    {
        inline double scaleCaves(double d)
        {
            if (d < -0.75)
                return 0.5;
            if (d < -0.5)
                return 0.75;
            if (d < 0.5)
                return 1.0;
            return d < 0.75 ? 2.0 : 3.0;
        }

        inline double scaleTunnels(double d)
        {
            if (d < -0.5)
                return 0.75;
            if (d < 0.0)
                return 1.0;
            return d < 0.5 ? 1.5 : 2.0;
        }
    }

    class NoiseColumnSampler
    {
    public:
        static const NoiseSamplingConfig NOISE_SAMPLING_CONFIG;
        static const SlideConfig TOP_SLIDE;
        static const SlideConfig BOTTOM_SLIDE;

        struct MultiNoisePoint
        {
            TerrainNoisePoint terrainNoisePoint;
        };

        explicit NoiseColumnSampler(uint64_t seed);

        MultiNoisePoint sampleTerrainNoisePoint(int biomeX, int biomeZ) const;
        TerrainNoisePoint createTerrainNoisePoint(float f, float g, float h) const;
        int findSurfaceYFromNoise(int x, int z, const TerrainNoisePoint &tp) const;

        double sampleNoiseColumn(int x, int y, int z, const TerrainNoisePoint &tp,
                                 double terrainOverride = std::numeric_limits<double>::quiet_NaN(),
                                 bool skipCaves = false) const;

        std::unique_ptr<AquiferSampler> createAquiferSampler(
            ChunkNoiseSampler &chunkNoiseSampler,
            int startX, int startZ,
            int startY, int height,
            FluidLevelSamplerFn fluidLevelSampler) const;

        ChunkNoiseSampler::BlockStateSampler createInitialNoiseBlockStateSampler(
            ChunkNoiseSampler &cns,
            ChunkNoiseSampler::ColumnSampler columnSampler,
            bool skipCaves = false) const;

        RandomDeriver &aquiferDeriver();
        const DoublePerlinNoiseSampler &aquiferBarrierNoise() const;
        const DoublePerlinNoiseSampler &aquiferFloodednessNoise() const;
        const DoublePerlinNoiseSampler &aquiferSpreadNoise() const;
        const DoublePerlinNoiseSampler &aquiferLavaNoise() const;

    private:
        std::unique_ptr<InterpolatedNoiseSampler> terrainNoise_;
        std::unique_ptr<DoublePerlinNoiseSampler>
            aquiferBarrier_,
            aquiferFloodedness_,
            aquiferLava_,
            aquiferSpread_,
            shiftNoise_,
            pillarNoise_,
            pillarRareness_,
            pillarThickness_,
            spaghetti2d_,
            spaghetti2dElev_,
            spaghetti2dMod_,
            spaghetti2dThick_,
            spaghetti3d1_,
            spaghetti3d2_,
            spaghetti3dRarity_,
            spaghetti3dThick_,
            spaghettiRough_,
            spaghettiRoughMod_,
            caveEntrance_,
            caveLayer_,
            caveCheeseNoise_,
            continentalnessNoise_,
            erosionNoise_,
            weirdnessNoise_,
            noodleNoise_,
            noodleThickness_,
            noodleRidgeA_,
            noodleRidgeB_;
        std::unique_ptr<RandomDeriver> aquiferDeriver_;
        VanillaTerrainParameters vanillaTerrain_;

        double sampleNoiseColumnInternal(int x, int y, int z,
                                         const TerrainNoisePoint &tp,
                                         double terrainD, bool skipCaves) const;

        double sampleCaveEntranceNoise(int x, int y, int z) const;
        double samplePillarNoise(int x, int y, int z) const;
        double sampleCaveLayerNoise(int x, int y, int z) const;
        double sampleSpaghetti3dNoise(int x, int y, int z) const;
        double sampleSpaghetti2dNoise(int x, int y, int z) const;
        double sampleSpaghettiRoughnessNoise(int x, int y, int z) const;

        static std::shared_ptr<ChunkNoiseSampler::NoiseInterpolator>
        makeNoodleSampler(ChunkNoiseSampler &cns,
                          const DoublePerlinNoiseSampler &noise,
                          double defaultVal, double scale);
    };

} // namespace mc