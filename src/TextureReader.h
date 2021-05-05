#pragma once

#include "acmath.h"

namespace actracer
{

enum class ImageType
{
    EXR,
    PNG
};

enum class InterpolationMethodCode;

class TextureReader;
class TextureValueRetrieveMethod;

class TextureReader
{
public:
    static TextureReader *CreateTextureReader(const std::string &imagePath, ImageType imageType, InterpolationMethodCode interplationMethodCode);

    Vector3f ComputeRGBValueOn(float u, float v);

    Vector3f FetchPixelValueFromTexture(int row, int colum) const;

    virtual ~TextureReader() { }
public:
    int GetWidth() const;
    int GetHeight() const;
protected:
    TextureReader(InterpolationMethodCode interpolationMethodCode);
    virtual Vector3f GetColorData(int index) const = 0;
private:
    TextureValueRetrieveMethod* mTextureValueRetriever;

    int GetRepeatRow(int row) const;
    int GetRepeatColumn(int column) const;

    int CalculateIndexFor(int row, int column) const;
protected:
    int mWidth;
    int mHeight;
};

inline int TextureReader::GetWidth() const
{
    return mWidth;
}

inline int TextureReader::GetHeight() const
{
    return mHeight;
}

class EXRTextureReader : public TextureReader
{
public:
    EXRTextureReader(const std::string &exrImagePath, InterpolationMethodCode interpolationMethodCode);

    virtual ~EXRTextureReader() override;
protected:
    virtual Vector3f GetColorData(int index) const override;
private:
    float *mExrImageData;
};

class PNGTextureReader : public TextureReader
{
public:
    PNGTextureReader(const std::string &pngImagePath, InterpolationMethodCode interpolationMethodCode);

    virtual ~PNGTextureReader() override;
protected:
    virtual Vector3f GetColorData(int index) const override;
private:
    int mBytesPerPixel;
    unsigned char* mPngImageData;
};

}