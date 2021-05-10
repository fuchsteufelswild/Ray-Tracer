#include "MultiSampledRayGenerator.h"

#include "Camera.h"

#include "acmath.h"
#include "Material.h"

namespace actracer
{

    MultiSampledRayGenerator::MultiSampledRayGenerator(const Camera &camera, const Pixel& pixelToMultiSample)
        : mRayShooter(camera), mPixel(pixelToMultiSample)
    {
        mRayTimeRandom = Random<double>{};
    }

    Ray MultiSampledRayGenerator::GetNextSampleRay()
    {
        if(FinishedSamples())
            return GetIthSampleRay(mCurrentSampleIndex - 1);
        
        return GetIthSampleRay(mCurrentSampleIndex++);
    }

    Ray MultiSampledRayGenerator::GetIthSampleRay(int sampleIndex) const
    {
        Ray r = mRayShooter.GenerateRayForPixelSample(mPixel, sampleIndex);

        r.currMat = Material::DefaultMaterial;
        r.time = mRayTimeRandom(0.0, 1.0);

        return r;
    }

}