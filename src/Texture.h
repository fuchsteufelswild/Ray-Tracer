#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#include <string>
#include <iostream>
#include "stb_image.h"
#include "acmath.h"
#include "Random.h"

#include <random>
#include <chrono>
#include <algorithm>

namespace actracer
{

class Texture
{
public:
    enum DecalMode
    {
        REPLACE_NORMAL,
        REPLACE_KD,
        REPLACE_BACKGROUND,
        REPLACE_ALL,
        BLEND_KD,
        BUMP_NORMAL
    };
    enum TextureType
    {
        PERLIN,
        IMAGE
    };

public:
    

protected:
    
    

public:

    int textureID;
    int height;
    int width;
    int bpp;
    DecalMode decalMode;
    TextureType texType;
    float bumpFactor;
    int normalizer;

protected:
    

protected:
    Texture(int id, DecalMode dm, float bumpFactor, int normalizer);
    Texture();
public:
    virtual Vector3f RetrieveRGBFromUV(float u, float v, float z = 0.0f) const { }
    virtual Vector3f GetHorizontalDifference(float u, float v, float z = 0.0f) { }
    virtual Vector3f GetVerticalDifference(float u, float v, float z = 0.0f) {}
    virtual Vector3f GetPixelOffset(float u, float v, float offsetX, float offsetY) { }
    virtual Vector3f GetBumpNormal(const Vector3f& n, float u, float v, Vector3f pu, Vector3f pv) {}

    virtual Vector3f Fetch(int i, int j, int w = 0.0f /* = 0.f */) const {}
};

class ImageTexture : public Texture
{
public:
    enum InterpolationType
    {
        NEAREST,
        BILINEAR
    };

protected:
    unsigned char *textureData;
    
    InterpolationType interpolationMethod;
    bool isExr = false;
    ImageTexture();
public:
    float *data;
    ImageTexture(std::string imagePath, int id, DecalMode dm, float bumpFactor, InterpolationType itype, int _normalizer, float* _d = nullptr);

    Vector3f FetchRGB(int x, int y);
    Vector3f CalculateNearest(float row, float column);
    Vector3f CalculateBilinear(float row, float column);

    virtual Vector3f Fetch(int i, int j, int w = 0.0f /* = 0.f */) const override;
    virtual Vector3f RetrieveRGBFromUV(float u, float v, float z = 0.0f) const override;
    virtual Vector3f GetHorizontalDifference(float u, float v, float z = 0.0f) override;
    virtual Vector3f GetVerticalDifference(float u, float v, float z = 0.0f) override;
    virtual Vector3f GetPixelOffset(float u, float v, float offsetX, float offsetY) override;
    virtual Vector3f GetBumpNormal(const Vector3f &n, float u, float v, Vector3f pu, Vector3f pv) override;
};

class PerlinTexture : public Texture
{
public:
    enum NoiseConversionType
    {
        LINEAR,
        ABSVAL
    };

protected:
    float noiseScale;
    NoiseConversionType conversionMethod;

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
                                    Vector3f{0.0f, -1.0f, -1.0f} 
                                   };

    Vector3f gradients[16];
                                
    std::vector<int> indices;

public:
    PerlinTexture(int id, DecalMode dm, float bumpFactor, float scale, NoiseConversionType method);

    virtual Vector3f RetrieveRGBFromUV(float u, float v, float z = 0.0f) const override;

private:
    unsigned seed;
    std::default_random_engine engine;

    int N = 16;
    int tableSize = 16;
    int tableSizeMask = 15;

    template<typename T>
    void ShuffleArray(std::vector<T>& vec)
    {
        unsigned ss = std::chrono::system_clock::now().time_since_epoch().count();
        std::shuffle(vec.begin(), vec.end(), std::default_random_engine(ss));
    }

    void ShuffleIndices();

    float PerlinWeight(float x) const;

    Vector3f GetGradientVector(int i, int j, int k) const;
    int GetShuffledIndex(int i) const;
    int      hash(int, int, int);
};

} // namespace actracer
#endif