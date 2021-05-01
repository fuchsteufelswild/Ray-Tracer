#include "DefaultTonemapStrategy.h"
#include "Tonemapper.h"
#include "Image.h"
#include "TonemapStrategyFactory.h"

namespace actracer
{

TonemapStrategy* TonemapStrategyFactory::BuildTonemapStrategy(TonemapType tonemapType, const TonemapSettings &tmo, const Image &image)
{
    switch(tonemapType)
    {
        case TonemapType::DEFAULT:
            return new DefaultTonemapStrategy(tmo, image);
        default:
            break;
    }

    return nullptr;
}

}