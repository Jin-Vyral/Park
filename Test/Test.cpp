// Park - Parallel Algorithm Resource Kit
//
// Protected under Karma License 1.0
// 
// Test.cpp : Testing Park functionality
//

#include "Test.h"

#include "../src/dump.hpp"
#include "../src/vector.hpp"

#include <cstring>
#include <random>
#include <thread>

static constexpr uint32_t NUM_ADDS = 1024 * 1024;
static constexpr size_t NUM_THREADS = 16;
static constexpr uint32_t TOTAL_ELEMENTS = NUM_ADDS * NUM_THREADS;

park::dump<uint32_t> d;

uint32_t adds[NUM_ADDS]{ 0 };

void TestDump()
{
	std::cout << "Test dump...\n\n";

	while(true)
	{
		std::vector<std::thread> threads;
		threads.reserve(NUM_THREADS);

		// Build the dump
		for(uint32_t i = 1; i <= NUM_THREADS; ++i)
		{
			threads.emplace_back([i]()
			{
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
		std::memset(adds, 0, NUM_ADDS * sizeof(adds[0]));

		std::cout << "." << std::flush;
	}
}

struct Item
{
	Item() = delete;
	Item(const uint32_t id)
		: _id{ id }
	{
	}

	uint32_t _id;
	uint32_t _index{ 0 };
	bool _in{ false };
};

std::vector<Item> _items;
park::vector<Item*> _v;

void TestVector()
{
	for(uint32_t i = 0; i < TOTAL_ELEMENTS; ++i)
		_items.emplace_back(i);

	while(true)
	{
		std::atomic<uint32_t> numIn = 0;

		std::vector<std::thread> threads;
		threads.reserve(NUM_THREADS);

		const uint32_t each = NUM_ADDS;
		uint32_t start = 0;
		uint32_t end = start + each;

		// Update vector
		for(uint32_t i = 1; i <= NUM_THREADS; ++i)
		{
			threads.emplace_back([&numIn](const uint32_t start, const uint32_t end)
			{
				std::random_device rd;
				std::mt19937 gen(rd());
				std::uniform_int_distribution<> distr(0, 1);

				for(uint32_t j = start; j < end; ++j)
				{
					Item* pItem = &_items[j];
					const bool wasIn = pItem->_in;
					const bool isIn = (bool)distr(gen);

					if(isIn)
						++numIn;

					if(!wasIn && isIn)
						_v.push_back(pItem, &pItem->_index);
					else if(wasIn && !isIn)
						_v.remove(pItem->_index);

					_items[j]._in = isIn;
				}
			}, start, end);

			start = end;
			end += each;
		}

		// Wait til finished
		for(auto& t : threads)
		{
			if(t.joinable())
				t.join();
		}

		const uint32_t removes = _v.prepare();
		if(removes != 0)
		{
			std::vector<std::thread> threads;
			threads.reserve(NUM_THREADS);

			const uint32_t each = removes / NUM_THREADS;
			uint32_t start = 0;
			uint32_t end = start + each;

			// Compress
			for(uint32_t i = 1; i <= NUM_THREADS; ++i)
			{
				threads.emplace_back([i](const uint32_t start, const uint32_t end)
				{
					for(uint32_t j = start; j < end; ++j)
						_v.compress(j);
				}, start, end);

				start = end;
				if(i == (NUM_THREADS - 1))
					end = removes;
				else
					end += each;
			}

			// Wait til finished
			for(auto& t : threads)
			{
				if(t.joinable())
					t.join();
			}
		}

		if(_v.size() != numIn)
		{
			std::cout << "FAILURE!!!! Size mismatch\n\n";
			return;
		}

		for(uint32_t i = 0; i < _v.size(); ++i)
		{
			const Item* pItem = _v.get()[i];
			if(pItem->_in == false)
			{
				std::cout << "FAILURE!!!! Missing element\n\n";
				return;
			}

			if(pItem->_index != i)
			{
				std::cout << "FAILURE!!!! Index mismatch\n\n";
				return;
			}
		}

		std::cout << "." << std::flush;
		_v.finalize();
	}
}

int main()
{
	//TestDump();
	TestVector();

	return 0;
}