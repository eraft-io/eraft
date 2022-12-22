#ifndef __TRIVIALDB_DATA_PAGE__
#define __TRIVIALDB_DATA_PAGE__

#include <cassert>
#include "variant_page.h"

/* In the data page, the first sizeof(Key) bytes must be
 * the key, and the remaining bytes are free to use. */
template<typename Key>
class data_page : public variant_page
{
	static_assert(
		sizeof(block_header) + sizeof(Key) <= PAGE_OV_KEEP_SIZE,
		"Key size of data page is too large."
	);
public:
	using variant_page::variant_page;

	PAGE_FIELD_ACCESSER(Key, key, get_block(id).second);

	std::pair<int, data_page> split(int cur_id)
	{
		auto ret = variant_page::split(cur_id);
		return { ret.first,
			*reinterpret_cast<data_page*>(&ret.second)
		};
	}
};

#endif
