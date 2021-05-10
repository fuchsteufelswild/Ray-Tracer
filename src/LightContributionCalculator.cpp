#include "LightContributionCalculator.h"

#include "Texture.h"
#include "Intersection.h"
#include "AccelerationStructure.h"
#include "Material.h"

#include "Shape.h"
#include "Light.h"
#include "brdf.h"

#include "RecursiveComputation.h"

namespace actracer
{

bool IsInternalReflection(const Ray& ray, const SurfaceIntersection& intersectedSurface);

void LightContributionCalculator::SetSceneLights(const std::vector<Light*>& lights, const Vector3f& ambientLightColor, 
                                                 const Vector3f& backgroundColor, const Texture* backgroundTexture)
{
    this->lights = &lights;
    mAmbientLightColor = ambientLightColor;
    mBackgroundColor.Init(backgroundColor, backgroundTexture);
}

void LightContributionCalculator::SetSceneAccelerator(const AccelerationStructure &accelerator)
{
    this->accelerator = &accelerator;
}

void LightContributionCalculator::SetRenderParameters(int maximumRecursionDepth, float intersectionTestEpsilon, float shadowRayEpsilon, float gamma, Random<double>& randomGenerator)
{
    this->maximumRecursionDepth = maximumRecursionDepth;
    this->intersectionTestEpsilon = intersectionTestEpsilon;
    this->shadowRayEpsilon = shadowRayEpsilon;
    this->gamma = gamma;

    this->randomGenerator = &randomGenerator;
}

/*
 * Recursive light calculation,
 * returns true if ray intersects with any primitive
 */ 
bool LightContributionCalculator::CalculateLight(Ray &cameraRay, Vector3f &outColor, int depth, float columnNormalized01, float rowNormalized01) const
{
    SurfaceIntersection intersection{};
    accelerator->Intersect(cameraRay, intersection);

    if (intersection.IsValid())
    {
        // Do not calculate costly light contribution if texture replaces all color directly
        if (intersection.DoesSurfaceTextureReplaceAllColor())
            outColor = intersection.mColorChangerTexture->RetrieveRGBFromUV(intersection.uv.x, intersection.uv.y);
        else
        {
            intersection.TweakSurfaceNormal();
            CalculateContribution(cameraRay, intersection, outColor, depth);
        }

        return true;
    }
    else
        outColor = mBackgroundColor.GetBackgroundColorAt(columnNormalized01, rowNormalized01);

    return false;
}

/*
 * Calls the light contribution computation then calls recursive computation
 */ 
void LightContributionCalculator::CalculateContribution(Ray &cameraRay, SurfaceIntersection &intersectedSurface, Vector3f &outColor, int depth) const
{
    Vector3f pointToViewer = Normalize(cameraRay.o - intersectedSurface.ip);

    if(!IsInternalReflection(cameraRay, intersectedSurface))
    {
        outColor += ProcessLights(intersectedSurface, pointToViewer, cameraRay.time);
    }

    if (depth < maximumRecursionDepth)
    {
        RecursiveComputation* recursiveComputation = RecursiveComputation::CreateRecursiveComputation(intersectedSurface, *this, cameraRay, depth, *randomGenerator);
        if(recursiveComputation)
        {
            recursiveComputation->AddRecursiveComputationToColor(outColor);
            delete recursiveComputation;
        }
    }
}

bool IsInternalReflection(const Ray &ray, const SurfaceIntersection &intersectedSurface)
{
    return intersectedSurface.containerShape && ray.currShape && intersectedSurface.containerShape == ray.currShape;
}

Vector3f LightContributionCalculator::ProcessLights(const SurfaceIntersection &intersectedSurface, const Vector3f &pointToViewer, float rayTime) const
{
    Vector3f allLightContribution{};
    allLightContribution += CalculateAmbientLightContribution(intersectedSurface);

    for (Light *light : *lights)
        allLightContribution += ProcessLight(light, intersectedSurface, pointToViewer, rayTime);

    return allLightContribution;
}

Vector3f LightContributionCalculator::CalculateAmbientLightContribution(const SurfaceIntersection &intersectedSurface) const
{
    return mAmbientLightColor * intersectedSurface.mat->GetAmbientReflectionCoefficient();
}

Vector3f LightContributionCalculator::ProcessLight(const Light *light, const SurfaceIntersection &intersectedSurface, const Vector3f &pointToViewer, float rayTime) const
{
    Vector3f pointToLight;
    float distanceToLight;
    light->AssignLightFormulaVariables(intersectedSurface, pointToLight, distanceToLight);

    if(!IsThereAnObjectBetweenLightAndIntersectionPoint(intersectedSurface, pointToLight, distanceToLight, rayTime))
        return light->ComputeResultingColorContribution(intersectedSurface, pointToViewer, pointToLight, distanceToLight);

    return {};
}

/*
 * Fires a ray from intersection point through light with a small epsilon with surface normal(to prevent self intersection),
 * if this ray intersects with an object that is closer than light returns true
 */ 
bool LightContributionCalculator::IsThereAnObjectBetweenLightAndIntersectionPoint(const SurfaceIntersection &intersection, const Vector3f &pointToLight, const float distanceToLight, float rayTime) const
{
    SurfaceIntersection closestObjectInPath{};
    Ray tempRay = Ray(intersection.ip + intersection.n * shadowRayEpsilon, pointToLight, nullptr, nullptr, rayTime);
    accelerator->Intersect(tempRay, closestObjectInPath);

    if (closestObjectInPath.IsValid())                                                                                                                       
    {
        float distanceToClosestObject = SqLength(closestObjectInPath.ip - intersection.ip);

        if (distanceToClosestObject > shadowRayEpsilon && distanceToClosestObject < distanceToLight - shadowRayEpsilon)
            return true;
    }

    return false;
}

Vector3f BackgroundColor::GetBackgroundColorAt(int columnOverWidth, int rowOverHeight) const
{
    if(mBackgroundTexture)
        return mBackgroundTexture->RetrieveRGBFromUV(columnOverWidth, rowOverHeight);

    return mStaticBackgroundColor;
}

void BackgroundColor::Init(const Vector3f& staticBackgroundColor, const Texture* backgroundTexture)
{
    mStaticBackgroundColor = staticBackgroundColor;
    mBackgroundTexture = backgroundTexture;
}

}