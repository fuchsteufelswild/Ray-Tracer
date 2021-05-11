#ifndef _PRIMITIVE_H_
#define _PRIMITIVE_H_

#include "acmath.h"
#include "Shape.h"

namespace actracer {

class Material;
class SurfaceIntersection;

class Primitive {
private:
    Shape* containedShape;
    Material* containedMaterial;
    
public:
    static int id;
    int mID;

    Primitive(Shape* shape, Material* mat)
        : containedShape(shape), containedMaterial(mat) 
    { 
        this->bbox = shape->GetBoundingBox(); 
        mID = ++id;
    }

    SurfaceIntersection Intersect(Ray& r, SurfaceIntersection& rt, float intersectionTestEpsilon);
public:
    BoundingVolume3f bbox; 
};


}

#endif