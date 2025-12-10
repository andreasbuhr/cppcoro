///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////

#include <cppcoro/net/ip_address.hpp>

#include "doctest/cppcoro_doctest.h"
#include <string_view>

TEST_SUITE_BEGIN("ip_address");

using cppcoro::net::ip_address;
using cppcoro::net::ipv4_address;
using cppcoro::net::ipv6_address;

TEST_CASE("default constructor")
{
	ip_address x;
	CHECK(x.is_ipv4());
	CHECK(x.to_ipv4() == ipv4_address{});
}

TEST_CASE("to_string")
{
	ip_address a = ipv6_address{ 0xAABBCCDD00112233, 0x0102030405060708 };
	ip_address b = ipv4_address{ 192, 168, 0, 1 };

	CHECK(a.to_string() == "aabb:ccdd:11:2233:102:304:506:708");
	CHECK(b.to_string() == "192.168.0.1");
}

TEST_CASE("from_string")
{
    CHECK(ip_address::from_string("") == std::nullopt);
    CHECK(ip_address::from_string("foo") == std::nullopt);
    CHECK(ip_address::from_string(" 192.168.0.1") == std::nullopt);
    CHECK(ip_address::from_string("192.168.0.1asdf") == std::nullopt);

    CHECK(ip_address::from_string("192.168.0.1") == ipv4_address(192, 168, 0, 1));
    CHECK(ip_address::from_string("::192.168.0.1") == ipv6_address(0, 0, 0, 0, 0, 0, 0xc0a8, 0x1));
    CHECK(ip_address::from_string("aabb:ccdd:11:2233:102:304:506:708") ==
        ipv6_address{ 0xAABBCCDD00112233, 0x0102030405060708 });
}

TEST_CASE("round-trip and ordering")
{
    // Round-trip: to_string(from_string(s)) yields canonical string, and parsing back preserves value
    auto check_round_trip = [](std::string_view s) {
        auto p = ip_address::from_string(s);
        REQUIRE(p.has_value());
        auto canon = p->to_string();
        auto p2 = ip_address::from_string(canon);
        REQUIRE(p2.has_value());
        CHECK(*p == *p2);
        // Canonical string should be stable
        CHECK(p2->to_string() == canon);
    };

    // IPv4
    check_round_trip("0.0.0.0");
    check_round_trip("255.255.255.255");

    // IPv6 (mixed case input should parse and normalise to lower-case hex without leading zeros)
    check_round_trip("::");
    check_round_trip("::1");
    check_round_trip("FFFF:0000:0000:0000:0000:0000:0000:0001");
    check_round_trip("::192.168.0.1");

    // Ordering: IPv4 sorts less than IPv6
    ip_address v4 = ipv4_address{127, 0, 0, 1};
    ip_address v6 = ipv6_address{0, 0, 0, 0, 0, 0, 0, 1}; // ::1
    CHECK(v4 < v6);

    // Ordering within same family
    CHECK(ipv4_address{1, 1, 1, 1} < ipv4_address{1, 1, 1, 2});
    CHECK(ipv6_address{0, 0, 0, 0, 0, 0, 0, 1} < ipv6_address{0, 0, 0, 0, 0, 0, 0, 2});

    // Reject trailing/leading whitespace
    CHECK(ip_address::from_string("192.168.0.1 ") == std::nullopt);
    CHECK(ip_address::from_string("\t::1") == std::nullopt);

    // Reject malformed IPv6
    CHECK(ip_address::from_string("::::") == std::nullopt);
    CHECK(ip_address::from_string("gggg::1") == std::nullopt);
    CHECK(ip_address::from_string("12345::1") == std::nullopt);
}

TEST_SUITE_END();
