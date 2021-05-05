#include "TextureImpl.h"

#include "acmath.h"

namespace actracer
{

enum class InterpolationMethodCode;

enum class ImageType;

class TextureReader;

class ImageTextureImpl : public Texture::TextureImpl
{
public:
    ImageTextureImpl(const std::string& imagePath, float bumpFactor, int normalizer, ImageType imageType, InterpolationMethodCode interpolationMethod);

    virtual Vector3f RetrieveRGBFromUV(float u, float v, float w = 0) const override;

    virtual Vector3f GetBumpedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Triangle *triangle = nullptr) const override;
    virtual Vector3f GetBumpedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Sphere *sphere = nullptr) const override;

    virtual Vector3f GetReplacedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Triangle *triangle = nullptr) const override;
    virtual Vector3f GetReplacedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Sphere *sphere = nullptr) const override;

    virtual Vector3f GetBaseTextureColorForColorChange(const SurfaceIntersection& intersection) const override;
private:
    /*
     * Computes two tangents of the surface and derives normal vector from them
     * Results are written into passed references
     */
    void ComputeTBNVectors(const Triangle &triangle, glm::vec3 &tangent, glm::vec3 &bitangent, glm::vec3 &normal) const;
    void AssignCombinedMatrixRows(const Triangle &triangle, glm::vec2 &row1, glm::vec2 &row2, glm::vec2 &row3) const;
    glm::mat3x2 ComputePositionEdgeMatrixInWorldSpace(const Triangle &triangle) const;
    glm::mat3x2 ComputePositionEdgeMatrixInObjectSpace(const Triangle &triangle) const;
    glm::mat2x2 ComputeUVEdgeMatrix(const Triangle& triangle) const;
private:
    void ComputeTextureColorValues(const Vector2f& intersectionPointUV, float& standardColor, float& horizontalColor, float& verticalColor, float uvOffsetEpsilon = 0.001f) const;

    Vector3f ComputeNormalValueOn(float u, float v) const;
private:
    void ComputeTBNVectors(const Sphere &sphere,const SurfaceIntersection& intersection, glm::vec3 &tangent, glm::vec3 &bitangent, glm::vec3 &normal) const;
    void GetTangentValues(const Sphere& sphere, const SurfaceIntersection& intersection, Vector3f& tangent, Vector3f& bitangent) const;
    Vector3f ComputeSphereBumpedNormalFromTBN(const glm::vec3& tangent, const glm::vec3& bitangent, const glm::vec3& normal,
                                              const float standardColor, const float horizontalOffsetColor, const float verticalOffsetColor) const;
private:
    TextureReader* mTextureReader;

    float mNormalizer;
};

}