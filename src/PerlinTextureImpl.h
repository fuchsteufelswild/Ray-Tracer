#pragma once

#include "TextureImpl.h"
#include "acmath.h"

#include <vector>

namespace actracer
{

enum class NoiseConversionType
{
    LINEAR,
    ABSVAL
};

class PerlinTextureImpl : public Texture::TextureImpl
{
public:
    PerlinTextureImpl(float bumpFactor, float noiseScale, NoiseConversionType method);

    virtual Vector3f RetrieveRGBFromUV(float u, float v, float z) const override;

    virtual Vector3f GetBumpedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Triangle *triangle = nullptr) const override;
    virtual Vector3f GetBumpedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Sphere *sphere = nullptr) const override;

    virtual Vector3f GetReplacedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Triangle *triangle = nullptr) const override;
    virtual Vector3f GetReplacedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Sphere *sphere = nullptr) const override;

    virtual Vector3f GetBaseTextureColorForColorChange(const SurfaceIntersection& intersection) const override;
private:
    Vector3f GetTweakedNormal(const SurfaceIntersection& intersectedSurfaceInformation) const;
    Vector3f GetDifferenceVector(const Vector3f& intersectionPoint) const;
private:
    /*
     * Use i, j, k to compute index
     * The indices array is shuffled in constructor, results are random 
     * but same (i, j, k) triple always returns the same GradientVector
     */ 
    Vector3f GetGradientVector(int i, int j, int k) const;
    int GetShuffledIndex(int i) const;

    Vector3f GetNoiseScaledUVParameters(float u, float v, float z) const;
    Vector3i BoundFloorCoordinates(const Vector3i &unboundedFlooredPoint) const;
    Vector3i GetBoundedCeiledCoordinates(const Vector3i &boundedFlooredPoint) const;

    float ComputeResult(const Vector3i &boundedFlooredLatticePoint, const Vector3i &boundedCeiledLatticePoint,
                        const Vector3f &directionToFlooredPoint, const Vector3f &directionToCeiledPoint) const;

    void ApplyNoiseConversion(float& result) const;

private:
    float mOffsetEpsilon = 0.001f;

    Vector3f gradientVectors[16] = {
        Vector3f{1.0f, 1.0f, 0.0f},
        Vector3f{-1.0f, 1.0f, 0.0f},
        Vector3f{1.0f, -1.0f, 0.0f},
        Vector3f{-1.0f, -1.0f, 0.0f},
        Vector3f{1.0f, 0.0f, 1.0f},
        Vector3f{-1.0f, 0.0f, 1.0f},
        Vector3f{1.0f, 0.0f, -1.0f},
        Vector3f{-1.0f, 0.0f, -1.0f},
        Vector3f{0.0f, 1.0f, 1.0f},
        Vector3f{0.0f, -1.0f, 1.0f},
        Vector3f{0.0f, 1.0f, -1.0f},
        Vector3f{0.0f, -1.0f, -1.0f},
        Vector3f{1.0f, 1.0f, 0.0f},
        Vector3f{-1.0f, 1.0f, 0.0f},
        Vector3f{0.0f, -1.0f, 1.0f},
        Vector3f{0.0f, -1.0f, -1.0f}};

    std::vector<int> mIndices;
private:
    float mNoiseScale;
    NoiseConversionType mConversionMethod;

    int mTableSize = 16;
};

}