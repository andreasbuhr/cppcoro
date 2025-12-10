///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////

#include <cppcoro/config.hpp>
#include <cppcoro/net/ip_endpoint.hpp>

#include "doctest/cppcoro_doctest.h"

TEST_SUITE_BEGIN("ip_endpoint");

using namespace cppcoro::net;

namespace
{
	constexpr bool isMsvc15_5X86Optimised =
#if CPPCORO_COMPILER_MSVC && CPPCORO_CPU_X86 && _MSC_VER == 1912 && defined(CPPCORO_RELEASE_OPTIMISED)
		true;
#else
		false;
#endif
}

// BUG: Skip this test under MSVC 15.5 x86 optimised builds due to a compiler bug
// that generates bad code.
// See https://developercommunity.visualstudio.com/content/problem/177151/bad-code-generation-under-x86-optimised-for-stdopt.html
TEST_CASE("to_string" * doctest::skip{ isMsvc15_5X86Optimised })
{
	ip_endpoint a = ipv4_endpoint{ ipv4_address{ 192, 168, 2, 254 }, 80 };
	ip_endpoint b = ipv6_endpoint{
		*ipv6_address::from_string("2001:0db8:85a3:0000:0000:8a2e:0370:7334"),
		22 };

	CHECK(a.to_string() == "192.168.2.254:80");
	CHECK(b.to_string() == "[2001:db8:85a3::8a2e:370:7334]:22");
}

TEST_CASE("from_string" * doctest::skip{ isMsvc15_5X86Optimised })
{
	CHECK(ip_endpoint::from_string("") == std::nullopt);
	CHECK(ip_endpoint::from_string("[foo]:123") == std::nullopt);
	CHECK(ip_endpoint::from_string("[123]:1000") == std::nullopt);
	CHECK(ip_endpoint::from_string("[10.11.12.13]:1000") == std::nullopt);

	CHECK(ip_endpoint::from_string("192.168.2.254:80") ==
		ipv4_endpoint{
			ipv4_address{ 192, 168, 2, 254 }, 80 });
	CHECK(ip_endpoint::from_string("[2001:0db8:85a3:0000:0000:8a2e:0370:7334]:443") ==
		ipv6_endpoint{
			ipv6_address{ 0x2001, 0xdb8, 0x85a3, 0x0, 0x0, 0x8a2e, 0x370, 0x7334 },
			443 });
}

TEST_CASE("round-trip and ordering" * doctest::skip{ isMsvc15_5X86Optimised })
{
    // Round-trip
    auto check_round_trip = [](const char* s) {
        auto p = ip_endpoint::from_string(s);
        REQUIRE(p.has_value());
        auto canon = p->to_string();
        auto p2 = ip_endpoint::from_string(canon);
        REQUIRE(p2.has_value());
        CHECK(*p == *p2);
        CHECK(p2->to_string() == canon);
    };

    check_round_trip("192.168.2.254:80");
    check_round_trip("[2001:db8:85a3::8a2e:370:7334]:22");

    // Invalid ports/whitespace
    CHECK(ip_endpoint::from_string("192.168.1.1:65536") == std::nullopt);
    CHECK(ip_endpoint::from_string("[::1]:65536") == std::nullopt);
    CHECK(ip_endpoint::from_string(" 192.168.1.1:80") == std::nullopt);
    CHECK(ip_endpoint::from_string("[::1]:80 ") == std::nullopt);

    // Ordering: IPv4 < IPv6, and within family compare address/port
    ip_endpoint v4a = ipv4_endpoint{ ipv4_address{127,0,0,1}, 1 };
    ip_endpoint v4b = ipv4_endpoint{ ipv4_address{127,0,0,1}, 2 };
    ip_endpoint v6 = ipv6_endpoint{ ipv6_address{0,0,0,0,0,0,0,1}, 1 };
    CHECK(v4a < v6);
    CHECK(v4a < v4b);
}

TEST_SUITE_END();

