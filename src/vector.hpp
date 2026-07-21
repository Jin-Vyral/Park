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
		this->lock_index();

		if(pIndex)
			*pIndex = index;
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