#pragma once

// Included, after wil/result.h with RESULT_LAZY_MESSAGE_ARGUMENTS set to 1, by LazyMessageArgumentsTests.cpp and
// LazyMessageArgumentsStdcallTests.cpp, which x86 builds compile with different default calling conventions. Member
// functions are __thiscall whatever the default, so both units define the same symbols here, and the linker keeps one
// definition of each, which must report correctly when the other unit calls it.
struct LazyMessageArgumentsSharedObject
{
    int value;

    HRESULT Check(HRESULT hr) const
    {
        return LOG_IF_FAILED_MSG(hr, "shared %d", value);
    }

    bool CheckLastError(DWORD error) const
    {
        ::SetLastError(error);
        return LOG_LAST_ERROR_IF_MSG(error != ERROR_SUCCESS, "shared last error %d", value);
    }
};
