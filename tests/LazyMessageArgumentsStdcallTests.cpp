// Built without the precompiled header of the other sources, and on x86 with another default calling convention (/Gz;
// see CMakeLists.txt): LazyMessageArgumentsTests.cpp calls this unit, and both report failures through the same macros
#include "pch.h"

#define RESULT_LAZY_MESSAGE_ARGUMENTS 1

#include <windows.h>
#include <wil/result.h>

#include "LazyMessageArgumentsShared.h"

// Reports failures through conditional _MSG macros, its own and the shared ones; returns how many it reported
extern "C" int __cdecl LazyMessageArgumentsFromOtherUnit(int value)
{
    int reported = 0;
    LOG_IF_FAILED_MSG(E_FAIL, "other unit %d", value);
    ++reported;

    const LazyMessageArgumentsSharedObject object{value};
    object.Check(E_FAIL);
    ++reported;
    object.CheckLastError(ERROR_INVALID_DATA);
    ++reported;

#ifdef WIL_ENABLE_EXCEPTIONS
    try
    {
        THROW_IF_FAILED_MSG(E_INVALIDARG, "other unit throw %d", value);
    }
    catch (const wil::ResultException& exception)
    {
        if (exception.GetErrorCode() == E_INVALIDARG)
        {
            ++reported;
        }
    }
#endif

    LOG_HR_IF_MSG(E_ACCESSDENIED, value > 0, "other unit hr_if %d", value);
    ++reported;
    return reported;
}
