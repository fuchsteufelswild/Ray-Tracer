#include "stb_image.h"

#include "TextureReader.h"
#include "Tonemapper.h"

#include "TextureValueRetrieveMethod.h"

#define COLOUR_CHANNEL_COUNT 3

namespace actracer
{

int GetRepeatCoordinate(int coordinate, int minimumCoordinateValue, int maximumCoordinateValue);

TextureReader *TextureReader::CreateTextureReader(const std::string& imagePath, ImageType imageType, InterpolationMethodCode interplationMethodCode)
{
    switch(imageType)
    {
        case ImageType::EXR:
            return new EXRTextureReader(imagePath, interplationMethodCode);
        case ImageType::PNG:
            return new PNGTextureReader(imagePath, interplationMethodCode);
    }

    return nullptr;
}

EXRTextureReader::EXRTextureReader(const std::string &exrImagePath, InterpolationMethodCode interpolationMethodCode)
    : TextureReader(interpolationMethodCode)
{
    TMOData out = Tonemapper::ReadExr(exrImagePath);

    mExrImageData = out.data;
    mWidth = out.width;
    mHeight = out.height;
}

PNGTextureReader::PNGTextureReader(const std::string &pngImagePath, InterpolationMethodCode interpolationMethodCode)
    : TextureReader(interpolationMethodCode)
{
    mPngImageData = stbi_load(pngImagePath.c_str(), &mWidth, &mHeight, &mBytesPerPixel, 3);
}

TextureReader::TextureReader(InterpolationMethodCode interpolationMethodCode)
{
    mTextureValueRetriever = TextureValueRetrieveMethod::CreateTextureValueRetrieveMethod(interpolationMethodCode);
}

EXRTextureReader::~EXRTextureReader() 
{
    delete[] mExrImageData;
}

PNGTextureReader::~PNGTextureReader()
{
    delete[] mPngImageData;
}

Vector3f TextureReader::ComputeRGBValueOn(float u, float v)
{
    return mTextureValueRetriever->RetrieveValueFromUVCoordinate(this, u, v);
}

Vector3f TextureReader::FetchPixelValueFromTexture(int row, int column) const
{
    int index = CalculateIndexFor(row, column);
    
    return GetColorData(index);
}

int TextureReader::CalculateIndexFor(int row, int column) const
{
    row = GetRepeatRow(row);
    column = GetRepeatColumn(column);

    return COLOUR_CHANNEL_COUNT * (row * mWidth + column);
}

int TextureReader::GetRepeatRow(int row) const
{
    return GetRepeatCoordinate(row, 0, mHeight - 1);
}

int TextureReader::GetRepeatColumn(int column) const
{
    return GetRepeatCoordinate(column, 0, mWidth - 1);
}

int GetRepeatCoordinate(int coordinate, int minimumCoordinateValue, int maximumCoordinateValue)
{
    if(coordinate > maximumCoordinateValue) return minimumCoordinateValue;
    else if(coordinate < minimumCoordinateValue) return maximumCoordinateValue;

    return coordinate;
}

Vector3f EXRTextureReader::GetColorData(int index) const
{
    return {mExrImageData[index], mExrImageData[index + 1], mExrImageData[index + 2]};
}

Vector3f PNGTextureReader::GetColorData(int index) const
{
    return Vector3f(mPngImageData[index], mPngImageData[index + 1], mPngImageData[index + 2]);
}

}