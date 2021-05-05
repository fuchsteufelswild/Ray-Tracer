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
void LightContributionCalculator::ComputeTiltedGlossyReflectionRay(Vector3f &vrd, const Material *intersectionMat)
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

bool LightContributionCalculator::CalculateLight(Ray &r, Vector3f &outColor, int depth, float columnOverWidth, float rowOverHeight)
{
    SurfaceIntersection intersection{};
    accelerator->Intersect(r, intersection); // Find the closest object in the path

    if(!intersection.IsValid())
    {
        outColor = mBackgroundColor.GetBackgroundColorAt(columnOverWidth, rowOverHeight);
        return false;
    }

    if(intersection.DoesSurfaceTextureReplaceAllColor())
    {
        outColor = intersection.mColorChangerTexture->RetrieveRGBFromUV(intersection.uv.x, intersection.uv.y);

        return true;
    }

    if (!(intersection.shape && r.currShape && intersection.shape == r.currShape)) // If it is not an internal reflection
        outColor = mAmbientLightColor * intersection.mat->ARC;                     // Add ambient light to object colour

    Vector3f viewerDirection = Normalize(r.o - intersection.ip); // Camera look direction

    intersection.TweakSurfaceNormal();

    for (Light *light : *lights) // For each point light
    {

        if (intersection.shape && r.currShape && intersection.shape == r.currShape) // If internal reflection
            break;

        Vector3f pointToLight = light->GetLightPosition() - intersection.ip; // Light direction

        float distanceToLight = SqLength(pointToLight); // Distance to light

        pointToLight = Normalize(pointToLight); // Normalize for the ray direction

        if (light->lType == Light::LightType::DIRECTIONAL)
        {
            distanceToLight = 1e9;
            pointToLight = -light->GetLightDirection();
            pointToLight = Normalize(pointToLight);
        }
        else if (light->lType == Light::LightType::AREA)
        {
            Vector3f tempPos = light->GetLightDirection();
            pointToLight = tempPos - intersection.ip;

            distanceToLight = SqLength(pointToLight);
            pointToLight = Normalize(pointToLight);
        }
        else if (light->lType == Light::LightType::ENVIRONMENT)
        {
            Vector3f tempPos = light->GetLightDirection(intersection.n);
            pointToLight = tempPos;

            distanceToLight = SqLength(pointToLight);
            pointToLight = Normalize(pointToLight);
        }

        // Closest object between the intersection point and light
        SurfaceIntersection closestObjectInPath{};

        Ray tempRay = Ray(intersection.ip + intersection.n * shadowRayEpsilon, pointToLight, nullptr, nullptr, r.time);

        accelerator->Intersect(tempRay, closestObjectInPath); // Find closest object in the path
        if (closestObjectInPath.mat)                                             // If found                                                                                                                                                                // Closest object in the path of the ray from intersection point to light exists
        {
            float distanceToObject = SqLength(closestObjectInPath.ip - intersection.ip); // Distance to closest found object between intersection point and the light

            if (distanceToObject > shadowRayEpsilon && distanceToObject < distanceToLight - shadowRayEpsilon) // Object is closer than light ( it is in-between )
                continue;
        }

        if (intersection.mat->brdf)
        {
            Vector3f wi = pointToLight;
            Vector3f wo = viewerDirection;
            Vector3f kd = intersection.mat->DRC;
            Vector3f ks = intersection.mat->SRC;
            Vector3f surfaceNormal = intersection.n;

            Vector3f brdfValue = intersection.mat->brdf->f(wi, wo, surfaceNormal, kd, ks, intersection.mat->rIndex);
            outColor += light->GetLightIntensity() / (distanceToLight * distanceToLight) * brdfValue;
        }
        else
        {
            outColor += light->ResultingColorContribution(intersection, viewerDirection, intersection.mColorChangerTexture, gamma); // Add the light contribution
        }
            
    }

    // Recursive tracing
    if (depth < maximumRecursionDepth && intersection.mat->mType != Material::MatType::DEFAULT)
    {
        Vector3f sn = Normalize(intersection.n); // Surface normal
        float Fr = 1;
        bool flag = false;
        if (intersection.mat->mType != Material::MatType::MIRROR)
        {
            Vector3f tiltedRay{};

            if (SetRefraction(intersection, r, flag, sn, Fr, tiltedRay)) // Get the tilted ray
            {
                bool hasIntersected = false;
                Vector3f fracColor{};
                if (!flag) // Ray is coming from outside
                {
                    Ray tempRay = Ray(intersection.ip + -sn * shadowRayEpsilon, tiltedRay, intersection.mat, intersection.shape, r.time);
                    hasIntersected = CalculateLight(tempRay, fracColor, depth + 1);
                }
                else // Ray is inside
                {
                    Ray tempRay = Ray(intersection.ip + -sn * shadowRayEpsilon, tiltedRay, Material::DefaultMaterial, nullptr, r.time);
                    hasIntersected = CalculateLight(tempRay, fracColor, depth + 1);
                }
                if (hasIntersected) // If it Intersects with an object
                    outColor = outColor + fracColor * (1 - Fr);
            }
            else if (intersection.shape == r.currShape) // Did not change
                Fr = 1;
        }

        if (intersection.mat->mType == Material::MatType::MIRROR || intersection.mat->mType == Material::MatType::CONDUCTOR)
        {

            if (intersection.mat->MRC.x != 0 || intersection.mat->MRC.y != 0 || intersection.mat->MRC.z != 0)
            {
                float hasIntersected = false;
                float gam = 1.0f;
                if (intersection.mat->degamma)
                    gam = 2.2f;

                Vector3f vrd = Normalize(sn * 2.0f * Dot(viewerDirection, sn) - viewerDirection); // Viewer reflection direction
                Vector3f mirCol = Vector3f(0.0f, 0.0f, 0.0f);                                     // Resulting mirror color

                // If roughness > 0 apply glossy reflection
                if (intersection.mat->roughness > 0)
                    ComputeTiltedGlossyReflectionRay(vrd, intersection.mat);

                Ray tempRay = Ray(intersection.ip + sn * shadowRayEpsilon, vrd, r.currMat, r.currShape, r.time);
                hasIntersected = CalculateLight(tempRay, mirCol, depth + 1); // Calculate color of the object for the viewer reflection direction ray

                if (hasIntersected && intersection.mat->mType == Material::MatType::MIRROR)
                    outColor = outColor + mirCol * intersection.mat->MRC * gam;
                if (hasIntersected && intersection.mat->mType == Material::MatType::CONDUCTOR) // If it Intersects with an object
                    outColor = outColor + mirCol * intersection.mat->MRC * Fr;
            }
        }

        if (intersection.mat->mType == Material::MatType::DIELECTRIC)
        {
            Vector3f vrd = Normalize(sn * 2.0f * Dot(viewerDirection, sn) - viewerDirection); // Viewer reflection direction
            Vector3f mirCol = Vector3f(0.0f, 0.0f, 0.0f);                                     // Resulting mirror color

            Ray tempRay = Ray(intersection.ip + sn * shadowRayEpsilon, vrd, r.currMat, r.currShape, r.time);
            CalculateLight(tempRay, mirCol, depth + 1); // Calculate color of the object for the viewer reflection direction ray

            if (intersection.mat->mType == Material::MatType::DIELECTRIC)
            {
                outColor = outColor + mirCol * Fr;

                if (flag) // If ray was inside
                {
                    Vector3f param = DielectricPowerAbsorption(r, intersection);
                    outColor *= param;
                }
            }
        }
    }

    return true;
}

// bool LightContributionCalculator::CalculateLight(Ray &cameraRay, Vector3f &outColor, int depth, float columnOverWidth, float rowOverHeight)
// {
//     SurfaceIntersection intersection{};
//     accelerator->Intersect(cameraRay, intersection);

//     if (intersection.IsValid())
//     {
//         // Short curcuit if replacer texture is on the surface
//         if (intersection.DoesSurfaceTextureReplaceAllColor())
//             outColor = intersection.tex1->RetrieveRGBFromUV(intersection.uv.x, intersection.uv.y);
//         else
//             CalculateContribution(cameraRay, intersection, outColor, depth);
//     }
//     else
//         outColor = mBackgroundColor.GetBackgroundColorAt(columnOverWidth, rowOverHeight);
// }

void LightContributionCalculator::CalculateContribution(Ray &cameraRay, SurfaceIntersection &intersectedSurface, Vector3f &outColor, int depth) const
{
    outColor += CalculateAmbientLightContribution(cameraRay, intersectedSurface);

    
}

Vector3f LightContributionCalculator::CalculateAmbientLightContribution(const Ray &ray, const SurfaceIntersection &intersectedSurface) const
{
    if (!IsInternalReflection(ray, intersectedSurface))
        return mAmbientLightColor * intersectedSurface.mat->ARC;

    return Vector3f{};
}

bool IsInternalReflection(const Ray& ray, const SurfaceIntersection& intersectedSurface)
{
    return intersectedSurface.shape && ray.currShape && intersectedSurface.shape == ray.currShape;
}


Vector3f BackgroundColor::GetBackgroundColorAt(int columnOverWidth, int rowOverHeight)
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