#pragma once

#include "Texture.h"

namespace actracer
{

class Triangle;
class Sphere;
class SurfaceIntersection;

class NormalChangerTexture : public Texture
{
public:
    virtual Vector3f GetChangedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Triangle *triangle = nullptr) const = 0;
    virtual Vector3f GetChangedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Sphere *sphere = nullptr) const = 0;
protected:
    NormalChangerTexture() = default;
};

class NormalReplacer : public NormalChangerTexture
{
public:
    virtual Vector3f GetChangedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Triangle *triangle = nullptr) const override;
    virtual Vector3f GetChangedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Sphere *sphere = nullptr) const override;
};

class NormalBumper : public NormalChangerTexture
{
public:
    virtual Vector3f GetChangedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Triangle *triangle = nullptr) const override;
    virtual Vector3f GetChangedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Sphere *sphere = nullptr) const override;
};

}