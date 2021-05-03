#include "Primitive.h"

namespace actracer {

SurfaceIntersection Primitive::Intersect(Ray &r, SurfaceIntersection &rt) 
{ 
    containedShape->Intersect(r, rt); 
}

}