#ifndef _IMAGE_H_
#define _IMAGE_H_

#include <cstdio>
#include <cstdlib>

namespace actracer {
    

class Tonemapper;
// Access the color using either individual component names
// or the channel array.
typedef union Color {
    struct
    {
        unsigned char red;
        unsigned char grn;
        unsigned char blu;
    };

    unsigned char channel[3];

    Color() : red(0), grn(0), blu(0) { }
    Color(unsigned char r, unsigned char g, unsigned char b)
        : red(r), grn(g), blu(b) { }


} Color;

// Image file to create, save, and manipulate a .ppm file
class Image
{
public:
    Image(int width, int height, const char* imageName, const Tonemapper* tonemapper = nullptr);
    ~Image();
    void SetPixelColor(int col, int row, const Color &color);
    Color GetPixelColor(int col, int row) const;
    void SaveImage() const;
protected:
    void SaveImageAsPPM() const;
    void SaveImageAsEXR() const;
public:
    int GetImageWidth() const;
    int GetImageHeight() const;
    int GetImageSize() const;

private:
    Color **imageData;
    int mImageWidth;
    int mImageHeight;
    const char* mImageName;
    const Tonemapper* mTonemapper;
};

inline int Image::GetImageWidth() const
{
    return mImageWidth;
}

inline int Image::GetImageHeight() const
{
    return mImageWidth;
}

inline int Image::GetImageSize() const
{
    return mImageWidth * mImageHeight;
}

}
#endif
