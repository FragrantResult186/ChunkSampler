#include "chunk_noise_sampler.hpp"
#include "noise_column_sampler.hpp"
#include "aquifer_sampler.hpp"
#include "terrain.hpp"

namespace mc
{

    void ChunkNoiseSampler::sampleStartNoise()
    {
        for (auto *i : interpolators_)
            if (i)
                i->sampleStartNoise();
    }
    void ChunkNoiseSampler::sampleEndNoise(int i)
    {
        for (auto *p : interpolators_)
            if (p)
                p->sampleEndNoise(i);
    }
    void ChunkNoiseSampler::sampleNoiseCorners(int s, int r)
    {
        for (auto *i : interpolators_)
            if (i)
                i->sampleNoiseCorners(s, r);
    }
    void ChunkNoiseSampler::sampleNoiseY(double d)
    {
        for (auto *i : interpolators_)
            if (i)
                i->sampleNoiseY(d);
    }
    void ChunkNoiseSampler::sampleNoiseX(double d)
    {
        for (auto *i : interpolators_)
            if (i)
                i->sampleNoiseX(d);
    }
    void ChunkNoiseSampler::sampleNoise(double d)
    {
        for (auto *i : interpolators_)
            if (i)
                i->sampleNoise(d);
    }
    void ChunkNoiseSampler::swapBuffers()
    {
        for (auto *i : interpolators_)
            if (i)
                i->swapBuffers();
    }

    int ChunkNoiseSampler::method_39900(int blockX, int blockZ)
    {
        uint64_t key = ChunkPos::toLong(
            BiomeCoords::fromBlock(blockX),
            BiomeCoords::fromBlock(blockZ));

        int cached = field36273_.find(key);
        if (cached != SurfaceCache::EMPTY)
            return cached;

        int bx = ChunkPos::getPackedX(key);
        int bz = ChunkPos::getPackedZ(key);
        int ki = bx - biomeX_;
        int kj = bz - biomeZ_;
        int n = static_cast<int>(field35486_.size());

        TerrainNoisePoint tp = (ki >= 0 && kj >= 0 && ki < n && kj < n)
                                   ? field35486_[ki][kj].terrainNoisePoint
                                   : noiseColumnSampler_->sampleTerrainNoisePoint(bx, bz).terrainNoisePoint;

        int result = noiseColumnSampler_->findSurfaceYFromNoise(
            BiomeCoords::toBlock(bx),
            BiomeCoords::toBlock(bz),
            tp);

        field36273_.insert(key, result);
        return result;
    }

    BlockState ChunkNoiseSampler::applyAquifer(int x, int y, int z, double density, double extra)
    {
        if (aquifer_)
            return aquifer_->apply(x, y, z, density, extra);
        return (extra > 0.0) ? Blocks::STONE() : Blocks::AIR();
    }

} // namespace mc