#include "PixelSampler.h"
#include "Camera.h"

namespace actracer
{
    PixelSampler *PixelSampler::CreatePixelSampler(PixelSampleMethod method)
    {
        switch(method)
        {
            case PixelSampleMethod::JITTERED:
                return new JitteredPixelSampler();
        }

        return nullptr;
    }

    void JitteredPixelSampler::Init(const Camera &camera)
    {
        mRandomizerX = Random<double>{};
        mRandomizerY = Random<double>{};

        mSampleCount = camera.GetSampleCount();
        mSampleCountSquared = std::sqrt(mSampleCount);
        mSampleWidthHeight = 1.0f / mSampleCountSquared;
    }

    std::pair<float, float> JitteredPixelSampler::operator()(const Pixel& px, int sampleId) const
    {
        std::pair<float, float> samplePos = px.pixelBox.FindIthPartition(sampleId, mSampleCountSquared, mSampleWidthHeight); // Find the position of the ith partition sample (e.g 6x6 pixel's 20th)

        float randX = mRandomizerX(0.0f, mSampleWidthHeight);
        float randY = mRandomizerY(0.0f, mSampleWidthHeight);

        samplePos.first += randX;
        samplePos.second += randY;

        return samplePos;
    }
}