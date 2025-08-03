#define BOOST_TEST_MODULE concurrency.hpp
#include "common.hpp"
#include "concurrency.hpp"
#include <string>
#include <thread>
#include <vector>
#include <format>
#include <ranges>
#include <atomic>

using namespace utils;
using namespace std::chrono_literals;

BOOST_AUTO_TEST_CASE(sanity_test) {
    unique_resource<std::string> res(1, "sanity_test");
    BOOST_CHECK_EQUAL(*res, "sanity_test");
    {
        auto p1 = res.acquire();
        BOOST_CHECK_EQUAL(*p1, "sanity_test");
        BOOST_CHECK(!res.try_acquire());
        p1.write()->append(" modified!");
    }
    {
        auto p2 = res.acquire();
        BOOST_CHECK_EQUAL(*p2, "sanity_test modified!");
    }
}

BOOST_AUTO_TEST_CASE(sync_test) {
    unique_resource<std::string, true> res(9);
    std::atomic failed_i = -1;
    {
        std::vector<std::jthread> threads;
        for (int i = 0; i < 10; ++i) {
            threads.emplace_back([&res, i, &failed_i]() {
                auto p = res.try_acquire_for(100ms);
                if (p) {
                    char s[] = "0 ";
                    s[0] += i;
                    for (int j = 0; j < 100; ++j) {
                        const integer_alias::size_t og_size = p->read()->size();
                        p->write()->append(s);
                        BOOST_CHECK_GT(p->read()->size(), og_size);
                    }
                    failed_i.wait(-1);
                } else {
                    BOOST_TEST_MESSAGE(std::format("Thread #{} failed to acquire the resource!", i));
                    failed_i = i;
                    failed_i.notify_all();
                }
            });
        }
    }
    auto p = res.try_acquire();
    BOOST_REQUIRE(p.has_value());
    for (char c = '0'; c <= '9'; ++c) {
        if (c == '0' + failed_i) BOOST_CHECK(!std::ranges::contains(*p->read(), c));
        else BOOST_CHECK(std::ranges::count(*p->read(), c) == 100);
    }
    BOOST_TEST_MESSAGE(*p->read());
}