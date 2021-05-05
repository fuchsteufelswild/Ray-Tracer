#ifndef _MESH_H_
#define _MESH_H_

#include <vector>

#include "Shape.h"
#include "Triangle.h"

namespace actracer {

class Mesh : public Shape {
private:
    std::vector<Shape*>* triangles;
    std::vector<Vertex*>* meshVertices;
public:
    Mesh(int _id, Material *_mat, const std::vector<std::pair<int, Material *>> &faces, const std::vector<Vector3f *> *pIndices, const std::vector<Vector2f *> *pUVs, Transform *objToWorld = nullptr, ShadingMode shMode = ShadingMode::DEFAULT);
    Mesh() { }

    void Intersect(Ray &r, SurfaceIntersection &rt) override;
    Shape *Clone(bool resetTransform) const override;


    void SetMaterial(Material* newMat) override;
    void SetTransformation(Transform* newTransform, bool owned = false) override;
    virtual void SetTextures(const ColorChangerTexture *colorChangerTexture, const NormalChangerTexture *normalChangerTexture) override;

    virtual void SetMotionBlur(const Vector3f& v);
};


}

#endif