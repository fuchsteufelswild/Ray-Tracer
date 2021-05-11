#include "PerlinTextureImpl.h"
#include "Scene.h"

#include <algorithm>

namespace actracer
{
static Vector3i GetUnboundedFlooredCoordinates(const Vector3i& point);
static Vector3f GetFlooredDirectionWeights(const Vector3f& flooredPoint);

Vector3f PerlinTextureImpl::GetBaseTextureColorForColorChange(const SurfaceIntersection &intersection) const
{
    return RetrieveRGBFromUV(intersection.ip.x, intersection.ip.y, intersection.ip.z);
}

Vector3f PerlinTextureImpl::GetReplacedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Triangle *triangle) const
{
    return {};
}

Vector3f PerlinTextureImpl::GetReplacedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Sphere *sphere) const
{
    return {};
}

PerlinTextureImpl::PerlinTextureImpl(float bumpFactor, float noiseScale, NoiseConversionType method, std::default_random_engine& generator)
    : TextureImpl(bumpFactor), mNoiseScale(noiseScale), mConversionMethod(method)
{
    for (int i = 0; i < mTableSize; i++)
        mIndices.push_back(i);

    std::shuffle(mIndices.begin(), mIndices.end(), generator);
}

Vector3f PerlinTextureImpl::GetBumpedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Triangle *triangle) const
{
    return GetTweakedNormal(intersectedSurfaceInformation);
}

Vector3f PerlinTextureImpl::GetBumpedNormal(const SurfaceIntersection &intersectedSurfaceInformation, const Sphere *sphere) const
{
    return GetTweakedNormal(intersectedSurfaceInformation);
}

Vector3f PerlinTextureImpl::GetTweakedNormal(const SurfaceIntersection &intersectedSurfaceInformation) const
{
    Vector3f differenceVector = GetDifferenceVector(intersectedSurfaceInformation.ip);

    Vector3f gp = Dot(differenceVector, intersectedSurfaceInformation.n) * intersectedSurfaceInformation.n;
    Vector3f gn = differenceVector - gp;

    gn = intersectedSurfaceInformation.n - gn * mBumpFactor;
    return Normalize(gn);
}

Vector3f PerlinTextureImpl::GetDifferenceVector(const Vector3f &intersectionPoint) const
{
    float standardColorValue = RetrieveRGBFromUV(intersectionPoint.x, intersectionPoint.y, intersectionPoint.z).x;

    float xOffsetColorValue = RetrieveRGBFromUV(intersectionPoint.x + mOffsetEpsilon, intersectionPoint.y, intersectionPoint.z).x;
    float yOffsetColorValue = RetrieveRGBFromUV(intersectionPoint.x, intersectionPoint.y + mOffsetEpsilon, intersectionPoint.z).x;
    float zOffsetColorValue = RetrieveRGBFromUV(intersectionPoint.x, intersectionPoint.y, intersectionPoint.z + mOffsetEpsilon).x;

    return {(xOffsetColorValue - standardColorValue) / mOffsetEpsilon,
            (yOffsetColorValue - standardColorValue) / mOffsetEpsilon,
            (zOffsetColorValue - standardColorValue) / mOffsetEpsilon};
}

/*
 * TODO: Refactor this function
 * First Attempt: Results are wrong when decomposed into functions,
 * probably floating precision.
 */ 
Vector3f PerlinTextureImpl::RetrieveRGBFromUV(float u, float v, float z) const
{
    
    Vector3f p = GetNoiseScaledUVParameters(u, v, z);

    int N = mTableSize;

    int xFloor = ((int)std::floor(p.x)) % N; //
    int yFloor = ((int)std::floor(p.y)) % N; // Floored coordinates
    int zFloor = ((int)std::floor(p.z)) % N; //

    int xCeil = (xFloor + 1) % N; //
    int yCeil = (yFloor + 1) % N; // Ceiled coordinates
    int zCeil = (zFloor + 1) % N; //

    int xfc[2] = {xFloor, xCeil}; //
    int yfc[2] = {yFloor, yCeil}; // Group for easier use
    int zfc[2] = {zFloor, zCeil}; //

    float dxf = p.x - ((int)std::floor(p.x)); //
    float dyf = p.y - ((int)std::floor(p.y)); // Directions to floored points
    float dzf = p.z - ((int)std::floor(p.z)); //

    float dxc = dxf - 1; //
    float dyc = dyf - 1; // Directions to ceiled points
    float dzc = dzf - 1; //

    float w0 = Smoothstep(dxf); //
    float w1 = Smoothstep(dyf); // Perlin weights of directions
    float w2 = Smoothstep(dzf); //

    Vector3f g0 = GetGradientVector(xfc[0], yfc[0], zfc[0]); //
    Vector3f g1 = GetGradientVector(xfc[0], yfc[0], zfc[1]); //
    Vector3f g2 = GetGradientVector(xfc[0], yfc[1], zfc[0]); //
    Vector3f g3 = GetGradientVector(xfc[0], yfc[1], zfc[1]); // Gradient vectors of the lattice points
    Vector3f g4 = GetGradientVector(xfc[1], yfc[0], zfc[0]); //
    Vector3f g5 = GetGradientVector(xfc[1], yfc[0], zfc[1]); //
    Vector3f g6 = GetGradientVector(xfc[1], yfc[1], zfc[0]); //
    Vector3f g7 = GetGradientVector(xfc[1], yfc[1], zfc[1]); //

    Vector3f v0(dxf, dyf, dzf); //
    Vector3f v1(dxf, dyf, dzc); //
    Vector3f v2(dxf, dyc, dzf); //
    Vector3f v3(dxf, dyc, dzc); // Direction vectors of the lattice points
    Vector3f v4(dxc, dyf, dzf); // ( Direction towards the intersection point p )
    Vector3f v5(dxc, dyf, dzc); //
    Vector3f v6(dxc, dyc, dzf); //
    Vector3f v7(dxc, dyc, dzc); //

    // linear interpolation
    float l0 = Lerp(Dot(g0, v0), Dot(g4, v4), w0); //
    float l1 = Lerp(Dot(g2, v2), Dot(g6, v6), w0); // Lerp between corners
    float l2 = Lerp(Dot(g1, v1), Dot(g5, v5), w0); //
    float l3 = Lerp(Dot(g3, v3), Dot(g7, v7), w0); //

    float ll0 = Lerp(l0, l1, w1); // Lerp lerped corners
    float ll1 = Lerp(l2, l3, w1); //

    float result = Lerp(ll0, ll1, w2); // Lerp last two values

    // Noise conversion
    if (this->mConversionMethod == NoiseConversionType::ABSVAL)
        result = std::abs(result);
    else if (this->mConversionMethod == NoiseConversionType::LINEAR)
        result = (result + 1) / 2.0f;

    return {result, result, result};
    // Vector3f point = GetNoiseScaledUVParameters(u, v, z);
    // Vector3i unboundedFlooredLatticePoint = GetUnboundedFlooredCoordinates(Vector3i(point.x, point.y, point.z));

    // Vector3i boundedFlooredLatticePoint = BoundFloorCoordinates(unboundedFlooredLatticePoint);
    // Vector3i boundedCeiledLatticePoint = GetBoundedCeiledCoordinates(boundedFlooredLatticePoint);

    // Vector3f directionToFlooredLatticePoint = point - unboundedFlooredLatticePoint;
    // Vector3f directionToCeiledLatticePoint = directionToFlooredLatticePoint - Vector3i(1, 1, 1);

    // float result = ComputeResult(boundedFlooredLatticePoint, boundedCeiledLatticePoint, directionToFlooredLatticePoint, directionToCeiledLatticePoint);

    // ApplyNoiseConversion(result);

    // return {result, result, result};
}

Vector3f PerlinTextureImpl::GetNoiseScaledUVParameters(float u, float v, float z) const
{
    return {u * mNoiseScale, v * mNoiseScale, z * mNoiseScale};    
}

static Vector3i GetUnboundedFlooredCoordinates(const Vector3i &point)
{
    return { (int)std::floor(point.x), (int)std::floor(point.y), (int)std::floor(point.z) };
}

Vector3i PerlinTextureImpl::BoundFloorCoordinates(const Vector3i &unboundedFlooredPoint) const
{
    return { unboundedFlooredPoint.x % mTableSize, unboundedFlooredPoint.y % mTableSize, unboundedFlooredPoint.z % mTableSize };
}

Vector3i PerlinTextureImpl::GetBoundedCeiledCoordinates(const Vector3i &boundedFlooredPoint) const
{
    return { (boundedFlooredPoint.x + 1) % mTableSize,
             (boundedFlooredPoint.y + 1) % mTableSize,
             (boundedFlooredPoint.z + 1) % mTableSize};
}

float PerlinTextureImpl::ComputeResult(const Vector3i &boundedFlooredLatticePoint, const Vector3i &boundedCeiledLatticePoint,
                                       const Vector3f& directionToFlooredPoint, const Vector3f& directionToCeiledPoint) const
{
    Vector3f flooredDirectionWeights = GetFlooredDirectionWeights(directionToFlooredPoint);

    // Rename/Regroup for easier use
    int xFlooredCeiled[2]{boundedFlooredLatticePoint.x, boundedCeiledLatticePoint.x};
    int yFlooredCeiled[2]{boundedFlooredLatticePoint.y, boundedCeiledLatticePoint.y};
    int zFlooredCeiled[2]{boundedFlooredLatticePoint.z, boundedCeiledLatticePoint.z};

    const float& dxf = directionToFlooredPoint.x;
    const float& dyf = directionToFlooredPoint.y;
    const float& dzf = directionToFlooredPoint.z;

    const float& dxc = directionToCeiledPoint.x;
    const float& dyc = directionToCeiledPoint.y;
    const float& dzc = directionToCeiledPoint.z;
    // --

    Vector3f g0 = GetGradientVector(xFlooredCeiled[0], yFlooredCeiled[0], zFlooredCeiled[0]); //
    Vector3f g1 = GetGradientVector(xFlooredCeiled[0], yFlooredCeiled[0], zFlooredCeiled[1]); //
    Vector3f g2 = GetGradientVector(xFlooredCeiled[0], yFlooredCeiled[1], zFlooredCeiled[0]); //
    Vector3f g3 = GetGradientVector(xFlooredCeiled[0], yFlooredCeiled[1], zFlooredCeiled[1]); // Gradient vectors of the lattice points
    Vector3f g4 = GetGradientVector(xFlooredCeiled[1], yFlooredCeiled[0], zFlooredCeiled[0]); //
    Vector3f g5 = GetGradientVector(xFlooredCeiled[1], yFlooredCeiled[0], zFlooredCeiled[1]); //
    Vector3f g6 = GetGradientVector(xFlooredCeiled[1], yFlooredCeiled[1], zFlooredCeiled[0]); //
    Vector3f g7 = GetGradientVector(xFlooredCeiled[1], yFlooredCeiled[1], zFlooredCeiled[1]); //

    Vector3f v0(dxf, dyf, dzf); //
    Vector3f v1(dxf, dyf, dzc); //
    Vector3f v2(dxf, dyc, dzf); //
    Vector3f v3(dxf, dyc, dzc); // Direction vectors of the lattice points
    Vector3f v4(dxc, dyf, dzf); // ( Direction towards the intersection point p )
    Vector3f v5(dxc, dyf, dzc); //
    Vector3f v6(dxc, dyc, dzf); //
    Vector3f v7(dxc, dyc, dzc); //

    // linear interpolation
    float l0 = Lerp(Dot(g0, v0), Dot(g4, v4), flooredDirectionWeights.x); //
    float l1 = Lerp(Dot(g2, v2), Dot(g6, v6), flooredDirectionWeights.x); // Lerp between corners
    float l2 = Lerp(Dot(g1, v1), Dot(g5, v5), flooredDirectionWeights.x); //
    float l3 = Lerp(Dot(g3, v3), Dot(g7, v7), flooredDirectionWeights.x); //

    float ll0 = Lerp(l0, l1, flooredDirectionWeights.y); // Lerp lerped corners
    float ll1 = Lerp(l2, l3, flooredDirectionWeights.y); //

    return Lerp(ll0, ll1, flooredDirectionWeights.z);
}

static Vector3f GetFlooredDirectionWeights(const Vector3f &directionToFlooredPoint)
{
    return {Smoothstep(directionToFlooredPoint.x), Smoothstep(directionToFlooredPoint.y), Smoothstep(directionToFlooredPoint.z)};
}

Vector3f PerlinTextureImpl::GetGradientVector(int i, int j, int k) const
{
    return gradientVectors[GetShuffledIndex(i + GetShuffledIndex(j + GetShuffledIndex(k)))];
}

int PerlinTextureImpl::GetShuffledIndex(int i) const
{
    int res = i % mTableSize;
    if (res < 0)
        res += mTableSize;
    return mIndices[res];
}

void PerlinTextureImpl::ApplyNoiseConversion(float &result) const
{
    switch (mConversionMethod)
    {
    case NoiseConversionType::ABSVAL:
        result = std::abs(result);
        break;
    case NoiseConversionType::LINEAR:
        result = (result + 1) / 2.0f;
        break;
    }
}

}