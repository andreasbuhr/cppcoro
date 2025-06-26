#include <iostream>
#include <cppcoro/generator.hpp>

cppcoro::generator<int> sequence()
{
	co_yield 1;
	co_yield 22;
	co_yield 333;
}

int main()
{
	for (auto i : sequence())
	{
		std::cout << i << std::endl;
	}
}
