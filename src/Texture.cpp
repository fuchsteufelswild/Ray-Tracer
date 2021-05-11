#include "Texture.h"

#include "ImageTextureImpl.h"
#include "PerlinTextureImpl.h"

namespace actracer
{

Texture::Texture()
    : mTextureImpl(nullptr)
    { }

void Texture::SetupImageTexture(const std::string &imagePath, float bumpFactor, int normalizer, ImageType imageType, InterpolationMethodCode interpolationMethod)
{
    mTextureImpl = new ImageTextureImpl(imagePath, bumpFactor, normalizer, imageType, interpolationMethod);
}

void Texture::SetupPerlinTexture(float bumpFactor, float noiseScale, NoiseConversionType method, std::default_random_engine& generator)
{
    mTextureImpl = new PerlinTextureImpl(bumpFactor, noiseScale, method, generator);
}

Vector3f Texture::RetrieveRGBFromUV(float u, float v, float z) const
{
    if(IsValid())
        return mTextureImpl->RetrieveRGBFromUV(u, v, z);

    return {};
}

}