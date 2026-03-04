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
        int gx0 = grad(x);
        int gx1 = grad(x + 1);

        int gy00 = grad(gx0 + y);
        int gy01 = grad(gx0 + y + 1);
        int gy10 = grad(gx1 + y);
        int gy11 = grad(gx1 + y + 1);

        double n000 = SimplexNoiseSampler::dot(SimplexNoiseSampler::GRADIENTS[grad(gy00 + z) & 15], dx, dy, dz);
        double n100 = SimplexNoiseSampler::dot(SimplexNoiseSampler::GRADIENTS[grad(gy10 + z) & 15], dx - 1, dy, dz);
        double n010 = SimplexNoiseSampler::dot(SimplexNoiseSampler::GRADIENTS[grad(gy01 + z) & 15], dx, dy - 1, dz);
        double n110 = SimplexNoiseSampler::dot(SimplexNoiseSampler::GRADIENTS[grad(gy11 + z) & 15], dx - 1, dy - 1, dz);
        double n001 = SimplexNoiseSampler::dot(SimplexNoiseSampler::GRADIENTS[grad(gy00 + z + 1) & 15], dx, dy, dz - 1);
        double n101 = SimplexNoiseSampler::dot(SimplexNoiseSampler::GRADIENTS[grad(gy10 + z + 1) & 15], dx - 1, dy, dz - 1);
        double n011 = SimplexNoiseSampler::dot(SimplexNoiseSampler::GRADIENTS[grad(gy01 + z + 1) & 15], dx, dy - 1, dz - 1);
        double n111 = SimplexNoiseSampler::dot(SimplexNoiseSampler::GRADIENTS[grad(gy11 + z + 1) & 15], dx - 1, dy - 1, dz - 1);

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
        double freq = lacunarity_;
        double amp = persistence_;

        for (size_t i = 0; i < samplers_.size(); ++i)
        {
            if (samplers_[i])
            {
                double m = samplers_[i]->sample(
                    maintainPrecision(d * freq),
                    useOriginY ? -samplers_[i]->originY : maintainPrecision(e * freq),
                    maintainPrecision(f * freq),
                    yScale * freq,
                    yMax * freq);
                result += amplitudes_[i] * m * amp;
            }
            freq *= 2.0;
            amp /= 2.0;
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