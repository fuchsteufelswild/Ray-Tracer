#include "Scene.h"
#include "Sphere.h"
#include "Texture.h"

#include "NormalChangerTexture.h"

namespace actracer {

Sphere::Sphere(int _id, Material *_mat, float _radius, const Vector3f &centerPoint, Transform *objToWorld, ShadingMode shMode)
    : Shape(_id, _mat, objToWorld, shMode), radius(_radius), center(centerPoint)
{
    if (objTransform)
        objTransform->UpdateTransform();
        
    orgBbox = BoundingVolume3f(centerPoint + Vector3f(-radius, -radius, -radius), centerPoint + Vector3f(radius, radius, radius));

    if(objTransform)
        bbox = (*objTransform)(orgBbox);
    else
        bbox = orgBbox;
}

Vector3f Sphere::GetChangedNormal(const SurfaceIntersection &intersection) const
{
    if (mNormalChangerTexture != nullptr)
        return mNormalChangerTexture->GetChangedNormal(intersection, this);

    return intersection.n;
}

void Sphere::Intersect(Ray &rr, SurfaceIntersection &rt)
{
    Ray r = rr;
    TransformaRayIntoObjectSpace(r);

    bool hasIntersected;
    float ot;
    CalculateTValueForIntersection(r, hasIntersected, ot);

    if (hasIntersected) // If there is an intersection
    {
        Vector3f intersectionPoint = r(ot); // intersection point
        Vector3f surfaceNormal = (intersectionPoint - center) / radius; // surface normal

        Vector2f uv{};
        float phi, theta;
        CalculateThetaPhiValuesForPoint(intersectionPoint, uv, theta, phi);

        TransformSurfaceVariablesIntoWorldSpace(r.time, intersectionPoint, surfaceNormal);
        ot = rr(intersectionPoint); // t value for intersection point in world space

        rt = SurfaceIntersection(Vector3f{theta, phi, 0.0f}, intersectionPoint, surfaceNormal, uv, Vector3f{}, ot, mat, this, this, mColorChangerTexture, mNormalChangerTexture);
    }
}

/*
 * Returns the t value which results in intersection point on the surface of sphere
 * when put into Ray equation -> origin + direction * t
 * returns (IsValid, IntersectionT)
 */ 
void Sphere::CalculateTValueForIntersection(const Ray& r, bool& hasIntersected, float& t) const
{
    // x - c.x  y - c.y z - c.z = r
    // s + tv
    // (s.x + tv.x - c.x)
    // s.x ^ 2 + t ^ 2 * v.x ^ 2 + 2 * s.x tv.x + c.x ^ 2 - 2 * c.x * s.x - c.x * t * v.x
    // t ^ 2 * s.x ^ 2 * v.x ^ 2
    // 2 * s.x * t * v.x = c.x * t * v.x
    hasIntersected = false;
    Vector3f originToCenter = r.o - center; // Direction vector pointing from sphere's center to ray's origin

    float dirOtC = Dot(originToCenter, r.d); // Correlation between ray direction and ray from center to origin
    float dd = Dot(r.d, r.d);                // Direction ^ 2

    // Discriminant of the quadratic sphere equation
    float discriminant = dirOtC * dirOtC - dd * (Dot(originToCenter, originToCenter) - radius * radius);

    // sqrt(< 0) -> nan
    if (discriminant < 0)
        return; // Return default no intersection

    // Calculate closer intersection point
    discriminant = std::sqrt(discriminant);
    t = -1 * dirOtC - discriminant;

    if (t < 0 || ((-dirOtC + discriminant) < t && -dirOtC + discriminant > 0))
        t = -1 * dirOtC + discriminant;
    
    t /= dd;
    hasIntersected = t > 0;
}

/*
 * These are values for rotating a base vector to "point" to given point Vector
 * e.g rotate theta angles around x axis, phi angles around y axis
 * Using these angles we also calculate UV's for the point
 */ 
void Sphere::CalculateThetaPhiValuesForPoint(const Vector3f& point, Vector2f& uv, float &theta, float &phi) const
{
    theta = std::acos((point.y - center.y) / radius);
    phi = std::atan2((point.z - center.z), (point.x - center.x));

    uv.x = (-phi + PI) / (2 * PI);
    uv.y = theta / PI;

    Vector3f worldCenteredPosition = point - center;
    worldCenteredPosition = (*this->objTransform)(worldCenteredPosition, true);

    theta = std::acos(worldCenteredPosition.y / radius);
    phi = std::atan2(worldCenteredPosition.z, worldCenteredPosition.x);

    theta = uv.y * PI;
    phi = PI - (2 * PI) * uv.x;
}

void Sphere::TransformSurfaceVariablesIntoWorldSpace(float rayTime, Vector3f &intersectionPoint, Vector3f &surfaceNormal) const
{
    Transform transformObject = *this->objTransform;

    if (IsMotionBlurActive())
    {
        Vector3f timeExtendedMotionBlur = this->motionBlur * rayTime;
        Transformation motionBlurTranslation = Translation(-1, (glm::vec3)timeExtendedMotionBlur);
        transformObject = (*objTransform)(motionBlurTranslation);
        transformObject.UpdateTransform();
    }

    intersectionPoint = (transformObject)(Vector4f(intersectionPoint, 1.0f), true);
    surfaceNormal = (transformObject)(Vector4f(surfaceNormal, 0.0f), true, true);
    surfaceNormal = Normalize(surfaceNormal);
}

Shape *Sphere::Clone(bool resetTransform) const
{
    Sphere* cloned = new Sphere{};

    cloned->id = this->id;
    cloned->mat = this->mat;
    cloned->orgBbox = this->orgBbox;

    cloned->mColorChangerTexture = mColorChangerTexture;
    cloned->mNormalChangerTexture = mNormalChangerTexture;

    if (!resetTransform)
    {
        cloned->bbox = this->bbox;
        Transform *newTransformation = new Transform(*(this->objTransform));
        cloned->objTransform = newTransformation;
    }
    else
    {
        cloned->bbox = this->bbox;
        glm::mat4 dummy = glm::mat4(1);
        cloned->objTransform = new Transform{dummy};
    }

    cloned->radius = this->radius;
    cloned->center = this->center;

    return cloned;
}

}