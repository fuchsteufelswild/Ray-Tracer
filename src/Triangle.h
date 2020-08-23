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

    float surfaceArea;
public:
    Triangle(int _id, Material *_mat, const Vector3f &p0, const Vector3f &p1, const Vector3f &p2, 
             const Vector2f &uv0, const Vector2f &uv1, const Vector2f &uv2, Transform *objToWorld = nullptr, Shape *_m = nullptr, 
             ShadingMode shMode = ShadingMode::DEFAULT);
    Triangle(int _id, Material *_mat, Vertex *p0, Vertex* p1, Vertex *p2, Transform *objToWorld = nullptr, Shape *_m = nullptr, 
             ShadingMode shMode = ShadingMode::DEFAULT);
    Triangle() { }

    void ModifyVertices();

    virtual Vector3f RegulateNormal(Vector3f textureNormal, SurfaceIntersection &intersection) override;
    virtual Vector3f GetBumpedNormal(Texture *tex, SurfaceIntersection &intersection, Ray &ray) override;

    void PerformVertexModification() override;
    void RegulateVertices() override;

    void Intersect(Ray& r, SurfaceIntersection& rt) override;
    Shape *Clone(bool resetTransform) const override;
};

}

#endif