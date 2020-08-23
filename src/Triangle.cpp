#include "Scene.h"
#include "Triangle.h"
#include "Texture.h"

namespace actracer {

Triangle::Triangle(int _id, Material *_mat, const Vector3f &p0, const Vector3f &p1, const Vector3f &p2,
                   const Vector2f& uv0, const Vector2f& uv1, const Vector2f& uv2,
                   Transform *objToWorld, Shape *_m, ShadingMode shMode)
    : Shape(_id, _mat, objToWorld, shMode)
{
    ownerMesh = _m;
    shType = ShapeType::TRIANGLE;

    if(!_m && objTransform)
        objTransform->UpdateTransform();


    if(!_m)
        ownerMesh = (Shape*)this;

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

    this->orgBbox = BoundingVolume3f(MaxElements(p0, MaxElements(p1, p2)),
                                     MinElements(p0, MinElements(p1, p2)));

    if(this->objTransform)
        this->bbox = (*objTransform)(this->orgBbox);
    else
        this->bbox = this->orgBbox;
}

Triangle::Triangle(int _id, Material *_mat, Vertex *p0, Vertex *p1, Vertex* p2, Transform *objToWorld, Shape *_m, ShadingMode shMode)
    : Shape(_id, _mat, objToWorld, shMode)
{
    ownerMesh = _m;
    shType = ShapeType::TRIANGLE;

    if (!_m && objTransform)
        objTransform->UpdateTransform();

    if (!_m)
        ownerMesh = (Shape *)this;

    v0 = p0;
    v1 = p1;
    v2 = p2;

    p0p1 = p0->p - p1->p;
    p0p2 = p0->p - p2->p;

    surfaceArea = Length(Cross(-p0p1, -p0p2));
    normal = Normalize(Cross(-p0p1, -p0p2));

    Vector2f duv02 = p0->uv - p2->uv;
    Vector2f duv12 = p1->uv - p2->uv;

    Vector3f dp02 = p0->p - p2->p;
    Vector3f dp12 = p1->p - p2->p;

    float det = duv02[0] * duv12[1] - duv02[1] * duv12[0];

    this->orgBbox = BoundingVolume3f(MaxElements(p0->p, MaxElements(p1->p, p2->p)),
                                     MinElements(p0->p, MinElements(p1->p, p2->p)));

    if (this->objTransform)
        this->bbox = (*objTransform)(this->orgBbox);
    else
        this->bbox = this->orgBbox;

}

void Triangle::PerformVertexModification()
{
    this->v0->n += this->normal;
    this->v0->refCount++;
    this->v1->n += this->normal;
    this->v1->refCount++;
    this->v2->n += this->normal;
    this->v2->refCount++;
}

void Triangle::RegulateVertices()
{
    if(this->v0->refCount != 0)
    {
        //this->v0->n /= this->v0->refCount;
        this->v0->n = Normalize(this->v0->n);
        this->v0->refCount = 0;
    }
    if (this->v1->refCount != 0)
    {
        //this->v1->n /= this->v1->refCount;
        this->v1->n = Normalize(this->v1->n);
        this->v1->refCount = 0;
    }
    if (this->v2->refCount != 0)
    {
        // this->v2->n /= this->v2->refCount;
        this->v2->n = Normalize(this->v2->n);
        this->v2->refCount = 0;
    }
}

bool SolveSystem(float A[2][2], float B[2], float* x0, float* x1)
{
    float det = A[0][0] * A[1][1] - A[0][1] * A[1][0];
    if (std::abs(det) < 1e-10f)
        return false;
    *x0 = (A[1][1] * B[0] - A[0][1] * B[1]) / det;
    *x1 = (A[0][0] * B[1] - A[1][0] * B[0]) / det;
    if (std::isnan(*x0) || std::isnan(*x1))
        return false;
    return true;
}

Vector3f Triangle::GetBumpedNormal(Texture* tex, SurfaceIntersection& intersection, Ray& ray)
{
    float u = intersection.uv.x;
    float v = intersection.uv.y;

    Vector3f p0p1t = (*this->objTransform)(-p0p1, true);
    Vector3f p0p2t = (*this->objTransform)(-p0p2, true);

    glm::vec3 e1 = -p0p1;
    glm::vec3 e2 = -p0p2;

    e1 = p0p1t;
    e2 = p0p2t;

    glm::mat3x2 E(glm::vec2(e1.x, e2.x), glm::vec2(e1.y, e2.y), glm::vec2(e1.z, e2.z));

    glm::vec2 a1 = glm::vec2(this->v0->uv.u - this->v1->uv.u, this->v0->uv.v - this->v1->uv.v);
    glm::vec2 a2 = glm::vec2(this->v0->uv.u - this->v2->uv.u, this->v0->uv.v - this->v2->uv.v);

    a1 *= -1;
    a2 *= -1;
    
    glm::mat2x2 A(glm::vec2(a1.x, a2.x), glm::vec2(a1.y, a2.y));

    A = glm::inverse(A);

    glm::mat3x2 soughtMat = A * E;

    glm::vec2 rr1 = soughtMat[0];
    glm::vec2 rr2 = soughtMat[1];
    glm::vec2 rr3 = soughtMat[2];

    glm::vec3 T = Normalize(Vector3f(rr1.x, rr2.x, rr3.x));
    glm::vec3 B = Normalize(Vector3f(rr1.y, rr2.y, rr3.y));

    glm::vec3 N = glm::cross(T, B);

    T = T - N * glm::dot(T, N);
    B = B - glm::dot(B, N) * N - glm::dot(T, B) * T;

    u -= std::floor(u);
    v -= std::floor(v);

    int i = round(u * (tex->width - 1));
    int j = round(v * (tex->height - 1));

    float epsilon = 0.001f;

    Vector3f col = tex->RetrieveRGBFromUV(u, v)       / 255 * tex->bumpFactor;
    Vector3f hCol = tex->RetrieveRGBFromUV(u + epsilon, v) / 255 * tex->bumpFactor;
    Vector3f vCol = tex->RetrieveRGBFromUV(u, v + epsilon) / 255 * tex->bumpFactor;

    float sc = (col.x + col.y + col.z) / 3;
    float hc = (hCol.x + hCol.y + hCol.z) / 3;
    float vc = (vCol.x + vCol.y + vCol.z) / 3;

    glm::vec3 dpdu = T + (hc - sc) * N;
    glm::vec3 dpdv = B + (vc - sc) * N;

    glm::vec3 resss = glm::cross(dpdu, dpdv);

    Vector3f nnn(resss.x, resss.y, resss.z);

    return nnn;
}

Vector3f Triangle::RegulateNormal(Vector3f textureNormal, SurfaceIntersection &intersection)
{

    glm::vec3 e1 = p0p1;
    glm::vec3 e2 = p0p2;

    glm::mat3x2 E(glm::vec2(e1.x, e2.x), glm::vec2(e1.y, e2.y), glm::vec2(e1.z, e2.z));

    glm::vec2 a1 = glm::vec2(this->v0->uv.u - this->v1->uv.u, this->v0->uv.v - this->v1->uv.v);
    glm::vec2 a2 = glm::vec2(this->v0->uv.u - this->v2->uv.u, this->v0->uv.v - this->v2->uv.v);

    glm::mat2x2 A(glm::vec2(a1.x, a2.x), glm::vec2(a1.y, a2.y));

    A = glm::inverse(A);

    glm::mat3x2 soughtMat = A * E;

    glm::vec2 rr1 = soughtMat[0];
    glm::vec2 rr2 = soughtMat[1];
    glm::vec2 rr3 = soughtMat[2];

    glm::vec3 T = Normalize(Vector3f(rr1.x, rr2.x, rr3.x));
    glm::vec3 B = Normalize(Vector3f(rr1.y, rr2.y, rr3.y));

    glm::mat3x3 lastRegul(T, B, normal);

    Vector3f newNormal = lastRegul * textureNormal;

    return Normalize(newNormal);
}

void Triangle::Intersect(Ray &rr, SurfaceIntersection &rt)
{
    Ray* r; // Ray to use in intersection test
    if (false && Scene::debugCurrent >= Scene::debugBegin && Scene::debugCurrent <= Scene::debugEnd && this->ownerMesh != this)
    {
        std::cout << "Before Transformation Ray Stats: --- \n"
                    << r->o
                    << r->d;
        std::cout << "Operating with transformation matrices: --- \n"
                    << "plain: " << this->objTransform->transformationMatrix
                    << "inv: " << this->objTransform->invTransformationMatrix;
    }

    Ray transformedRay;

    if(this->ownerMesh == this || !this->activeMotion) // Not owned by a mesh
    {
        if(this->activeMotion)
        {
            Vector3f motBlurToRay = this->motionBlur * rr.time;
            Transformation tr = Translation(-1, (glm::vec3)motBlurToRay);

            Transform lastTransform = (*this->objTransform)(tr);

            transformedRay = (lastTransform)(rr, false);

            r = &transformedRay;
        }
        else
        {
            transformedRay = (*this->objTransform)(rr, false);
            r = &transformedRay;
        }

    }
    else if (rr.transformedModes.find(this->ownerMesh) != rr.transformedModes.end()) // Sibling has transformed the ray before
    {
        r = (rr.transformedModes.at(this->ownerMesh));
    }
    else // First triangle in the mesh to transform the ray
    {
        Vector3f motBlurToRay = this->motionBlur * rr.time;
        Transformation tr = Translation(-1, (glm::vec3)motBlurToRay);

        Transform* trr = new Transform((*this->objTransform)(tr));

        Ray* resultingRay = new Ray((*trr)(rr, false));
        resultingRay->time = rr.time;
        resultingRay->currMat = rr.currMat;
        resultingRay->currShape = rr.currShape;

        rr.InsertRay(this->ownerMesh, resultingRay);
        rr.InsertTransform(this->ownerMesh, trr);

        r = (rr.transformedModes.at(this->ownerMesh));
    }

    if (false && Scene::debugCurrent >= Scene::debugBegin && Scene::debugCurrent <= Scene::debugEnd && this->ownerMesh != this)
    {
        std::cout << "After Transformation Ray Stats: --- \n"
                  << r->o
                  << r->d;
    }

    Vector3f r1 = {p0p1.x, p0p2.x, r->d.x};
    Vector3f r2 = {p0p1.y, p0p2.y, r->d.y};
    Vector3f r3 = {p0p1.z, p0p2.z, r->d.z};

    Vector3f p0ro = {v0->p.x - r->o.x, v0->p.y - r->o.y, v0->p.z - r->o.z}; // Precalculate commonly used vector

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

    float err = 1 + Scene::pScene->intTestEps; // Calculate 1 + error rate once

    // Check bounds
    if(v1 < -Scene::pScene->intTestEps || v1 > err)
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
    if (v2 < -Scene::pScene->intTestEps || v2 > err || v1 + v2 > err)
        return; // Default value no intersection

    r1.y = p0p2.x;
    r2.y = p0p2.y;
    r3.y = p0p2.z;

    r1.z = p0ro.x;
    r2.z = p0ro.y;
    r3.z = p0ro.z;

    float det3 = r1.x * (r2.y * r3.z - r2.z * r3.y) + r2.x * (r1.z * r3.y - r1.y * r3.z) + r3.x * (r1.y * r2.z - r1.z * r2.y);

    float t = det3 * invDetM; // Derive t

    // Check if t value is in the front
    if (t >= -Scene::pScene->intTestEps)
    {
        // Transforming normal and intersection point mistake
        // not setting 1.0f at the end
        Vector3f po = (*r)(t); // Get intersection point using parameter t
        Vector3f normal = this->normal;

        Vector2f uv;
        Transform *trr = this->objTransform;
        uv = this->v0->uv * (err - v1 - v2) +
             this->v1->uv * v1 +
             this->v2->uv * v2;

        if(this->shadingMode == Shape::ShadingMode::SMOOTH)
        {
            normal = this->v0->n * (err - v1 - v2) +
                     this->v1->n * v1 +
                     this->v2->n * v2;

            normal = Normalize(normal);
        }

        if(this->ownerMesh == this)
        {
            if(this->activeMotion)
            {
                Vector3f motBlurToRay = this->motionBlur * rr.time;
                Transformation tr = Translation(-1, (glm::vec3)motBlurToRay);

                Transform lastTransform = (*this->objTransform)(tr);
                trr = &lastTransform;
            }
            else
                trr = this->objTransform;
        }
        else
        {
            if(this->activeMotion)
            {
                trr = (rr.transformMatrices.at(this->ownerMesh));
            }
            else
            {
                trr = this->objTransform;
            }

        }

        Vector3f lip = po;

        if(trr->transformationMatrix != glm::mat4(1))
        {
            po = (*trr)(Vector4f(po, 1.0f), true);
            normal = (*trr)(Vector4f(normal, 0.0f), true, true); // !!!!!!!! was this->normal
            normal = Normalize(normal); 
        }

        t = rr(po);

        rt = SurfaceIntersection(lip, po, normal, uv, Normalize(rr.o - po), t, this->mat, this, textures[0], textures[1]);
    }

}

Shape *Triangle::Clone(bool resetTransform) const
{
    Triangle* cloned = new Triangle{};
    cloned->shType = ShapeType::TRIANGLE;

    cloned->id = this->id;
    cloned->mat = this->mat;
    cloned->orgBbox = this->orgBbox;

    cloned->ownerMesh = cloned;

    for (Texture *t : this->textures)
        cloned->textures.push_back(t);

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