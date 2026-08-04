// Park - Parallel Algorithm Resource Kit
//
// Protected under Karma License 1.0
// 
// Test.cpp : Testing Park functionality
//

#include "Test.h"

#include "../src/dump.hpp"
#include "../src/pool.hpp"
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
	std::cout << "Test vector...\n\n";

	for(uint32_t i = 0; i < TOTAL_ELEMENTS; ++i)
		_items.emplace_back(i);

	while(true)
	{
		std::atomic<uint32_t> numIn = 0;

		{
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

						pItem->_in = isIn;
					}
				}, start, end);

				start = end;
				if(i == (NUM_THREADS - 1))
					end = TOTAL_ELEMENTS;
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
				threads.emplace_back([](const uint32_t start, const uint32_t end)
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

		_v.finalize();

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
				std::cout << "FAILURE!!!! Invalid element\n\n";
				return;
			}

			if(pItem->_index != i)
			{
				std::cout << "FAILURE!!!! Index mismatch\n\n";
				return;
			}
		}

		std::cout << "." << std::flush;
	}
}

//std::atomic<uint32_t> sadds[NUM_ADDS]{ 0 }; // Increases compile time significantly and is tested when pool is tested anyway
//
//void TestStack()
//{
//	std::cout << "Test stack...\n\n";
//
//	park::stack<uint32_t> stk;
//
//	while(true)
//	{
//		{
//			std::vector<std::thread> threads;
//			threads.reserve(NUM_THREADS);
//
//			// Build the dump
//			for(uint32_t i = 1; i <= NUM_THREADS; ++i)
//			{
//				threads.emplace_back([i, &stk]()
//				{
//					for(uint32_t j = 0; j < NUM_ADDS; ++j)
//						stk.push_back(j);
//				});
//			}
//
//			// Wait til finished
//			for(auto& t : threads)
//			{
//				if(t.joinable())
//					t.join();
//			}
//		}
//
//		// Validate vector contents
//		if(stk.size() != TOTAL_ELEMENTS)
//		{
//			std::cout << "FAILURE!!!! Size mismatch\n\n";
//			return;
//		}
//
//		{
//			std::vector<std::thread> threads;
//			threads.reserve(NUM_THREADS);
//
//			for(uint32_t i = 1; i <= NUM_THREADS; ++i)
//			{
//				threads.emplace_back([i, &stk]()
//				{
//					uint32_t val = 0;
//					bool res = stk.pop_back(val);
//
//					while(res)
//					{
//						++sadds[val];
//						res = stk.pop_back(val);
//					}
//
//					//for(uint32_t j = 0; j < NUM_ADDS; ++j)
//					//{
//					//	uint32_t val = 0;
//					//	const bool res = stk.pop_back(val);
//					//	if(!res)
//					//	{
//					//		std::cout << "FAILURE!!!! Size mismatch during pop\n\n";
//					//		return;
//					//	}
//
//					//	++adds[val];
//					//}
//				});
//			}
//
//			// Wait til finished
//			for(auto& t : threads)
//			{
//				if(t.joinable())
//					t.join();
//			}
//		}
//
//		for(uint32_t i = 0; i < NUM_ADDS; ++i)
//		{
//			if(sadds[i] != NUM_THREADS)
//			{
//				std::cout << "FAILURE!!!! Missing element\n\n";
//				//return;
//			}
//		}
//
//		// Clear for another run
//		stk.reset();
//
//		constexpr bool release = true;
//		stk.clear(release);
//		std::memset(sadds, 0, NUM_ADDS * sizeof(sadds[0]));
//
//		std::cout << "." << std::flush;
//	}
//}
//

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
park::pool _p;

void TestPool()
{
	std::cout << "Test pool...\n\n";

	for(uint32_t i = 0; i < TOTAL_ELEMENTS; ++i)
		_pitems.emplace_back(i);

	while(true)
	{
		std::atomic<uint32_t> numIn = 0;

		{
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
				}, start, end);

				start = end;
				if(i == (NUM_THREADS - 1))
					end = TOTAL_ELEMENTS;
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

		_p.reconcile();

		for(uint32_t i = 0; i < _pitems.size(); ++i)
		{
			const PItem* pItem = &_pitems[i];
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