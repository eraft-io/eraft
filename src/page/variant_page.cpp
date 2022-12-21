#include <algorithm>
#include "variant_page.h"
#include "overflow_page.h"

#define LOAD_FREEBLK(offset) \
	reinterpret_cast<free_block_header*>(buf + (offset))
#define LOAD_BLK(offset) \
	reinterpret_cast<block_header*>(buf + (offset))

void variant_page::init(int)
{
	magic_ref() = PAGE_VARIANT;
	flags_ref() = 0;
	free_block_ref() = 0;
	free_size_ref() = PAGE_SIZE - header_size();
	size_ref() = 0;
	bottom_used_ref() = 0;
	next_page_ref() = prev_page_ref() = 0;
}

void variant_page::erase(int pos, bool follow_ov_page)
{
	assert(0 <= pos && pos < size());

	uint16_t* slots_ptr = slots();
	uint16_t slot = slots_ptr[pos];
	for(int i = pos, s = size(); i < s; ++i)
		slots_ptr[i] = slots_ptr[i + 1];

	block_header *header = (block_header*)(buf + slot);
	if(header->ov_page && follow_ov_page)
		pg->free_overflow_page(header->ov_page);

	free_size_ref() += 2 + header->size;
	--size_ref();
	set_freeblock(slot);
}

void variant_page::set_freeblock(int offset)
{
	block_header *header = (block_header*)(buf + offset);
	if(offset + bottom_used() == PAGE_SIZE)
	{
		bottom_used_ref() -= header->size;
	} else {
		free_block_header *fb_header = LOAD_FREEBLK(offset);
		fb_header->size = header->size;
		fb_header->next = free_block();
		free_block_ref() = offset;
	}
}

bool variant_page::insert(int pos, const char *data, int data_size)
{
	assert(0 <= pos && pos <= size());
	int real_size = data_size + sizeof(block_header);
	bool ov = (real_size > PAGE_BLOCK_MAX_SIZE);
	int size_required = ov ? PAGE_OV_KEEP_SIZE : real_size;
	char *dest = allocate(size_required);
	if(!dest) return false;

	// set slot
	uint16_t *slots_ptr = slots();
	for(int i = size(); i > pos; --i)
		slots_ptr[i] = slots_ptr[i - 1];
	slots_ptr[pos] = dest - buf;
	
	// set data
	block_header* header = (block_header*)dest;
	header->size = size_required;
	int copied_size = size_required - sizeof(block_header);
	std::memcpy(dest + sizeof(block_header), data, copied_size);

	free_size_ref() -= 2; // size of a slot
	++size_ref();

	if(!ov)
	{
		header->ov_page = 0;
	} else {
		auto create_and_copy = [&](const char *src, int size) {
			int pid = pg->new_page();
			overflow_page page = overflow_page(pg->read_for_write(pid), pg);
			page.init();
			page.size_ref() = size;
			std::memcpy(page.block(), src, size);
			return std::make_pair(pid, page);
		};

		data += copied_size;
		int remain = data_size - copied_size;
		int to_copy = std::min(overflow_page::block_size(), remain);

		auto ret = create_and_copy(data, to_copy);
		overflow_page ov_page = ret.second;
		data   += to_copy;
		remain -= to_copy;
		header->ov_page = ret.first;

		while(remain > 0)
		{
			to_copy = std::min(overflow_page::block_size(), remain);
			auto ret = create_and_copy(data, to_copy);
			ov_page.next_ref() = ret.first;
			ov_page = ret.second;
			data   += to_copy;
			remain -= to_copy;
		}
	}

	return true;
}

std::pair<int, variant_page> variant_page::split(int cur_id)
{
	if(size() < PAGE_BLOCK_MIN_NUM)
		return { 0, { nullptr, nullptr } };
	int page_id = pg->new_page();
	if(!page_id) return { 0, { nullptr, nullptr } };
	variant_page upper_page { pg->read_for_write(page_id), pg };
	upper_page.init();
	upper_page.flags_ref() = flags();

	if(next_page())
	{
		variant_page page { pg->read_for_write(next_page()), pg };
		assert(page.magic() == magic());
		page.prev_page_ref() = page_id;
	}
	upper_page.next_page_ref() = next_page();
	upper_page.prev_page_ref() = cur_id;
	next_page_ref() = page_id;

	int to_move = used_size() / 2, moved = 0;
	char *dest_addr = upper_page.buf + PAGE_SIZE;
	uint16_t *dest_slots = upper_page.slots();
	for(int i = size() - 1; i >= PAGE_BLOCK_MIN_NUM / 2; --i)
	{
		char *src_addr = buf + slots()[i];
		block_header *src_header = (block_header*)src_addr;

		int size_req = src_header->size + 2;
		if(to_move - size_req < 0 && upper_page.size() >= PAGE_BLOCK_MIN_NUM / 2)
			break;

		dest_addr -= src_header->size;
		*dest_slots++ = dest_addr - upper_page.buf;
		std::memcpy(dest_addr, src_addr, src_header->size);

		--size_ref();
		++upper_page.size_ref();
		free_size_ref() += size_req;
		upper_page.free_size_ref() -= size_req;
		set_freeblock(slots()[i]);

		to_move  -= size_req;
		moved += size_req;
	}

	std::reverse(upper_page.slots(), upper_page.slots() + upper_page.size());
	upper_page.bottom_used_ref() = moved;

	assert(size() >= PAGE_BLOCK_MIN_NUM / 2);
	assert(upper_page.size() >= PAGE_BLOCK_MIN_NUM / 2);

	return { page_id, upper_page };
}

bool variant_page::merge(variant_page page, int cur_id)
{
	int space_req = PAGE_SIZE - page.free_size() - header_size();
	if(space_req > free_size())
		return false;

	next_page_ref() = page.next_page_ref();
	if(next_page())
	{
		variant_page page { pg->read_for_write(next_page()), pg };
		assert(page.magic() == magic());
		page.prev_page_ref() = cur_id;
	}

	defragment();
	uint16_t *src_slot = page.slots();
	uint16_t *dest_slot = slots() + size();
	char *dest = buf + PAGE_SIZE - bottom_used();
	for(int i = 0, t = page.size(); i < t; ++i)
	{
		char *src = page.buf + *src_slot++;
		int sz = ((block_header*)src)->size;
		dest -= sz;
		std::memcpy(dest, src, sz);
		*dest_slot++ = dest - buf;
		bottom_used_ref() += sz;
	}

	size_ref() += page.size();
	free_size_ref() -= space_req;

	return true;
}

char* variant_page::allocate(int sz)
{
	if(free_size() < sz + 2) return nullptr;  // no space for data

	int unallocated = PAGE_SIZE - (header_size() + size() * 2 + bottom_used());
	auto free_blk = LOAD_FREEBLK(free_block());

	if(unallocated < 2 && free_size() - 2 >= sz)
	{
		defragment();
		return allocate(sz);
	}

	if(free_block() && free_blk->size >= sz)
	{
		char *addr = buf + free_block();
		if(free_blk->size - sz < PAGE_FREE_BLOCK_MIN_SIZE)
		{
			free_block_ref() = free_blk->next;
		} else {
			auto free_blk_new = LOAD_FREEBLK(free_block() + sz);
			free_blk_new->size = free_blk->size - sz;
			free_blk_new->next = free_blk->next;
			free_block_ref() += sz;
		}

		free_size_ref() -= sz;
		return addr;
	} else if(unallocated - 2 >= sz) {
		// minus one slot size for this item
		bottom_used_ref() += sz;
		free_size_ref() -= sz;
		return buf + PAGE_SIZE - bottom_used();
	} else if(free_size() - 2 >= sz) {
		defragment();
		return allocate(sz);
	} else return nullptr;
}

void variant_page::defragment()
{
	int sz = size();
	uint16_t *slots_ptr = slots();
	int *index = new int[sz];
	for(int i = 0; i != sz; ++i)
		index[i] = i;
	std::sort(index, index + size(), [=](int a, int b) {
		return slots_ptr[a] > slots_ptr[b];
	} );

	int total_blk_sz = 0;
	char *ptr = buf + PAGE_SIZE;
	for(int i = 0; i < sz; ++i)
	{
		char *blk = buf + slots_ptr[index[i]];
		int blk_sz = reinterpret_cast<block_header*>(blk)->size;
		total_blk_sz += blk_sz;
		ptr -= blk_sz;
		slots_ptr[index[i]] = ptr - buf;
		std::memmove(ptr, blk, blk_sz);
	}

	delete[] index;

	free_block_ref()  = 0;
	bottom_used_ref() = total_blk_sz;

	assert(total_blk_sz + header_size() + 2 * size() + free_size() == PAGE_SIZE);
}

void variant_page::move_from(variant_page page, int src_pos, int dest_pos)
{
	assert(page.magic() == magic());
	auto src_block = page.get_block(src_pos);
	bool succ_ins = insert(dest_pos,
		src_block.second, src_block.first.size - sizeof(block_header));
	UNUSED(succ_ins);
	assert(succ_ins);
	*(block_header*)(buf + slots()[dest_pos]) = src_block.first;
	page.erase(src_pos, false);
}
