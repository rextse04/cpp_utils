#define BOOST_TEST_MODULE containers/interval.hpp
#include "common.hpp"
#include "containers/interval.hpp"

using namespace utils;

BOOST_AUTO_TEST_CASE(endpoint_test) {
    using enum interval_endpoint_type;
    const interval_endpoint a = 0 | closed, b = 0 | open, c = 1 | open, d = 1 | closed;
    BOOST_CHECK(a == a); BOOST_CHECK(b <= b); BOOST_CHECK(c >= c); BOOST_CHECK(d == d);
    BOOST_CHECK(a.compare(b, false) > 0); BOOST_CHECK(a < c); BOOST_CHECK(a < d);
    BOOST_CHECK(c > b); BOOST_CHECK(d >= b);
    BOOST_CHECK(c <= d);
}

BOOST_AUTO_TEST_CASE(interval_test) {
    using enum interval_endpoint_type;
    const interval_endpoint a = 0 | closed, b = 0 | open, c = 2 | open, d = 2 | closed;
    const interval ac = {a, c}, ad = {a, d}, bc = {b, c}, bd = {b, d};
    BOOST_CHECK(!(a,a).empty()); BOOST_CHECK((b,b).empty()); BOOST_CHECK((c,c).empty()); BOOST_CHECK(!(d,d).empty());
    BOOST_CHECK((a,b).empty()); BOOST_CHECK((c,d).empty());
    BOOST_CHECK(!ac.empty()); BOOST_CHECK(!ad.empty()); BOOST_CHECK(!bc.empty()); BOOST_CHECK(!bd.empty());
    BOOST_CHECK(ac.contains(0)); BOOST_CHECK(ac.contains(1)); BOOST_CHECK(!ac.contains(2));
    BOOST_CHECK(ad.contains(0)); BOOST_CHECK(ad.contains(1)); BOOST_CHECK(ad.contains(2));
    BOOST_CHECK(!bc.contains(0)); BOOST_CHECK(bc.contains(1)); BOOST_CHECK(!bc.contains(2));
    BOOST_CHECK(!bd.contains(0)); BOOST_CHECK(bd.contains(1)); BOOST_CHECK(bd.contains(2));
    BOOST_CHECK(ac.superset_of(bc));
    BOOST_CHECK(ad.superset_of(ac)); BOOST_CHECK(ad.superset_of(bc)); BOOST_CHECK(ad.superset_of(bd));
    BOOST_CHECK(ac.strict_superset_of(bc));
    BOOST_CHECK(ad.strict_superset_of(ac)); BOOST_CHECK(ad.strict_superset_of(bc)); BOOST_CHECK(ad.strict_superset_of(bd));
    BOOST_CHECK(ac.superset_of(ac)); BOOST_CHECK(!ac.strict_superset_of(ac));
    BOOST_CHECK(ac >= bc);
    BOOST_CHECK(ad >= bc); BOOST_CHECK(ad >= bc); BOOST_CHECK(ad >= bc);
    BOOST_CHECK(ac > bc);
    BOOST_CHECK(ac < ad); BOOST_CHECK(bc < ad); BOOST_CHECK(bd < ad);
    BOOST_CHECK(ac == ac); BOOST_CHECK(!(ac != ac));
    BOOST_CHECK((a,a).intersection_with((b,b)).empty());
    BOOST_CHECK(ac.intersection_with(ac) == ac);
    BOOST_CHECK(ac.intersection_with(ad) == ac);
    BOOST_CHECK(ac.intersection_with(bc) == bc);
    BOOST_CHECK(ac.intersection_with(bd) == bc);
    BOOST_CHECK(ad.intersection_with(ad) == ad);
    BOOST_CHECK(ad.intersection_with(bc) == bc);
    BOOST_CHECK(ad.intersection_with(bd) == bd);
    BOOST_CHECK(bc.intersection_with(bc) == bc);
    BOOST_CHECK(bc.intersection_with(bd) == bc);
    BOOST_CHECK(bd.intersection_with(bd) == bd);
}