#ifndef _ACCELERATION_H_
#define _ACCELERATION_H_

#include <vector>
#include <iostream>

#include "AccelerationStructure.h"
#include "acmath.h"

namespace actracer {

class Primitive;
class SurfaceIntersection;

class BVHTree : public AccelerationStructure {
private:
    struct BVHNode {
        BVHNode* left; // Left child
        BVHNode* right; // Right child
        BoundingVolume3f bbox; // Combined bounding box of the node
        int axis; // The axis the volume is split upon, can be [0,1,2]
        int startIndex, endIndex; // The interval of the primitives contained in this branch

        bool IsLeaf() const
        {
            return left == nullptr;
        }

        // Fill the info for a leaf node
        void BuildLeaf(int ax, int start, int end, const BoundingVolume3f& box)
        {
            axis = ax;
            startIndex = start;
            endIndex = end;

            left = right = nullptr;

            bbox = box;
        }
        // Fill the info for an internal node
        void BuildInternal(int ax, int start, int end, BVHNode* l, BVHNode* r)
        {
            axis = ax;
            startIndex = start;
            endIndex = end;

            left = l;
            right = r;

            bbox = Merge(l->bbox, r->bbox);
        }
    };


private:
    const int maxPrimitiveCountInLeaf;
    static constexpr int partitionCount = 4; // Number of partitions that will be used to divide the box 

    std::vector<Primitive* > primitives;
private:
    BVHNode* BuildTree(int start, int end, BVHNode*);
    void Clear(BVHNode *head);
    
public:
    virtual void Intersect(Ray &cameraRay, SurfaceIntersection &intersectedSurfaceInformation, float intersectionTestEpsilon) const override;

    BVHTree(int mpc, int pc, const std::vector<Primitive*>& prims);
    ~BVHTree();

private:
    BVHNode *root;
    void IntersectThroughHierarchy(BVHNode *head, Ray &cameraRay, SurfaceIntersection &intersectedSurfaceInformation, float intersectionTestEpsilon) const;
    // No need for polymorphic node structure, only two exists
    void ProcessIntersectionForLeafNode(const BVHNode *head, Ray &r, SurfaceIntersection &rt, float intersectionTestEpsilon) const;
    void ProcessIntersectionForInternalNode(const BVHNode *head, Ray &r, SurfaceIntersection &rt, float intersectionTestEpsilon) const;
};

}
#endif