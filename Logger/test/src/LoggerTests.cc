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
#include <eraftio/eraftpb.pb.h>

TEST(LoggerTests, MakeLogFileName) {
    std::shared_ptr<Logger> testLog (new Logger);
    testLog->Init(logDEBUG, logConsole, "logout"); 
    std::shared_ptr<eraftpb::Entry> e (new eraftpb::Entry);
    e->set_term(1);
    e->set_index(1);
    std::cout << testLog->MakeFileName() << std::endl;
}
