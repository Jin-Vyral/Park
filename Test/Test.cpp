// Park - Parallel Algorithm Resource Kit
//
// Protected under Karma License 1.0
// 
// Test.cpp : Testing Park functionality
//

#include "Test.h"
#include "../src/dump.hpp"

#include <thread>

using namespace park;

static constexpr uint32_t NUM_ADDS = 1024 * 100;
static const size_t NUM_THREADS = 16;

dump<uint32_t> d;
uint32_t adds[NUM_ADDS]{ 0 };

void TestDump()
{
	while(true)
	{
		std::vector<std::thread> threads;
		threads.reserve(NUM_THREADS);

		// Build the dump
		for(uint32_t i = 1; i <= NUM_THREADS; ++i)
		{
			threads.emplace_back([i]()
			{
				std::this_thread::yield();

				for(uint32_t j = 0; j < NUM_ADDS; ++j)
					d.push_back(j);
			});
		}

		// Wait til finished
		for(auto& t : threads)
		{
			if(t.joinable())
				t.join();
		}

		// Validate vector contents
		d.trim();
		const std::vector<uint32_t>& vec = d.get();

		constexpr uint32_t TOTAL_ELEMENTS = NUM_ADDS * NUM_THREADS;

		if(vec.size() != TOTAL_ELEMENTS)
		{
			std::cout << "FAILURE!!!! Size mismatch\n\n";
			return;
		}

		for(uint32_t i = 0; i < TOTAL_ELEMENTS; ++i)
			++adds[vec[i]];

		for(uint32_t i = 0; i < NUM_ADDS; ++i)
		{
			if(adds[i] != NUM_THREADS)
			{
				std::cout << "FAILURE!!!! Missing element\n\n";
				return;
			}
		}

		// Clear for another run
		constexpr bool release = true;
		d.clear(release);
		memset(adds, 0, NUM_ADDS * sizeof(adds[0]));

		std::cout << ".";
	}
}

int main()
{
	TestDump();
	return 0;
}