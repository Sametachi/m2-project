#pragma once

#include "entity.h"

enum ESectree
{
	SECTREE_SIZE	= 6400,
	SECTREE_HALF_SIZE	= 3200,
	CELL_SIZE		= 50
};

typedef struct sectree_coord
{
	unsigned            x : 16;
	unsigned            y : 16;
} SECTREE_COORD;

typedef union sectreeid
{
	uint32_t		package;
	SECTREE_COORD	coord;
} SECTREEID;

enum
{
	ATTR_BLOCK = (1 << 0),
	ATTR_WATER = (1 << 1),
	ATTR_BANPK = (1 << 2),
	ATTR_OBJECT = (1 << 7),
};

struct FCollectEntity {
	void operator()(LPENTITY entity) {
		result.push_back(entity);
	}
	template<typename F>
	void ForEach(F& f) {
		std::vector<LPENTITY>::iterator it = result.begin();
		for (; it != result.end(); ++it) {
			LPENTITY entity = *it;
			f(entity);
		}
	}
	typedef std::vector<LPENTITY> ListType;
	ListType result; // list collected
};

class CAttribute;

class SECTREE
{
	public:
		friend class SECTREE_MANAGER;
		friend class SECTREE_MAP;

		template <class _Func> LPENTITY	find_if (_Func & func) const
		{
			LPSECTREE_LIST::iterator it_tree = m_neighbor_list.begin();

			while (it_tree != m_neighbor_list.end())
			{
				ENTITY_SET::iterator it_entity = (*it_tree)->m_set_entity.begin();

				while (it_entity != (*it_tree)->m_set_entity.end())
				{
					if (func(*it_entity))
						return (*it_entity);

					++it_entity;
				}

				++it_tree;
			}

			return NULL;
		}

		template <class _Func> void ForEachAround(_Func & func)
		{
			// <Factor> Using snapshot copy to avoid side-effects
			FCollectEntity collector;
			LPSECTREE_LIST::iterator it = m_neighbor_list.begin();
			for (; it != m_neighbor_list.end(); ++it)
			{
				LPSECTREE sectree = *it;
				sectree->for_each_entity(collector);
			}
			collector.ForEach(func);
			/*
			LPSECTREE_LIST::iterator it_tree = m_neighbor_list.begin();
			for (; it_tree != m_neighbor_list.end(); ++it_tree) {
				(*it_tree)->for_each_entity(func);
			}
			*/
		}

		template <class _Func> void for_each_for_find_victim(_Func & func)
		{
			LPSECTREE_LIST::iterator it_tree = m_neighbor_list.begin();

			while (it_tree != m_neighbor_list.end())
			{
				// If the first one is found, return immediately
				if ((*(it_tree++))->for_each_entity_for_find_victim(func))
					return;
			}
		}
		template <class _Func> bool for_each_entity_for_find_victim(_Func & func)
		{
			auto it = m_set_entity.begin();

			while (it != m_set_entity.end())
			{
				if (func(*it++))
					return true;
			}
			return false;
		}
		

	public:
		SECTREE();
		~SECTREE();

		void				Initialize();
		void				Destroy();

		SECTREEID			GetID();

		bool				InsertEntity(LPENTITY ent);
		void				RemoveEntity(LPENTITY ent);

		void				SetRegenEvent(LPEVENT event);
		bool				Regen();

		void				IncreasePC();
		void				DecreasePC();

		void				BindAttribute(CAttribute* pAttribute);

		CAttribute *			GetAttributePtr() { return m_pAttribute; }

		uint32_t				GetAttribute(int32_t x, int32_t y);
		bool				IsAttr(int32_t x, int32_t y, uint32_t dwFlag);

		void				CloneAttribute(LPSECTREE tree);

		int32_t				GetEventAttribute(int32_t x, int32_t y);

		void				SetAttribute(uint32_t x, uint32_t y, uint32_t dwAttr);
		void				RemoveAttribute(uint32_t x, uint32_t y, uint32_t dwAttr);

	private:
		template <class _Func> void for_each_entity(_Func & func)
		{
			auto it = m_set_entity.begin();
			for (; it != m_set_entity.end(); ++it) {
				LPENTITY entity = *it;
				// <Factor> Sanity check
				if (entity->GetSectree() != this) {
					SysLog("<Factor> SECTREE-ENTITY relationship mismatch");
					m_set_entity.erase(it);
					continue;
				}
				func(entity);
			}
		}

		SECTREEID			m_id;
		ENTITY_SET			m_set_entity;
		LPSECTREE_LIST			m_neighbor_list;
		int32_t				m_iPCCount;
		bool				isClone;

		CAttribute *			m_pAttribute;
};
