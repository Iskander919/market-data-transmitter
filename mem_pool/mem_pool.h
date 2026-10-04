#include <cstdint>
#include <iostream>
#include <vector>
#include <string>

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

/**
 * @brief this template class implements the pre-allocated memory pool of
 *        user objects.
 *        It provides the allocate(), deallocate() methods
 */
template <typename T>
class MemoryPool final {

public:

    MemoryPool(const MemoryPool&)              = delete;
    MemoryPool &operator = (const MemoryPool&) = delete;
    MemoryPool (MemoryPool&&)                  = delete;
    MemoryPool &operator = (MemoryPool&&)      = delete;


    /**
     * @brief constructor of the memory pool storage. Here we 
     *        allocate the vector where we store data
     * @param numElements is the size of memory pool vector
     * @return 
     */
    explicit MemoryPool(std::size_t numElements) : _storageVector(numElements, {T(), true}) { }

    /**
     * @brief function that serves new allocation requests
     * @param args
     * @return pointer to the allocated object
     */
    template <typename ... Args>
    T* allocate(Args && ... args) noexcept {

        // getting pointer to the next free slot in the object vector 
        // (pointer to the insatnce of the ObjecBlock)
        auto objBlock = &(_storageVector[_nextFreeIndex]); 

        T* ret = &(objBlock -> _object);

        // using placement new 
        ret = new(ret)T(args...);

        objBlock -> _isFree = false;

        updateNextFreeIndex();

        return ret;

    }

    /**
     * @brief 
     * @param 
     * @return 
     */
    void deallocate(const T* elem) {

        // getting the index in the vector
        // by which we want to deallocate memory
        const auto elemIndex = reinterpret_cast<const ObjectBlock *>(elem) - &(_storageVector[0]);

        // checking if the element belongs to this memory pool
        if (!((elemIndex >= 0) && static_cast<size_t>(elemIndex) < _storageVector.size())) {

            std::cerr << "Element index is out of range of _storageVector" << std::endl;
            return;

        }

        if(_storageVector[elemIndex]._isFree == true) {

            std::cerr << "Element for deallocation is expected to be in use" << std::endl;
            return;

        }

        _storageVector[elemIndex]._isFree = true;

    }

    /**
     * @brief fucntion that prints _storageVector. 
     * @warning << operator must be overloaded for T
     * @param none
     * @return none
     */
    void printVector() {
        static std::size_t previousLines = 0;

        if (previousLines > 0)
            std::cout << "\033[" << previousLines << "A";

        for (std::size_t i = 0; i < _storageVector.size(); ++i) {

            const auto& element = _storageVector[i];

            std::cout << "\033[2K"
                      << "[" << i << "] "
                      << element._object
                    << " | "
                    << std::boolalpha << element._isFree
                    << '\n';
        }

        if (previousLines > _storageVector.size()) {
            for (std::size_t i = _storageVector.size(); i < previousLines; ++i)
                std::cout << "\033[2K\n";
        }

        previousLines = _storageVector.size();
        std::cout.flush();
        
}

private:

    /**
     * @brief updater of the free index value 
     * @param none
     * @return none 
     */ 
    void updateNextFreeIndex() noexcept {

        const std::size_t initialFreeIndex = _nextFreeIndex;

        // searching for the next free index
        while(!_storageVector[_nextFreeIndex]._isFree) {

            ++_nextFreeIndex;

            if(UNLIKELY(_nextFreeIndex == _storageVector.size())) {

                _nextFreeIndex = 0;

            }

            if (UNLIKELY(_nextFreeIndex == initialFreeIndex)) {

                std::cerr << "Memory Pool is out of space" << std::endl;
                return;

            }

        }

    }

    /**
     * @brief this struct represents one element of the vector
     */
    struct ObjectBlock {

        T _object;
        bool _isFree;

    };

    std::vector<ObjectBlock> _storageVector;

    // tracker of the next free block in the vector
    std::size_t _nextFreeIndex = 0;

};