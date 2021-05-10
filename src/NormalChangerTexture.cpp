#include "NormalChangerTexture.h"

#include "TextureImpl.h"

namespace actracer
{

Vector3f NormalReplacer::GetChangedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Triangle *triangle) const
{
    return mTextureImpl->GetReplacedNormal(intersectedSurfaceInformation, triangle);
}

Vector3f NormalReplacer::GetChangedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Sphere *sphere) const
{
    return mTextureImpl->GetReplacedNormal(intersectedSurfaceInformation, sphere);
}

Vector3f NormalBumper::GetChangedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Triangle *triangle) const
{
    return mTextureImpl->GetBumpedNormal(intersectedSurfaceInformation, triangle);
}

Vector3f NormalBumper::GetChangedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Sphere *sphere) const
{
    return mTextureImpl->GetBumpedNormal(intersectedSurfaceInformation, sphere);
}

}