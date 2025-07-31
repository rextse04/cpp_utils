#define BOOST_TEST_MODULE math.cpp
#include "common.hpp"
#include "math.hpp"

using namespace utils;

BOOST_AUTO_TEST_CASE(pow_test) {
    BOOST_CHECK_EQUAL(utils::pow<3>(2), 8);
    BOOST_CHECK_EQUAL(utils::pow<0>(2), 1);
    BOOST_CHECK_EQUAL(utils::pow<-1>(2), 0);
    BOOST_CHECK_EQUAL(utils::pow<-1>(2, as<float>{}), 0.5);
    BOOST_CHECK_NE(utils::pow<-1>(3, as<float>{}), utils::pow<-1>(3, as<double>{}));
}