#ifndef _IMAGE_H_
#define _IMAGE_H_

#include <cstdio>
#include <cstdlib>
#include "acmath.h"

namespace actracer {
    
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
    Color **data; // Image data
    int width;    // Image width
    int height;   // Image height

    Image(int width, int height);                             // Constructor
    void SetPixelValue(int col, int row, const Color &color); // Sets the value of the pixel at the given column and row
    void SaveImage(const char *imageName) const;              // Takes the image name as a file and saves it as a ppm file.
};

}
#endif
