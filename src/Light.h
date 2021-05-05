#ifndef _LIGHT_H_
#define _LIGHT_H_

#include "Material.h"
#include "Intersection.h"
#include "Random.h"

namespace actracer {

class Texture;
class ColorChangerTexture;

// Base class for all light types
// Stores the most basic information
// Position and intensity
// Every light type must implement its own light formula for the "ResultingColorContribution"
class Light
{
public:
    enum LightType { POINT, DIRECTIONAL, SPOT, AREA, ENVIRONMENT };
protected:
    Vector3f lightIntenstiy; // Intensity of the light
    Vector3f lightPosition;  // Position of the light
    Light() {}
public:
    LightType lType;

    // Getters
    inline Vector3f GetLightPosition() const { return lightPosition; }
    inline Vector3f GetLightIntensity() const { return lightIntenstiy; }
    virtual Vector3f GetLightDirection(Vector3f normal = Vector3f{}) {}

    // --

    virtual bool IsPointInside(const Vector3f& target) { return false; }

    Light(const Vector3f &position, const Vector3f &intensity); // Construct with essentials

    virtual Vector3f ResultingColorContribution(SurfaceIntersection target, const Vector3f &viewerDirection, const ColorChangerTexture *tex = nullptr, float gamma = 2.2f) = 0; // Abstract function to calculate the contribution of the light to the given surface
};

class EnvironmentLight : public Light
{
protected:
    Vector3f resultingParams;
    Random<double> randPoint;

public:
    Texture* tex;
    EnvironmentLight(Texture* _tex);
    virtual Vector3f ResultingColorContribution(SurfaceIntersection target, const Vector3f &viewerDirection, const ColorChangerTexture *tex = nullptr, float gamma = 2.2f) override;
    virtual Vector3f GetLightDirection(Vector3f normal = Vector3f{}) override;
};


// Base class for lights with a ceratin direction ( stores additional direction data )
// e.g Directional and Spot lights
// Will be added with full implementation later
class AdjustableLight : public Light
{
protected:
    Vector3f lightDirection; // Direction of the light
public:
    inline Vector3f GetLightDirection() const { return lightDirection; }

    AdjustableLight(const Vector3f &position, const Vector3f &intensity, const Vector3f &direction); // Construct with additional direction parameter

    Vector3f GetLightDirection(Vector3f normal = Vector3f{}) override { return lightDirection; }
};

class DirectionalLight : public AdjustableLight
{
public:
    DirectionalLight(const Vector3f &position, const Vector3f &intensity, const Vector3f &direction)
        : AdjustableLight(position, intensity, direction) { lType = LightType::DIRECTIONAL; }

    Vector3f ResultingColorContribution(SurfaceIntersection target, const Vector3f &viewerDirection, const ColorChangerTexture *tex = nullptr, float gamma = 2.2f) override;
};

// Stores additional exponent variable which will be used in the light contribution formula for the spot light
class SpotLight : public AdjustableLight
{
protected:
    float spotLightExponent; // Spotlight exponent ( is used to control spotlight's effect radius )
    float coverageAngle;
    float falloffAngle;
public:
    SpotLight(const Vector3f &position, const Vector3f &intensity, const Vector3f &direction, float ca, float fa, float exponent = 1);

    Vector3f ResultingColorContribution(SurfaceIntersection target, const Vector3f &viewerDirection, const ColorChangerTexture *tex = nullptr, float gamma = 2.2f) override;
};

class AreaLight : public AdjustableLight
{
protected:
    float size;
    Random<double> randPoint{};

    Vector3f sampledPoint;
public:
    AreaLight(const Vector3f& p, const Vector3f& r, const Vector3f& d, float s);

    Vector3f GetRandomPointInSquare();

    Vector3f GetLightDirection(Vector3f normal = Vector3f{}) override;

    virtual bool IsPointInside(const Vector3f &target) override;

    Vector3f ResultingColorContribution(SurfaceIntersection target, const Vector3f &viewerDirection, const ColorChangerTexture *tex = nullptr, float gamma = 2.2f) override;
};


class PointLight : public Light
{
public:
    PointLight(const Vector3f &position, const Vector3f &intensity);

    Vector3f ResultingColorContribution(SurfaceIntersection target, const Vector3f &viewerDirection, const ColorChangerTexture *tex = nullptr, float gamma = 2.2f) override;
};

}

#endif
