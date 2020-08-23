#include <algorithm>
#include <thread>
#include <future>

#include "Acceleration.h"
#include "Primitive.h"

namespace actracer {

    BVHTree::BVHNode* BVHTree::BuildTree(int start, int end, BVHNode* currentNode)
    {
        int primCount = end - start; // Number of primitives that will reside under this node ( this node and its children )

        if(primCount == 1 || start == end) // No or one primitive exist
        {
            Vector3f center = (primitives[start]->bbox.max + primitives[start]->bbox.min) * 0.5f; // Get the mid point of the box
            currentNode->Leaf(MaxElementIndex(center), start, end, primitives[start]->bbox); // Build a leaf node

            return currentNode;
        }

        // Compute the smallest bounding box that
        // would envelop all of the primitives
        // that would be under this node 
        BoundingVolume3f combinedVolume{};
        BoundingVolume3f combinedCenter{}; // Largest extend of the centers
        for(int i = start; i < end; ++i)
        {
            combinedVolume = Merge(combinedVolume, primitives[i]->bbox); 
            combinedCenter = Merge(combinedCenter, (primitives[i]->bbox.max + primitives[i]->bbox.min) * 0.5f); 
        }

        int splitax = MaxElementIndex(combinedCenter.max - combinedCenter.min); // Longest axis will be chosen as primary axis

        if(std::abs(combinedCenter.max[splitax] - combinedCenter.min[splitax]) < 0.00000001f) // Difference between the max and min extend of the center is less than some amount
        {
            currentNode->Leaf(splitax, start, end, combinedVolume);
            return currentNode;
        }

        // Hold information about the partitions 
        // that would cut volume into parts
        struct PartitionInfo {
            BoundingVolume3f partitionBound{}; // Extent of the partition
            int count = 0; // Number of primitives in the partition

            PartitionInfo() = default;
            PartitionInfo(const BoundingVolume3f& b, int c) : partitionBound(b), count(c) { }
            PartitionInfo operator+(const PartitionInfo& p) { return PartitionInfo(Merge(partitionBound, p.partitionBound), count + p.count); }
        };

        PartitionInfo partitions[partitionCount]; // We will divide the bound in partition count little pieces and compare

        // Find primitive center in the bounds converted to [0 - 1] interval
        // Using this find the partition it fits into
        for(int i = start; i < end; ++i)
        {
            Vector3f v((primitives[i]->bbox.max + primitives[i]->bbox.min) * 0.5f); // Center of the primitive

            float cp = ClippedPosition(combinedCenter, v, splitax); // Clipped position

            int index = partitionCount * cp; // Between [0, partitionCount]
            if(index == partitionCount) --index;

            partitions[index].count++;
            partitions[index].partitionBound = Merge(partitions[index].partitionBound, primitives[i]->bbox);
        }

        float partitionCost[partitionCount];

        PartitionInfo forwardSum[partitionCount]; // Sums of infos such that [1, 1 + 2, 1 + 2 + 3, .. ]
        forwardSum[0] = partitions[0];
        for(int i = 1; i < partitionCount; ++i)
            forwardSum[i] = partitions[i] + forwardSum[i - 1];
 
        PartitionInfo backwardSum[partitionCount]; // Sums of infos in reverse order such that [sum(1 to n), sum(1 to n - 1), ..]
        backwardSum[partitionCount - 1] = partitions[partitionCount - 1];
        partitionCost[partitionCount - 1] = 0.125f + ( forwardSum[partitionCount - 1].count * forwardSum[partitionCount - 1].partitionBound.SA()) / combinedVolume.SA();
        for(int i = partitionCount - 2; i >= 0; --i)
        {
            backwardSum[i] = partitions[i] + backwardSum[i + 1];

            // Calculate the cost SAH
            partitionCost[i] = 0.125f + (forwardSum[i].count * forwardSum[i].partitionBound.SA() + backwardSum[i + 1].count * backwardSum[i + 1].partitionBound.SA()) / combinedVolume.SA();
        }

        // Using partition Cost array find the best partition index 
        float minCost = 1e9;
        int index = -1;
        for(int i = 0; i < partitionCount; ++i)
        {
            if(minCost > partitionCost[i])
            {
                minCost = partitionCost[i];
                index = i;
            }
        }

        int midIndex = -1;
        if(minCost < primCount) // Partition primitives such that the primitives stands left of the "indexth" partition will be put left in the array. Same for right.
        {
            Primitive** mid = std::partition(&primitives[start], &primitives[end - 1] + 1,
                           [=](Primitive* p0) {
                                Vector3f cen((p0->bbox.max + p0->bbox.min) * 0.5f); // center
                                int in = partitionCount * ClippedPosition(combinedCenter, cen, splitax);
                                return in <= index;    
                           });

            midIndex = mid - &primitives[start];
        }
        else
        {
            currentNode->Leaf(splitax, start, end, combinedVolume);
            return currentNode;
        }
        
        // Initialize children with default values
        BVHNode* l = new BVHNode{}; 
        BVHNode* r = new BVHNode{};

        BuildTree(start, start + midIndex, l);            //
        BuildTree(start + midIndex, end, r);              // Build left and right then combine
        currentNode->Internal(splitax, start, end, l, r); // 

        return currentNode;
    }


    void BVHTree::Intersect(BVHNode* head, Ray& r, SurfaceIntersection& rt)
    {
        if(head == nullptr)
            return;
        float tn = 0, tf;
        if(head->bbox.Intersect(r, tn, tf)) // Test if ray intersects with the bounding box
        {
            if(head->left == nullptr) // Leaf
            {
                // Select the closest one out of all primitives that are
                // kept in this bounding box
                for(int i = head->startIndex; i < head->endIndex; ++i) 
                {
                    SurfaceIntersection t{};
                    primitives[i]->Intersect(r, t);

                    if (t.mat && t.t > 0 && t.t < rt.t - 0.001f)
                        rt = t;
                }
            }
            else // Internal node
            {
                SurfaceIntersection li{};
                SurfaceIntersection ri{};

                Intersect(head->left, r, li);  // Check left node
                Intersect(head->right, r, ri); // Check right node

                // Pick the closest 
                if(li.mat && ri.mat) rt = li.t < ri.t ? li : ri;
                else if(li.mat) rt = li;
                else if(ri.mat) rt = ri;
            }
        }
    }
}