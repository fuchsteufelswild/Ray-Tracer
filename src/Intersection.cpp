#include "Intersection.h"

#include "Shape.h"

namespace actracer
{

void SurfaceIntersection::TweakSurfaceNormal()
{
    if (shape)
    {
        n = shape->GetChangedNormal(*this);
    }

}

}