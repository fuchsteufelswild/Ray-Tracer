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
protected:
    virtual void RetrieveRenderingParamsFromScene(Scene *scene) override;

private:
    void RenderCamera(const Camera* camera);
    Image &RenderCameraViewOntoImage(const Camera *camera, Image &image, int startRowIndex, int endRowIndex);

    Color RenderWithOneSample(const Camera *camera, int row, int column);

    /*
     * Sampled Params:
     * CurrentColumn / ImageWidth
     * CurrentRow / ImageHeight
     */ 
    Color RenderPixel(Ray &cameraRay, float columnSampled01, float rowSampled01);

    Color RenderMultiSampled(const Camera *cam, int row, int col);

    void SaveResultingImage(const char* imageName, Image& image);

    /*
     * columnOverWidth -> ColumnPosition Mapped to 01: column / width
     * rowOverHeight -> RowPosition Mapped to 01: row / height
     * depth -> reflection/refraction Depth
     */ 
    void CalculateLight(Ray &cameraRay, Vector3f &outColor, int depth, float ctw, float rth);

    void PutDefaultColor(int columnOverWidth, int rowOverHeight);
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
