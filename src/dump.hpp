// Park - Parallel Algorithm Resource Kit
//
// Protected under Karma License 1.0
//
// dump.hpp - simplest possible container

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

template<typename T_Type>
struct dump
{
	void push_back(T_Type obj)
	{
		const uint32_t index = get_index();
		_vec[index] = obj;
		lock_index();
	}

	void trim(const bool release = false)
	{
		_vec.resize(size());

		if(release)
			_vec.shrink_to_fit();
	}

	std::vector<T_Type>& get() { return _vec; }
	const std::vector<T_Type>& get() const { return _vec; }
	uint32_t size() const { return _adds; }
	bool empty() const { return _vec.empty(); }

	void clear(const bool release = false)
	{
		State* pState = (State*)&_state;
		pState->_end = _adds = 0;

		if(release)
		{
			pState->_max = 0;
			_vec.resize(0);
			_vec.shrink_to_fit();
		}
	}

protected:
	virtual uint32_t get_index()
	{
	try_add:
		const uint64_t val = _state.fetch_add(1);
		const State* pState = (State*)&val;
		const uint32_t index = pState->_end;

		if(index >= pState->_max)
		{
			if(index != pState->_max)
			{
				_mm_pause();
				goto try_add;
			}

			while(_adds.load() != index)
				_mm_pause();

			const uint32_t size = std::max((uint32_t)_vec.size(), uint32_t(1)) * 2;
			realloc(size);

			State newState{ ._end = index + 1, ._max = size };
			_state.store(*(uint64_t*)&newState);
		}

		return index;
	}

	virtual void lock_index()
	{
		_adds.fetch_add(1);
	}

	virtual void realloc(const uint32_t size)
	{
		_vec.resize(size);
	}

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
