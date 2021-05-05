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
    inline Vector3f GetLightPosition() const { return lightPosition; }
    inline Vector3f GetLightIntensity() const { return lightIntenstiy; }
    virtual Vector3f GetLightDirection(Vector3f normal = Vector3f{}) const {}
public:
    virtual void AssignLightFormulaVariables(const SurfaceIntersection &intersection, Vector3f &pointToLight, float &distanceToLight) const;
    virtual bool IsPointInside(const Vector3f& target) const { return false; }
    Vector3f ComputeResultingColorContribution(const SurfaceIntersection &intersection, const Vector3f &viewerDirection, const Vector3f &pointToLight, const float distanceToLight, float gamma = 2.2f) const;
private:
    Vector3f ComputeBRDFForLight(const SurfaceIntersection &intersection, const Vector3f &pointToViewer, const Vector3f &pointToLight) const;
    virtual Vector3f GetLightIntensityAtPoint(const Vector3f &pointToLight, const float distanceToLight) const = 0;
protected:
    Vector3f lightIntenstiy;
    Vector3f lightPosition;
    Light() {}
};

class EnvironmentLight : public Light
{
public:
    Texture* tex;
    EnvironmentLight(Texture* _tex);
    virtual Vector3f GetLightDirection(Vector3f normal = Vector3f{}) const override;

protected:
    Vector3f resultingParams;
    Random<double> randPoint;
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
    virtual bool IsPointInside(const Vector3f &target) const override;

protected:
    float size;
    mutable Random<double> randPoint{};

    mutable Vector3f sampledPoint;
};

class PointLight : public Light
{
public:
    PointLight(const Vector3f &position, const Vector3f &intensity);

    virtual Vector3f GetLightIntensityAtPoint(const Vector3f &pointToLight, const float distanceToLight) const override;
};

}

#endif
