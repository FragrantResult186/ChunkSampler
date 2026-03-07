#pragma once

#include "math_helper.hpp"
#include "abstract_random.hpp"
#include <vector>
#include <memory>
#include <cmath>
#include <stdexcept>
#include <algorithm>

namespace mc
{

    struct SimplexNoiseSampler
    {
        static constexpr int GRADIENTS[16][3] = {
            {1, 1, 0}, {-1, 1, 0}, {1, -1, 0}, {-1, -1, 0}, {1, 0, 1}, {-1, 0, 1}, {1, 0, -1}, {-1, 0, -1}, {0, 1, 1}, {0, -1, 1}, {0, 1, -1}, {0, -1, -1}, {1, 1, 0}, {0, -1, 1}, {-1, 1, 0}, {0, -1, -1}};

        static double dot(const int g[3], double dx, double dy, double dz)
        {
            return (double)g[0] * dx + (double)g[1] * dy + (double)g[2] * dz;
        }
    };

    class PerlinNoiseSampler
    {
    public:
        double originX, originY, originZ;

        explicit PerlinNoiseSampler(AbstractRandom &rand);

        double sample(double d, double e, double f, double g, double h) const;

    private:
        uint8_t perm_[512];
        uint8_t permGrad16_[512];

        int grad(int i) const { return perm_[i & 255]; }
        double sampleInternal(int x, int y, int z,
                              double dx, double dy, double dz, double fadeY) const;
    };

    class OctavePerlinNoiseSampler
    {
    public:
        static OctavePerlinNoiseSampler createLegacy(AbstractRandom &rand, int firstOctave, int lastOctave);
        static OctavePerlinNoiseSampler create(AbstractRandom &rand, int firstOctave,
                                               const std::vector<double> &amps);

        double sample(double d, double e, double f,
                      double yScale = 0.0, double yMax = 0.0, bool useOriginY = false) const;

        PerlinNoiseSampler *getOctave(int i) const;

        static double maintainPrecision(double d);

    private:
        std::vector<std::unique_ptr<PerlinNoiseSampler>> samplers_;
        std::vector<double> amplitudes_;
        double lacunarity_;
        double persistence_;
        std::vector<double> precompFreq_;
        std::vector<double> precompAmpScale_;

        OctavePerlinNoiseSampler(AbstractRandom &rand, int firstOctaveIndex,
                                 const std::vector<double> &amps, bool useDeriver);
    };

    class DoublePerlinNoiseSampler
    {
    public:
        struct NoiseParameters
        {
            int firstOctave;
            std::vector<double> amplitudes;

            NoiseParameters(int firstOctave, std::initializer_list<double> amps)
                : firstOctave(firstOctave), amplitudes(amps) {}
        };

        static DoublePerlinNoiseSampler create(AbstractRandom &rand, const NoiseParameters &params);

        double sample(double d, double e, double f) const;

    private:
        OctavePerlinNoiseSampler first_;
        OctavePerlinNoiseSampler second_;
        double amplitude_;

        DoublePerlinNoiseSampler(AbstractRandom &rand, int firstOctave,
                                 const std::vector<double> &amps);
    };

    struct NoiseSamplingConfig
    {
        double xzScale, yScale, xzFactor, yFactor;

        NoiseSamplingConfig(double xzScale, double yScale, double xzFactor, double yFactor)
            : xzScale(xzScale), yScale(yScale), xzFactor(xzFactor), yFactor(yFactor) {}
    };

    class InterpolatedNoiseSampler
    {
    public:
        InterpolatedNoiseSampler(AbstractRandom &rand, const NoiseSamplingConfig &cfg,
                                 int cellWidth, int cellHeight);

        double calculateNoise(int i, int j, int k) const;

    private:
        OctavePerlinNoiseSampler lower_, upper_, interp_;
        double xzScale_, yScale_, xzMainScale_, yMainScale_;
        int cellWidth_, cellHeight_;
    };

} // namespace mc