// we build the lock-free queue here.
// this queue has type of SPSC (single consumer single producer)
// so only one thread can write to it and only one thread can consume from it.

#pragma once

#include <atomic>
#include <vector>


template <typename T>
class LockFreeQueue final {
public:
    LockFreeQueue(size_t size) : store(size, T()) { }

    // deleting move, copy and default constructor
    LockFreeQueue()                                  = delete;
    LockFreeQueue(const LockFreeQueue&)              = delete;
    LockFreeQueue(const LockFreeQueue&&)             = delete;
    LockFreeQueue &operator =(const LockFreeQueue&)  = delete;
    LockFreeQueue &operator =(const LockFreeQueue&&) = delete;

    /**
     * @brief function that returns a refernce to the next slot available to write to
     * @param none
     * @return &store[nextIndexToWrite]
     */
    T* getNextSlotToWrite() noexcept {

        return &store[nextIndexToWrite];

    }

    /**
     * @brief function that increases number of stored elements and calculates the next 
     *        available index to write
     * @param none
     * @return none
     */
    void updateWriteIndex() noexcept {

        nextIndexToWrite = (nextIndexToWrite + 1) % store.size();
        if(numElements >= store.size()) {

            numElements = store.size();
            return;
        }

        numElements++;

    }

    /**
     * @brief function that returns a pointer to the next available slot to read from
     * @param none
     * @return &store[nextIndexToRead])
     */
    T* getNextSlotToRead() noexcept {

        return (nextIndexToRead == nextIndexToWrite ? nullptr : &store[nextIndexToRead]);

    }

    /**
     * @brief function that updates read index
     * @param none
     * @return none 
     */
    void updateReadIndex() {

        nextIndexToRead = (nextIndexToRead + 1) % store.size();
        if(numElements < 0) {

            numElements = 0;
            return;

        }

        else {

            numElements--;

        }

    }

    size_t size() const noexcept {

        return this -> numElements;

    }    

    // next index of the free slot of the store vector.
    // It is possible to write new data at this index.
    std::atomic<size_t> nextIndexToWrite = {0};

    // 
    std::atomic<size_t> nextIndexToRead = {0};

private:

    // this vector is used to store elements of the queue
    std::vector<T> store;
    
    //
    std::atomic<size_t> numElements = {0};

};