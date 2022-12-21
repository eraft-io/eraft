#ifndef __TRIVIALDB_PAGE_DEFS__
#define __TRIVIALDB_PAGE_DEFS__

#include <stdint.h>
#include "../defs.h"

#define PAGE_FIELD_REF(name, type, offset) \
	type name() { return *reinterpret_cast<type*>(buf + offset); } \
	type& name##_ref() { return *reinterpret_cast<type*>(buf + offset); }

#define PAGE_FIELD_PTR(name, type, offset) \
	type* name() { return reinterpret_cast<type*>(buf + offset); }

#define PAGE_FIELD_ACCESSER(type, name, addr) \
	type get_##name(int id) { \
		assert(0 <= id && id < size()); \
		return *reinterpret_cast<type*>(addr); \
	} \
	void set_##name(int id, const type& val) { \
		assert(0 <= id && id < size()); \
		*reinterpret_cast<type*>(addr) = val; \
	}

class pager;

struct general_page
{
	char* buf;
	pager* pg;
	general_page(char *buf, pager *pg)
		: buf(buf), pg(pg) {}
	general_page(const general_page&) = default;

	static uint16_t get_magic_number(const void* addr) {
		return *reinterpret_cast<const uint16_t*>(addr);
	}
};

#endif
