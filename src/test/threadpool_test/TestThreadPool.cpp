#include "threadpool/StateThreadPool.h"
#include "threadpool/ThreadPool.h"
#include "gtest/gtest.h"
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <set>

using std::cout;
using std::endl;
using std::make_shared;
using std::set;
using std::shared_ptr;
using std::unique_ptr;

// 定义测试类FooTest
class ThreadPoolTest : public testing::Test {
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

public:
    unique_ptr<ThreadPool> threadPool;
    unique_ptr<StateThreadPool> stateThreadPool;
};

TEST_F(ThreadPoolTest, threadPool)
{
    int count = 0;
    bool result = false;
    while (count < 300000) {
        threadPool.reset(new ThreadPool(1000, 1000));
        result = false;
        while (!result) {
            threadPool->execute([&count, &result]() {
                cout << "test: " << ++count << endl;
                if (count > 1000) {
                    result = true;
                }
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            });
        }
        threadPool.reset();
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    EXPECT_TRUE(true);
}

TEST_F(ThreadPoolTest, stateThreadPool)
{
    int count = 0;
    bool result = false;
    set<string> s;
    while (count < 300000) {
        stateThreadPool.reset(new StateThreadPool(10, 10));
        result = false;
        while (!result) {
            int id = count % 3;
            string stateId = std::to_string(id);
            stateThreadPool->execute(stateId, [&s, &count, &result, stateId]() {
                cout << "test " << stateId << ": " << ++count << endl;
                s.insert(stateId);
                if (count > 1000) {
                    result = true;
                }
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            });
        }
        stateThreadPool.reset();
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    cout << "thread count: " << s.size() << endl;
    EXPECT_TRUE(s.size() == 3);
}
