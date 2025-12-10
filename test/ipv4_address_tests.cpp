///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////

#include <cppcoro/net/ipv4_address.hpp>

#include "doctest/cppcoro_doctest.h"


TEST_SUITE_BEGIN("ipv4_address");

using cppcoro::net::ipv4_address;

TEST_CASE("DefaultConstructToZeroes")
{
	CHECK(ipv4_address{}.to_integer() == 0u);
}

TEST_CASE("to_integer() is BigEndian")
{
	ipv4_address address{ 10, 11, 12, 13 };
	CHECK(address.to_integer() == 0x0A0B0C0Du);
}

TEST_CASE("is_loopback()")
{
	CHECK(ipv4_address{ 127, 0, 0, 1 }.is_loopback());
	CHECK(ipv4_address{ 127, 0, 0, 50 }.is_loopback());
	CHECK(ipv4_address{ 127, 5, 10, 15 }.is_loopback());
	CHECK(!ipv4_address{ 10, 11, 12, 13 }.is_loopback());
}

TEST_CASE("bytes()")
{
	ipv4_address ip{ 19, 63, 129, 200 };
	CHECK(ip.bytes()[0] == 19);
	CHECK(ip.bytes()[1] == 63);
	CHECK(ip.bytes()[2] == 129);
	CHECK(ip.bytes()[3] == 200);
}

TEST_CASE("to_string()")
{
	CHECK(ipv4_address(0, 0, 0, 0).to_string() == "0.0.0.0");
	CHECK(ipv4_address(10, 125, 255, 7).to_string() == "10.125.255.7");
	CHECK(ipv4_address(123, 234, 101, 255).to_string() == "123.234.101.255");
}

TEST_CASE("from_string")
{
	// Check for some invalid strings.
	CHECK(ipv4_address::from_string("") == std::nullopt);
	CHECK(ipv4_address::from_string("asdf") == std::nullopt);
	CHECK(ipv4_address::from_string(" 123.34.56.8") == std::nullopt);
	CHECK(ipv4_address::from_string("123.34.56.8 ") == std::nullopt);
	CHECK(ipv4_address::from_string("123.") == std::nullopt);
	CHECK(ipv4_address::from_string("123.1") == std::nullopt);
	CHECK(ipv4_address::from_string("123.12") == std::nullopt);
	CHECK(ipv4_address::from_string("123.12.") == std::nullopt);
	CHECK(ipv4_address::from_string("123.12.4") == std::nullopt);
	CHECK(ipv4_address::from_string("123.12.45") == std::nullopt);
	CHECK(ipv4_address::from_string("123.12.45.") == std::nullopt);

	// Overflow of individual parts
	CHECK(ipv4_address::from_string("456.12.45.30") == std::nullopt);
	CHECK(ipv4_address::from_string("45.256.45.30") == std::nullopt);
	CHECK(ipv4_address::from_string("45.25.677.30") == std::nullopt);
	CHECK(ipv4_address::from_string("123.12.45.301") == std::nullopt);

	// Can't parse octal yet.
	CHECK(ipv4_address::from_string("00") == std::nullopt);
	CHECK(ipv4_address::from_string("012345") == std::nullopt);
	CHECK(ipv4_address::from_string("045.25.67.30") == std::nullopt);
	CHECK(ipv4_address::from_string("45.025.67.30") == std::nullopt);
	CHECK(ipv4_address::from_string("45.25.067.30") == std::nullopt);
	CHECK(ipv4_address::from_string("45.25.67.030") == std::nullopt);

	// Parse single integer format
	CHECK(ipv4_address::from_string("0") == ipv4_address(0));
	CHECK(ipv4_address::from_string("1") == ipv4_address(0, 0, 0, 1));
	CHECK(ipv4_address::from_string("255") == ipv4_address(0, 0, 0, 255));
	CHECK(ipv4_address::from_string("43534243") == ipv4_address(43534243));

	// Parse dotted decimal format
	CHECK(ipv4_address::from_string("45.25.67.30") == ipv4_address(45, 25, 67, 30));
	CHECK(ipv4_address::from_string("0.0.0.0") == ipv4_address(0, 0, 0, 0));
	CHECK(ipv4_address::from_string("1.2.3.4") == ipv4_address(1, 2, 3, 4));
}

TEST_CASE("round-trip and ordering")
{
    // Round-trip canonicalisation via to_string()/from_string()
    auto check_round_trip = [](const char* s) {
        auto p = ipv4_address::from_string(s);
        REQUIRE(p.has_value());
        auto canon = p->to_string();
        auto p2 = ipv4_address::from_string(canon);
        REQUIRE(p2.has_value());
        CHECK(*p == *p2);
        CHECK(p2->to_string() == canon); // stable
    };

    check_round_trip("0.0.0.0");
    check_round_trip("255.255.255.255");
    check_round_trip("192.168.0.1");

    // Single-integer form canonicals to dotted-decimal
    auto p = ipv4_address::from_string("1");
    REQUIRE(p.has_value());
    CHECK(p->to_string() == std::string("0.0.0.1"));

    // Ordering within IPv4 space
    CHECK(ipv4_address{1,2,3,4} < ipv4_address{1,2,3,5});
    CHECK(ipv4_address{10,0,0,1} < ipv4_address{10,0,0,2});
}
TEST_SUITE_END();
