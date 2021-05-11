#include "Scene.h"
#include "Triangle.h"
#include "Texture.h"

#include "NormalChangerTexture.h"

namespace actracer {

Triangle::Triangle(int _id, Material *_mat, const Vector3f &p0, const Vector3f &p1, const Vector3f &p2,
                   const Vector2f& uv0, const Vector2f& uv1, const Vector2f& uv2,
                   Transform *objToWorld, Shape *_m, ShadingMode shMode)
    : Shape(_id, _mat, objToWorld, shMode)
{
    ownerMesh = _m;

    if(!_m && objTransform)
        objTransform->UpdateTransform();

    if(!_m)
        ownerMesh = this;

    v0 = new Vertex{};
    v1 = new Vertex{};
    v2 = new Vertex{};

    v0->p = p0;
    v1->p = p1;
    v2->p = p2;

    v0->uv = uv0;
    v1->uv = uv1;
    v2->uv = uv2;

    p0p1 = p0 - p1;
    p0p2 = p0 - p2;

    normal = Normalize(Cross(-p0p1, -p0p2));

    orgBbox = BoundingVolume3f(MaxElements(p0, MaxElements(p1, p2)),
                               MinElements(p0, MinElements(p1, p2)));

    if(objTransform)
        bbox = (*objTransform)(orgBbox);
    else
        bbox = orgBbox;
}

Triangle::Triangle(int _id, Material *_mat, Vertex *p0, Vertex *p1, Vertex* p2, Transform *objToWorld, Shape *_m, ShadingMode shMode)
    : Shape(_id, _mat, objToWorld, shMode)
{
    ownerMesh = _m;

    if (!_m && objTransform)
        objTransform->UpdateTransform();

    if (!_m)
        ownerMesh = (Shape *)this;

    v0 = p0;
    v1 = p1;
    v2 = p2;

    p0p1 = p0->p - p1->p;
    p0p2 = p0->p - p2->p;
    
    normal = Normalize(Cross(-p0p1, -p0p2));

    orgBbox = BoundingVolume3f(MaxElements(p0->p, MaxElements(p1->p, p2->p)),
                               MinElements(p0->p, MinElements(p1->p, p2->p)));

    if (objTransform)
        bbox = (*objTransform)(orgBbox);
    else
        bbox = orgBbox;

}

void Triangle::PerformVertexModification()
{
    v0->n += normal;
    v0->refCount++;
    v1->n += normal;
    v1->refCount++;
    v2->n += normal;
    v2->refCount++;
}

void Triangle::RegulateVertices()
{
    if(this->v0->refCount != 0)
    {
        v0->n = Normalize(v0->n);
        v0->refCount = 0;
    }
    if (this->v1->refCount != 0)
    {
        v1->n = Normalize(v1->n);
        v1->refCount = 0;
    }
    if (this->v2->refCount != 0)
    {
        v2->n = Normalize(v2->n);
        v2->refCount = 0;
    }
}

Vector3f Triangle::GetChangedNormal(const SurfaceIntersection &intersection) const
{
    if(mNormalChangerTexture != nullptr)
        return mNormalChangerTexture->GetChangedNormal(intersection, this);

    return intersection.n;
}

void Triangle::Intersect(Ray &rr, SurfaceIntersection &rt, float intersectionTestEpsilon)
{
    Ray transformedRay = rr; // Ray to use in intersection test
    TransformRayIntoObjectSpace(rr, transformedRay);

    float t, beta, gamma;
    bool hasIntersected;
    CalculateTValueForIntersection(transformedRay, hasIntersected, t, beta, gamma, intersectionTestEpsilon);

    // Check if t value is in the front
    if (hasIntersected && t >= -intersectionTestEpsilon)
    {
        Vector3f intersectionPoint = (transformedRay)(t);
        Vector3f surfaceNormal = this->normal;
        Vector3f localIntersectionPoint = intersectionPoint;

        Vector2f uv;
        CalculateSurfaceValues(1 + intersectionTestEpsilon, beta, gamma, uv, surfaceNormal);
        TransformSurfaceValues(rr, transformedRay, intersectionPoint, surfaceNormal);

        t = rr(intersectionPoint);

        rt = SurfaceIntersection(localIntersectionPoint, intersectionPoint, surfaceNormal, uv, Normalize(rr.o - intersectionPoint), t, mat, this, ownerMesh, mColorChangerTexture, mNormalChangerTexture);
    }
}

void Triangle::TransformRayIntoObjectSpace(Ray &baseRay, Ray &r) const
{
    if(IsOwnedByComposite())
    {
        if(HasRayTransformedBefore(baseRay))
        {
            r = *(baseRay.transformedModes.at(ownerMesh));
        }
        else
        {
            if(activeMotion)
                TransformAndRecordRay(baseRay, r);
            else
                r = (*objTransform)(r, false);
        }
    }
    else
    {
        Shape::TransformRayIntoObjectSpace(r);
    }
}

bool Triangle::HasRayTransformedBefore(Ray &r) const
{
    return r.transformedModes.find(ownerMesh) != r.transformedModes.end();
}

void Triangle::TransformAndRecordRay(Ray& baseRay, Ray &r) const
{
    Vector3f timeExtendedMotionBlur = motionBlur * baseRay.time;
    Transformation motionBlurTranslation = Translation(-1, (glm::vec3)timeExtendedMotionBlur);

    Transform *objectTransformExtendedWithMotionBlurMove = new Transform((*objTransform)(motionBlurTranslation));

    Ray *resultingRay = new Ray((*objectTransformExtendedWithMotionBlurMove)(baseRay, false));
    resultingRay->time = baseRay.time;
    resultingRay->currMat = baseRay.currMat;
    resultingRay->currShape = baseRay.currShape;

    baseRay.InsertRay(ownerMesh, resultingRay);
    baseRay.InsertTransform(ownerMesh, objectTransformExtendedWithMotionBlurMove);

    r = *(baseRay.transformedModes.at(ownerMesh));
}

void Triangle::CalculateTValueForIntersection(const Ray &r, bool &hasIntersected, float &t, float &beta, float &gamma, float intersectionTestEpsilon) const
{
    hasIntersected = false;
    t = 0;

    Vector3f r1 = {p0p1.x, p0p2.x, r.d.x};
    Vector3f r2 = {p0p1.y, p0p2.y, r.d.y};
    Vector3f r3 = {p0p1.z, p0p2.z, r.d.z};

    Vector3f p0ro = {v0->p.x - r.o.x, v0->p.y - r.o.y, v0->p.z - r.o.z}; // Precalculate commonly used vector

    float detM = r1.x * (r2.y * r3.z - r2.z * r3.y) + r2.x * (r1.z * r3.y - r1.y * r3.z) + r3.x * (r1.y * r2.z - r1.z * r2.y); // Main determinant

    // Close to zero ( NaN upon division )
    if (detM == 0.0f)
        return; // Default value no intersection

    // Use cramer's rule to solve for v1, v2 and t
    float invDetM = 1 / detM; // Calculate 1 / detM since it is faster for cpu to do multiplication

    r1.x = p0ro.x;
    r2.x = p0ro.y;
    r3.x = p0ro.z;

    float det1 = r1.x * (r2.y * r3.z - r2.z * r3.y) + r2.x * (r1.z * r3.y - r1.y * r3.z) + r3.x * (r1.y * r2.z - r1.z * r2.y);

    float v1 = det1 * invDetM; // beta val

    float err = 1 + intersectionTestEpsilon; // Calculate 1 + error rate once

    // Check bounds
    if (v1 < -intersectionTestEpsilon || v1 > err)
        return; // Default value no intersection

    r1.x = p0p1.x;
    r2.x = p0p1.y;
    r3.x = p0p1.z;

    r1.y = p0ro.x;
    r2.y = p0ro.y;
    r3.y = p0ro.z;

    float det2 = r1.x * (r2.y * r3.z - r2.z * r3.y) + r2.x * (r1.z * r3.y - r1.y * r3.z) + r3.x * (r1.y * r2.z - r1.z * r2.y);

    float v2 = det2 * invDetM; // gamma val

    // Check bounds
    if (v2 < -intersectionTestEpsilon || v2 > err || v1 + v2 > err)
        return; // Default value no intersection

    r1.y = p0p2.x;
    r2.y = p0p2.y;
    r3.y = p0p2.z;

    r1.z = p0ro.x;
    r2.z = p0ro.y;
    r3.z = p0ro.z;

    float det3 = r1.x * (r2.y * r3.z - r2.z * r3.y) + r2.x * (r1.z * r3.y - r1.y * r3.z) + r3.x * (r1.y * r2.z - r1.z * r2.y);

    hasIntersected = true;
    t = det3 * invDetM;
    beta = v1;
    gamma = v2;
}

void Triangle::CalculateSurfaceValues(const float epsilon, const float beta, const float gamma, Vector2f &uv, Vector3f &surfaceNormal) const
{
    uv = v0->uv * (epsilon - beta - gamma) +
         v1->uv * beta +
         v2->uv * gamma;

    if (shadingMode == Shape::ShadingMode::SMOOTH)
    {
        surfaceNormal = v0->n * (epsilon - beta - gamma) +
                        v1->n * beta +
                        v2->n * gamma;

        surfaceNormal = Normalize(surfaceNormal);
    }
}

void Triangle::TransformSurfaceValues(const Ray &baseRay, const Ray &transformedRay, Vector3f &intersectionPoint, Vector3f &surfaceNormal) const
{
    Transform tempTransform{};
    Transform* extendedTransform;

    if (IsOwnedByComposite())
    {
        if (activeMotion)
            extendedTransform = (baseRay.transformMatrices.at(ownerMesh));
        else
            extendedTransform = objTransform;
    }
    else
    {
        if(activeMotion)
        {
            Vector3f timeExtendedMotionBlur = motionBlur * baseRay.time; // [0, 0, 0] - [motionBlur.x, motionBlur.y, motionBlur.z]
            Transformation motionBlurTranslation = Translation(-1, (glm::vec3)timeExtendedMotionBlur);

            tempTransform = (*objTransform)(motionBlurTranslation); // Update transform omitted
            extendedTransform = &tempTransform;
        }
        else
        {
            extendedTransform = objTransform;
        }
    }

    if (extendedTransform->transformationMatrix != glm::mat4(1))
    {
        intersectionPoint = (*extendedTransform)(Vector4f(intersectionPoint, 1.0f), true);
        surfaceNormal = (*extendedTransform)(Vector4f(surfaceNormal, 0.0f), true, true); // !!!!!!!! was this->normal
        surfaceNormal = Normalize(surfaceNormal);
    }
}

Triangle *Triangle::Clone(bool resetTransform) const
{
    Triangle* cloned = new Triangle{};

    cloned->id = this->id;
    cloned->mat = this->mat;
    cloned->orgBbox = this->orgBbox;

    cloned->ownerMesh = cloned;

    cloned->mColorChangerTexture = mColorChangerTexture;
    cloned->mNormalChangerTexture = mNormalChangerTexture;

    if(!resetTransform)
    {
        cloned->bbox = this->bbox;
        Transform* newTransformation = new Transform(*(this->objTransform));
        cloned->objTransform = newTransformation;
    }
    else
    {
        cloned->bbox = this->orgBbox;
        glm::mat4 dummy = glm::mat4(1);
        cloned->objTransform = new Transform{dummy};
    }

    cloned->v0 = this->v0;
    cloned->v1 = this->v1;
    cloned->v2 = this->v2;

    cloned->p0p1 = this->p0p1;
    cloned->p0p2 = this->p0p2;

    cloned->normal = this->normal;

    return cloned;
}

}