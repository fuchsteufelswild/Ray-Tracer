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

namespace actracer
{

static Color ObtainColor(const Vector3f &unclampedColor)
{
    // Clamp the raw values between 0 - 255
    unsigned char colorRed = unclampedColor.x > 255.0f ? 255 : unclampedColor.x;
    unsigned char colorGreen = unclampedColor.y > 255.0f ? 255 : unclampedColor.y;
    unsigned char colorBlue = unclampedColor.z > 255.0f ? 255 : unclampedColor.z;
    // --

    return {colorRed, colorGreen, colorBlue};
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

    // std::thread th1(&Scene::RenderCam, this, cam, std::ref(img), 0, rowDiff);
    // std::thread th2(&Scene::RenderCam, this, cam, std::ref(img), rowDiff * 1, rowDiff * 2);
    // std::thread th3(&Scene::RenderCam, this, cam, std::ref(img), rowDiff * 2, rowDiff * 3);
    // std::thread th4(&Scene::RenderCam, this, cam, std::ref(img), rowDiff * 3, rowDiff * 4);
    // std::thread th5(&Scene::RenderCam, this, cam, std::ref(img), rowDiff * 4, rowDiff * 5);
    // std::thread th6(&Scene::RenderCam, this, cam, std::ref(img), rowDiff * 5, rowDiff * 6);
    // std::thread th7(&Scene::RenderCam, this, cam, std::ref(img), rowDiff * 6, rowDiff * 7);
    // std::thread th8(&Scene::RenderCam, this, cam, std::ref(img), rowDiff * 7, cam->imgPlane.ny);

    // th1.join(); th2.join(); th3.join(); th4.join(); th5.join(); th6.join(); th7.join(); th8.join();

    RenderCameraViewOntoImage(camera, sceneImage, 0, camera->imgPlane.ny);

    SaveResultingImage(camera->imageName, sceneImage);
}

void DefaultRenderer::SaveResultingImage(const char *imageName, Image &image)
{
    if (tonemapper)
    {
        float *tonemappedColorOutputValues = tonemapper->Tonemap(image);
        tonemapper->SaveEXR(tonemappedColorOutputValues, image.GetImageWidth(), image.GetImageHeight(), imageName);
    }
    else
        image.SaveImageAsPPM(imageName);
}

Image &DefaultRenderer::RenderCameraViewOntoImage(const Camera *camera, Image &image, int startRowIndex, int endRowIndex)
{
    for (int i = startRowIndex; i < endRowIndex; ++i)
    {
        for (int j = 0; j < camera->imgPlane.nx; ++j)
        {
            Scene::debugCurrent = j;

            Color res = Color(0, 0, 0);
            if (camera->IsMultiSamplingOn())
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

    return ObtainColor(sumOfPixelSampleColors);
}

Color DefaultRenderer::RenderWithOneSample(const Camera *camera, int row, int column)
{
    Ray r = camera->GenerateRay(row, column);
    r.currMat = Material::DefaultMaterial;

    Vector3f pixelColor{};
    CalculateLight(r, pixelColor, 0, (float) column / camera->imgPlane.nx, (float)row / camera->imgPlane.ny);

    return ObtainColor(pixelColor);
}

Color DefaultRenderer::RenderPixel(Ray &cameraRay, float columnSampled01, float rowSampled01)
{
    Vector3f out;
    CalculateLight(cameraRay, out, 0, columnSampled01, rowSampled01);

    return ObtainColor(out);
}

void DefaultRenderer::CalculateLight(Ray &r, Vector3f &outColor, int depth, float ctw, float rth)
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

    contributionCalculator.CalculateLight(r, outColor, 0, ctw, rth);
}

}