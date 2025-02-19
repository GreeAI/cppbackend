#define BOOST_TEST_MODULE urlencode tests
#include <boost/test/unit_test.hpp>

#include "../src/urldecode.h"

BOOST_AUTO_TEST_CASE(UrlDecode_tests) {
    using namespace std::literals;

    BOOST_TEST(UrlDecode(""sv) == ""s);
    BOOST_TEST(UrlDecode("Hello+World%20%21"sv) == "Hello World !"s);
    BOOST_TEST(UrlDecode("Hello+C%2b%2b"sv) == "Hello C++"s);
    BOOST_TEST(UrlDecode("Test%25+Complete"sv) == "Test% Complete"s);
    BOOST_TEST(UrlDecode("I+sit%20in%20a+%23+dunk"sv) == "I sit in a # dunk"s);
}