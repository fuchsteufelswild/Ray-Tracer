#include "Tonemapper.h"
#include "TonemapStrategy.h"
#include "Image.h"

namespace actracer
{
    TonemapStrategy::TonemapStrategy(const TonemapSettings &tmoSettings, const Image &image)
        : mTMOSettings(tmoSettings), mInputImage(image)
    { }
}