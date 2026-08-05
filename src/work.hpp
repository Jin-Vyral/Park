// Park - Parallel Algorithm Resource Kit
//
// Protected under Karma License 1.0
//
// work.hpp - simple jobs

#pragma once

#include <atomic>
#include <bit>
#include <type_traits>
#include <thread>
#include <vector>

#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__GNUC__) || defined(__clang__)
#include <immintrin.h>
#else
#error "Compiler not supported"
#endif

namespace park
{

	struct work
	{
		work() = delete;
		work(const uint32_t count)
		{
			_threads.reserve(count);
		}

		void prepare()
		{
			_threads.clear();
		}

		template<typename T_Func>
		void run(T_Func&& func)
		{
			const size_t threads = _threads.capacity();
			for(size_t i = 0; i < threads; ++i)
				_threads.emplace_back(func);
		}

		void wait()
		{
			for(auto& t : _threads)
			{
				if(t.joinable())
					t.join();
			}
		}

	protected:
		std::vector<std::thread> _threads;
	};

}
