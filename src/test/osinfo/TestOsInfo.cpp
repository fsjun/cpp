#include "gtest/gtest.h"
#include <memory>
#include <mutex>
#include "osinfo/CpuInfo.h"
#include "osinfo/ThreadInfo.h"
#include "osinfo/ProcessInfo.h"
#include "log/Log.h"

class OsInfoTest : public testing::Test {
protected:
    // Code here will be called immediately after the constructor (right before each test)
    void SetUp() {
        Log::Init(LOG_LEVEL_INFO);
        INFO("SetUp\n");
    }

    // Code here will be called immediately after each test (right before the destructor)
    void TearDown() {
        INFO("TearDown\n");
    }
};

TEST_F(OsInfoTest, cpuCount) {
    int num = CpuInfo::GetCpuCount();
    INFO("cpu num: %d\n", num);
    EXPECT_TRUE(true);
}

TEST_F(OsInfoTest, threadId) {
    int tid = ThreadInfo::GetTid();
    INFO("thread id: %d\n", tid);
    EXPECT_TRUE(true);
}

TEST_F(OsInfoTest, process) {
    int pid = ProcessInfo::GetPid();
    int ppid = ProcessInfo::GetParentPid();
    int gid = ProcessInfo::GetGid();
    int sid = ProcessInfo::GetSid();
    INFO("process id: %d, parent process id: %d, gid: %d, sid: %d\n", pid, ppid, gid, sid);
    EXPECT_TRUE(true);
}
