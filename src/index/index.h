#ifndef __TRIVIALDB_INDEX__
#define __TRIVIALDB_INDEX__
#include <functional>
#include "../btree/btree.h"
#include "../btree/iterator.h"

class index_manager
{
	char *buf;
	index_btree *btr;
	int size;
	pager *pg;

	void fill_buf(const char *key, int rid);

public:
	typedef int(*comparer_t)(const char*, const char*);

	index_manager(pager *pg, int size, int root_pid, comparer_t comparer);
	~index_manager();

	int get_root_pid();
	void insert(const char *key, int rid);
	void erase(const char *key, int rid);
	index_btree::search_result lower_bound(const char *key, int rid = 0);
	btree_iterator<index_btree::leaf_page> get_iterator_lower_bound(const char *key, int rid = 0);

};

#endif
