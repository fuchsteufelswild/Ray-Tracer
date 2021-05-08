#pragma once

#include "RenderStrategy.h"
#include "acmath.h"

namespace actracer
{

class AccelerationStructure;
class Tonemapper;
class Image;
class Camera;
class Scene;
union Color;

class DefaultRenderer : public RenderStrategy
{
public:
    virtual void RenderSceneIntoPPM(Scene* scene) override;
    DefaultRenderer() : RenderStrategy() {}
    virtual ~DefaultRenderer();
protected:
    /*
     * Retrieves maximum recursion depth for recursive ray tracing,
     * epsilon values for tests,
     * and lights for light computation
     */ 
    virtual void RetrieveRenderingParamsFromScene(Scene *scene) override;

private:
    void RenderCamera(const Camera* camera);
    /*
     * Computes the values of pixels of rows [startRowIndex-endRowIndex] and writes into image
     */ 
    Image &RenderCameraViewOntoImage(const Camera *camera, Image &image, int startRowIndex, int endRowIndex);

    Color RenderWithOneSample(const Camera *camera, int row, int column);
    Color RenderMultiSampled(const Camera *cam, int row, int col);

    void CalculateLight(Ray &cameraRay, Vector3f &outColor, int depth, float columnNormalized01, float rowNormalized01);

private:
    const Scene* mCurrentRenderedScene;

    int maximumRecursionDepth;
    float intersectionTestEpsilon;
    float shadowRayEpsilon;

    Vector3f backgroundColor;
    Vector3f ambientColor;

    const Tonemapper* tonemapper;

    AccelerationStructure* accelerator;
};


} 
