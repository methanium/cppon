#include "pch.h"
#include <cppon/c++on.h>

TEST(CPPON_Parse, ParsesSimpleInt) {
    auto v = ch5::eval("{\"a\":1}");
    EXPECT_EQ(ch5::get_strict<int64_t>(v["a"]), 1);
}

TEST(CPPON_Parse, ParsesString) {
    auto v = ch5::eval("{\"s\":\"hi\"}");
    EXPECT_EQ(std::string(std::get<ch5::string_view_t>(v["s"])), "hi");
}