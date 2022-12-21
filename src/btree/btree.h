#ifndef __TRIVIALDB_BTREE__
#define __TRIVIALDB_BTREE__

#include "../utils/comparer.h"
#include "../page/pager.h"
#include "../page/fixed_page.h"
#include "../page/data_page.h"
#include "../page/index_leaf_page.h"
#include <functional>
#include <memory>
#include <type_traits>

/* Each node of the b-tree is a page.
 * For an interior node, the key of a page element is the largest
 * element of its children. */

template<typename KeyType, typename Comparer, typename Copier>
class btree
{
	pager *pg;
	int root_page_id, field_size;
	Comparer compare;
	Copier copy_to_temp;
public:
	typedef KeyType key_t;
	typedef fixed_page<key_t> interior_page;
	typedef typename std::conditional<
		std::is_same<KeyType, const char*>::value,
		index_leaf_page<key_t>,
		data_page<key_t>>::type leaf_page;
	typedef std::pair<int, int> search_result;  // (page_id, pos)
public:
	/* create/load a btree
	 * If root_page_id = 0, create a new btree.
	 * If root_page_id != 0, load an existed btree */
	btree(pager *pg, int root_page_id, int field_size, Comparer compare, Copier copier);

	void insert(key_t key, const char* data, int data_size);
	// erase one of the elements with specified key randomly
	bool erase(key_t key);
	// the first element x for which x >= key
	search_result lower_bound(key_t key);

	int get_root_page_id() { return root_page_id; }

private:
	struct insert_ret
	{
		bool split;
		int upper_pid;
		char *lower_half, *upper_half;
	};

	struct erase_ret
	{
		bool found;
		bool merged_left, merged_right;
		int merged_pid;
		key_t largest;
	};

	struct merge_ret
	{
		bool merged_left, merged_right;
		int merged_pid;
	};

	template<typename Page, typename ChPage>
	insert_ret insert_post_process(int, int, int, insert_ret);
	template<typename Page>
	void insert_split_root(insert_ret);
	insert_ret insert_interior(int, char*, key_t, const char*, int);
	insert_ret insert_leaf(int, char*, key_t, const char*, int);
	search_result lower_bound(int now, key_t key);
	erase_ret erase(int, key_t);
	template<typename Page>
	merge_ret erase_try_merge(int pid, char *addr);
};

class int_btree : public btree<int, int(*)(int, int), int(*)(int)>
{
	static int copy_int(int x) { return x; }
public:
	int_btree(pager *pg, int root_page_id = 0)
		: btree(pg, root_page_id, sizeof(int),
				&integer_comparer,
				&int_btree::copy_int) {}
};

namespace __impl
{
	template<typename T>
	struct array_deleter
	{
		void operator () (T const* p) { delete [] p; }
	};

	struct index_btree_copier_t
	{
		int size;
		std::shared_ptr<char> buf;
	public:
		index_btree_copier_t(int size)
			: size(size),
			  buf(new char[size], array_deleter<char>()) {}

		char *operator () (const char *src)
		{
			std::memcpy(buf.get(), src, size);
			return buf.get();
		}
	};
}

class index_btree : public btree<const char*,
	std::function<int(const char*, const char*)>,
	__impl::index_btree_copier_t>
{
	typedef std::function<int(const char*, const char*)> comparer_t;
	typedef btree<const char*, comparer_t,
		__impl::index_btree_copier_t> base_class;
public:
	index_btree(pager *pg,
			int root_page_id,
			int size,
			comparer_t comparer
		) : btree(
			pg,
			root_page_id,
			size,
			comparer,
			__impl::index_btree_copier_t(size)
		) {}

	void insert(const char* key, int rid)
	{
		base_class::insert(key, key, rid);
	}
};

#endif
