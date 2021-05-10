#include "Image.h"

#include "Tonemapper.h"

namespace actracer {

Image::Image(int width, int height, const char* imageName, const Tonemapper* tonemapper)
    : mImageWidth(width), mImageHeight(height), mTonemapper(tonemapper), mImageName(imageName)
{
    imageData = new Color *[height];

    for (int y = 0; y < height; ++y)
    {
        imageData[y] = new Color[width];
    }
}

Image::~Image()
{
    for (int y = 0; y < mImageHeight; ++y)
    {
        delete[] imageData[y];
    }

    delete[] imageData;
}

Color Image::GetPixelColor(int col, int row) const
{
    return imageData[row][col];
}

void Image::SetPixelColor(int col, int row, const Color &color)
{
    imageData[row][col] = color;
}

void Image::SaveImage() const
{
    if (mTonemapper)
        SaveImageAsEXR();
    else
        SaveImageAsPPM();
}

void Image::SaveImageAsEXR() const
{
    float *tonemappedColorOutputValues = mTonemapper->Tonemap(*this);
    mTonemapper->SaveEXR(tonemappedColorOutputValues, GetImageWidth(), GetImageHeight(), mImageName);

    delete[] tonemappedColorOutputValues;
}

void Image::SaveImageAsPPM() const
{
    FILE *output;

    output = fopen(mImageName, "w");
    fprintf(output, "P3\n");
    fprintf(output, "%d %d\n", mImageWidth, mImageHeight);
    fprintf(output, "255\n");

    for (int y = 0; y < mImageHeight; y++)
    {
        for (int x = 0; x < mImageWidth; x++)
        {
            for (int c = 0; c < 3; ++c)
            {
                fprintf(output, "%d ", imageData[y][x].channel[c]);
            }
        }

        fprintf(output, "\n");
    }

    fclose(output);
}

}
