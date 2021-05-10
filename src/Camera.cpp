#include "Camera.h"

#include <string.h>
#include <iostream>

#include <cmath>
#include <chrono>
#include "Random.h"
#include "Scene.h"

#include "PixelSampler.h"

namespace actracer {

// Construct camera object with given parameters
Camera::Camera(int id, const char *imageName, const Vector3f &pos, const Vector3f &gaze, const Vector3f &up, const Camera::ImagePlane &imgPlane, int numberOfSamples, PixelSampleMethod sampleMethod, float focDis, float apertSize)
    : imgPlane(imgPlane), m_Id(id), m_Pos(pos), m_Gaze(gaze), m_Up(up), focusDistance(focDis), apertureSize(apertSize), nSamples(numberOfSamples)
{
    SetImageName(imageName);
    SetupCameraCoordinateAxes();

    mSinglePixelWidth = (imgPlane.right - imgPlane.left) / imgPlane.nx;
    mSinglePixelHeight = (imgPlane.top - imgPlane.bottom) / imgPlane.ny;

    mSampler = PixelSampler::CreatePixelSampler(sampleMethod);
    mSampler->Init(*this);

    camRandom = Random<double>{};
}

void Camera::SetImageName(const char* imageName)
{
    const char *c = imageName;

    char *f = this->imageName;

    while (*c != '\0')
    {
        if (*c != ' ')
            *(f++) = *(c++);
        else
            c++;
    }

    *f = '\0';
}

void Camera::SetupCameraCoordinateAxes()
{
    m_Gaze = Normalize(m_Gaze);
    m_Up = Normalize(m_Up);

    float correlation = Dot(m_Gaze, m_Up);
    if(AroundZero(correlation, 0.01f))
    {
        m_Up = Normalize(m_Up);
        m_Right = Normalize(Cross(m_Gaze, m_Up));
    }
    else
    {
        m_Right = Normalize(Cross(m_Gaze, m_Up));
        m_Up = Normalize(Cross(m_Right, m_Gaze));
    }
}

Camera::~Camera()
{
    if(mSampler)
        delete mSampler;
}

/*
 * Takes coordinate of an image pixel as row and col, and
 * returns the ray going through the center of that pixel.
 */
Ray Camera::GenerateRay(int row, int col) const
{
    return Ray(m_Pos, CalculateRayDirectionFor(row, col, 0.5f, 0.5f)); // No need for time no sampling motion blur would be sluggy
}

/*
 * Generates ray that shoots through sample on the pixel
 * Tilts ray if lens exists
 */ 
Ray Camera::GenerateRayForPixelSample(const Pixel &px, int sampleId) const
{
    std::pair<float, float> samplePositionOffset = (*mSampler)(px, sampleId); // Find the position of the ith partition sample (e.g 6x6 pixel's 20th)

    Vector3f rayDirection = CalculateRayDirectionFor(px.row, px.col, samplePositionOffset.first, samplePositionOffset.second);
    Ray cameraRay(m_Pos, rayDirection); // Standard ray ( without any tilt by lens )

    ApplyLensTilt(cameraRay);

    return cameraRay; 
}

/*
 * Moves mGaze * imgPlane.distance on z-axis
 * Moves mRight * horizontalMove on x-axis
 * Moves mUp * verticalMove on y-axis
 * Normalize results in a direction vector from Camera center to a pixel on ImagePlane
 */ 
Vector3f Camera::CalculateRayDirectionFor(int row, int col, float horizontalOffset, float verticalOffset) const
{
    float horizontalMove = CalculateHorizontalPositionOnImagePlane(col, horizontalOffset);
    float verticalMove = CalculateVerticalPositionOnImagePlane(row, verticalOffset);

    return Normalize(m_Gaze * (imgPlane.distance) + m_Right * horizontalMove + m_Up * verticalMove);
}

/*
 * Moves (col + 0.5f) pixels from left-most position of the plane to right
 */
float Camera::CalculateHorizontalPositionOnImagePlane(int col, float horizontalOffset) const
{
    return imgPlane.left + (col + horizontalOffset) * mSinglePixelWidth;
}

/*
 * Moves (row + 0.5f) pixels from top position of the plane to bottom
 */
float Camera::CalculateVerticalPositionOnImagePlane(int row, float verticalOffset) const
{
    return imgPlane.top - (row + verticalOffset) * mSinglePixelHeight;
}

/*
 * We first randomly move ray start direction in both Right and Up axes,
 * then for destination, we calculate a point which is focus distance away 
 * from original ray. With this two points we construct the new tilted Ray
 */ 
void Camera::ApplyLensTilt(Ray &standardRay) const
{
    if(apertureSize > 0)
    {
        float arandX = camRandom(-apertureSize / 2.0, apertureSize / 2.0);
        float arandY = camRandom(-apertureSize / 2.0, apertureSize / 2.0);

        Vector3f movedPointInAperture = m_Pos + m_Right * arandX + m_Up * arandY;

        float td = focusDistance / (Dot(standardRay.d, m_Gaze));      // t value to reach focal distance with primary ray
        Vector3f p = standardRay(td);  // Hit point on the plane at focal distance from camera
        Vector3f tiltedDirection = Normalize(p - movedPointInAperture);

        standardRay = Ray(movedPointInAperture, tiltedDirection); // Tilted ray
    }
}

/*
 * normalizedPixelVolume refers to Pixel box left-bottom point is (0,0) right-top point is (1,1)
 */ 
Pixel Camera::GeneratePixelDataAt(int row, int col) const
{
    BoundingVolume2f normalizedPixelVolume(Vector2f(0.0f, 0.0f), Vector2f(1.0f, 1.0f));

    return Pixel{row, col, normalizedPixelVolume};
}

// Given different camera construct the image plane and essential attributes for the camera
// using given values
void Camera::DeriveBoundaries(float fov, float a, const Vector3f& gazePoint, const Vector3f& pos, float d, float &l, float &r, float &b, float &t, Vector3f &camGaze)
{
    camGaze = Normalize(gazePoint - pos); // Derive camera gaze

    // Given FoV derive the image plane
    t = d * tan((fov / 2) * (PI / 180.0f));
    b = -t;

    r = a * t;
    l = -r;
    // --
}

}