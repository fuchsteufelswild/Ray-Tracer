#pragma once

#include "Texture.h"

namespace actracer
{

class SurfaceIntersection;

class ColorChangerTexture : public Texture
{
public:
    virtual bool IsReplacingAllColor() const;

    virtual Vector3f GetChangedColorAddition(const Vector3f &diffuseReflectionCoefficient, const SurfaceIntersection& intersection) const = 0;

protected:
    ColorChangerTexture() = default;
};

class KDReplacer : public ColorChangerTexture
{
public:
    virtual Vector3f GetChangedColorAddition(const Vector3f &diffuseReflectionCoefficient, const SurfaceIntersection &intersection) const override;
};

class KDBlender : public ColorChangerTexture
{
public:
    virtual Vector3f GetChangedColorAddition(const Vector3f &diffuseReflectionCoefficient, const SurfaceIntersection& intersection) const override;
};

class ColorReplacer : public ColorChangerTexture
{
public:
    virtual Vector3f GetChangedColorAddition(const Vector3f &diffuseReflectionCoefficient, const SurfaceIntersection &intersection) const override;

    virtual bool IsReplacingAllColor() const override;
};

}