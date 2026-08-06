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
	uint32_t push_back(T_Type obj)
	{
		const uint32_t index = this->get_index();
		this->_vec[index] = obj;
		this->lock_index();
		return index;
	}


	template<typename... Args>
	uint32_t emplace_back(Args&&... args)
	{
		const uint32_t index = this->get_index();
		new (&this->_vec[index]) T_Type(std::forward<Args>(args)...);
		this->lock_index();
		return index;
	}

	void trim(const bool release = false)
	{
		this->_vec.resize(this->size());
		if(release)
			this->_vec.shrink_to_fit();
	}
};

}
