#ifndef _TRIANGLE_H_
#define _TRIANGLE_H_

#include "Shape.h"

namespace actracer {

class Triangle : public Shape {
private:
    Vertex* v0; //
    Vertex* v1; // Vertices
    Vertex* v2; //

    Vector3f normal; // Normal of the triangle

    // Precalculated edge vectors
    Vector3f p0p1;  
    Vector3f p0p2; 
public:
    Triangle(int _id, Material *_mat, const Vector3f &p0, const Vector3f &p1, const Vector3f &p2, 
             const Vector2f &uv0, const Vector2f &uv1, const Vector2f &uv2, Transform *objToWorld = nullptr, Shape *_m = nullptr, 
             ShadingMode shMode = ShadingMode::DEFAULT);
    Triangle(int _id, Material *_mat, Vertex *p0, Vertex* p1, Vertex *p2, Transform *objToWorld = nullptr, Shape *_m = nullptr, 
             ShadingMode shMode = ShadingMode::DEFAULT);
    Triangle() { }
public:
    virtual Vector3f GetChangedNormal(const SurfaceIntersection &intersection) const override;
public:
    const Vector3f &GetEdgeVectorFromFirstToSecondVertex() const;
    const Vector3f &GetEdgeVectorFromFirstToThirdVertex() const;

    const Vertex* GetFirstVertex() const;
    const Vertex* GetSecondVertex() const;
    const Vertex* GetThirdVertex() const;

    const Vector3f& GetNormal() const;
public:
    void ModifyVertices();

    void PerformVertexModification();
    void RegulateVertices();

    void Intersect(Ray &r, SurfaceIntersection &rt, float intersectionTestEpsilon) override;
    Triangle *Clone(bool resetTransform) const override;
private:
    void TransformRayIntoObjectSpace(Ray &baseRay, Ray &r) const;
    bool HasRayTransformedBefore(Ray& r) const;

    void TransformAndRecordRay(Ray &baseRay, Ray &r) const;

    void CalculateTValueForIntersection(const Ray &r, bool &hasIntersected, float &t, float &beta, float &gamma, float intersectionTestEpsilon) const;
    void TransformSurfaceValues(const Ray &baseRay, const Ray &transformedRay, Vector3f &intersectionPoint, Vector3f &surfaceNormal) const;

    void CalculateSurfaceValues(const float epsilon, const float beta, const float gamma, Vector2f &uv, Vector3f &surfaceNormal) const;
};

inline const Vector3f& Triangle::GetNormal() const
{
    return normal;
}

inline const Vector3f& Triangle::GetEdgeVectorFromFirstToSecondVertex() const
{
    return p0p1;
}

inline const Vector3f &Triangle::GetEdgeVectorFromFirstToThirdVertex() const
{
    return p0p2;
}

inline const Vertex *Triangle::GetFirstVertex() const
{
    return v0;
}

inline const Vertex *Triangle::GetSecondVertex() const
{
    return v1;
}

inline const Vertex *Triangle::GetThirdVertex() const
{
    return v2;
}

}

#endif