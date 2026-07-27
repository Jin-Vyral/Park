// Park - Parallel Algorithm Resource Kit
//
// Protected under Karma License 1.0
//
// pool.hpp - object pool

#pragma once

#include "vector.hpp"

namespace park
{

struct pool
{
	using T_Type = uint32_t;

	static constexpr uint32_t BLOCK_SIZE = 1024;

	pool()
	{
		_pLast = _pFirst = new block();
	}

	virtual ~pool()
	{
		block* pCur = _pFirst;
		while(pCur)
		{
			block* pNext = pCur->_pNext;
			delete pCur;
			pCur = pNext;
		}
	}

	template<typename... Args>
	T_Type* acquire(Args&&... args)
	{
	try_add:
		uint32_t cur = _cur.fetch_add(1);
		block* pCur;

		if(cur >= BLOCK_SIZE)
		{
			if(cur > BLOCK_SIZE)
			{
				_mm_pause();
				goto try_add;
			}

			while(_set != cur)
				_mm_pause();

			block* pNew = new block(_pLast);
			pNew->_pPrev->_pNext = pNew;
			_pLast.store(pNew);
			_set.store(0);
			_cur.store(1);
			pCur = pNew;
			cur = 0;
		}
		else
			pCur = _pLast.load();

		object* pObj = &pCur->_objects[cur];
		T_Type* pPtr = new (pObj->_bytes) T_Type(std::forward<Args>(args)...);
		pObj->_pBlock = pCur;
		pCur->_acquired.fetch_add(1);
		_set.fetch_add(1);

		return pObj->obj();
	}

	void release(T_Type* pObj)
	{
		object* pObject = reinterpret_cast<object*>(pObj);
		pObject->obj()->~T_Type();
		_released.push_back(pObject);
	}

protected:

	struct block;

	struct object
	{
		object() = default;

		T_Type* obj() { return reinterpret_cast<T_Type*>(_bytes); }

		uint8_t _bytes[sizeof(T_Type)]{ 0 };
		block* _pBlock{ nullptr };
	};

	static_assert(std::is_standard_layout_v<object>, "object must be standard layout");

	struct block
	{
		block() = default;
		block(block* pPrev)
			: _pPrev(pPrev)
		{
		}

		block* _pPrev{ nullptr };
		block* _pNext{ nullptr };
		object _objects[BLOCK_SIZE];
		std::atomic<uint32_t> _acquired{ 0 };
	};

	block* _pFirst{ nullptr };
	std::atomic<block*> _pLast{ nullptr };
	std::atomic<uint32_t> _cur{ 0 };
	std::atomic<uint32_t> _set{ 0 };

	dump<object*> _released;
	std::vector<object*> _free;
};

}