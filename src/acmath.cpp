#include "acmath.h"
#include "Shape.h"
#include "Scene.h"

namespace actracer 
{
    void Ray::InsertRay(Shape* origin, Ray* newRay)
    {
        this->transformedModes.insert(std::make_pair(origin, newRay));
    }

    void Ray::InsertTransform(Shape *origin, Transform* newTransform)
    {
        this->transformMatrices.insert(std::make_pair(origin, newTransform));
    }
}