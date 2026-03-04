#pragma once
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>

namespace mc
{

    class MathHelper
    {
    public:
        static int floor(double d)
        {
            int i = (int)d;
            return d < (double)i ? i - 1 : i;
        }

        static int floorDiv(int x, int y)
        {
            int q = x / y;
            if ((x ^ y) < 0 && (q * y != x))
                return q - 1;
            return q;
        }

        static int64_t lfloor(double d)
        {
            int64_t l = (int64_t)d;
            return d < (double)l ? l - 1L : l;
        }

        static int clamp(int i, int j, int k)
        {
            if (i < j)
                return j;
            return i < k ? i : k;
        }

        static double clamp(double d, double e, double f)
        {
            if (d < e)
                return e;
            return d < f ? d : f;
        }

        static double lerp(double t, double a, double b)
        {
            return a + t * (b - a);
        }

        static float lerp(float t, float a, float b)
        {
            return a + t * (b - a);
        }

        static double lerp2(double d, double e,
                            double f, double g,
                            double h, double i)
        {
            return lerp(e, lerp(d, f, g), lerp(d, h, i));
        }

        static double lerp3(double d, double e, double f,
                            double g, double h, double i, double j,
                            double k, double l, double m, double n)
        {
            return lerp(f, lerp2(d, e, g, h, i, j),
                        lerp2(d, e, k, l, m, n));
        }

        static double clampedLerp(double a, double b, double t)
        {
            if (t < 0.0)
                return a;
            if (t > 1.0)
                return b;
            return lerp(t, a, b);
        }

        static double getLerpProgress(double d, double e, double f)
        {
            return (d - e) / (f - e);
        }

        static double lerpFromProgress(double d, double e, double f, double g, double h)
        {
            return lerp(getLerpProgress(d, e, f), g, h);
        }

        static double clampedLerpFromProgress(double d, double e, double f, double g, double h)
        {
            return clampedLerp(g, h, getLerpProgress(d, e, f));
        }

        static double perlinFade(double d)
        {
            return d * d * d * (d * (d * 6.0 - 15.0) + 10.0);
        }

        static double square(double d)
        {
            return d * d;
        }

        static int roundDownToMultiple(double d, int i)
        {
            return floor(d / (double)i) * i;
        }

        static int64_t hashCode(int i, int j, int k)
        {
            int64_t l = (int64_t)(i * 3129871) ^ (int64_t)k * 116129781LL ^ (int64_t)j;
            l = l * l * 42317861LL + l * 11LL;
            return l >> 16;
        }

        static int binarySearch(int lo, int hi, std::function<bool(int)> pred)
        {
            int k = hi - lo;
            while (k > 0)
            {
                int l = k / 2;
                int m = lo + l;
                if (pred(m))
                {
                    k = l;
                }
                else
                {
                    lo = m + 1;
                    k -= l + 1;
                }
            }
            return lo;
        }

        static int smallestEncompassingPowerOfTwo(int i)
        {
            int j = i - 1;
            j |= j >> 1;
            j |= j >> 2;
            j |= j >> 4;
            j |= j >> 8;
            j |= j >> 16;
            return j + 1;
        }

        static bool isPowerOfTwo(int i)
        {
            return i != 0 && (i & (i - 1)) == 0;
        }

        static int ceilLog2(int i)
        {
            static const int DEBRUIJN[32] = {
                0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
                31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9};
            i = isPowerOfTwo(i) ? i : smallestEncompassingPowerOfTwo(i);
            return DEBRUIJN[((uint32_t)(i * 125613361U)) >> 27];
        }

        static int floorLog2(int i) { return ceilLog2(i) - (isPowerOfTwo(i) ? 0 : 1); }
    };

} // namespace mc