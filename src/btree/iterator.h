#ifndef __TRIVIALDB_BTREE_ITERATOR__
#define __TRIVIALDB_BTREE_ITERATOR__

#include "btree.h"
#include "../defs.h"
#include <utility>

template<typename PageType>
class btree_iterator
{
private:
	pager *pg;
	int pid, pos;
	int cur_size, prev_pid, next_pid;

	void load_info(int p)
	{
		pid = p;
		if(p)
		{
			PageType page { pg->read(p), pg };
			assert(page.magic() == PAGE_VARIANT || page.magic() == PAGE_INDEX_LEAF);
			cur_size = page.size();
			next_pid = page.next_page();
			prev_pid = page.prev_page();
		}
	}
public:
	typedef std::pair<int, int> value_t;
public:
	btree_iterator(pager *pg, int pid, int pos)
		: pg(pg), pid(pid), pos(pos) { load_info(pid); }

	btree_iterator(pager *pg, value_t p)
		: btree_iterator(pg, p.first, p.second) {}

	pager *get_pager() { return pg; }
	value_t get() { return { pid, pos }; }
	value_t operator * () { return get(); }
	value_t next()
	{
		assert(pid);
		if(++pos == cur_size)
		{
			load_info(next_pid);
			pos = 0;
		}

		return get();
	}
	
	value_t prev()
	{
		assert(pid);
		if(pos-- == 0)
		{
			load_info(prev_pid);
			pos = cur_size - 1;
		}

		return get();
	}

	bool is_end() { return pid == 0; }
};

#endif
