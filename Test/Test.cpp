// Park - Parallel Algorithm Resource Kit
//
// Protected under Karma License 1.0
// 
// Test.cpp : Testing Park functionality
//

#include "Test.h"

#include "../src/batch.hpp"
#include "../src/dump.hpp"
#include "../src/pool.hpp"
#include "../src/vector.hpp"
#include "../src/work.hpp"

#include <cstring>
#include <random>

static constexpr uint32_t NUM_ADDS = 1024 * 1024;
static constexpr size_t NUM_THREADS = 16;
static constexpr uint32_t TOTAL_ELEMENTS = NUM_ADDS * NUM_THREADS;

park::dump<uint32_t> _d;
uint32_t _adds[NUM_ADDS]{ 0 };

void TestDump()
{
	std::cout << "Test dump...\n\n";

	park::batch bat(NUM_THREADS);

	while(true)
	{
		bat.run(TOTAL_ELEMENTS, [](const uint32_t start, const uint32_t end)
		{
			for(uint32_t j = 0; j < NUM_ADDS; ++j)
				_d.push_back(j);
		});

		bat.wait();

		// Validate vector contents
		_d.trim();
		const std::vector<uint32_t>& vec = _d.get();

		if(vec.size() != TOTAL_ELEMENTS)
		{
			std::cout << "FAILURE!!!! Size mismatch\n\n";
			return;
		}

		for(uint32_t i = 0; i < TOTAL_ELEMENTS; ++i)
			++_adds[vec[i]];

		for(uint32_t i = 0; i < NUM_ADDS; ++i)
		{
			if(_adds[i] != NUM_THREADS)
			{
				std::cout << "FAILURE!!!! Missing element\n\n";
				return;
			}
		}

		// Clear for another run
		constexpr bool release = true;
		_d.clear(release);
		std::memset(_adds, 0, NUM_ADDS * sizeof(_adds[0]));

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
	std::cout << "Test vector...\n\n";

	for(uint32_t i = 0; i < TOTAL_ELEMENTS; ++i)
		_items.emplace_back(i);

	park::batch bat(NUM_THREADS);

	while(true)
	{
		std::atomic<uint32_t> numIn{ 0 };

		bat.run(TOTAL_ELEMENTS, [&numIn](const uint32_t start, const uint32_t end)
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

				pItem->_in = isIn;
			}
		});

		bat.wait();

		const uint32_t removes = _v.prepare();
		if(removes != 0)
		{
			bat.run(removes, [](const uint32_t start, const uint32_t end)
			{
				for(uint32_t j = start; j < end; ++j)
					_v.compress(j);
			});

			bat.wait();
		}

		_v.finalize();

		if(_v.size() != numIn)
		{
			std::cout << "FAILURE!!!! Size mismatch\n\n";
			return;
		}

		bat.run(_v.size(), [](const uint32_t start, const uint32_t end)
		{
			for(uint32_t j = start; j < end; ++j)
			{
				const Item* pItem = _v.get()[j];
				if(pItem->_in == false)
				{
					std::cout << "FAILURE!!!! Invalid element\n\n";
					return;
				}

				if(pItem->_index != j)
				{
					std::cout << "FAILURE!!!! Index mismatch\n\n";
					return;
				}
			}
		});

		bat.wait();
		
		std::cout << "." << std::flush;
	}
}

void TestStack()
{
	std::cout << "Test stack...\n\n";

	park::stack<uint32_t> stk;
	park::work wrk(NUM_THREADS);

	while(true)
	{
		std::vector<std::atomic<uint32_t>> sadds(NUM_ADDS);

		wrk.prepare();
		wrk.run([&stk]()
		{
			for(uint32_t j = 0; j < NUM_ADDS; ++j)
				stk.push_back(j);
		});

		wrk.wait();

		// Validate vector contents
		if(stk.size() != TOTAL_ELEMENTS)
		{
			std::cout << "FAILURE!!!! Size mismatch\n\n";
			return;
		}

		wrk.prepare();

		wrk.run([&stk, &sadds]()
		{
			uint32_t val = 0;
			bool res = stk.pop_back(val);

			while(res)
			{
				++sadds[val];
				res = stk.pop_back(val);
			}
		});

		wrk.wait();

		park::batch bat(NUM_THREADS);

		bat.run(NUM_ADDS, [&sadds](const uint32_t start, const uint32_t end)
		{
			for(uint32_t j = start; j < end; ++j)
			{
				if(sadds[j] != NUM_THREADS)
				{
					std::cout << "FAILURE!!!! Missing element\n\n";
					return;
				}
			}
		});
		
		bat.wait();

		// Clear for another run
		stk.reset();

		constexpr bool release = true;
		stk.clear(release);

		std::cout << "." << std::flush;
	}
}

struct PItem
{
	PItem() = delete;
	PItem(const uint32_t id)
		: _id{ id }
	{
	}

	uint32_t _id;
	uint32_t* _pIndex{ nullptr };
	bool _in{ false };
};

std::vector<PItem> _pitems;
park::pool<uint32_t> _p;

void TestPool()
{
	std::cout << "Test pool...\n\n";

	for(uint32_t i = 0; i < TOTAL_ELEMENTS; ++i)
		_pitems.emplace_back(i);

	park::batch bat(NUM_THREADS);

	while(true)
	{
		std::atomic<uint32_t> numIn{ 0 };

		bat.run(TOTAL_ELEMENTS, [&numIn](const uint32_t start, const uint32_t end)
		{
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<> distr(0, 1);

			for(uint32_t j = start; j < end; ++j)
			{
				PItem* pItem = &_pitems[j];
				const bool wasIn = pItem->_in;
				const bool isIn = (bool)distr(gen);

				if(isIn)
					++numIn;

				if(!wasIn && isIn)
					pItem->_pIndex = _p.acquire(pItem->_id);
				else if(wasIn && !isIn)
				{
					_p.release(pItem->_pIndex);
					pItem->_pIndex = nullptr;
				}

				pItem->_in = isIn;
			}
		});

		bat.wait();

		_p.reconcile();

		bat.run(TOTAL_ELEMENTS, [&numIn](const uint32_t start, const uint32_t end)
		{
			for(uint32_t j = start; j < end; ++j)
			{
				const PItem* pItem = &_pitems[j];
				if(pItem->_in != (pItem->_pIndex != nullptr))
				{
					std::cout << "FAILURE!!!! Invalid element\n\n";
					return;
				}

				if(pItem->_in && (*pItem->_pIndex != pItem->_id))
				{
					std::cout << "FAILURE!!!! Index mismatch\n\n";
					return;
				}
			}
		});

		bat.wait();

		std::cout << "." << std::flush;
	}
}

int main()
{
	//TestDump();
	//TestVector();
	//TestStack();
	TestPool();

	return 0;
}