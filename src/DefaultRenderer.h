#pragma once

#include "RenderStrategy.h"
#include "acmath.h"

namespace actracer
{

class Tonemapper;
class Image;
class Camera;
class Scene;

class DefaultRenderer : public RenderStrategy
{
public:
    virtual void RenderSceneIntoPPM(Scene* scene) override;

protected:
    virtual void RetrieveRenderingParamsFromScene(Scene *scene) override;

private:
    Image& RenderCameraViewOntoImage(Camera* camera, Image& image, int startRowIndex, int endRowIndex);

    void SaveResultingImage(const char* imageName, Image& image);

private:
    int maximumRecursionDepth;
    float intersectionTestEpsilon;
    float shadowRayEpsilon;

    Vector3f backgroundColor;
    Vector3f ambientColor;

    Tonemapper* tonemapper;
};


} 
