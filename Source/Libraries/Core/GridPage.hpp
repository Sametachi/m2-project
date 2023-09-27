#pragma once
#include <algorithm>
#include <memory>
#include <cstdint>
#include <cassert>
//#include "../libthecore/log.hpp"

template <class T>
class PagedGrid
{
public:
	PagedGrid(uint32_t width, uint32_t height, uint32_t pages = 1, PagedGrid<T>* prev = nullptr);

	uint32_t GetSize() const;
	uint32_t GetWidth() const;
	uint32_t GetHeight() const;

	const T& Get(uint32_t position) const;
	const T& Get(uint32_t x, uint32_t y) const;

	bool Put(const T& item, uint32_t x, uint32_t y, uint32_t height = 1);
	bool PutPos(const T& item, uint32_t position, uint32_t height = 1);

	void Clear();

	void Clear(uint32_t x, uint32_t y, uint32_t height = 1);
	void ClearPos(uint32_t position, uint32_t height = 1);
	bool FindBlank(uint32_t height, uint32_t& blank) const;

	bool IsEmpty(uint32_t position, uint32_t height = 1, const T* except = nullptr) const;

	T* begin() { return m_grid.get(); }
	const T* begin() const { return m_grid.get(); }

	T* end() { return &m_grid[GetSize()]; }
	const T* end() const { return &m_grid[GetSize()]; }

protected:
	std::unique_ptr<T[]> m_grid;
	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_pages;
};

template <class T>
PagedGrid<T>::PagedGrid(uint32_t width, uint32_t height, uint32_t pages, PagedGrid<T>* prev)
	: m_grid(new T[width * height * pages])
	, m_width(width)
	, m_height(height)
	, m_pages(pages)
{
	if (prev) 
	{
		const uint32_t cSize = std::min<uint32_t>(GetSize(), prev->GetSize());
		std::uninitialized_copy_n(prev->m_grid.get(), cSize, m_grid.get());
	}
	else 
	{
		std::uninitialized_fill_n(m_grid.get(), GetSize(), T());
	}
}

template <class T>
uint32_t PagedGrid<T>::GetSize() const
{
	return m_height * m_width * m_pages;
}

template <class T>
uint32_t PagedGrid<T>::GetWidth() const
{
	return m_width;
}

template <class T>
uint32_t PagedGrid<T>::GetHeight() const
{
	return m_height;
}

template <class T>
const T& PagedGrid<T>::Get(uint32_t position) const
{
	assert(position < GetSize() && "Out of bounds");
	return m_grid[position];
}

template <class T>
const T& PagedGrid<T>::Get(uint32_t x, uint32_t y) const
{
	assert(x < m_width && y < m_height * m_pages && "Out of bounds");
	return m_grid[x + y * m_width];
}

template <class T>
bool PagedGrid<T>::Put(const T& item, uint32_t x, uint32_t y, uint32_t height)
{
	return PutPos(item, x + y * m_width, height);
}

template <class T>
void PagedGrid<T>::Clear()
{
	std::fill_n(m_grid.get(), GetSize(), T());
}

template <class T>
bool PagedGrid<T>::PutPos(const T& item, uint32_t position, uint32_t height)
{
	assert(height > 0 && "Sanity check");

	const uint32_t lastPos = position + ((height - 1) * m_width);
	const uint32_t pageSize = m_width * m_height;

	assert(lastPos < GetSize() && "Out of range");

	if (position / pageSize != lastPos / pageSize) 
	{
		//STORM_LOG(Error, "Item Pos %u Height %u is across pages", position, height);
		return false;
	}

	for (uint32_t i = position; i <= lastPos; i += m_width)
		m_grid[i] = item;

	return true;
}

template <class T>
void PagedGrid<T>::ClearPos(uint32_t position, uint32_t height)
{
	assert(height > 0 && "Sanity check");

	const uint32_t lastPos = position + (height - 1) * m_width;
	assert(lastPos < GetSize() && "Out of bounds");

	for (; position <= lastPos; position += m_width)
		m_grid[position] = T();
}

template <class T>
bool PagedGrid<T>::FindBlank(uint32_t height, uint32_t& blank) const
{
	assert(height < m_height && "Sanity check");
	assert(height > 0 && "Sanity check");

	const uint32_t pageSize = m_width * m_height;
	const uint32_t possibleHeight = m_height - height;

	for (uint32_t i = 0; i < m_pages; ++i) 
	{
		for (uint32_t y = 0; y <= possibleHeight; ++y) 
		{
			for (uint32_t x = 0; x < m_width; ++x) 
			{
				const auto p = pageSize * i + y * m_width + x;
				if (IsEmpty(p, height)) 
				{
					blank = p;
					return true;
				}
			}
		}
	}

	return false;
}

template <class T>
bool PagedGrid<T>::IsEmpty(uint32_t position, uint32_t height, const T* except) const
{
	assert(height > 0 && "Sanity check");

	const uint32_t lastPos = position + ((height - 1) * m_width);
	if (lastPos >= GetSize()) 
	{
		//STORM_DLOG(Error, "range [%u, %u+%u=%u) > %u", position, height, lastPos + m_width, GetSize());
		return false;
	}

	const uint32_t pageSize = m_width * m_height;
	if (position / pageSize != lastPos / pageSize)
		return false;

	for (uint32_t i = position; i <= lastPos; i += m_width) 
	{
		if (m_grid[i] && (!except || m_grid[i] != *except))
			return false;
	}

	return true;
}