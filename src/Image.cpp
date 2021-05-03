#include "Image.h"

namespace actracer {

Image::Image(int width, int height)
    : mImageWidth(width), mImageHeight(height)
{
    data = new Color *[height];

    for (int y = 0; y < height; ++y)
    {
        data[y] = new Color[width];
    }
}

Color Image::GetPixelColor(int col, int row) const
{
    return data[row][col];
}

void Image::SetPixelColor(int col, int row, const Color &color)
{
    data[row][col] = color;
}

void Image::SaveImageAsPPM(const char *imageName) const
{
    FILE *output;

    output = fopen(imageName, "w");
    fprintf(output, "P3\n");
    fprintf(output, "%d %d\n", mImageWidth, mImageHeight);
    fprintf(output, "255\n");

    for (int y = 0; y < mImageHeight; y++)
    {
        for (int x = 0; x < mImageWidth; x++)
        {
            for (int c = 0; c < 3; ++c)
            {
                fprintf(output, "%d ", data[y][x].channel[c]);
            }
        }

        fprintf(output, "\n");
    }

    fclose(output);
}

}
