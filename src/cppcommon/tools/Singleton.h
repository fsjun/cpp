#pragma once
#include <thread>
#include <mutex>
#include <memory>

template<typename T>
class Singleton
{
public:
    static std::shared_ptr<T> GetInstance()
	{
		std::call_once(sOnceFlag, [=] {sInstance.reset(new T());});
        return sInstance;
    }

protected:
    Singleton() { }
    virtual ~Singleton() { }

    static std::shared_ptr<T> sInstance;
	static std::once_flag sOnceFlag;

private:
    Singleton(const Singleton&) = delete;
    Singleton& operator =(const Singleton&) = delete;
};

template<typename T>
std::shared_ptr<T> Singleton<T>::sInstance;
template<typename T>
std::once_flag Singleton<T>::sOnceFlag;
