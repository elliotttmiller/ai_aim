#pragma once
#include <mutex>

// Thread-safe singleton pattern for any class T
// Usage: class MyClass : public Singleton<MyClass> { ... } 
template <typename T>
class Singleton {
public:
    static T* GetInstance() {
        static std::once_flag flag;
        std::call_once(flag, []() {
            instance_ = new T();
        });
        return instance_;
    }
protected:
    Singleton() = default;
    virtual ~Singleton() = default;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
private:
    static T* instance_;
};

template <typename T>
T* Singleton<T>::instance_ = nullptr;
