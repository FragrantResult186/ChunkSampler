#include "aquifer_sampler.hpp"
#include "chunk_noise_sampler.hpp"
#include <algorithm>
#include <cstdint>

namespace mc
{

    AquiferSamplerImpl::AquiferSamplerImpl(
        ChunkNoiseSampler *chunkNoiseSampler,
        const ChunkPos &chunkPos,
        const DoublePerlinNoiseSampler &barrierNoise,
        const DoublePerlinNoiseSampler &floodednessNoise,
        const DoublePerlinNoiseSampler &spreadNoise,
        const DoublePerlinNoiseSampler &lavaNoise,
        RandomDeriver &randomDeriver,
        int i, int j,
        FluidLevelSamplerFn fluidLevelSampler)
        : chunkNoiseSampler_(chunkNoiseSampler),
          barrierNoise_(barrierNoise),
          floodednessNoise_(floodednessNoise),
          spreadNoise_(spreadNoise),
          lavaNoise_(lavaNoise),
          randomDeriver_(randomDeriver),
          fluidLevelSampler_(std::move(fluidLevelSampler))
    {
        startX_ = getLocalX(chunkPos.getStartX()) - 1;
        startY_ = getLocalY(i) - 1;
        startZ_ = getLocalZ(chunkPos.getStartZ()) - 1;

        sizeX_ = getLocalX(chunkPos.getEndX()) + 1 - startX_ + 1;
        sizeY_ = getLocalY(i + j) + 1 - startY_ + 1;
        sizeZ_ = getLocalZ(chunkPos.getEndZ()) + 1 - startZ_ + 1;

        int total = sizeX_ * sizeY_ * sizeZ_;
        waterLevels_.resize(total);
        blockPositions_.resize(total);
        // Initialize random positions for the aquifer sampling grid
        for (int iy = startY_; iy < startY_ + sizeY_; ++iy) {
            for (int iz = startZ_; iz < startZ_ + sizeZ_; ++iz) {
                for (int ix = startX_; ix < startX_ + sizeX_; ++ix)
                {
                    auto rand = randomDeriver_.createRandom(ix, iy, iz);
                    int ax = ix * 16 + rand->nextInt(10);
                    int ay = iy * 12 + rand->nextInt(9);
                    int az = iz * 16 + rand->nextInt(10);
                    int idx = ((iy - startY_) * sizeZ_ + (iz - startZ_)) * sizeX_ + (ix - startX_);
                    blockPositions_[idx] = BlockPos::asLong(ax, ay, az);
                }
            }
        }
    }

    BlockState AquiferSamplerImpl::apply(int i, int j, int k, double d, double e)
    {
        if (d <= -64.0)
            return fluidLevelSampler_(i, j, k).getBlockState(j);

        if (e <= 0.0)
        {
            FluidLevel fluidLevel = fluidLevelSampler_(i, j, k);
            double f;
            BlockState blockState;

            if (fluidLevel.getBlockState(j).isOf(BlockType::LAVA))
            {
                blockState = Blocks::LAVA();
                f = 0.0;
            }
            else
            {
                int l = MathHelper::floorDiv(i - 5, 16);
                int m = MathHelper::floorDiv(j + 1, 12);
                int n = MathHelper::floorDiv(k - 5, 16);

                int32_t o = std::numeric_limits<int32_t>::max();
                int32_t p = std::numeric_limits<int32_t>::max();
                int32_t q = std::numeric_limits<int32_t>::max();
                int64_t r = 0L;
                int64_t s = 0L;
                int64_t t = 0L;

                const int ix0 = l - startX_, iy0 = m - startY_, iz0 = n - startZ_;
                for (int u = 0; u <= 1; ++u)
                {
                    for (int v = -1; v <= 1; ++v)
                    {
                        for (int w = 0; w <= 1; ++w)
                        {
                            const int idx = ((iy0 + v) * sizeZ_ + (iz0 + w)) * sizeX_ + (ix0 + u);
                            int64_t ac = blockPositions_[idx];

                            int ag = BlockPos::unpackLongX(ac) - i;
                            int ad = BlockPos::unpackLongY(ac) - j;
                            int ae = BlockPos::unpackLongZ(ac) - k;
                            int32_t af = ag * ag + ad * ad + ae * ae;

                            if (o >= af)
                            {
                                t = s;
                                s = r;
                                r = ac;
                                q = p;
                                p = o;
                                o = af;
                            }
                            else if (p >= af)
                            {
                                t = s;
                                s = ac;
                                q = p;
                                p = af;
                            }
                            else if (q >= af)
                            {
                                t = ac;
                                q = af;
                            }
                        }
                    }
                }

                FluidLevel u = getWaterLevel(r);
                FluidLevel v = getWaterLevel(s);
                FluidLevel w = getWaterLevel(t);

                double x = maxDistance((int)o, (int)p);
                double z = maxDistance((int)o, (int)q);
                double ac = maxDistance((int)p, (int)q);

                if (u.getBlockState(j).isOf(BlockType::WATER) &&
                    fluidLevelSampler_(i, j - 1, k).getBlockState(j - 1).isOf(BlockType::LAVA))
                {
                    f = 1.0;
                }
                else if (x > -1.0)
                {
                    double ab = std::numeric_limits<double>::quiet_NaN();
                    double g = calculateDensity(i, j, k, ab, u, v);
                    double ad = calculateDensity(i, j, k, ab, u, w);
                    double af = calculateDensity(i, j, k, ab, v, w);
                    double h = std::max(0.0, x);
                    double ag = std::max(0.0, z);
                    double ah = std::max(0.0, ac);
                    double ai = 2.0 * h * std::max(g, std::max(ad * ag, af * ah));
                    f = std::max(0.0, ai);
                }
                else
                {
                    f = 0.0;
                }

                blockState = u.getBlockState(j);
            }

            if (e + f <= 0.0)
                return blockState;
        }

        return Blocks::NONE();
    }

    FluidLevel AquiferSamplerImpl::getWaterLevel(int64_t l)
    {
        int i = BlockPos::unpackLongX(l);
        int j = BlockPos::unpackLongY(l);
        int k = BlockPos::unpackLongZ(l);
        int m = getLocalX(i);
        int n = getLocalY(j);
        int o = getLocalZ(k);
        int p = index(m, n, o);

        FluidLevel fluidLevel = waterLevels_[p];
        if (fluidLevel.y != std::numeric_limits<int>::min())
            return fluidLevel;

        FluidLevel fluidLevel2 = getFluidLevel(i, j, k);
        waterLevels_[p] = fluidLevel2;
        return fluidLevel2;
    }

    double AquiferSamplerImpl::maxDistance(int d1, int d2)
    {
        return 1.0 - static_cast<double>(std::abs(d2 - d1)) / 25.0;
    }

    double AquiferSamplerImpl::calculateDensity(int x, int y, int z, double &barrierCache,
                                                const FluidLevel &f1, const FluidLevel &f2)
    {
        BlockState s1 = f1.getBlockState(y);
        BlockState s2 = f2.getBlockState(y);

        if ((s1.isOf(BlockType::LAVA) && s2.isOf(BlockType::WATER)) ||
            (s1.isOf(BlockType::WATER) && s2.isOf(BlockType::LAVA)))
            return 1.0;

        int dy = std::abs(f1.y - f2.y);
        if (dy == 0)
            return 0.0;

        double mid = 0.5 * (f1.y + f2.y);
        double relY = y + 0.5 - mid;
        double q = dy * 0.5 - std::abs(relY);
        double s;

        if (relY > 0.0)
        {
            double r = q;
            s = (r > 0.0) ? r * (1.0 / 1.5) : r * (1.0 / 2.5);
        }
        else
        {
            double r = 3.0 + q;
            s = (r > 0.0) ? r * (1.0 / 3.0) : r * (1.0 / 10.0);
        }

        if (s < -2.0 || s > 2.0)
            return s;

        if (barrierCache != barrierCache)
            barrierCache = barrierNoise_.sample(x, y * 0.5, z);
        return barrierCache + s;
    }

    FluidLevel AquiferSamplerImpl::getFluidLevel(int x, int y, int z)
    {
        FluidLevel base = fluidLevelSampler_(x, y, z);
        int lowestSurface = std::numeric_limits<int>::max();
        int yPlus12 = y + 12;
        int yMinus12 = y - 12;
        bool foundCenter = false;

        for (auto &off : NEIGHBOUR_OFFSETS)
        {
            int nx = x + off[0] * GRID_SIZE_X;
            int nz = z + off[1] * GRID_SIZE_Z;
            int surfaceY = chunkNoiseSampler_->method_39900(nx, nz);
            int checkY = surfaceY + 8;
            bool isCenter = (off[0] == 0 && off[1] == 0);

            if (isCenter && yMinus12 > checkY)
                return base;

            bool aboveSurface = yPlus12 > checkY;
            if (aboveSurface || isCenter)
            {
                FluidLevel neighbourFluid = fluidLevelSampler_(nx, checkY, nz);
                if (!neighbourFluid.getBlockState(checkY).isAir())
                {
                    if (isCenter)
                        foundCenter = true;
                    if (aboveSurface)
                        return neighbourFluid;
                }
            }
            lowestSurface = std::min(lowestSurface, surfaceY);
        }

        int surfaceOffset = lowestSurface + 8 - y;
        double d = foundCenter
                       ? MathHelper::clampedLerpFromProgress(surfaceOffset, 0.0, 64.0, 1.0, 0.0)
                       : 0.0;
        double floodedness = MathHelper::clamp(floodednessNoise_.sample(x, y * 0.67, z), -1.0, 1.0);
        double threshold1 = MathHelper::lerpFromProgress(d, 1.0, 0.0, -0.3, 0.8);

        if (floodedness > threshold1)
            return base;

        double threshold2 = MathHelper::lerpFromProgress(d, 1.0, 0.0, -0.8, 0.4);
        if (floodedness <= threshold2)
            return FluidLevel(-32512, base.state);

        int gx = MathHelper::floorDiv(x, 16);
        int gy = MathHelper::floorDiv(y, 40);
        int gz = MathHelper::floorDiv(z, 16);
        int centerY = gy * 40 + 20;
        double spread = spreadNoise_.sample(gx, gy / 1.4, gz) * 10.0;
        int spreadRounded = MathHelper::roundDownToMultiple(spread, 3);
        int targetY = centerY + spreadRounded;
        int finalY = std::min(lowestSurface, targetY);
        BlockState fluidState = determineFluidType(x, y, z, base, targetY);
        return FluidLevel(finalY, fluidState);
    }

    BlockState AquiferSamplerImpl::determineFluidType(int x, int y, int z,
                                                      const FluidLevel &base,
                                                      int targetY)
    {
        if (targetY <= -10)
        {
            int cx = MathHelper::floorDiv(x, 64);
            int cy = MathHelper::floorDiv(y, 40);
            int cz = MathHelper::floorDiv(z, 64);
            double lava = lavaNoise_.sample(cx, cy, cz);
            if (std::abs(lava) > 0.3)
                return BlockState{BlockType::LAVA};
        }
        return base.state;
    }

} // namespace mc