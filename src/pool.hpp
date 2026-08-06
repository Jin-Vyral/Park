// Park - Parallel Algorithm Resource Kit
//
// Protected under Karma License 1.0
//
// pool.hpp - object pool

#pragma once

#include "stack.hpp"
#include "vector.hpp"

#include <cstring>

namespace park
{

template<typename T_Type>
struct pool
{
	static constexpr uint32_t BLOCK_SIZE = 1024;
	static constexpr uint32_t RELEASES_SIZE = 1024;
	static constexpr uint32_t PANIC_BUFFER = 1024 * 1024;

	pool()
	{
		_pLastBlock = _pFirstBlock = new block();
		_pLastRel = _pFirstRel = new releases();
		_blocks = 1;
	}

	virtual ~pool()
	{
		block* pCur = _pFirstBlock;
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
		object* pObj;

	try_reuse:
		uint32_t curFree = _freeCur.fetch_add(1);
		releases* pCurFree;

		if(curFree >= BLOCK_SIZE)
		{
			if(curFree > BLOCK_SIZE)
			{
				if(curFree == (BLOCK_SIZE + PANIC_BUFFER))
					_freeCur.store(BLOCK_SIZE + 1);

				if((_frees.load() == 0) || !should_reuse())
					goto try_add;

				goto try_reuse;
			}

			while(_freeSet != curFree)
				_mm_pause();

			_frees.fetch_sub(1);

			releases* pLast = _pLastFree.load();
			pCurFree = pLast->_pPrev;
			if(!pCurFree)
			{
				_pFirstFree = nullptr;
				_pLastFree.store(nullptr);

				goto try_add;
			}

			pCurFree->_pNext = nullptr;
			delete pLast;
			_pLastFree.store(pCurFree);
			_freeSet.store(0);
			_freeCur.store(1);
			curFree = 0;
		}
		else
			pCurFree = _pLastFree.load();

		pObj = pCurFree->_ptrs[curFree];
		_freeSet.fetch_add(1);

		return new (pObj->_bytes) T_Type(std::forward<Args>(args)...);

	try_add:
		uint32_t cur = _blockCur.fetch_add(1);
		block* pCur;

		if(cur >= BLOCK_SIZE)
		{
			if(cur > BLOCK_SIZE)
			{
				_mm_pause();
				goto try_add;
			}

			while(_blockSet != cur)
				_mm_pause();

			++_blocks;

			block* pLast = _pLastBlock.load();
			pCur = pLast->_pNext = new block(pLast);
			_pLastBlock.store(pCur);
			_blockSet.store(0);
			_blockCur.store(1);
			cur = 0;
		}
		else
			pCur = _pLastBlock.load();

		pObj = &pCur->_objects[cur];
		pObj->_pBlock = pCur;
		pCur->_acquired.fetch_add(1);
		_acquired.fetch_add(1);
		_blockSet.fetch_add(1);

		return new (pObj->_bytes) T_Type(std::forward<Args>(args)...);
	}

	void release(T_Type* pObj)
	{
		object* pObject = reinterpret_cast<object*>(pObj);

		if constexpr(std::is_trivially_destructible_v<T_Type> == false)
			pObject->obj()->~T_Type();

		std::memset(pObject->_bytes, 0xdeadbeef, object::SIZE * sizeof(uint32_t));

	try_release:
		uint32_t cur = _relCur.fetch_add(1);
		releases* pCur;

		if(cur >= RELEASES_SIZE)
		{
			if(cur > RELEASES_SIZE)
			{
				_mm_pause();
				goto try_release;
			}

			while(_relSet != cur)
				_mm_pause();

			++_rels;

			releases* pLast = _pLastRel.load();
			pCur = pLast->_pNext = new releases(pLast);
			_pLastRel.store(pCur);
			_relSet.store(0);
			_relCur.store(1);
			cur = 0;
		}
		else
			pCur = _pLastRel.load();

		pCur->_ptrs[cur] = pObject;
		pCur->_acquired.fetch_add(1);
		_acquired.fetch_sub(1);
		_relSet.fetch_add(1);
	}

	void reconcile()
	{
		if(!_pFirstRel)
			return;

		releases* pFirstRel = _pFirstRel;
		releases* pLastRel = _pLastRel;

		if(pLastRel->_acquired != RELEASES_SIZE)
		{
			if(pFirstRel == pLastRel)
				return;

			releases* pTmp = pLastRel->_pPrev;
			pLastRel->_pPrev = nullptr;
			pTmp->_pNext = nullptr;
			_pFirstRel = pLastRel;
			_pLastRel = pLastRel;
			pLastRel = pTmp;
		}
		else
		{
			_pFirstRel = new releases();
			_pLastRel = _pFirstRel;
			_relSet = _relCur = 0;
			++_rels;
		}

		releases* pFirstFree = _pFirstFree;
		if(pFirstFree)
		{
			pFirstFree->_pPrev = pLastRel;
			pLastRel->_pNext = pFirstFree;
			_pFirstFree = pFirstRel;
		}
		else
		{
			_pFirstFree = pFirstRel;
			_freeSet = _freeCur = 0;
			_pLastFree = pLastRel;
		}

		_frees.fetch_add(_rels);
		_rels = 0;
	}

protected:
	virtual bool should_reuse() { return true; }

	struct block;

	struct object
	{
		static constexpr uint32_t SIZE = sizeof(T_Type) / sizeof(uint32_t);

		object() = default;

		T_Type* obj() { return reinterpret_cast<T_Type*>(_bytes); }

		uint32_t _bytes[SIZE]{ 0xdeadbeef };
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

	block* _pFirstBlock{ nullptr };
	std::atomic<block*> _pLastBlock{ nullptr };
	std::atomic<uint32_t> _blockCur{ 0 };
	std::atomic<uint32_t> _blockSet{ 0 };

	struct releases
	{
		releases() = default;
		releases(releases* pPrev)
			: _pPrev(pPrev)
		{
		}

		releases* _pPrev{ nullptr };
		releases* _pNext{ nullptr };
		object* _ptrs[RELEASES_SIZE]{ nullptr };
		std::atomic<uint32_t> _acquired{ 0 };
	};

	releases* _pFirstRel{ nullptr };
	std::atomic<releases*> _pLastRel{ nullptr };
	std::atomic<uint32_t> _relCur{ 0 };
	std::atomic<uint32_t> _relSet{ 0 };

	releases* _pFirstFree{ nullptr };
	std::atomic<releases*> _pLastFree{ nullptr };
	std::atomic<uint32_t> _freeCur{ BLOCK_SIZE + 1 };
	std::atomic<uint32_t> _freeSet{ 0 };
	std::atomic<uint32_t> _frees{ 0 };

	std::atomic<uint32_t> _acquired{ 0 };
	uint32_t _blocks{ 0 };
	uint32_t _rels{ 0 };
};

}