#include "../TestFramework.h"
#include "../../src/core/SmartHandle.h"

using namespace PrivatizeWin;

TEST_CASE(Unit_SmartHandle, DefaultState) {
    UniqueHandle h;
    ASSERT_FALSE(h.isValid());
    ASSERT_EQ(h.get(), nullptr);

    UniqueHKey hk;
    ASSERT_FALSE(hk.isValid());
    ASSERT_EQ(hk.get(), nullptr);
}

TEST_CASE(Unit_SmartHandle, MoveSemantics) {
    HANDLE hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    ASSERT_TRUE(hEvent != nullptr);

    UniqueHandle h1(hEvent);
    ASSERT_TRUE(h1.isValid());
    ASSERT_EQ(h1.get(), hEvent);

    // Move construct
    UniqueHandle h2(std::move(h1));
    ASSERT_FALSE(h1.isValid());
    ASSERT_TRUE(h2.isValid());
    ASSERT_EQ(h2.get(), hEvent);

    // Move assign
    UniqueHandle h3;
    h3 = std::move(h2);
    ASSERT_FALSE(h2.isValid());
    ASSERT_TRUE(h3.isValid());
    ASSERT_EQ(h3.get(), hEvent);
}

TEST_CASE(Unit_SmartHandle, ReleaseAndReset) {
    HANDLE hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    UniqueHandle h(hEvent);

    HANDLE released = h.release();
    ASSERT_FALSE(h.isValid());
    ASSERT_EQ(released, hEvent);

    // Clean up manually since released
    CloseHandle(released);

    // Reset with new event
    HANDLE hEvent2 = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    h.reset(hEvent2);
    ASSERT_TRUE(h.isValid());
    ASSERT_EQ(h.get(), hEvent2);

    h.reset();
    ASSERT_FALSE(h.isValid());
}
