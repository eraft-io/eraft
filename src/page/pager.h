#ifndef __TRIVIALDB_PAGER__
#define __TRIVIALDB_PAGER__

#include <utility>
#include "../fs/page_file.h"
#include "overflow_page.h"

class pager : public page_file
{
public:
	using page_file::page_file;

	void free_overflow_page(int page_id, bool recursive = true)
	{
		auto ov_page = overflow_page(read_for_write(page_id), this);
		assert(ov_page.magic() == PAGE_OVERFLOW);
		int next_page_id = ov_page.next();
		free_page(page_id);
		if(recursive && next_page_id)
			free_overflow_page(next_page_id, true);
	}
};

#endif

