// Park - Parallel Algorithm Resource Kit
//
// Protected under Karma License 1.0
//
// dump.hpp - simple thread-safe container

#pragma once

#include <atomic>
#include <bit>
#include <type_traits>
#include <vector>

#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__GNUC__) || defined(__clang__)
#include <immintrin.h> // Or <xmmintrin.h>
#else
#error "Compiler not supported"
#endif

namespace park
{

template<typename T_Type>
struct dump
{
	void push_back(T_Type t)
	{
	try_add:
		const uint64_t val = _state.fetch_add(1);
		const State* pState = (State*)&val;
		const uint32_t end = pState->_end;

		if(end >= pState->_max)
		{
			if(end != pState->_max)
			{
				_mm_pause();
				goto try_add;
			}

			while(_adds.load() != end)
				_mm_pause();

			const uint32_t size = std::max((uint32_t)_vec.size(), uint32_t(1)) * 2;
			_vec.resize(size);

			State newState{ ._end = end + 1, ._max = size };
			_state.store(*(uint64_t*)&newState);
		}

		_vec[end] = t;
		_adds.fetch_add(1);
	}

	void trim() { _vec.resize(size()); }
	std::vector<T_Type>& get() { return _vec; }
	const std::vector<T_Type>& get() const { return _vec; }
	uint32_t size() const { return _adds; }

	void clear(const bool release = false)
	{
		State* pState = (State*)&_state;
		pState->_end = _adds = 0;

		if(release)
		{
			pState->_max = 0;
			_vec.resize(0);
		}
	}

private:
	std::vector<T_Type> _vec;

	struct State_LE
	{
		uint32_t _end{ 0 };
		uint32_t _max{ 0 };
	};

	struct State_BE
	{
		uint32_t _max{ 0 };
		uint32_t _end{ 0 };
	};

	using State = std::conditional_t<std::endian::native == std::endian::little, State_LE, State_BE>;
	std::atomic<uint64_t> _state;
	std::atomic<uint32_t> _adds{ 0 };
};

}
