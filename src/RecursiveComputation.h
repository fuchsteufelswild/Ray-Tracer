#pragma once

#include "LightContributionCalculator.h"
#include "Random.h"

namespace actracer
{

class SurfaceIntersection;
class LightContributionCalculator;
class Ray;

class LightContributionCalculator::RecursiveComputation
{
public:
    static RecursiveComputation* CreateRecursiveComputation(const SurfaceIntersection &intersection, const LightContributionCalculator &baseContributor, Ray &baseRay, float depth, Random<double>& randomGenerator);
    virtual void AddRecursiveComputationToColor(Vector3f &outColor) = 0;
protected:
    RecursiveComputation(const SurfaceIntersection &intersection, const LightContributionCalculator &baseContributor, Ray &baseRay, float depth, Random<double>& randomGenerator);
protected:
    Vector3f GetViewerReflectionDirection() const;
protected:
    /*
     * Fires ray and adds the reflection color into out color
     */ 
    void PerformReflection(Vector3f& outColor);
    bool ComputeMirrorColor(Vector3f &mirrorColor);

    void ComputeTiltedGlossyReflectionDirection(Vector3f &vrd) const;
    virtual float GetMirrorCoefficient() const { return 0; }
protected:
    float depth;
    Vector3f mIntersectionSurfaceNormal;

    Vector3f mPointToViewer;
    const SurfaceIntersection& mIntersection;
    const LightContributionCalculator& mBaseContributor;
    Ray& mBaseRay;

    Random<double>& randomGenerator;
};

class RecursiveRefractiveComputation : public LightContributionCalculator::RecursiveComputation
{
public:
    RecursiveRefractiveComputation(const SurfaceIntersection &intersection, const LightContributionCalculator &baseContributor, Ray &baseRay, float depth, Random<double> &randomGenerator);

protected:
    void PerformRefraction(Vector3f &outColor);
    // Calculates the refraction color and returns if the ray is intersecting from inside to outside
    bool ComputeRefractionParameters(Vector3f &tiltedRay);
    /* 
     * Returns fresnel value for the given intersection point and a ray
     * Side-effects are the change of the surface normal and flag that
     * indicates if the ray is coming from inside to outside
     * The cosine the ray and the normal also changes
     * Returns the outside and inside materials' refraction index values
     */
    std::pair<float, float> ComputeFresnel(float &corr);

protected:
    float mFraction;
    bool mIsRayInsideObject;
};

class RecursiveDielectricComputation : public RecursiveRefractiveComputation
{
public:
    RecursiveDielectricComputation(const SurfaceIntersection &intersection, const LightContributionCalculator &baseContributor, Ray &baseRay, float depth, Random<double> &randomGenerator);

    virtual void AddRecursiveComputationToColor(Vector3f &outColor) override;
private:
    void AddDielectricConsumption(Vector3f &outColor);
    Vector3f GetDielectricPowerAbsorptionParameter();
};

class RecursiveConductorComputation : public RecursiveRefractiveComputation
{
public:
    RecursiveConductorComputation(const SurfaceIntersection &intersection, const LightContributionCalculator &baseContributor, Ray &baseRay, float depth, Random<double> &randomGenerator);

    virtual void AddRecursiveComputationToColor(Vector3f &outColor) override;

protected:
    virtual float GetMirrorCoefficient() const override;
};

class RecursiveMirrorComputation : public LightContributionCalculator::RecursiveComputation
{
public:
    RecursiveMirrorComputation(const SurfaceIntersection &intersection, const LightContributionCalculator &baseContributor, Ray &baseRay, float depth, Random<double> &randomGenerator);

    virtual void AddRecursiveComputationToColor(Vector3f &outColor) override;
protected:
    virtual float GetMirrorCoefficient() const override;
};

}