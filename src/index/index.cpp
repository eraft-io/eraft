#include "index.h"
#include "../utils/comparer.h"
#include <cstring>

index_manager::index_manager(pager *pg, int size, int root_pid, comparer_t comparer)
{
	this->pg = pg;
	this->size = size;
	// [rid, nullmark, data]
	buf = new char[size + sizeof(int) + 1];
	btr = new index_btree(pg, root_pid, size + sizeof(int) + 1,
		[comparer](const char *a, const char *b) -> int {
			if(a[4] != b[4])
			{
				// one of A and B is NULL
				return a[4] ? -1 : 1;
			} else if(!a[4]) {
				// A and B are not NULL
				int r = comparer(a + sizeof(int) + 1, b + sizeof(int) + 1);
				if(r != 0) return r;
			}

			return integer_comparer(*(int*)a, *(int*)b);
		} );
}

index_manager::~index_manager()
{
	delete []buf;
	delete btr;
	buf = nullptr;
	btr = nullptr;
}

int index_manager::get_root_pid()
{
	return btr->get_root_page_id();
}

void index_manager::fill_buf(const char *key, int rid)
{
	*(int*)buf = rid;
	if(key != nullptr)
	{
		buf[4] = 0;
		std::memcpy(buf + sizeof(int) + 1, key, size);
	} else {
		buf[4] = 1;
		std::memset(buf + sizeof(int) + 1, 0, size);
	}
}

void index_manager::insert(const char *key, int rid)
{
	fill_buf(key, rid);
	btr->insert(buf, rid);
}

void index_manager::erase(const char *key, int rid)
{
	fill_buf(key, rid);
	bool ret = btr->erase(buf);
	assert(ret);
	UNUSED(ret);
}

index_btree::search_result index_manager::lower_bound(const char *key, int rid)
{
	fill_buf(key, rid);
	return btr->lower_bound(buf);
}

btree_iterator<index_btree::leaf_page> index_manager::get_iterator_lower_bound(const char *key, int rid)
{
	auto ret = lower_bound(key, rid);
	return { pg, ret.first, ret.second };
}
