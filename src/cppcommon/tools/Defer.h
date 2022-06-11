#pragma once

#include "tools/cpp_common.h"

class Defer {
public:
    Defer(function<void()> cb)
        : mCb(cb)
    {
    }

    ~Defer()
    {
        if (mCb) {
            mCb();
        }
    }

private:
    function<void()> mCb;
};
