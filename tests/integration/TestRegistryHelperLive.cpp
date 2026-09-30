#include "../TestFramework.h"
#include "../../src/core/RegistryHelper.h"

using namespace PrivatizeWin;

TEST_CASE(Integration_RegistryHelper, SandboxedRegistryReadWriteAuditDelete) {
    const std::wstring testKey = L"Software\\PrivatizeWin_Test_Sandbox";
    const std::wstring testDwordVal = L"TestTelemetryDword";
    const std::wstring testStrVal = L"TestTelemetryString";

    // 1. Clean up any previous remnants
    RegistryHelper::DeleteValue(HKEY_CURRENT_USER, testKey, testDwordVal);
    RegistryHelper::DeleteValue(HKEY_CURRENT_USER, testKey, testStrVal);
    RegistryHelper::DeleteKeyIfEmpty(HKEY_CURRENT_USER, testKey);

    // 2. Write DWORD
    const bool writeDwordOk = RegistryHelper::WriteDword(HKEY_CURRENT_USER, testKey, testDwordVal, 1);
    ASSERT_TRUE(writeDwordOk);
    ASSERT_TRUE(RegistryHelper::KeyExists(HKEY_CURRENT_USER, testKey));
    ASSERT_TRUE(RegistryHelper::ValueExists(HKEY_CURRENT_USER, testKey, testDwordVal));

    // 3. Read DWORD
    const auto dwordRead = RegistryHelper::ReadDword(HKEY_CURRENT_USER, testKey, testDwordVal);
    ASSERT_TRUE(dwordRead.has_value());
    ASSERT_EQ(dwordRead.value(), 1);

    // 4. Audit Action
    RegistryAction action;
    action.subKey = testKey;
    action.valueName = testDwordVal;
    action.type = RegType::Dword;
    action.dwordProtected = 1;
    action.dwordDefault = 0;

    ASSERT_EQ(static_cast<int>(RegistryHelper::AuditAction(HKEY_CURRENT_USER, action)),
              static_cast<int>(SettingStatus::Protected));

    // Apply Default
    RegistryHelper::ApplyAction(HKEY_CURRENT_USER, action, false);
    ASSERT_EQ(static_cast<int>(RegistryHelper::AuditAction(HKEY_CURRENT_USER, action)),
              static_cast<int>(SettingStatus::Default));

    // 5. Write String
    const bool writeStrOk = RegistryHelper::WriteString(HKEY_CURRENT_USER, testKey, testStrVal, L"ProtectedValue");
    ASSERT_TRUE(writeStrOk);
    const auto strRead = RegistryHelper::ReadString(HKEY_CURRENT_USER, testKey, testStrVal);
    ASSERT_TRUE(strRead.has_value());
    ASSERT_EQ(strRead.value(), L"ProtectedValue");

    // 6. Clean up
    RegistryHelper::DeleteValue(HKEY_CURRENT_USER, testKey, testDwordVal);
    RegistryHelper::DeleteValue(HKEY_CURRENT_USER, testKey, testStrVal);
    RegistryHelper::DeleteKeyIfEmpty(HKEY_CURRENT_USER, testKey);
    ASSERT_FALSE(RegistryHelper::ValueExists(HKEY_CURRENT_USER, testKey, testDwordVal));
}
