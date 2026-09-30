#include "RestorePoint.h"

namespace PrivatizeWin {

#pragma pack(push, 1)
struct RESTOREPOINTINFOW_LOCAL {
    DWORD dwEventType;
    DWORD dwRestorePtType;
    INT64 llSequenceNumber;
    WCHAR szDescription[256];
};

struct STATEMGRSTATUS_LOCAL {
    DWORD nStatus;
    INT64 llSequenceNumber;
};
#pragma pack(pop)

typedef BOOL(WINAPI* PFN_SRSetRestorePointW)(RESTOREPOINTINFOW_LOCAL*, STATEMGRSTATUS_LOCAL*);

// RAII HMODULE
class UniqueHModule {
public:
    explicit UniqueHModule(HMODULE h = nullptr) noexcept : m_h(h) {}
    ~UniqueHModule() noexcept { if (m_h) FreeLibrary(m_h); }
    UniqueHModule(const UniqueHModule&) = delete;
    UniqueHModule& operator=(const UniqueHModule&) = delete;
    UniqueHModule(UniqueHModule&& o) noexcept : m_h(o.m_h) { o.m_h = nullptr; }
    UniqueHModule& operator=(UniqueHModule&& o) noexcept {
        if (this != &o) {
            if (m_h) FreeLibrary(m_h);
            m_h = o.m_h;
            o.m_h = nullptr;
        }
        return *this;
    }
    [[nodiscard]] HMODULE get() const noexcept { return m_h; }
    explicit operator bool() const noexcept { return m_h != nullptr; }
private:
    HMODULE m_h{ nullptr };
};

bool RestorePoint::IsSystemRestoreAvailable() noexcept {
    UniqueHModule hSrClient(LoadLibraryW(L"srclient.dll"));
    if (!hSrClient) return false;
    auto pfn = reinterpret_cast<PFN_SRSetRestorePointW>(GetProcAddress(hSrClient.get(), "SRSetRestorePointW"));
    return (pfn != nullptr);
}

bool RestorePoint::Create(std::wstring_view description, int64_t& outSequenceNumber) {
    UniqueHModule hSrClient(LoadLibraryW(L"srclient.dll"));
    if (!hSrClient) return false;

    auto pfnSRSetRestorePointW = reinterpret_cast<PFN_SRSetRestorePointW>(GetProcAddress(hSrClient.get(), "SRSetRestorePointW"));
    if (!pfnSRSetRestorePointW) {
        return false;
    }

    RESTOREPOINTINFOW_LOCAL rpi = { 0 };
    rpi.dwEventType = 100;      // BEGIN_SYSTEM_CHANGE
    rpi.dwRestorePtType = 12;   // MODIFY_SETTINGS
    rpi.llSequenceNumber = 0;
    
    std::wstring descStr(description);
    wcsncpy_s(rpi.szDescription, descStr.c_str(), 255);

    STATEMGRSTATUS_LOCAL sms = { 0 };
    const BOOL ok = pfnSRSetRestorePointW(&rpi, &sms);

    if (ok && sms.nStatus == ERROR_SUCCESS) {
        outSequenceNumber = sms.llSequenceNumber;

        // Immediately close the restore point session cleanly
        RESTOREPOINTINFOW_LOCAL rpiEnd = { 0 };
        rpiEnd.dwEventType = 101; // END_SYSTEM_CHANGE
        rpiEnd.llSequenceNumber = sms.llSequenceNumber;
        pfnSRSetRestorePointW(&rpiEnd, &sms);

        return true;
    }

    return false;
}

bool RestorePoint::Cancel(int64_t sequenceNumber) {
    UniqueHModule hSrClient(LoadLibraryW(L"srclient.dll"));
    if (!hSrClient) return false;

    auto pfnSRSetRestorePointW = reinterpret_cast<PFN_SRSetRestorePointW>(GetProcAddress(hSrClient.get(), "SRSetRestorePointW"));
    if (!pfnSRSetRestorePointW) {
        return false;
    }

    RESTOREPOINTINFOW_LOCAL rpiEnd = { 0 };
    rpiEnd.dwEventType = 101; // END_SYSTEM_CHANGE
    rpiEnd.dwRestorePtType = 13; // CANCELLED_OPERATION
    rpiEnd.llSequenceNumber = sequenceNumber;

    STATEMGRSTATUS_LOCAL sms = { 0 };
    const BOOL ok = pfnSRSetRestorePointW(&rpiEnd, &sms);
    return (ok == TRUE);
}

} // namespace PrivatizeWin
