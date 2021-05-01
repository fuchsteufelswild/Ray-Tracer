#ifndef _TMO_H_
#define _TMO_H_

#include <vector>
#include <algorithm>
#include <string>

namespace actracer {

enum class TonemapType;
class Image;

struct TMOData {
    float* data;
    int width;
    int height;
};

struct TonemapSettings
{
    float imageBurnKey;
    float saturationPercentage;
    float saturation;
    float gamma;
};

class Tonemapper {

public:
    Tonemapper(float key, float satperc, float sat, float gam);

public:
    static bool SaveEXR(const float *rgb, int width, int height, const char *outfilename);

    static TMOData ReadExr(std::string file);

public:
    void SetTonemapStrategy(TonemapType tonemapType);
    void BuildTonemapStrategy() const;

    float* Tonemap(const Image &image);
public:
    float GetImageBurnKey() const;
    float GetSaturationPercentage() const;
    float GetSaturation() const;
    float GetGamma() const;
private:
    float ComputeLuminanceWhite(const std::vector<float> &luminances) const;
private:
    class TonemapStrategy* mTonemapStrategy;
    TonemapSettings mTonemapSettings;
    TonemapType mTonemapType;

    float mImageBurnKey;
    float mSaturationPercentage;
    float mSaturation;
    float mGamma;
};

inline float Tonemapper::GetImageBurnKey() const
{
    return mTonemapSettings.imageBurnKey;
}

inline float Tonemapper::GetSaturationPercentage() const
{
    return mTonemapSettings.saturationPercentage;
}

inline float Tonemapper::GetSaturation() const
{
    return mTonemapSettings.saturation;
}

inline float Tonemapper::GetGamma() const
{
    return mTonemapSettings.gamma;
}

}
#endif