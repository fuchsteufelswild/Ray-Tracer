#include "DefaultRenderer.h"
#include "Scene.h"

#include "Camera.h"
#include "Image.h"

#include "Tonemapper.h"
#include "Timer.h"


namespace actracer
{

void DefaultRenderer::RenderSceneIntoPPM(Scene* scene)
{
    RetrieveRenderingParamsFromScene(scene);

    for (Camera* camera : scene->GetAllCameras()) // For each camera in the scene
    {
        Timer cameraRenderTimer{camera->imageName};

        Image img(camera->imgPlane.nx, camera->imgPlane.ny);
        int rowDiff = camera->imgPlane.ny / 8; // Get row difference between successive chunks

        RenderCameraViewOntoImage(camera, img, 0, camera->imgPlane.ny);

        // std::thread th1(&Scene::RenderCam, this, cam, std::ref(img), 0, cam->imgPlane.ny);
        // th1.join();
        // std::thread th1(&Scene::RenderCam, this, cam, std::ref(img), 0, rowDiff);
        // std::thread th2(&Scene::RenderCam, this, cam, std::ref(img), rowDiff * 1, rowDiff * 2);
        // std::thread th3(&Scene::RenderCam, this, cam, std::ref(img), rowDiff * 2, rowDiff * 3);
        // std::thread th4(&Scene::RenderCam, this, cam, std::ref(img), rowDiff * 3, rowDiff * 4);
        // std::thread th5(&Scene::RenderCam, this, cam, std::ref(img), rowDiff * 4, rowDiff * 5);
        // std::thread th6(&Scene::RenderCam, this, cam, std::ref(img), rowDiff * 5, rowDiff * 6);
        // std::thread th7(&Scene::RenderCam, this, cam, std::ref(img), rowDiff * 6, rowDiff * 7);
        // std::thread th8(&Scene::RenderCam, this, cam, std::ref(img), rowDiff * 7, cam->imgPlane.ny);

        // th1.join(); th2.join(); th3.join(); th4.join(); th5.join(); th6.join(); th7.join(); th8.join();
        
        SaveResultingImage(camera->imageName, img);
    }
}

Image& DefaultRenderer::RenderCameraViewOntoImage(Camera *camera, Image &image, int startRowIndex, int endRowIndex)
{

}

void DefaultRenderer::SaveResultingImage(const char* imageName, Image& image)
{
    if (tonemapper)
    {
        float *tonemappedColorOutputValues = tonemapper->Tonemap(image);
        tonemapper->SaveEXR(tonemappedColorOutputValues, image.GetImageWidth(), image.GetImageHeight(), imageName);
    }
    else
        image.SaveImageAsPPM(imageName);
}

void DefaultRenderer::RetrieveRenderingParamsFromScene(Scene *scene)
{
    // maximumRecursionDepth = scene->GetMaximumRecursionDepth();
    // intersectionTestEpsilon = scene->GetIntersectionTestEpsilon();
    // shadowRayEpsilon = scene->GetShadowRayEpsilon();

    // backgroundColor = scene->GetBackgroundColor();
    // ambientColor = scene->GetAmbientColor();
}

}