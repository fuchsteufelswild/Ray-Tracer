#pragma once

#include "acmath.h"
#include "Random.h"

namespace actracer
{

class Camera;
struct Pixel;

enum class PixelSampleMethod
{
    JITTERED
};

class PixelSampler
{
public:
    static PixelSampler* CreatePixelSampler(PixelSampleMethod method);

    virtual void Init(const Camera& camera) { }

    virtual std::pair<float, float> operator()(const Pixel &px, int sampleId) const = 0;

protected:
    PixelSampler() = default;
};


class JitteredPixelSampler : public PixelSampler
{
public:
    virtual void Init(const Camera &camera) override;

    virtual std::pair<float, float> operator()(const Pixel &px, int sampleId) const override;
private:
    mutable Random<double> mRandomizerX;
    mutable Random<double> mRandomizerY;

    int mSampleCount;
    int mSampleCountSquared;
    float mSampleWidthHeight;
};

}