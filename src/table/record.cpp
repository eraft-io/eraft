#include "record.h"
#include "table.h"
#include "../page/overflow_page.h"
#include <cstring>

void record_manager::open(int pid, int pos, bool dirty)
{
	this->pid = pid;
	this->pos = pos;
	this->cur_pid = pid;
	this->dirty = dirty;
	this->offset = 0;

	if(pid)
	{
		data_page<int> page { dirty ? pg->read_for_write(pid) : pg->read(pid), pg };
		auto block = page.get_block(pos);
		remain = block.first.size - sizeof(data_page<int>::block_header);
		next_pid = block.first.ov_page;
		cur_buf = block.second;
	}
}

record_manager& record_manager::seek(int offset)
{
	if(offset >= this->offset)
	{
		forward(offset - this->offset);
	} else {
		this->cur_pid = pid;
		this->offset = 0;
		data_page<int> page { dirty ? pg->read_for_write(pid) : pg->read(pid), pg };
		auto block = page.get_block(pos);
		remain = block.first.size - sizeof(data_page<int>::block_header);
		next_pid = block.first.ov_page;
		cur_buf = block.second;
		forward(offset);
	}

	return *this;
}

record_manager& record_manager::write(const void *b, int size)
{
	const char *data = (const char*)b;
	while(size)
	{
		int l = size < remain ? size : remain;
		std::memcpy(cur_buf, data, l);
		if(!dirty) pg->mark_dirty(cur_pid);
		data += l;
		size -= l;
		forward(l);
	}

	return *this;
}

record_manager& record_manager::read(void *b, int size)
{
	char *data = (char*)b;
	while(size)
	{
		int l = size < remain ? size : remain;
		std::memcpy(data, cur_buf, l);
		data += l;
		size -= l;
		forward(l);
	}

	return *this;
}

record_manager& record_manager::forward(int size)
{
	remain -= size;
	offset += size;
	cur_buf += size;
	while(remain <= 0 && next_pid)
	{
		overflow_page page { dirty ? pg->read_for_write(next_pid) : pg->read(next_pid), pg };
		remain += page.size();
		cur_buf = page.block() + (page.size() - remain);
		cur_pid = next_pid;
		next_pid = page.next();
	}

	return *this;
}

bool record_manager::forward_page()
{
	if(next_pid)
	{
		forward(remain);
		return true;
	} else return false;
}
