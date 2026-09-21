#pragma once

// #define DEBUG_MODE

#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#include <atomic>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>
#include <thread>

#include "lock_free_queue.h"

constexpr size_t LOG_QUEUE_SIZE = 8 * 8 * 1024;

// this enumeration represents
// variants of data that can be stored in LogElement
// structure
enum class LogType : int8_t {

    CHAR                       = 0,
    INTEGER                    = 1,
    LONG_INTEGER               = 2,
    LONG_LONG_INTEGER          = 3,
    UNISIGNED_INTEGER          = 4,
    UNSIGNED_LONG_INTEGER      = 5,
    UNSIGNED_LONG_LONG_INTEGER = 6,
    FLOAT                      = 7,
    DOUBLE                     = 8
};

// this structure represenets 
// the contents of the single logging element.
// This struct is pushed to the lock-free queue.
struct LogElement {

    LogType type = LogType::CHAR;

    union {

        char                   chr;
        int                    integer;
        long int               l_integer;
        long long int          ll_integer;
        unsigned int           u_integer;
        unsigned long int      ul_integer;
        unsigned long long int ull_integer;
        float                  floa;
        double                 doub;

    } typeUnion;

};

// we will use lock-free queue to 
// transfer logging information 
// from performance-critical section to logger section
class Logger final {
public:

    /**
     * @brief function that reads a LogElement from the 
     *        queue and passes it to the file
     * @param none 
     * @return none
     */
    void flushQueue() noexcept {

        #ifdef DEBUG_MODE
        std::cout << "Entered flushQueue(). isRunning = " << _isRunning << std::endl;
        #endif

        // тут лучше сделать через condition variable, а не busy loop
        while(_isRunning || _queue.size()) {

            std::string text = "";

            // std::cout << "Entered while(_isRunning)" << std::endl;
            // reading log elements from the queue
            for(LogElement* next = _queue.getNextSlotToRead(); 
            next != nullptr && _queue.size();
            next = _queue.getNextSlotToRead()) {

                // defining the type of the log element and 
                // writing it to the logs file
                switch (next -> type) {

                    case LogType::CHAR:

                        file << next -> typeUnion.chr;
                
                    break;

                    case LogType::INTEGER:

                        file << next -> typeUnion.integer;

                    break;

                    case LogType::LONG_INTEGER:

                        file << next -> typeUnion.l_integer;

                    break;

                    case LogType::LONG_LONG_INTEGER:

                        file << next -> typeUnion.ll_integer;
                    
                    break;

                    case LogType::UNISIGNED_INTEGER:

                        file << next -> typeUnion.u_integer;

                    break;

                    case LogType::UNSIGNED_LONG_INTEGER:

                        file << next -> typeUnion.ul_integer;

                    break;

                    case LogType::UNSIGNED_LONG_LONG_INTEGER:

                        file << next -> typeUnion.ull_integer;

                    break;

                    case LogType::DOUBLE:

                        file << next -> typeUnion.doub;

                    break;

                    case LogType::FLOAT:

                        file << next -> typeUnion.floa;
                    
                    break;

                }

                file.flush();

                _queue.updateReadIndex();

            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        }

        #ifdef DEBUG_MODE
        std::cout << "Exit flushQueue()" << std::endl;
        #endif

    }

    /**
     * @brief constructor of Logger class 
     * @param fileName
     * @return none
     */
    explicit Logger(const std::string &fileName) : _fileName(fileName), _queue(LOG_QUEUE_SIZE) {

        // opening the file where we write logs
        file.open(fileName);

        // checking if we were able to open the file
        if(!file.is_open()) {

            throw std::runtime_error("Failed to open log file");

        }

        #ifdef DEBUG_MODE
        std::cout << std::filesystem::current_path() << std::endl;
        std::cout << fileName << std::endl; 
        std::cout << "is open: " << file.is_open() << std::endl;
        std::cerr << "bad: " << file.bad() << std::endl;
        std::cerr << "fail: " << file.fail() << std::endl; 
        #endif

        _isRunning.store(true);

        #ifdef DEBUG_MODE
        std::cout << "isRunning (constructor)" << _isRunning << std::endl;
        #endif

    } 

    /**
     * @brief function that creates and starts logger thread
     * @param none
     * @return none
     */
    void startLoggerThread() noexcept {

        #ifdef DEBUG_MODE
        std::cout << "START LOGGER: this = " << this
              << ", running = " << _isRunning.load()
              << '\n';
        #endif

        // creating and starting the logger 
        // thread. The thread will flush the queue
        _logThread = std::thread([this](){ 

            #ifdef DEBUG_MODE
            std::cout << "THREAD: this = " << this
                  << ", running = " << _isRunning.load()
                  << '\n';
            #endif

            flushQueue();

        });

    }

    /**
     * @brief function that stops logger thread by setting _isRunning value 
     *        as false 
     * @param none
     * @return none
     */
    void stopLoggerThread() noexcept {

        _isRunning =  false;

    }

    ~Logger() {

        #ifdef DEBUG_MODE
        std::cout << "DESTRUCTOR: this = " << this
              << ", running = " << _isRunning.load()
              << '\n';
        #endif

        _isRunning = false;

        // we should wait until the thread is
        // done executing before destroying the Logger
        // object
        _logThread.join();

        file.close();

    }

    /**
     * @brief function that writes log element to the queue
     * @param none 
     * @return none 
     */
    void pushValue(const LogElement &logElement) noexcept {

        LogElement *writeSlot = _queue.getNextSlotToWrite();
        *writeSlot = logElement;
        _queue.updateWriteIndex();
        // debug: std::cout << "Pushed value: " << logElement.typeUnion.chr << std::endl;
        // debug: std::cout << "Queue value: " << (_queue.getNextSlotToRead()) -> typeUnion.chr << std::endl;

    }

    // overload pushing function by types
    void pushValue(const char value) noexcept {

        pushValue(LogElement{LogType::CHAR, {.chr = value}});

    }

    void pushValue(const int value) noexcept {

        pushValue(LogElement{LogType::INTEGER, {.integer = value}});

    }

    void pushValue(const long int value) noexcept {

        pushValue(LogElement{LogType::LONG_INTEGER, {.l_integer = value}});

    }

    void pushValue(const long long int value) noexcept {

        pushValue(LogElement{LogType::LONG_LONG_INTEGER, {.ll_integer = value}});

    }

    void pushValue(const unsigned int value) noexcept {

        pushValue(LogElement{LogType::UNISIGNED_INTEGER, {.u_integer = value}});

    }

    void pushValue(const unsigned long int value) noexcept {

        pushValue(LogElement{LogType::UNSIGNED_LONG_INTEGER, {.ul_integer = value}});

    }

    void pushValue(const unsigned long long int value) noexcept {

        pushValue(LogElement{LogType::UNSIGNED_LONG_LONG_INTEGER, {.ull_integer = value}});

    }

    void pushValue(const double value) noexcept {

        pushValue(LogElement{LogType::DOUBLE, {.doub = value}});

    }

    void pushValue(const float value) noexcept {

        pushValue(LogElement{LogType::FLOAT, {.floa = value}});

    }


    /**
     * @brief function that accepts string and some other args
     *        of different data types, formats the log string
     *        and pushes it to the log file
     * @param *s is string that needs to be formatted
     * @param T
     * @return 
     */
    template <typename T, typename ... A>
    void log(const char *s, T& value, A ... args) {

        while(*s) {

            if(*s == '%') {

                // if '%%' is detected, we skip one symbol
                if (UNLIKELY(*(s + 1) == '%')) 
                    ++s;

                else {

                    // we substitute '%' with the T value from the arguments
                    pushValue(value);
                    log(s + 1, args...);
                    return;

                }
                
            }

            pushValue(*s++);

        }

    }

    /**
     * @brief high-level function that pushes the 
     *        logElement object to the queue
     * @param s is the string that will be written to the file
     * @param args
     * @return none
     */
    void log(const char *s) {

        while(*s) {

            if(*s == '%') {

                if (*(s + 1) == '%')
                    ++s;

            }

            pushValue(*s++);

        }

    }

    /**
     * @brief
     * @param 
     * @return
     */
    static std::string getCurrentTimeStr() {

        const time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        return "0";
    }

    Logger()                = delete;
    Logger(const Logger &)  = delete;
    Logger(const Logger &&) = delete;
    Logger& operator = (const Logger &)  = delete;
    Logger& operator = (const Logger &&) = delete; 

private:
    std::thread _logThread;
    const std::string _fileName = {""};
    std::ofstream file;
    std::atomic<bool> _isRunning{false};
    LockFreeQueue<LogElement> _queue;
    
}; 