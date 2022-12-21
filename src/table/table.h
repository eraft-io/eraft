#ifndef __TRIVIALDB_TABLE__
#define __TRIVIALDB_TABLE__

#include <stdint.h>
#include <fstream>
#include <memory>
#include <vector>

#include "../defs.h"
#include "../btree/btree.h"
#include "../btree/iterator.h"
#include "../index/index.h"
#include "table_header.h"
#include "record.h"

/*    Data page structure for rows
 *  | rid (main index) | notnull | fixed col 1 | ... | fixed col n |
 */

struct expr_node_t;
class table_manager
{
	bool is_open, is_mirror;
	table_header_t header;
	std::shared_ptr<int_btree> btr;
	std::shared_ptr<pager> pg;
	std::string tname;
	index_manager *indices[MAX_COL_NUM];
	expr_node_t *check_conds[MAX_CHECK_CONSTRAINT_NUM];
	const char *error_msg;

	int tmp_record_size;
	char *tmp_record;
	char *tmp_cache, *tmp_index;
	int *tmp_null_mark;
	void allocate_temp_record();
	void load_indices();
	void free_indices();
	void load_check_constraints();
	void free_check_constraints();
public:
	table_manager() : is_open(false), tmp_record(nullptr) { }
	~table_manager() { if(is_open) close(); }
	bool create(const char *table_name, const table_header_t *header);
	bool open(const char *table_name);
	void drop();
	void close();
	std::shared_ptr<table_manager> mirror(const char *alias_name);

	int lookup_column(const char *col_name);
	int get_column_offset(int col) { return header.col_offset[col]; }
	int get_column_length(int col) { return header.col_length[col]; }
	const char* get_column_name(int col) { return header.col_name[col]; }
	uint8_t get_column_type(int col) { return header.col_type[col]; }
	int get_column_num() { return header.col_num; }
	const char *get_table_name() { return header.table_name; }
	void dump_table_info() { header.dump(); }

	void init_temp_record();
	int insert_record();
	bool remove_record(int rid);
	bool modify_record(int rid, int col, const void* data);
	bool set_temp_record(int col, const void* data);

	void cache_record(record_manager *rm);
	const char* get_cached_column(int cid);

	void create_index(const char *col_name);
	bool has_index(const char *col_name);
	bool has_index(int cid);
	index_manager *get_index(int cid);
	record_manager open_record_from_index_lower_bound(std::pair<int, int> idx_pos, int *rid = nullptr);
	bool value_exists(const char *column, const char *key);

	// get the record R such that R.rid = min_{r.rid >= rid} r.rid
	record_manager get_record_ptr_lower_bound(int rid, bool dirty=false);
	btree_iterator<int_btree::leaf_page> get_record_iterator_lower_bound(int rid);
	// get the record R such that R.rid = rid
	record_manager get_record_ptr(int rid, bool dirty=false);

	void dump_header(FILE *f, std::vector<std::string>& heads);
	void dump_record(FILE *f, int rid, std::vector<std::string>& row_);
	void dump_record(FILE *f, record_manager *rm, std::vector<std::string>& row_);

private:
	bool check_constraints(const char *buf);
	bool check_unique(const char *buf, int col);
	bool check_primary(const char *buf);
	bool check_foreign(const char *buf, int key_id);
	bool check_notnull(const char *buf);
	bool check_value_constraint(const expr_node_t *expr);
	void cache_record_from_tmp_cache();
};

#endif
