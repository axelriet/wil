#include "pch.h"

// The conditional XXX_MSG macros with RESULT_LAZY_MESSAGE_ARGUMENTS set to 1; ResultTests.cpp tests the default (0). The
// setting may differ between translation units, as it does in this test binary.
#define RESULT_LAZY_MESSAGE_ARGUMENTS 1

#include <wil/result.h>

#include <memory>

#include "common.h"
#include "LazyMessageArgumentsShared.h"

// Defined in LazyMessageArgumentsStdcallTests.cpp, which x86 builds compile with another default calling convention
extern "C" int __cdecl LazyMessageArgumentsFromOtherUnit(int value);

namespace
{
// Every failure reported on this thread: its HRESULT, message, line, function and return address
struct FailureCapture final : wil::details::IFailureCallback
{
    int count = 0;
    HRESULT hr = S_OK;
    wchar_t message[256] = {};
    unsigned int line = 0;
    char function[256] = {};
    void* returnAddress = nullptr;
    wil::details::ThreadFailureCallbackHolder holder{this};

    bool NotifyFailure(wil::FailureInfo const& failure) WI_NOEXCEPT override
    {
        ++count;
        hr = failure.hr;
        wcscpy_s(message, (failure.pszMessage != nullptr) ? failure.pszMessage : L"");
        line = failure.uLineNumber;
        strcpy_s(function, (failure.pszFunction != nullptr) ? failure.pszFunction : "");
        returnAddress = failure.returnAddress;
        return false;
    }
};

int evaluations = 0;

int Counted(int value)
{
    ++evaluations;
    return value;
}

struct Entry
{
    unsigned int port;
};

// Stands for an argument that is valid only in the failure state (the member of an object that does not exist yet)
unsigned int PortOf(const Entry* entry)
{
    REQUIRE(entry != nullptr);
    return (entry != nullptr) ? entry->port : 0;
}

BOOL FailWith(DWORD error)
{
    ::SetLastError(error);
    return FALSE;
}

void* FailWithNull(DWORD error)
{
    ::SetLastError(error);
    return nullptr;
}

// A message argument that changes the last error
int SetsLastError(DWORD error)
{
    ::SetLastError(error);
    return static_cast<int>(error);
}

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_SYSTEM)
HMODULE ModuleOf(const void* address)
{
    HMODULE module = nullptr;
    REQUIRE(
        ::GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, static_cast<PCWSTR>(address), &module));
    return module;
}
#endif

#ifdef WIL_ENABLE_EXCEPTIONS
struct ArgumentException
{
};

int Throwing(bool enabled)
{
    if (enabled)
    {
        throw ArgumentException{};
    }

    return 0;
}
#endif
} // namespace

TEST_CASE("LazyMessageArgumentsTests::NotEvaluatedOnSuccess", "[result]")
{
    evaluations = 0;
    Entry* missing = nullptr;
    int value = 42;
    int* pointer = &value;
    HANDLE handle = ::GetCurrentProcess();

    REQUIRE(LOG_IF_FAILED_MSG(S_OK, "%u %d", PortOf(missing), Counted(1)) == S_OK);
    REQUIRE(LOG_IF_WIN32_BOOL_FALSE_MSG(TRUE, "%u %d", PortOf(missing), Counted(1)) == TRUE);
    REQUIRE(LOG_IF_WIN32_ERROR_MSG(ERROR_SUCCESS, "%u %d", PortOf(missing), Counted(1)) == ERROR_SUCCESS);
    REQUIRE(LOG_IF_NULL_ALLOC_MSG(pointer, "%u %d", PortOf(missing), Counted(1)) == pointer);
    REQUIRE(LOG_HR_IF_MSG(E_FAIL, false, "%u %d", PortOf(missing), Counted(1)) == false);
    REQUIRE(LOG_HR_IF_NULL_MSG(E_FAIL, pointer, "%u %d", PortOf(missing), Counted(1)) == pointer);
    REQUIRE(LOG_LAST_ERROR_IF_MSG(false, "%u %d", PortOf(missing), Counted(1)) == false);
    REQUIRE(LOG_LAST_ERROR_IF_NULL_MSG(handle, "%u %d", PortOf(missing), Counted(1)) == handle);
    REQUIRE(LOG_IF_NTSTATUS_FAILED_MSG(0, "%u %d", PortOf(missing), Counted(1)) == 0);

    REQUIRE_NOERROR(FAIL_FAST_IF_FAILED_MSG(S_OK, "%u %d", PortOf(missing), Counted(1)));
    REQUIRE_NOERROR(FAIL_FAST_IF_WIN32_BOOL_FALSE_MSG(TRUE, "%u %d", PortOf(missing), Counted(1)));
    REQUIRE_NOERROR(FAIL_FAST_IF_WIN32_ERROR_MSG(ERROR_SUCCESS, "%u %d", PortOf(missing), Counted(1)));
    REQUIRE_NOERROR(FAIL_FAST_IF_NULL_ALLOC_MSG(pointer, "%u %d", PortOf(missing), Counted(1)));
    REQUIRE_NOERROR(FAIL_FAST_HR_IF_MSG(E_FAIL, false, "%u %d", PortOf(missing), Counted(1)));
    REQUIRE_NOERROR(FAIL_FAST_HR_IF_NULL_MSG(E_FAIL, pointer, "%u %d", PortOf(missing), Counted(1)));
    REQUIRE_NOERROR(FAIL_FAST_LAST_ERROR_IF_MSG(false, "%u %d", PortOf(missing), Counted(1)));
    REQUIRE_NOERROR(FAIL_FAST_LAST_ERROR_IF_NULL_MSG(handle, "%u %d", PortOf(missing), Counted(1)));
    REQUIRE_NOERROR(FAIL_FAST_IF_NTSTATUS_FAILED_MSG(0, "%u %d", PortOf(missing), Counted(1)));
    REQUIRE_NOERROR(FAIL_FAST_IF_MSG(false, "%u %d", PortOf(missing), Counted(1)));
    REQUIRE_NOERROR(FAIL_FAST_IF_NULL_MSG(pointer, "%u %d", PortOf(missing), Counted(1)));

#ifdef WIL_ENABLE_EXCEPTIONS
    REQUIRE(THROW_IF_FAILED_MSG(S_OK, "%u %d", PortOf(missing), Counted(1)) == S_OK);
    REQUIRE(THROW_IF_WIN32_BOOL_FALSE_MSG(TRUE, "%u %d", PortOf(missing), Counted(1)) == TRUE);
    REQUIRE(THROW_IF_WIN32_ERROR_MSG(ERROR_SUCCESS, "%u %d", PortOf(missing), Counted(1)) == ERROR_SUCCESS);
    REQUIRE(THROW_IF_NULL_ALLOC_MSG(pointer, "%u %d", PortOf(missing), Counted(1)) == pointer);
    REQUIRE(THROW_HR_IF_MSG(E_FAIL, false, "%u %d", PortOf(missing), Counted(1)) == false);
    REQUIRE(THROW_HR_IF_NULL_MSG(E_FAIL, pointer, "%u %d", PortOf(missing), Counted(1)) == pointer);
    REQUIRE(THROW_WIN32_IF_MSG(ERROR_GEN_FAILURE, false, "%u %d", PortOf(missing), Counted(1)) == false);
    REQUIRE(THROW_LAST_ERROR_IF_MSG(false, "%u %d", PortOf(missing), Counted(1)) == false);
    REQUIRE(THROW_LAST_ERROR_IF_NULL_MSG(handle, "%u %d", PortOf(missing), Counted(1)) == handle);
    REQUIRE(THROW_IF_NTSTATUS_FAILED_MSG(0, "%u %d", PortOf(missing), Counted(1)) == 0);

    // A smart pointer, compared to nullptr through its own operator
    std::unique_ptr<int> owned(new int(7));
    REQUIRE_NOERROR(FAIL_FAST_IF_NULL_MSG(owned, "%u %d", PortOf(missing), Counted(1)));
    THROW_IF_NULL_ALLOC_MSG(owned, "%u %d", PortOf(missing), Counted(1));
#endif

    REQUIRE(evaluations == 0);
}

TEST_CASE("LazyMessageArgumentsTests::EvaluatedOnceOnFailure", "[result]")
{
    FailureCapture capture;
    evaluations = 0;
    int* nullPointer = nullptr;

    REQUIRE(LOG_IF_FAILED_MSG(E_INVALIDARG, "value=%d", Counted(7)) == E_INVALIDARG);
    const unsigned int line = __LINE__ - 1;
    REQUIRE(evaluations == 1);
    REQUIRE(capture.count == 1);
    REQUIRE(capture.hr == E_INVALIDARG);
    REQUIRE(wcscmp(capture.message, L"value=7") == 0);
    REQUIRE(capture.returnAddress != nullptr);
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_SYSTEM)
    // In this function once the compiler inlines, in the lambda of the macro otherwise (/Od): in this module either way
    REQUIRE(ModuleOf(capture.returnAddress) == ModuleOf(&evaluations));
#endif
#if (RESULT_DIAGNOSTICS_LEVEL >= 2)
    REQUIRE(capture.line == line);
#else
    (void)line;
#endif
#if (RESULT_DIAGNOSTICS_LEVEL >= 4)
    // The function name is taken outside the lambda: it names this function
    REQUIRE(strstr(capture.function, "operator") == nullptr);
#endif

    REQUIRE(LOG_HR_IF_NULL_MSG(E_POINTER, nullPointer, "%hs %d", "text", Counted(8)) == nullptr);
    REQUIRE(capture.count == 2);
    REQUIRE(capture.hr == E_POINTER);
    REQUIRE(wcscmp(capture.message, L"text 8") == 0);

    REQUIRE(LOG_IF_NULL_ALLOC_MSG(nullPointer, "no arguments") == nullptr);
    REQUIRE(capture.count == 3);
    REQUIRE(capture.hr == E_OUTOFMEMORY);
    REQUIRE(wcscmp(capture.message, L"no arguments") == 0);

    REQUIRE(LOG_IF_WIN32_ERROR_MSG(ERROR_ACCESS_DENIED, "%d", Counted(9)) == ERROR_ACCESS_DENIED);
    REQUIRE(capture.count == 4);
    REQUIRE(capture.hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));

    REQUIRE_ERROR(FAIL_FAST_IF_MSG(true, "%d", Counted(10)));
    REQUIRE(capture.count == 5);
    REQUIRE(capture.hr == E_UNEXPECTED);
    REQUIRE(wcscmp(capture.message, L"10") == 0);

    REQUIRE_ERROR(FAIL_FAST_IF_NULL_ALLOC_MSG(nullPointer, "%d", Counted(11)));
    REQUIRE(capture.count == 6);
    REQUIRE(capture.hr == E_OUTOFMEMORY);

    REQUIRE(evaluations == 5);

#ifdef WIL_ENABLE_EXCEPTIONS
    bool caught = false;

    try
    {
        THROW_IF_FAILED_MSG(E_ACCESSDENIED, "value=%d", Counted(12));
    }
    catch (const wil::ResultException& exception)
    {
        caught = true;
        REQUIRE(exception.GetErrorCode() == E_ACCESSDENIED);
        REQUIRE(wcscmp(exception.GetFailureInfo().pszMessage, L"value=12") == 0);
    }

    REQUIRE(caught);
    REQUIRE(evaluations == 6);
#endif
}

// The macros that report the last error read it right after the check, before the message arguments run, and restore
// it after them: an argument that changes the last error, or clears it, changes neither the error reported (or
// thrown) nor the last error the macro leaves.
TEST_CASE("LazyMessageArgumentsTests::LastErrorReadBeforeArguments", "[result]")
{
    FailureCapture capture;
    int count = 0;

    for (const DWORD change : {static_cast<DWORD>(ERROR_FILE_NOT_FOUND), static_cast<DWORD>(ERROR_SUCCESS)})
    {
        // The last error is read right after each macro, before REQUIRE runs any code of its own
        const BOOL ok = LOG_IF_WIN32_BOOL_FALSE_MSG(FailWith(ERROR_ACCESS_DENIED), "%d", SetsLastError(change));
        DWORD lastError = ::GetLastError();
        REQUIRE(ok == FALSE);
        REQUIRE(lastError == ERROR_ACCESS_DENIED);
        REQUIRE(capture.count == ++count);
        REQUIRE(capture.hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));

        const bool condition = LOG_LAST_ERROR_IF_MSG(!FailWith(ERROR_ACCESS_DENIED), "%d", SetsLastError(change));
        lastError = ::GetLastError();
        REQUIRE(condition);
        REQUIRE(lastError == ERROR_ACCESS_DENIED);
        REQUIRE(capture.count == ++count);
        REQUIRE(capture.hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));

        const void* const pointer = LOG_LAST_ERROR_IF_NULL_MSG(FailWithNull(ERROR_ACCESS_DENIED), "%d", SetsLastError(change));
        lastError = ::GetLastError();
        REQUIRE(pointer == nullptr);
        REQUIRE(lastError == ERROR_ACCESS_DENIED);
        REQUIRE(capture.count == ++count);
        REQUIRE(capture.hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));

        REQUIRE_ERROR(FAIL_FAST_IF_WIN32_BOOL_FALSE_MSG(FailWith(ERROR_ACCESS_DENIED), "%d", SetsLastError(change)));
        REQUIRE(capture.count == ++count);
        REQUIRE(capture.hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));

        REQUIRE_ERROR(FAIL_FAST_LAST_ERROR_IF_MSG(!FailWith(ERROR_ACCESS_DENIED), "%d", SetsLastError(change)));
        REQUIRE(capture.count == ++count);
        REQUIRE(capture.hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));

        REQUIRE_ERROR(FAIL_FAST_LAST_ERROR_IF_NULL_MSG(FailWithNull(ERROR_ACCESS_DENIED), "%d", SetsLastError(change)));
        REQUIRE(capture.count == ++count);
        REQUIRE(capture.hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));

#ifdef WIL_ENABLE_EXCEPTIONS
        HRESULT thrown = S_OK;

        try
        {
            THROW_IF_WIN32_BOOL_FALSE_MSG(FailWith(ERROR_ACCESS_DENIED), "%d", SetsLastError(change));
        }
        catch (const wil::ResultException& exception)
        {
            thrown = exception.GetErrorCode();
        }

        REQUIRE(thrown == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));
        thrown = S_OK;

        try
        {
            THROW_LAST_ERROR_IF_MSG(!FailWith(ERROR_ACCESS_DENIED), "%d", SetsLastError(change));
        }
        catch (const wil::ResultException& exception)
        {
            thrown = exception.GetErrorCode();
        }

        REQUIRE(thrown == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));
        thrown = S_OK;

        try
        {
            THROW_LAST_ERROR_IF_NULL_MSG(FailWithNull(ERROR_ACCESS_DENIED), "%d", SetsLastError(change));
        }
        catch (const wil::ResultException& exception)
        {
            thrown = exception.GetErrorCode();
        }

        REQUIRE(thrown == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));
        count = capture.count;
#endif
    }
}

// The LOG_ and FAIL_FAST_ functions are noexcept, as with the setting at 0; the THROW_ functions are not. Before C++20
// a macro cannot appear in noexcept() (its lambda would be in an unevaluated operand), so the functions are checked
// with a named lambda.
TEST_CASE("LazyMessageArgumentsTests::Noexcept", "[result]")
{
    HRESULT hr = S_OK;
    auto format = [](const auto&, bool) {};

    static_assert(noexcept(__R_FN(Log_IfFailedMsg)(__R_INFO("hr") hr, format)), "Log_IfFailedMsg is noexcept");
    static_assert(noexcept(__R_FN(Log_GetLastErrorIfNullMsg)(__R_INFO("hr") & hr, format)), "Log_GetLastErrorIfNullMsg is noexcept");
    static_assert(noexcept(__RFF_FN(FailFast_IfMsg)(__RFF_INFO("hr") hr != S_OK, format)), "FailFast_IfMsg is noexcept");
#ifdef WIL_ENABLE_EXCEPTIONS
    static_assert(!noexcept(__R_FN(Throw_IfFailedMsg)(__R_INFO("hr") hr, format)), "Throw_IfFailedMsg may throw");
#endif

#if (__WI_LIBCPP_STD_VER >= 20)
    static_assert(noexcept(LOG_IF_FAILED_MSG(hr, "%d", 1)), "LOG_IF_FAILED_MSG is noexcept");
    static_assert(noexcept(FAIL_FAST_IF_MSG(hr != S_OK, "%d", 1)), "FAIL_FAST_IF_MSG is noexcept");
#ifdef WIL_ENABLE_EXCEPTIONS
    static_assert(!noexcept(THROW_IF_FAILED_MSG(hr, "%d", 1)), "THROW_IF_FAILED_MSG may throw");
#endif
#endif

    (void)format;
    REQUIRE(LOG_IF_FAILED_MSG(hr, "%d", 1) == S_OK);
}

#ifdef WIL_ENABLE_EXCEPTIONS
// A message argument that throws is not evaluated on success; on failure, an exception from it propagates out of a
// THROW_ macro, and nothing is reported (out of a LOG_ or FAIL_FAST_ macro, it would end the process: they are noexcept)
TEST_CASE("LazyMessageArgumentsTests::ThrowingArguments", "[result]")
{
    FailureCapture capture;

    REQUIRE(LOG_IF_FAILED_MSG(S_OK, "%d", Throwing(true)) == S_OK);
    REQUIRE(THROW_IF_FAILED_MSG(S_OK, "%d", Throwing(true)) == S_OK);
    REQUIRE(THROW_LAST_ERROR_IF_MSG(false, "%d", Throwing(true)) == false);
    REQUIRE(capture.count == 0);

    bool caught = false;

    try
    {
        THROW_IF_FAILED_MSG(E_FAIL, "%d", Throwing(true));
    }
    catch (const ArgumentException&)
    {
        caught = true;
    }

    REQUIRE(caught);
    REQUIRE(capture.count == 0);
}
#endif

namespace
{
struct Flags
{
    unsigned int kind : 4;
    unsigned int level : 4;
};

struct Pair
{
    HRESULT hr;
    int index;
};

struct Holder
{
    int member = 41;

#if (RESULT_DIAGNOSTICS_LEVEL < 4) // From level 4, __FUNCTION__ (outside a function body here) does not compile, lazy or not
    HRESULT initialized = LOG_IF_FAILED_MSG(S_OK, "member %d", Counted(member)); // A default member initializer
#endif

    void Log() const
    {
        LOG_IF_FAILED_MSG(E_FAIL, "member %d", member); // Captures this
    }
};

template <typename... Args>
void LogPack(Args... args)
{
    LOG_IF_FAILED_MSG(E_FAIL, "pack %d %d", args...);
}
} // namespace

TEST_CASE("LazyMessageArgumentsTests::ArgumentKinds", "[result]")
{
    FailureCapture capture;

    Flags flags{3, 9};
    LOG_IF_FAILED_MSG(E_FAIL, "%u %u", flags.kind, flags.level);
    REQUIRE(wcscmp(capture.message, L"3 9") == 0);

    evaluations = 0;
    Holder holder;
    REQUIRE(evaluations == 0);
    holder.Log();
    REQUIRE(wcscmp(capture.message, L"member 41") == 0);

    LogPack(1, 2);
    REQUIRE(wcscmp(capture.message, L"pack 1 2") == 0);

    char text[] = "array";
    const float half = 0.5f;
    LOG_IF_FAILED_MSG(E_FAIL, "%hs %.1f", text, half);
    REQUIRE(wcscmp(capture.message, L"array 0.5") == 0);

#if (__WI_LIBCPP_STD_VER >= 17)
    // A structured binding is captured, which C++17 allows only as an extension (clang's warning is silenced there)
    auto [hr, index] = Pair{E_FAIL, 42};
    LOG_IF_FAILED_MSG(hr, "index=%d", index);
    REQUIRE(wcscmp(capture.message, L"index=42") == 0);
#endif
}

namespace
{
// Local constants named in message arguments from a lambda that cannot capture them and from a local class: the
// arguments are passed on as var-args, as with the setting at 0, which does not odr-use a constant, so nothing captures
// them
int LogConstantsFromNestedScopes()
{
    constexpr int retries = 3;
    constexpr PCSTR name = "store";
    const DWORD timeoutMs = 5000;
    int reported = 0;

    auto captureless = [](HRESULT hr) {
        return LOG_IF_FAILED_MSG(hr, "%hs after %d retries", name, retries);
    };
    reported += FAILED(captureless(E_FAIL)) ? 1 : 0;

    const int id = 7;
    auto explicitCapture = [&reported](HRESULT hr) {
        reported += FAILED(LOG_IF_FAILED_MSG(hr, "%hs %d %lu", name, id, timeoutMs)) ? 1 : 0;
    };
    explicitCapture(E_INVALIDARG);

    // A capture-less lambda converted to a function pointer, as for a callback
    BOOL(CALLBACK * callback)(PINIT_ONCE, PVOID, PVOID*) = [](PINIT_ONCE, PVOID, PVOID*) -> BOOL {
        ::SetLastError(ERROR_INVALID_DATA);
        return LOG_IF_WIN32_BOOL_FALSE_MSG(FALSE, "%hs %d", name, retries);
    };
    reported += callback(nullptr, nullptr, nullptr) ? 0 : 1;

    struct Callback
    {
        static bool Invoke(HRESULT hr)
        {
            return LOG_HR_IF_MSG(hr, true, "%d %lu", retries, timeoutMs);
        }
    };
    reported += Callback::Invoke(E_ACCESSDENIED) ? 1 : 0;
    return reported;
}

// Two uses of a macro whose arguments have the same types, each in a function of its own
__declspec(noinline) void LogFromFirstSite(int value)
{
    LOG_IF_FAILED_MSG(E_FAIL, "first %d", value);
}

__declspec(noinline) void LogFromSecondSite(int value)
{
    LOG_IF_FAILED_MSG(E_FAIL, "second %d", value);
}

__declspec(noinline) bool FailFastFromFirstSite(int value)
{
    return FAIL_FAST_IF_MSG(value != 0, "first %d", value);
}

__declspec(noinline) bool FailFastFromSecondSite(int value)
{
    return FAIL_FAST_IF_MSG(value != 0, "second %d", value);
}
} // namespace

TEST_CASE("LazyMessageArgumentsTests::ConstantsInNestedScopes", "[result]")
{
    FailureCapture capture;

    REQUIRE(LogConstantsFromNestedScopes() == 4);
    REQUIRE(capture.count == 4);
    REQUIRE(capture.hr == E_ACCESSDENIED);
    REQUIRE(wcscmp(capture.message, L"3 5000") == 0);
}

// The return address reported (the exception address of a fail fast) is in the function that uses the macro once the
// compiler inlines, and in the lambda of the macro otherwise (/Od), which each use of a macro has: two uses do not
// report the same address
TEST_CASE("LazyMessageArgumentsTests::ReturnAddressPerUse", "[result]")
{
    FailureCapture capture;

    LogFromFirstSite(1);
    void* const firstLog = capture.returnAddress;
    LogFromSecondSite(2);
    void* const secondLog = capture.returnAddress;
    REQUIRE(capture.count == 2);
    REQUIRE(firstLog != nullptr);
    REQUIRE(firstLog != secondLog);

    REQUIRE_ERROR(FailFastFromFirstSite(3));
    void* const firstFailFast = capture.returnAddress;
    REQUIRE_ERROR(FailFastFromSecondSite(4));
    void* const secondFailFast = capture.returnAddress;
    REQUIRE(capture.count == 4);
    REQUIRE(firstFailFast != nullptr);
    REQUIRE(firstFailFast != secondFailFast);
}

// The message arguments are code of the lambda: __func__ there names the lambda; a local read before the macro keeps
// the name of the calling function
TEST_CASE("LazyMessageArgumentsTests::FunctionNamesInArguments", "[result]")
{
    FailureCapture capture;
    PCSTR const caller = __func__;

    LOG_IF_FAILED_MSG(E_FAIL, "%hs", __func__);
    REQUIRE(capture.count == 1);
    REQUIRE(wcsstr(capture.message, L"operator") != nullptr);

    LOG_IF_FAILED_MSG(E_FAIL, "%hs", caller);
    wchar_t expected[256] = {};
    swprintf_s(expected, L"%hs", caller);
    REQUIRE(wcscmp(capture.message, expected) == 0);
}

// On x86, LazyMessageArgumentsStdcallTests.cpp is built with another default calling convention (/Gz); both units report
// failures through the same macros, which must not share code that depends on the default convention
TEST_CASE("LazyMessageArgumentsTests::OtherCallingConvention", "[result]")
{
    FailureCapture capture;

    LOG_IF_FAILED_MSG(E_FAIL, "this unit %d", 1);
    REQUIRE(capture.count == 1);
    REQUIRE(wcscmp(capture.message, L"this unit 1") == 0);

    const int reported = LazyMessageArgumentsFromOtherUnit(2);
    REQUIRE(capture.count == 1 + reported);
    REQUIRE(capture.hr == E_ACCESSDENIED);
    REQUIRE(wcscmp(capture.message, L"other unit hr_if 2") == 0);

    REQUIRE_ERROR(FAIL_FAST_IF_MSG(true, "this unit %d", 3));
    REQUIRE(capture.count == 2 + reported);
    REQUIRE(wcscmp(capture.message, L"this unit 3") == 0);

    // Inline member functions that both units define (LazyMessageArgumentsShared.h), called from this unit too
    const LazyMessageArgumentsSharedObject object{4};
    REQUIRE(object.Check(E_INVALIDARG) == E_INVALIDARG);
    REQUIRE(capture.count == 3 + reported);
    REQUIRE(capture.hr == E_INVALIDARG);
    REQUIRE(wcscmp(capture.message, L"shared 4") == 0);

    REQUIRE(object.CheckLastError(ERROR_INVALID_DATA));
    REQUIRE(capture.count == 4 + reported);
    REQUIRE(capture.hr == HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
    REQUIRE(wcscmp(capture.message, L"shared last error 4") == 0);
}

// The checked expression runs once on both paths, and noncanonical Win32 TRUE values retain their value.
TEST_CASE("LazyMessageArgumentsTests::CheckedExpressionEvaluatedOnce", "[result]")
{
    FailureCapture capture;
    int checks = 0;
    int messages = 0;
    const auto checkedHr = [&](HRESULT result) {
        ++checks;
        return result;
    };
    const auto checkedBool = [&](BOOL result) {
        ++checks;
        ::SetLastError(ERROR_ACCESS_DENIED);
        return result;
    };
    const auto message = [&]() {
        ++messages;
        return 7;
    };

    REQUIRE(LOG_IF_FAILED_MSG(checkedHr(S_OK), "%d", message()) == S_OK);
    REQUIRE(checks == 1);
    REQUIRE(messages == 0);
    REQUIRE(LOG_IF_FAILED_MSG(checkedHr(E_INVALIDARG), "%d", message()) == E_INVALIDARG);
    REQUIRE(checks == 2);
    REQUIRE(messages == 1);

    REQUIRE(LOG_IF_WIN32_BOOL_FALSE_MSG(checkedBool(2), "%d", message()) == 2);
    REQUIRE(checks == 3);
    REQUIRE(messages == 1);
    REQUIRE(LOG_IF_WIN32_BOOL_FALSE_MSG(checkedBool(FALSE), "%d", message()) == FALSE);
    REQUIRE(checks == 4);
    REQUIRE(messages == 2);
    REQUIRE(capture.hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));

    REQUIRE_NOERROR(FAIL_FAST_IF_FAILED_MSG(checkedHr(S_OK), "%d", message()));
    REQUIRE(checks == 5);
    REQUIRE(messages == 2);
    REQUIRE(witest::DoesCodeFailFast([&]() {
        return FAIL_FAST_IF_FAILED_MSG(checkedHr(E_FAIL), "%d", message());
    }));
    REQUIRE(checks == 6);
    REQUIRE(messages == 3);

#ifdef WIL_ENABLE_EXCEPTIONS
    REQUIRE(THROW_IF_FAILED_MSG(checkedHr(S_OK), "%d", message()) == S_OK);
    REQUIRE(checks == 7);
    REQUIRE(messages == 3);
    bool caught = false;
    try
    {
        THROW_IF_FAILED_MSG(checkedHr(E_ACCESSDENIED), "%d", message());
    }
    catch (const wil::ResultException& exception)
    {
        caught = true;
        REQUIRE(exception.GetErrorCode() == E_ACCESSDENIED);
    }
    REQUIRE(caught);
    REQUIRE(checks == 8);
    REQUIRE(messages == 4);
#endif
}
