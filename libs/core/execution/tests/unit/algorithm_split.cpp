//  Copyright (c) 2021 ETH Zurich
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/execution.hpp>
#include <hpx/modules/testing.hpp>

#include "algorithm_test_utils.hpp"

#include <atomic>
#include <exception>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace ex = hpx::execution::experimental;

// This overload is only used to check dispatching. It is not a useful
// implementation.
template <typename Allocator = hpx::util::internal_allocator<>>
auto tag_invoke(
    ex::split_t, custom_sender_tag_invoke s, Allocator const& = Allocator{})
{
    s.tag_invoke_overload_called = true;
    return void_sender{};
}

int main()
{
    // Success path
    {
        std::atomic<bool> set_value_called{false};
        auto s1 = void_sender{};
        auto s2 = ex::split(std::move(s1));
        auto f = [] {};
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s2), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        auto s1 = ex::just(0);
        auto s2 = ex::split(std::move(s1));
        auto f = [](int x) { HPX_TEST_EQ(x, 0); };
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s2), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        auto s1 = ex::just(custom_type_non_default_constructible{42});
        auto s2 = ex::split(std::move(s1));
        auto f = [](auto x) { HPX_TEST_EQ(x.x, 42); };
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s2), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        auto s1 =
            ex::just(custom_type_non_default_constructible_non_copyable{42});
        auto s2 = ex::split(std::move(s1));
        auto f = [](auto& x) { HPX_TEST_EQ(x.x, 42); };
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s2), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    // operator| overload
    {
        std::atomic<bool> set_value_called{false};
        auto s = void_sender{} | ex::split();
        auto f = [] {};
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
    }

    // tag_invoke overload
    {
        std::atomic<bool> receiver_set_value_called{false};
        std::atomic<bool> tag_invoke_overload_called{false};
        auto s =
            custom_sender_tag_invoke{tag_invoke_overload_called} | ex::split();
        auto f = [] {};
        auto r = callback_receiver<decltype(f)>{f, receiver_set_value_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(receiver_set_value_called);
        HPX_TEST(tag_invoke_overload_called);
    }

    // Failure path
    {
        std::atomic<bool> set_error_called{false};
        auto s = error_sender{} | ex::split();
        auto r = error_callback_receiver<decltype(check_exception_ptr)>{
            check_exception_ptr, set_error_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_error_called);
    }

    {
        std::atomic<bool> set_error_called{false};
        auto s = error_sender{} | ex::split() | ex::split() | ex::split();
        auto r = error_callback_receiver<decltype(check_exception_ptr)>{
            check_exception_ptr, set_error_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_error_called);
    }

    // Chained split calls do not create new shared states
    {
        std::atomic<bool> receiver_set_value_called{false};
        auto s1 = ex::just() | ex::split();
        auto s2 = ex::split(s1);
        HPX_TEST_EQ(s1.state, s2.state);
        auto s3 = ex::split(std::move(s2));
        HPX_TEST_EQ(s1.state, s3.state);
        auto f = [] {};
        auto r = callback_receiver<decltype(f)>{f, receiver_set_value_called};
        auto os = ex::connect(std::move(s3), std::move(r));
        ex::start(os);
        HPX_TEST(receiver_set_value_called);
    }

    {
        std::atomic<bool> receiver_set_value_called{false};
        auto s1 = ex::just(42) | ex::split();
        auto s2 = ex::split(s1);
        HPX_TEST_EQ(s1.state, s2.state);
        auto s3 = ex::split(std::move(s2));
        HPX_TEST_EQ(s1.state, s3.state);
        auto f = [](int x) { HPX_TEST_EQ(x, 42); };
        auto r = callback_receiver<decltype(f)>{f, receiver_set_value_called};
        auto os = ex::connect(std::move(s3), std::move(r));
        ex::start(os);
        HPX_TEST(receiver_set_value_called);
    }

    return hpx::util::report_errors();
}
