#include "database.h"
#include <fstream>
#include <string>
#include <cstring>
#include <cstdio>
#include <iostream>

database::database() : opened(false), tab_count(0)
{
}

database::~database()
{
	if(is_opened()) close();
}

void database::open(const char *db_name)
{
	assert(!is_opened());
	std::string filename = "data/" + std::string(db_name);
	filename += ".database";
	std::ifstream ifs(filename, std::ios::binary);
	ifs.read((char*)&info, sizeof(info));
	std::memset(tables, 0, sizeof(tables));
	for(int i = 0; i < info.table_num; ++i)
	{
		tables[i] = new table_manager;
		tables[i]->open(info.table_name[i]);
	}
	opened = true;
}

void database::create(const char *db_name)
{
	assert(!is_opened());
	std::memset(&info, 0, sizeof(info));
	std::memset(tables, 0, sizeof(tables));
	std::strncpy(info.db_name, db_name, MAX_NAME_LEN);
	opened = true;
	printf("OK!\n");
}

void database::close()
{
	assert(is_opened());
	for(table_manager *tb : tables)
	{
		if(tb != nullptr)
		{
			tb->close();
			delete tb;
			tb = nullptr;
		}
	}

	std::string filename =  "data/" + std::string(info.db_name);
	filename += ".database";
	std::ofstream ofs(filename, std::ios::binary);
	ofs.write((char*)&info, sizeof(info));
	opened = false;
}

void database::create_table(const table_header_t *header)
{
	if(!is_opened())
	{
		std::fprintf(stderr, "[Error] database not opened.\n");
	} else if(get_table(header->table_name)) {
		std::fprintf(stderr, "[Error] table `%s` already exists.\n", header->table_name);
	} else {
		int id = info.table_num++;
		std::cout << "tab num " << std::to_string(info.table_num) << std::endl;
		std::strncpy(info.table_name[id], header->table_name, MAX_NAME_LEN);
		tables[id] = new table_manager;
		tables[id]->create(header->table_name, header);
		tab_count += 1;
	}
	printf("OK!\n");
}

void database::drop()
{
	assert(is_opened());
	for(int i = 0; i != info.table_num; ++i)
	{
		tables[i]->drop();
		delete tables[i];
		tables[i] = nullptr;
	}

	info.table_num = 0;
	std::string filename = "data/" + std::string(info.db_name);
	filename += ".database";
	close();
	std::remove(filename.c_str());
	printf("OK!\n");
}

table_manager* database::get_table(const char *name)
{
	assert(is_opened());
	int id = get_table_id(name);
	return id >= 0 ? tables[id] : nullptr;
}

table_manager* database::get_table(int id)
{
	assert(is_opened());
	if(id >= 0 && id < info.table_num)
		return tables[id];
	else return nullptr;
}

int database::get_table_id(const char *name)
{
	assert(is_opened());
	for(int i = 0; i < info.table_num; ++i)
	{
		if(std::strcmp(name, info.table_name[i]) == 0)
			return i;
	}

	return -1;
}

void database::drop_table(const char *name)
{
	assert(is_opened());
	int id = get_table_id(name);
	if(id < 0)
	{
		std::fprintf(stderr, "[Error] DROP TABLE: table `%s` not found!\n", name);
		return;
	}

	--info.table_num;
	tables[id]->drop();
	delete tables[id];
	for(int i = id; i < info.table_num; ++i)
	{
		tables[i] = tables[i + 1];
		std::strcpy(info.table_name[i], info.table_name[i + 1]);
	}

	tables[info.table_num] = nullptr;
	printf("OK!\n");
}

void database::show_info()
{
	std::printf("======== Database Info Begin ========\n");
	std::printf("Database name = %s\n", info.db_name);
	std::printf("Table number  = %d\n", tab_count);
	for(int i = 0; i != info.table_num; ++i)
		std::printf("  [table] name = %s\n", info.table_name[i]);
	std::printf("======== Database Info End   ========\n");
}
