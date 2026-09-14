#include <string>
#include <thread>
#include <functional>

/**
 * @brief function that creates a thread and returns object of std::thread. We
 *        perfect forwarding with std::forward to transfer the thread function
 *        to the thread instance that we are currently constructing
 * @param func is function template that thread would run (rvalue reference)
 * @param args are the arguments of the function that thread would run (rvalue reference)
 * 
 */
template <typename T, typename... A>
std::thread startThread(T&& func, A&& ... args) {

    // creating the thread
    // std::thread &resThread = new 

}