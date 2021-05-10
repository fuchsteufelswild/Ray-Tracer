#ifndef _LIGHT_H_
#define _LIGHT_H_

#include "Material.h"
#include "Intersection.h"
#include "Random.h"

namespace actracer {

class Texture;
class ColorChangerTexture;

// Base class for all light types
class Light
{
public:
    enum LightType { POINT, DIRECTIONAL, SPOT, AREA, ENVIRONMENT };
public:
    Light(const Vector3f &position, const Vector3f &intensity);
public:
    Vector3f GetLightPosition() const { return lightPosition; }
    Vector3f GetLightIntensity() const { return lightIntenstiy; }
    virtual Vector3f GetLightDirection(Vector3f normal = Vector3f{}) const {}
public:
    /*
     * Calculates pointToLight direction vector and distance value,
     * it may be specialized by derived classes, (e.g point light distance is different than directional light(infinite) )
     */ 
    virtual void AssignLightFormulaVariables(const SurfaceIntersection &intersection, Vector3f &pointToLight, float &distanceToLight) const;
    Vector3f ComputeResultingColorContribution(const SurfaceIntersection &intersection, const Vector3f &viewerDirection, const Vector3f &pointToLight, const float distanceToLight, float gamma = 2.2f) const;
private:
    /*
     * Calls material's BRDF function with respective parameters,
     * for more information check brdf.cpp source file
     */ 
    Vector3f ComputeBRDFForLight(const SurfaceIntersection &intersection, const Vector3f &pointToViewer, const Vector3f &pointToLight) const;
    virtual Vector3f GetLightIntensityAtPoint(const Vector3f &pointToLight, const float distanceToLight) const = 0;
protected:
    Vector3f lightIntenstiy;
    Vector3f lightPosition;
    Light() {}
};

// Base class for lights with a ceratin direction ( stores additional direction data )
// e.g Directional and Spot lights
class AdjustableLight : public Light
{
public:
    AdjustableLight(const Vector3f &position, const Vector3f &intensity, const Vector3f &direction);

    virtual Vector3f GetLightDirection(Vector3f normal = Vector3f{}) const override { return lightDirection; }
protected:
    Vector3f lightDirection;
};

class DirectionalLight : public AdjustableLight
{
public:
    DirectionalLight(const Vector3f &position, const Vector3f &intensity, const Vector3f &direction);

    virtual void AssignLightFormulaVariables(const SurfaceIntersection &intersection, Vector3f &pointToLight, float &distanceToLight) const override;
    virtual Vector3f GetLightIntensityAtPoint(const Vector3f &pointToLight, const float distanceToLight) const override;
};

class SpotLight : public AdjustableLight
{
public:
    SpotLight(const Vector3f &position, const Vector3f &intensity, const Vector3f &direction, float ca, float fa, float exponent = 1);

    virtual Vector3f GetLightIntensityAtPoint(const Vector3f &pointToLight, const float distanceToLight) const override;
protected:
    float spotLightExponent; // Used to control spotlight's effect radius
    float coverageAngle;
    float falloffAngle;
};

class AreaLight : public AdjustableLight
{
public:
    AreaLight(const Vector3f& p, const Vector3f& r, const Vector3f& d, float s);

    virtual void AssignLightFormulaVariables(const SurfaceIntersection &intersection, Vector3f &pointToLight, float &distanceToLight) const override;
    virtual Vector3f GetLightIntensityAtPoint(const Vector3f &pointToLight, const float distanceToLight) const override;

    Vector3f GetRandomPointInSquare() const;
    virtual Vector3f GetLightDirection(Vector3f normal = Vector3f{}) const override;

protected:
    float size;
    mutable Random<double> randPoint{};
};

class PointLight : public Light
{
public:
    PointLight(const Vector3f &position, const Vector3f &intensity);

    virtual Vector3f GetLightIntensityAtPoint(const Vector3f &pointToLight, const float distanceToLight) const override;
};

}

#endif
