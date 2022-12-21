#include "table.h"
#include "../index/index.h"
#include "../expression/expression.h"
#include "../utils/type_cast.h"
#include "../database/dbms.h"
#include <cstdio>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>

index_manager::comparer_t get_index_comparer(int type)
{
	switch(type)
	{
		case COL_TYPE_INT:
			return integer_bin_comparer;
		case COL_TYPE_FLOAT:
			return float_bin_comparer;
		case COL_TYPE_VARCHAR:
			return string_comparer;
		default:
			assert(0);
			return string_comparer;
	}
}

record_manager table_manager::open_record_from_index_lower_bound(
	std::pair<int, int> idx_pos, int *rid)
{
	index_btree::leaf_page page { pg->read(idx_pos.first), pg.get() };
	int r = page.get_child(idx_pos.second);
	record_manager rm = get_record_ptr_lower_bound(r, false);
	if(rid != nullptr) rm.read(rid, 4);
	return rm;
}

void table_manager::cache_record(record_manager *rm)
{
	rm->seek(0);
	rm->read(tmp_cache, tmp_record_size);
	cache_record_from_tmp_cache();
}

void table_manager::cache_record_from_tmp_cache()
{
	expression::cache_clear(header.table_name);
	int null_mark = ((int*)tmp_cache)[1];
	for(int i = 0; i < header.col_num; ++i)
	{
		if(i == header.main_index && header.is_main_index_additional)
			continue;

		char *buf = nullptr;
		if(!((null_mark >> i) & 1))
			buf = tmp_cache + header.col_offset[i];

		expression::cache_column(
			header.table_name,
			header.col_name[i],
			typecast::column_to_expr(buf, header.col_type[i])
		);
	}
}

const char* table_manager::get_cached_column(int cid)
{
	assert(cid >= 0 && cid < header.col_num);
	int null_mark = ((int*)tmp_cache)[1];
	if(!((null_mark >> cid) & 1))
		return tmp_cache + header.col_offset[cid];
	else return nullptr;
}

void table_manager::load_indices()
{
	std::memset(indices, 0, sizeof(indices));
	for(int i = 0; i < header.col_num; ++i)
	{
		if(i != header.main_index && ((1u << i) & header.flag_indexed))
		{
			indices[i] = new index_manager(pg.get(),
				header.col_length[i],
				header.index_root[i],
				get_index_comparer(header.col_type[i])
			);
		}
	}
}

void table_manager::free_indices()
{
	for(int i = 0; i < header.col_num; ++i)
	{
		if(i != header.main_index && ((1u << i) & header.flag_indexed))
		{
			assert(indices[i]);
			header.index_root[i] = indices[i]->get_root_pid();
			delete indices[i];
			indices[i] = nullptr;
		}
	}
}

void table_manager::free_check_constraints()
{
	for(int i = 0; i != header.check_constaint_num; ++i)
	{
		expression::free_exprnode(check_conds[i]);
		check_conds[i] = nullptr;
	}
}

void table_manager::load_check_constraints()
{
	std::memset(check_conds, 0, sizeof(check_conds));
	for(int i = 0; i != header.check_constaint_num; ++i)
	{
		std::istringstream is(header.check_constaints[i]);
		check_conds[i] = expression::load_exprnode(is);
	}
}

std::shared_ptr<table_manager> table_manager::mirror(const char *alias_name)
{
	auto tb = std::make_shared<table_manager>();
	tb->tname = alias_name;
	tb->is_open = true;
	tb->is_mirror = true;
	tb->pg = pg;
	tb->btr = btr;
	tb->header = header;
	tb->allocate_temp_record();
	std::memcpy(tb->indices, indices, sizeof(indices));
	std::memcpy(tb->check_conds, check_conds, sizeof(check_conds));
	std::strcpy(tb->header.table_name, alias_name);
	return tb;
}

bool table_manager::open(const char *table_name)
{
	if(is_open) return false;
	tname = table_name;
	std::string thead = "data/" + tname + ".thead";
	std::string tdata = "data/" + tname + ".tdata";

	std::ifstream ifs(thead, std::ios::binary);
	ifs.read((char*)&header, sizeof(header));
	pg = std::make_shared<pager>(tdata.c_str());
	btr = std::make_shared<int_btree>(
			pg.get(), header.index_root[header.main_index]);
	allocate_temp_record();
	load_indices();
	load_check_constraints();

	is_mirror = false;
	return is_open = true;
}

bool table_manager::create(const char *table_name, const table_header_t *header)
{
	if(is_open) return false;
	tname = table_name;
	std::string tdata = "data/" + tname + ".tdata";

	pg = std::make_shared<pager>(tdata.c_str());
	btr = std::make_shared<int_btree>(pg.get(), 0);

	this->header = *header;
	this->header.index_root[header->main_index] = btr->get_root_page_id();
	allocate_temp_record();
	load_indices();
	load_check_constraints();

	is_mirror = false;
	return is_open = true;
}

void table_manager::drop()
{
	if(!is_open) return;
	close();
	std::string thead = "data/" + tname + ".thead";
	std::string tdata = "data/" + tname + ".tdata";
	std::remove(thead.c_str());
	std::remove(tdata.c_str());
}

void table_manager::close()
{
	if(!is_open) return;

	if(!is_mirror)
	{
		std::string thead = "data/" + tname + ".thead";
		std::string tdata = "data/" + tname + ".tdata";

		header.index_root[header.main_index] = btr->get_root_page_id();
		free_indices();
		free_check_constraints();

		std::ofstream ofs(thead, std::ios::binary);
		ofs.write((char*)&header, sizeof(header));
		pg->close();
	}

	btr = nullptr;
	pg = nullptr;
	delete []tmp_record;
	delete []tmp_cache;
	delete []tmp_index;
	tmp_cache = nullptr;
	tmp_record = nullptr;
	tmp_index = nullptr;
	is_open = false;
	is_mirror = false;
}

int table_manager::lookup_column(const char *col_name)
{
	for(int i = 0; i < header.col_num; ++i)
	{
		if(std::strcmp(col_name, header.col_name[i]) == 0)
			return i;
	}

	return -1;
}

void table_manager::allocate_temp_record()
{
	if(tmp_record) delete[] tmp_record;
	int tot_len = 4; // 4 bytes for not_null
	for(int i = 0; i < header.col_num; ++i)
		tot_len += header.col_length[i];
	tmp_record = new char[tmp_record_size = tot_len];
	tmp_cache = new char[tot_len];
	tmp_index = new char[tot_len];
	tmp_null_mark = reinterpret_cast<int*>(tmp_record + 4);
}

bool table_manager::set_temp_record(int col, const void *data)
{
	if(data == nullptr)
	{
		*tmp_null_mark |= 1u << col;
		return true;
	}

	*tmp_null_mark &= ~(1u << col);
	switch(header.col_type[col])
	{
		case COL_TYPE_INT:
		case COL_TYPE_FLOAT:
		case COL_TYPE_DATE:
			memcpy(tmp_record + header.col_offset[col], data, 4);
			break;
		case COL_TYPE_VARCHAR:
			strncpy(tmp_record + header.col_offset[col], (const char*)data, header.col_length[col]);
			break;
		default:
			assert(false);
	}

	return true;
}

void table_manager::init_temp_record()
{
	std::memset(tmp_record, 0, tmp_record_size);
	*tmp_null_mark = (1u << header.col_num) - 1;
	*tmp_null_mark &= ~(1u << header.main_index);
	*tmp_null_mark &= ~header.flag_default;
	for(int i = 0; i != header.col_num; ++i)
	{
		if(header.flag_default & (1u << i))
		{
			std::memcpy(
				tmp_record + header.col_offset[i],
				header.default_values[i], 
				header.col_length[i]
			);
		}
	}
}

int table_manager::insert_record()
{
	assert(header.col_offset[header.main_index] == 0);
	int *rid = (int*)tmp_record;
	if(header.is_main_index_additional)
		*rid = header.auto_inc;

	if(header.check_constaint_num != 0)
	{
		std::memcpy(tmp_cache, tmp_record, tmp_record_size);
		cache_record_from_tmp_cache();
	}

	if(!check_constraints(tmp_record))
		return false;

	btr->insert(*rid, tmp_record, tmp_record_size);

	for(int i = 0; i < header.col_num; ++i)
	{
		if(i != header.main_index && ((1u << i) & header.flag_indexed))
		{
			assert(indices[i]);
			if(*tmp_null_mark & (1u << i))
				indices[i]->insert(nullptr, *rid);
			else indices[i]->insert(tmp_record + header.col_offset[i], *rid);
		}
	}

	if(header.is_main_index_additional)
	{
		++header.records_num;
		++header.auto_inc;
	}
	return *rid;
}

bool table_manager::remove_record(int rid)
{
	assert(!is_mirror);
	record_manager rm = get_record_ptr(rid);
	if(rm.valid())
	{
		int null_mark;
		rm.seek(4);
		rm.read(&null_mark, 4);
		for(int i = 0; i < header.col_num; ++i)
		{
			if(i != header.main_index && ((1u << i) & header.flag_indexed))
			{
				assert(indices[i]);
				if(!((null_mark >> i) & 1))
				{
					rm.seek(header.col_offset[i]);
					rm.read(tmp_index, header.col_length[i]);
					indices[i]->erase(tmp_index, rid);
				} else indices[i]->erase(nullptr, rid);
			}
		}
		btr->erase(rid);
		return true;
	} else return false;
}

btree_iterator<int_btree::leaf_page> table_manager::get_record_iterator_lower_bound(int rid)
{
	auto ret = btr->lower_bound(rid);
	return { pg.get(), ret.first, ret.second };
}

record_manager table_manager::get_record_ptr_lower_bound(int rid, bool dirty)
{
	auto ret = btr->lower_bound(rid);
	record_manager r(pg.get());
	r.open(ret.first, ret.second, dirty);
	return r;
}

record_manager table_manager::get_record_ptr(int rid, bool dirty)
{
	auto r = get_record_ptr_lower_bound(rid, dirty);
	if(r.valid() && *(int*)r.ptr().first == rid)
		return r;
	else return record_manager(pg.get());
}

void table_manager::dump_header(FILE *f, std::vector<std::string>& heads)
{
	for(int i = 0; i < header.col_num - 1; ++i)
	{
		if(i != 0) { 
			std::fprintf(f, ","); 
			printf(",");
		}
		std::fprintf(f, "%s.%s", header.table_name, header.col_name[i]);
		printf("%s.%s", header.table_name, header.col_name[i]);
		heads.insert(heads.begin(), std::string(header.col_name[i]));
	}
}

void table_manager::dump_record(FILE *f, int rid, std::vector<std::string>& row_)
{
	record_manager rec = get_record_ptr(rid);
	dump_record(f, &rec, row_);
}

void table_manager::dump_record(FILE *f, record_manager *rm, std::vector<std::string>& row_)
{
	rm->seek(0);
	rm->read(tmp_cache, tmp_record_size);
	int null_mark = ((int*)tmp_cache)[1];
	for(int i = 0; i < header.col_num - 1; ++i)
	{
		if(i != 0) {
			std::fprintf(f, ",");
			printf(",");
		}
		if(null_mark & (1u << i))
		{
			std::fprintf(f, "NULL");
			printf("NULL");
			row_.insert(row_.begin(), "NULL");
			continue;
		}

		char *buf = tmp_cache + header.col_offset[i];
		switch(header.col_type[i])
		{
			case COL_TYPE_INT:
				std::fprintf(f, "%d", *(int*)buf);
				printf("%d", *(int*)buf);
				row_.insert(row_.begin(), std::to_string(*(int*)buf));
				break;
			case COL_TYPE_FLOAT:
				std::fprintf(f, "%f", *(float*)buf);
				printf("%f", *(float*)buf);
				row_.insert(row_.begin(), std::to_string(*(float*)buf));
				break;
			case COL_TYPE_VARCHAR:
				std::fprintf(f, "%s", buf);
				printf("%s", buf);
				row_.insert(row_.begin(), std::string(buf));
				break;
			case COL_TYPE_DATE: {
				char date_buf[32];
				time_t time = *(int*)buf;
				auto tm = std::localtime(&time);
				std::strftime(date_buf, 32, DATE_TEMPLATE, tm);
				std::fprintf(f, "%s", date_buf);
				printf("%s", date_buf);
				row_.insert(row_.begin(), std::string(date_buf));
				break;
				}
			default:
				debug_puts("[Error] Data type not supported!");
		}
	}
}

bool table_manager::modify_record(int rid, int col, const void* data)
{
	assert(!is_mirror);
	record_manager rec = get_record_ptr(rid, true);
	if(!rec.valid()) return false;
	assert(col >= 0 && col < header.col_num);

	// record must be cached by cache_record()
	int is_old_record_null = ((int*)tmp_cache)[1] & (1u << col);
	if(data == nullptr)
	{
		((int*)tmp_cache)[1] |= 1u << col;
	} else {
		std::memcpy(tmp_record, tmp_cache + header.col_offset[col], header.col_length[col]);
		std::memcpy(tmp_cache + header.col_offset[col], data, header.col_length[col]);
	}

	expression::cache_replace(
		header.table_name,
		header.col_name[col], 
		typecast::column_to_expr((char*)data, header.col_type[col])
	);

	if(!check_constraints(tmp_cache))
		return false;

	if(data != nullptr)
	{
		rec.seek(header.col_offset[col]);
		rec.write(data, header.col_length[col]);
	} else {
		rec.seek(4);
		rec.write(tmp_cache + 4, 4);
	}

	if(indices[col] != nullptr)
	{
		// update index
		if(is_old_record_null)
			indices[col]->erase(nullptr, rid);
		else indices[col]->erase(tmp_record, rid);
		indices[col]->insert((const char*)data, rid);
	}
	return true;
}

index_manager* table_manager::get_index(int cid)
{
	assert(cid >= 0 && cid < header.col_num);
	return indices[cid];
}

bool table_manager::has_index(const char *col_name)
{
	int cid = lookup_column(col_name);
	return has_index(cid);
}

bool table_manager::has_index(int cid)
{
	assert(cid >= 0 && cid < header.col_num);
	return (header.flag_indexed >> cid) & 1u;
}

void table_manager::create_index(const char *col_name)
{
	int cid = lookup_column(col_name);
	if(cid < 0)
	{
		std::fprintf(stderr, "[Error] column `%s' not exists.\n", col_name);
	} else if(has_index(cid)) {
		std::fprintf(stderr, "[Error] index for column `%s' already exists.\n", col_name);
	} else {
		header.flag_indexed |= 1u << cid;
		indices[cid] = new index_manager(pg.get(),
			header.col_length[cid],
			header.index_root[cid],
			get_index_comparer(header.col_type[cid])
		);

		// TODO: add existed data.
	}
}

bool table_manager::check_constraints(const char *buf)
{
	if(!check_notnull(buf))
		return false;

	int main_index_col = 1u << header.main_index;
	if(!(header.flag_primary & main_index_col))
	{
		// primary key is not __rowid__
		if(!check_primary(buf))
			return false;
	}

	int unique_to_check = header.flag_unique & ~main_index_col;
	for(int i = 0; i != header.col_num; ++i)
	{
		if(unique_to_check & (1u << i))
			if(!check_unique(buf, i))
			{
				std::fprintf(stderr, "[Error] Record not unique!\n");
				return false;
			}
	}

	for(int i = 0; i != header.check_constaint_num; ++i)
	{
		if(!check_value_constraint(check_conds[i]))
		{
			std::fprintf(stderr, "[Error] Value constraint broken!\n");
			return false;
		}
	}

	for(int i = 0; i != header.foreign_key_num; ++i)
	{
		if(!check_foreign(buf, i))
		{
			std::fprintf(stderr, "[Error] Foreign key constraint broken!\n");
			return false;
		}
	}

	return true;
}

bool table_manager::check_foreign(const char *buf, int key_id)
{
	int cid = header.foreign_key[key_id];
	return dbms::get_instance()->value_exists(
		header.foreign_key_ref_table[key_id],
		header.foreign_key_ref_column[key_id],
		buf + header.col_offset[cid]
	);
}

bool table_manager::check_unique(const char *buf, int col)
{
	assert(indices[col]);
	auto it = indices[col]->get_iterator_lower_bound(buf + header.col_offset[col]);
	if(it.is_end())
		return true;

	int dest_rid;
	record_manager r = open_record_from_index_lower_bound(it.get(), &dest_rid);
	if(dest_rid == *(int*)buf)
	{
		it.next();
		if(it.is_end()) return true;
		r = open_record_from_index_lower_bound(it.get(), &dest_rid);
	}

	// compare values 
	r.seek(header.col_offset[col]);
	r.read(tmp_index, header.col_length[col]);
	auto comparer = get_index_comparer(header.col_type[col]);
	return comparer(tmp_index, buf + header.col_offset[col]) != 0;
}

bool table_manager::check_primary(const char *buf)
{
	int first_primary = 0;
	while(!(header.flag_primary & (1u << first_primary)))
		++first_primary;

	assert(indices[first_primary]);
	auto it = indices[first_primary]->get_iterator_lower_bound(
			buf + header.col_offset[first_primary]);

	for(; !it.is_end(); it.next())
	{
		int rid;
		record_manager rm = open_record_from_index_lower_bound(it.get(), &rid);
		if(rid == *(int*)buf)
			continue;

		int first_not_conflicted = -1;
		for(int i = 0; i != header.col_num; ++i)
		{
			if(!(header.flag_primary & (1u << i)))
				continue;
			rm.seek(header.col_offset[i]);
			rm.read(tmp_index, header.col_length[i]);
			auto comparer = get_index_comparer(header.col_type[i]);
			if(comparer(tmp_index, buf + header.col_offset[i]) != 0)
			{
				first_not_conflicted = i;
				break;
			}
		}

		if(first_not_conflicted == -1)
		{
			std::fprintf(stderr, "[Error] Primary key confliction with __rowid__ = %d\n", rid);
			return false;
		} else if(first_not_conflicted == first_primary) {
			return true;
		}
	}

	return true;
}

bool table_manager::check_notnull(const char *buf)
{
	int null_mark = ((int*)buf)[1];
	if(null_mark & header.flag_notnull)
		return false;
	return true;
}

bool table_manager::check_value_constraint(const expr_node_t *expr)
{
	try {
		return typecast::expr_to_bool(expression::eval(expr));
	} catch(const char *msg) {
		std::puts(msg);
		return false;
	}
}

bool table_manager::value_exists(const char *column, const char *key)
{
	int cid = lookup_column(column);
	if(cid < 0)
	{
		std::printf("[Error] No column named `%s` in `%s`\n",
				column, header.table_name);
		return false;
	}

	index_manager *idx = indices[cid];
	if(!idx) 
	{
		std::printf("[Error] No index for column `%s` in table `%s`\n",
				column, header.table_name);
		return false;
	}

	auto it = idx->get_iterator_lower_bound(key);
	if(it.is_end()) return false;
	record_manager rm = open_record_from_index_lower_bound(it.get());
	auto comparer = get_index_comparer(get_column_type(cid));
	rm.seek(header.col_offset[cid]);
	rm.read(tmp_index, header.col_length[cid]);
	return comparer(key, tmp_index) == 0;
}
