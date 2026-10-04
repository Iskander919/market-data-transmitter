#include "mem_pool.h"
#include <chrono>
#include <thread>

int main() {

    MemoryPool<int> mp(8);

    mp.allocate(5);
    int *p2 = mp.allocate(6);

    mp.deallocate(p2);

    mp.printVector();

    return 0;
    
}