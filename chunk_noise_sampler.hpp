#pragma once

#include "math_helper.hpp"
#include "block_pos.hpp"
#include "noise.hpp"
#include "terrain.hpp"
#include "chunk.hpp"

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc
{

    class AquiferSampler;
    class NoiseColumnSampler;

    struct FluidLevel
    {
        int y;
        BlockState state;

        FluidLevel() : y(std::numeric_limits<int>::min()), state(Blocks::AIR()) {}
        FluidLevel(int y_, BlockState s) : y(y_), state(s) {}

        BlockState getBlockState(int blockY) const
        {
            return blockY < y ? state : Blocks::AIR();
        }
    };

    using FluidLevelSamplerFn = std::function<FluidLevel(int, int, int)>;

    class ChunkNoiseSampler
    {
    public:
        using ColumnSampler = std::function<double(int, int, int)>;
        using BlockStateSampler = std::function<BlockState(int, int, int)>;
        using ValueSampler = std::function<double()>;
        using ValueSamplerFactory = std::function<ValueSampler(ChunkNoiseSampler &)>;

        std::unique_ptr<AquiferSampler> aquifer_;

        // ---- NoiseInterpolator ----
        class NoiseInterpolator
        {
        public:
            NoiseInterpolator(ChunkNoiseSampler &parent, ColumnSampler columnSampler)
                : parent_(parent),
                  columnSampler_(std::move(columnSampler))
            {
                startBuf_ = makeBuffer(parent.height_, parent.horizontalSize_);
                endBuf_ = makeBuffer(parent.height_, parent.horizontalSize_);
                parent_.interpolators_.push_back(this);
            }

            void sampleStartNoise() { sampleInto(startBuf_, parent_.x_); }
            void sampleEndNoise(int i) { sampleInto(endBuf_, parent_.x_ + i + 1); }

            void sampleNoiseCorners(int s, int r)
            {
                x0y0z0_ = startBuf_[r][s];
                x0y0z1_ = startBuf_[r + 1][s];
                x1y0z0_ = endBuf_[r][s];
                x1y0z1_ = endBuf_[r + 1][s];
                x0y1z0_ = startBuf_[r][s + 1];
                x0y1z1_ = startBuf_[r + 1][s + 1];
                x1y1z0_ = endBuf_[r][s + 1];
                x1y1z1_ = endBuf_[r + 1][s + 1];
            }

            void sampleNoiseY(double d)
            {
                x0z0_ = MathHelper::lerp(d, x0y0z0_, x0y1z0_);
                x1z0_ = MathHelper::lerp(d, x1y0z0_, x1y1z0_);
                x0z1_ = MathHelper::lerp(d, x0y0z1_, x0y1z1_);
                x1z1_ = MathHelper::lerp(d, x1y0z1_, x1y1z1_);
            }

            void sampleNoiseX(double d)
            {
                z0_ = MathHelper::lerp(d, x0z0_, x1z0_);
                z1_ = MathHelper::lerp(d, x0z1_, x1z1_);
            }

            void sampleNoise(double d) { result_ = MathHelper::lerp(d, z0_, z1_); }

            double sample() const { return result_; }

            void swapBuffers() { std::swap(startBuf_, endBuf_); }

        private:
            ChunkNoiseSampler &parent_;
            ColumnSampler columnSampler_;

            std::vector<std::vector<double>> startBuf_;
            std::vector<std::vector<double>> endBuf_;

            double x0y0z0_, x0y0z1_, x1y0z0_, x1y0z1_;
            double x0y1z0_, x0y1z1_, x1y1z0_, x1y1z1_;
            double x0z0_, x1z0_, x0z1_, x1z1_;
            double z0_, z1_;
            double result_ = 0.0;

            static std::vector<std::vector<double>> makeBuffer(int height, int hSize)
            {
                int w = hSize + 1;
                int h = height + 1;
                return std::vector<std::vector<double>>(w, std::vector<double>(h, 0.0));
            }

            void sampleInto(std::vector<std::vector<double>> &buf, int xi)
            {
                constexpr int H_BLOCK = 4;
                constexpr int V_BLOCK = 8;

                for (int l = 0; l <= parent_.horizontalSize_; ++l)
                {
                    int mz = parent_.z_ + l;
                    for (int n = 0; n <= parent_.height_; ++n)
                    {
                        int y = n + parent_.minimumY_;
                        int py = y * V_BLOCK;
                        buf[l][n] = columnSampler_(xi * H_BLOCK, py, mz * H_BLOCK);
                    }
                }
            }
        };

        static ChunkNoiseSampler create(
            const ChunkPos &pos,
            NoiseColumnSampler &noiseColumnSampler,
            ColumnSampler columnSampler,
            FluidLevelSamplerFn fluidLevelSampler,
            bool skipCaves = false);

        void sampleStartNoise();
        void sampleEndNoise(int i);
        void sampleNoiseCorners(int s, int r);
        void sampleNoiseY(double d);
        void sampleNoiseX(double d);
        void sampleNoise(double d);
        void swapBuffers();

        BlockState sampleInitialNoiseBlockState(int x, int y, int z)
        {
            return blockStateSampler_(x, y, z);
        }

        int method_39900(int blockX, int blockZ);

        struct MultiNoisePoint
        {
            TerrainNoisePoint terrainNoisePoint;
        };

        const MultiNoisePoint &createMultiNoisePoint(int biomeX, int biomeZ) const
        {
            int ki = biomeX - biomeX_;
            int kj = biomeZ - biomeZ_;
            return field35486_[ki][kj];
        }

        BlockState applyAquifer(int x, int y, int z, double density, double extra);

        int horizontalSize_;
        int height_;
        int minimumY_;
        int x_;
        int z_;
        int biomeX_;
        int biomeZ_;

    private:
        ChunkNoiseSampler() = default;

        std::vector<NoiseInterpolator *> interpolators_;
        std::vector<std::unique_ptr<NoiseInterpolator>> ownedInterpolators_;
        std::vector<std::vector<MultiNoisePoint>> field35486_;
        std::unordered_map<uint64_t, int> field36273_;
        BlockStateSampler blockStateSampler_;
        NoiseColumnSampler *noiseColumnSampler_ = nullptr;
    };

} // namespace mc