#include "General.h"
#include "Lock.h"

namespace base {

Lock::Lock()
{
    // 2000 ??¦Ì?¨º??¨²¦Ì¨¨¡äy??¨¨?¨¢¨´????¨º¡À¡Á?Dy??¨ºy¡ê?¡¤¨¤?1??3¨¬?¡¥??
    ::InitializeCriticalSectionAndSpinCount(&cs_, 2000);
}

Lock::~Lock()
{
    ::DeleteCriticalSection(&cs_);
}

bool Lock::Try()
{
    if (::TryEnterCriticalSection(&cs_) != FALSE) {
        return true;
    }
    return false;
}

void Lock::Acquire()
{
    ::EnterCriticalSection(&cs_);
}

void Lock::Release()
{
    ::LeaveCriticalSection(&cs_);
}

}
