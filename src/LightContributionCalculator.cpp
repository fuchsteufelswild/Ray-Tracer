#include "LightContributionCalculator.h"

#include "Texture.h"
#include "Intersection.h"
#include "AccelerationStructure.h"
#include "Material.h"

#include "Shape.h"
#include "Light.h"
#include "brdf.h"

namespace actracer
{

bool IsInternalReflection(const Ray& ray, const SurfaceIntersection& intersectedSurface);

// Returns fresnel value for the given intersection point and a ray
// Side-effects are the change of the surface normal and flag that
// indicates if the ray is coming from inside to outside
// Correlation between the ray and the normal also changes
// Returns the new n1, n2 values
static std::pair<float, float> ComputeFresnel(const SurfaceIntersection &intersection, const Ray &r, bool &flag, Vector3f &sn, float &corr, float &Fr)
{
    float n1 = r.currMat->rIndex;        // Index of outside mat
    float n2 = intersection.mat->rIndex; // Index of inside mat

    if (intersection.shape && r.currShape && intersection.shape == r.currShape) // If the ray is inside
    {
        flag = true;
        n2 = 1;
    }

    corr = Dot(r.d, sn);
    corr = Clamp<float>(corr, -1, 1);

    if (corr > 0) // Invert the normal for internal refraction
    {
        if (intersection.mat->mType == Material::MatType::CONDUCTOR)
            std::swap(n1, n2);
        sn = -sn;
    }
    else
        corr = -corr;

    float sin1 = std::sqrt(1 - corr * corr);
    float sin2 = (n1 * sin1) / n2;
    float cos2 = std::sqrt(1 - sin2 * sin2);
    Fr = intersection.mat->ComputeFresnelEffect(n1, n2, corr, cos2);
    return std::make_pair(n1, n2);
}

// Calculates the refraction color and returns if the ray is intersecting from inside to outside
static bool SetRefraction(const SurfaceIntersection &intersection, const Ray &r, bool &flag, Vector3f &sn, float &Fr, Vector3f &tiltedRay)
{
    float corr;
    std::pair<float, float> tee = ComputeFresnel(intersection, r, flag, sn, corr, Fr);

    float n1 = tee.first;
    float n2 = tee.second;
    float param = n1 / n2;
    float cost = 1 - corr * corr;
    float det = 1 - param * param * cost;

    // Check if we can pass through the media
    if (det >= 0)
    {
        tiltedRay = Normalize((r.d + sn * corr) * (param)-sn * std::sqrt(det));
        return true;
    }

    return false;
}

// Compute the power absorption for given ray and intersection point
static Vector3f DielectricPowerAbsorption(const Ray& r, const Intersection& intersection)
{
    Vector3f ac = intersection.mat->ACR(); // Retrieve material AC
    float distt = r(intersection.ip); // Distance
    float coeff = 0.01f * distt; // Absorption coeff
    float ecoeff = std::exp(-coeff);
    Vector3f param = {std::exp(-ac.x * distt), std::exp(-ac.y * distt), std::exp(-ac.z * distt)};

    return param;
}

// Get the tilted glossy reflection ray
void LightContributionCalculator::ComputeTiltedGlossyReflectionRay(Vector3f &vrd, const Material *intersectionMat) const
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
    Vector3f r1 = static_cast<float>((*randomGenerator)(-0.5, 0.5)) * u;
    Vector3f r2 = static_cast<float>((*randomGenerator)(-0.5, 0.5)) * v;
    // --

    // Compute the resulting reflection ray
    Vector3f resulting = Normalize(vrd + intersectionMat->roughness * (r1 + r2));

    vrd = resulting;
}

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

bool LightContributionCalculator::CalculateLight(Ray &cameraRay, Vector3f &outColor, int depth, float columnOverWidth, float rowOverHeight) const
{
    SurfaceIntersection intersection{};
    accelerator->Intersect(cameraRay, intersection);

    if (intersection.IsValid())
    {
        // Do not calculate costly light contribution if texture replaces all color directly
        if (intersection.DoesSurfaceTextureReplaceAllColor())
            outColor = intersection.mColorChangerTexture->RetrieveRGBFromUV(intersection.uv.x, intersection.uv.y);

        intersection.TweakSurfaceNormal();
        CalculateContribution(cameraRay, intersection, outColor, depth);

        return true;
    }
    else
        outColor = mBackgroundColor.GetBackgroundColorAt(columnOverWidth, rowOverHeight);

    return false;
}

void LightContributionCalculator::CalculateContribution(Ray &cameraRay, SurfaceIntersection &intersectedSurface, Vector3f &outColor, int depth) const
{
    Vector3f pointToViewer = Normalize(cameraRay.o - intersectedSurface.ip);

    if(!IsInternalReflection(cameraRay, intersectedSurface))
    {
        outColor += CalculateAmbientLightContribution(intersectedSurface);
        outColor += ProcessLights(intersectedSurface, pointToViewer, cameraRay.time);
    }

    if (depth < maximumRecursionDepth && intersectedSurface.mat->mType != Material::MatType::DEFAULT)
    {
        Vector3f sn = Normalize(intersectedSurface.n); // Surface normal
        float Fr = 1;
        bool flag = false;
        if (intersectedSurface.mat->mType != Material::MatType::MIRROR)
        {
            Vector3f tiltedRay{};
            bool hasIntersected = false;
            if (SetRefraction(intersectedSurface, cameraRay, flag, sn, Fr, tiltedRay)) // Get the tilted ray
            {
                Vector3f fracColor{};
                if (!flag) // Ray is coming from outside
                {
                    Ray tempRay = Ray(intersectedSurface.ip + -sn * shadowRayEpsilon, tiltedRay, intersectedSurface.mat, intersectedSurface.shape, cameraRay.time);
                    hasIntersected = CalculateLight(tempRay, fracColor, depth + 1);
                }
                else // Ray is inside
                {
                    Ray tempRay = Ray(intersectedSurface.ip + -sn * shadowRayEpsilon, tiltedRay, Material::DefaultMaterial, nullptr, cameraRay.time);
                    hasIntersected = CalculateLight(tempRay, fracColor, depth + 1);
                }
                if (hasIntersected)
                    outColor = outColor + fracColor * (1 - Fr);
            }
            else if (intersectedSurface.shape == cameraRay.currShape) // Did not change
                Fr = 1;
        }

        if (intersectedSurface.mat->mType == Material::MatType::MIRROR || intersectedSurface.mat->mType == Material::MatType::CONDUCTOR)
        {

            if (intersectedSurface.mat->MRC.x != 0 || intersectedSurface.mat->MRC.y != 0 || intersectedSurface.mat->MRC.z != 0)
            {
                float gam = 1.0f;
                if (intersectedSurface.mat->degamma)
                    gam = 2.2f;

                Vector3f vrd = Normalize(sn * 2.0f * Dot(pointToViewer, sn) - pointToViewer);     // Viewer reflection direction
                Vector3f mirCol = Vector3f(0.0f, 0.0f, 0.0f);                                     // Resulting mirror color

                bool hasIntersection = false;

                // If roughness > 0 apply glossy reflection
                if (intersectedSurface.mat->roughness > 0)
                    ComputeTiltedGlossyReflectionRay(vrd, intersectedSurface.mat);

                Ray tempRay = Ray(intersectedSurface.ip + sn * shadowRayEpsilon, vrd, cameraRay.currMat, cameraRay.currShape, cameraRay.time);
                hasIntersection = CalculateLight(tempRay, mirCol, depth + 1); // Calculate color of the object for the viewer reflection direction ray

                if (hasIntersection && intersectedSurface.mat->mType == Material::MatType::MIRROR)
                    outColor = outColor + mirCol * intersectedSurface.mat->MRC * gam;
                if (hasIntersection && intersectedSurface.mat->mType == Material::MatType::CONDUCTOR) // If it Intersects with an object
                    outColor = outColor + mirCol * intersectedSurface.mat->MRC * Fr;
            }
        }

        if (intersectedSurface.mat->mType == Material::MatType::DIELECTRIC)
        {
            Vector3f vrd = Normalize(sn * 2.0f * Dot(pointToViewer, sn) - pointToViewer); // Viewer reflection direction
            Vector3f mirCol = Vector3f(0.0f, 0.0f, 0.0f); // Resulting mirror color

            Ray tempRay = Ray(intersectedSurface.ip + sn * shadowRayEpsilon, vrd, cameraRay.currMat, cameraRay.currShape, cameraRay.time);
            CalculateLight(tempRay, mirCol, depth + 1); // Calculate color of the object for the viewer reflection direction ray

            if (intersectedSurface.mat->mType == Material::MatType::DIELECTRIC)
            {
                outColor = outColor + mirCol * Fr;

                if (flag) // If ray was inside
                {
                    Vector3f param = DielectricPowerAbsorption(cameraRay, intersectedSurface);
                    outColor *= param;
                }
            }
        }
    }
}

bool IsInternalReflection(const Ray &ray, const SurfaceIntersection &intersectedSurface)
{
    return intersectedSurface.shape && ray.currShape && intersectedSurface.shape == ray.currShape;
}

Vector3f LightContributionCalculator::CalculateAmbientLightContribution(const SurfaceIntersection &intersectedSurface) const
{
    return mAmbientLightColor * intersectedSurface.mat->ARC;
}

Vector3f LightContributionCalculator::ProcessLights(const SurfaceIntersection &intersectedSurface, const Vector3f &pointToViewer, float rayTime) const
{
    Vector3f allLightContribution{};
    allLightContribution += CalculateAmbientLightContribution(intersectedSurface);

    for (Light *light : *lights)
        allLightContribution += ProcessLight(light, intersectedSurface, pointToViewer, rayTime);

    return allLightContribution;
}

Vector3f LightContributionCalculator::ProcessLight(Light *light, const SurfaceIntersection &intersectedSurface, const Vector3f &pointToViewer, float rayTime) const
{
    Vector3f pointToLight;
    float distanceToLight;
    light->AssignLightFormulaVariables(intersectedSurface, pointToLight, distanceToLight);

    if(!IsThereAnObjectBetweenLightAndIntersectionPoint(intersectedSurface, pointToLight, distanceToLight, rayTime))
        return light->ComputeResultingColorContribution(intersectedSurface, pointToViewer, pointToLight, distanceToLight);

    return {};
}

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