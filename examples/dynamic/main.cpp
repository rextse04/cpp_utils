#define UTILS_DYN_NO_ARR

#include "dynamic.hpp"
#include <cmath>
#include <iostream>
#include <string_view>

using namespace utils;

struct shape {
    dyn_method<std::string_view()> name;
    dyn_method<double(const_obj_ptr)> area;
};

struct circle : implements<shape> {
    double r;
    UTILS_DYN
    shape {
        .name = []() -> std::string_view { return "circle"; },
        .area = [](const_obj_ptr self) -> double {
            const auto self_ = static_cast<const circle*>(self);
            return M_PI * self_->r * self_->r;
        }
    }
    UTILS_DYN_END
};

struct rectangular {
    dyn_method<bool(const_obj_ptr)> is_square;
};

struct rectangle : implements<shape, rectangular> {
    double w, h;
    UTILS_DYN
    shape {
        .name = []() -> std::string_view { return "rectangle"; },
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
};

struct square : implements<shape, rectangular> {
    double l;
    UTILS_DYN
    shape {
        .name = []() -> std::string_view { return "square"; },
        .area = [](const_obj_ptr self) -> double {
            const auto self_ = static_cast<const square*>(self);
            return self_->l * self_->l;
        }
    },
    rectangular {
        .is_square = +[](const_obj_ptr) -> bool { return true; }
    }
    UTILS_DYN_END
};

struct dotted_square : square {
    int dot_count;
    UTILS_DYN
    []() {
        auto parent = square::vtable();
        parent.name = +[]() -> std::string_view { return "dotted_square"; };
        return parent;
    }()
    UTILS_DYN_END
};

int main() {
    for (int i = 0; i < 2; ++i) {
        unique_dptr<const shape> p;
        if (i) {
            p.reset(new circle{.r = 3});
        } else {
            p.reset(new rectangle{.w = 3, .h = 4});
        }
        std::cout << p[&shape::name]() << '\n';
    }
}