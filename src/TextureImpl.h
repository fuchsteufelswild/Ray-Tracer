#pragma once

#include "acmath.h"
#include "Texture.h"
namespace actracer
{

class Triangle;
class Sphere;
class SurfaceIntersection;

class Texture::TextureImpl
{
public:
    virtual ~TextureImpl() {}

    virtual Vector3f RetrieveRGBFromUV(float u, float v, float w = 0) const = 0;
    virtual Vector3f GetBumpedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Triangle *triangle = nullptr) const = 0;
    virtual Vector3f GetBumpedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Sphere* sphere = nullptr) const = 0;

    virtual Vector3f GetReplacedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Triangle *triangle = nullptr) const = 0;
    virtual Vector3f GetReplacedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Sphere *sphere = nullptr) const = 0;

    virtual Vector3f GetBaseTextureColorForColorChange(const SurfaceIntersection &intersection) const = 0;

    // GetBlendedColor
    // GetReplacedColor
protected:
    TextureImpl(float bumpFactor);
protected:
    float mBumpFactor;
};

}