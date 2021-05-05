#include "Intersection.h"

#include "Shape.h"
#include "Material.h"

namespace actracer
{

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