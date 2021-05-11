#ifndef _MESH_H_
#define _MESH_H_

#include <vector>

#include "Shape.h"
#include "Triangle.h"

namespace actracer {

class Mesh : public Shape {
private:
    std::vector<Triangle*>* triangles;
    std::vector<Vertex*>* meshVertices;
public:
    Mesh(int _id, Material *_mat, const std::vector<std::pair<int, Material *>> &faces, const std::vector<Vector3f *> *pIndices, const std::vector<Vector2f *> *pUVs, std::vector<Primitive *> &primitives, Transform *objToWorld = nullptr, ShadingMode shMode = ShadingMode::DEFAULT);
    Mesh() { }

    void FindClosestObject(Ray &r, SurfaceIntersection &rt, float intersectionTestEpsilon);
    void Intersect(Ray &r, SurfaceIntersection &rt, float intersectionTestEpsilon) override;
    Shape *Clone(bool resetTransform) const override;
public:
    virtual void SetMaterial(Material* newMat) override;
    virtual void SetTransformation(Transform* newTransform, bool owned = false) override;
    virtual void SetTextures(const ColorChangerTexture *colorChangerTexture, const NormalChangerTexture *normalChangerTexture) override;
    virtual void SetMotionBlur(const Vector3f &v, std::vector<Primitive *> &primitives) override;
};


}

#endif