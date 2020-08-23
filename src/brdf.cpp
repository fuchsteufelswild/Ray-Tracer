#include "brdf.h"
#include <algorithm>

const float pi = 3.14159265f;

namespace actracer {

    float BRDFTorranceSparrow::d(float cosa)
    {
        return std::pow(cosa, specularPhong) * (specularPhong + 2) / (2.0f * pi); 
    }

    float BRDFTorranceSparrow::fr(float cosb, float ri)
    {
        float n1 = 1.0f;        // Index of outside mat
        float n2 = ri; // Index of inside mat

        
        cosb = Clamp<float>(cosb, -1, 1);

        if (cosb > 0) // Invert the normal for internal refraction
        {
            std::swap(n1, n2);
        }
        else
            cosb = -cosb;

        float sin1 = std::sqrt(1 - cosb * cosb);
        float sin2 = (n1 * sin1) / n2;
        float cos2 = std::sqrt(1 - sin2 * sin2);

        n1 = ri;

        float c2 = cosb * cosb;
        float nk = n1 * n1 + ai * ai;
        float n2c = 2 * n1 * cosb;

        float Rs = (nk - n2c + c2) / (nk + n2c + c2);
        float Rp = (nk * c2 - n2c + 1) / (nk * c2 + n2c + 1);

        return 0.5f * (Rs + Rp);

        float temp = ((ri - 1) * (ri - 1)) / ((ri + 1) * (ri + 1));
        return std::pow(1 - cosb, 5) * (1 - temp) + temp;
    }

    float BRDFTorranceSparrow::g(const Vector3f& wi, const Vector3f& wo, const Vector3f& n)
    {
        Vector3f wh = Normalize(wi + wo);
        return std::min(1.0f, Dot(n, wh) * 2.0f / Dot(wo, wh) * std::min(Dot(n, wo), Dot(n, wi)));
    }

    Vector3f BRDFTorranceSparrow::f(const Vector3f &wi, const Vector3f &wo, const Vector3f &n, const Vector3f &kd, const Vector3f &ks, float ri)
    {
        float cost = Dot(n, wi);

        if(cost < 0)
            return Vector3f{};

        if(std::acos(cost) < pi / 2.0)
        {
            Vector3f wh = Normalize(wi + wo);
            float cosa = std::max(0.0f, Dot(n, wh));
            float cosb = std::max(0.0f, Dot(wo, wh));
            float cosc = Dot(wo, n);
            if(kdfresnel)
                return (1.0f - fr(cosb, ri)) * kd * (1.0f / pi) * cost + ks * g(wi, wo, n) * fr(cosb, ri) * d(cosa) / (4.0f * cosc);
            else
                return kd * (1.0f / pi) * cost + ks * g(wi, wo, n) * d(cosa) / (4.0f * cosc);
        }

        return Vector3f{};
    }

    void BRDFBlinnPhongModified::NormalizedCalculation(const Vector3f &wi, const Vector3f &wo, const Vector3f &n, const Vector3f &kd, const Vector3f &ks, Vector3f& result)
    {
        float cost = Dot(wi, n);

        if(cost < 0)
            return;
        
        if(std::acos(cost) < pi / 2.0)
            result = (kd * (1.0f / pi) + (ks * (specularPhong + 8) * std::pow(std::max(0.0f, Dot(n, Normalize(wi + wo))), specularPhong) / (8 * pi))) * cost;
    }

    void BRDFBlinnPhongModified::ClassicCalculation(const Vector3f &wi, const Vector3f &wo, const Vector3f &n, const Vector3f &kd, const Vector3f &ks, Vector3f &result)
    {
        float cost = Dot(wi, n);

        if (cost < 0)
            return;

        if (std::acos(cost) < pi / 2.0)
            result = (kd + ks * std::pow(std::max(0.0f, Dot(n, Normalize(wi + wo))), specularPhong)) * cost;
    }

    Vector3f BRDFBlinnPhongModified::f(const Vector3f &wi, const Vector3f &wo, const Vector3f &n, const Vector3f &kd, const Vector3f &ks, float)
    {
        Vector3f result{};

        if(normalized)
            NormalizedCalculation(wi, wo, n, kd, ks, result);
        else
            ClassicCalculation(wi, wo, n, kd, ks, result);

        return result;
        
    }

    void BRDFPhongModified::NormalizedCalculation(const Vector3f &wi, const Vector3f &wo, const Vector3f &n, const Vector3f &kd, const Vector3f &ks, Vector3f &result)
    {
        float cost = Dot(wi, n);

        if (cost < 0.0f)
            return;

        if (std::acos(cost) < pi / 2.0)
            result = (kd * (1.0f / pi) + ks * (specularPhong + 2) * std::pow(Dot(wo, wi - 2.0f * n * cost), specularPhong) / (2.0f * pi)) * cost;
    }

    void BRDFPhongModified::ClassicCalculation(const Vector3f &wi, const Vector3f &wo, const Vector3f &n, const Vector3f &kd, const Vector3f &ks, Vector3f &result)
    {
        float cost = Dot(wi, n);

        if(cost < 0.0f)
            return;

        if(std::acos(cost) < pi / 2.0)
            result = (kd + ks * std::pow(Dot(wo, wi - 2.0f * n * cost), specularPhong)) * cost;
        
    }

    Vector3f BRDFPhongModified::f(const Vector3f &wi, const Vector3f &wo, const Vector3f &n, const Vector3f &kd, const Vector3f &ks, float)
    {
        Vector3f result{};

        if (normalized)
            NormalizedCalculation(wi, wo, n, kd, ks, result);
        else
            ClassicCalculation(wi, wo, n, kd, ks, result);

        return result;
    }

    Vector3f BRDFBlinnPhongOriginal::f(const Vector3f &wi, const Vector3f &wo, const Vector3f &n, const Vector3f &kd, const Vector3f &ks, float)
    {
        float cost = Dot(wi, n);

        if (cost < 0)
            return Vector3f{};

        if (std::acos(cost) < pi / 2.0)
            return kd * cost + ks * std::pow(std::max(0.0f, Dot(n, Normalize(wi + wo))), specularPhong);
    }

    Vector3f BRDFPhongOriginal::f(const Vector3f &wi, const Vector3f &wo, const Vector3f &n, const Vector3f &kd, const Vector3f &ks, float)
    {
        float cost = Dot(wi, n);

        if (cost < 0.0f)
            return Vector3f{};

        if (std::acos(cost) < pi / 2.0)
            return kd * cost + ks * std::pow(Dot(wo, Normalize(wi - 2.0f * n * cost)), specularPhong);
    }
}