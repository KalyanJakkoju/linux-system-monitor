#pragma once

#include <cmath>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace test {

struct TestCase {
    std::string suite_name;
    std::string test_name;
    std::function<void()> func;
};

class TestRegistry {
public:
    static TestRegistry& instance() {
        static TestRegistry reg;
        return reg;
    }

    void add_test(const std::string& suite, const std::string& name, std::function<void()> func) {
        tests_.push_back({suite, name, std::move(func)});
    }

    int run_all() {
        int passed = 0;
        int failed = 0;

        std::cout << "\n=======================================================\n";
        std::cout << "                 RUNNING UNIT TEST SUITE               \n";
        std::cout << "=======================================================\n";

        for (const auto& t : tests_) {
            std::cout << "[ RUN      ] " << t.suite_name << "." << t.test_name << "\n";
            try {
                t.func();
                std::cout << "\033[32m[       OK ]\033[0m " << t.suite_name << "." << t.test_name << "\n";
                passed++;
            } catch (const std::exception& e) {
                std::cout << "\033[31m[  FAILED  ]\033[0m " << t.suite_name << "." << t.test_name
                          << " -> " << e.what() << "\n";
                failed++;
            } catch (...) {
                std::cout << "\033[31m[  FAILED  ]\033[0m " << t.suite_name << "." << t.test_name
                          << " -> Unknown exception\n";
                failed++;
            }
        }

        std::cout << "=======================================================\n";
        std::cout << "Total Tests: " << tests_.size() << " | Passed: " << passed
                  << " | Failed: " << failed << "\n";
        std::cout << "=======================================================\n\n";

        return failed == 0 ? 0 : 1;
    }

private:
    std::vector<TestCase> tests_;
};

struct TestRegistrar {
    TestRegistrar(const std::string& suite, const std::string& name, std::function<void()> func) {
        TestRegistry::instance().add_test(suite, name, std::move(func));
    }
};

#define TEST(suite, name) \
    void suite##_##name(); \
    static test::TestRegistrar registrar_##suite##_##name(#suite, #name, suite##_##name); \
    void suite##_##name()

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            throw std::runtime_error(std::string("Assertion failed: ") + #condition + \
                                     " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while (0)

#define ASSERT_FALSE(condition) \
    do { \
        if ((condition)) { \
            throw std::runtime_error(std::string("Assertion failed: NOT ") + #condition + \
                                     " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while (0)

template <typename T>
inline std::string to_string_repr(const T& val) {
    std::ostringstream oss;
    oss << val;
    return oss.str();
}

#define ASSERT_EQ(val1, val2) \
    do { \
        if (!((val1) == (val2))) { \
            throw std::runtime_error(std::string("Assertion failed: ") + #val1 + " == " + #val2 + \
                                     " (" + test::to_string_repr(val1) + " vs " + test::to_string_repr(val2) + ")" + \
                                     " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while (0)

#define ASSERT_STR_EQ(str1, str2) \
    do { \
        if (std::string(str1) != std::string(str2)) { \
            throw std::runtime_error(std::string("Assertion failed: \"") + std::string(str1) + \
                                     "\" == \"" + std::string(str2) + "\"" + \
                                     " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while (0)

#define ASSERT_NEAR(val1, val2, epsilon) \
    do { \
        if (std::abs((val1) - (val2)) > (epsilon)) { \
            throw std::runtime_error(std::string("Assertion failed: ") + #val1 + " ~= " + #val2 + \
                                     " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while (0)

#define ASSERT_THROW(statement, exception_type) \
    do { \
        bool caught = false; \
        try { \
            statement; \
        } catch (const exception_type&) { \
            caught = true; \
        } catch (...) {} \
        if (!caught) { \
            throw std::runtime_error(std::string("Expected exception: ") + #exception_type + \
                                     " for " + #statement + " at " + __FILE__ + ":" + std::to_string(__LINE__)); \
        } \
    } while (0)

} // namespace test
