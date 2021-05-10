#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "acmath.h"
#include "Random.h"
#include "PixelSampler.h"

namespace actracer {

class PixelSampler;
union Color;

typedef struct Pixel
{
    int row;
    int col;
    BoundingVolume2f pixelBox;
} Pixel;

class Camera
{
public:
    
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

public:
    static void DeriveBoundaries(float fov, float a, const Vector3f &gazePoint, const Vector3f &pos, float d, float &l, float &r, float &b, float &t, Vector3f &camGaze);
public:
    ImagePlane imgPlane;

    Camera(int id, const char *imageName, const Vector3f &pos, const Vector3f &gaze, const Vector3f &up, const Camera::ImagePlane &imgPlane, int numberOfSamples = 1, PixelSampleMethod sampleMethod = PixelSampleMethod::JITTERED, float focalDistance = 0.0f, float apertSize = 0.0f);
    ~Camera();

    Ray GenerateRay(int row, int col) const;
    /*
     * Generates Ray from ith sample of the Pixel
     */
    Ray GenerateRayForPixelSample(const Pixel &px, int sampleId) const;
    Pixel GeneratePixelDataAt(int row, int col) const;

    bool IsMultiSamplingOn() const;
public:
    int GetID() const;
    int GetSampleCount() const;
    const char* GetImageName() const;
private:
    Vector3f CalculateRayDirectionFor(int row, int col, float horizontalOffset, float verticalOffset) const;
    float CalculateHorizontalPositionOnImagePlane(int col, float horizontalOffset) const;
    float CalculateVerticalPositionOnImagePlane(int row, float verticalOffset) const;

    void ApplyLensTilt(Ray& standardRay) const;
private:
    void SetImageName(const char *imageName);
    void SetupCameraCoordinateAxes();
private:
    float mSinglePixelWidth;
    float mSinglePixelHeight;

private:
    char imageName[64]; // Target file name
    int m_Id;           // Camera id in the scene

    Vector3f m_Pos;   // Camera position
    Vector3f m_Gaze;  // Gaze of the camera ( negative z axis)
    Vector3f m_Up;    // Y axis of the camera
    Vector3f m_Right; // X axis of the camera

    int nSamples;

    float focusDistance;
    float apertureSize;

    mutable Random<double> camRandom;
    PixelSampler *mSampler;
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
