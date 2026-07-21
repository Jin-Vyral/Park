// Park - Parallel Algorithm Resource Kit
//
// Protected under Karma License 1.0
//
// base.hpp - base for containers

#pragma once

#include <atomic>
#include <bit>
#include <type_traits>
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

struct BaseState_LE
{
	uint32_t _end{ 0 };
	uint32_t _max{ 0 };
};

struct BaseState_BE
{
	uint32_t _max{ 0 };
	uint32_t _end{ 0 };
};

using BaseState = std::conditional_t<std::endian::native == std::endian::little, BaseState_LE, BaseState_BE>;

template<typename T_Type>
struct base
{
	std::vector<T_Type>& get() { return _vec; }
	const std::vector<T_Type>& get() const { return _vec; }
	uint32_t size() const { return _size; }
	bool empty() const { return _vec.empty(); }

	virtual void clear(const bool release = false)
	{
		BaseState* pState = (BaseState*)&_state;
		pState->_end = _size = 0;

		if(release)
		{
			pState->_max = 0;
			_vec.resize(0);
			_vec.shrink_to_fit();
		}
	}

protected:
	uint32_t get_index()
	{
	try_add:
		const uint64_t val = _state.fetch_add(1);
		const BaseState* pState = (BaseState*)&val;
		const uint32_t index = pState->_end;

		if(index >= pState->_max)
		{
			if(index != pState->_max)
			{
				_mm_pause();
				goto try_add;
			}

			while(_size.load() != index)
				_mm_pause();

			const uint32_t size = std::max((uint32_t)_vec.size(), uint32_t(1)) * 2;
			realloc(size);

			BaseState newState{ ._end = index + 1, ._max = size };
			_state.store(*(uint64_t*)&newState);
		}

		return index;
	}

	void lock_index()
	{
		_size.fetch_add(1);
	}

	virtual void realloc(const uint32_t size)
	{
		_vec.resize(size);
	}

	std::vector<T_Type> _vec;
	std::atomic<uint64_t> _state;
	std::atomic<uint32_t> _size{ 0 };
};

}
