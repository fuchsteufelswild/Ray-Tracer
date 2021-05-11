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

enum class ImageType;
enum class InterpolationMethodCode;
enum class NoiseConversionType;

enum class DecalMode
{
    REPLACE_NORMAL,
    REPLACE_KD,
    REPLACE_BACKGROUND,
    REPLACE_ALL,
    BLEND_KD,
    BUMP_NORMAL
};

enum class TextureType
{
    PERLIN,
    IMAGE
};

class Texture
{
friend class PerlinTextureImpl;
friend class ImageTextureImpl;

public:
    void SetupImageTexture(const std::string &imagePath, float bumpFactor, int normalizer, ImageType imageType, InterpolationMethodCode interpolationMethod);
    void SetupPerlinTexture(float bumpFactor, float noiseScale, NoiseConversionType method, std::default_random_engine& generator);

    bool IsValid() const;
public:
    Texture();
public:
    virtual Vector3f RetrieveRGBFromUV(float u, float v, float z = 0.0f) const;
protected:
    class TextureImpl;
    TextureImpl* mTextureImpl;
};

inline bool Texture::IsValid() const
{
    return mTextureImpl != nullptr;
}

} // namespace actracer
#endif