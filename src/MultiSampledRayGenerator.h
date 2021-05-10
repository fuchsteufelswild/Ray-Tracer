#pragma once

#include "Random.h"
#include "Camera.h"

namespace actracer
{

class Ray;

class MultiSampledRayGenerator
{
public:  
    MultiSampledRayGenerator(const Camera& camera, const Pixel& pixelToMultiSample);
    Ray GetNextSampleRay();
    bool FinishedSamples() const;

    Ray GetIthSampleRay(int sampleIndex) const;
private:
    int mCurrentSampleIndex = 0;

    mutable Random<double> mRayTimeRandom;

    const Camera& mRayShooter;
    const Pixel& mPixel; 
};

inline bool MultiSampledRayGenerator::FinishedSamples() const
{
    return mRayShooter.GetSampleCount() == mCurrentSampleIndex;
}

}