#ifndef __TRIVIALDB_PAGE_FILE__
#define __TRIVIALDB_PAGE_FILE__

#include "page_fs.h"

class page_file
{
	int fid;
public:
	page_file() : fid(0) {}
	page_file(const char* filename) : fid(0) { open(filename); }
	~page_file() { close(); }
	
	bool open(const char* filename)
	{
		page_fs *fs = page_fs::get_instance();
		if(fid) fs->close(fid);
		fid = fs->open(filename);
		return fid;
	}

	void close()
	{
		if(fid) 
			page_fs::get_instance()->close(fid);
		fid = 0;
	}

	void flush()
	{
		page_fs::get_instance()->writeback(fid);
	}

	int new_page()
	{
		return page_fs::get_instance()->allocate(fid);
	}

	void free_page(int page_id)
	{
		page_fs::get_instance()->deallocate(fid, page_id);
	}

	char* read(int page_id)
	{
		return page_fs::get_instance()->read(fid, page_id);
	}

	char* read_for_write(int page_id)
	{
		return page_fs::get_instance()->read_for_write(fid, page_id);
	}

	void mark_dirty(int page_id)
	{
		page_fs::get_instance()->mark_dirty(fid, page_id);
	}
};

#endif
