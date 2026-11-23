#include "pch.h"

#include "catch.hpp"

#define PROVIDER_CLASS_NAME TestProvider
#include "TraceLoggingTests.h"

TEST_CASE("TraceLoggingTests::TrySplitKeepsActivityRunningUntilLastStop", "[tracelogging]")
{
    auto root = TestProvider::TraceloggingActivity::Start();
    TestProvider::TraceloggingActivity first;
    TestProvider::TraceloggingActivity second;

    REQUIRE(root.TrySplit(&first));
    REQUIRE(root.TrySplit(&second));
    REQUIRE(IsEqualGUID(*root.Id(), *first.Id()));
    REQUIRE(IsEqualGUID(*root.Id(), *second.Id()));

    root.Stop(S_OK);
    REQUIRE(first.IsRunning());

    first.Stop(E_FAIL);
    REQUIRE(second.IsRunning());

    second.Stop(S_OK);
    REQUIRE_FALSE(root.IsRunning());
}
