#ifndef __TRIVIALDB_CACHE_MANAGER__
#define __TRIVIALDB_CACHE_MANAGER__
#include <assert.h>
#include <cstring>

#include "../defs.h"

class cache_manager
{
private:
	int head;
	struct node_t
	{
		int prev, next;
	} *nodes;
public:
	cache_manager()
	{
		head = 0;
		nodes = new node_t[PAGE_CACHE_CAPACITY];
		for(int i = 0; i != PAGE_CACHE_CAPACITY; ++i)
		{
			nodes[i].next = (i + 1 == PAGE_CACHE_CAPACITY) ? 0 : i + 1;
			nodes[i].prev = i ? i - 1 : PAGE_CACHE_CAPACITY - 1;
		}
	}

	~cache_manager()
	{
		delete[] nodes;
	}

public:
	void access(int id)
	{
		assert(0 <= id && id < PAGE_CACHE_CAPACITY);

		if(id == head) return;

		// remove
		nodes[nodes[id].prev].next = nodes[id].next;
		nodes[nodes[id].next].prev = nodes[id].prev;

		// insert
		nodes[nodes[head].prev].next = id;
		nodes[id].next   = head;
		nodes[id].prev   = nodes[head].prev;
		nodes[head].prev = id;

		head = id;

		assert(_check_valid());
	}

	int last() const 
	{
		return nodes[head].prev;
	}

private:
	int _check_valid() const
	{
		char *mark = new char[PAGE_CACHE_CAPACITY];
		std::memset(mark, 0, PAGE_CACHE_CAPACITY);
		for(int i = 0, k = head; i != PAGE_CACHE_CAPACITY; ++i, k = nodes[k].next)
			mark[k] = 1;
		int ret = 1;
		for(int i = 0; i != PAGE_CACHE_CAPACITY; ++i)
			ret &= mark[i];
		delete[] mark;
		return ret;
	}
};

#endif
