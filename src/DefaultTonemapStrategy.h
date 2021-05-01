#pragma once

#include "TonemapStrategy.h"
#include "acmath.h"
#include "Image.h"

namespace actracer 
{

struct TonemapSettings;
class Image;

class DefaultTonemapStrategy : public TonemapStrategy
{
public:
    DefaultTonemapStrategy(const TonemapSettings& tmoSettings, const Image& image);

public:
    virtual float *ComputeTonemappedValues() override;

private:
    void SetupGeneralAttributes();

    float FillLuminanceVectorAndRecordSum();
    void HeapifyLuminanceVector();
    void ComputeLuminanceWhite();

    void RecordPixelLuminance(int row, int column, float &luminanceSum);
    float ComputeLuminance(const Color &pixelColor);

    float ComputeResultingLuminance(float plainLuminance) const;
    Vector3f ComputeDisplayValues(const Color &pixelColor, float luminance, float resultingLuminance) const;
    Vector3f ComputeTonemappedPixelColor(int row, int column);
    void RecordTonemappedColor(int row, int column, float *array, const Vector3f &tonemappedColor) const;

private:
    std::vector<float> mLuminances;

    float mLuminanceWhite;
    float mAverageLuminance;
};

inline float DefaultTonemapStrategy::ComputeLuminance(const Color &pixelColor)
{
    return 0.27f * pixelColor.red + 0.67f * pixelColor.grn + 0.06f * pixelColor.blu;
}

}