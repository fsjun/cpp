#pragma once

#include <typeinfo>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace MemTrack
{
    class MemStamp
    {
        public:
            char const * const filename;
            int const lineNum;
        public:
            MemStamp(char const *filename, int lineNum)
                : filename(filename), lineNum(lineNum) {}
            ~MemStamp() {}
    };
    class MemFree
    {
        public:
            char const * const filename;
            int const lineNum;
        public:
            MemFree(char const *filename, int lineNum)
                : filename(filename), lineNum(lineNum) {}
            ~MemFree() {}
    };
			
	void TrackStamp(void *p, const MemStamp &stamp, char const *typeName);
    void TrackFree(void *p, const MemFree &stamp, char const *typeName);

    template <class T> inline T *operator*(const MemStamp &stamp, T *p)
    {
        TrackStamp(p, stamp, typeid(T).name());
        return p;
    }
    
    template <class T> inline T *operator*(const MemFree &stamp, T *p)
    {
        TrackFree(p, stamp, typeid(T).name());
        delete p;
    }
}

#define MEMTRACK_NEW MemTrack::MemStamp(__FILE__, __LINE__) * new
#define new MEMTRACK_NEW

//#define MEMTRACK_DELETE MemTrack::MemFree(__FILE__, __LINE__) *
//#define delete MEMTRACK_DELETE
