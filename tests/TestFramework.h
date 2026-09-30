#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <string_view>
#include <functional>
#include <chrono>
#include <sstream>

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

    [[nodiscard]] int RunAll() {
        int passed = 0;
        int failed = 0;

        std::cout << "\n========================================================\n";
        std::cout << "  PrivatizeWin Automated Test Runner\n";
        std::cout << "  Total Registered Tests: " << m_tests.size() << "\n";
        std::cout << "========================================================\n\n";

        const auto totalStart = std::chrono::high_resolution_clock::now();

        std::string currentSuite;
        for (const auto& test : m_tests) {
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
                  << m_tests.size() << " total (" << totalMs << " ms)\n";
        std::cout << "--------------------------------------------------------\n";

        return (failed == 0) ? 0 : 1;
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

} // namespace PrivatizeWin::Test

#define TEST_CASE(suite, name) \
    static void test_##suite##_##name(); \
    static const ::PrivatizeWin::Test::TestRegistrar reg_##suite##_##name(#suite, #name, test_##suite##_##name); \
    static void test_##suite##_##name()

#define ASSERT_TRUE(expr) ::PrivatizeWin::Test::AssertTrue((expr), #expr, __FILE__, __LINE__)
#define ASSERT_FALSE(expr) ::PrivatizeWin::Test::AssertTrue(!(expr), "!" #expr, __FILE__, __LINE__)
#define ASSERT_EQ(a, b) ::PrivatizeWin::Test::AssertEq((a), (b), #a, #b, __FILE__, __LINE__)
#define ASSERT_NE(a, b) ::PrivatizeWin::Test::AssertNe((a), (b), #a, #b, __FILE__, __LINE__)
