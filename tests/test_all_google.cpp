#include "pch.h"
using namespace ch5;
using namespace ch5::literals;
using namespace std::string_view_literals;

TEST(CPPON_Parse, ParsesSimpleInt) {
    auto v = ch5::eval("{\"a\":1}");
    EXPECT_EQ(ch5::get_strict<int64_t>(v["a"]), 1);
}

TEST(CPPON_Parse, ParsesString) {
    auto v = ch5::eval("{\"s\":\"hi\"}");
    EXPECT_EQ(std::string(std::get<ch5::string_view_t>(v["s"])), "hi");
}

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
    auto outOk = to_string_view(ok, R"({"layout":"json"})");
    EXPECT_NE(outOk.find("\"ok\""), std::string::npos);

    // Beyond JSON integer precision limit -> throws json_compatibility_error
    cppon bad; bad = static_cast<uint64_t>(9007199254740992ULL);
    EXPECT_THROW((void)to_string(bad, R"({"layout":"json"})"), json_compatibility_error);
}

TEST(SIMD_Override, GlobalIsCappedToCPU) {
    // Force AVX-512 even if CPU lacks it -> capped to highest supported level
    set_global_simd_override(SimdLevel::AVX512);
    auto eff = effective_simd_level();
    clear_global_simd_override();
    // On AVX2 CPUs, eff should be AVX2; on AVX-512 CPUs it may be AVX-512.
    // Minimal assertion: it must not be SimdLevel::None.
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

TEST(RootStack, RePushSameIsNoOp) {
    cppon a;

    const auto d0 = visitors::root_stack().size();
    visitors::push_root(a);
    const auto d1 = visitors::root_stack().size();

    visitors::push_root(visitors::get_root()); // no-op
    const auto d2 = visitors::root_stack().size();

    EXPECT_EQ(d1, d0 + 1);
    EXPECT_EQ(d2, d1);

    visitors::pop_root(a);
    EXPECT_EQ(visitors::root_stack().size(), d0);
}

TEST(UDL, JsonQuickAndFull) {
    auto q = R"({"n":1,"s":"x"})"_json;
    auto f = R"({"n":1,"s":"x"})"_jsonf;
    EXPECT_TRUE(std::holds_alternative<object_t>(q));
    EXPECT_TRUE(std::holds_alternative<object_t>(f));
    // quick: lazy number possible (number_t or concrete, depending on options)
    // full: eager numeric conversion done
    auto& nq = q["/n"];
    auto& nf = f["/n"];
    EXPECT_NO_THROW((void)get_cast<int32_t>(nq));
    EXPECT_NO_THROW((void)get_strict<int64_t>(f["/n"]));
    EXPECT_EQ(std::get<string_view_t>(q["/s"]), "x");
    EXPECT_EQ(std::get<string_view_t>(f["/s"]), "x");
}

TEST(UDL, OptionsAndToString) {
    auto doc = R"({"a":1,"b":2})"_json;
    auto opts = R"({"pretty":true})"_opts;
    auto out = to_string_view(doc, opts);
    EXPECT_NE(out.find('\n'), std::string::npos); // pretty = contains newline
}

TEST(UDL, PathAndBlob) {
    cppon doc;
    doc["/img/format"] = "png";
    // base64: blob_string_t
    doc["/img/data64"] = "QUJD"_b64;
    // raw binary
    doc["/img/data"] = "\x01\x02""ABC"_blob;
    // path_t
    doc["/ref"] = "/img/format"_path;

    EXPECT_TRUE(std::holds_alternative<blob_string_t>(doc["/img/data64"]));
    EXPECT_TRUE(std::holds_alternative<blob_t>(doc["/img/data"]));
    EXPECT_TRUE(std::holds_alternative<path_t>(doc["/ref"]));

    // Sérialize a sub-object (compact by default)
    auto json = to_string_view(doc["/img"]);
    EXPECT_NE(json.find("\"format\""), std::string::npos);
}

TEST(RootStack, NonLifoPopNoFail) {
    cppon a, b;
    visitors::push_root(a);
    visitors::push_root(b);

    visitors::pop_root(a);
    visitors::pop_root(b);
}

TEST(PathThroughPointer, AutovivifyViaPointerPath) {
    ch5::cppon root;
    // Autovivify /array/2 = null
    (void)root["/array/2"];
    // pointer[3] -> /array/2
    root["/pointer/3"] = &root["/array/2"];
    // Writing through pointer: must autovivify /array/2 into object_t and create member3
    root["/pointer/3/member3"] = "value3";

    EXPECT_TRUE(std::holds_alternative<ch5::object_t>(root["/array/2"]));
    EXPECT_EQ(std::get<ch5::string_view_t>(root["/array/2/member3"]), "value3");
}

TEST(PointerReseat, ReseatAndAutovivify) {
    ch5::cppon root;
    root["/array/0"] = 1;
    root["/object/b"] = 2;

    // /p -> /a
    root["/p"] = &root["/array/0"];
    EXPECT_EQ(ch5::get_cast<int>(root["/p"]), 1);

    // Reseat: /p -> /b
    root["/p"] = &root["/object/b"];
    EXPECT_EQ(ch5::get_cast<int>(root["/p"]), 2);

    // Prepare target for autovivification
    root["/object/b"] = nullptr;

    // Write through pointer: autovivify /b as object_t then create sub=3
    root["/p/sub"] = 3;
    EXPECT_EQ(ch5::get_cast<int>(root["/object/b/sub"]), 3);
    EXPECT_TRUE(std::holds_alternative<ch5::object_t>(root["/object/b"]));
}

TEST(PathReseat, ReseatResolvesToNewTarget) {
    ch5::cppon root;
    root["/obj1/x"] = "v1";
    root["/obj2/y"] = "v2";

    // /r = path to /obj1/x
    root["/r"] = ch5::path_t{ "/obj1/x" };
    auto p1 = ch5::get_optional<ch5::string_view_t>(root["/r"]);
    EXPECT_TRUE(p1 != nullptr);
    EXPECT_EQ(*p1, "v1");

    // Reseat: /r = path to /obj2/y
    root["/r"] = ch5::path_t{ "/obj2/y" };
    auto p2 = ch5::get_optional<ch5::string_view_t>(root["/r"]);
    EXPECT_TRUE(p2 != nullptr);
    EXPECT_EQ(*p2, "v2");
}

TEST(PointerNullPath, AutovivifyAtOriginWithoutMutatingNullSentinel) {
    using namespace ch5;

    EXPECT_TRUE(visitors::null().is_null()); // sentinel

    pointer_t pnull = nullptr;
    cppon root;
    root["/p"] = pointer_t{ nullptr }; // null pointer_t (not a null object)

    // Explicitly create a null pointer_t (distinct from null value type)

    // Descending write through the null pointer:
    // Expected: autovivification on origin slot (/p), sentinel remains unchanged.
    // Non-leaf path write: must autovivify /p (not the sentinel)
    root["/p/sub"] = 3;

    EXPECT_EQ(get_cast<int>(root["/p/sub"]), 3);
    EXPECT_TRUE(std::holds_alternative<object_t>(root["/p"]));
    EXPECT_TRUE(visitors::null().is_null()); // sentinel not promoted
}

TEST(PathT_RootAndInvalid, RootAndEmpty) {
    ch5::cppon root;
    root["/x"] = 7;
    root["/p"] = ch5::path_t{ "/" }; // points to root
    EXPECT_EQ(ch5::get_cast<int>(root["/p/x"]), 7);
    EXPECT_THROW((ch5::path_t{ "" }), ch5::invalid_path_error);
}

TEST(JSON_NumberEOT, SimdAcceptsSentinel) {
#if CPPON_USE_SIMD
    using namespace std::string_view_literals;
    // Force effective SIMD level (SSE sufficient)
    set_global_simd_override(SimdLevel::SSE);
    EXPECT_NO_THROW((void)eval("123"sv));
    clear_global_simd_override();
#else
    GTEST_SKIP() << "Scalar-only build: override no-op";
#endif
}

TEST(JSON_NumberEOT, NoSimdAcceptsSentinel) {
#if CPPON_USE_SIMD
    using namespace std::string_view_literals;
    // Force strict None fallback (reads sentinel byte via <= end)
    set_global_simd_override(SimdLevel::None);
    EXPECT_NO_THROW((void)eval("123"sv));
    clear_global_simd_override();
#else
    GTEST_SKIP() << "Scalar-only build: override no-op";
#endif
}