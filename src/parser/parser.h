#ifndef __TRIVIALDB_PARSER__
#define __TRIVIALDB_PARSER__

#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif

extern parser_result_t result;

void parser_create_database(const char *db_name);
void parser_use_database(const char *db_name);
void parser_drop_database(const char *db_name);
void parser_show_database(const char *db_name);
void parser_create_table(const table_def_t *table);
void parser_drop_table(const char *table_name);
void parser_show_table(const char *table_name);
void parser_insert(const insert_info_t *insert_info);
void parser_delete(const delete_info_t *delete_info);
void parser_select(const select_info_t *select_info);
void parser_update(const update_info_t *update_info);
void parser_create_index(const char *table_name, const char *col_name);
void parser_drop_index(const char *table_name, const char *col_name);
void parser_switch_output(const char *output_filename);
void parser_quit();

#ifdef __cplusplus
}
#endif

#endif
