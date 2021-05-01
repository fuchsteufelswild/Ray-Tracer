#pragma once

namespace actracer
{

enum class TonemapType;
class TonemapStrategy;
class Image;
struct TonemapSettings;

class TonemapStrategyFactory
{
public:
    static TonemapStrategy* BuildTonemapStrategy(TonemapType tonemapType, const TonemapSettings &tmo, const Image &image);
}; 

}