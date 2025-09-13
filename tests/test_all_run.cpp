#include <cppon/c++on.h>
#include "run_test.h"
#include <thread>

using namespace ch5;

int main() {
	std::cout << "=== Test basic creation of objects and arrays ===\n\n";

    run("create empty object", [] {
        cppon v; v = object_t{};
        EXPECT_TRUE(std::holds_alternative<object_t>(v));
        EXPECT_EQ(v.object().size(), 0u);
        }, true);

    run("create array and push", [] {
        cppon v; v = array_t{};
        v.array().push_back(cppon{ 1 });
        EXPECT_EQ(v.array().size(), 1u);
        EXPECT_EQ(get_cast<int>(v.array()[0]), 1);
        }, true);

	std::cout << "\n=== Test exceptions from various operations ===\n\n";

    // invalid_base64_error: invalid base64 in full mode
    run("invalid_base64_error", [] {
        (void)eval(R"({"b":"$cppon-blob:###"})", options::full);
        });

    // blob_not_realized_error: get_blob(const) on non-realized blob_string_t
    run("blob_not_realized_error", [] {
        auto doc = eval(R"({"b":"$cppon-blob:SGVsbG8="})", options::eval);
        const cppon& c = doc["/b"];
        (void)get_blob(c); // doit lever
        });

    // number_not_converted_error: get_strict<T>(const) on number_t (quick)
    run("number_not_converted_error", [] {
        const auto doc = eval(R"({"n":123})", options::quick);
        (void)get_strict<int>(doc["/n"]); // const + number_t => lève
        });

	std::cout << "\n=== Test exceptions from printer ===\n\n";

    // bad_option_error: invalid type inside "buffer"
    run("bad_option_error", [] {
        cppon doc;
        (void)to_string(doc, R"({"buffer":{"reset":123}})");
        });

    // json_compatibility_error: 64-bit integer out of JSON safe range
    run("json_compatibility_error", [] {
        cppon doc;
        doc["/ok"] = static_cast<int64_t>(9007199254740991LL);
        (void)to_string(doc, R"({"layout":{"json":true}})");
        doc["/big"] = static_cast<int64_t>(9007199254740992LL);
        (void)to_string(doc, R"({"layout":{"json":true}})");
        });

    std::cout << "\n=== Test exceptions from scanner ===\n\n";

    // unexpected_utf32_BOM_error (UTF-32 BOM big/little endian)
    run("unexpected_utf32_BOM_error (BE)", [] {
        const char s[] = "\x00\x00\xFE\xFF" "{}";
        (void)eval(s, options::quick);
            });
    run("unexpected_utf32_BOM_error (LE)", [] {
        const char s[] = "\xFF\xFE\x00\x00" "{}";
        (void)eval(s, options::quick);
        });

    // unexpected_utf16_BOM_error
    run("unexpected_utf16_BOM_error (BE)", [] {
        const char s[] = "\xFE\xFF" "{}";
        (void)eval(s, options::quick);
        });
    run("unexpected_utf16_BOM_error (LE)", [] {
        const char s[] = "\xFF\xFE" "{}";
        (void)eval(s, options::quick);
        });

    // invalid_utf8_sequence_error (0xF8..0xFD never valid)
    run("invalid_utf8_sequence_error", [] {
        const char s[] = "\xF8" "{}";
        (void)eval(s, options::quick);
        });

    // invalid_utf8_continuation_error (leading continuation byte 10xxxxxx)
    run("invalid_utf8_continuation_error", [] {
        const char s[] = "\x80" "{}";
        (void)eval(s, options::quick);
        });

    // unexpected_end_of_text_error (unterminated string)
    run("unexpected_end_of_text_error", [] {
        (void)eval("\"abc", options::quick);
        });

    // unexpected_symbol_error
    run("unexpected_symbol_error", [] {
        (void)eval("]", options::quick);
        });

    // expected_symbol_error (missing ':')
    run("expected_symbol_error", [] {
        (void)eval("{\"a\" 1}", options::quick);
        });

    // --------------------------------------------------
    // Cas VALIDE (doivent réussir sans exception)
    // --------------------------------------------------

	std::cout << "\n=== Test valid inputs (should not throw) ===\n\n";

    run("valid empty object", [] {
        (void)eval("{}", options::quick);
        }, true);

    run("valid empty array", [] {
        (void)eval("[]", options::quick);
        }, true);

    run("valid UTF-8 BOM accepted", [] {
        (void)eval("\xEF\xBB\xBF{\"a\":1}", options::quick);
        }, true);

    run("valid multibyte utf8", [] {
        (void)eval(u8"{\"é\":\"à\"}", options::quick);
        }, true);

    run("valid null literal", [] {
        auto v = eval("null", options::quick);
        (void)std::get<nullptr_t>(v);
        }, true);

    run("valid number literal", [] {
        auto v = eval("12345", options::quick);
        (void)get_cast<int>(v);
        }, true);

    run("valid leading spaces", [] {
        auto v = eval("   {\"x\":1}", options::quick);
        (void)get_cast<int>(v["/x"]);
        }, true);

    run("valid leading tabs/newlines", [] {
        (void)eval("\t\n\r {\"x\":1}", options::quick);
        }, true);

    run("valid array with leading spaces", [] {
        (void)eval("   [1,2,3]", options::quick);
        }, true);

    run("valid UTF-8 BOM + leading spaces", [] {
        (void)eval("\xEF\xBB\xBF   {\"y\":2}", options::quick);
        }, true);

	std::cout << "\n=== Test parsing of simple objects and arrays (should not throw) ===\n\n";

    run("parse simple object", [] {
        auto v = eval(R"({"a":1,"s":"x"})", options::quick);
        EXPECT_TRUE(std::holds_alternative<object_t>(v));
        EXPECT_EQ(get_cast<int>(v["/a"]), 1);
        EXPECT_EQ(std::get<string_view_t>(v["/s"]), "x");
        }, true);

    run("parse array", [] {
        auto v = eval(R"([1,2,3])", options::quick);
        EXPECT_TRUE(std::holds_alternative<array_t>(v));
        EXPECT_EQ(get_cast<int>(v[1]), 2);
        }, true);

	std::cout << "\n=== Test path traversal and member access (should not throw) ===\n\n";

    run("absolute path access", [] {
        auto v = eval(R"({"a":{"b":2}})", options::quick);
        EXPECT_EQ(get_cast<int>(v["/a/b"]), 2);
        }, true);

    run("missing member returns null (const object)", [] {
        const auto v = eval(R"({"a":{}})", options::quick);
        const auto& n = v["/a/missing"];
        EXPECT_TRUE(n.is_null());
        }, true);

	std::cout << "\n=== Test special types (path_t, blob_string_t) and their behavior ===\n\n";

    run("path reference resolves through traversal", [] {
        // r is a path_t to /t ; /r/v access calls deref then visits 'v'
        auto v = eval(R"({"t":{"v":3},"r":"$cppon-path:/t"})", options::quick);
        EXPECT_EQ(get_cast<int>(v["/r/v"]), 3);
        }, true);

    run("blob prefix stays as blob_string in quick", [] {
        auto v = eval(R"({"b":"$cppon-blob:SGVsbG8="})", options::quick);
        auto& b = v["/b"];
        // get_blob(const) must throw if not realized
        EXPECT_THROW((void)get_blob(static_cast<const cppon&>(b)), blob_not_realized_error);
        // realization via non-const
        EXPECT_NO_THROW((void)get_blob(b));
        }, true);

	std::cout << "\n=== Test printer exceptions and JSON compatibility mode ===\n\n";

    run("to_string json layout", [] {
        cppon v; v = object_t{};
        v["ok"] = static_cast<int64_t>(42);
        auto out = to_string_view(v, R"({"layout":"json"})");
        //EXPECT_NE(out.find("\"ok\""), std::string::npos);
        //EXPECT_NE(out.find("42"), std::string::npos);
        }, true);

    run("json compatibility error for big integer", [] {
        cppon v; v = static_cast<uint64_t>(9007199254740992ULL);
        (void)to_string_view(v, R"({"layout":"json"})");
        });

	std::cout << "\n=== Test thread-local root stacks and isolation between threads (should not throw) ===\n\n";

    run("thread-local roots do not interfere", [] {
        const auto d0 = visitors::root_stack().size();

        std::thread th([] {
            // Absolute path -> push_root(*this) via operator[]
            auto v = eval(R"({"x":1})", options::quick);
            (void)get_cast<int>(v["/x"]);
            });
        th.join();

		// Current thread's stack didn't change
        EXPECT_EQ(visitors::root_stack().size(), d0);
        }, true);

    run("two threads isolated root stacks", [] {
        const auto main_d0 = visitors::root_stack().size();

        std::exception_ptr e1, e2;

        std::thread t1([&] {
            try {
                const auto d0 = visitors::root_stack().size();
                for (int i = 0; i < 64; ++i) {
                    auto v = eval(R"({"a":{"b":1},"arr":[0,1,2]})", options::quick);
                    (void)get_cast<int>(v["/a/b"]);
                    (void)get_cast<int>(v["/arr/2"]);
                }
                if (visitors::root_stack().size() != d0)
                    throw std::runtime_error("root stack leak in t1");
            }
            catch (...) {
                e1 = std::current_exception();
            }
            });

        std::thread t2([&] {
            try {
                const auto d0 = visitors::root_stack().size();
                for (int i = 0; i < 64; ++i) {
                    auto v = eval(R"({"x":1,"r":"$cppon-path:/x"})", options::quick);
                    (void)get_cast<int>(v["/x"]);
                    (void)get_cast<int>(v["/r"]); // path deref -> push/pop root via traversal
                }
                if (visitors::root_stack().size() != d0)
                    throw std::runtime_error("root stack leak in t2");
            }
            catch (...) {
                e2 = std::current_exception();
            }
            });

        t1.join();
        t2.join();

        if (e1) std::rethrow_exception(e1);
        if (e2) std::rethrow_exception(e2);

		// Main thread's stack didn't change
        EXPECT_EQ(visitors::root_stack().size(), main_d0);
        }, true);

    std::cout << "\n=== Test exceptions from path traversal and pointer assignment ===\n\n";

    // member_not_found_error (object, missing non-terminal segment)
    run("member_not_found_error", [] {
        const cppon doc; // objet vide
        (void)doc["/missing/path"]; // non terminal 'missing' absent => lève
        });

    // null_value_error (path continues through null)
    run("null_value_error", [] {
        cppon doc{ eval(R"({"arr":[null]})") };                // {}
        (void)static_cast<const cppon&>(doc)["/arr/0/next"]; // arr[0] => null then /next => throws
        });

    // type_mismatch_error (path stepping onto scalar)
    run("type_mismatch_error", [] {
        cppon doc;
        doc["/x"] = 1;
        (void)static_cast<const cppon&>(doc)["/x/y"]; // 'x' is not an objet/array
        });

    // bad_array_index_error (non-numeric segment inside array)
    run("bad_array_index_error", [] {
        cppon doc;
        doc["/arr"] = array_t{};   // []
		(void)static_cast<const cppon&>(doc)["/arr/foo"]; // 'foo' not numeric
        });

    // excessive_array_resize_error (indexed write far beyond size)
    run("excessive_array_resize_error", [] {
        cppon doc;
        // index >> size + CPPON_MAX_ARRAY_DELTA (default 256)
        doc["/arr/100000"] = 1; // devrait lever
        });


    run("unsafe_pointer_assignment_error", [] {
        cppon doc;
        doc["/a"] = 42; // target
        doc["/p"] = &doc["/a"];
        });

    // Safe pattern: sequence slot creation then pointer_t assignment
    run("safe_pointer_assignment (no throw)", [] {
        cppon doc;
        doc["/a"] = 42;
        auto& slot = doc["/p"];     // sequenced, no evaluation ambiguity
        slot = &doc["/a"];          // no exception
        // vérif rapide
        const auto& p = doc["/p"];  // pointer dereferenced immediately: result = 42
        if (!std::holds_alternative<pointer_t>(p) || std::get<pointer_t>(p) != &doc["/a"])
            throw std::runtime_error("pointer mismatch");
        }, true);

    return test_succeeded();
}