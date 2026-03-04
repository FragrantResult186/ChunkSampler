#pragma once

#include "math_helper.hpp"
#include <vector>
#include <memory>
#include <functional>
#include <stdexcept>
#include <algorithm>
#include <cmath>

namespace mc
{

    struct TerrainNoisePoint
    {
        double offset, factor, peaks;
        TerrainNoisePoint(double o, double f, double p) : offset(o), factor(f), peaks(p) {}
    };

    struct BiomeCoords
    {
        static int fromBlock(int i) { return i >> 2; }
        static int toBlock(int i) { return i << 2; }
    };

    struct NoisePoint
    {
        float continentalnessNoise;
        float erosionNoise;
        float normalizedWeirdness;
        float weirdnessNoise;
    };

    class Spline
    {
    public:
        virtual ~Spline() = default;
        virtual float apply(const NoisePoint &p) const = 0;

        static std::shared_ptr<Spline> fixed(float v);

        class Builder;
        static Builder builder(std::function<float(const NoisePoint &)> locationFn,
                               std::function<float(float)> valueFn);
    };

    class FixedSpline : public Spline
    {
    public:
        explicit FixedSpline(float v) : value_(v) {}
        float apply(const NoisePoint &) const override { return value_; }

    private:
        float value_;
    };

    class CubicSpline : public Spline
    {
    public:
        CubicSpline(std::function<float(const NoisePoint &)> loc,
                    std::vector<float> locs,
                    std::vector<std::shared_ptr<Spline>> vals,
                    std::vector<float> derivs);

        float apply(const NoisePoint &p) const override;

    private:
        std::function<float(const NoisePoint &)> loc_;
        std::vector<float> locations_;
        std::vector<std::shared_ptr<Spline>> values_;
        std::vector<float> derivatives_;
    };

    class Spline::Builder
    {
    public:
        Builder(std::function<float(const NoisePoint &)> locFn,
                std::function<float(float)> valFn);

        Builder &add(float loc, float val, float deriv);
        Builder &add(float loc, std::shared_ptr<Spline> spline, float deriv);
        std::shared_ptr<Spline> build();

    private:
        std::function<float(const NoisePoint &)> locFn_;
        std::function<float(float)> valFn_;
        std::vector<float> locations_;
        std::vector<std::shared_ptr<Spline>> values_;
        std::vector<float> derivatives_;
    };

    namespace LocationFn
    {
        inline float CONTINENTS(const NoisePoint &p) { return p.continentalnessNoise; }
        inline float EROSION(const NoisePoint &p) { return p.erosionNoise; }
        inline float WEIRDNESS(const NoisePoint &p) { return p.weirdnessNoise; }
        inline float RIDGES(const NoisePoint &p) { return p.normalizedWeirdness; }
    }

    class VanillaTerrainParameters
    {
    public:
        std::shared_ptr<Spline> offsetSpline;
        std::shared_ptr<Spline> factorSpline;
        std::shared_ptr<Spline> peakSpline;

        static VanillaTerrainParameters createSurfaceParameters();

        float getOffset(const NoisePoint &p) const { return offsetSpline->apply(p) - 0.50375f; }
        float getFactor(const NoisePoint &p) const { return factorSpline->apply(p); }
        float getPeak(const NoisePoint &p) const { return peakSpline->apply(p); }

        NoisePoint createNoisePoint(float f, float g, float h) const
        {
            return {f, g, getNormalizedWeirdness(h), h};
        }

        static float getNormalizedWeirdness(float f)
        {
            return -(std::abs(std::abs(f) - 0.6666667f) - 0.33333334f) * 3.0f;
        }

    private:
        static auto identity()
        {
            return [](float f)
            { return f; };
        }

        static float getOffsetValue(float f, float g, float h)
        {
            float k = 1.0f - (1.0f - g) * 0.5f;
            float l = 0.5f * (1.0f - g);
            float m = (f + 1.17f) * 0.46082947f;
            float n = m * k - l;
            return f < h ? std::max(n, -0.2222f) : std::max(n, 0.0f);
        }

        static float computeRidgeSplitPoint(float f)
        {
            float i = 1.0f - (1.0f - f) * 0.5f;
            float j = 0.5f * (1.0f - f);
            return j / (0.46082947f * i) - 1.17f;
        }

        static std::shared_ptr<Spline> buildErosionFactorSpline(float f, bool bl);
        static std::shared_ptr<Spline> createErosionRidgeWeirdnessSpline(float f, float g, float h, float i);
        static std::shared_ptr<Spline> createRidgeWeirdnessSpline(float f, float g);
        static std::shared_ptr<Spline> createWeirdnessTransitionSpline(float f);
        static std::shared_ptr<Spline> createRidgeOffsetSpline(float f, bool bl);
        static std::shared_ptr<Spline> createLandSpline(float f, float g, float h, float i, float j, float k, bool bl, bool bl2);
        static std::shared_ptr<Spline> createFlatOffsetSpline(float f, float g, float h, float i, float j, float k);
    };

} // namespace mc