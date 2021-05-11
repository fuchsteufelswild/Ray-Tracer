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
class Primitive;

class Shape {
public:
    enum ShadingMode { DEFAULT, SMOOTH };
public:
    virtual void SetTextures(const ColorChangerTexture *colorChangerTexture, const NormalChangerTexture *normalChangerTexture);
    virtual void SetMaterial(Material *newMaterial);
    virtual void SetTransformation(Transform *newTransform, bool owned = false);
    virtual void SetMotionBlur(const Vector3f &motBlur, std::vector<Primitive *> &primitives);
public:
    virtual Vector3f GetChangedNormal(const SurfaceIntersection &intersection) const {}
    virtual void Intersect(Ray &r, SurfaceIntersection &rt, float intersectionTestEpsilon) = 0;
    virtual Shape *Clone(bool resetTransform) const = 0;

    virtual void TransformRayIntoObjectSpace(Ray& r) const;
protected:
    Shape(int _id, Material* _mat, Transform* objToWorld = nullptr, ShadingMode shMode = ShadingMode::DEFAULT);
    Shape() { }

    void AdaptWorldBoundingBoxForMotionBlur(const Vector3f& motBlur);
    bool IsMotionBlurActive() const;
    bool IsOwnedByComposite() const;

    Transform GetMotionBlurExtendedTransform(float time) const;
public:
    int GetID() const;
    Material* GetMaterial() const;
    BoundingVolume3f GetBoundingBox() const;

    const Transform *GetObjectTransform() const;
public:
    void SetObjectTransform(Transform* newTransform);
    void SetHasActiveMotion(bool hasActiveMotion);
    void SetOwnerMesh(Shape* ownerMesh);
    void SetColorChangerTexture(const ColorChangerTexture *colorChangerTexture);
    void SetNormalChangerTexture(const NormalChangerTexture *normalChangerTexture);

protected:
    int id;
    Material *mat;
    BoundingVolume3f bbox; // Bounding box in world space
    BoundingVolume3f orgBbox; // Bounding box in object space
    Transform *objTransform;
    ShadingMode shadingMode;
    Vector3f motionBlur;
    bool activeMotion = false;
    Shape *ownerMesh;

    const ColorChangerTexture *mColorChangerTexture = nullptr;
    const NormalChangerTexture *mNormalChangerTexture = nullptr;
};

inline int Shape::GetID() const
{
    return id;
}

inline Material *Shape::GetMaterial() const
{
    return mat;
}

inline const Transform *Shape::GetObjectTransform() const
{
    return objTransform;
}

inline void Shape::SetObjectTransform(Transform *newTransform)
{
    objTransform = newTransform;
}

inline void Shape::SetHasActiveMotion(bool hasActiveMotion)
{
    activeMotion = hasActiveMotion;
}

inline void Shape::SetOwnerMesh(Shape *ownerMesh)
{
    this->ownerMesh = ownerMesh;
}

inline void Shape::SetColorChangerTexture(const ColorChangerTexture *colorChangerTexture)
{
    mColorChangerTexture = colorChangerTexture;
}

inline void Shape::SetNormalChangerTexture(const NormalChangerTexture *normalChangerTexture)
{
    mNormalChangerTexture = normalChangerTexture;
}

inline bool Shape::IsMotionBlurActive() const
{
    return motionBlur.x != 0 || motionBlur.y != 0 || motionBlur.z != 0;
}

inline bool Shape::IsOwnedByComposite() const
{
    return ownerMesh != this;
}

}


#endif