#ifndef __TRIVIALDB_TABLE_HEADER__
#define __TRIVIALDB_TABLE_HEADER__
#include "../defs.h"
#include <stdint.h>
#include "../parser/defs.h"


struct table_header_t
{
	// the number of columns (fixed and variant)
	uint8_t col_num;
	// main index for this table
	uint8_t main_index, is_main_index_additional;

	int records_num, primary_key_num, check_constaint_num, foreign_key_num;
	uint32_t flag_notnull, flag_primary, flag_indexed, flag_unique, flag_default;
	uint8_t col_type[MAX_COL_NUM];

	// the length of columns
	int col_length[MAX_COL_NUM];
	// the offset of columns
	int col_offset[MAX_COL_NUM];
	// root page of index, 0 if no index
	int index_root[MAX_COL_NUM];
	// auto increment counter
	int64_t auto_inc;

	char check_constaints[MAX_CHECK_CONSTRAINT_NUM][MAX_CHECK_CONSTRAINT_LEN];
	int foreign_key[MAX_COL_NUM];
	char default_values[MAX_COL_NUM][MAX_DEFAULT_LEN];
	char foreign_key_ref_table[MAX_COL_NUM][MAX_NAME_LEN];
	char foreign_key_ref_column[MAX_COL_NUM][MAX_NAME_LEN];
	char col_name[MAX_COL_NUM][MAX_NAME_LEN];
	char table_name[MAX_NAME_LEN];

	void dump();
};

bool fill_table_header(table_header_t *header, const table_def_t *table);

#endif
