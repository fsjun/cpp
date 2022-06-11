
#include <map>
#include <sstream>
#include <cstdlib>
#include <cstring>

#include "CppMemory.h"
using namespace std;

#undef new
#undef delete

map<void *, map<string,string> > g_memory;
static mutex s_mutex;

namespace MemTrack
{
	string Int_to_String(int n)
	{
		ostringstream stream;
		stream<<n;
		return stream.str();
	}

    void TrackStamp(void *p, const MemStamp &stamp, char const *typeName)
    {
    	map<string,string> info;
    	info.insert(make_pair("fileName", stamp.filename));
    	string line = Int_to_String(stamp.lineNum);
    	info.insert(make_pair("line", line));
    	info.insert(make_pair("typeName", typeName));

        s_mutex.lock();
    	g_memory.insert(make_pair(p,info));
        s_mutex.unlock();
    }
    
    void TrackFree(void *p)
    {
        s_mutex.lock();
    	g_memory.erase(p);
        s_mutex.unlock();
    }
}

void *operator new(size_t size)
{
    void *p = malloc(size);
    if (p == NULL) throw std::bad_alloc();
    memset(p, 0, size);
    return p;
}

void *operator new[](size_t size)
{
    void *p = malloc(size);
    if (p == NULL) throw std::bad_alloc();
    memset(p, 0, size);
    return p;
}

void operator delete(void *p)
{
    MemTrack::TrackFree(p);
    free(p);
}

void operator delete[](void *p)
{
    MemTrack::TrackFree(p);
    free(p);
}
