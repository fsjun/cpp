#pragma once

#include "tools/cpp_common.h"
#include <thread>

template <typename T>
class Storage {
public:
    static bool AddForce(string id, shared_ptr<T> object)
    {
        std::lock_guard l(sMutex);
        sMap.erase(id);
        auto ret = sMap.emplace(id, object);
        return ret.second;
    }

    static bool Add(string id, shared_ptr<T> object)
    {
        std::lock_guard l(sMutex);
        auto ret = sMap.emplace(id, object);
        return ret.second;
    }

    static shared_ptr<T> Get(string id)
    {
        std::lock_guard l(sMutex);
        auto it = sMap.find(id);
        if (it == sMap.end()) {
            return nullptr;
        }
        return it->second;
    }

    static void Del(string id)
    {
        std::lock_guard l(sMutex);
        sMap.erase(id);
    }

protected:
    static std::map<string, shared_ptr<T>> sMap;
    static std::mutex sMutex;
};

template<class T>
std::map<string, shared_ptr<T>> Storage<T>::sMap;

template<class T>
std::mutex Storage<T>::sMutex;
