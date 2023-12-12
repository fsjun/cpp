#include "threadpool/EventThread.h"
#include "threadpool/ThreadManager.h"
#include "gtest/gtest.h"
#include <iostream>
#include <memory>
#include <chrono>

using std::cout;
using std::make_shared;
using std::shared_ptr;

class ThreadTest : public testing::Test {
protected:
    // Code here will be called immediately after the constructor (right before each test)
    void SetUp()
    {
        cout << "SetUp\n";
    }

    // Code here will be called immediately after each test (right before the destructor)
    void TearDown()
    {
        cout << "TearDown\n";
    }

};

TEST_F(ThreadTest, threadPool)
{
    auto tm = ThreadManager::GetInstance();
    tm->start();
    auto et = make_shared<EventThread<shared_ptr<int>>>();
    et->start();
    shared_ptr<int> data = make_shared<int>(3);
    Event<shared_ptr<int>> event = {"id1", data};
    et->push(event);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    et->stop();
    tm->join();
    EXPECT_TRUE(true);
}

