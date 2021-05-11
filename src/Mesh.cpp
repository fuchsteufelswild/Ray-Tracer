#include "Scene.h"
#include "Mesh.h"
#include "Primitive.h"

#include <set>
#include <unordered_map>

namespace actracer {

    Mesh::Mesh(int _id, Material *_mat, const std::vector<std::pair<int, Material *>> &faces, const std::vector<Vector3f *> *pIndices, const std::vector<Vector2f *> *pUVs, std::vector<Primitive *> &primitives, Transform *objToWorld, ShadingMode shMode)
        : Shape(_id, _mat, objToWorld, shMode)
    {
        if (objTransform)
            objTransform->UpdateTransform();

        triangles = new std::vector<Triangle *>();

        meshVertices = new std::vector<Vertex *>();

        float maxX = -1e9, minX = 1e9;
        float maxY = -1e9, minY = 1e9;
        float maxZ = -1e9, minZ = 1e9;

        std::unordered_map<Vector3f *, Vertex *> vertexHash;

        // Fill up vertices to reuse
        for (int i = 0; i < faces.size(); ++i)
        {
            if (vertexHash.find(((*pIndices)[i * 3 + 0])) == vertexHash.end())
            {
                meshVertices->push_back(new Vertex{});
                meshVertices->back()->p = *((*pIndices)[i * 3 + 0]);
                meshVertices->back()->uv = *((*pUVs)[i * 3 + 0]);

                vertexHash.insert(std::make_pair(((*pIndices)[i * 3 + 0]), meshVertices->back()));
            }
            if (vertexHash.find(((*pIndices)[i * 3 + 1])) == vertexHash.end())
            {
                meshVertices->push_back(new Vertex{});
                meshVertices->back()->p = *((*pIndices)[i * 3 + 1]);
                meshVertices->back()->uv = *((*pUVs)[i * 3 + 1]);

                vertexHash.insert(std::make_pair(((*pIndices)[i * 3 + 1]), meshVertices->back()));
            }
            if (vertexHash.find(((*pIndices)[i * 3 + 2])) == vertexHash.end())
            {
                meshVertices->push_back(new Vertex{});
                meshVertices->back()->p = *((*pIndices)[i * 3 + 2]);
                meshVertices->back()->uv = *((*pUVs)[i * 3 + 2]);

                vertexHash.insert(std::make_pair(((*pIndices)[i * 3 + 2]), meshVertices->back()));
            }
        }

        for (int i = 0; i < faces.size(); ++i) // Populate the vector with the triangles that makes up this mesh
        {
            triangles->push_back(new Triangle(_id, faces[i].second, vertexHash[((*pIndices)[i * 3 + 0])], vertexHash[((*pIndices)[i * 3 + 1])], vertexHash[((*pIndices)[i * 3 + 2])], objTransform, this, shMode));
            primitives.push_back(new Primitive(triangles->back(), mat));

            Vector3f &p0 = *((*pIndices)[i * 3 + 0]); //
            Vector3f &p1 = *((*pIndices)[i * 3 + 1]); // Vertex points
            Vector3f &p2 = *((*pIndices)[i * 3 + 2]); //

            float xOfThree = p0.x > p1.x ? (p0.x > p2.x ? p0.x : p2.x) : (p1.x > p2.x ? p1.x : p2.x); //
            float yOfThree = p0.y > p1.y ? (p0.y > p2.y ? p0.y : p2.y) : (p1.y > p2.y ? p1.y : p2.y); // Maximum extents
            float zOfThree = p0.z > p1.z ? (p0.z > p2.z ? p0.z : p2.z) : (p1.z > p2.z ? p1.z : p2.z); //

            SetMax(maxX, xOfThree); //
            SetMax(maxY, yOfThree); // Update max vertex extents
            SetMax(maxZ, zOfThree); //

            xOfThree = p0.x < p1.x ? (p0.x < p2.x ? p0.x : p2.x) : (p1.x < p2.x ? p1.x : p2.x); //
            yOfThree = p0.y < p1.y ? (p0.y < p2.y ? p0.y : p2.y) : (p1.y < p2.y ? p1.y : p2.y); // Minimum extents
            zOfThree = p0.z < p1.z ? (p0.z < p2.z ? p0.z : p2.z) : (p1.z < p2.z ? p1.z : p2.z); //

            SetMin(minX, xOfThree); //
            SetMin(minY, yOfThree); // Update min vertex extents
            SetMin(minZ, zOfThree); //
        }

        orgBbox = BoundingVolume3f(Vector3f(maxX, maxY, maxZ), Vector3f(minX, minY, minZ));

        if (objTransform)
            bbox = (*objTransform)(orgBbox);
        else
            bbox = orgBbox;

        if (shadingMode == Shape::ShadingMode::SMOOTH)
        {
            for (Triangle *sh : *(triangles))
                sh->PerformVertexModification();

            for (Triangle *sh : *(triangles))
                sh->RegulateVertices();
        }
}

void Mesh::FindClosestObject(Ray &r, SurfaceIntersection &rt, float intersectionTestEpsilon)
{
    float minT = std::numeric_limits<float>::max(); // Initialize min t with inf

    for (Shape *shape : *triangles)
    {
        SurfaceIntersection io{};
        shape->Intersect(r, io, intersectionTestEpsilon); // Find the surface of intersection

        // If there is no intersection
        if (!io.IsValid())
            continue;

        // Update the closest surface information and the minimum distance variable
        if (io.t < minT && io.t > -intersectionTestEpsilon)
        {
            minT = io.t; // New minimum distance
            rt = io;     // New closest surface
        }
    }
}

void Mesh::Intersect(Ray &rr, SurfaceIntersection &rt, float intersectionTestEpsilon)
{
    Ray r = rr;
    FindClosestObject(rr, rt, intersectionTestEpsilon);
}

Shape* Mesh::Clone(bool resetTransform) const
{
    Mesh* cloned = new Mesh{};
    cloned->id = this->id;
    cloned->mat = this->mat;
    cloned->orgBbox = this->orgBbox;

    cloned->triangles = new std::vector<Triangle*>();

    cloned->mColorChangerTexture = mColorChangerTexture;
    cloned->mNormalChangerTexture = mNormalChangerTexture;
    
    if(!resetTransform)
    {
        Transform* newTransformation = new Transform(*(this->objTransform));
        cloned->objTransform = newTransformation;
        cloned->bbox = this->bbox;
    }
    else
    {
        glm::mat4 dummy = glm::mat4(1);
        cloned->objTransform = new Transform{dummy};
        cloned->bbox = this->orgBbox;
    }
    
    for(Triangle* tr : (*triangles))
    {
        cloned->triangles->push_back(tr->Clone(resetTransform));
        cloned->triangles->back()->SetObjectTransform(cloned->objTransform);
        cloned->triangles->back()->SetOwnerMesh(cloned);

        cloned->triangles->back()->SetColorChangerTexture(mColorChangerTexture);
        cloned->triangles->back()->SetNormalChangerTexture(mNormalChangerTexture);
    }
    
    return cloned;
}

void Mesh::SetMaterial(Material* newMat)
{
    Shape::SetMaterial(newMat);

    for(Shape* t : (*triangles))
        t->SetMaterial(newMat);
}

void Mesh::SetTransformation(Transform* newTransform, bool owned)
{
    Shape::SetTransformation(newTransform);

    for(Shape* tr : (*triangles))
    {
        tr->SetTransformation(newTransform, true);
    }
}

void Mesh::SetMotionBlur(const Vector3f& mb, std::vector<Primitive *> &primitives)
{
    Shape::SetMotionBlur(mb, primitives);

    for (Shape *tr : (*triangles))
    {
        tr->SetMotionBlur(mb, primitives);
        tr->SetHasActiveMotion(this->activeMotion);
        primitives.push_back(new Primitive(tr, this->mat));
    }
}

void Mesh::SetTextures(const ColorChangerTexture *colorChangerTexture, const NormalChangerTexture *normalChangerTexture)
{
    Shape::SetTextures(colorChangerTexture, normalChangerTexture);

    for(Shape* tr : (*triangles))
    {
        tr->SetTextures(colorChangerTexture, normalChangerTexture);
    }
}


}