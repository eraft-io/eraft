#ifndef __TRIVIALDB_RECORD__
#define __TRIVIALDB_RECORD__

#include "../page/pager.h"
#include "../page/data_page.h"

class table_manager;
class record_manager
{
	pager *pg;
	int pid, pos, cur_pid;
	char *cur_buf;
	int remain, next_pid, offset;
	bool dirty;
public:
	record_manager(pager *pg) : pg(pg), pid(0) {}
	void open(int pid, int pos, bool dirty);
	void open(std::pair<int, int> pw, bool dirty) {
		open(pw.first, pw.second, dirty);
	}
	record_manager& seek(int offset);
	record_manager& write(const void* data, int size);
	record_manager& read(void* buf, int size);
	record_manager& forward(int size);
	// goto next page
	bool forward_page();
	std::pair<char*, int> ptr_for_write();
	std::pair<const char*, int> ptr() { return { cur_buf, remain }; }
	bool valid() const { return pid != 0; }
};

inline std::pair<char*, int> record_manager::ptr_for_write()
{
	if(!dirty)
		pg->mark_dirty(cur_pid);
	return { cur_buf, remain };
}

#endif
