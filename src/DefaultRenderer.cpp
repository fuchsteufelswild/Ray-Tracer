#include "DefaultRenderer.h"
#include "Scene.h"
#include "Camera.h"
#include "Image.h"
#include "Tonemapper.h"
#include "Timer.h"
#include "Material.h"
#include "MultiSampledRayGenerator.h"
#include "AccelerationStructureFactory.h"
#include "AccelerationStructure.h"
#include "LightContributionCalculator.h"

#include <thread>

namespace actracer
{

static Color ObtainColorFromUnclampedVector(const Vector3f &unclampedColor)
{
    // Clamp the raw values between 0 - 255
    unsigned char colorRed = unclampedColor.x > 255.0f ? 255 : unclampedColor.x;
    unsigned char colorGreen = unclampedColor.y > 255.0f ? 255 : unclampedColor.y;
    unsigned char colorBlue = unclampedColor.z > 255.0f ? 255 : unclampedColor.z;
    // --

    return {colorRed, colorGreen, colorBlue};
}

DefaultRenderer::~DefaultRenderer()
{
    if(accelerator)
        delete accelerator;
}

void DefaultRenderer::RenderSceneIntoPPM(Scene* scene)
{
    mCurrentRenderedScene = scene;

    RetrieveRenderingParamsFromScene(scene);

    for (Camera* camera : scene->GetAllCameras())
    {
        RenderCamera(camera);
    }
}

void DefaultRenderer::RetrieveRenderingParamsFromScene(Scene *scene)
{
    accelerator = AccelerationStructureFactory::CreateAccelerationStructure(AccelerationStructure::AccelerationStructureAlgorithmCode::BVH, scene->GetAllPrimitives());
    maximumRecursionDepth = scene->GetMaximumRecursionDepth();
    intersectionTestEpsilon = scene->GetIntersectionTestEpsilon();
    shadowRayEpsilon = scene->GetShadowRayEpsilon();
    backgroundColor = scene->GetBackgroundColor();
    ambientColor = scene->GetAmbientColor();

    tonemapper = scene->GetTonemapper();
}

void DefaultRenderer::RenderCamera(const Camera *camera)
{
    Timer cameraRenderTimer{camera->imageName};

    Image sceneImage(camera->imgPlane.nx, camera->imgPlane.ny);

    int rowDiff = camera->imgPlane.ny / 8; // Get row difference between successive chunks
    std::thread th1(&DefaultRenderer::RenderCameraViewOntoImage, this, camera, std::ref(sceneImage), 0, rowDiff);
    std::thread th2(&DefaultRenderer::RenderCameraViewOntoImage, this, camera, std::ref(sceneImage), rowDiff * 1, rowDiff * 2);
    std::thread th3(&DefaultRenderer::RenderCameraViewOntoImage, this, camera, std::ref(sceneImage), rowDiff * 2, rowDiff * 3);
    std::thread th4(&DefaultRenderer::RenderCameraViewOntoImage, this, camera, std::ref(sceneImage), rowDiff * 3, rowDiff * 4);
    std::thread th5(&DefaultRenderer::RenderCameraViewOntoImage, this, camera, std::ref(sceneImage), rowDiff * 4, rowDiff * 5);
    std::thread th6(&DefaultRenderer::RenderCameraViewOntoImage, this, camera, std::ref(sceneImage), rowDiff * 5, rowDiff * 6);
    std::thread th7(&DefaultRenderer::RenderCameraViewOntoImage, this, camera, std::ref(sceneImage), rowDiff * 6, rowDiff * 7);
    std::thread th8(&DefaultRenderer::RenderCameraViewOntoImage, this, camera, std::ref(sceneImage), rowDiff * 7, camera->imgPlane.ny);

    th1.join(); th2.join(); th3.join(); th4.join(); th5.join(); th6.join(); th7.join(); th8.join();

    // RenderCameraViewOntoImage(camera, sceneImage, 0, camera->imgPlane.ny);
    sceneImage.SaveImage(camera->imageName);
}

/*
 * Renders [startRowIndex-endRowIndex], goes column by column 
 * executes rendering of pixel at [row, column] and writes the result into image
 */ 
Image &DefaultRenderer::RenderCameraViewOntoImage(const Camera *camera, Image &image, int startRowIndex, int endRowIndex)
{
    bool isCameraMultiSampled = camera->IsMultiSamplingOn();

    for (int i = startRowIndex; i < endRowIndex; ++i)
    {
        for (int j = 0; j < camera->imgPlane.nx; ++j)
        {
            Color res = Color(0, 0, 0);
            if (isCameraMultiSampled)
                res = RenderMultiSampled(camera, i, j);
            else
                res = RenderWithOneSample(camera, i, j);

            image.SetPixelColor(j, i, res);
        }
    }

    return image;
}

Color DefaultRenderer::RenderMultiSampled(const Camera *cam, int row, int column)
{
    Pixel currentPixel = cam->GenerateSampleForPixel(row, column);
    MultiSampledRayGenerator rayGenerator{*cam, currentPixel};

    Vector3f sumOfPixelSampleColors{};
    while(!rayGenerator.FinishedSamples())
    {
        Ray ray = rayGenerator.GetNextSampleRay();
        Vector3f pixelSampleColor;
        CalculateLight(ray, pixelSampleColor, 0, (float)column / cam->imgPlane.nx, (float)row / cam->imgPlane.ny);
        sumOfPixelSampleColors += pixelSampleColor;
    }

    sumOfPixelSampleColors /= cam->GetSampleCount();

    return ObtainColorFromUnclampedVector(sumOfPixelSampleColors);
}

/*
 * Shoots the ray directly to the center of the pixel located at [row, column] and returns the calculated color
 */ 
Color DefaultRenderer::RenderWithOneSample(const Camera *camera, int row, int column)
{
    Ray r = camera->GenerateRay(row, column);
    r.currMat = Material::DefaultMaterial;

    Vector3f pixelColor{};
    CalculateLight(r, pixelColor, 0, (float) column / camera->imgPlane.nx, (float)row / camera->imgPlane.ny);

    return ObtainColorFromUnclampedVector(pixelColor);
}

/*
 * columnNormalized01 -> ColumnPosition Mapped to [0-1]: column / width
 * rowNormalized01 -> RowPosition Mapped to [0-1]: row / height
 * depth -> recursion depth
 * Light contribution result is written into outColor
 */
void DefaultRenderer::CalculateLight(Ray &r, Vector3f &outColor, int depth, float columnNormalized01, float rowNormalized01)
{
    LightContributionCalculator contributionCalculator{};
    contributionCalculator.SetSceneLights(mCurrentRenderedScene->GetAllLights(), 
                                          mCurrentRenderedScene->GetAmbientColor(), 
                                          mCurrentRenderedScene->GetBackgroundColor(),
                                          mCurrentRenderedScene->GetBackgroundTexture());

    contributionCalculator.SetSceneAccelerator(*accelerator);
    contributionCalculator.SetRenderParameters(mCurrentRenderedScene->GetMaximumRecursionDepth(),
                                               mCurrentRenderedScene->GetIntersectionTestEpsilon(),
                                               mCurrentRenderedScene->GetShadowRayEpsilon(),
                                               2.2f,
                                               randomGenerator);

    contributionCalculator.CalculateLight(r, outColor, 0, columnNormalized01, rowNormalized01);
}

}