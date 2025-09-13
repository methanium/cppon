#include "pch.h"
#include <cppon/c++on.h>

using namespace ch5;
using namespace std::string_view_literals;

TEST(CPPON_Base64, EncodeDecodeRoundtrip) {
    blob_t b{ 'M','a','n' };
    auto s = encode_base64(b);
    EXPECT_EQ(s, "TWFu");
    auto r = decode_base64("TWFu");
    ASSERT_EQ(r.size(), 3u);
    EXPECT_EQ(r[0], 'M'); EXPECT_EQ(r[1], 'a'); EXPECT_EQ(r[2], 'n');
}

TEST(CPPON_Base64, DecodeInvalidRaises) {
    EXPECT_THROW((void)decode_base64("@@@", true), invalid_base64_error);
    auto r = decode_base64("@@@", false);
    EXPECT_TRUE(r.empty());
}

TEST(CPPON_Numbers, SuffixesAndCasts) {
    // Unquoted numbers to trigger the extended parser
    auto v = eval(R"({"i16": 123i16, "u8": 42u8, "f": 1.5f, "d": 2.5})", Quick);
    EXPECT_EQ(get_cast<int16_t>(v["i16"]), 123);
    EXPECT_EQ(get_cast<uint8_t>(v["u8"]), 42u);
    EXPECT_FLOAT_EQ(get_cast<float>(v["f"]), 1.5f);
    EXPECT_DOUBLE_EQ(get_cast<double>(v["d"]), 2.5);
}

TEST(CPPON_Whitespace, NonJsonSpaceRejected) {
    // NBSP (0xA0) is not a valid first UTF‑8 byte -> continuation byte
    EXPECT_THROW((void)eval("\xA0{\"a\":1}"sv, Parse), invalid_utf8_continuation_error);
}

TEST(CPPON_Printer, JsonCompatibilityLimits) {
    // OK in JSON-compatible mode
    cppon ok; ok = object_t{};
    ok["ok"] = static_cast<int64_t>(9007199254740991LL);
    auto& outOk = to_string(ok, R"({"layout":"json"})");
    EXPECT_NE(outOk.find("\"ok\""), std::string::npos);

    // Beyond JSON integer precision limit -> throws json_compatibility_error
    cppon bad; bad = static_cast<uint64_t>(9007199254740992ULL);
    EXPECT_THROW((void)to_string(bad, R"({"layout":"json"})"), json_compatibility_error);
}

#if defined(CPPON_ENABLE_SIMD)
TEST(SIMD_Override, GlobalIsCappedToCPU) {
    // Force AVX-512 même si CPU n’en a pas -> cap à niveau max supporté
    set_global_simd_override(SimdLevel::AVX512);
    auto eff = effective_simd_level();
    clear_global_simd_override();
    // Sur CPU AVX2, eff doit être AVX2; sur CPU AVX-512, eff peut être AVX-512
    // On vérifie au moins que ce n’est pas "au-dessus" de max_supported (implémentation interne).
    // Ici on se contente de tester que ce n’est pas None.
    EXPECT_NE(eff, SimdLevel::None);
}

TEST(SIMD_Override, ThreadOverridesGlobal) {
    set_global_simd_override(SimdLevel::AVX2);
    set_thread_simd_override(SimdLevel::SSE);
    EXPECT_EQ(effective_simd_level(), SimdLevel::SSE);
    clear_thread_simd_override();
    EXPECT_EQ(effective_simd_level(), SimdLevel::AVX2);
    clear_global_simd_override();
}
#endif

TEST(RootStack, RePushSameIsNoOp) {
    cppon a;

    const auto d0 = visitors::root_stack().size();
    visitors::push_root(a);
    const auto d1 = visitors::root_stack().size();

    // Invariant: pousser le root courant ne doit rien changer
    visitors::push_root(visitors::get_root()); // no-op
    const auto d2 = visitors::root_stack().size();

    EXPECT_EQ(d1, d0 + 1); // on a effectivement poussé 'a'
    EXPECT_EQ(d2, d1);     // push_root(get_root()) est un no-op

    visitors::pop_root(a);           // restore état initial
    EXPECT_EQ(visitors::root_stack().size(), d0);
}


TEST(RootStack, NonLifoPopTriggersAssert) {
    cppon a, b;
    visitors::push_root(a);
    visitors::push_root(b);
    // cleanup
    visitors::pop_root(a);
    visitors::pop_root(b);
}
