#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <string_view>
#include <functional>
#include <chrono>
#include <sstream>
#include <windows.h>
#include <filesystem>
#include <atomic>

namespace PrivatizeWin::Test {

// C++ Core Guidelines:
// F.16: Pass string types by string_view
// I.10: Use [[nodiscard]]

struct TestCase {
    std::string name;
    std::string suite;
    std::function<void()> func;
};

class TestRegistry {
public:
    static TestRegistry& Instance() noexcept {
        static TestRegistry reg;
        return reg;
    }

    void Register(std::string_view suite, std::string_view name, std::function<void()> func) {
        m_tests.push_back({ std::string(name), std::string(suite), std::move(func) });
    }

    [[nodiscard]] int Run(std::string_view filter = "") {
        int passed = 0;
        int failed = 0;
        int matched = 0;

        std::cout << "\n========================================================\n";
        std::cout << "  PrivatizeWin Automated Test Runner\n";
        if (!filter.empty()) {
            std::cout << "  Filter: " << filter << "\n";
        }
        std::cout << "  Total Registered Tests: " << m_tests.size() << "\n";
        std::cout << "========================================================\n\n";

        const auto totalStart = std::chrono::high_resolution_clock::now();

        std::string currentSuite;
        for (const auto& test : m_tests) {
            const std::string fullName = test.suite + "." + test.name;
            if (!filter.empty()) {
                if (test.suite.find(filter) == std::string::npos &&
                    test.name.find(filter) == std::string::npos &&
                    fullName.find(filter) == std::string::npos) {
                    continue;
                }
            }

            matched++;
            if (test.suite != currentSuite) {
                currentSuite = test.suite;
                std::cout << "\n[" << currentSuite << "]\n";
            }

            std::cout << "  - " << test.name << " ... ";
            const auto start = std::chrono::high_resolution_clock::now();

            try {
                test.func();
                const auto end = std::chrono::high_resolution_clock::now();
                const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                std::cout << "[PASS] (" << ms << " ms)\n";
                passed++;
            } catch (const std::exception& ex) {
                const auto end = std::chrono::high_resolution_clock::now();
                const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                std::cout << "[FAIL] (" << ms << " ms)\n";
                std::cout << "      Error: " << ex.what() << "\n";
                failed++;
            } catch (...) {
                std::cout << "[FAIL] (Unknown exception thrown)\n";
                failed++;
            }
        }

        const auto totalEnd = std::chrono::high_resolution_clock::now();
        const auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(totalEnd - totalStart).count();

        std::cout << "\n--------------------------------------------------------\n";
        std::cout << "Test Summary: " << passed << " passed, " << failed << " failed, "
                  << matched << " matched (" << totalMs << " ms)\n";
        std::cout << "--------------------------------------------------------\n";

        if (matched == 0 && !filter.empty()) {
            std::cout << "Warning: No tests matched filter '" << filter << "'\n";
            return 1;
        }

        return (failed == 0) ? 0 : 1;
    }

    [[nodiscard]] int RunAll() {
        return Run("");
    }

private:
    TestRegistry() = default;
    std::vector<TestCase> m_tests;
};

class TestRegistrar {
public:
    TestRegistrar(std::string_view suite, std::string_view name, std::function<void()> func) {
        TestRegistry::Instance().Register(suite, name, std::move(func));
    }
};

class TestAssertionException : public std::runtime_error {
public:
    explicit TestAssertionException(const std::string& msg) : std::runtime_error(msg) {}
};

inline void AssertTrue(bool condition, std::string_view expr, std::string_view file, int line) {
    if (!condition) {
        std::ostringstream ss;
        ss << "Assertion failed: (" << expr << ") at " << file << ":" << line;
        throw TestAssertionException(ss.str());
    }
}

#include <utility>

template <typename T1, typename T2>
void AssertEq(const T1& a, const T2& b, std::string_view exprA, std::string_view exprB, std::string_view file, int line) {
    bool equal = false;
    if constexpr (std::is_integral_v<T1> && std::is_integral_v<T2>) {
        equal = std::cmp_equal(a, b);
    } else {
        equal = (a == b);
    }
    if (!equal) {
        std::ostringstream ss;
        ss << "Equality failed: " << exprA << " == " << exprB << " at " << file << ":" << line;
        throw TestAssertionException(ss.str());
    }
}

template <typename T1, typename T2>
void AssertNe(const T1& a, const T2& b, std::string_view exprA, std::string_view exprB, std::string_view file, int line) {
    bool notEqual = false;
    if constexpr (std::is_integral_v<T1> && std::is_integral_v<T2>) {
        notEqual = std::cmp_not_equal(a, b);
    } else {
        notEqual = (a != b);
    }
    if (!notEqual) {
        std::ostringstream ss;
        ss << "Inequality failed: " << exprA << " != " << exprB << " at " << file << ":" << line;
        throw TestAssertionException(ss.str());
    }
}

class TestTempDirectory {
public:
    TestTempDirectory() : TestTempDirectory(L"") {}

    explicit TestTempDirectory(std::wstring_view overrideBasePath) {
        std::wstring tempBase;
        if (!overrideBasePath.empty()) {
            tempBase = std::wstring(overrideBasePath);
        } else {
            wchar_t buf[MAX_PATH];
            DWORD len = GetTempPathW(MAX_PATH, buf);
            if (len == 0 || len > MAX_PATH) {
                wcscpy_s(buf, L"C:\\Windows\\Temp");
            }
            tempBase = buf;
        }

        static std::atomic<uint64_t> s_counter{ 0 };
        const auto counterVal = ++s_counter;
        const auto pid = GetCurrentProcessId();
        const auto tick = GetTickCount64();

        m_path = tempBase;
        if (!m_path.empty() && m_path.back() != L'\\') {
            m_path += L'\\';
        }
        m_path += L"privatizewin_test_" + std::to_wstring(pid) + L"_" + std::to_wstring(tick) + L"_" + std::to_wstring(counterVal);

        if (!CreateDirectoryW(m_path.c_str(), nullptr)) {
            m_path.clear();
            throw TestAssertionException("Failed to create temporary directory for test fixture: " + std::to_string(GetLastError()));
        }
    }

    ~TestTempDirectory() {
        Cleanup();
    }

    TestTempDirectory(const TestTempDirectory&) = delete;
    TestTempDirectory& operator=(const TestTempDirectory&) = delete;
    TestTempDirectory(TestTempDirectory&& other) noexcept : m_path(std::move(other.m_path)) {
        other.m_path.clear();
    }
    TestTempDirectory& operator=(TestTempDirectory&& other) noexcept {
        if (this != &other) {
            Cleanup();
            m_path = std::move(other.m_path);
            other.m_path.clear();
        }
        return *this;
    }

    [[nodiscard]] bool IsValid() const noexcept { return !m_path.empty(); }
    [[nodiscard]] const std::wstring& GetPath() const noexcept { return m_path; }
    [[nodiscard]] std::wstring GetFilePath(const std::wstring& filename) const {
        if (m_path.empty()) {
            throw TestAssertionException("Attempted to construct file path from invalid or uninitialized TestTempDirectory fixture");
        }
        return m_path + L"\\" + filename;
    }

    void Cleanup() {
        if (!m_path.empty()) {
            std::error_code ec;
            std::filesystem::remove_all(m_path, ec);
            m_path.clear();
        }
    }

private:
    std::wstring m_path;
};

} // namespace PrivatizeWin::Test

#define TEST_CASE(suite, name) \
    static void test_##suite##_##name(); \
    static const ::PrivatizeWin::Test::TestRegistrar reg_##suite##_##name(#suite, #name, test_##suite##_##name); \
    static void test_##suite##_##name()

#define ASSERT_TRUE(expr) ::PrivatizeWin::Test::AssertTrue((expr), #expr, __FILE__, __LINE__)
#define ASSERT_FALSE(expr) ::PrivatizeWin::Test::AssertTrue(!(expr), "!" #expr, __FILE__, __LINE__)
#define ASSERT_EQ(a, b) ::PrivatizeWin::Test::AssertEq((a), (b), #a, #b, __FILE__, __LINE__)
#define ASSERT_NE(a, b) ::PrivatizeWin::Test::AssertNe((a), (b), #a, #b, __FILE__, __LINE__)
