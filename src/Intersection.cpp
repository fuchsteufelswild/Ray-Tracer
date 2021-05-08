#include "Intersection.h"

#include "Shape.h"
#include "Material.h"

namespace actracer
{

bool SurfaceIntersection::CanReflectLight() const
{
    if(mat)
        return mat->MRC.x != 0 || mat->MRC.y != 0 || mat->MRC.z != 0;

    return false;
}

bool SurfaceIntersection::IsInternalReflection(const Ray& ray) const
{
    return shape && ray.currShape && shape == ray.currShape;
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
            return mColorChangerTexture->GetChangedColorAddition(mat->DRC, *this);

        return mat->DRC;
    }

    return {};
}

Vector3f SurfaceIntersection::GetSpecularReflectionCoefficient() const
{
    if(mat)
        return mat->SRC;

    return {};
}

Vector3f SurfaceIntersection::GetSurfaceNormal() const
{
    return n;
}

float SurfaceIntersection::GetReflectionIndex() const
{
    if(mat)
        return mat->rIndex;

    return 0;
}

}