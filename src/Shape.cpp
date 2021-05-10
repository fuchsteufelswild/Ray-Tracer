#include "Scene.h"
#include "Shape.h"
#include "Texture.h"

namespace actracer {

Shape::Shape(int _id, Material *_mat, Transform *objToWorld, ShadingMode shMode)
    : id{_id}, mat{_mat}, objTransform(objToWorld), shadingMode(shMode)  
{ 
    motionBlur = Vector3f{}; 
}

BoundingVolume3f Shape::GetBoundingBox() const
{
    return bbox;
}

void Shape::SetTextures(const ColorChangerTexture* colorChangerTexture, const NormalChangerTexture* normalChangerTexture)
{
    mColorChangerTexture = colorChangerTexture;
    mNormalChangerTexture = normalChangerTexture;
}

void Shape::SetMaterial(Material* newMat) 
{ 
    this->mat = newMat; 
}

void Shape::SetTransformation(Transform *newTransform, bool owned) 
{
    // If has its own identity ( not owned by composite ) then multiply transform with new one
    if(!owned)
    {
        *objTransform = (*(objTransform))(*newTransform);
        objTransform->UpdateTransform();
    }
    
    bbox = (*(objTransform))(orgBbox);
}

void Shape::SetMotionBlur(const Vector3f& motBlur) 
{ 
    activeMotion = false;
    
    if(motBlur.x == 0 && motBlur.y == 0 && motBlur.z == 0)
        return;

    activeMotion = true;

    AdaptWorldBoundingBoxForMotionBlur(motBlur);

    this->motionBlur = motBlur; 
}

void Shape::AdaptWorldBoundingBoxForMotionBlur(const Vector3f& motBlur)
{
    Transformation motionBlurTranslation = Translation(-1, (glm::vec3)(motBlur)); // Object space

    Transform motionBlurInWorldSpace = (*objTransform)(motionBlurTranslation);
    motionBlurInWorldSpace.UpdateTransform();

    BoundingVolume3f movedBoundingBoxWithMotionBlur = motionBlurInWorldSpace(orgBbox);

    bbox = Merge(bbox, movedBoundingBoxWithMotionBlur); // Extend bounding box with motion blur
}
 
void Shape::TransformaRayIntoObjectSpace(Ray &r) const
{
    if(!objTransform) return;

    if(IsMotionBlurActive())
    {
        Vector3f timeExtendedMotionBlur = motionBlur * r.time; // [0, 0, 0] - [motionBlur.x, motionBlur.y, motionBlur.z]
        Transformation motionBlurTranslation = Translation(-1, (glm::vec3)timeExtendedMotionBlur);

        Transform objectTransformExtendedWithMotionBlurMove = (*objTransform)(motionBlurTranslation); // Update transform omitted

        r = (objectTransformExtendedWithMotionBlurMove)(r, false);
    }   
    else 
    {
        r = (*objTransform)(r, false);
    }
}


}