#ifndef _ACCELERATION_H_
#define _ACCELERATION_H_

#include <vector>
#include <iostream>
#include "acmath.h"
#include "Primitive.h"

namespace actracer {

class Primitive;
class SurfaceIntersection;

class BVHTree {
private:
    struct BVHNode {
        BVHNode* left; // Left child
        BVHNode* right; // Right child
        BoundingVolume3f bbox; // Combined bounding box of the node
        int axis; // The axis the volume is split upon
        int startIndex, endIndex; // The interval of the primitives contained in this branch

        // Fill the info for a leaf node
        void Leaf(int ax, int start, int end, const BoundingVolume3f& box)
        {
            axis = ax;
            startIndex = start;
            endIndex = end;

            left = right = nullptr;

            bbox = box;
        }
        // Fill the info for an internal node
        void Internal(int ax, int start, int end, BVHNode* l, BVHNode* r)
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
    const int maxPrimitiveCount; // Maximum number of primitives that may be contained in a leaf node
    static constexpr int partitionCount = 4;    // Number of partitions that will be used to divide the box 

    std::vector<Primitive* > primitives;
private:
    BVHNode* BuildTree(int start, int end, BVHNode*);
    
public:
    BVHNode *root;

    BVHTree(int mpc, int pc, const std::vector<Primitive*>& prims)
        : maxPrimitiveCount(mpc), primitives(prims) 
    {
        if(prims.size() == 0)
            return;

        root = new BVHNode{};
        BuildTree(0, prims.size(), root);
    }

    void Intersect(BVHNode* head, Ray& r, SurfaceIntersection& rt);
};

}
#endif