#include "terrain.hpp"

namespace mc
{

    // ---- Spline static methods ------------------------------------------------------------

    std::shared_ptr<Spline> Spline::fixed(float v)
    {
        return std::make_shared<FixedSpline>(v);
    }

    Spline::Builder Spline::builder(
        std::function<float(const NoisePoint &)> locationFn,
        std::function<float(float)> valueFn)
    {
        return Builder(std::move(locationFn), std::move(valueFn));
    }

    // ---- CubicSpline ----------------------------------------------------------------------

    CubicSpline::CubicSpline(std::function<float(const NoisePoint &)> loc,
                             std::vector<float> locs,
                             std::vector<std::shared_ptr<Spline>> vals,
                             std::vector<float> derivs)
        : loc_(std::move(loc)),
          locations_(std::move(locs)),
          values_(std::move(vals)),
          derivatives_(std::move(derivs))
    {
    }

    float CubicSpline::apply(const NoisePoint &p) const
    {
        float f = loc_(p);
        int n = static_cast<int>(locations_.size());

        int i = MathHelper::binarySearch(0, n, [&](int idx){ 
            return f < locations_[idx]; 
        }) - 1;
        int j = n - 1;

        if (i < 0)
            return values_[0]->apply(p) + derivatives_[0] * (f - locations_[0]);
        if (i == j)
            return values_[j]->apply(p) + derivatives_[j] * (f - locations_[j]);

        float x0 = locations_[i];
        float x1 = locations_[i + 1];
        float t = (f - x0) / (x1 - x0);

        float v0 = values_[i]->apply(p);
        float v1 = values_[i + 1]->apply(p);
        float d0 = derivatives_[i];
        float d1 = derivatives_[i + 1];

        float pVal = d0 * (x1 - x0) - (v1 - v0);
        float qVal = -d1 * (x1 - x0) + (v1 - v0);

        return MathHelper::lerp(t, v0, v1) + t * (1.0f - t) * MathHelper::lerp(t, pVal, qVal);
    }

    // ---- Spline::Builder ------------------------------------------------------------------

    Spline::Builder::Builder(std::function<float(const NoisePoint &)> locFn,
                             std::function<float(float)> valFn)
        : locFn_(std::move(locFn)), valFn_(std::move(valFn))
    {
    }

    Spline::Builder &Spline::Builder::add(float loc, float val, float deriv)
    {
        return add(loc, Spline::fixed(valFn_(val)), deriv);
    }

    Spline::Builder &Spline::Builder::add(float loc, std::shared_ptr<Spline> spline, float deriv)
    {
        if (!locations_.empty() && loc <= locations_.back())
            throw std::invalid_argument("Points must be in ascending order");
        locations_.push_back(loc);
        values_.push_back(std::move(spline));
        derivatives_.push_back(deriv);
        return *this;
    }

    std::shared_ptr<Spline> Spline::Builder::build()
    {
        if (locations_.empty())
            throw std::runtime_error("No elements added");
        return std::make_shared<CubicSpline>(locFn_, locations_, values_, derivatives_);
    }

    // ---- VanillaTerrainParameters private helpers ----------------------------------------

    std::shared_ptr<Spline> VanillaTerrainParameters::createWeirdnessTransitionSpline(float f)
    {
        float g = 0.63f * f;
        float h = 0.3f * f;
        return Spline::builder(LocationFn::WEIRDNESS, identity())
            .add(-0.01f, g, 0.0f)
            .add(0.01f, h, 0.0f)
            .build();
    }

    std::shared_ptr<Spline> VanillaTerrainParameters::createRidgeWeirdnessSpline(float f, float g)
    {
        float h = getNormalizedWeirdness(0.4f);
        float i = getNormalizedWeirdness(0.56666666f);
        float j = (h + i) / 2.0f;

        auto builder = Spline::builder(LocationFn::RIDGES, identity());
        builder.add(h, 0.0f, 0.0f);

        if (g > 0.0f)
            builder.add(j, createWeirdnessTransitionSpline(g), 0.0f);
        else
            builder.add(j, 0.0f, 0.0f);

        if (f > 0.0f)
            builder.add(1.0f, createWeirdnessTransitionSpline(f), 0.0f);
        else
            builder.add(1.0f, 0.0f, 0.0f);

        return builder.build();
    }

    std::shared_ptr<Spline> VanillaTerrainParameters::createErosionRidgeWeirdnessSpline(float f, float g, float h, float i)
    {
        auto s1 = createRidgeWeirdnessSpline(f, h);
        auto s2 = createRidgeWeirdnessSpline(g, i);
        return Spline::builder(LocationFn::EROSION, identity())
            .add(-1.0f, s1, 0.0f)
            .add(-0.78f, s2, 0.0f)
            .add(-0.5775f, s2, 0.0f)
            .add(-0.375f, 0.0f, 0.0f)
            .build();
    }

    std::shared_ptr<Spline> VanillaTerrainParameters::buildErosionFactorSpline(float f, bool bl)
    {
        auto baseWeirdnessSpline =
            Spline::builder(LocationFn::WEIRDNESS, identity())
                .add(-0.2f, 6.3f, 0.0f)
                .add(0.2f, f, 0.0f)
                .build();

        auto builder =
            Spline::builder(LocationFn::EROSION, identity())
                .add(-0.6f, baseWeirdnessSpline, 0.0f)
                .add(-0.5f,
                     Spline::builder(LocationFn::WEIRDNESS, identity())
                         .add(-0.05f, 6.3f, 0.0f)
                         .add(0.05f, 2.67f, 0.0f)
                         .build(),
                     0.0f)
                .add(-0.35f, baseWeirdnessSpline, 0.0f)
                .add(-0.25f, baseWeirdnessSpline, 0.0f)
                .add(-0.1f,
                     Spline::builder(LocationFn::WEIRDNESS, identity())
                         .add(-0.05f, 2.67f, 0.0f)
                         .add(0.05f, 6.3f, 0.0f)
                         .build(),
                     0.0f)
                .add(0.03f, baseWeirdnessSpline, 0.0f);

        if (bl)
        {
            auto weirdnessMid =
                Spline::builder(LocationFn::WEIRDNESS, identity())
                    .add(0.0f, f, 0.0f)
                    .add(0.1f, 0.625f, 0.0f)
                    .build();
            auto ridgeHigh =
                Spline::builder(LocationFn::RIDGES, identity())
                    .add(-0.9f, f, 0.0f)
                    .add(-0.69f, weirdnessMid, 0.0f)
                    .build();
            builder
                .add(0.35f, f, 0.0f)
                .add(0.45f, ridgeHigh, 0.0f)
                .add(0.55f, ridgeHigh, 0.0f)
                .add(0.62f, f, 0.0f);
        }
        else
        {
            auto ridgeLow =
                Spline::builder(LocationFn::RIDGES, identity())
                    .add(-0.7f, baseWeirdnessSpline, 0.0f)
                    .add(-0.15f, 1.37f, 0.0f)
                    .build();
            auto ridgeMid =
                Spline::builder(LocationFn::RIDGES, identity())
                    .add(0.45f, baseWeirdnessSpline, 0.0f)
                    .add(0.7f, 1.56f, 0.0f)
                    .build();
            builder
                .add(0.05f, ridgeMid, 0.0f)
                .add(0.4f, ridgeMid, 0.0f)
                .add(0.45f, ridgeLow, 0.0f)
                .add(0.55f, ridgeLow, 0.0f)
                .add(0.58f, f, 0.0f);
        }

        return builder.build();
    }

    std::shared_ptr<Spline> VanillaTerrainParameters::createRidgeOffsetSpline(float f, bool bl)
    {
        auto builder = Spline::builder(LocationFn::RIDGES, identity());

        float i = getOffsetValue(-1.0f, f, -0.7f);
        float k = getOffsetValue(1.0f, f, -0.7f);
        float l = computeRidgeSplitPoint(f);

        if (-0.65f < l && l < 1.0f)
        {
            float n = getOffsetValue(-0.65f, f, -0.7f);
            float p = getOffsetValue(-0.75f, f, -0.7f);
            float q = (p - i) / 0.25f;
            builder.add(-1.0f, i, q)
                .add(-0.75f, p, 0.0f)
                .add(-0.65f, n, 0.0f);

            float r = getOffsetValue(l, f, -0.7f);
            float s = (k - r) / (1.0f - l);
            builder.add(l, r, s)
                .add(1.0f, k, s);
        }
        else
        {
            float n = (k - i) / 2.0f;
            if (bl)
                builder.add(-1.0f, std::max(0.2f, i), 0.0f)
                    .add(0.0f, MathHelper::lerp(0.5f, i, k), n);
            else
                builder.add(-1.0f, i, n);
            builder.add(1.0f, k, n);
        }

        return builder.build();
    }

    std::shared_ptr<Spline> VanillaTerrainParameters::createFlatOffsetSpline(
        float f, float g, float h, float i, float j, float k)
    {
        float l = std::max(0.5f * (g - f), k);
        float m = 5.0f * (h - g);
        return Spline::builder(LocationFn::RIDGES, identity())
            .add(-1.0f, f, l)
            .add(-0.4f, g, std::min(l, m))
            .add(0.0f, h, m)
            .add(0.4f, i, 2.0f * (i - h))
            .add(1.0f, j, 0.7f * (j - i))
            .build();
    }

    std::shared_ptr<Spline> VanillaTerrainParameters::createLandSpline(
        float f, float g, float h, float i, float j, float k, bool bl, bool bl2)
    {
        auto s1 = createRidgeOffsetSpline(MathHelper::lerp(i, 0.6f, 1.5f), bl2);
        auto s2 = createRidgeOffsetSpline(MathHelper::lerp(i, 0.6f, 1.0f), bl2);
        auto s3 = createRidgeOffsetSpline(i, bl2);
        auto s4 = createFlatOffsetSpline(f - 0.15f, 0.5f * i, 0.5f * i, 0.5f * i, 0.6f * i, 0.5f);
        auto s5 = createFlatOffsetSpline(f, j * i, g * i, 0.5f * i, 0.6f * i, 0.5f);
        auto s6 = createFlatOffsetSpline(f, j, j, g, h, 0.5f);
        auto s7 = createFlatOffsetSpline(f, j, j, g, h, 0.5f);
        auto s8 = Spline::builder(LocationFn::RIDGES, identity())
                      .add(-1.0f, f, 0.0f)
                      .add(-0.4f, s6, 0.0f)
                      .add(0.0f, h + 0.07f, 0.0f)
                      .build();
        auto s9 = createFlatOffsetSpline(-0.02f, k, k, g, h, 0.0f);

        auto builder = Spline::builder(LocationFn::EROSION, identity());
        builder
            .add(-0.85f, s1, 0.0f)
            .add(-0.70f, s2, 0.0f)
            .add(-0.40f, s3, 0.0f)
            .add(-0.35f, s4, 0.0f)
            .add(-0.10f, s5, 0.0f)
            .add(0.20f, s6, 0.0f);

        if (bl)
        {
            builder
                .add(0.40f, s7, 0.0f)
                .add(0.45f, s8, 0.0f)
                .add(0.55f, s8, 0.0f)
                .add(0.58f, s7, 0.0f);
        }
        builder.add(0.70f, s9, 0.0f);
        return builder.build();
    }

    VanillaTerrainParameters VanillaTerrainParameters::createSurfaceParameters()
    {
        VanillaTerrainParameters vtp;

        auto sp1 = createLandSpline(-0.15f, 0.0f, 0.0f, 0.1f, 0.0f, -0.03f, false, false);
        auto sp2 = createLandSpline(-0.1f, 0.03f, 0.1f, 0.1f, 0.01f, -0.03f, false, false);
        auto sp3 = createLandSpline(-0.1f, 0.03f, 0.1f, 0.7f, 0.01f, -0.03f, true, true);
        auto sp4 = createLandSpline(-0.05f, 0.03f, 0.1f, 1.0f, 0.01f, 0.01f, true, true);

        vtp.offsetSpline = Spline::builder(LocationFn::CONTINENTS, identity())
                               .add(-1.10f, 0.044f, 0.0f)
                               .add(-1.02f, -0.2222f, 0.0f)
                               .add(-0.51f, -0.2222f, 0.0f)
                               .add(-0.44f, -0.12f, 0.0f)
                               .add(-0.18f, -0.12f, 0.0f)
                               .add(-0.16f, sp1, 0.0f)
                               .add(-0.15f, sp1, 0.0f)
                               .add(-0.10f, sp2, 0.0f)
                               .add(0.25f, sp3, 0.0f)
                               .add(1.00f, sp4, 0.0f)
                               .build();

        vtp.factorSpline = Spline::builder(LocationFn::CONTINENTS, identity())
                               .add(-0.19f, 3.95f, 0.0f)
                               .add(-0.15f, buildErosionFactorSpline(6.25f, true), 0.0f)
                               .add(-0.10f, buildErosionFactorSpline(5.47f, true), 0.0f)
                               .add(0.03f, buildErosionFactorSpline(5.08f, true), 0.0f)
                               .add(0.06f, buildErosionFactorSpline(4.69f, false), 0.0f)
                               .build();

        vtp.peakSpline = Spline::builder(LocationFn::CONTINENTS, identity())
                             .add(-0.11f, 0.0f, 0.0f)
                             .add(0.03f, createErosionRidgeWeirdnessSpline(1.0f, 0.5f, 0.0f, 0.0f), 0.0f)
                             .add(0.65f, createErosionRidgeWeirdnessSpline(1.0f, 1.0f, 1.0f, 0.0f), 0.0f)
                             .build();

        return vtp;
    }

} // namespace mc