#include "Scene.h"
#include "Shape.h"
#include "Texture.h"

namespace actracer {

Shape::Shape(int _id, Material *_mat, Transform *objToWorld, ShadingMode shMode)
    : id{_id}, mat{_mat}, objTransform(objToWorld), shadingMode(shMode)  { motionBlur = Vector3f{}; }

void Shape::FindClosestObject(Ray &r, const std::vector<Shape *> &objects, SurfaceIntersection& rt)
{
    float minT = std::numeric_limits<float>::max(); // Initialize min t with inf

    // For each object in the scene
    for (Shape *shape : objects)
    {
        SurfaceIntersection io{};
        shape->Intersect(r, io); // Find the surface of intersection

        // If there is no intersection
        if (!io.mat)
            continue;

        // Update the closest surface information and the minimum distance variable
        if (io.t < minT && io.t > -Scene::pScene->intTestEps)
        {
            minT = io.t; // New minimum distance
            rt = io;    // New closest surface
        }
    }
}

void Shape::PerformVertexModification()
{

}

void Shape::RegulateVertices()
{

}

void Shape::SetTextures(const ColorChangerTexture* colorChangerTexture, const NormalChangerTexture* normalChangerTexture)
{
    mColorChangerTexture = colorChangerTexture;
    mNormalChangerTexture = normalChangerTexture;
}

void Shape::SetMaterial(Material* newMat) { this->mat = newMat; }
void Shape::SetTransformation(Transform *newTransform, bool owned) 
{

    if(!owned)
    {
        if(newTransform->transformationMatrix != glm::mat4(1))
            this->haveTransform = true;

        if (newTransform->transformationMatrix[0][0] != newTransform->transformationMatrix[1][1] ||
            newTransform->transformationMatrix[0][0] != newTransform->transformationMatrix[2][2] ||
            newTransform->transformationMatrix[1][1] != newTransform->transformationMatrix[2][2] )
            this->nonUniformScaling = true;

        
        *this->objTransform = (*(this->objTransform))(*newTransform);
        this->objTransform->UpdateTransform();
    }
    
    this->bbox = (*(this->objTransform))(this->orgBbox);
}
void Shape::SetMotionBlur(const Vector3f& motBlur) 
{ 
    this->activeMotion = false;
    
    if(motBlur.x == 0 && motBlur.y == 0 && motBlur.z == 0)
        return;

    this->activeMotion = true;

    Transformation tr = Translation(-1, (glm::vec3)(motBlur));

    Transform trr = (*this->objTransform)(tr);

    trr.UpdateTransform();

    BoundingVolume3f tt = trr(this->orgBbox);

    this->bbox = Merge(this->bbox, tt);

    this->motionBlur = motBlur; 
}

void Shape::ApplyTransformationToRay(Ray &r) const
{
    

    if(this->motionBlur.x != 0 || this->motionBlur.y != 0 || this->motionBlur.z != 0)
    {
        
        
        Transform trr = (*this->objTransform);
        
        Vector3f motBlurToRay = this->motionBlur * r.time;


        Transformation tr = Translation(-1, (glm::vec3)motBlurToRay);

        trr = (*this->objTransform)(tr); // Update transform omitted

        if (Scene::debugCurrent >= Scene::debugBegin && Scene::debugCurrent <= Scene::debugEnd)
        {
           // std::cout << "Mot transform " << trr.transformationMatrix;
           ;
        }

        r = (trr)(r, false);
    }   
    else 
    {
        r = (*this->objTransform)(r, false);
    }
}


}