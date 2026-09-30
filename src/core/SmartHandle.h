#pragma once

#include <windows.h>
#include <utility>

namespace PrivatizeWin {

// C++ Core Guidelines R.1: Make interfaces that manage resources RAII
// C++ Core Guidelines R.10: Avoid malloc() and free() / raw handle management

// RAII wrapper for general Win32 HANDLE
class UniqueHandle {
public:
    UniqueHandle() noexcept : m_handle(nullptr) {}
    explicit UniqueHandle(HANDLE h) noexcept : m_handle(h) {}

    ~UniqueHandle() noexcept {
        reset();
    }

    // Rule of Five: Move-only
    UniqueHandle(const UniqueHandle&) = delete;
    UniqueHandle& operator=(const UniqueHandle&) = delete;

    UniqueHandle(UniqueHandle&& other) noexcept : m_handle(other.m_handle) {
        other.m_handle = nullptr;
    }

    UniqueHandle& operator=(UniqueHandle&& other) noexcept {
        if (this != &other) {
            reset(other.m_handle);
            other.m_handle = nullptr;
        }
        return *this;
    }

    [[nodiscard]] HANDLE get() const noexcept { return m_handle; }
    [[nodiscard]] bool isValid() const noexcept { return m_handle != nullptr && m_handle != INVALID_HANDLE_VALUE; }
    explicit operator bool() const noexcept { return isValid(); }

    HANDLE* put() noexcept {
        reset();
        return &m_handle;
    }

    HANDLE release() noexcept {
        HANDLE h = m_handle;
        m_handle = nullptr;
        return h;
    }

    void reset(HANDLE h = nullptr) noexcept {
        if (isValid()) {
            CloseHandle(m_handle);
        }
        m_handle = h;
    }

private:
    HANDLE m_handle{ nullptr };
};

// RAII wrapper for Windows Registry HKEY
class UniqueHKey {
public:
    UniqueHKey() noexcept : m_hKey(nullptr) {}
    explicit UniqueHKey(HKEY h) noexcept : m_hKey(h) {}

    ~UniqueHKey() noexcept {
        reset();
    }

    UniqueHKey(const UniqueHKey&) = delete;
    UniqueHKey& operator=(const UniqueHKey&) = delete;

    UniqueHKey(UniqueHKey&& other) noexcept : m_hKey(other.m_hKey) {
        other.m_hKey = nullptr;
    }

    UniqueHKey& operator=(UniqueHKey&& other) noexcept {
        if (this != &other) {
            reset(other.m_hKey);
            other.m_hKey = nullptr;
        }
        return *this;
    }

    [[nodiscard]] HKEY get() const noexcept { return m_hKey; }
    [[nodiscard]] bool isValid() const noexcept { return m_hKey != nullptr; }
    explicit operator bool() const noexcept { return isValid(); }

    HKEY* put() noexcept {
        reset();
        return &m_hKey;
    }

    HKEY release() noexcept {
        HKEY h = m_hKey;
        m_hKey = nullptr;
        return h;
    }

    void reset(HKEY h = nullptr) noexcept {
        if (m_hKey != nullptr) {
            RegCloseKey(m_hKey);
            m_hKey = nullptr;
        }
        m_hKey = h;
    }

private:
    HKEY m_hKey{ nullptr };
};

// RAII wrapper for Windows Service Control Manager SC_HANDLE
class UniqueScHandle {
public:
    UniqueScHandle() noexcept : m_handle(nullptr) {}
    explicit UniqueScHandle(SC_HANDLE h) noexcept : m_handle(h) {}

    ~UniqueScHandle() noexcept {
        reset();
    }

    UniqueScHandle(const UniqueScHandle&) = delete;
    UniqueScHandle& operator=(const UniqueScHandle&) = delete;

    UniqueScHandle(UniqueScHandle&& other) noexcept : m_handle(other.m_handle) {
        other.m_handle = nullptr;
    }

    UniqueScHandle& operator=(UniqueScHandle&& other) noexcept {
        if (this != &other) {
            reset(other.m_handle);
            other.m_handle = nullptr;
        }
        return *this;
    }

    [[nodiscard]] SC_HANDLE get() const noexcept { return m_handle; }
    [[nodiscard]] bool isValid() const noexcept { return m_handle != nullptr; }
    explicit operator bool() const noexcept { return isValid(); }

    SC_HANDLE* put() noexcept {
        reset();
        return &m_handle;
    }

    SC_HANDLE release() noexcept {
        SC_HANDLE h = m_handle;
        m_handle = nullptr;
        return h;
    }

    void reset(SC_HANDLE h = nullptr) noexcept {
        if (m_handle != nullptr) {
            CloseServiceHandle(m_handle);
            m_handle = nullptr;
        }
        m_handle = h;
    }

private:
    SC_HANDLE m_handle{ nullptr };
};

} // namespace PrivatizeWin
