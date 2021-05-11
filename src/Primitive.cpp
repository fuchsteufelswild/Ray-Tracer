#include "Primitive.h"

namespace actracer {

SurfaceIntersection Primitive::Intersect(Ray &r, SurfaceIntersection &rt, float intersectionTestEpsilon) 
{ 
    containedShape->Intersect(r, rt, intersectionTestEpsilon); 
}

}