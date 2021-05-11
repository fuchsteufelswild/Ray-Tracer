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
public:
    virtual Vector3f GetChangedNormal(const SurfaceIntersection &intersection) const override;
public:
    float GetRadius() const;
public:
    void Intersect(Ray &r, SurfaceIntersection &rt, float intersectionTestEpsilon) override;
    Shape* Clone(bool resetTransform) const override;
private:
    void CalculateTValueForIntersection(const Ray &r, bool &hasIntersected, float &t) const;
    void CalculateThetaPhiValuesForPoint(const Vector3f &point, Vector2f &uv, float &theta, float &phi) const;
    void TransformSurfaceVariablesIntoWorldSpace(float rayTime, Vector3f &intersectionPoint, Vector3f &surfaceNormal) const;
};

inline float Sphere::GetRadius() const
{
    return radius;
}

}

#endif