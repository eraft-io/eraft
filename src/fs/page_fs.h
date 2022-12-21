#ifndef __TRIVIALDB_PAGE_FS__
#define __TRIVIALDB_PAGE_FS__

#include <utility>
#include <cstdio>
#include <unordered_map>

#include "../defs.h"
#include "fid_manager.h"
#include "cache_manager.h"

/* The first page is file info, not counted into `page_num`
 * `first_freepage` indicates the first freepage if it is not zero
 * and there is no freepage if it is zero */
struct page_fs_header_t
{
	int page_num;
	int first_freepage;
};

class page_fs
{
	struct pair_hash
	{
		template<typename T1, typename T2>
		std::size_t operator () (const std::pair<T1, T2> &p) const {
			return std::hash<T1>{}(p.first) ^ (std::hash<T2>{}(p.second) << 1);
		}
	};
private:
	/* cache */
	char dirty[PAGE_CACHE_CAPACITY];
	char buffer[PAGE_CACHE_CAPACITY * PAGE_SIZE];
	char tmp_buffer[PAGE_SIZE];
	cache_manager cm;

	typedef std::pair<int, int> file_page_t;
	std::unordered_map<file_page_t, int, pair_hash> page2index;

	// cache is used if `first` != 0
	file_page_t index2page[PAGE_CACHE_CAPACITY];

	/* file */
	fid_manager fm;
	FILE *files[MAX_FILE_ID];
	page_fs_header_t file_info[MAX_FILE_ID];

private:
	char* read(int file_id, int page_id, int& index);
	void free_last_cache();
	void write_page_to_file(int file_id, int page_id, const char* data);

private:
	page_fs();

public:
	~page_fs();

	int open(const char* filename);
	void close(int file_id);
	void writeback(int file_id);

	/* allocate a new page */
	int allocate(int file_id);
	/* free an existed page */
	void deallocate(int file_id, int page_id);

	void mark_dirty(int file_id, int page_id);

	char* read(int file_id, int page_id) {
		int index;
		return read(file_id, page_id, index);
	}

	char* read_for_write(int file_id, int page_id) {
		int index;
		char *buf = read(file_id, page_id, index);
		dirty[index] = 1;
		return buf;
	}

public:
	static page_fs* get_instance()
	{
		static page_fs fs;
		return &fs;
	}
};

#endif
