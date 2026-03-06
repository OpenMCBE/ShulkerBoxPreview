#pragma once
#include <atomic>
#include <type_traits>
#include <utility>
#include <cstddef>

template <typename T>
class SharedPtr;

template <typename T>
class WeakPtr;

template <typename T>
class SharedCounter {
private:
    template <typename>
    friend class SharedPtr;
    template <typename>
    friend class WeakPtr;

    T* ptr;
    std::atomic<int> share_count;
    std::atomic<int> weak_count;

    SharedCounter(T* p) : ptr(p), share_count(0), weak_count(0) {}
    ~SharedCounter() {}

    void addSharedRef() { ++share_count; }
    void addWeakRef()   { ++weak_count; }

    bool releaseSharedRef() {
        --share_count;
        if (share_count <= 0) {
            if (ptr) { T* tmp = ptr; ptr = nullptr; delete tmp; }
            if (weak_count <= 0) return true;
        }
        return false;
    }

    bool releaseWeakRef() {
        --weak_count;
        if (weak_count <= 0 && !ptr) return true;
        return false;
    }
};

template <typename T>
class SharedPtr {
private:
    SharedCounter<T>* pc;
    template <typename> friend class WeakPtr;
public:
    SharedPtr() : pc(nullptr) {}
    explicit SharedPtr(T* rawPtr) : pc(nullptr) {
        if (rawPtr) { pc = new SharedCounter<T>(rawPtr); pc->addSharedRef(); }
    }
    SharedPtr(const SharedPtr& sp) : pc(sp.pc) { if (pc) pc->addSharedRef(); }
    ~SharedPtr() { reset(); }

    T* get() const { return (pc) ? pc->ptr : nullptr; }
    operator T*() const { return get(); }
    T& operator*()  const { return *pc->ptr; }
    T* operator->() const { return pc->ptr; }

    SharedPtr& operator=(const SharedPtr& other) {
        SharedPtr tmp(other);
        std::swap(pc, tmp.pc);
        return *this;
    }

    void reset() {
        if (pc) {
            if (pc->releaseSharedRef()) delete pc;
            pc = nullptr;
        }
    }
};

template <typename T>
class WeakPtr {
private:
    SharedCounter<T>* pc;
    template <typename> friend class SharedPtr;
public:
    WeakPtr() : pc(nullptr) {}
    WeakPtr(std::nullptr_t) : pc(nullptr) {}
    WeakPtr(const SharedPtr<T>& sp) : pc(sp.pc) { if (pc) pc->addWeakRef(); }
    WeakPtr(const WeakPtr& wp) : pc(wp.pc)      { if (pc) pc->addWeakRef(); }
    ~WeakPtr() { reset(); }

    WeakPtr& operator=(const WeakPtr& other) { WeakPtr<T> tmp(other); std::swap(pc, tmp.pc); return *this; }
    WeakPtr& operator=(const SharedPtr<T>& other) { SharedPtr<T> tmp(other); std::swap(pc, tmp.pc); return *this; }

    bool isNull() const { return (pc) ? pc->ptr == nullptr : true; }
    T* get()      const { return (pc) ? pc->ptr : nullptr; }
    operator T*() const { return get(); }
    T& operator*()  const { return *pc->ptr; }
    T* operator->() const { return pc->ptr; }

    void reset() {
        if (pc) {
            if (pc->releaseWeakRef()) delete pc;
            pc = nullptr;
        }
    }

    static const WeakPtr<T>& null() { static WeakPtr<T> wnull; return wnull; }
};
