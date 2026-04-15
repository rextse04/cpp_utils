#define BOOST_TEST_MODULE math.cpp
#include "common.hpp"
#include "math.hpp"

using namespace utils;

BOOST_AUTO_TEST_CASE(pow_test) {
    BOOST_CHECK_EQUAL(utils::pow<3>(2), 8);
    BOOST_CHECK_EQUAL(utils::pow<0>(2), 1);
    BOOST_CHECK_EQUAL(utils::pow<-1>(2), 0);
    BOOST_CHECK_EQUAL((utils::pow<-1, float>(2)), 0.5);
    BOOST_CHECK_NE((utils::pow<-1, float>(3)), (utils::pow<-1, double>(3)));
}