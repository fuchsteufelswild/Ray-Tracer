#pragma once

#include <vector>

#include "acmath.h"
#include "Random.h"

namespace actracer
{

class Ray;
class Light;
class Texture;
class AccelerationStructure;
class SurfaceIntersection;
class Material;

class BackgroundColor
{
public:
    void Init(const Vector3f &staticBackgroundColor, const Texture *backgroundTexture);

    Vector3f GetBackgroundColorAt(int columnOverWidth, int rowOverHeight) const;

private:
    Vector3f mStaticBackgroundColor;
    const Texture *mBackgroundTexture;
};

class LightContributionCalculator
{
public:
    float GetShadownRayEpsilon() const;
public:
    /*
     * columnNormalized01 -> ColumnPosition Mapped to 01: column / width
     * rowNormalized01 -> RowPosition Mapped to 01: row / height
     * depth -> reflection/refraction Depth
     */
    bool CalculateLight(Ray &cameraRay, Vector3f &outColor, int depth, float columnNormalized01 = 0, float rowNormalized01 = 0) const;

    void SetSceneLights(const std::vector<Light*>& lights, const Vector3f& ambientLightColor, const Vector3f& backgroundColor, const Texture* backgroundTexture);
    void SetSceneAccelerator(const AccelerationStructure& accelerator);
    void SetRenderParameters(int maximumRecursionDepth, float intersectionTestEpsilon, float shadowRayEpsilon, float gamma, Random<double>& randomGenerator);
private:
    void CalculateContribution(Ray &cameraRay, SurfaceIntersection &intersectedSurface, Vector3f &outColor, int depth) const;

    Vector3f ProcessLights(const SurfaceIntersection &intersectedSurface, const Vector3f &viewerDirection, float rayTime = 0.0f) const;
    Vector3f ProcessLight(const Light *light, const SurfaceIntersection &intersectedSurface, const Vector3f &pointToViewer, float rayTime) const;
    Vector3f CalculateAmbientLightContribution(const SurfaceIntersection &intersectedSurface) const;

    bool IsThereAnObjectBetweenLightAndIntersectionPoint(const SurfaceIntersection &intersection, const Vector3f &pointToLight, const float distanceToClosestObject, float rayTime) const;
private:
    const std::vector<Light*>* lights;
    Random<double>* randomGenerator;

    float gamma;

    int maximumRecursionDepth;
    float intersectionTestEpsilon;
    float shadowRayEpsilon;

    Vector3f mAmbientLightColor;
    BackgroundColor mBackgroundColor;
    const AccelerationStructure* accelerator;
public:
    class RecursiveComputation;

};

inline float LightContributionCalculator::GetShadownRayEpsilon() const
{
    return shadowRayEpsilon;
}

}