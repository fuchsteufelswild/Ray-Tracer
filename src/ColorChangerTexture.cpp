#include "ColorChangerTexture.h"

#include "TextureImpl.h"
#include "Intersection.h"

namespace actracer
{

bool ColorChangerTexture::IsReplacingAllColor() const
{
    return false;
}

bool ColorReplacer::IsReplacingAllColor() const
{
    return true;
}

Vector3f KDReplacer::GetChangedColorAddition(const Vector3f &diffuseReflectionCoefficient, const SurfaceIntersection &intersection) const
{
    return mTextureImpl->GetBaseTextureColorForColorChange(intersection);
}

Vector3f KDBlender::GetChangedColorAddition(const Vector3f &diffuseReflectionCoefficient, const SurfaceIntersection &intersection) const
{
    Vector3f textureColor = mTextureImpl->GetBaseTextureColorForColorChange(intersection);

    Vector3f resultingDp = (diffuseReflectionCoefficient + textureColor) / 2;
    resultingDp.x = Clamp(resultingDp.x, 0.0f, 1.0f);
    resultingDp.y = Clamp(resultingDp.y, 0.0f, 1.0f);
    resultingDp.z = Clamp(resultingDp.z, 0.0f, 1.0f);

    return resultingDp;
}

Vector3f ColorReplacer::GetChangedColorAddition(const Vector3f &diffuseReflectionCoefficient, const SurfaceIntersection &intersection) const
{
    return mTextureImpl->RetrieveRGBFromUV(intersection.uv.x, intersection.uv.y);
}

}