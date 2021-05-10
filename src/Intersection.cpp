#include "Intersection.h"

#include "Shape.h"
#include "Material.h"

namespace actracer
{

bool SurfaceIntersection::CanReflectLight() const
{
    if(mat)
    {
        Vector3f MRC = mat->GetMirrorReflectionCoefficient();
        return MRC.x != 0 || MRC.y != 0 || MRC.z != 0;
    }

    return false;
}

bool SurfaceIntersection::IsInternalReflection(const Ray& ray) const
{
    return containerShape && ray.currShape && containerShape == ray.currShape;
}

void SurfaceIntersection::TweakSurfaceNormal()
{
    if (shape)
        n = shape->GetChangedNormal(*this);
}

Vector3f SurfaceIntersection::GetDiffuseReflectionCoefficient() const
{
    if(mat)
    {
        if(mColorChangerTexture)
            return mColorChangerTexture->GetChangedColorAddition(mat->GetDiffuseReflectionCoefficient(), *this);

        return mat->GetDiffuseReflectionCoefficient();
    }

    return {};
}

Vector3f SurfaceIntersection::GetSpecularReflectionCoefficient() const
{
    if(mat)
        return mat->GetSpecularReflectionCoefficient();

    return {};
}

Vector3f SurfaceIntersection::GetSurfaceNormal() const
{
    return n;
}

float SurfaceIntersection::GetRefractionIndex() const
{
    if(mat)
        return mat->GetRefractionIndex();

    return 0;
}

}