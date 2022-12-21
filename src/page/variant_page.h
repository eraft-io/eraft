#ifndef __TRIVIALDB_VARIANT_PAGE__
#define __TRIVIALDB_VARIANT_PAGE__

#include <cstring>
#include <cassert>
#include <utility>
#include "page_defs.h"
#include "pager.h"

/* For each item, let S be the size of it. If S > PAGE_BLOCK_MAX_SIZE,
 * then part of it will be stored in overflow pages and the first
 * PAGE_OV_KEEP_SIZE will stay in the data page. If S <= PAGE_BLOCK_MAX_SIZE,
 * all the data will be stored in the data page.
 * */

class variant_page : public general_page
{
	struct free_block_header
	{
		uint16_t size;
		uint16_t next;
	};

public:
	struct block_header
	{
		uint16_t size;  // block size (overflow is not included)
		int ov_page;    // overflow page
	};

private:
	char *allocate(int sz);
	void defragment();
	void set_freeblock(int offset);
	void erase(int pos, bool follow_ov_page);

public:
	using general_page::general_page;
	PAGE_FIELD_REF(magic,       uint16_t, 0);   // page type
	PAGE_FIELD_REF(flags,       uint16_t, 2);   // flags
	PAGE_FIELD_REF(free_block,  uint16_t, 4);   // pointer to the first freeslot
	PAGE_FIELD_REF(free_size,   uint16_t, 6);   // size of free space
	PAGE_FIELD_REF(size,        uint16_t, 8);   // number of items
	PAGE_FIELD_REF(bottom_used, uint16_t, 10);
	PAGE_FIELD_REF(next_page,   int,      12);
	PAGE_FIELD_REF(prev_page,   int,      16);
	PAGE_FIELD_PTR(slots,       uint16_t, 20);  // slots
	static constexpr int header_size() { return 20; }
	int used_size() {
		return PAGE_SIZE - free_size() - size() * 2 - header_size();
	}

	bool underflow()
	{
		return free_size() > PAGE_FREE_SPACE_MAX
			|| size() < PAGE_BLOCK_MIN_NUM / 2;
	}

	bool underflow_if_remove(int pos)
	{
		assert(0 <= pos && pos < size());
		int free_size_if_remove = free_size() - get_block(pos).first.size - 2;
		return free_size_if_remove > PAGE_FREE_SPACE_MAX
			|| size() - 1 < PAGE_BLOCK_MIN_NUM / 2;
	}

	void init(int = 0);
	void erase(int pos) { erase(pos, true); }
	bool insert(int pos, const char *data, int data_size);
	void move_from(variant_page page, int src_pos, int dest_pos);

	/* Split the (full) page into two parts, each of which has at least
	 * (PAGE_BLOCK_MIN_NUM / 2) used blocks, and the upper part of the
	 * splited page id is returned. If the block requirement cannnot be
	 * satisfied, 0 is returned. The free_size of the two parts is
	 * as close as possible. */
	std::pair<int, variant_page> split(int cur_id);
	bool merge(variant_page page, int cur_id);

	std::pair<block_header, char*> get_block(int id)
	{
		assert(0 <= id && id < size());
		char *addr = buf + slots()[id];
		return { *(block_header*)addr, addr + sizeof(block_header) };
	}
};

#endif
