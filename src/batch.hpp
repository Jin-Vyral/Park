// Park - Parallel Algorithm Resource Kit
//
// Protected under Karma License 1.0
//
// batch.hpp - simple batch jobs

#pragma once

#include <thread>
#include <vector>

namespace park
{

	struct batch
	{
		batch() = delete;
		batch(const uint32_t count)
		{
			_threads.reserve(count);
		}

		void prepare(const uint32_t count)
		{
			_count = count;
			_threads.clear();
		}

		template<typename T_Func>
		void run(T_Func&& func)
		{
			const size_t threads = _threads.capacity();
			const size_t block = _count / threads;
			const size_t extra = _count % threads;
			size_t start = 0;
			size_t end = block;

			for(size_t i = 0; i < threads; ++i)
			{
				end += (i < extra);
				_threads.emplace_back(func, (uint32_t)start, (uint32_t)end);
				start = end;
				end = start + block;
			}
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
		size_t _count{ 0 };
	};

}
