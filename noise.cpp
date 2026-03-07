#include "noise.hpp"
#include <cstring>

namespace mc
{

    // ---- PerlinNoiseSampler ---------------------------------------------------------------

    PerlinNoiseSampler::PerlinNoiseSampler(AbstractRandom &rand)
    {
        originX = rand.nextDouble() * 256.0;
        originY = rand.nextDouble() * 256.0;
        originZ = rand.nextDouble() * 256.0;

        for (int i = 0; i < 256; ++i)
            perm_[i] = (uint8_t)i;
        for (int i = 0; i < 256; ++i)
        {
            int j = rand.nextInt(256 - i);
            std::swap(perm_[i], perm_[i + j]);
        }
        for (int i = 0; i < 256; ++i)
            perm_[i + 256] = perm_[i];
        for (int i = 0; i < 512; ++i)
            permGrad16_[i] = perm_[i] & 15;
    }

    double PerlinNoiseSampler::sample(double d, double e, double f, double g, double h) const
    {
        double ix = d + originX;
        double iy = e + originY;
        double iz = f + originZ;
        int li = MathHelper::floor(ix);
        int mi = MathHelper::floor(iy);
        int ni = MathHelper::floor(iz);
        double o = ix - li;
        double p = iy - mi;
        double q = iz - ni;

        double s;
        if (g != 0.0)
        {
            double r = (h >= 0.0 && h < p) ? h : p;
            s = (double)MathHelper::floor(r / g + 1e-7) * g;
        }
        else
        {
            s = 0.0;
        }

        return sampleInternal(li, mi, ni, o, p - s, q, p);
    }

    double PerlinNoiseSampler::sampleInternal(int x, int y, int z,
                                              double dx, double dy, double dz, double fadeY) const
    {
        int gx0 = perm_[x & 255];
        int gx1 = perm_[(x + 1) & 255];

        int gy00 = perm_[(gx0 + y) & 255];
        int gy01 = perm_[(gx0 + y + 1) & 255];
        int gy10 = perm_[(gx1 + y) & 255];
        int gy11 = perm_[(gx1 + y + 1) & 255];

        const int g000 = permGrad16_[(gy00 + z) & 255];
        const int g100 = permGrad16_[(gy10 + z) & 255];
        const int g010 = permGrad16_[(gy01 + z) & 255];
        const int g110 = permGrad16_[(gy11 + z) & 255];
        const int g001 = permGrad16_[(gy00 + z + 1) & 255];
        const int g101 = permGrad16_[(gy10 + z + 1) & 255];
        const int g011 = permGrad16_[(gy01 + z + 1) & 255];
        const int g111 = permGrad16_[(gy11 + z + 1) & 255];

        const double dx1 = dx - 1.0, dy1 = dy - 1.0, dz1 = dz - 1.0;
        const auto *G = SimplexNoiseSampler::GRADIENTS;
        double n000 = G[g000][0] * dx + G[g000][1] * dy + G[g000][2] * dz;
        double n100 = G[g100][0] * dx1 + G[g100][1] * dy + G[g100][2] * dz;
        double n010 = G[g010][0] * dx + G[g010][1] * dy1 + G[g010][2] * dz;
        double n110 = G[g110][0] * dx1 + G[g110][1] * dy1 + G[g110][2] * dz;
        double n001 = G[g001][0] * dx + G[g001][1] * dy + G[g001][2] * dz1;
        double n101 = G[g101][0] * dx1 + G[g101][1] * dy + G[g101][2] * dz1;
        double n011 = G[g011][0] * dx + G[g011][1] * dy1 + G[g011][2] * dz1;
        double n111 = G[g111][0] * dx1 + G[g111][1] * dy1 + G[g111][2] * dz1;

        double fx = MathHelper::perlinFade(dx);
        double fy = MathHelper::perlinFade(fadeY);
        double fz = MathHelper::perlinFade(dz);

        return MathHelper::lerp3(fx, fy, fz,
                                 n000, n100, n010, n110,
                                 n001, n101, n011, n111);
    }

    // ---- OctavePerlinNoiseSampler ---------------------------------------------------------

    OctavePerlinNoiseSampler OctavePerlinNoiseSampler::createLegacy(
        AbstractRandom &rand, int firstOctave, int lastOctave)
    {
        int minOct = std::min(firstOctave, lastOctave);
        int maxOct = std::max(firstOctave, lastOctave);
        int count = maxOct - minOct + 1;
        std::vector<double> amps(count, 1.0);
        return OctavePerlinNoiseSampler(rand, minOct, amps, false);
    }

    OctavePerlinNoiseSampler OctavePerlinNoiseSampler::create(
        AbstractRandom &rand, int firstOctave, const std::vector<double> &amps)
    {
        return OctavePerlinNoiseSampler(rand, firstOctave, amps, true);
    }

    OctavePerlinNoiseSampler::OctavePerlinNoiseSampler(
        AbstractRandom &rand, int firstOctaveIndex,
        const std::vector<double> &amps, bool useDeriver)
    {
        int count = (int)amps.size();
        amplitudes_ = amps;
        samplers_.resize(count);

        lacunarity_ = std::pow(2.0, (double)firstOctaveIndex);
        persistence_ = std::pow(2.0, count - 1) / (std::pow(2.0, count) - 1.0);

        precompFreq_.resize(count);
        precompAmpScale_.resize(count);
        double f = lacunarity_, a = persistence_;
        for (int i = 0; i < count; ++i, f *= 2.0, a /= 2.0)
        {
            precompFreq_[i] = f;
            precompAmpScale_[i] = amplitudes_[i] * a;
        }

        if (useDeriver)
        {
            auto deriver = rand.createRandomDeriver();
            for (int i = 0; i < count; ++i)
            {
                if (amps[i] != 0.0)
                {
                    char key[64];
                    snprintf(key, sizeof(key), "octave_%d", firstOctaveIndex + i);
                    auto r = deriver->createRandom(std::string(key));
                    samplers_[i] = std::make_unique<PerlinNoiseSampler>(*r);
                }
            }
        }
        else
        {
            auto mainSampler = std::make_unique<PerlinNoiseSampler>(rand);
            int mainIndex = -firstOctaveIndex;

            if (mainIndex >= 0 && mainIndex < count && amplitudes_[mainIndex] != 0.0)
                samplers_[mainIndex] = std::move(mainSampler);

            for (int k = mainIndex - 1; k >= 0; --k)
            {
                if (k < count && amplitudes_[k] != 0.0)
                    samplers_[k] = std::make_unique<PerlinNoiseSampler>(rand);
            }
        }
    }

    double OctavePerlinNoiseSampler::sample(double d, double e, double f,
                                            double yScale, double yMax, bool useOriginY) const
    {
        double result = 0.0;

        for (size_t i = 0; i < samplers_.size(); ++i)
        {
            if (samplers_[i])
            {
                const double freq = precompFreq_[i];
                double m = samplers_[i]->sample(
                    maintainPrecision(d * freq),
                    useOriginY ? -samplers_[i]->originY : maintainPrecision(e * freq),
                    maintainPrecision(f * freq),
                    yScale * freq,
                    yMax * freq);
                result += precompAmpScale_[i] * m;
            }
        }
        return result;
    }

    PerlinNoiseSampler *OctavePerlinNoiseSampler::getOctave(int i) const
    {
        int idx = (int)samplers_.size() - 1 - i;
        if (idx < 0 || idx >= (int)samplers_.size())
            return nullptr;
        return samplers_[idx].get();
    }

    double OctavePerlinNoiseSampler::maintainPrecision(double d)
    {
        return d - (double)MathHelper::lfloor(d / 33554432.0 + 0.5) * 33554432.0;
    }

    // ---- DoublePerlinNoiseSampler ---------------------------------------------------------

    DoublePerlinNoiseSampler DoublePerlinNoiseSampler::create(
        AbstractRandom &rand, const NoiseParameters &params)
    {
        return DoublePerlinNoiseSampler(rand, params.firstOctave, params.amplitudes);
    }

    DoublePerlinNoiseSampler::DoublePerlinNoiseSampler(
        AbstractRandom &rand, int firstOctave, const std::vector<double> &amps)
        : first_(OctavePerlinNoiseSampler::create(rand, firstOctave, amps)),
          second_(OctavePerlinNoiseSampler::create(rand, firstOctave, amps))
    {
        int minIdx = -1, maxIdx = -1;
        for (size_t i = 0; i < amps.size(); ++i)
        {
            if (amps[i] != 0.0)
            {
                if (minIdx < 0)
                    minIdx = i;
                maxIdx = i;
            }
        }
        double range = (maxIdx >= 0 && minIdx >= 0) ? (maxIdx - minIdx) : 0;
        amplitude_ = 0.16666666666666666 / (0.1 * (1.0 + 1.0 / (range + 1.0)));
    }

    double DoublePerlinNoiseSampler::sample(double d, double e, double f) const
    {
        double g = d * 1.0181268882175227;
        double h = e * 1.0181268882175227;
        double i = f * 1.0181268882175227;
        return (first_.sample(d, e, f) + second_.sample(g, h, i)) * amplitude_;
    }

    // ---- InterpolatedNoiseSampler ---------------------------------------------------------

    InterpolatedNoiseSampler::InterpolatedNoiseSampler(
        AbstractRandom &rand, const NoiseSamplingConfig &cfg,
        int cellWidth, int cellHeight)
        : lower_(OctavePerlinNoiseSampler::createLegacy(rand, -15, 0)),
          upper_(OctavePerlinNoiseSampler::createLegacy(rand, -15, 0)),
          interp_(OctavePerlinNoiseSampler::createLegacy(rand, -7, 0)),
          xzScale_(684.412 * cfg.xzScale),
          yScale_(684.412 * cfg.yScale),
          xzMainScale_(684.412 * cfg.xzScale / cfg.xzFactor),
          yMainScale_(684.412 * cfg.yScale / cfg.yFactor),
          cellWidth_(cellWidth),
          cellHeight_(cellHeight)
    {
    }

    double InterpolatedNoiseSampler::calculateNoise(int i, int j, int k) const
    {
        int l = MathHelper::floor((double)i / cellWidth_);
        int m = MathHelper::floor((double)j / cellHeight_);
        int n = MathHelper::floor((double)k / cellWidth_);

        double d = 0, e = 0, f = 0, g = 1.0;
        for (int o = 0; o < 8; ++o)
        {
            PerlinNoiseSampler *ps = interp_.getOctave(o);
            if (ps)
            {
                f += ps->sample(
                         OctavePerlinNoiseSampler::maintainPrecision((double)l * xzMainScale_ * g),
                         OctavePerlinNoiseSampler::maintainPrecision((double)m * yMainScale_ * g),
                         OctavePerlinNoiseSampler::maintainPrecision((double)n * xzMainScale_ * g),
                         yMainScale_ * g,
                         (double)m * yMainScale_ * g) /
                     g;
            }
            g /= 2.0;
        }

        double h = (f / 10.0 + 1.0) / 2.0;
        bool upper = h >= 1.0;
        bool lower = h <= 0.0;
        g = 1.0;

        for (int p = 0; p < 16; ++p)
        {
            double q = OctavePerlinNoiseSampler::maintainPrecision((double)l * xzScale_ * g);
            double r = OctavePerlinNoiseSampler::maintainPrecision((double)m * yScale_ * g);
            double s = OctavePerlinNoiseSampler::maintainPrecision((double)n * xzScale_ * g);
            double t = yScale_ * g;

            if (!upper)
            {
                PerlinNoiseSampler *ps = lower_.getOctave(p);
                if (ps)
                    d += ps->sample(q, r, s, t, (double)m * t) / g;
            }
            if (!lower)
            {
                PerlinNoiseSampler *ps = upper_.getOctave(p);
                if (ps)
                    e += ps->sample(q, r, s, t, (double)m * t) / g;
            }
            g /= 2.0;
        }

        return MathHelper::clampedLerp(d / 512.0, e / 512.0, h) / 128.0;
    }

} // namespace mc