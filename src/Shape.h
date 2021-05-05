#ifndef _SHAPE_H_
#define _SHAPE_H_

#include "acmath.h"
#include "Intersection.h"

#include <vector>

#define PI 3.14159265

namespace actracer{

class ColorChangerTexture;
class NormalChangerTexture;

class Material;
class Texture;

class Shape {
public:
    enum ShapeType { SPHERE, TRIANGLE, MESH };
    enum ShadingMode { DEFAULT, SMOOTH };
public:
    int id;
    Material *mat; // Material of the object
    BoundingVolume3f bbox;
    BoundingVolume3f orgBbox;
    Transform* objTransform;

    ShapeType shType;
    ShadingMode shadingMode;

    Vector3f motionBlur;
    bool haveTransform = false;
    bool activeMotion = false;
    bool nonUniformScaling = false;

    Shape* ownerMesh;

public:
    const ColorChangerTexture* mColorChangerTexture = nullptr;
    const NormalChangerTexture* mNormalChangerTexture = nullptr;
protected:
    Shape(int _id, Material* _mat, Transform* objToWorld = nullptr, ShadingMode shMode = ShadingMode::DEFAULT);
    Shape() { }
public:
    virtual void SetTextures(const ColorChangerTexture *colorChangerTexture, const NormalChangerTexture *normalChangerTexture);

    virtual Vector3f GetChangedNormal(const SurfaceIntersection &intersection) const {}

    virtual void Intersect(Ray &r, SurfaceIntersection& rt) = 0;
    virtual Shape *Clone(bool resetTransform) const = 0;
    virtual void SetMaterial(Material* newMaterial);
    virtual void SetTransformation(Transform* newTransform, bool owned = false);

    virtual void ApplyTransformationToRay(Ray& r) const;
    
    virtual void SetMotionBlur(const Vector3f& motBlur);

    virtual void PerformVertexModification();
    virtual void RegulateVertices();

public:
    static void FindClosestObject(Ray& r, const std::vector<Shape*>& objects, SurfaceIntersection& rt);
};

}


#endif