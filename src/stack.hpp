// Park - Parallel Algorithm Resource Kit
//
// Protected under Karma License 1.0
//
// stack.hpp - a stack

#pragma once

#include "dump.hpp"

namespace park
{

	template<typename T_Type, uint64_t Max_Factor = (0xFFFFFFFFui64 >> 1), uint32_t Panic_Factor = 100>
	struct stack : public dump<T_Type>
	{
		bool pop_back(T_Type& objOut)
		{
			const uint64_t val = this->_state.fetch_sub(1);
			const BaseState* pState = (BaseState*)&val;
			const uint64_t index = pState->_end - 1;
			if(index >= Max_Factor)
			{
				if(index == Max_Factor * Panic_Factor)
				{
					BaseState newState{ ._end = 0, ._max = pState->_max };
					this->_state.store(*(uint64_t*)&newState);
				}

				return false;
			}

			objOut = this->_vec[index];

			return true;
		}

		void reset()
		{
			const BaseState* pState = (BaseState*)&this->_state;
			BaseState newState{ ._end = 0, ._max = pState->_max };
			this->_state = *(uint64_t*)&newState;
			this->_size = 0;
		}
	};

}
