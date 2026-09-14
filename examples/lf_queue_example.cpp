#include <chrono>
#include <iostream>
#include <thread>
#include "lock_free_queue.h"

struct theStruct {

    int a;
    int b;
    int c;

};

void produce(LockFreeQueue<theStruct> *lf, theStruct structToWrite, size_t *nextToWrite) {

    
    for(int i = 0; i < 50; i++) {

        structToWrite.a = 10 * i;
        structToWrite.b = 100 * i;
        structToWrite.c = 1000 * i;

        *(lf -> getNextSlotToWrite()) = structToWrite;
        lf -> updateWriteIndex();

        *nextToWrite = lf -> nextIndexToWrite;

        std::cout << "Wrote an element. Size: " << lf -> size() << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    
    }

}

void consume(LockFreeQueue<theStruct> *lf, size_t *nextToRead) {

    while(1) {

        if(lf -> size() > 0) {

            theStruct *read = (lf -> getNextSlotToRead());
            lf -> updateReadIndex();

            if(read == nullptr) 
                continue;

            if(lf -> size() > 0) {

                *nextToRead = lf -> nextIndexToRead; 

                std::cout << "Read an element. Size: " << lf -> size() << std::endl; 
                std::cout << " a: " << read -> a << std::endl;

            }

            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            
        }

    }

}

void printer(LockFreeQueue<theStruct> *lf) {}

size_t nextToRead = {0}, nextToWrite = {0};

int main() {

    theStruct structure = {10, 100, 1000};

    LockFreeQueue<theStruct> lf(10);

    std::thread producerThread(produce, &lf, structure, &nextToWrite);
    std::thread consumerThread(consume, &lf, &nextToRead);

    producerThread.join();
    consumerThread.join();

    return 0;

}