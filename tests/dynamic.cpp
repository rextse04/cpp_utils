#define BOOST_TEST_MODULE dynamic.hpp
#include "common.hpp"
#include "dynamic.hpp"
#include <string>
#include <cmath>
#include <thread>
#include <vector>
#include <atomic>

using namespace utils;

struct shape {
    dyn_method<std::string()> name;
    dyn_method<double(const_obj_ptr)> area;
};
std::string dtor_msg;

struct circle : implements<shape> {
    double r;
    UTILS_DYN
    shape {
        .name = []() -> std::string { return "circle"; },
        .area = [](const_obj_ptr self) -> double {
            const auto self_ = static_cast<const circle*>(self);
            return M_PI * self_->r * self_->r;
        }
    }
    UTILS_DYN_END
    ~circle() { dtor_msg += vtable().name(); }
};

struct rectangular {
    dyn_method<bool(const_obj_ptr)> is_square;
};

struct rectangle : implements<shape, rectangular> {
    double w, h;
    UTILS_DYN
    shape {
        .name = []() -> std::string { return "rectangle"; },
        .area = [](const_obj_ptr self) -> double {
            const auto self_ = static_cast<const rectangle*>(self);
            return self_->w * self_->h;
        }
    },
    rectangular {
        .is_square = +[](const_obj_ptr self) -> bool {
            const auto self_ = static_cast<const rectangle*>(self);
            return self_->w == self_->h;
        }
    }
    UTILS_DYN_END
    ~rectangle() { dtor_msg += vtable().name(); }
};

struct square : implements<shape, rectangular> {
    double l;
    UTILS_DYN
    shape {
        .name = []() -> std::string { return "square"; },
        .area = [](const_obj_ptr self) -> double {
            const auto self_ = static_cast<const square*>(self);
            return self_->l * self_->l;
        }
    },
    rectangular {
        .is_square = +[](const_obj_ptr) -> bool { return true; }
    }
    UTILS_DYN_END
    ~square() { dtor_msg += vtable().name(); }
};

struct dotted_square : square {
    int dot_count;
    UTILS_DYN
    []() {
        auto parent = square::vtable();
        parent.name = +[]() -> std::string { return "dotted_square"; };
        return parent;
    }()
    UTILS_DYN_END
};

BOOST_AUTO_TEST_CASE(traits_test) {
    BOOST_CHECK((implemented<circle, shape>));
    BOOST_CHECK((implemented<rectangle, shape>));
    BOOST_CHECK((implemented<square, shape>));
    BOOST_CHECK((implemented<dotted_square, shape>));
}

BOOST_AUTO_TEST_CASE(fptr_test) {
    square s{.l = 1};
    dotted_square ds{s, 10};

    fptr<square> sfp = &s;
    BOOST_CHECK_EQUAL(sfp->l, 1);
    BOOST_CHECK_EQUAL(sfp[&shape::name](), "square");
    BOOST_CHECK_EQUAL(sfp[&shape::area](), 1);
    BOOST_CHECK(sfp[&rectangular::is_square]());
    sfp = &ds;
    BOOST_CHECK_EQUAL(sfp->l, 1);
    BOOST_CHECK_EQUAL(sfp[&shape::name](), "dotted_square");
    BOOST_CHECK_EQUAL(sfp[&shape::area](), 1);
    BOOST_CHECK(sfp[&rectangular::is_square]());
}

BOOST_AUTO_TEST_CASE(dptr_test) {
    dtor_msg.clear();

    dptr<const shape> p;
    dptr<const shape> p2(new circle{.r = 1});
    dptr<const shape&> p3(new rectangle{.w = 1, .h = 2});
    p = p2;
    BOOST_CHECK_EQUAL(p2.get(), p.get());
    BOOST_CHECK_EQUAL(p[&shape::name](), "circle");
    BOOST_CHECK_EQUAL(p[&shape::area](), M_PI);
    p.destroy_and_delete();
    BOOST_CHECK_EQUAL(dtor_msg, "circle");

    p = std::move(p3);
    BOOST_CHECK_EQUAL(p3.get(), nullptr);
    BOOST_CHECK_EQUAL(dtor_msg, "circle");
    BOOST_CHECK_EQUAL(p[&shape::name](), "rectangle");
    BOOST_CHECK_EQUAL(p[&shape::area](), 2);

    dtor_msg += " ";
    p.destroy_and_delete();
    BOOST_CHECK_EQUAL(dtor_msg, "circle rectangle");
}

template <typename T, typename U>
constexpr bool is_settable_v = std::is_constructible_v<T, U> && std::is_assignable_v<T, U>;
template <typename T, typename U>
constexpr bool is_not_settable_v = !std::is_constructible_v<T, U> && !std::is_assignable_v<T, U>;
BOOST_AUTO_TEST_CASE(dptr_constraints_test) {
    BOOST_CHECK((is_not_settable_v<dptr<shape>, const circle*>));
    BOOST_CHECK((is_settable_v<dptr<const shape>, const circle*>));
    BOOST_CHECK((is_settable_v<dptr<const shape>, circle*>));
    BOOST_CHECK((is_settable_v<dptr<shape>, dptr<shape>>));
    BOOST_CHECK((is_not_settable_v<dptr<shape, rectangular>, dptr<shape>>));
    BOOST_CHECK((is_not_settable_v<dptr<shape>, dptr<rectangular>>));
    BOOST_CHECK((is_not_settable_v<dptr<shape>, dptr<const rectangular>>));
    BOOST_CHECK((is_settable_v<dptr<const shape>, dptr<shape>>));
    BOOST_CHECK((is_not_settable_v<dptr<shape>, dptr<const shape>>));
    BOOST_CHECK((is_not_settable_v<dptr<shape&>, const dptr<shape&>&>));
}

BOOST_AUTO_TEST_CASE(unique_dptr_test) {
    dtor_msg.clear();
    {
        unique_dptr<const shape, rectangular> p(new dotted_square{{.l = 5}, 10});
        BOOST_CHECK_EQUAL(p[&shape::name](), "dotted_square");
        BOOST_CHECK_EQUAL(p[&shape::area](), 25);
        BOOST_CHECK(p[&rectangular::is_square]());

        unique_dptr<const shape> p2 = std::move(p);
        BOOST_CHECK_EQUAL(p2[&shape::name](), "dotted_square");
        BOOST_CHECK_EQUAL(p2[&shape::area](), 25);
    }
    BOOST_CHECK_EQUAL(dtor_msg, "square");
}

BOOST_AUTO_TEST_CASE(unique_dptr_array_test) {
    dtor_msg.clear();
    {
        unique_dptr<const shape[]> p(new square[2]{{.l = 5}, {.l = 10}});
        BOOST_CHECK_EQUAL(p[&shape::name](), "square");
        BOOST_CHECK_EQUAL(p[&shape::area](), 25);
        const auto p2 = p+1;
        BOOST_CHECK_EQUAL(p2[&shape::name](), "square");
        BOOST_CHECK_EQUAL(p2[&shape::area](), 100);
    }
    BOOST_CHECK_EQUAL(dtor_msg, "squaresquare");
}

BOOST_AUTO_TEST_CASE(shared_dptr_test) {
    dtor_msg.clear();
    shared_dptr<const shape, rectangular> p(new dotted_square{{.l = 5}, 10});
    weak_dptr<const shape> w(p);
    BOOST_CHECK_EQUAL(w.use_count(), 1);
    {
        auto w2 = w;
        shared_dptr<const shape> p2(w2);
        BOOST_CHECK_EQUAL(p2.use_count(), 2);
        BOOST_CHECK_EQUAL(p2[&shape::name](), "dotted_square");
        BOOST_CHECK_EQUAL(p2[&shape::area](), 25);
    }
    circle c{.r = 3};
    {
        shared_dptr<const shape> p3(alias_with, p, std::addressof(c));
        BOOST_CHECK_EQUAL(p3.use_count(), 2);
        BOOST_CHECK_EQUAL(p3[&shape::name](), "circle");
        BOOST_CHECK_EQUAL(p3[&shape::area](), 9 * M_PI);
    }
    BOOST_CHECK_EQUAL(dtor_msg, "");
    weak_dptr<const rectangular> w2(p);
    {
        shared_dptr<const rectangular> p4(std::move(p));
        BOOST_CHECK_EQUAL(p.get(), nullptr);
        BOOST_CHECK_EQUAL(p.get_control(), nullptr);
        BOOST_CHECK_EQUAL(p4.use_count(), 1);
        p4 = w2;
        BOOST_CHECK_EQUAL(p4.use_count(), 1);
        BOOST_CHECK(p4[&rectangular::is_square]());
    }
    BOOST_CHECK_EQUAL(dtor_msg, "square");
    BOOST_CHECK(w.expired());
}

BOOST_AUTO_TEST_CASE(shared_dptr_multithreading_test) {
    dtor_msg.clear();
    shared_dptr<shape, rectangular> p(new rectangle{.w = 1, .h = 2});
    weak_dptr<shape, rectangular> w(p);
    constexpr int total_count = 100;
    std::atomic<int> good_count = 0, bad_count = 0;
    {
        std::vector<std::jthread> threads;
        for (int i = 0; i < total_count; ++i) {
            threads.emplace_back([i, &p, &w, &good_count, &bad_count]() {
                if (i == 0) {
                    shared_dptr<const rectangular> p2 = std::move(p);
                    ++good_count;
                } else {
                    try {
                        shared_dptr<const rectangular> p2(w);
                        ++good_count;
                    } catch (const std::bad_weak_ptr&) {
                        ++bad_count;
                    }
                }
            });
        }
    }
    BOOST_CHECK_EQUAL(dtor_msg, "rectangle");
    BOOST_CHECK_MESSAGE(good_count + bad_count == total_count,
        "good_count: " << good_count << ", bad_count: " << bad_count << ", total_count: " << total_count);
}