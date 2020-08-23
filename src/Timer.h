#ifndef _TIMER_H_
#define _TIMER_H_

#include <chrono>
#include <iostream>
#include <string>

namespace actracer {
class Timer {
private:
    std::chrono::high_resolution_clock::time_point startingTime;
    int blockID;
    std::string blockName;
public:
    Timer(std::string name = "", int id = -1) : blockName(name), blockID(id) { startingTime = std::chrono::high_resolution_clock::now(); }
    
    ~Timer() 
    {
        std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

        if(blockName.size() > 0)
        {
            std::cout << "Block: " << blockName << " completed execution in "
                      << std::chrono::duration_cast<std::chrono::microseconds>(end - startingTime).count() / 1000.0f << " ms\n";
        }
        else
        {
            std::cout << "Block: " << blockID << " completed execution in "
                      << std::chrono::duration_cast<std::chrono::microseconds>(end - startingTime).count() / 1000.0f << " ms\n";
        }
        
    }

};

}

#endif