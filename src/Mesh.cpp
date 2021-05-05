#include "Scene.h"
#include "Mesh.h"
#include "Primitive.h"

#include <set>
#include <unordered_map>

namespace actracer {

Mesh::Mesh(int _id, Material *_mat, const std::vector<std::pair<int, Material *>> &faces, const std::vector<Vector3f *> *pIndices, const std::vector<Vector2f *> *pUVs, Transform *objToWorld, ShadingMode shMode)
    : Shape(_id, _mat, objToWorld, shMode)
{
    shType = ShapeType::MESH;

    if (objTransform)
        objTransform->UpdateTransform();
    if(objTransform->transformationMatrix != glm::mat4(1))
        this->haveTransform = true;

    if (objTransform->transformationMatrix[0][0] != objTransform->transformationMatrix[1][1] ||
        objTransform->transformationMatrix[0][0] != objTransform->transformationMatrix[2][2] ||
        objTransform->transformationMatrix[1][1] != objTransform->transformationMatrix[2][2])
        this->nonUniformScaling = true;

    triangles = new std::vector<Shape*>();

    meshVertices = new std::vector<Vertex*>();

    float maxX = -1e9, minX = 1e9;
    float maxY = -1e9, minY = 1e9;
    float maxZ = -1e9, minZ = 1e9;

    
    std::unordered_map<Vector3f*, Vertex*> vertexHash; 

    for(int i = 0; i < faces.size(); ++i)
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
        Scene::pScene->primitives.push_back(new Primitive(triangles->back(), mat));

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

    
    this->orgBbox = BoundingVolume3f(Vector3f(maxX, maxY, maxZ), Vector3f(minX, minY, minZ));

    if(this->objTransform)
        this->bbox = (*objTransform)(this->orgBbox);
    else
        this->bbox = this->orgBbox;


    if(this->shadingMode == Shape::ShadingMode::SMOOTH)
    {
        for(Shape* sh : *(this->triangles))
            sh->PerformVertexModification();

        for(Shape* sh : *(this->triangles))
            sh->RegulateVertices();
    }
}

void Mesh::Intersect(Ray &rr, SurfaceIntersection &rt)
{
    Ray r = rr;
    Shape::FindClosestObject(rr, *(this->triangles), rt);
}

Shape* Mesh::Clone(bool resetTransform) const
{
    Mesh* cloned = new Mesh{};
    cloned->shType = ShapeType::MESH;
    cloned->id = this->id;
    cloned->mat = this->mat;
    cloned->orgBbox = this->orgBbox;

    cloned->triangles = new std::vector<Shape*>();

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
    
    for(Shape* tr : (*triangles)) // May add to primitives in this section
    {
        cloned->triangles->push_back(tr->Clone(resetTransform));
        cloned->triangles->back()->objTransform = cloned->objTransform;
        cloned->triangles->back()->ownerMesh = cloned;

        cloned->triangles->back()->mColorChangerTexture = mColorChangerTexture;
        cloned->triangles->back()->mNormalChangerTexture = mNormalChangerTexture;
    }
    
    return cloned;
}

void Mesh::SetMaterial(Material* newMat)
{
    this->mat = newMat;

    for(Shape* t : (*triangles))
        t->SetMaterial(newMat);
}

void Mesh::SetTransformation(Transform* newTransform, bool owned)
{
    Shape::SetTransformation(newTransform);

    for(Shape* tr : (*triangles))
    {
        tr->SetTransformation(newTransform, true);
        tr->haveTransform = this->haveTransform;
        tr->nonUniformScaling = this->nonUniformScaling;
    }
}

void Mesh::SetMotionBlur(const Vector3f& mb)
{
    Shape::SetMotionBlur(mb);

    for (Shape *tr : (*triangles))
    {
        tr->SetMotionBlur(mb);
        tr->activeMotion = this->activeMotion;
        Scene::pScene->primitives.push_back(new Primitive(tr, this->mat));
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