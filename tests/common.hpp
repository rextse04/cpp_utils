#pragma once
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>
#include <type_traits>

#define CONCAT2(x, y) x ## y
#define CONCAT(x, y) CONCAT2(x, y)
#define BOOST_ANON_TEST_CASE(...) BOOST_AUTO_TEST_CASE(CONCAT(test_, __LINE__) __VA_OPT__(,) __VA_ARGS__)

#define BOOST_CHECK_EQUAL_TYPES(...) BOOST_CHECK((std::is_same_v<__VA_ARGS__>))