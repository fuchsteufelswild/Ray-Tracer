#include "Scene.h"
#include "Sphere.h"
#include "Texture.h"

#include "NormalChangerTexture.h"

namespace actracer {

Sphere::Sphere(int _id, Material *_mat, float _radius, const Vector3f &centerPoint, Transform *objToWorld, ShadingMode shMode)
    : Shape(_id, _mat, objToWorld, shMode), radius(_radius), center(centerPoint)
{
    shType = ShapeType::SPHERE;

    if (objTransform)
        objTransform->UpdateTransform();
        
    this->orgBbox = BoundingVolume3f(centerPoint + Vector3f(-radius, -radius, -radius), centerPoint + Vector3f(radius, radius, radius));

    if(this->objTransform)
        this->bbox = (*objTransform)(this->orgBbox);
    else
        this->bbox = this->orgBbox;

    // this->bbox = this->orgBbox;
}

Vector3f Sphere::GetChangedNormal(const SurfaceIntersection &intersection) const
{
    if (mNormalChangerTexture != nullptr)
        return mNormalChangerTexture->GetChangedNormal(intersection, this);

    return intersection.n;
}

void Sphere::Intersect(Ray &rr, SurfaceIntersection &rt)
{
    // x - c.x  y - c.y z - c.z = r
    // s + tv
    // (s.x + tv.x - c.x)
    // s.x ^ 2 + t ^ 2 * v.x ^ 2 + 2 * s.x tv.x + c.x ^ 2 - 2 * c.x * s.x - c.x * t * v.x
    // t ^ 2 * s.x ^ 2 * v.x ^ 2
    // 2 * s.x * t * v.x = c.x * t * v.x
    Ray r = rr;

    if (this->objTransform)
        ApplyTransformationToRay(r);

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
    float t = -1 * dirOtC - discriminant;

    if (t < 0 || ((-dirOtC + discriminant) < t && -dirOtC + discriminant > 0))
        t = -1 * dirOtC + discriminant;

    float ot = t / dd;

    if (ot > 0) // If there is an intersection
    {
        Transform trr = *this->objTransform;

        Vector3f point = r(ot);                  // intersection point
        Vector3f no = (point - center) / radius; // surface normal

        Vector2f uv{};

        float phi, theta;

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

        if (this->motionBlur.x != 0 || this->motionBlur.y != 0 || this->motionBlur.z != 0)
        {
            Vector3f motBlurToRay = this->motionBlur * r.time;
            Transformation tr = Translation(-1, (glm::vec3)motBlurToRay);
            trr = (*this->objTransform)(tr);
            trr.UpdateTransform();
        }
        
        Vector3f lip = point;
        point = (trr)(Vector4f(point, 1.0f), true);
        no = (trr)(Vector4f(no, 0.0f), true, true);
        no = Normalize(no);

        ot = rr(point);

        rt = SurfaceIntersection(Vector3f{theta, phi, 0.0f}, point, no, uv, originToCenter, ot, mat, (Shape*)this, (Shape*)this, mColorChangerTexture, mNormalChangerTexture);
    }
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