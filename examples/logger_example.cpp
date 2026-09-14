#include <iostream>
#include "logger.h"

int main() {

    std::string filename = "log.txt";
    Logger logger(filename);

    logger.startLoggerThread();

    logger.log("Some log\n");
    logger.log("Another log\n");
    int num = 124;
    logger.log("Logged a num: %\n", num);

    return 0;

}