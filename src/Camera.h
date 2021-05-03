#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "acmath.h"
#include "Random.h"

namespace actracer {

union Color;

// Structure for holding variables related to the image plane
typedef struct ImagePlane
{
    float left;     // "u" coordinate of the left edge
    float right;    // "u" coordinate of the right edge
    float bottom;   // "v" coordinate of the bottom edge
    float top;      // "v" coordinate of the top edge
    float distance; // distance to the camera (always positive)
    int nx;         // number of pixel columns
    int ny;         // number of pixel rows
} ImagePlane;

typedef struct Pixel {
    int row;
    int col;

    BoundingVolume2f pixelBox;
    // Precalculated values
    float calcHorzOffset;
    float calcVertOffset;
} Pixel;

typedef struct PixelSample {
    Vector3f resultingColor;
    Vector2f positionRelativeToCenter;
} PixelSample;

class Camera
{
private:
    int m_Id; // Camera id in the scene

    Vector3f m_Pos;   // Camera position
    Vector3f m_Gaze;  // Gaze of the camera ( negative z axis)
    Vector3f m_Up;    // Y axis of the camera
    Vector3f m_Right; // X axis of the camera

    bool isOrthographic; // Camera type ( Either orthographic or projective )

    int nSamples; // Number of samples

    int    nSamplesSqrt; 
    float  sampleHorzDiff; // Horizontal and vertical difference
    float  sampleVertDiff; // in pixel sample sections

    float focusDistance; // Focal distance
    float apertureSize; // Aperture size

    mutable Random<double> camRandom;
    mutable Random<double> rayRandomX;
    mutable Random<double> rayRandomY;

    unsigned camSeed;
    std::default_random_engine camGenerator;
    std::uniform_real_distribution<double> camDistribution;

public:
    static void DeriveBoundaries(float fov, float a, const Vector3f &gazePoint, const Vector3f &pos, float d, float &l, float &r, float &b, float &t, Vector3f &camGaze);
public:
    char imageName[64]; // Target file name
    int id;
    ImagePlane imgPlane; // Image plane

    Camera(int id, const char *imageName, const Vector3f &pos, const Vector3f &gaze, const Vector3f &up, const ImagePlane &imgPlane, int numberOfSamples = 1, float focalDistance = 0.0f, float apertSize = 0.0f);

    Ray GenerateRay(int row, int col) const; // Generates the ray going through pixel (row, col)
    Ray GenerateRayFromPixelSample(const Pixel& px, int id, PixelSample& pxs) const; // Generate pixel sample from pixel with given id
    Pixel GenerateSampleForPixel(int row, int col) const; // Generates a pixel

    bool IsMultiSamplingOn() const;
public:
    int GetID() const;
    int GetSampleCount() const;
    const char* GetImageName() const;
private:
    float CalculateHorizontalOffsetOnImagePlane(int col) const;
    float CalculateVerticalOffsetOnImagePlane(int row) const;

    void SetImageName(const char *imageName);
    void SetupCameraCoordinateAxes();
};

inline int Camera::GetID() const 
{
    return m_Id;
}

inline int Camera::GetSampleCount() const
{ 
     return nSamples; 
}

inline const char* Camera::GetImageName() const
{
    return imageName;
}

inline bool Camera::IsMultiSamplingOn() const
{
    return nSamples > 1;
}

}
#endif
