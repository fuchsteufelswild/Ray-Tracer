#ifndef _SPHERE_H_
#define _SPHERE_H_

#include "Shape.h"

namespace actracer {

class Sphere : public Shape {
private:
    float    radius;    // Radius of the sphere
    Vector3f center; // Center point of the sphere
public:
    Sphere(int _id, Material *_mat, float _radius, const Vector3f &centerPoint, Transform *objToWorld = nullptr, ShadingMode shMode = ShadingMode::DEFAULT);
    Sphere() { }

    virtual Vector3f RegulateNormal(Vector3f textureNormal, SurfaceIntersection &intersection) override;
    virtual Vector3f GetBumpedNormal(Texture *tex, SurfaceIntersection &intersection, Ray &ray) override;

    void Intersect(Ray &r, SurfaceIntersection &rt) override;
    Shape* Clone(bool resetTransform) const override;
};

}

#endif