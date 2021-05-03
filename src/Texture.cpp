#include "Texture.h"

#include "Scene.h"

#include <cmath>
#include <cstdio>
#include <random>
#include <functional>
#include <iostream>
#include <fstream>
#include <algorithm>

namespace actracer
{

Texture::Texture(int id, DecalMode dm, float bump, int _normalizer)
    : textureID(id), decalMode(dm), bumpFactor(bump), normalizer(_normalizer)
{
}

ImageTexture::ImageTexture(std::string imagePath, int id, DecalMode dm, float bump, InterpolationType itype, int _normalizer, float* _d)
    : Texture(id, dm, bump, _normalizer), interpolationMethod(itype)
{
    this->texType = TextureType::IMAGE;

    if(imagePath.back() == 'r')
    {
        data = _d;
        isExr = true;
        // std::cout << " It is exr \n";
        // std::cout << data[2304500] << " " << data[2304501] << " " << data[2304502] << " \n";
    }
    else
    {
        textureData = stbi_load(imagePath.c_str(), &width, &height, &bpp, 3);
        if (textureData == nullptr)
            std::cout << "E R R O R\n";
    }
}

Vector3f ImageTexture::FetchRGB(int x, int y)
{
    if(x >= height)
        x = 0;
    if(y >= width)
        y = 0;

    if(x < 0)
        x = height - 1;
    if(y < 0)
        y = width - 1;


    unsigned char *start = ((textureData + this->width * y * 3) + x * 3);

    return {(float)(*start), (float)(*(start + 1)), (float)(*(start + 2))};
}

Vector3f ImageTexture::CalculateNearest(float row, float column)
{
    //std::cout << "Nearest calculation\n";
    int nearestRow = std::round(row);
    int nearestColumn = std::round(column);

    return FetchRGB(nearestColumn, nearestRow);
}

Vector3f ImageTexture::CalculateBilinear(float row, float column)
{
    int p, q; // Floored value
    p = std::floor(column);
    q = std::floor(row);

    float dx = column - p;
    float dy = row - q;

    return FetchRGB(p, q) * (1 - dx) * (1 - dy) +
           FetchRGB(p + 1, q) * (dx) * (1 - dy) +
           FetchRGB(p + 1, q + 1) * (dx) * (dy) +
           FetchRGB(p, q + 1) * (1 - dx) * (dy);
}



Vector3f ImageTexture::GetHorizontalDifference(float u, float v, float z)
{
    float column = u * width;
    float row = v * height;

    float E = 1;

    switch (this->interpolationMethod)
    {
    case InterpolationType::NEAREST:
        return ((CalculateNearest(row, column + E)) - (CalculateNearest(row, column - E)));
    case InterpolationType::BILINEAR:
        return ((CalculateBilinear(row, column + E)) - (CalculateNearest(row, column - E)));
    default:
        break;
    }
}

Vector3f ImageTexture::GetVerticalDifference(float u, float v, float z)
{
    float column = u * width;
    float row = v * height;
    // return CalculateNearest(row, column);

    float E = 1;
    

    switch (this->interpolationMethod)
    {
    case InterpolationType::NEAREST:
        return ((CalculateNearest(row + E, column)) - (CalculateNearest(row - E, column)));
    case InterpolationType::BILINEAR:
        return ((CalculateBilinear(row + E, column)) - (CalculateNearest(row - E, column)));
    default:
        break;
    }
}

Vector3f ImageTexture::GetPixelOffset(float u, float v, float offsetX, float offsetY)
{
    float di = u * (width - 1);
    float dj = v * (height - 1);

    int i = di + offsetX;
    int j = dj + offsetY;


    switch (this->interpolationMethod)
    {
    case InterpolationType::NEAREST:
        return CalculateNearest(j, i);
    case InterpolationType::BILINEAR:
        return CalculateBilinear(j, i);
    default:
        break;
    }
}

Vector3f ImageTexture::GetBumpNormal(const Vector3f &n, float u, float v, Vector3f pu, Vector3f pv)
{
    float EPSILON = 0.001f;
    float referenceU = u < 1 - EPSILON ? u + EPSILON : u - EPSILON;
    float referenceV = v < 1 - EPSILON ? v + EPSILON : v - EPSILON;

    Vector3f textureColor = RetrieveRGBFromUV(u, v);
    Vector3f referenceTextureColor = RetrieveRGBFromUV(referenceU, v);
    float averageColorDU = (textureColor.x + textureColor.y + textureColor.z) /* / 3.f * bumpmapMultiplier */;
    float averageReferenceDU = (referenceTextureColor.x + referenceTextureColor.y + referenceTextureColor.z) /*  / 3.f * bumpmapMultiplier */;
    float du = (averageReferenceDU - averageColorDU) / 6.f * bumpFactor;

    referenceTextureColor = RetrieveRGBFromUV(u, referenceV);
    float averageColorDV = (textureColor.x + textureColor.y + textureColor.z) /* / 3.f * bumpmapMultiplier */;
    float averageReferenceDV = (referenceTextureColor.x + referenceTextureColor.y + referenceTextureColor.z) /* / 3.f * bumpmapMultiplier */;
    float dv = (averageReferenceDV - averageColorDV) / 6.f * bumpFactor;

    Vector3f nPrime = Cross(pv + dv * n, pu + du * n);

    return Normalize(nPrime);
}

Vector3f ImageTexture::Fetch(int i, int j, int w /* = 0.f */) const
{

    if(i < 0 || j < 0 || i >= width || j >= height)
    {
        // std::cout << i << " " << j << "\n";
        ;
    }



    if(i > width - 1)
        i = 0;
    if(i < 0)
        i = width - 1;
    if(j > height - 1)
        j = 0;
    if(j < 0)
        j = height - 1;

    int index = 3 * (j * width + i);

    Vector3f color;

    if(isExr)
    {
        color.x = data[index];
        color.y = data[index + 1];
        color.z = data[index + 2];
    }
    else
    {
        color.x = textureData[index];
        color.y = textureData[index + 1];
        color.z = textureData[index + 2];
    }

    return color;
}

Vector3f ImageTexture::RetrieveRGBFromUV(float u, float v, float w) const
{
    if(u > 1.0f)
        u = u - (int)u;
    if(v > 1.0f)
        v = v - (int)v;

    if (interpolationMethod == InterpolationType::NEAREST)
    {
        int i = round(u * (width - 1));
        int j = round(v * (height - 1));
        return Fetch(i, j);
    }

    // Else if interpolationMethod == INTERPOLATION::BILINEAR
    float i = u * (width - 1);
    float j = v * (height - 1);

    int p = floor(i);
    int q = floor(j);
    float dx = i - p;
    float dy = j - q;

    return Fetch(p, q) * (1 - dx) * (1 - dy) +
           Fetch(p + 1, q) * (dx) * (1 - dy) +
           Fetch(p, q + 1) * (1 - dx) * (dy) +
           Fetch(p + 1, q + 1) * (dx) * (dy);
}

float PerlinTexture::PerlinWeight(float x) const
{
    return 6 * x * x * x * x * x - 15 * x * x * x * x + 10 * x * x * x;
}

int PerlinTexture::GetShuffledIndex(int i) const
{
    int res = i % N;
    if(res < 0)
        res += N;
    return indices[res];
}

Vector3f PerlinTexture::GetGradientVector(int i, int j, int k) const
{
    return gradientVectors[GetShuffledIndex(i + GetShuffledIndex(j + GetShuffledIndex(k)))];
}

int PerlinTexture::hash(int i, int j, int k)
{
    return GetShuffledIndex(i + GetShuffledIndex(j + GetShuffledIndex(k)));
}

template <typename T = float>
inline T lerp(const T &lo, const T &hi, const T &t)
{
    return lo * (1 - t) + hi * t;
}

inline float smoothstep(const float &t)
{
    return 6 * t * t * t * t * t - 15 * t * t * t * t + 10 * t * t * t;
}

Vector3f PerlinTexture::RetrieveRGBFromUV(float u, float v, float z) const
{
    u *= noiseScale;
    v *= noiseScale;
    z *= noiseScale;

    Vector3f p(u, v, z);

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

    float w0 = smoothstep(dxf); // 
    float w1 = smoothstep(dyf); // Perlin weights of directions
    float w2 = smoothstep(dzf); // 

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

    float result = Lerp(ll0,ll1, w2); // Lerp last two values

    // Noise conversion
    if (this->conversionMethod == NoiseConversionType::ABSVAL)
        result = std::abs(result);
    else if (this->conversionMethod == NoiseConversionType::LINEAR)
        result = (result + 1) / 2.0f;

    return {result, result, result};
}

PerlinTexture::PerlinTexture(int id, DecalMode dm, float bump, float scale, NoiseConversionType method)
    : Texture(id, dm, bump, 1.0f), noiseScale(scale), conversionMethod(method)
{
    this->texType = TextureType::PERLIN;

    this->texType = TextureType::PERLIN;
    seed = std::chrono::system_clock::now().time_since_epoch().count();
    engine = std::default_random_engine(seed);

    for (int i = 0; i < tableSize; i++)
        indices.push_back(i);

    std::shuffle(indices.begin(), indices.end(), Scene::pScene->generator);
}
} // namespace actracer