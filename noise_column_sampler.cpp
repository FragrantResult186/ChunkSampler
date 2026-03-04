#include "noise_column_sampler.hpp"

namespace mc
{

    const NoiseSamplingConfig NoiseColumnSampler::NOISE_SAMPLING_CONFIG{1.0, 1.0, 80.0, 160.0};
    const SlideConfig NoiseColumnSampler::TOP_SLIDE{-0.078125, 2, 8};
    const SlideConfig NoiseColumnSampler::BOTTOM_SLIDE{0.1171875, 3, 0};

    NoiseColumnSampler::NoiseColumnSampler(uint64_t seed)
    {
        Xoroshiro128PlusPlusRandom baseRng(seed);
        auto baseDeriver = baseRng.createRandomDeriver();

        auto mkRng = [&](const std::string &key) -> std::unique_ptr<AbstractRandom>
        {
            return baseDeriver->createRandom("minecraft:" + key);
        };

        auto mkNoise = [&](const std::string &key,
                           const DoublePerlinNoiseSampler::NoiseParameters &params)
            -> std::unique_ptr<DoublePerlinNoiseSampler>
        {
            auto r = mkRng(key);
            return std::make_unique<DoublePerlinNoiseSampler>(
                DoublePerlinNoiseSampler::create(*r, params));
        };

        {
            auto r = mkRng("terrain");
            terrainNoise_ = std::make_unique<InterpolatedNoiseSampler>(*r, NOISE_SAMPLING_CONFIG, 4, 8);
        }
        {
            auto r = mkRng("aquifer");
            aquiferDeriver_ = r->createRandomDeriver();
        }

        aquiferBarrier_ =
            mkNoise("aquifer_barrier", NoiseParameters::AQUIFER_BARRIER);
        aquiferFloodedness_ =
            mkNoise("aquifer_fluid_level_floodedness", NoiseParameters::AQUIFER_FLUID_LEVEL_FLOODEDNESS);
        aquiferLava_ =
            mkNoise("aquifer_lava", NoiseParameters::AQUIFER_LAVA);
        aquiferSpread_ =
            mkNoise("aquifer_fluid_level_spread", NoiseParameters::AQUIFER_FLUID_LEVEL_SPREAD);
        shiftNoise_ =
            mkNoise("offset", NoiseParameters::OFFSET);
        pillarNoise_ =
            mkNoise("pillar", NoiseParameters::PILLAR);
        pillarRareness_ =
            mkNoise("pillar_rareness", NoiseParameters::PILLAR_RARENESS);
        pillarThickness_ =
            mkNoise("pillar_thickness", NoiseParameters::PILLAR_THICKNESS);
        spaghetti2d_ =
            mkNoise("spaghetti_2d", NoiseParameters::SPAGHETTI_2D);
        spaghetti2dElev_ =
            mkNoise("spaghetti_2d_elevation", NoiseParameters::SPAGHETTI_2D_ELEVATION);
        spaghetti2dMod_ =
            mkNoise("spaghetti_2d_modulator", NoiseParameters::SPAGHETTI_2D_MODULATOR);
        spaghetti2dThick_ =
            mkNoise("spaghetti_2d_thickness", NoiseParameters::SPAGHETTI_2D_THICKNESS);
        spaghetti3d1_ =
            mkNoise("spaghetti_3d_1", NoiseParameters::SPAGHETTI_3D_1);
        spaghetti3d2_ =
            mkNoise("spaghetti_3d_2", NoiseParameters::SPAGHETTI_3D_2);
        spaghetti3dRarity_ =
            mkNoise("spaghetti_3d_rarity", NoiseParameters::SPAGHETTI_3D_RARITY);
        spaghetti3dThick_ =
            mkNoise("spaghetti_3d_thickness", NoiseParameters::SPAGHETTI_3D_THICKNESS);
        spaghettiRough_ =
            mkNoise("spaghetti_roughness", NoiseParameters::SPAGHETTI_ROUGHNESS);
        spaghettiRoughMod_ =
            mkNoise("spaghetti_roughness_modulator", NoiseParameters::SPAGHETTI_ROUGHNESS_MODULATOR);
        caveEntrance_ =
            mkNoise("cave_entrance", NoiseParameters::CAVE_ENTRANCE);
        caveLayer_ =
            mkNoise("cave_layer", NoiseParameters::CAVE_LAYER);
        caveCheeseNoise_ =
            mkNoise("cave_cheese", NoiseParameters::CAVE_CHEESE);
        continentalnessNoise_ =
            mkNoise("continentalness", NoiseParameters::CONTINENTALNESS);
        erosionNoise_ =
            mkNoise("erosion", NoiseParameters::EROSION);
        weirdnessNoise_ =
            mkNoise("ridge", NoiseParameters::RIDGE);
        noodleNoise_ =
            mkNoise("noodle", NoiseParameters::NOODLE);
        noodleThickness_ =
            mkNoise("noodle_thickness", NoiseParameters::NOODLE_THICKNESS);
        noodleRidgeA_ =
            mkNoise("noodle_ridge_a", NoiseParameters::NOODLE_RIDGE_A);
        noodleRidgeB_ =
            mkNoise("noodle_ridge_b", NoiseParameters::NOODLE_RIDGE_B);

        vanillaTerrain_ = VanillaTerrainParameters::createSurfaceParameters();
    }

    NoiseColumnSampler::MultiNoisePoint NoiseColumnSampler::sampleTerrainNoisePoint(int biomeX, int biomeZ) const
    {
        double d = biomeX + shiftNoise_->sample(biomeX, 0, biomeZ) * 4.0;
        double e = biomeZ + shiftNoise_->sample(biomeZ, biomeX, 0) * 4.0;
        float f = (float)continentalnessNoise_->sample(d, 0, e);
        float g = (float)weirdnessNoise_->sample(d, 0, e);
        float h = (float)erosionNoise_->sample(d, 0, e);
        return {createTerrainNoisePoint(f, g, h)};
    }

    TerrainNoisePoint NoiseColumnSampler::createTerrainNoisePoint(float f, float g, float h) const
    {
        NoisePoint np = vanillaTerrain_.createNoisePoint(f, h, g);
        return TerrainNoisePoint(
            vanillaTerrain_.getOffset(np),
            vanillaTerrain_.getFactor(np),
            vanillaTerrain_.getPeak(np));
    }

    int NoiseColumnSampler::findSurfaceYFromNoise(int x, int z, const TerrainNoisePoint &tp) const
    {
        for (int k = -8 + 48; k >= -8; --k)
        {
            int blockY = k * 8;
            double e = sampleNoiseColumn(x, blockY, z, tp, -0.703125, true);
            if (e > 0.390625)
                return blockY;
        }
        return std::numeric_limits<int>::max();
    }

    double NoiseColumnSampler::sampleNoiseColumn(int x, int y, int z, const TerrainNoisePoint &tp,
                                                 double terrainOverride, bool skipCaves) const
    {
        double d = std::isnan(terrainOverride)
                       ? terrainNoise_->calculateNoise(x, y, z)
                       : terrainOverride;
        return sampleNoiseColumnInternal(x, y, z, tp, d, skipCaves);
    }

    std::unique_ptr<AquiferSampler> NoiseColumnSampler::createAquiferSampler(
        ChunkNoiseSampler &chunkNoiseSampler,
        int startX, int startZ,
        int startY, int height,
        FluidLevelSamplerFn fluidLevelSampler) const
    {
        ChunkPos chunkPos(ChunkSectionPos::getSectionCoord(startX),
                          ChunkSectionPos::getSectionCoord(startZ));
        return std::make_unique<AquiferSamplerImpl>(
            &chunkNoiseSampler,
            chunkPos,
            *aquiferBarrier_,
            *aquiferFloodedness_,
            *aquiferSpread_,
            *aquiferLava_,
            *aquiferDeriver_,
            startY, height,
            std::move(fluidLevelSampler));
    }

    ChunkNoiseSampler::BlockStateSampler NoiseColumnSampler::createInitialNoiseBlockStateSampler(
        ChunkNoiseSampler &cns,
        ChunkNoiseSampler::ColumnSampler columnSampler,
        bool skipCaves) const
    {
        auto mainInterp = std::make_shared<ChunkNoiseSampler::NoiseInterpolator>(
            cns,
            [this, &cns, skipCaves](int i, int j, int k)
            {
                auto mnp = cns.createMultiNoisePoint(
                    BiomeCoords::fromBlock(i), BiomeCoords::fromBlock(k));
                return sampleNoiseColumn(i, j, k, mnp.terrainNoisePoint,
                                         std::numeric_limits<double>::quiet_NaN(), skipCaves);
            });

        if (skipCaves)
        {
            return [mainInterp, columnSampler = std::move(columnSampler), &cns](int i, int j, int k) -> BlockState
            {
                double d = mainInterp->sample();
                double e = MathHelper::clamp(d * 0.64, -1.0, 1.0);
                e = e / 2.0 - e * e * e / 24.0;
                e += columnSampler(i, j, k);
                return cns.applyAquifer(i, j, k, d, e);
            };
        }

        auto noodleInterp = makeNoodleSampler(cns, *noodleNoise_, -1.0, 1.0);
        auto thickInterp = makeNoodleSampler(cns, *noodleThickness_, 0.0, 1.0);
        auto ridgeAInterp = makeNoodleSampler(cns, *noodleRidgeA_, 0.0, 2.6666666666666665);
        auto ridgeBInterp = makeNoodleSampler(cns, *noodleRidgeB_, 0.0, 2.6666666666666665);

        return [mainInterp, noodleInterp, thickInterp, ridgeAInterp, ridgeBInterp,
                columnSampler = std::move(columnSampler), &cns](int i, int j, int k) -> BlockState
        {
            double d = mainInterp->sample();
            double e = MathHelper::clamp(d * 0.64, -1.0, 1.0);
            e = e / 2.0 - e * e * e / 24.0;

            if (noodleInterp->sample() >= 0.0)
            {
                double h = MathHelper::clampedLerpFromProgress(thickInterp->sample(), -1.0, 1.0, 0.05, 0.1);
                double l = std::abs(1.5 * ridgeAInterp->sample()) - h;
                double m = std::abs(1.5 * ridgeBInterp->sample()) - h;
                e = std::min(e, std::max(l, m));
            }

            e += columnSampler(i, j, k);
            return cns.applyAquifer(i, j, k, d, e);
        };
    }

    RandomDeriver &NoiseColumnSampler::aquiferDeriver() { return *aquiferDeriver_; }
    const DoublePerlinNoiseSampler &NoiseColumnSampler::aquiferBarrierNoise() const { return *aquiferBarrier_; }
    const DoublePerlinNoiseSampler &NoiseColumnSampler::aquiferFloodednessNoise() const { return *aquiferFloodedness_; }
    const DoublePerlinNoiseSampler &NoiseColumnSampler::aquiferSpreadNoise() const { return *aquiferSpread_; }
    const DoublePerlinNoiseSampler &NoiseColumnSampler::aquiferLavaNoise() const { return *aquiferLava_; }

    double NoiseColumnSampler::sampleNoiseColumnInternal(int x, int y, int z,
                                                         const TerrainNoisePoint &tp,
                                                         double terrainD, bool skipCaves) const
    {
        double g = (1.0 - (double)y / 128.0) + tp.offset;
        double e = g * tp.factor;
        e = e * (e > 0.0 ? 4.0 : 1.0);
        double f = e + terrainD;

        double h, l, m;
        if (!skipCaves && !(f < -64.0))
        {
            double n = f - 1.5625;
            bool below = n < 0.0;
            double cave = sampleCaveEntranceNoise(x, y, z);
            double rough = sampleSpaghettiRoughnessNoise(x, y, z);
            double spag3 = sampleSpaghetti3dNoise(x, y, z);
            double r = std::min(cave, spag3 + rough);

            if (below)
            {
                h = f;
                l = r * 5.0;
                m = -64.0;
            }
            else
            {
                double layer = sampleCaveLayerNoise(x, y, z);
                double cheese = caveCheeseNoise_->sample(x, (double)y / 1.5, z);
                double u = MathHelper::clamp(cheese + 0.27, -1.0, 1.0);
                double v = n * 1.28;
                double w = u + MathHelper::clampedLerp(0.5, 0.0, v);
                h = w + layer;
                double spag2 = sampleSpaghetti2dNoise(x, y, z);
                l = std::min(r, spag2 + rough);
                m = samplePillarNoise(x, y, z);
            }
        }
        else
        {
            h = f;
            l = 64.0;
            m = -64.0;
        }

        double result = std::max(std::min(h, l), m);
        result = TOP_SLIDE.apply(result, 48 - (y / 8 - (-8)));
        result = BOTTOM_SLIDE.apply(result, y / 8 - (-8));
        return MathHelper::clamp(result, -64.0, 64.0);
    }

    double NoiseColumnSampler::sampleCaveEntranceNoise(int x, int y, int z) const
    {
        double g = caveEntrance_->sample(x * 0.75, y * 0.5, z * 0.75) + 0.37;
        double h = (double)(y + 10) / 40.0;
        return g + MathHelper::clampedLerp(0.3, 0.0, h);
    }

    double NoiseColumnSampler::samplePillarNoise(int x, int y, int z) const
    {
        double f = NoiseHelper::lerpFromProgress(*pillarRareness_, x, y, z, 0.0, 2.0);
        double l = NoiseHelper::lerpFromProgress(*pillarThickness_, x, y, z, 0.0, 1.1);
        l = std::pow(l, 3.0);
        double o = pillarNoise_->sample(x * 25.0, y * 0.3, z * 25.0);
        o = l * (o * 2.0 - f);
        return o > 0.03 ? o : std::numeric_limits<double>::lowest();
    }

    double NoiseColumnSampler::sampleCaveLayerNoise(int x, int y, int z) const
    {
        double d = caveLayer_->sample(x, y * 8, z);
        return MathHelper::square(d) * 4.0;
    }

    double NoiseColumnSampler::sampleSpaghetti3dNoise(int x, int y, int z) const
    {
        double d = spaghetti3dRarity_->sample(x * 2, y, z * 2);
        double e = CaveScaler::scaleTunnels(d);
        double h = NoiseHelper::lerpFromProgress(*spaghetti3dThick_, x, y, z, 0.065, 0.088);
        double l = std::abs(e * spaghetti3d1_->sample(x / e, y / e, z / e)) - h;
        double o = std::abs(e * spaghetti3d2_->sample(x / e, y / e, z / e)) - h;
        return MathHelper::clamp(std::max(l, o), -1.0, 1.0);
    }

    double NoiseColumnSampler::sampleSpaghetti2dNoise(int x, int y, int z) const
    {
        double d = spaghetti2dMod_->sample(x * 2, y, z * 2);
        double e = CaveScaler::scaleCaves(d);
        double h = NoiseHelper::lerpFromProgress(*spaghetti2dThick_, x * 2, y, z * 2, 0.6, 1.3);
        double l = std::abs(e * spaghetti2d_->sample(x / e, y / e, z / e)) - 0.083 * h;
        double q = NoiseHelper::lerpFromProgress(*spaghetti2dElev_, x, 0, z, -8.0, 8.0);
        double r = std::abs(q - (double)y / 8.0) - h;
        r = r * r * r;
        return MathHelper::clamp(std::max(r, l), -1.0, 1.0);
    }

    double NoiseColumnSampler::sampleSpaghettiRoughnessNoise(int x, int y, int z) const
    {
        double d = NoiseHelper::lerpFromProgress(*spaghettiRoughMod_, x, y, z, 0.0, 0.1);
        return (0.4 - std::abs(spaghettiRough_->sample(x, y, z))) * d;
    }

    std::shared_ptr<ChunkNoiseSampler::NoiseInterpolator>
    NoiseColumnSampler::makeNoodleSampler(ChunkNoiseSampler &cns,
                                          const DoublePerlinNoiseSampler &noise,
                                          double defaultVal, double scale)
    {
        constexpr int kMin = -60, kMax = 320;
        return std::make_shared<ChunkNoiseSampler::NoiseInterpolator>(
            cns,
            [&noise, defaultVal, scale](int x, int y, int z) -> double
            {
                if (y < kMin || y > kMax)
                    return defaultVal;
                return noise.sample(x * scale, y * scale, z * scale);
            });
    }

    // ChunkNoiseSampler::create is defined here because it needs NoiseColumnSampler
    ChunkNoiseSampler ChunkNoiseSampler::create(
        const ChunkPos &pos,
        NoiseColumnSampler &ncs,
        ColumnSampler columnSampler,
        FluidLevelSamplerFn fluidSampler,
        bool skipCaves)
    {
        ChunkNoiseSampler cns;
        cns.horizontalSize_ = 4;
        cns.height_ = 48;
        cns.minimumY_ = -8;
        cns.noiseColumnSampler_ = &ncs;

        constexpr int hBlockSize = 4;
        cns.x_ = pos.getStartX() / hBlockSize;
        cns.z_ = pos.getStartZ() / hBlockSize;
        cns.biomeX_ = BiomeCoords::fromBlock(pos.getStartX());
        cns.biomeZ_ = BiomeCoords::fromBlock(pos.getStartZ());

        int o = BiomeCoords::fromBlock(cns.horizontalSize_ * hBlockSize);
        cns.field35486_.resize(o + 1, std::vector<MultiNoisePoint>(o + 1, {{0, 0, 0}}));

        for (int p = 0; p <= o; ++p)
        {
            int q = cns.biomeX_ + p;
            for (int r = 0; r <= o; ++r)
            {
                int s = cns.biomeZ_ + r;
                auto mnp = ncs.sampleTerrainNoisePoint(q, s);
                cns.field35486_[p][r].terrainNoisePoint = mnp.terrainNoisePoint;
            }
        }

        auto aquifer = ncs.createAquiferSampler(
            cns,
            pos.getStartX(), pos.getStartZ(),
            cns.minimumY_ * 8, cns.height_ * 8,
            std::move(fluidSampler));
        cns.aquifer_ = std::move(aquifer);
        cns.blockStateSampler_ = ncs.createInitialNoiseBlockStateSampler(cns, std::move(columnSampler), skipCaves);

        return cns;
    }

} // namespace mc