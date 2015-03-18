#ifndef _LOCK_H_
#define _LOCK_H_

#include "Base.h"

namespace base{

class Lock
{
public:
    Lock();

    ~Lock();

    bool Try();

    void Acquire();

    void Release();

private:
    CRITICAL_SECTION cs_;

    DISALLOW_COPY_AND_ASSIGN(Lock);
};

//
// Lock?“?迆角角
// 11?足o‘那y-?車??
// ??11o‘那y-那赤﹞???
//
class AutoLock {
public:
    explicit AutoLock(Lock& lock) : lock_(lock) {
        lock_.Acquire();
    }

    ~AutoLock() {
        lock_.Release();
    }

private:
    Lock& lock_;
    DISALLOW_COPY_AND_ASSIGN(AutoLock);
};

//
// Lock?“?迆角角
// 11?足o‘那y-那赤﹞???
// ??11o‘那y-?車??
//
class AutoUnlock {
public:
    explicit AutoUnlock(Lock& lock) : lock_(lock) {
        lock_.Release();
    }

    ~AutoUnlock() {
        lock_.Acquire();
    }

private:
    Lock& lock_;
    DISALLOW_COPY_AND_ASSIGN(AutoUnlock);
};

} // namespace Base

#endif // _LOCK_H_
