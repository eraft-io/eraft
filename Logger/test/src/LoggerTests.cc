/**
 * @file LoggerTests.cpp
 *
 * This module contains the unit tests of the Logger::Logger class.
 *
 * © 2020 by LiuJ
 */

#include <gtest/gtest.h>
#include <Logger/Logger.h>
#include <iostream>
#include <memory>

TEST(LoggerTests, MakeLogFileName) {
    std::shared_ptr<Logger> testLog (new Logger);
    testLog->Init(logDEBUG, logConsole, "logout"); 
    std::cout << testLog->MakeFileName() << std::endl;
}
