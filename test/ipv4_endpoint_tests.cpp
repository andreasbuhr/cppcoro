///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////

#include <cppcoro/net/ipv4_endpoint.hpp>

#include "doctest/cppcoro_doctest.h"

TEST_SUITE_BEGIN("ip_endpoint");

using namespace cppcoro::net;

TEST_CASE("to_string")
{
	CHECK(ipv4_endpoint{ ipv4_address{ 192, 168, 2, 254 }, 80 }.to_string() == "192.168.2.254:80");
}

TEST_CASE("from_string")
{
	CHECK(ipv4_endpoint::from_string("") == std::nullopt);
	CHECK(ipv4_endpoint::from_string("  ") == std::nullopt);
	CHECK(ipv4_endpoint::from_string("100") == std::nullopt);
	CHECK(ipv4_endpoint::from_string("100.10.200.20") == std::nullopt);
	CHECK(ipv4_endpoint::from_string("100.10.200.20:") == std::nullopt);
	CHECK(ipv4_endpoint::from_string("100.10.200.20::80") == std::nullopt);
	CHECK(ipv4_endpoint::from_string("100.10.200.20 80") == std::nullopt);

	CHECK(ipv4_endpoint::from_string("192.168.2.254:80") ==
		ipv4_endpoint{ ipv4_address{ 192, 168, 2, 254 }, 80 });
}

TEST_CASE("round-trip, boundary ports and invalid inputs")
{
    // Round-trip canonicalisation
    auto check_round_trip = [](const char* s) {
        auto p = ipv4_endpoint::from_string(s);
        REQUIRE(p.has_value());
        auto canon = p->to_string();
        auto p2 = ipv4_endpoint::from_string(canon);
        REQUIRE(p2.has_value());
        CHECK(*p == *p2);
        CHECK(p2->to_string() == canon);
    };

    check_round_trip("192.168.2.254:80");
    check_round_trip("0.0.0.0:0");
    check_round_trip("255.255.255.255:65535");

    // Boundary ports
    CHECK(ipv4_endpoint{ ipv4_address{0,0,0,0}, 0 }.to_string() == std::string("0.0.0.0:0"));
    CHECK(ipv4_endpoint{ ipv4_address{255,255,255,255}, 65535 }.to_string() == std::string("255.255.255.255:65535"));

    // Invalid: whitespace and out-of-range ports
    CHECK(ipv4_endpoint::from_string(" 192.168.0.1:80") == std::nullopt);
    CHECK(ipv4_endpoint::from_string("192.168.0.1:80 ") == std::nullopt);
    CHECK(ipv4_endpoint::from_string("192.168.0.1:65536") == std::nullopt);
    CHECK(ipv4_endpoint::from_string("192.168.0.1:-1") == std::nullopt);
}

TEST_SUITE_END();
