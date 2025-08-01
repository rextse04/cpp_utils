#define BOOST_TEST_MODULE integer.cpp
#include "common.hpp"
#include "integer.hpp"
#include <sstream>
#include <format>
#include <type_traits>

using namespace utils::integer_alias;
namespace ib = utils::integral_behavior;
namespace sb = utils::shift_behavior;
using test_types = std::tuple<
    utils::integer<int, ib::standard>,
    utils::integer<int, ib::sane>,
    utils::integer<int, ib::ub>,
    utils::integer<int, ib::wrap>,
    utils::integer<int, ib::sat>,
    utils::integer<int, ib::checked>>;

BOOST_AUTO_TEST_CASE(sanity_test) {
    using s = utils::integer<short>;
    using u = utils::integer<unsigned short>;
    using ull = utils::integer<unsigned long long>;
    BOOST_CHECK_EQUAL(s(123.1), 123);
    BOOST_CHECK_EQUAL(s(0xA'BCDE), static_cast<short>(0xBCDE));
    BOOST_CHECK_NE(s(0xABCD), 0xABCD);
    BOOST_CHECK_EQUAL(u(123.1), static_cast<unsigned short>(123));
    BOOST_CHECK_EQUAL(u(-1), std::numeric_limits<unsigned short>::max());
    BOOST_CHECK_EQUAL(ull(-1), std::numeric_limits<unsigned long long>::max());
    BOOST_CHECK_EQUAL(u(0xA'BCDE), static_cast<unsigned short>(0xBCDE));

    const s lhs(0xF0FF);
    const u rhs(0xFF00);
    BOOST_CHECK_EQUAL(lhs & rhs, s(0xF000));
    BOOST_CHECK_EQUAL(lhs | rhs, s(0xFFFF));
    BOOST_CHECK_EQUAL(lhs ^ rhs, s(0x0FFF));
    BOOST_CHECK_EQUAL(~lhs, 0x0F00);
    BOOST_CHECK_EQUAL(~rhs, static_cast<unsigned>(0x00FF));

    BOOST_CHECK_EQUAL(s(0xFFFF) << 4, s(0xFFF0));
    BOOST_CHECK_EQUAL(u(0xFFFF) << 4, u(0xFFF0));
    BOOST_CHECK_EQUAL(s(0xFFFF) >> 4, s(0xFFFF));
    BOOST_CHECK_EQUAL(u(0xFFFF) >> 4, u(0x0FFF));

    BOOST_CHECK_LT(s(0xABCD), 0xF'ABCD);
    BOOST_CHECK_GT(ullong(-1), u(-1));

    s sn(0);
    BOOST_CHECK_EQUAL(++sn, s(1));
    BOOST_CHECK_EQUAL(sn++, s(1));
    BOOST_CHECK_EQUAL(sn, s(2));
    u un(0);
    BOOST_CHECK_EQUAL(--un, u(-1));
    BOOST_CHECK_EQUAL(un--, u(-1));
    BOOST_CHECK_EQUAL(un, u(-2));
}

using normal_test_types = std::tuple<
    utils::integer<int, ib::standard>,
    utils::integer<int, ib::sane>,
    utils::integer<int, ib::ub>,
    utils::integer<int, ib::wrap>,
    utils::integer<int, ib::sat>,
    utils::integer<int, ib::checked>>;
BOOST_AUTO_TEST_CASE_TEMPLATE(normal_test, T, normal_test_types) {
    using s = T::template to<short>;
    using u = T::template to<unsigned short>;
    for (short n = std::numeric_limits<short>::min(); n < std::numeric_limits<short>::max(); ++n) {
        BOOST_CHECK_EQUAL(s(n) + s(1), n+1);
    }
    for (unsigned short n = 0; n < std::numeric_limits<unsigned short>::max(); ++n) {
        BOOST_CHECK_EQUAL(u(n) + u(1), static_cast<unsigned>(n+1));
    }
}

using unsigned_test_types = std::tuple<
    utils::integer<int, ib::sane>,
    utils::integer<int, ib::wrap>,
    utils::integer<int, ib::sat>>;
BOOST_AUTO_TEST_CASE_TEMPLATE(unsigned_test, T, unsigned_test_types) {
    using U = T::template to<unsigned short>;
    const auto umax = std::numeric_limits<U>::max();
    const unsigned short ans1 = std::is_same_v<T, utils::integer<int, ib::sat>>
        ? std::numeric_limits<unsigned short>::max()
        : std::numeric_limits<unsigned short>::max() - 1,
    ans2 = std::is_same_v<T, utils::integer<int, ib::sat>> ? 0 : 1;
    BOOST_CHECK_EQUAL(umax + umax, ans1);
    BOOST_CHECK_EQUAL(U(0) - umax, ans2);
    BOOST_CHECK_EQUAL(umax * static_cast<unsigned short>(2), ans1);
}

BOOST_AUTO_TEST_CASE(denorm_test) {
    const auto smax = std::numeric_limits<short>::max(),
    smin = std::numeric_limits<short>::min();
    const utils::integer<short, ib::wrap> smax_wrap(smax);
    BOOST_CHECK_EQUAL(smax_wrap + smax_wrap, -2);
    BOOST_CHECK_EQUAL(-smax_wrap - smax_wrap, 2);
    BOOST_CHECK_EQUAL(smax_wrap * static_cast<short>(2), -2);
    const utils::integer<short, ib::sat> smax_sat(smax), smin_sat(smin);
    BOOST_CHECK_EQUAL(smax_sat + smax_sat, smax_sat);
    BOOST_CHECK_EQUAL(-smax_sat - smax_sat, std::numeric_limits<short>::min());
    BOOST_CHECK_EQUAL(smax_sat * static_cast<short>(2), smax_sat);
    BOOST_CHECK_EQUAL(smin_sat / static_cast<short>(-1), smax_sat);
    BOOST_CHECK_EQUAL(smin_sat % static_cast<short>(-1), 0);
    const utils::integer<short, ib::checked> smax_checked(smax), smin_checked(smin);
    BOOST_CHECK_THROW(smax_checked + smax_checked, std::overflow_error);
    BOOST_CHECK_THROW(-smax_checked - smax_checked, std::overflow_error);
    BOOST_CHECK_THROW(smax_checked * static_cast<short>(2), std::overflow_error);
    BOOST_CHECK_THROW(smax_checked / static_cast<short>(0), std::domain_error);
    BOOST_CHECK_THROW(smin_checked / static_cast<short>(-1), std::overflow_error);
    BOOST_CHECK_THROW(smax_checked % static_cast<short>(0), std::domain_error);
    BOOST_CHECK_THROW(smin_checked % static_cast<short>(-1), std::overflow_error);
}

using shift_test_types = std::tuple<
    utils::integer<unsigned, ib::sane, sb::standard>,
    utils::integer<unsigned, ib::sane, sb::scalar>,
    utils::integer<unsigned, ib::sane, sb::circular>,
    utils::integer<unsigned, ib::sane, sb::checked>>;
BOOST_AUTO_TEST_CASE_TEMPLATE(shift_normal_test, T, shift_test_types) {
    T s(0xABCD);
    BOOST_CHECK_EQUAL(s << 16, 0xABCD'0000);
    BOOST_CHECK_EQUAL(s <<= 16, 0xABCD'0000);
    BOOST_CHECK_EQUAL(s >> 8, 0xAB'CD00u);
    BOOST_CHECK_EQUAL(s >>= 8, 0xAB'CD00u);
}

BOOST_AUTO_TEST_CASE(shift_denorm_test) {
    const utils::integer<int, ib::sane, sb::scalar> s_scalar(0xAABB'CCDD);
    BOOST_CHECK_EQUAL(s_scalar << 32, 0);
    BOOST_CHECK_EQUAL(s_scalar << -16, static_cast<int>(0xFFFF'AABB));
    BOOST_CHECK_EQUAL(s_scalar >> 32, static_cast<int>(0xFFFF'FFFF));
    BOOST_CHECK_EQUAL(s_scalar >> -16, static_cast<int>(0xCCDD'0000));
    const utils::integer<unsigned, ib::sane, sb::circular> s_circular(0xAABB'CCDD);
    const unsigned circular_ans = 0xCCDD'AABB;
    BOOST_CHECK_EQUAL(s_circular << 48, circular_ans);
    BOOST_CHECK_EQUAL(s_circular << -48, circular_ans);
    BOOST_CHECK_EQUAL(s_circular >> 48, circular_ans);
    BOOST_CHECK_EQUAL(s_circular >> -48, circular_ans);
    const utils::integer<int, ib::sane, sb::checked> s_checked(0xAABB'CCDD);
    BOOST_CHECK_THROW(s_checked << 32, std::domain_error);
    BOOST_CHECK_THROW(s_checked << -16, std::domain_error);
    BOOST_CHECK_THROW(s_checked >> 32, std::domain_error);
    BOOST_CHECK_THROW(s_checked >> -16, std::domain_error);
}

BOOST_AUTO_TEST_CASE(io_test) {
    const utils::integer<int> s(123);
    std::stringstream ss;
    ss << s;
    BOOST_CHECK_EQUAL(ss.str(), "123");
    utils::integer<unsigned char> u;
    ss >> u;
    BOOST_CHECK_EQUAL(u, static_cast<unsigned char>(123));
    BOOST_CHECK_EQUAL(std::format("{} {}", s, u), "123 123");
}

BOOST_AUTO_TEST_CASE(type_test) {
    BOOST_CHECK((std::is_convertible_v<int, sint>));
    BOOST_CHECK((std::is_convertible_v<short, sint>));
    BOOST_CHECK((!std::is_convertible_v<long long, sint>));
    BOOST_CHECK((!std::is_convertible_v<unsigned, sint>));
    BOOST_CHECK((!std::is_convertible_v<int, utils::integer_alias::uint>));
    BOOST_CHECK((std::is_convertible_v<sint, int>));
    BOOST_CHECK((std::is_convertible_v<sint, long long>));
    BOOST_CHECK((!std::is_convertible_v<sint, short>));
    BOOST_CHECK((!std::is_convertible_v<sint, unsigned>));
    BOOST_CHECK((std::is_convertible_v<sint, float>));
    BOOST_CHECK((!std::is_convertible_v<float, sint>));
}