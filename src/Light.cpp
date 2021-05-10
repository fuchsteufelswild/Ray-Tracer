#include "Light.h"
#include "Material.h"
#include "Shape.h"
#include "Texture.h"

#include "Intersection.h"
#include "brdf.h"


namespace actracer {

static float deg2radians = 3.1415926f / 180.0f;
static float rad2deg = 180.0f / 3.1415926f;

Light::Light(const Vector3f &position, const Vector3f &intensity)
    : lightPosition(position), lightIntenstiy(intensity) {}

PointLight::PointLight(const Vector3f &position, const Vector3f &intensity)
    : Light(position, intensity) { }

AdjustableLight::AdjustableLight(const Vector3f &position, const Vector3f &intensity, const Vector3f &direction)
    : Light(position, intensity), lightDirection(direction) {}

SpotLight::SpotLight(const Vector3f &position, const Vector3f &intensity, const Vector3f &direction, float ca, float fa, float exponent)
    : AdjustableLight(position, intensity, direction), spotLightExponent(exponent), coverageAngle(ca), falloffAngle(fa) { }

AreaLight::AreaLight(const Vector3f &p, const Vector3f &r, const Vector3f &d, float s)
    : AdjustableLight(p, r, d), size(s) { }

DirectionalLight::DirectionalLight(const Vector3f &position, const Vector3f &intensity, const Vector3f &direction)
    : AdjustableLight(position, intensity, direction) { }

/*
 * Calculates light contribution using brdf and regulates the color using intensity
 * gammaMultiplier is used for applying degamma in case of Tonemapping
 */ 
Vector3f Light::ComputeResultingColorContribution(const SurfaceIntersection &intersection, const Vector3f &pointToViewer, const Vector3f& pointToLight, const float distanceToLight, float gamma) const
{
    float gammaMutliplier = 1;
    if(intersection.mat->GetDegamma()) 
        gammaMutliplier = static_cast<float>(std::pow(1, gamma));

    return ComputeBRDFForLight(intersection, pointToViewer, pointToLight) * GetLightIntensityAtPoint(pointToLight, distanceToLight) * gammaMutliplier;
}

Vector3f Light::ComputeBRDFForLight(const SurfaceIntersection &intersection, const Vector3f &pointToViewer, const Vector3f& pointToLight) const
{
    const Vector3f& wi = pointToLight;
    const Vector3f& wo = pointToViewer; 
    Vector3f kd = intersection.GetDiffuseReflectionCoefficient();
    Vector3f ks = intersection.GetSpecularReflectionCoefficient();
    Vector3f n = intersection.GetSurfaceNormal();
    float refractionIndex = intersection.GetRefractionIndex();

    return intersection.mat->GetBRDF()->f(wi, wo, n, kd, ks, refractionIndex);
}

Vector3f DirectionalLight::GetLightIntensityAtPoint(const Vector3f &pointToLight, const float distanceToLight) const
{
    return lightIntenstiy;
}

Vector3f SpotLight::GetLightIntensityAtPoint(const Vector3f &pointToLight, const float distanceToLight) const
{
    float cosAngle = std::cos(coverageAngle * deg2radians / 2);
    float theta = std::acos(-Dot(pointToLight, lightDirection));
    float falloff = (std::cos(theta) - cosAngle) / (std::cos(falloffAngle * deg2radians / 2) - cosAngle);

    Vector3f fadedLightIntenstiy = lightIntenstiy / (distanceToLight * distanceToLight);
    if (theta * rad2deg > falloffAngle / 2)
        fadedLightIntenstiy *= falloff;

    return fadedLightIntenstiy;
}

Vector3f AreaLight::GetLightIntensityAtPoint(const Vector3f &pointToLight, const float distanceToLight) const
{
    return lightIntenstiy * std::abs(Dot(-pointToLight, lightDirection)) * size * size / (distanceToLight * distanceToLight);
}

Vector3f PointLight::GetLightIntensityAtPoint(const Vector3f &pointToLight, const float distanceToLight) const
{
    return lightIntenstiy / (distanceToLight * distanceToLight);
}

void Light::AssignLightFormulaVariables(const SurfaceIntersection &intersection, Vector3f &pointToLight, float &distanceToLight) const
{
    pointToLight = GetLightPosition() - intersection.ip;
    distanceToLight = SqLength(pointToLight);
    pointToLight = Normalize(pointToLight);
}

void DirectionalLight::AssignLightFormulaVariables(const SurfaceIntersection &intersection, Vector3f &pointToLight, float &distanceToLight) const
{
    distanceToLight = 1e9;
    pointToLight = -GetLightDirection();
    pointToLight = Normalize(pointToLight);
}

void AreaLight::AssignLightFormulaVariables(const SurfaceIntersection &intersection, Vector3f &pointToLight, float &distanceToLight) const
{
    Vector3f tempPos = GetLightDirection();

    pointToLight = tempPos - intersection.ip;
    distanceToLight = SqLength(pointToLight);
    pointToLight = Normalize(pointToLight);
}

Vector3f AreaLight::GetLightDirection(Vector3f) const
{
    return GetRandomPointInSquare();
}

Vector3f AreaLight::GetRandomPointInSquare() const
{
    double param1 = randPoint(0.0f, 1.0f);
    double param2 = randPoint(0.0f, 1.0f);

    param1 -= 0.5;
    param2 -= 0.5;

    Vector3f toRight = (float)param1 * size * Vector3f(1.0f, 0.0f, 0.0f);
    Vector3f toUp = (float)param2 * size * Vector3f(0.0f, 0.0f, 1.0f);

    return lightPosition + toUp + toRight;
}

}
