#include "ImageTextureImpl.h"

#include "TextureReader.h"

#include "Triangle.h"
#include "Sphere.h"

namespace actracer
{

static float GetAverageValue(const Vector3f& color);
static void AssignRowsFromMatrix(const glm::mat3x2& matrix, glm::vec2& row1, glm::vec2& row2, glm::vec2& row3);

Vector3f ImageTextureImpl::GetBaseTextureColorForColorChange(const SurfaceIntersection &intersection) const
{
    return RetrieveRGBFromUV(intersection.uv.x, intersection.uv.y) / mNormalizer;
}

ImageTextureImpl::ImageTextureImpl(const std::string& imagePath, float bumpFactor,  int normalizer, ImageType imageType, InterpolationMethodCode interpolationMethodeCode)
    : TextureImpl(bumpFactor), mNormalizer(normalizer)
{
    mTextureReader = TextureReader::CreateTextureReader(imagePath, imageType, interpolationMethodeCode);
}

Vector3f ImageTextureImpl::RetrieveRGBFromUV(float u, float v, float w) const
{
    return mTextureReader->ComputeRGBValueOn(u, v);
}

Vector3f ImageTextureImpl::GetReplacedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Triangle *triangle) const
{
    if(!triangle) return {};

    glm::mat3x2 edgeMatrix = ComputePositionEdgeMatrixInObjectSpace(*triangle);
    glm::mat2x2 uvEdgeMatrix = glm::inverse(-ComputeUVEdgeMatrix(*triangle));

    glm::mat3x2 combinedMatrix = uvEdgeMatrix * edgeMatrix;

    glm::vec2 row1, row2, row3;
    AssignRowsFromMatrix(combinedMatrix, row1, row2, row3);

    glm::vec3 tangent = Normalize(Vector3f(row1.x, row2.x, row3.x));
    glm::vec3 bitangent = Normalize(Vector3f(row1.y, row2.y, row3.y));

    glm::mat3x3 TBNMatrix(tangent, bitangent, triangle->GetNormal());

    Vector3f textureNormal = ComputeNormalValueOn(intersectedSurfaceInformation.uv.x, intersectedSurfaceInformation.uv.y);
    return (TBNMatrix * textureNormal);
}

glm::mat3x2 ImageTextureImpl::ComputePositionEdgeMatrixInObjectSpace(const Triangle &triangle) const
{
    const Vector3f &p0p1 = triangle.GetEdgeVectorFromFirstToSecondVertex();
    const Vector3f &p0p2 = triangle.GetEdgeVectorFromFirstToThirdVertex();

    return glm::mat3x2(glm::vec2(p0p1.x, p0p2.x),
                       glm::vec2(p0p1.y, p0p2.y),
                       glm::vec2(p0p1.z, p0p2.z));
}

glm::mat2x2 ImageTextureImpl::ComputeUVEdgeMatrix(const Triangle &triangle) const
{
    const Vertex *v0 = triangle.GetFirstVertex();
    const Vertex *v1 = triangle.GetSecondVertex();
    const Vertex *v2 = triangle.GetThirdVertex();

    glm::vec2 v0v1UVEdgeVector = -glm::vec2(v0->uv.u - v1->uv.u, v0->uv.v - v1->uv.v);
    glm::vec2 v0v2UVEdgeVector = -glm::vec2(v0->uv.u - v2->uv.u, v0->uv.v - v2->uv.v);

    glm::mat2x2 uvEdgeMatrix(glm::vec2(v0v1UVEdgeVector.x, v0v2UVEdgeVector.x), glm::vec2(v0v1UVEdgeVector.y, v0v2UVEdgeVector.y));
    return uvEdgeMatrix;
}

static void AssignRowsFromMatrix(const glm::mat3x2 &matrix, glm::vec2 &row1, glm::vec2 &row2, glm::vec2 &row3)
{
    row1 = matrix[0];
    row2 = matrix[1];
    row3 = matrix[2];
}

Vector3f ImageTextureImpl::ComputeNormalValueOn(float u, float v) const
{
    Vector3f normal = mTextureReader->ComputeRGBValueOn(u, v) / 255.0f;

    normal = Normalize((normal - 0.5f) * 2.0f); // [0,1] -> [-1, 1]

    return normal;
}

Vector3f ImageTextureImpl::GetBumpedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Triangle *triangle) const
{
    if(!triangle) return {};

    glm::vec3 tangent, bitangent, normal;
    ComputeTBNVectors(*triangle, tangent, bitangent, normal);

    float standardColor, horizontalOffsetColor, verticalOffsetColor;
    ComputeTextureColorValues(intersectedSurfaceInformation.uv, standardColor, horizontalOffsetColor, verticalOffsetColor);

    // dpdu, dpdv
    // Tangents are tweaked using how much Color values are changed with very small u and v change
    glm::vec3 tweakedTangent = tangent + (horizontalOffsetColor - standardColor) * normal;
    glm::vec3 tweakedBitangent = bitangent + (verticalOffsetColor - standardColor) * normal;

    glm::vec3 bumpedNormal = glm::cross(tweakedTangent, tweakedBitangent);
    return Vector3f{bumpedNormal};
}

void ImageTextureImpl::ComputeTBNVectors(const Triangle &triangle, glm::vec3 &tangent, glm::vec3 &bitangent, glm::vec3 &normal) const
{
    glm::vec2 combinedMatrixRow1;
    glm::vec2 combinedMatrixRow2;
    glm::vec2 combinedMatrixRow3;
    AssignCombinedMatrixRows(triangle, combinedMatrixRow1, combinedMatrixRow2, combinedMatrixRow3);

    tangent = Normalize(Vector3f(combinedMatrixRow1.x, combinedMatrixRow2.x, combinedMatrixRow3.x));
    bitangent = Normalize(Vector3f(combinedMatrixRow1.y, combinedMatrixRow2.y, combinedMatrixRow3.y));

    normal = glm::cross(tangent, bitangent);

    tangent = tangent - normal * glm::dot(tangent, normal);
    bitangent = bitangent - glm::dot(bitangent, normal) * normal - glm::dot(tangent, bitangent) * tangent;
}

void ImageTextureImpl::AssignCombinedMatrixRows(const Triangle &triangle, glm::vec2 &row1, glm::vec2 &row2, glm::vec2 &row3) const
{
    glm::mat3x2 edgeMatrix = ComputePositionEdgeMatrixInWorldSpace(triangle);
    glm::mat2x2 uvEdgeMatrix = glm::inverse(ComputeUVEdgeMatrix(triangle));

    glm::mat3x2 combinedMatrix = uvEdgeMatrix * edgeMatrix;

    row1 = combinedMatrix[0];
    row2 = combinedMatrix[1];
    row3 = combinedMatrix[2];
}

glm::mat3x2 ImageTextureImpl::ComputePositionEdgeMatrixInWorldSpace(const Triangle &triangle) const
{
    Vector3f p0p1t = (*triangle.objTransform)(-triangle.GetEdgeVectorFromFirstToSecondVertex(), true);
    Vector3f p0p2t = (*triangle.objTransform)(-triangle.GetEdgeVectorFromFirstToThirdVertex(), true);

    glm::vec3 firstEdgeVectorWorldSpace = p0p1t;
    glm::vec3 secondEdgeVectorWorldSpace = p0p2t;

    return glm::mat3x2(glm::vec2(firstEdgeVectorWorldSpace.x, secondEdgeVectorWorldSpace.x),
                       glm::vec2(firstEdgeVectorWorldSpace.y, secondEdgeVectorWorldSpace.y),
                       glm::vec2(firstEdgeVectorWorldSpace.z, secondEdgeVectorWorldSpace.z));
}

void ImageTextureImpl::ComputeTextureColorValues(const Vector2f &intersectionPointUV, float &standardColor, float &horizontalColor, float &verticalColor, float uvOffsetEpsilon) const
{
    float u = intersectionPointUV.x - std::floor(intersectionPointUV.x);
    float v = intersectionPointUV.y - std::floor(intersectionPointUV.y);

    float multiplier = mBumpFactor / 255;

    Vector3f uvColor = mTextureReader->ComputeRGBValueOn(u, v) * multiplier;
    Vector3f horizontalOffsetUVColor = mTextureReader->ComputeRGBValueOn(u + uvOffsetEpsilon, v) * multiplier;
    Vector3f verticalOffsetUVColor = mTextureReader->ComputeRGBValueOn(u, v + uvOffsetEpsilon) * multiplier;

    standardColor = GetAverageValue(uvColor);
    horizontalColor = GetAverageValue(horizontalOffsetUVColor);
    verticalColor = GetAverageValue(verticalOffsetUVColor);
}

static float GetAverageValue(const Vector3f &color)
{
    return (color.x + color.y + color.z) / 3;
}

Vector3f ImageTextureImpl::GetReplacedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Sphere *sphere) const
{
    if(!sphere) return {};

    glm::vec3 tangent, bitangent, normal;
    ComputeTBNVectors(*sphere, intersectedSurfaceInformation, tangent, bitangent, normal);

    glm::mat3x3 TBNMatrix(tangent, bitangent, normal);
    Vector3f replacedNormal = TBNMatrix * ComputeNormalValueOn(intersectedSurfaceInformation.uv.x, intersectedSurfaceInformation.uv.y);
    return Normalize(replacedNormal);
}

Vector3f ImageTextureImpl::GetBumpedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Sphere *sphere) const
{
    if(!sphere) return {};

    glm::vec3 tangent, bitangent, normal;
    ComputeTBNVectors(*sphere, intersectedSurfaceInformation, tangent, bitangent, normal);

    float standardColor, horizontalOffsetColor, verticalOffsetColor;
    ComputeTextureColorValues(intersectedSurfaceInformation.uv, standardColor, horizontalOffsetColor, verticalOffsetColor, 0.002f);

    return ComputeSphereBumpedNormalFromTBN(tangent, bitangent, normal, standardColor, horizontalOffsetColor, verticalOffsetColor);
}

void ImageTextureImpl::ComputeTBNVectors(const Sphere &sphere, const SurfaceIntersection& intersection, glm::vec3 &tangent, glm::vec3 &bitangent, glm::vec3 &normal) const
{
    Vector3f T, B;
    GetTangentValues(sphere, intersection, T, B);
    tangent = Normalize(T);
    bitangent = Normalize(B);

    normal = intersection.n;

    tangent = tangent - normal * glm::dot(tangent, normal);
    bitangent = bitangent - glm::dot(bitangent, normal) * normal - glm::dot(tangent, bitangent) * tangent;
}

void ImageTextureImpl::GetTangentValues(const Sphere &sphere, const SurfaceIntersection &intersection, Vector3f &tangent, Vector3f &bitangent) const
{
    float phi = intersection.lip.y, theta = intersection.lip.x;
    float u = intersection.uv.u;
    float v = intersection.uv.v;

    float x = sphere.GetRadius() * std::sin(v * PI) * std::cos(PI - 2*u*PI);
    float y = sphere.GetRadius() * std::cos(v * PI);
    float z = sphere.GetRadius() * std::sin(v * PI) * std::sin(PI - 2*u*PI);

    tangent = Vector3f(2*z*PI, .0f, -2*x*PI);
    bitangent = Vector3f(y * std::cos(phi) * PI, -sphere.GetRadius() * std::sin(theta) * PI, y * std::sin(phi) * PI);
}

Vector3f ImageTextureImpl::ComputeSphereBumpedNormalFromTBN(const glm::vec3 &tangent, const glm::vec3 &bitangent, const glm::vec3 &normal,
                                                            const float standardColor, const float horizontalOffsetColor, const float verticalOffsetColor) const
{
    float horizontalDifference = (horizontalOffsetColor - standardColor);
    float verticalDifference = (verticalOffsetColor - standardColor);

    glm::vec3 bumpedNormal = normal - tangent*horizontalDifference - bitangent*verticalDifference;
    bumpedNormal = glm::normalize(bumpedNormal);

    if (glm::dot(bumpedNormal, normal) < 0)
        bumpedNormal *= -1;

    return bumpedNormal;
}

}