#include "../TestFramework.h"
#include "../../src/core/ServiceHelper.h"

using namespace PrivatizeWin;

TEST_CASE(Integration_ServiceHelper, QueryStandardWindowsServices) {
    // RpcSs (Remote Procedure Call) is guaranteed to exist and run on every Windows NT installation
    ASSERT_TRUE(ServiceHelper::ServiceExists(L"RpcSs"));

    const auto startType = ServiceHelper::GetServiceStartType(L"RpcSs");
    ASSERT_TRUE(startType.has_value());
    // SERVICE_AUTO_START is 2
    ASSERT_EQ(startType.value(), 2);

    // Non-existent service should return false
    ASSERT_FALSE(ServiceHelper::ServiceExists(L"NonExistentPrivatizeWinDummyService_XYZ"));
    ASSERT_FALSE(ServiceHelper::GetServiceStartType(L"NonExistentPrivatizeWinDummyService_XYZ").has_value());
}
