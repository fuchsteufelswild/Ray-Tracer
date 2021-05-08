#include "RecursiveComputation.h"
#include "Intersection.h"
#include "Material.h"

namespace actracer
{

LightContributionCalculator::RecursiveComputation *LightContributionCalculator::RecursiveComputation::CreateRecursiveComputation(const SurfaceIntersection &intersection, const LightContributionCalculator &baseContributor, Ray &baseRay, float depth, Random<double> &randomGenerator)
{
    switch (intersection.mat->mType)
    {
        case Material::MatType::DIELECTRIC:
            return new RecursiveDielectricComputation(intersection, baseContributor, baseRay, depth, randomGenerator);
        case Material::MatType::CONDUCTOR:
            return new RecursiveConductorComputation(intersection, baseContributor, baseRay, depth, randomGenerator);
        case Material::MatType::MIRROR:
            return new RecursiveMirrorComputation(intersection, baseContributor, baseRay, depth, randomGenerator);
        default:
            break;
    }

    return nullptr;
}

LightContributionCalculator::RecursiveComputation::RecursiveComputation(const SurfaceIntersection &intersection, const LightContributionCalculator &baseContributor, Ray &baseRay, float depth, Random<double>& randomGenerator)
    : mIntersection(intersection), mBaseContributor(baseContributor), mBaseRay(baseRay), depth(depth), randomGenerator(randomGenerator)
{
    mPointToViewer = Normalize(baseRay.o - intersection.ip);
    mIntersectionSurfaceNormal = Normalize(intersection.n);
}

RecursiveRefractiveComputation::RecursiveRefractiveComputation(const SurfaceIntersection &intersection, const LightContributionCalculator &baseContributor, Ray &baseRay, float depth, Random<double> &randomGenerator)
    : RecursiveComputation(intersection, baseContributor, baseRay, depth, randomGenerator)
{
    mIsRayInsideObject = intersection.IsInternalReflection(baseRay);
    mFraction = 0;
}

RecursiveDielectricComputation::RecursiveDielectricComputation(const SurfaceIntersection &intersection, const LightContributionCalculator &baseContributor, Ray &baseRay, float depth, Random<double> &randomGenerator)
    : RecursiveRefractiveComputation(intersection, baseContributor, baseRay, depth, randomGenerator)
{ }

RecursiveConductorComputation::RecursiveConductorComputation(const SurfaceIntersection &intersection, const LightContributionCalculator &baseContributor, Ray &baseRay, float depth, Random<double> &randomGenerator)
    : RecursiveRefractiveComputation(intersection, baseContributor, baseRay, depth, randomGenerator)
{ }

RecursiveMirrorComputation::RecursiveMirrorComputation(const SurfaceIntersection &intersection, const LightContributionCalculator &baseContributor, Ray &baseRay, float depth, Random<double> &randomGenerator)
    : RecursiveComputation(intersection, baseContributor, baseRay, depth, randomGenerator)
{ }

void RecursiveConductorComputation::AddRecursiveComputationToColor(Vector3f &outColor)
{
    PerformRefraction(outColor);
    PerformReflection(outColor);
}

void RecursiveMirrorComputation::AddRecursiveComputationToColor(Vector3f &outColor)
{
    PerformReflection(outColor);
}

void LightContributionCalculator::RecursiveComputation::PerformReflection(Vector3f &outColor)
{
    Vector3f mirrorColor{};
    bool hasIntersection = ComputeMirrorColor(mirrorColor);

    if(hasIntersection)
        outColor = outColor + mirrorColor * mIntersection.mat->MRC * GetMirrorCoefficient();
}

float RecursiveMirrorComputation::GetMirrorCoefficient() const
{
    return mIntersection.mat->degamma ? 2.2f : 1.0f;
}

float RecursiveConductorComputation::GetMirrorCoefficient() const
{
    return mFraction;
}

bool LightContributionCalculator::RecursiveComputation::ComputeMirrorColor(Vector3f &mirrorColor)
{
    Vector3f viewerReflectionDirection = GetViewerReflectionDirection();
    ComputeTiltedGlossyReflectionDirection(viewerReflectionDirection);

    Ray tempRay = Ray(mIntersection.ip + mIntersectionSurfaceNormal * mBaseContributor.GetShadownRayEpsilon(), viewerReflectionDirection, mBaseRay.currMat, mBaseRay.currShape, mBaseRay.time);
    bool hasIntersection = mBaseContributor.CalculateLight(tempRay, mirrorColor, depth + 1); // Calculate color of the object for the viewer reflection direction ray

    return hasIntersection;
}

void LightContributionCalculator::RecursiveComputation::ComputeTiltedGlossyReflectionDirection(Vector3f &vrd) const
{
    if(mIntersection.mat->roughness > 0)
    {
        int mIndex = MinAbsElementIndex(vrd); // Get minimum absolute index

        // Create an orthonormal basis using vrd
        Vector3f rprime = vrd;
        rprime[mIndex] = 1.0f;
        rprime = Normalize(rprime);

        Vector3f u = Normalize(Cross(vrd, rprime));
        Vector3f v = Normalize(Cross(vrd, u));
        // --

        // Tilt randomly to sides
        Vector3f r1 = static_cast<float>((randomGenerator)(-0.5, 0.5)) * u;
        Vector3f r2 = static_cast<float>((randomGenerator)(-0.5, 0.5)) * v;
        // --

        // Compute the resulting reflection ray
        Vector3f resulting = Normalize(vrd + mIntersection.mat->roughness * (r1 + r2));

        vrd = resulting;
    }
}

Vector3f LightContributionCalculator::RecursiveComputation::GetViewerReflectionDirection() const
{
    return Normalize(mIntersectionSurfaceNormal * 2.0f * Dot(mPointToViewer, mIntersectionSurfaceNormal) - mPointToViewer);
}

void RecursiveDielectricComputation::AddRecursiveComputationToColor(Vector3f &outColor)
{
    PerformRefraction(outColor);
    AddDielectricConsumption(outColor);
}

void RecursiveDielectricComputation::AddDielectricConsumption(Vector3f &outColor)
{
    Vector3f viewerReflectionDirection = GetViewerReflectionDirection();
    Vector3f mirrorColor = Vector3f(0.0f, 0.0f, 0.0f);
    Ray tempRay = Ray(mIntersection.ip + mIntersectionSurfaceNormal * mBaseContributor.GetShadownRayEpsilon(), viewerReflectionDirection, mBaseRay.currMat, mBaseRay.currShape, mBaseRay.time);
    mBaseContributor.CalculateLight(tempRay, mirrorColor, depth + 1); // Calculate color of the object for the viewer reflection direction ray

    outColor = outColor + mirrorColor * mFraction;

    if (mIsRayInsideObject)
        outColor *= GetDielectricPowerAbsorptionParameter();
}

Vector3f RecursiveDielectricComputation::GetDielectricPowerAbsorptionParameter()
{
    Vector3f ac = mIntersection.mat->ACR();

    float distance = mBaseRay(mIntersection.ip);
    float absorptionCoefficient = 0.01f * distance;
    float ecoeff = std::exp(-absorptionCoefficient);

    return ecoeff * Vector3f{std::exp(-ac.x * distance), std::exp(-ac.y * distance), std::exp(-ac.z * distance)};
}

void RecursiveRefractiveComputation::PerformRefraction(Vector3f& outColor)
{
    Vector3f tiltedRay{};
    if (ComputeRefractionParameters(tiltedRay))
    {
        bool intersectsAnObject = false;
        Vector3f fractionColor{};
        if (!mIsRayInsideObject)
        {
            Ray refractionRay = Ray(mIntersection.ip + -mIntersectionSurfaceNormal * mBaseContributor.GetShadownRayEpsilon(), tiltedRay, mIntersection.mat, mIntersection.shape, mBaseRay.time);
            intersectsAnObject = mBaseContributor.CalculateLight(refractionRay, fractionColor, depth + 1);
        }
        else
        {
            Ray refractionRay = Ray(mIntersection.ip + -mIntersectionSurfaceNormal * mBaseContributor.GetShadownRayEpsilon(), tiltedRay, Material::DefaultMaterial, nullptr, mBaseRay.time);
            intersectsAnObject = mBaseContributor.CalculateLight(refractionRay, fractionColor, depth + 1);
        }

        if (intersectsAnObject)
            outColor = outColor + fractionColor * (1 - mFraction);
    }
    else if (mIsRayInsideObject)
    {
        mFraction = 1;
    }
}

bool RecursiveRefractiveComputation::ComputeRefractionParameters(Vector3f &tiltedRay)
{
    float corr;
    std::pair<float, float> materialRefractionIndices = ComputeFresnel(corr);

    float n1 = materialRefractionIndices.first;
    float n2 = materialRefractionIndices.second;

    float param = n1 / n2;
    float cost = 1 - corr * corr;
    float det = 1 - param * param * cost;

    // Check if we can pass through the media
    if (det >= 0)
    {
        tiltedRay = Normalize((mBaseRay.d + mIntersectionSurfaceNormal * corr) * (param)- mIntersectionSurfaceNormal * std::sqrt(det));
        return true;
    }

    return false;
}

std::pair<float, float> RecursiveRefractiveComputation::ComputeFresnel(float &corr)
{
    float outsideMatIndex = mBaseRay.currMat->rIndex;
    float insideMatIndex = mIntersection.mat->rIndex;

    if (mIsRayInsideObject)
        insideMatIndex = 1;

    corr = Dot(mBaseRay.d, mIntersectionSurfaceNormal);
    corr = Clamp<float>(corr, -1, 1);

    if (corr > 0) // Invert the normal for internal refraction
    {
        if (mIntersection.mat->mType == Material::MatType::CONDUCTOR)
            std::swap(outsideMatIndex, insideMatIndex);
        mIntersectionSurfaceNormal = -mIntersectionSurfaceNormal;
    }
    else
        corr = -corr;

    float sin1 = std::sqrt(1 - corr * corr);
    float sin2 = (outsideMatIndex * sin1) / insideMatIndex;
    float cos2 = std::sqrt(1 - sin2 * sin2);
    mFraction = mIntersection.mat->ComputeFresnelEffect(outsideMatIndex, insideMatIndex, corr, cos2);
    return std::make_pair(outsideMatIndex, insideMatIndex);
}

}