#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include <string>

parser_result_t result;

void parser_switch_output(const char *output_filename)
{
	result.type = SQL_SWITCH_OUTPUT;
	result.param = (void*)output_filename;
}

void parser_create_table(const table_def_t *table)
{
	result.type = SQL_CREATE_TABLE;
	result.param = (void*)table;
}

void parser_create_database(const char *db_name)
{
	result.type = SQL_CREATE_DATABASE;
	result.param = (void *)db_name;
}

void parser_use_database(const char *db_name)
{
	result.type = SQL_USE_DATABASE;
	result.param = (void*)db_name;
}

void parser_drop_database(const char *db_name)
{
	result.type = SQL_DROP_DATABASE;
	result.param = (void*)db_name;
}

void parser_show_database(const char *db_name)
{
	result.type = SQL_SHOW_DATABASE;
	result.param = (void*)db_name;
}

void parser_drop_table(const char *table_name)
{
	result.type = SQL_DROP_TABLE;
	result.param = (void*)table_name;
}

void parser_show_table(const char *table_name)
{
	result.type = SQL_SHOW_TABLE;
	result.param = (void*)table_name;
}

void parser_insert(const insert_info_t *insert_info)
{
	result.type = SQL_INSERT;
	result.param = (void*)insert_info;
}

void parser_delete(const delete_info_t *delete_info)
{
	result.type = SQL_DELETE;
	result.param = (void *)delete_info;
}

void parser_select(const select_info_t *select_info)
{
	result.type = SQL_SELECT;
	result.param = (void *)select_info;
}

void parser_update(const update_info_t *update_info)
{
	result.type = SQL_UPDATE;
	result.param = (void *)update_info;
}

void parser_create_index(const char *table_name, const char *col_name)
{
	std::string param = std::string(table_name) + "$" + std::string(col_name);
	result.type = SQL_CREATE_INDEX;
	result.param = (void *)param.c_str();
}

void parser_drop_index(const char *table_name, const char *col_name)
{
	std::string param = std::string(table_name) + "$" + std::string(col_name);
	result.type = SQL_DROP_INDEX;
	result.param =  (void *)param.c_str();
}

void parser_quit()
{}
