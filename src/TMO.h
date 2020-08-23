#ifndef _TMO_H_
#define _TMO_H_

#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

#include <vector>
#include <algorithm>
#include "acmath.h"


namespace actracer {

struct TMOData {
    float* data;
    int width;
    int height;
};

class TMO {

public:
    float imKey;
    float saturationPercentage;
    float saturation;
    float gamma;
public:
    TMO(float key, float satperc, float sat, float gam)
        : imKey(key),
          saturationPercentage(satperc),
          saturation(sat),
          gamma(gam) {}

    void ComputeTonemappedArray(const std::vector<Vector3f> &input, std::vector<Vector3f> &output)
    {

        float lwhite = 0.f;
        float lumsum = 0.f;

        std::vector<float> luminances;

        for (unsigned int i = 0; i < input.size(); i++)
        {
            float lum = 0.27f * input[i].x + 0.67f * input[i].y + 0.06f * input[i].z;

            luminances.push_back(lum);
            lumsum += log(lum + 0.00001f);
        }

        std::make_heap(luminances.begin(), luminances.end());
        std::sort_heap(luminances.begin(), luminances.end());

        lwhite = luminances[round(luminances.size() * (100.f - saturationPercentage) / 100.f)] / luminances.back();

        lumsum /= input.size();
        float avglum = exp(lumsum);

        for (unsigned int i = 0; i < input.size(); i++)
        {
            float lum = 0.27f * input[i].x + 0.67f * input[i].y + 0.06f * input[i].z;
            float regulated = lum * imKey / avglum;

            float resultinglum = (regulated * (1 + (regulated / (lwhite * lwhite)))) / (1 + regulated);

            float displayR = std::pow(input[i].x / lum, saturation) * resultinglum;
            float displayG = std::pow(input[i].y / lum, saturation) * resultinglum;
            float displayB = std::pow(input[i].z / lum, saturation) * resultinglum;

            

            output.push_back(Vector3f{displayR, displayG, displayB});
        }


    }

    static bool SaveEXR(const float *rgb, int width, int height, const char *outfilename)
    {

        EXRHeader header;
        InitEXRHeader(&header);

        EXRImage image;
        InitEXRImage(&image);

        image.num_channels = 3;

        std::vector<float> images[3];
        images[0].resize(width * height);
        images[1].resize(width * height);
        images[2].resize(width * height);

        // Split RGBRGBRGB... into R, G and B layer
        for (int i = 0; i < width * height; i++)
        {
            images[0][i] = rgb[3 * i + 0];
            images[1][i] = rgb[3 * i + 1];
            images[2][i] = rgb[3 * i + 2];
        }

        float *image_ptr[3];
        image_ptr[0] = &(images[2].at(0)); // B
        image_ptr[1] = &(images[1].at(0)); // G
        image_ptr[2] = &(images[0].at(0)); // R

        image.images = (unsigned char **)image_ptr;
        image.width = width;
        image.height = height;

        header.num_channels = 3;
        header.channels = (EXRChannelInfo *)malloc(sizeof(EXRChannelInfo) * header.num_channels);
        // Must be (A)BGR order, since most of EXR viewers expect this channel order.
        strncpy(header.channels[0].name, "B", 255);
        header.channels[0].name[strlen("B")] = '\0';
        strncpy(header.channels[1].name, "G", 255);
        header.channels[1].name[strlen("G")] = '\0';
        strncpy(header.channels[2].name, "R", 255);
        header.channels[2].name[strlen("R")] = '\0';

        header.pixel_types = (int *)malloc(sizeof(int) * header.num_channels);
        header.requested_pixel_types = (int *)malloc(sizeof(int) * header.num_channels);
        for (int i = 0; i < header.num_channels; i++)
        {
            header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;          // pixel type of input image
            header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF; // pixel type of output image to be stored in .EXR
        }

        const char *err = NULL; // or nullptr in C++11 or later.
        int ret = SaveEXRImageToFile(&image, &header, outfilename, &err);
        if (ret != TINYEXR_SUCCESS)
        {
            fprintf(stderr, "Save EXR err: %s\n", err);
            FreeEXRErrorMessage(err); // free's buffer for an error message
            return ret;
        }
        printf("Saved exr file. [ %s ] \n", outfilename);

        // free(rgb);

        free(header.channels);
        free(header.pixel_types);
        free(header.requested_pixel_types);
    }

    static TMOData ReadExr(std::string file)
    {
        TMOData result;
        const char *input = file.c_str();
        float *out; // width * height * RGBA
        int width;
        int height;
        const char *err = NULL; // or nullptr in C++11
        


        int ret = LoadEXR(&out, &width, &height, input, &err);

        

        if (ret != TINYEXR_SUCCESS)
        {
            if (err)
            {
                fprintf(stderr, "ERR : %s\n", err);
                FreeEXRErrorMessage(err); // release memory of error message.
            }
        }
        else
        {
            ;
        }
        result.data = out;
        result.width = width;
        result.height = height;

        return result;
    }
};

}
#endif