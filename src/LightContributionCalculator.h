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

    Vector3f GetBackgroundColorAt(int columnOverWidth, int rowOverHeight);

private:
    Vector3f mStaticBackgroundColor;
    const Texture *mBackgroundTexture;
};

class LightContributionCalculator
{

public:
    /*
     * columnOverWidth -> ColumnPosition Mapped to 01: column / width
     * rowOverHeight -> RowPosition Mapped to 01: row / height
     * depth -> reflection/refraction Depth
     */
    bool CalculateLight(Ray &cameraRay, Vector3f &outColor, int depth, float ctw = 0, float rth = 0);

    void SetSceneLights(const std::vector<Light*>& lights, const Vector3f& ambientLightColor, const Vector3f& backgroundColor, const Texture* backgroundTexture);
    void SetSceneAccelerator(const AccelerationStructure& accelerator);
    void SetRenderParameters(int maximumRecursionDepth, float intersectionTestEpsilon, float shadowRayEpsilon, float gamma, Random<double>& randomGenerator);
private:
    void CalculateContribution(Ray &cameraRay, SurfaceIntersection &intersectedSurface, Vector3f &outColor, int depth) const;
    Vector3f CalculateAmbientLightContribution(const Ray &ray, const SurfaceIntersection &intersectedSurface) const;
    void ComputeTiltedGlossyReflectionRay(Vector3f &vrd, const Material *intersectionMat);
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


};




}