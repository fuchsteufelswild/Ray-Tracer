#pragma once

#include <vector>

namespace actracer
{

class Primitive;
class Ray;
class SurfaceIntersection;

class AccelerationStructure
{
public:
    enum class AccelerationStructureAlgorithmCode { BVH };
public:
    /*
     * Checks if given ray intersects with any of the primitives exist in the scene.
     * If so, puts the closest "Valid" SurfaceIntersection information into passed parameter
     */ 
    virtual void Intersect(Ray &cameraRay, SurfaceIntersection& intersectedSurfaceInformation, float intersectionTestEpsilon) const = 0;
protected:
    AccelerationStructure() { }
protected:
    std::vector<Primitive *> primitives;
};

}