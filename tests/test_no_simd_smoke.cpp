#define CPPON_NO_SIMD
#include <cppon/c++on.h>
#include "run_test.h"

using namespace ch5;

int main() {
#if CPPON_USE_SIMD
    std::cout << "[FAIL] build config -> Ce binaire n'est pas NO_SIMD (CPPON_USE_SIMD=1)\n";
    return -1;
#endif

    run("NO_SIMD: effective level is None", [] {
        if (effective_simd_level() != scanner::SimdLevel::None)
            throw std::runtime_error("effective_simd_level() != None");
        }, true);

    run("NO_SIMD: overrides are no-op", [] {
        set_global_simd_override(scanner::SimdLevel::AVX2);
        if (effective_simd_level() != scanner::SimdLevel::None)
            throw std::runtime_error("global override changed level");
        set_thread_simd_override(scanner::SimdLevel::SSE);
        if (effective_simd_level() != scanner::SimdLevel::None)
            throw std::runtime_error("thread override changed level");
        clear_thread_simd_override();
        clear_global_simd_override();
        }, true);

    run("NO_SIMD: basic parsing", [] {
        auto doc = eval(R"({"k":1,"s":"v"})", options::eval);
        if (!std::holds_alternative<object_t>(doc)) throw std::runtime_error("not an object");
        if (get_cast<int>(doc["/k"]) != 1) throw std::runtime_error("k!=1");
        if (std::get<string_view_t>(doc["/s"]) != "v") throw std::runtime_error("s!=v");
        }, true);

    return test_succeeded();
}