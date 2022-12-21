#ifndef __TRIVIALDB_FID_MANAGER__
#define __TRIVIALDB_FID_MANAGER__

#include <cassert>
#include <cstring>

#include "../defs.h"

class fid_manager
{
	int free_num, free[MAX_FILE_ID];
	char used[MAX_FILE_ID + 1];
public:
	fid_manager()
	{
		free_num = MAX_FILE_ID;
		for(int i = 0; i != MAX_FILE_ID; ++i)
			free[i] = MAX_FILE_ID - i;
		std::memset(used, 0, sizeof(used));
	}

	int allocate()
	{
		if(free_num)
		{
			int id = free[--free_num];
			used[id] = 1;
			return id;
		} else return 0;
	}

	void deallocate(int fid)
	{
		assert(1 <= fid && fid <= MAX_FILE_ID);

		if(used[fid])
			free[free_num++] = fid;
		used[fid] = 0;
	}

	bool is_used(int fid) const
	{
		return used[fid];
	}
};

#endif
