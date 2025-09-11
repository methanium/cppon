#include "pch.h"
#include <cppon/c++on.h>

TEST(CPPON_Smoke, EvalEmptyReturnsNull) {
    auto v = ch5::eval("");
    EXPECT_TRUE(v.is_null());
}