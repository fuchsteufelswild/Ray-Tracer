#pragma once

#include <vector>

namespace actracer
{

struct TonemapSettings;
class Image;

enum class TonemapType { DEFAULT };

class TonemapStrategy
{
public:
    TonemapStrategy(const TonemapSettings &tmo, const Image &image);

    TonemapStrategy() = delete;
public:
    virtual float* ComputeTonemappedValues() = 0;
protected:
    const TonemapSettings &mTMOSettings;
    const Image &mInputImage;
};

} 
