// Park - Parallel Algorithm Resource Kit
//
// Protected under Karma License 1.0
//
// dump.hpp - simplest possible container

#pragma once

#include "base.hpp"

namespace park
{
	
template<typename T_Type>
struct dump : public base<T_Type>
{
	void push_back(T_Type obj)
	{
		const uint32_t index = this->get_index();
		this->_vec[index] = obj;
		this->lock_index();
	}

	void trim(const bool release = false)
	{
		this->_vec.resize(this->size());
		if(release)
			this->_vec.shrink_to_fit();
	}
};

}
