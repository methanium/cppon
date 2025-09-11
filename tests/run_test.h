#ifndef RUN_TEST_H
#define RUN_TEST_H

#include <iostream>
#include <exception>
#include <sstream>
#include <utility>
#include <vector>
#include <string>
#include <cmath>

inline int failures = 0;
inline int passed = 0;
inline int total = 0;

inline auto run = [](const char* title, auto&& fn, bool must_succeed = false) {
    ++total;
    try {
        std::forward<decltype(fn)>(fn)();
        if (must_succeed) {
            std::cout << "[OK]   " << title << " did not throw\n";
            ++passed;
        }
        else {
            std::cout << "[FAIL] " << title << " did not throw\n";
            ++failures;
        }
    }
    catch (const std::exception& e) {
        if (must_succeed) {
            std::cout << "[FAIL] " << title << "  -> did throw " << e.what() << "\n";
            ++failures;
        }
        else {
            std::cout << "[OK]   " << title << "  -> did throw " << e.what() << "\n";
            ++passed;
        }
    }
    };

// Assertions style gtest
inline void rt_fail(const char* msg, const char* file, int line) {
    std::ostringstream os; os << file << ":" << line << " - " << msg;
    throw std::runtime_error(os.str());
}
inline void rt_expect_true(bool cond, const char* expr, const char* file, int line) {
    if (!cond) { std::ostringstream os; os << "EXPECT_TRUE(" << expr << ") failed";
        rt_fail(os.str().c_str(), file, line);
    }
}
template<typename L, typename R>
inline void rt_expect_eq(const L& lhs, const R& rhs, const char* ls, const char* rs, const char* file, int line) {
    if (!(lhs == rhs)) {std::ostringstream os;os << "EXPECT_EQ(" << ls << ", " << rs << ") failed: lhs != rhs";
        rt_fail(os.str().c_str(), file, line);
    }
}
template<typename L, typename R>
inline void rt_expect_ne(const L& lhs, const R& rhs, const char* ls, const char* rs, const char* file, int line) {
    if (!(lhs != rhs)) {std::ostringstream os;os << "EXPECT_NE(" << ls << ", " << rs << ") failed: lhs == rhs";
        rt_fail(os.str().c_str(), file, line);
    }
}
inline void rt_expect_streq(std::string_view a, std::string_view b, const char* as, const char* bs, const char* file, int line) {
    if (a != b) {std::ostringstream os;os << "EXPECT_STREQ(" << as << ", " << bs << ") failed: \"" << a << "\" != \"" << b << "\"";
        rt_fail(os.str().c_str(), file, line);
    }
}
inline void rt_expect_strne(std::string_view a, std::string_view b, const char* as, const char* bs, const char* file, int line) {
    if (a == b) {std::ostringstream os;os << "EXPECT_STRNE(" << as << ", " << bs << ") failed: both are \"" << a << "\"";
        rt_fail(os.str().c_str(), file, line);
    }
}
inline int test_succeeded() {
    std::cout << "\n[SUMMARY] passed " << passed << "/" << total
        << "  |  failed " << failures << "\n";
    return failures == 0 ? 0 : 1;
}

#define RUN_SUMMARY()         ::test_succeeded()
#define EXPECT_TRUE(expr)     ::rt_expect_true((expr), #expr, __FILE__, __LINE__)
#define EXPECT_EQ(lhs, rhs)   ::rt_expect_eq((lhs), (rhs), #lhs, #rhs, __FILE__, __LINE__)
#define EXPECT_NE(lhs, rhs)   ::rt_expect_ne((lhs), (rhs), #lhs, #rhs, __FILE__, __LINE__)
#define EXPECT_STREQ(a, b)    ::rt_expect_streq((std::string_view)(a), (std::string_view)(b), #a, #b, __FILE__, __LINE__)
#define EXPECT_STRNE(a, b)    ::rt_expect_strne((std::string_view)(a), (std::string_view)(b), #a, #b, __FILE__, __LINE__)
#define EXPECT_NEAR(lhs, rhs, abs_err) EXPECT_TRUE(std::fabs((lhs) - (rhs)) <= (abs_err))
#define EXPECT_FLOAT_EQ(lhs, rhs) EXPECT_NEAR(lhs, rhs, 1e-6)
#define EXPECT_DOUBLE_EQ(lhs, rhs) EXPECT_NEAR(lhs, rhs, 1e-12)
#define ASSERT_EQ(lhs, rhs)   EXPECT_EQ(lhs, rhs)

#define EXPECT_NO_THROW(stmt) do { bool threw=false; try { stmt; } catch (...) { threw=true; } \
    if (threw) { std::ostringstream os; os << "EXPECT_NO_THROW(" #stmt ") failed: statement threw"; \
    ::rt_fail(os.str().c_str(), __FILE__, __LINE__);} } while(0)

#define EXPECT_THROW(stmt, ex_type) do { bool threw=false; try { stmt; } \
    catch (const ex_type&) { threw=true; } catch (...) {} \
    if (!threw) { std::ostringstream os; os << "EXPECT_THROW(" #stmt ", " #ex_type ") failed: statement did not throw " #ex_type; \
    ::rt_fail(os.str().c_str(), __FILE__, __LINE__);} } while(0)

#define EXPECT_DEATH(stmt, regex) do { bool threw=false; try { stmt; } catch (...) { threw=true; } \
    if (!threw) { std::ostringstream os; os << "EXPECT_DEATH(" #stmt ", " #regex ") failed: statement did not throw"; \
    ::rt_fail(os.str().c_str(), __FILE__, __LINE__);} } while(0)

// Registre de tests
struct TestEntry { const char* name; void(*fn)(); };
inline std::vector<TestEntry>& test_registry() { static std::vector<TestEntry> reg; return reg; }
struct TestRegistrar { TestRegistrar(const char* name, void(*fn)()) { test_registry().push_back({ name, fn }); } };

// Macros TEST / TEST_F (fixture simplifiée: instanciation + SetUp/TearDown optionnels)
#define TEST(suite, name) \
    static void suite##_##name(); \
    static ::TestRegistrar registrar_##suite##_##name(#suite "." #name, &suite##_##name); \
    static void suite##_##name()

#define TEST_F(Fixture, name) \
    static void Fixture##_##name##_impl(Fixture& self); \
    static void Fixture##_##name() { Fixture self; if constexpr (requires(Fixture f){ f.SetUp(); }) self.SetUp(); \
        try { Fixture##_##name##_impl(self); } catch (...) { if constexpr (requires(Fixture f){ f.TearDown(); }) self.TearDown(); throw; } \
        if constexpr (requires(Fixture f){ f.TearDown(); }) self.TearDown(); } \
    static ::TestRegistrar registrar_##Fixture##_##name(#Fixture "." #name, &Fixture##_##name); \
    static void Fixture##_##name##_impl(Fixture& self)

// Exécution du registre (utilisé par RUN_ALL_TESTS du shim gtest)
inline int run_all_registered_tests() {
    int tests_failed = 0, tests_total = 0;
    for (auto& t : ::test_registry()) {
        ++tests_total;
        int f0 = ::failures;
        try { t.fn(); }
        catch (const std::exception& e) { std::cout << "[FAIL] TEST " << t.name << " -> " << e.what() << "\n"; ++tests_failed; continue; }
        if (::failures == f0) std::cout << "[OK]   TEST " << t.name << "\n";
        else { std::cout << "[FAIL] TEST " << t.name << " (inner cases failed)\n"; ++tests_failed; }
    }
    std::cout << "\n[SUMMARY] cases: passed " << ::passed << "/" << ::total << " | failed " << ::failures << "\n";
    std::cout << "[SUMMARY] tests: passed " << (tests_total - tests_failed) << "/" << tests_total << " | failed " << tests_failed << "\n";
    return (::failures == 0 && tests_failed == 0) ? 0 : 1;
}

#endif // RUN_TEST_H