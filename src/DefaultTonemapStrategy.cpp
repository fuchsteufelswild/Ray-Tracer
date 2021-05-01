#include "DefaultTonemapStrategy.h"
#include "TonemapStrategy.h"
#include "Tonemapper.h"

#define COLOUR_CHANNEL_COUNT 3

namespace actracer
{
    using LuminanceSum = float;

    DefaultTonemapStrategy::DefaultTonemapStrategy(const TonemapSettings &tmoSettings, const Image &image)
        : TonemapStrategy(tmoSettings, image)
        { }

    float* DefaultTonemapStrategy::ComputeTonemappedValues()
    {
        SetupGeneralAttributes();

        int imageHeight = mInputImage.GetImageHeight();
        int imageWidth = mInputImage.GetImageWidth();

        float *tonemappedPixelColors = new float[mInputImage.GetImageSize() * COLOUR_CHANNEL_COUNT];

        for (unsigned int i = 0; i < imageHeight; ++i)
        {
            for (unsigned int j = 0; j < imageWidth; ++j)
            {
                Vector3f tonemappedColor = ComputeTonemappedPixelColor(i, j);

                RecordTonemappedColor(i, j, tonemappedPixelColors, tonemappedColor);
            }
        }

        return tonemappedPixelColors;
    }

    void DefaultTonemapStrategy::SetupGeneralAttributes()
    {
        float luminanceSum = FillLuminanceVectorAndRecordSum();
        HeapifyLuminanceVector();

        ComputeLuminanceWhite();

        luminanceSum /= mInputImage.GetImageSize();
        mAverageLuminance = exp(luminanceSum);
    }

    LuminanceSum DefaultTonemapStrategy::FillLuminanceVectorAndRecordSum()
    {
        float luminanceSum = 0.0f;

        int imageHeight = mInputImage.GetImageHeight();
        int imageWidth = mInputImage.GetImageWidth();

        for (unsigned int i = 0; i < imageHeight; ++i)
        {
            for (unsigned int j = 0; j < imageWidth; ++j)
            {
                RecordPixelLuminance(i, j, luminanceSum);
            }
        }
    }

    void DefaultTonemapStrategy::RecordPixelLuminance(int row, int column, float &luminanceSum)
    {
        Color pixelColor = mInputImage.GetPixelValue(column, row);

        float luminance = ComputeLuminance(pixelColor);
        mLuminances.push_back(luminance);
        luminanceSum += log(luminance + 0.00001f);
    }

    void DefaultTonemapStrategy::HeapifyLuminanceVector()
    {
        std::make_heap(mLuminances.begin(), mLuminances.end());
        std::sort_heap(mLuminances.begin(), mLuminances.end());
    }

    Vector3f DefaultTonemapStrategy::ComputeTonemappedPixelColor(int row, int column)
    {
        Color pixelColor = mInputImage.GetPixelValue(column, row);

        float luminance = ComputeLuminance(pixelColor);
        float resultingLuminance = ComputeResultingLuminance(luminance);

        return ComputeDisplayValues(pixelColor, luminance, resultingLuminance);
    }

    float DefaultTonemapStrategy::ComputeResultingLuminance(float plainLuminance) const
    {
        float regulatedLuminance = plainLuminance * mTMOSettings.imageBurnKey / mAverageLuminance;

        return (regulatedLuminance * (1 + (regulatedLuminance / (mLuminanceWhite * mLuminanceWhite)))) / (1 + regulatedLuminance);
    }

    Vector3f DefaultTonemapStrategy::ComputeDisplayValues(const Color &pixelColor, float luminance, float resultingLuminance) const
    {
        return Vector3f{
            std::pow(pixelColor.red / luminance, mTMOSettings.saturation) * resultingLuminance,
            std::pow(pixelColor.grn / luminance, mTMOSettings.saturation) * resultingLuminance,
            std::pow(pixelColor.blu / luminance, mTMOSettings.saturation) * resultingLuminance};
    }

    void DefaultTonemapStrategy::ComputeLuminanceWhite()
    {
        mLuminanceWhite = mLuminances[round(mLuminances.size() * (100.f - mTMOSettings.saturationPercentage) / 100.f)] / mLuminances.back();
    }

    void DefaultTonemapStrategy::RecordTonemappedColor(int row, int column, float *array, const Vector3f &tonemappedColor) const
    {
        array[(row * mInputImage.GetImageWidth() + column) * COLOUR_CHANNEL_COUNT] = tonemappedColor.r;
        array[(row * mInputImage.GetImageWidth() + column) * COLOUR_CHANNEL_COUNT + 1] = tonemappedColor.g;
        array[(row * mInputImage.GetImageWidth() + column) * COLOUR_CHANNEL_COUNT + 2] = tonemappedColor.b;
    }
}
