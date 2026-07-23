// Park - Parallel Algorithm Resource Kit
//
// Protected under Karma License 1.0
//
// vector.hpp - compression vector

#pragma once

#include "dump.hpp"

namespace park
{

template<typename T_Type>
struct vector : public base<T_Type>
{
	void push_back(T_Type obj, uint32_t* pIndex = nullptr)
	{
		const uint32_t index = this->get_index();
		this->_vec[index] = obj;
		_info[index]._pIndex = pIndex;
		_info[index]._removed = false;

		if(pIndex)
			*pIndex = index;

		this->lock_index();
	}

	void trim(const bool release = false)
	{
		this->_vec.resize(this->size());
		_info.resize(this->size());

		if(release)
		{
			this->_vec.shrink_to_fit();
			_info.shrink_to_fit();
		}
	}

	void remove(const uint32_t index)
	{
		_removes.push_back(index);
		_info[index]._removed = true;
	}

	uint32_t prepare()
	{
		this->_size -= _removes.size();
		return _removes.size();
	}

	void compress(const uint32_t removal)
	{
		const uint32_t removeIndex = _removes.get()[removal];
		if(removeIndex > this->_size)
			return;

	try_remove:
		const uint64_t val = this->_state.fetch_sub(1);
		const BaseState* pState = (BaseState*)&val;
		const uint32_t moveIndex = pState->_end - 1;

		if(_info[moveIndex]._removed)
			goto try_remove;

		this->_vec[removeIndex] = this->_vec[moveIndex];
		_info[removeIndex] = _info[moveIndex];
	
		if(_info[removeIndex]._pIndex)
			*_info[removeIndex]._pIndex = removeIndex;
	}

	void finalize(const bool release = false)
	{
		_removes.clear(release);
	}

	void clear(const bool release = false) override
	{
		base<T_Type>::clear(release);
		_removes.clear(release);

		if(release)
		{
			_info.resize(0);
			_info.shrink_to_fit();
		}
	}

protected:
	void realloc(const uint32_t size) override
	{
		this->_vec.resize(size);
		_info.resize(size);
	}

	struct Info
	{
		uint32_t* _pIndex{ nullptr };
		bool _removed{ false };
	};

	std::vector<Info> _info;
	dump<uint32_t> _removes;
};

}