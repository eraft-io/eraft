#ifndef __TRIVIALDB_OVERFLOW_PAGE__
#define __TRIVIALDB_OVERFLOW_PAGE__

#include <cassert>
#include <utility>
#include "page_defs.h"

class overflow_page : public general_page
{
public:
	using general_page::general_page;

	PAGE_FIELD_REF(magic, uint16_t, 0);
	PAGE_FIELD_REF(size,  uint16_t, 2);  // number of bytes of data in this page
	PAGE_FIELD_REF(next,  int,      4);
	PAGE_FIELD_PTR(block, char,     8);
	static constexpr int header_size() { return 8; }
	static constexpr int block_size()  { return PAGE_SIZE - header_size(); }

	void init()
	{
		magic_ref() = PAGE_OVERFLOW;
		size_ref()  = 0;
		next_ref()  = 0;
	}
};

#endif
