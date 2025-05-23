#include "log/Log.h"
#include "osinfo/CpuInfo.h"
#include "osinfo/ProcessInfo.h"
#include "osinfo/ThreadInfo.h"
#include "gtest/gtest.h"
#include <memory>
#include <mutex>

class OsInfoTest : public testing::Test {
protected:
    // Code here will be called immediately after the constructor (right before each test)
    void SetUp()
    {
        INITLOG();
        INFOLN("SetUp");
    }

    // Code here will be called immediately after each test (right before the destructor)
    void TearDown()
    {
        INFOLN("TearDown");
    }
};

TEST_F(OsInfoTest, cpuCount)
{
    int num = CpuInfo::GetCpuCount();
    INFOLN("cpu num: {}", num);
    EXPECT_TRUE(true);
}

TEST_F(OsInfoTest, threadId)
{
    int tid = ThreadInfo::GetTid();
    INFOLN("thread id: {}", tid);
    EXPECT_TRUE(true);
}

TEST_F(OsInfoTest, process)
{
    int pid = ProcessInfo::GetPid();
    int ppid = ProcessInfo::GetParentPid();
    int gid = ProcessInfo::GetGid();
    int sid = ProcessInfo::GetSid();
    INFOLN("process id: {}, parent process id: {}, gid: {}, sid: {}", pid, ppid, gid, sid);
    EXPECT_TRUE(true);
}
