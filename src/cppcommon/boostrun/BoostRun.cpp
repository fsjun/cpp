#include "BoostRun.h"
#include "osinfo/CpuInfo.h"
#include "osinfo/OsSignal.h"

using std::make_shared;

BoostRun::BoostRun()
{
    int threadCnt = CpuInfo::GetCpuCount();
    mIoContext = make_shared<asio::io_context>(threadCnt);
}

shared_ptr<asio::io_context> BoostRun::getIoContext()
{
    return mIoContext;
}

void BoostRun::run()
{
    boost::asio::io_context::work work(*mIoContext);
    OsSignal::Recover();
    mIoContext->run();
}

void BoostRun::stop()
{
    mIoContext->stop();
}
