#define BOOST_TEST_MODULE dynamic.hpp
#include "common.hpp"
#include "dynamic.hpp"
#include <string>
#include <cmath>

using namespace utils;

struct shape {
    dyn_method<std::string()> name;
    dyn_method<double(const void*)> area;
};
std::string dtor_msg;

struct circle : implements<shape> {
    double r;
    UTILS_DYN
    shape {
        .name = +[]() -> std::string { return "circle"; },
        .area = +[](const void* self) -> double {
            const auto self_ = static_cast<const circle*>(self);
            return M_PI * self_->r * self_->r;
        }
    }
    UTILS_DYN_END
    ~circle() { dtor_msg = vtable().name(); }
};

struct rectangular {
    dyn_method<bool(const void*)> is_square;
};

struct rectangle : implements<shape, rectangular> {
    double w, h;
    UTILS_DYN
    shape {
        .name = +[]() -> std::string { return "rectangle"; },
        .area = +[](const void* self) -> double {
            const auto self_ = static_cast<const rectangle*>(self);
            return self_->w * self_->h;
        }
    },
    rectangular {
        .is_square = +[](const void* self) -> bool {
            const auto self_ = static_cast<const rectangle*>(self);
            return self_->w == self_->h;
        }
    }
    UTILS_DYN_END
    ~rectangle() { dtor_msg = vtable().name(); }
};

struct square : implements<shape, rectangular> {
    double l;
    UTILS_DYN
    shape {
        .name = +[]() -> std::string { return "square"; },
        .area = +[](const void* self) -> double {
            const auto self_ = static_cast<const square*>(self);
            return self_->l * self_->l;
        }
    },
    rectangular {
        .is_square = +[](const void*) -> bool { return true; }
    }
    UTILS_DYN_END
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
    BOOST_CHECK((has_implemented_v<shape, circle>));
    BOOST_CHECK((has_implemented_v<shape, rectangle>));
    BOOST_CHECK((has_implemented_v<shape, square>));
    BOOST_CHECK((has_implemented_v<shape, dotted_square>));
}

BOOST_AUTO_TEST_CASE(fptr_test) {
    square s{.l = 1};
    dotted_square ds{s, 10};

    fptr<square> sfp = &s;
    BOOST_CHECK_EQUAL(sfp->name(), "square");
    BOOST_CHECK_EQUAL(sfp->area(&s), 1);
    sfp = &ds;
    BOOST_CHECK_EQUAL(sfp->name(), "dotted_square");
    BOOST_CHECK_EQUAL(sfp->area(&ds), 1);
}

BOOST_AUTO_TEST_CASE(dptr_test) {
    {
        unique_dptr<shape> c(new circle{.r = 1}),
        r(new rectangle{.w = 1, .h = 2});

        BOOST_CHECK_EQUAL(c->name(), "circle");
        BOOST_CHECK_EQUAL(c->area(c), M_PI);
        BOOST_CHECK_EQUAL(r->name(), "rectangle");
        BOOST_CHECK_EQUAL(r->area(r), 2);

        c.reset();
        BOOST_CHECK_EQUAL(dtor_msg, "circle");
    }
    BOOST_CHECK_EQUAL(dtor_msg, "rectangle");
}