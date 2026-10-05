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

TEST_CASE(Integration_ServiceHelper, ServiceMatchesTargetAndFailurePropagation) {
    // 1. MatchesTarget on RpcSs
    ServiceAction action;
    action.serviceName = L"RpcSs";
    action.startupTypeDefault = 2;   // SERVICE_AUTO_START
    action.startupTypeProtected = 4; // SERVICE_DISABLED

    ASSERT_TRUE(ServiceHelper::MatchesTarget(action, false));
    ASSERT_FALSE(ServiceHelper::MatchesTarget(action, true));

    // 2. StopService on non-existent service returns false
    ASSERT_FALSE(ServiceHelper::StopService(L"NonExistentPrivatizeWinDummyService_XYZ"));

    // 3. ApplyAction on non-existent service returns false (does not falsely succeed)
    ServiceAction badAction;
    badAction.serviceName = L"NonExistentPrivatizeWinDummyService_XYZ";
    badAction.startupTypeProtected = 4;
    badAction.startupTypeDefault = 3;
    ASSERT_FALSE(ServiceHelper::ApplyAction(badAction, true));
}

