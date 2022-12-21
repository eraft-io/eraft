#ifndef __TRIVIALDB_FIXED_PAGE__
#define __TRIVIALDB_FIXED_PAGE__

#include <cstring>
#include <cassert>
#include <utility>
#include "page_defs.h"
#include "pager.h"

template<typename T>
class fixed_page : public general_page
{
public:
	using general_page::general_page;
	PAGE_FIELD_REF(magic,       uint16_t, 0);   // page type
	PAGE_FIELD_REF(field_size,  uint16_t, 2);   // size of keywords
	PAGE_FIELD_REF(size,        int,      4);   // number of items
	PAGE_FIELD_REF(next_page,   int,      8);
	PAGE_FIELD_REF(prev_page,   int,      12);
	PAGE_FIELD_PTR(children,    int,      16);   // pointer to child pages
	PAGE_FIELD_ACCESSER(T,   key,   begin() + id * field_size());
	PAGE_FIELD_ACCESSER(int, child, children() + id);
	static constexpr int header_size() { return 16; }
	int capacity() { return (PAGE_SIZE - header_size()) / (sizeof(T) + 4); }
	bool full() { return capacity() == size(); }
	bool empty() { return size() == 0; }
	bool underflow() { return size() < capacity() / 2 - 1; }
	bool underflow_if_remove(int) { return size() < capacity() / 2; }
	void init(int field_size)
	{
		magic_ref() = PAGE_FIXED;
		field_size_ref() = field_size;
		size_ref() = 0;
		next_page_ref() = prev_page_ref() = 0;
	}

	char* begin() { return end() - size() * field_size(); }
	char* end() { return buf + PAGE_SIZE; }

	bool insert(int pos, const T& key, int child);
	void erase(int pos);
	std::pair<int, fixed_page> split(int cur_id);
	bool merge(fixed_page page, int cur_id);
	void move_from(fixed_page page, int src_pos, int dest_pos);
};

/* Specialized class (for pointer) */
template<> inline
int fixed_page<const char*>::capacity()
{
	return (PAGE_SIZE - header_size()) / (field_size() + 4);
}

template<> inline
const char* fixed_page<const char*>::get_key(int pos)
{
	return buf + PAGE_SIZE - (size() - pos) * field_size();
}

template<> inline
void fixed_page<const char*>::set_key(int pos, const char * const &data)
{
	std::memcpy(
		buf + PAGE_SIZE - (size() - pos) * field_size(),
		data, field_size());
}

/* General class */
template<typename T>
bool fixed_page<T>::insert(int pos, const T& key, int child)
{
	assert(0 <= pos && pos <= size());
	if(full()) return false;

	int* ch_ptr = children();
	for(int i = size() - 1; i >= pos; --i)
		ch_ptr[i + 1] = ch_ptr[i];
	ch_ptr[pos] = child;

	std::memmove(
		reinterpret_cast<char*>(end()) - (size() + 1) * field_size(),
		reinterpret_cast<char*>(end()) - size() * field_size(),
		pos * field_size()
	);

	++size_ref();
	set_key(pos, key);
	return true;
}

template<typename T>
void fixed_page<T>::erase(int pos)
{
	assert(0 <= pos && pos < size());

	int* ch_ptr = children() + pos;
	for(int i = size(); i > pos; --i, ++ch_ptr)
		*ch_ptr = *(ch_ptr + 1);
	*ch_ptr = 0;

	std::memmove(
		reinterpret_cast<char*>(end()) - (size() - 1) * field_size(),
		reinterpret_cast<char*>(end()) - size() * field_size(),
		pos * field_size()
	);

	--size_ref();
}

template<typename T>
std::pair<int, fixed_page<T>> fixed_page<T>::split(int cur_id)
{
	if(size() < PAGE_BLOCK_MIN_NUM)
		return { 0, { nullptr, nullptr } };

	int page_id = pg->new_page();
	if(!page_id) return { 0, { nullptr, nullptr } };
	fixed_page upper_page { pg->read_for_write(page_id), pg };
	upper_page.init(field_size());
	upper_page.magic_ref() = magic();

	if(next_page())
	{
		fixed_page page { pg->read_for_write(next_page()), pg };
		assert(page.magic() == magic());
		page.prev_page_ref() = page_id;
	}
	upper_page.next_page_ref() = next_page();
	upper_page.prev_page_ref() = cur_id;
	next_page_ref() = page_id;

	int lower_size = size() >> 1;
	int upper_size = size() - lower_size;
	std::memcpy(
		upper_page.children(),
		children() + (size() - upper_size),
		upper_size * sizeof(int)
	);

	std::memcpy(
		upper_page.end() - upper_size * field_size(), 
		end() - upper_size * field_size(), 
		field_size() * upper_size
	);

	std::memmove(end() - lower_size * field_size(), begin(), field_size() * lower_size);
	size_ref() = lower_size;
	upper_page.size_ref() = upper_size;
	return { page_id, upper_page };
}

template<typename T>
bool fixed_page<T>::merge(fixed_page page, int cur_id)
{
	if(size() + page.size() > capacity())
		return false;

	next_page_ref() = page.next_page_ref();
	if(next_page())
	{
		fixed_page page { pg->read_for_write(next_page()), pg };
		assert(page.magic() == magic());
		page.prev_page_ref() = cur_id;
	}

	std::memcpy(children() + size(), page.children(), 4 * page.size());
	std::memmove(begin() - page.size() * field_size(), begin(), field_size() * page.size());
	std::memcpy(end() - page.size() * field_size(), page.begin(), field_size() * page.size());
	size_ref() += page.size();

	return true;
}

template<typename T>
void fixed_page<T>::move_from(fixed_page page, int src_pos, int dest_pos)
{
	assert(page.magic() == magic());
	bool succ_ins = insert(dest_pos,
		page.get_key(src_pos),
		page.get_child(src_pos));
	page.erase(src_pos);
	assert(succ_ins);
	UNUSED(succ_ins);
}

#endif
